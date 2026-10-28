@echo off
cd /d "%~dp0"
if not exist venv (
  py -3.12 -m venv venv
  venv\Scripts\python -m pip install -r requirements.txt
)
rem Change to:  venv\Scripts\python server.py --port COM5   if auto-detect fails
rem Use --simulate to try the dashboard without the ESP32
venv\Scripts\python server.py %*
pause
