<div align="center">
  <h1>Smart Car Parking System (ESP32 IoT-Based System)</h1>
  <p><strong>Jomo Kenyatta University of Agriculture and Technology</strong></p>
  <p><strong>Course:</strong> Electronic and Computer Engineering | <strong>Unit:</strong> Microprocessor II (EEE2412)</p>
  <p><strong>Supervisor:</strong> Dr. Kihato</p>
</div>

## 📖 1. Abstract
This project presents an IoT-based smart car parking system developed using the ESP32 microcontroller. The system integrates ultrasonic sensors, infrared entry/exit detection, LED indicators, a servo-controlled gate barrier, and a web-based dashboard for real-time monitoring and control. The system provides two user roles: a general user interface for parking slot monitoring and reservation, and an operator interface for full system diagnostics, control, and calibration. The solution reduces congestion, improves parking efficiency, and enables remote management through WiFi connectivity.

## 🚀 2. Introduction
Urban areas face increasing challenges in managing parking spaces efficiently due to rising vehicle numbers and limited infrastructure. Manual parking management leads to delays, congestion, and inefficient space utilization. This system addresses these issues by introducing an automated parking management solution using embedded systems and IoT technology. The ESP32 microcontroller serves as the core processing unit, enabling sensor integration, real-time decision-making, and web-based communication.

## 🎯 3. Objectives
The main objectives of the system are:
- Automate detection of parking slot availability.
- Provide real-time parking status to users.
- Enable online reservation of parking spaces.
- Control entry and exit gates automatically and manually.
- Provide an operator dashboard for diagnostics and system control.
- Allow calibration of sensor sensitivity (threshold adjustment).
- Log system events for monitoring and debugging.

## 🏗️ 4. System Overview
The system consists of three main components:

### 4.1 Hardware Layer
- **ESP32 Microcontroller:** Main processing unit
- **4 Ultrasonic Sensors:** Slot detection
- **2 Infrared Sensors:** Entry and exit detection
- **8 LEDs:** Green and red for slot indication
- **Servo Motor:** Gate control
- **LCD Display:** Local status feedback
- **WiFi Module:** Built-in ESP32

### 4.2 Software Layer
- Arduino C++ firmware
- Embedded web server (`WebServer.h`)
- JSON-based REST API
- HTML/CSS/JavaScript dashboard interface

### 4.3 Communication Layer
- WiFi-based local network communication
- HTTP REST APIs for data exchange
- Browser-based dashboard interface

## ⚙️ 5. System Architecture
The system is divided into:

### 5.1 Input Subsystem
- Ultrasonic sensors measure distance to detect vehicle presence.
- IR sensors detect vehicle entry and exit events.

### 5.2 Processing Subsystem
- ESP32 processes sensor data.
- Determines slot status using a configurable threshold.
- Manages reservations and gate logic.

### 5.3 Output Subsystem
- LED indicators (Green = free, Red = occupied)
- Servo motor controls gate barrier
- LCD displays system status
- Web dashboard displays real-time data

## 🔬 6. Working Principle

### 6.1 Slot Detection Logic
Each parking slot is monitored using ultrasonic sensors. The detection variables follow a standardized naming convention:
- If `measured_Distance` > threshold → Slot is **FREE**
- If `measured_Distance` ≤ threshold → Slot is **OCCUPIED**
- If `reserved_Flag` is active → Slot is **RESERVED**

The threshold is dynamically adjustable:
- **Default:** 5 cm
- **Adjustable range:** 2 cm to 100 cm

*(`images/fig1.png`)* **Fig 1:** Ultrasonic sensor distance adjusting

### 6.2 Entry and Exit Control
- `ir_Entry` sensor detects incoming vehicles.
- `ir_Exit` sensor detects outgoing vehicles.

When triggered:
1. System checks parking availability.
2. Gate opens or closes using servo motor.
3. Event is logged in system memory.

*(Please place your image here: `images/fig2.png`)* **Fig 2:** System availability

### 6.3 Reservation System
**Users can:**
- Reserve available slots
- Cancel reservations

