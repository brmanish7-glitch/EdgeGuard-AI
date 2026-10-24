#include <Wire.h>

#include <Adafruit_GFX.h>

#include <Adafruit_SH110X.h>

#include <OneWire.h>

#include <DallasTemperature.h>

#include <math.h>

// =====================================================

// OLED

// =====================================================

Adafruit_SH1106G display(128, 64, &Wire, -1);

// =====================================================

// DS18B20

// =====================================================

#define ONE_WIRE_BUS 13

OneWire oneWire(ONE_WIRE_BUS);

DallasTemperature sensors(&oneWire);

float temperature = 0;

unsigned long temperatureRequestTime = 0;

bool temperatureConversionRunning = false;

// =====================================================

// MPU6050

// =====================================================

#define MPU_ADDR 0x68

float previousMagnitude = 0;

float vibration = 0;

#define VIBRATION_SAMPLES 5
float vibrationSamples[VIBRATION_SAMPLES] = {0};
int vibrationSampleIndex = 0;
float vibrationSum = 0;

// =====================================================

// VIBRATION THRESHOLDS

// =====================================================
#define VIBRATION_DANGER 1500// =====================================================

// ACS712 CURRENT SENSOR

// =====================================================

#define CURRENT_PIN 34

#define ACS_SENSITIVITY 0.185

float ACS_ZERO_VOLTAGE = 0;

float current = 0;

float currentMagnitude = 0;

// Current thresholds

#define CURRENT_WARNING 2.5

#define CURRENT_DANGER 4.0

// =====================================================

// VOLTAGE SENSOR

// =====================================================

#define VOLTAGE_PIN 35

#define VOLTAGE_CALIBRATION 3.87

float inputVoltage = 0;

// Voltage thresholds

#define VOLTAGE_WARNING_LOW 10.0

#define VOLTAGE_WARNING_HIGH 13.0

#define VOLTAGE_DANGER_HIGH 14.0

// =====================================================

// HALL SENSOR / RPM

// =====================================================

#define HALL_PIN 32

#define HALL_MIN_SIGNAL 100
#define RPM_MIN_PULSE_INTERVAL 20
#define RPM_AVERAGE_WINDOW 60000UL

bool hallArmed = true;
unsigned long lastPulseTime = 0;
unsigned long pulseCount = 0;
float rpm = 0;

unsigned long lastPulseDetectedTime = 0;

int hallMin = 4095;
int hallMax = 0;
int hallThreshold = 2200;
int hallRearmThreshold = 2050;
unsigned long hallLearningStart = 0;

// =====================================================
// ONE-MINUTE RPM AVERAGE
// =====================================================

unsigned long rpmMeasurementStart = 0;
unsigned long startingPulseCount = 0;
float oneMinuteAverageRPM = 0;

// =====================================================
// RELAY
// =====================================================


// =====================================================

#define RELAY_PIN 25

// ACTIVE LOW

// HIGH = OFF

// LOW  = ON

// =====================================================

// L298N MOTOR DRIVER

// =====================================================

#define IN3 26

#define IN4 27

// =====================================================

// LEDs

// =====================================================

#define GREEN_LED 33

#define YELLOW_LED 19

#define RED_LED 23

// =====================================================

// BUZZER

// =====================================================

#define BUZZER_PIN 18

// =====================================================

// OLED PAGE CONTROL

// =====================================================

unsigned long previousPage = 0;

int page = 0;

#define TOTAL_PAGES 6

// =====================================================

// DISPLAY TIMER

// =====================================================

unsigned long previousDisplay = 0;

// =====================================================

// SERIAL TIMER

// =====================================================

unsigned long previousSerial = 0;

// =====================================================

// MPU6050 WRITE

// =====================================================

void writeRegister(byte reg, byte value)

{

  Wire.beginTransmission(MPU_ADDR);

  Wire.write(reg);

  Wire.write(value);

  Wire.endTransmission();

}

// =====================================================

// MPU6050 READ

// =====================================================

