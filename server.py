"""EdgeGuard AI live dashboard server.
Run:  python server.py                (auto-finds the ESP32 on USB)
      python server.py --port COM5    (choose the port yourself)
      python server.py --simulate     (fake data, no hardware needed)
      python server.py --list         (show serial ports)
"""
import argparse, asyncio, csv, json, math, os, random, threading, time
from contextlib import asynccontextmanager
from pathlib import Path
import serial, serial.tools.list_ports, uvicorn
from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.responses import FileResponse, JSONResponse

ap = argparse.ArgumentParser()
ap.add_argument("--port"); ap.add_argument("--baud", type=int, default=115200)
ap.add_argument("--simulate", action="store_true"); ap.add_argument("--list", action="store_true")
ap.add_argument("--pin", default=os.environ.get("EDGEGUARD_PIN"), help="PIN needed for motor ON/OFF")
ap.add_argument("--host", default="127.0.0.1"); ap.add_argument("--http", type=int, default=8000)
args = ap.parse_args()
if not args.pin:
    print("WARNING: EDGEGUARD_PIN is not set; motor-control requests will be rejected.")
if args.list:
    for p in serial.tools.list_ports.comports(): print(p.device, "-", p.description)
    raise SystemExit

HERE = Path(__file__).parent
(HERE / "logs").mkdir(exist_ok=True)
clients, hist, ack, state = set(), [], {"ev": threading.Event(), "msg": None}, {"ser": None, "loop": None, "motor": 1}
FIELDS = ["time", "temp", "vib", "cur", "volt", "rpm", "motor", "overall"]

def log_row(d):
    f = HERE / "logs" / ("readings_%s.csv" % time.strftime("%Y%m%d"))
    new = not f.exists()
    with open(f, "a", newline="") as fh:
        w = csv.writer(fh)
        if new: w.writerow(FIELDS)
        w.writerow([time.strftime("%H:%M:%S")] + [d.get(k) for k in FIELDS[1:]])

async def bcast(msg):
    for ws in list(clients):
        try: await ws.send_text(msg)
        except Exception: clients.discard(ws)

def push(d):
    d["ts"] = int(time.time() * 1000); hist.append(d); del hist[:-1800]
    if int(d["ts"] / 1000) % 2 == 0: log_row(d)           # about one CSV row per second
    if state["loop"]: asyncio.run_coroutine_threadsafe(bcast(json.dumps(d)), state["loop"])

def find_port():
    for p in serial.tools.list_ports.comports():
        if any(k in (p.description or "").upper() for k in ("CP210", "CH340", "USB SERIAL", "USB-SERIAL", "SILICON LABS")):
            return p.device

def serial_reader():
    while True:
        try:
            port = args.port or find_port()
            if not port: raise RuntimeError("no ESP32 port found (try --list)")
            s = serial.Serial(); s.port, s.baudrate, s.timeout = port, args.baud, 1
            s.dtr = s.rts = False                           # try not to reset the board on connect
            s.open(); state["ser"] = s; print("Connected to", port)
            while True:
                line = s.readline().decode(errors="ignore").strip()
                if not line.startswith("{"): continue
                try: d = json.loads(line)
                except ValueError: continue
                if "ack" in d: ack["msg"] = d; ack["ev"].set()
                else: push(d)
        except Exception as e:
            state["ser"] = None; print("Serial:", e, "- retrying in 2 s"); time.sleep(2)

def status(t, v, c, u):
    st = {"temp": "SAFE" if t < 35 else "WARNING" if t <= 40 else "DANGER",
          "vib": "SAFE" if v < 1500 else "DANGER",
          "cur": "SAFE" if c <= 2.5 else "WARNING" if c <= 4 else "DANGER",
          "volt": "WARNING" if u < 10 else "SAFE" if u <= 13 else "WARNING" if u <= 14 else "DANGER"}
    o = "DANGER" if "DANGER" in st.values() else "WARNING" if "WARNING" in st.values() else "SAFE"
    return st, o

def simulator():                                           # demo data: temperature slowly climbs, then resets
    t0 = time.time()
    while True:
        t, on = time.time() - t0, state["motor"]
        temp = 29 + (0.03 * t) % 14 if on else 29
        vib = random.gauss(450, 90) + (900 if temp > 41 else 0) if on else random.gauss(40, 10)
        cur = max(0, random.gauss(0.7, 0.2)) if on else 0
        volt = random.gauss(10.8, 0.15); rpm = random.gauss(320, 6) if on else 0
        st, o = status(temp, vib, cur, volt)
        push({"temp": round(temp, 2), "vib": round(vib, 1), "cur": round(cur, 2), "volt": round(volt, 2),
              "rpm": round(rpm, 1), "motor": on, "st": st, "overall": o})
        time.sleep(0.5)

@asynccontextmanager
async def lifespan(app):
    state["loop"] = asyncio.get_running_loop()
    threading.Thread(target=simulator if args.simulate else serial_reader, daemon=True).start()
    yield

app = FastAPI(lifespan=lifespan)

@app.get("/")
def index(): return FileResponse(HERE / "index.html")

@app.get("/api/info")
def info(): return {"simulate": args.simulate}

@app.get("/api/recent")
def recent(since: int = 0): return [d for d in hist[-900:] if d["ts"] > since]

@app.post("/api/motor")
async def motor(body: dict):
    if str(body.get("pin")) != args.pin: return JSONResponse({"ok": False, "msg": "Wrong PIN"}, 403)
    cmd = "MOTOR_ON" if body.get("action") == "on" else "MOTOR_OFF"
    if args.simulate:
        state["motor"] = int(cmd == "MOTOR_ON"); return {"ok": True, "msg": cmd + " (simulated)"}
    s = state["ser"]
    if not s: return JSONResponse({"ok": False, "msg": "ESP32 not connected"}, 503)
    ack["ev"].clear(); ack["msg"] = None
    s.write(("CMD:%s\n" % cmd).encode())
    await asyncio.get_running_loop().run_in_executor(None, ack["ev"].wait, 3)
    return ack["msg"] or {"ok": False, "msg": "No reply from ESP32"}

@app.websocket("/ws")
async def ws(ws: WebSocket):
    await ws.accept(); await ws.send_text(json.dumps({"snapshot": hist[-900:]})); clients.add(ws)
    try:
        while True: await ws.receive_text()
    except WebSocketDisconnect: clients.discard(ws)

if __name__ == "__main__":
    print("Dashboard: http://%s:%d   (%s)" % (args.host, args.http, "SIMULATION" if args.simulate else "live ESP32"))
    uvicorn.run(app, host=args.host, port=args.http, log_level="warning")
