# EdgeGuard AI — Industrial Machine Health Monitoring

An ESP32-based prototype for monitoring the operating condition of a DC motor using sensor readings and providing local status indication and relay-based motor control.

## 🎯 Project Overview

Industrial machines can experience abnormal temperature, current consumption, vibration, or rotational speed before a failure occurs. EdgeGuard AI explores how embedded sensors can monitor these parameters and help identify potentially abnormal operating conditions.

The prototype uses a DC motor to represent an industrial machine. Sensor readings are collected by an ESP32 and can be used to indicate machine status and support protective control.

## ✨ Key Features

- **Temperature monitoring:** DS18B20 temperature sensor.
- **Current monitoring:** ACS712 current sensor.
- **Motion and vibration-related measurements:** MPU6050 accelerometer and gyroscope.
- **Rotational speed monitoring:** Hall-effect sensor-based RPM measurement, where implemented.
- **Local display:** OLED for viewing sensor readings and status.
- **Motor control:** Relay-based switching and an L298N motor driver in the prototype setup.
- **Monitoring dashboard:** Intended to present machine readings and status.

> **Implementation status:** This repository currently contains the available reference and dashboard-link header files. The full firmware, dashboard source, and end-to-end integration are not included here. Features should be considered implemented only where supported by the available code and test results.

## 🧰 Hardware Components

| Component | Purpose |
|---|---|
| ESP32 | Main microcontroller |
| DC motor (12 V) | Represents the monitored machine |
| ACS712 | Measures motor current |
| DS18B20 | Measures temperature |
| MPU6050 | Measures acceleration and angular motion |
| Hall-effect sensor | RPM measurement, if integrated |
| OLED display | Local sensor-data display |
| Relay module | Motor power switching |
| L298N motor driver | Motor direction and drive control |

## ⚙️ System Workflow

1. Sensors measure selected motor operating parameters.
2. The ESP32 reads the available sensor data.
3. Readings can be displayed locally and processed against configured thresholds.
4. Abnormal readings can be used to indicate a warning or potential fault.
5. Relay-based control can stop the motor when the configured protection logic is triggered, if enabled and correctly integrated.

## 🖥️ Software and Tools

- Arduino IDE
- ESP32 Arduino framework
- C/C++
- Sensor and OLED libraries
- Dashboard integration components, where available

## 🚧 Current Limitations

- The complete firmware is not currently included in this repository.
- A full dashboard implementation is not included.
- Sensor calibration and thresholds depend on the hardware and operating conditions.
- Reliable failure prediction requires validated data and testing; sensor thresholds alone do not establish a predictive AI model.

## 🚀 Future Improvements

- Publish the complete, tested ESP32 firmware.
- Add circuit diagrams and verified pin mappings.
- Integrate a live dashboard with historical data logging.
- Validate machine-status thresholds using repeatable experiments.
- Develop and evaluate a machine-learning model using collected sensor data.
- Document test results, fault scenarios, and system limitations.

## 👨‍💻 Project Information

**Domain:** Embedded Systems, IoT, and Industrial Machine Monitoring  
**Controller:** ESP32  
**Prototype:** DC motor-based machine monitoring

---

*This README documents the project and its intended architecture. Actual implementation status should be confirmed against the available source code and hardware tests.*