int16_t read16(byte reg)

{

  Wire.beginTransmission(MPU_ADDR);

  Wire.write(reg);

  Wire.endTransmission(false);

  Wire.requestFrom(MPU_ADDR, 2);

  if (Wire.available() < 2)

  {

    return 0;

  }

  int16_t value =

      (Wire.read() << 8) | Wire.read();

  return value;

}

// =====================================================

// ACS712 CALIBRATION

// MOTOR MUST BE OFF

// =====================================================

void calibrateACS712()

{

  Serial.println();

  Serial.println("================================");

  Serial.println("       ACS712 CALIBRATION");

  Serial.println("================================");

  Serial.println("Motor is OFF");

  Serial.println("Please wait...");

  delay(1000);

  long totalADC = 0;

  for (int i = 0; i < 1000; i++)

  {

    totalADC += analogRead(CURRENT_PIN);

    delay(2);

  }

  float averageADC =

      totalADC / 1000.0;

  float pinVoltage =

      (averageADC / 4095.0) * 3.3;

  // Existing 1:2 divider

  ACS_ZERO_VOLTAGE =

      pinVoltage * 2.0;

  Serial.print("Average ADC : ");

  Serial.println(averageADC);

  Serial.print("ACS Zero    : ");

  Serial.print(ACS_ZERO_VOLTAGE, 4);

  Serial.println(" V");

  Serial.println("Calibration complete");

  Serial.println("================================");

}

// =====================================================

// READ ACS712 CURRENT

// =====================================================

float readCurrent()

{

  long totalADC = 0;

  for (int i = 0; i < 20; i++)

  {

    totalADC += analogRead(CURRENT_PIN);

  }

  float averageADC =

      totalADC / 20.0;

  float currentPinVoltage =

      (averageADC / 4095.0) * 3.3;

  float acsVoltage =

      currentPinVoltage * 2.0;

  float measuredCurrent =

      (acsVoltage - ACS_ZERO_VOLTAGE) /

      ACS_SENSITIVITY;

  if (fabs(measuredCurrent) < 0.05)

  {

    measuredCurrent = 0;

  }

  return measuredCurrent;

}

// =====================================================

// DRAW HEADER

// =====================================================

void drawHeader(const char* title)

{

  display.clearDisplay();

  display.setTextColor(SH110X_WHITE);

  // Main title

  display.setTextSize(1);

  display.setCursor(2, 0);

  display.println("EDGEGUARD AI");

  // Page title

  display.setCursor(80, 0);

  display.println(title);

  // Divider

  display.drawLine(

    0,

    10,

    127,

    10,

    SH110X_WHITE

  );

}

// =====================================================

// DRAW STATUS

// =====================================================

void drawStatus(

  int x,

  int y,

  const char* status

)

{

  display.setTextSize(1);

  display.setCursor(x, y);

  display.print("STATUS: ");

  display.println(status);

}
#include "dashboard_link.h"
// =====================================================

// SETUP

// =====================================================

void setup()

