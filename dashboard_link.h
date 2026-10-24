// EdgeGuard AI - dashboard link. Put this file in the SAME folder as your .ino sketch.
// It must be included AFTER your pin #defines and global variables (see the 4 edits).
#pragma once
bool g_danger = false;

bool motorIsOn() { return digitalRead(RELAY_PIN) == LOW && digitalRead(IN3) == HIGH && digitalRead(IN4) == LOW; }
void motorStart() { digitalWrite(RELAY_PIN, LOW); delay(200); digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW); }
void motorStop()  { digitalWrite(IN3, LOW); digitalWrite(IN4, LOW); delay(100); digitalWrite(RELAY_PIN, HIGH); }

void handleDashboardCommands() {
  static String buf;
  while (Serial.available()) {
    char c = Serial.read();
    if (c != '\n') { if (c != '\r') buf += c; continue; }
    String cmd = buf; buf = "";
    if (cmd == "CMD:MOTOR_ON") {
      if (g_danger) Serial.println("{\"ack\":\"MOTOR_ON\",\"ok\":false,\"msg\":\"Blocked: machine is in DANGER\"}");
      else { motorStart(); Serial.println("{\"ack\":\"MOTOR_ON\",\"ok\":true,\"msg\":\"Motor started\"}"); }
    } else if (cmd == "CMD:MOTOR_OFF") {
      motorStop(); Serial.println("{\"ack\":\"MOTOR_OFF\",\"ok\":true,\"msg\":\"Motor stopped\"}");
    }
  }
}

void sendTelemetry(const char* sT, const char* sV, const char* sC, const char* sU, const char* overall, int hallValue) {
  static unsigned long last = 0;
  if (millis() - last < 500) return;
  last = millis();
  char b[360];
  snprintf(b, sizeof(b),
    "{\"temp\":%.2f,\"vib\":%.1f,\"cur\":%.2f,\"volt\":%.2f,\"rpm\":%.1f,\"hall\":%d,\"motor\":%d,"
    "\"st\":{\"temp\":\"%s\",\"vib\":\"%s\",\"cur\":\"%s\",\"volt\":\"%s\"},\"overall\":\"%s\"}",
    temperature, vibration, currentMagnitude, inputVoltage, rpm, hallValue, motorIsOn() ? 1 : 0, sT, sV, sC, sU, overall);
  Serial.println(b);
}