**Rules:**
- Only **FREE** slots can be reserved.
- **OCCUPIED** slots cannot be reserved.

*(Please place your image here: `images/fig3.png`)* **Fig 3:** Shows reservation slots

## 💻 7. Web-Based Dashboard
The system includes a unified web interface with two modes:

### 7.1 User Dashboard
**Features:**
- View parking slot availability
- Real-time status updates
- Reserve and cancel slots
- Parking guidance messages

**Benefits:**
- Simplified interface for general users
- No system control privileges

*(Please place your image here: `images/fig4.png`)* **Fig 4:** Shows User dashboard

### 7.2 Operator Dashboard
**Features:**
- Full system diagnostics
- Real-time sensor telemetry
- WiFi signal strength monitoring
- System uptime and memory usage
- IR sensor status monitoring
- Gate status control (open/close)
- Sensor threshold calibration
- Event log terminal
- Slot override control

*(Please place your image here: `images/fig5.png`)* **Fig 5:** Shows Operator dashboard

*(Please place your image here: `images/fig6.png`)* **Fig 6:** Ultrasonic Telemetry Matrix

## 🎛️ 8. Sensor Calibration Feature
A major enhancement in the system is the adjustable ultrasonic detection threshold.
- **Function:** Allows operator to fine-tune sensor sensitivity depending on environment conditions.
- **Use cases:**
  - Adjust for vehicle size differences
  - Adapt to installation height variations
  - Reduce false detection in noisy environments
  - Improve accuracy in outdoor conditions

## 📜 9. Data Logging System
The system maintains a rolling log of important events stored in memory and displayed on the operator dashboard. Logged events include:
- Gate opening and closing
- Slot reservation and release
- System startup
- Access denial events
- Sensor calibration changes

## 🔄 10. Communication Protocol
The system uses REST APIs. Data is exchanged in JSON format for compatibility with web dashboards.

| Endpoint | Function |
| :--- | :--- |
| `/api/user/status` | User slot status |
| `/api/operator/diagnostics` | Full system diagnostics |
| `/reserve` | Reserve slot |
| `/unreserve` | Cancel reservation |
| `/open` | Open gate |
| `/close` | Close gate |
| `/set_threshold` | Adjust sensor sensitivity |

## ✨ 11. System Features

**Core Features:**
- Real-time parking detection
- Automated gate control
- Web-based monitoring system
- Slot reservation system

**Advanced Features:**
- Dual dashboard (User + Operator)
- Live telemetry monitoring
- System diagnostics panel
- Adjustable ultrasonic threshold
- Event logging system
- WiFi-based remote access

*(Please place your image here: `images/fig7.png`)* **Fig 7:** Shows Car Parking Model

*(Please place your image here: `images/fig8.png`)* **Fig 8:** Shows gate Status and available spaces

*(Please place your image here: `images/fig9.png`)* **Fig 9:** Car entering Parking premises

## ✅ 12. Advantages
- Reduces manual parking management
- Improves space utilization efficiency
- Enables remote monitoring and control
- Scalable for large parking systems
- Real-time decision-making
- Low-cost IoT implementation
- Highly customizable and upgradeable

## ⚠️ 13. Limitations
- Requires stable WiFi connection
- Ultrasonic sensors affected by environmental noise
- Limited range of IR sensors
- Servo motor wear over long-term use
- Not cloud-connected by default (local network only)

## 🔮 14. Future Improvements
- Cloud integration (AWS / Firebase)
- Mobile application support
- AI-based parking prediction
- License plate recognition (ANPR)
- Payment system integration
- Camera-based slot verification
- MQTT-based communication upgrade
- Database storage for historical analytics

## 🏁 15. Conclusion
The smart car parking system demonstrates an effective IoT-based solution for modern parking management challenges. By combining ESP32 microcontroller technology with ultrasonic sensing, servo automation, and a web-based interface, the system achieves real-time monitoring, user interaction, and administrative control. The addition of dual dashboards, logging systems, and adjustable sensor thresholds significantly improves flexibility, scalability, and operational accuracy. This makes the system suitable for deployment in small to medium-scale parking environments with potential for future expansion into smart city infrastructure.