{

  Serial.begin(115200);

  // ===================================================

  // I2C

  // ===================================================

  Wire.begin(21, 22);

  // ===================================================

  // OLED

  // ===================================================

  display.begin(0x3C, true);

  display.clearDisplay();

  display.display();

  // ===================================================

  // TEMPERATURE

  // ===================================================

  sensors.begin();

  sensors.setResolution(9);

  sensors.setWaitForConversion(false);

  sensors.requestTemperatures();

  temperatureRequestTime =

      millis();

  temperatureConversionRunning =

      true;

  // ===================================================

  // MPU6050

  // ===================================================

  writeRegister(0x6B, 0x00);

  // ===================================================

  // CURRENT

  // ===================================================

  pinMode(CURRENT_PIN, INPUT);

  analogSetPinAttenuation(

    CURRENT_PIN,

    ADC_11db

  );

  // ===================================================

  // VOLTAGE

  // ===================================================

  pinMode(VOLTAGE_PIN, INPUT);

  analogSetPinAttenuation(

    VOLTAGE_PIN,

    ADC_11db

  );

  // ===================================================

  // HALL

  // ===================================================

  pinMode(HALL_PIN, INPUT);

  analogSetPinAttenuation(

    HALL_PIN,

    ADC_11db

  );

  // ===================================================

  // RELAY

  // ===================================================

  pinMode(RELAY_PIN, OUTPUT);

  // ===================================================

  // L298N

  // ===================================================

  pinMode(IN3, OUTPUT);

  pinMode(IN4, OUTPUT);

  // ===================================================

  // LEDs

  // ===================================================

  pinMode(GREEN_LED, OUTPUT);

  pinMode(YELLOW_LED, OUTPUT);

  pinMode(RED_LED, OUTPUT);

  // ===================================================

  // BUZZER

  // ===================================================

  pinMode(BUZZER_PIN, OUTPUT);

  // ===================================================

  // INITIAL OUTPUT

  // ===================================================

  digitalWrite(GREEN_LED, HIGH);

  digitalWrite(YELLOW_LED, LOW);

  digitalWrite(RED_LED, LOW);

  digitalWrite(BUZZER_PIN, LOW);

  // ===================================================

  // ⭐ WORKING MOTOR STARTUP

  // ===================================================

  Serial.println();

  Serial.println("================================");

  Serial.println("        MOTOR STARTUP");

  Serial.println("================================");

  // Relay OFF

  digitalWrite(

    RELAY_PIN,

    HIGH

  );

  // Motor driver OFF

  digitalWrite(IN3, LOW);

  digitalWrite(IN4, LOW);

  Serial.println("Relay : OFF");

  Serial.println("Motor : OFF");

  delay(2000);

  // ===================================================

  // ACS712 CALIBRATION

  // ===================================================

  calibrateACS712();

  // ===================================================

  // START RELAY

  // ===================================================

  delay(1000);

  digitalWrite(

    RELAY_PIN,

    LOW

  );

  Serial.println("Relay : ON");

  delay(1000);

  // ===================================================

  // START MOTOR

  // ===================================================

  digitalWrite(

    IN3,

    HIGH

  );

  digitalWrite(

    IN4,

    LOW

  );

  Serial.println("IN3   : HIGH");

  Serial.println("IN4   : LOW");

  Serial.println("MOTOR : ON");

  Serial.println("================================");

  // ===================================================

  // SYSTEM READY

  // ===================================================

  Serial.println();

  Serial.println("================================");

  Serial.println("       EDGEGUARD AI");

  Serial.println("       SYSTEM READY");

  Serial.println("================================");

  Serial.println("Temperature : ACTIVE");

  Serial.println("Vibration   : ACTIVE");

  Serial.println("Current     : ACTIVE");

  Serial.println("Voltage     : ACTIVE");

  Serial.println("RPM         : ACTIVE");

  Serial.println("================================");

  // ===================================================

  // RPM START

  // ===================================================

  rpmMeasurementStart = millis();
  startingPulseCount = pulseCount;
  lastPulseTime = 0;
  lastPulseDetectedTime = 0;
  rpm = 0;
  hallMin = 4095;
  hallMax = 0;
  hallThreshold = 2200;
  hallRearmThreshold = 2050;
  hallLearningStart = millis();

}

// =====================================================

// LOOP

// =====================================================

void loop()

{

  unsigned long currentTime =

      millis();
      handleDashboardCommands();

  // ===================================================

  // HALL SENSOR

  // ===================================================

  int hallValue =

      analogRead(HALL_PIN);

  // ===================================================

  // RPM PULSE DETECTION
  // Adaptive threshold learns the actual KY-024 A0 signal.
  if (hallValue < hallMin) hallMin = hallValue;
  if (hallValue > hallMax) hallMax = hallValue;

  int hallRange = hallMax - hallMin;

  if (hallRange >= HALL_MIN_SIGNAL)
  {
    hallThreshold = hallMin + (hallRange * 60) / 100;
    hallRearmThreshold = hallMin + (hallRange * 30) / 100;
  }

  if (hallArmed &&
      hallValue >= hallThreshold &&
      (lastPulseDetectedTime == 0 ||
       currentTime - lastPulseDetectedTime >= RPM_MIN_PULSE_INTERVAL))
  {
    hallArmed = false;
    lastPulseDetectedTime = currentTime;
    pulseCount++;

    if (lastPulseTime != 0)
    {
      unsigned long pulseInterval = currentTime - lastPulseTime;

      if (pulseInterval >= RPM_MIN_PULSE_INTERVAL)
      {
        rpm = 60000.0 / pulseInterval;
      }
    }

    lastPulseTime = currentTime;
  }

  if (!hallArmed && hallValue <= hallRearmThreshold)
  {
    hallArmed = true;
  }

  if (lastPulseTime != 0 &&
      currentTime - lastPulseTime > 3000)
  {
    rpm = 0;
  }

  // One magnet = one pulse = one revolution.
  if (currentTime - rpmMeasurementStart >= RPM_AVERAGE_WINDOW)
  {
    unsigned long pulsesInOneMinute =
        pulseCount - startingPulseCount;

    oneMinuteAverageRPM = (float)pulsesInOneMinute;

    startingPulseCount = pulseCount;
    rpmMeasurementStart = currentTime;
  }

  // ===================================================
  // TEMPERATURE - NON BLOCKING

  // ===================================================

  if (

      temperatureConversionRunning &&

      currentTime -

      temperatureRequestTime >= 100

     )

  {

    temperature =

        sensors.getTempCByIndex(0);

    temperatureConversionRunning =

        false;

  }

  if (!temperatureConversionRunning)

  {

    sensors.requestTemperatures();

    temperatureRequestTime =

        currentTime;

    temperatureConversionRunning =

        true;

  }

  // ===================================================

  // MPU6050

  // ===================================================

  int16_t ax =

      read16(0x3B);

  int16_t ay =

      read16(0x3D);

  int16_t az =

      read16(0x3F);

  float magnitude =

      sqrt(

        (float)ax * ax +

        (float)ay * ay +

        (float)az * az

      );

static bool firstMpu = true;
if (firstMpu) { previousMagnitude = magnitude; firstMpu = false; }
float rawVibration = fabs(magnitude - previousMagnitude);

  previousMagnitude = magnitude;

  // Smooth the raw vibration over 5 samples.
  vibrationSum -= vibrationSamples[vibrationSampleIndex];
  vibrationSamples[vibrationSampleIndex] = rawVibration;
  vibrationSum += rawVibration;

  vibrationSampleIndex++;
  if (vibrationSampleIndex >= VIBRATION_SAMPLES)
  {
    vibrationSampleIndex = 0;
  }

  vibration = vibrationSum / VIBRATION_SAMPLES;

  // ===================================================

  // CURRENT

  // ===================================================

  current =

      readCurrent();

  currentMagnitude =

      fabs(current);

  // ===================================================

  // VOLTAGE

  // ===================================================

  int voltageADC =

      analogRead(VOLTAGE_PIN);

  float sensorVoltage =

      (voltageADC / 4095.0) * 3.3;

  inputVoltage =

      sensorVoltage *

      VOLTAGE_CALIBRATION;

  if (inputVoltage < 1.0)

  {

    inputVoltage = 0;

  }

  // ===================================================

  // STATUS VARIABLES

  // ===================================================

  const char* temperatureStatus;

  const char* vibrationStatus;

  const char* currentStatus;

  const char* voltageStatus;

  // ===================================================

  // TEMPERATURE STATUS

  // ===================================================

  if (temperature < 35.0)

  {

    temperatureStatus = "SAFE";

  }

  else if (temperature <= 40.0)

  {

    temperatureStatus = "WARNING";

  }

  else

  {

    temperatureStatus = "DANGER";

  }

  // ===================================================

  // VIBRATION STATUS
  if (vibration < VIBRATION_DANGER)
  {
    vibrationStatus = "SAFE";
  }
  else
  {
    vibrationStatus = "DANGER";
  }

  // ===================================================
  // CURRENT STATUS

  // ===================================================

  if (currentMagnitude <= 2.5)

  {

    currentStatus = "SAFE";

  }

  else if (currentMagnitude <= 4.0)

  {

    currentStatus = "WARNING";

  }

  else

  {

    currentStatus = "DANGER";

  }

  // ===================================================

  // VOLTAGE STATUS

  // ===================================================

  if (inputVoltage < 10.0)

  {

    voltageStatus = "WARNING";

  }

  else if (inputVoltage <= 13.0)

  {

    voltageStatus = "SAFE";

  }

  else if (inputVoltage <= 14.0)

  {

    voltageStatus = "WARNING";

  }

  else

  {

    voltageStatus = "DANGER";

  }

  // ===================================================

  // GLOBAL STATUS

  // ===================================================

  bool danger =

      (

        strcmp(

          temperatureStatus,

          "DANGER"

        ) == 0 ||

        strcmp(

          vibrationStatus,

          "DANGER"

        ) == 0 ||

        strcmp(

          currentStatus,

          "DANGER"

        ) == 0 ||

        strcmp(

          voltageStatus,

          "DANGER"

        ) == 0

      );

  bool warning =

      (

        strcmp(

          temperatureStatus,

          "WARNING"

        ) == 0 ||

        strcmp(

          vibrationStatus,

          "WARNING"

        ) == 0 ||

        strcmp(

          currentStatus,

          "WARNING"

        ) == 0 ||

        strcmp(

          voltageStatus,

          "WARNING"

        ) == 0

      );
g_danger = danger;
sendTelemetry(temperatureStatus, vibrationStatus, currentStatus, voltageStatus,
              danger ? "DANGER" : (warning ? "WARNING" : "SAFE"), hallValue);
  // ===================================================

  // LEDs + BUZZER

  // ===================================================

  if (danger)

  {

    digitalWrite(

      GREEN_LED,

      LOW

    );

    digitalWrite(

      YELLOW_LED,

      LOW

    );

    digitalWrite(

      RED_LED,

      HIGH

    );

    digitalWrite(

      BUZZER_PIN,

      HIGH

    );

  }

  else if (warning)

  {

    digitalWrite(

      GREEN_LED,

      LOW

    );

    digitalWrite(

      YELLOW_LED,

      HIGH

    );

    digitalWrite(

      RED_LED,

      LOW

    );

    digitalWrite(

      BUZZER_PIN,

      LOW

    );

  }

  else

  {

    digitalWrite(

      GREEN_LED,

      HIGH

    );

    digitalWrite(

      YELLOW_LED,

      LOW

    );

    digitalWrite(

      RED_LED,

      LOW

    );

    digitalWrite(

      BUZZER_PIN,

      LOW

    );

  }

  // ===================================================

  // SERIAL MONITOR

  // ===================================================

  if (

      currentTime -

      previousSerial >= 1000

     )

  {

    previousSerial =

        currentTime;

    Serial.println();

    Serial.println(

      "================================"

    );

    Serial.println(

      "       EDGEGUARD AI STATUS"

    );

    Serial.println(

      "================================"

    );

    // Temperature

    Serial.print(

      "Temperature : "

    );

    Serial.print(

      temperature,

      2

    );

    Serial.print(

      " C  -->  "

    );

    Serial.println(

      temperatureStatus

    );

    // Vibration

    Serial.print(

      "Vibration   : "

    );

    Serial.print(

      vibration,

      2

    );

    Serial.print(

      "  -->  "

    );

    Serial.println(

      vibrationStatus

    );

    // Current

    Serial.print(

      "Current     : "

    );

    Serial.print(

      currentMagnitude,

      2

    );

    Serial.print(

      " A  -->  "

    );

    Serial.println(

      currentStatus

    );

    // Voltage

    Serial.print(

      "Voltage     : "

    );

    Serial.print(

      inputVoltage,

      2

    );

    Serial.print(

      " V  -->  "

    );

    Serial.println(

      voltageStatus

    );

    // Hall

    Serial.print(

      "Hall Value  : "

    );

    Serial.println(

      hallValue

    );

    Serial.print("Hall State  : ");

    Serial.println(hallArmed ? "ARMED" : "WAITING");

    Serial.print("Pulses/min  : ");

    Serial.println(pulseCount - startingPulseCount);

    // RPM

    Serial.print(

      "RPM         : "

    );

    Serial.println(

      rpm,

      1

    );

    // One-minute average

    Serial.print(

      "1 Min Avg   : "

    );

    Serial.println(

      oneMinuteAverageRPM,

      1

    );

    // Pulse count

    Serial.print(

      "Pulse Count : "

    );

    Serial.println(

      pulseCount

    );

    // Overall

    Serial.print(

      "OVERALL     : "

    );

    if (danger)

    {

      Serial.println(

        "DANGER"

      );

    }

    else if (warning)

    {

      Serial.println(

        "WARNING"

      );

    }

    else

    {

      Serial.println(

        "SAFE"

      );

    }

    Serial.println(

      "================================"

    );

  }

  // ===================================================

  // PAGE SWITCHING

  // ===================================================

  if (

      currentTime -

      previousPage >= 4000

     )

  {

    page++;

    if (page >= TOTAL_PAGES)

    {

      page = 0;

    }

    previousPage =

        currentTime;

  }

  // ===================================================

  // OLED UPDATE
  // Simple display for the 0.96-inch 128x64 OLED.

  if (currentTime - previousDisplay >= 250)
  {
    previousDisplay = currentTime;

    display.clearDisplay();
    display.setTextColor(SH110X_WHITE);

    // PAGE 0 - TEMPERATURE
    if (page == 0)
    {
      display.setTextSize(1);
      display.setCursor(46, 4);
      display.println("TEMP");

      display.setTextSize(3);
      display.setCursor(18, 23);
      display.print(temperature, 1);

      display.setTextSize(2);
      display.setCursor(94, 31);
      display.println("C");
    }

    // PAGE 1 - VIBRATION
    else if (page == 1)
    {
      display.setTextSize(1);
      display.setCursor(37, 4);
      display.println("VIBRATION");

      display.setTextSize(3);
      display.setCursor(22, 25);
      display.print(vibration, 0);
    }

    // PAGE 2 - CURRENT
    else if (page == 2)
    {
      display.setTextSize(1);
      display.setCursor(40, 4);
      display.println("CURRENT");

      display.setTextSize(3);
      display.setCursor(10, 25);
      display.print(currentMagnitude, 2);

      display.setTextSize(2);
      display.setCursor(105, 31);
      display.println("A");
    }

    // PAGE 3 - VOLTAGE
    else if (page == 3)
    {
      display.setTextSize(1);
      display.setCursor(42, 4);
      display.println("VOLTAGE");

      display.setTextSize(3);
      display.setCursor(18, 25);
      display.print(inputVoltage, 1);

      display.setTextSize(2);
      display.setCursor(94, 31);
      display.println("V");
    }

    // PAGE 4 - RPM
    else if (page == 4)
    {
      display.setTextSize(1);
      display.setCursor(52, 4);
      display.println("RPM");

      display.setTextSize(3);
      display.setCursor(17, 25);
      display.print((int)rpm);
    }

    // PAGE 5 - MACHINE STATUS
    else if (page == 5)
    {
      display.setTextSize(1);
      display.setCursor(31, 4);
      display.println("MACHINE");

      display.setTextSize(2);
      display.setCursor(30, 25);

      if (danger)
      {
        display.println("DANGER");
      }
      else if (warning)
      {
        display.println("WARNING");
      }
      else
      {
        display.println("SAFE");
      }
    }

    display.display();
  }
}
