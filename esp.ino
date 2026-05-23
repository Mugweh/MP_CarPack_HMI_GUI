#include <WebServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>

// ---------------- WIFI CONFIG ----------------
const char* ssid = "Winterfell";
const char* password = "@kyouma254";

// ---------------- HARDWARE MAPPING ----------------
WebServer server(80);
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo gateServo;

#define IR_ENTRY 19
#define IR_EXIT 34

int trigPins[4] = {5, 17, 4, 2};
int echoPins[4] = {18, 16, 0, 15};
int greenLED[4] = {26, 14, 25, 32};
int redLED[4]   = {27, 12, 33, 13};

// ---------------- STATE VARIABLES ----------------
float distance[4];
bool slotFree[4];
bool reserved[4] = {false, false, false, false};
bool gateOpen = false;

// Dynamic Threshold configuration variable
float parkingThresholdCm = 5.0; 

unsigned long lastBlink = 0;
bool blinkState = false;
bool lastEntryIR = false;
bool lastExitIR = false;
unsigned long lastIRTrigger = 0;
const int debounceDelay = 300;

// ---------------- DIAGNOSTICS & LOGGING SYSTEM ----------------
String systemLogs[5] = {"", "", "", "", ""};
int totalGateRotations = 0;

void addLog(String message) {
  for (int i = 4; i > 0; i--) {
    systemLogs[i] = systemLogs[i-1];
  }
  systemLogs[0] = "[" + String(millis() / 1000) + "s] " + message;
  Serial.println(systemLogs[0]);
}

// ---------------- SENSOR LOGIC ----------------
float readDistance(int trig, int echo) {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  long duration = pulseIn(echo, HIGH, 30000);
  return duration * 0.034 / 2;
}

bool isParkingFull() {
  for (int i = 0; i < 4; i++) {
    if (slotFree[i] || reserved[i]) return false;
  }
  return true;
}

void openGateSlow() {
  addLog("Command: Opening gate arm.");
  for (int pos = 0; pos <= 90; pos += 2) {
    gateServo.write(pos);
    delay(15);
  }
  totalGateRotations++;
}

void closeGateSlow() {
  addLog("Command: Closing gate arm.");
  for (int pos = 90; pos >= 0; pos -= 2) {
    gateServo.write(pos);
    delay(15);
  }
}

// ---------------- DATA API ENDPOINTS ----------------

void handleUserStatusAPI() {
  String json = "{";
  json += "\"slots\":[";
  for (int i = 0; i < 4; i++) {
    if (reserved[i]) json += "\"RESERVED\"";
    else json += slotFree[i] ? "\"FREE\"" : "\"OCCUPIED\"";
    if (i < 3) json += ",";
  }
  json += "],";
  
  int freeCount = 0;
  for(int i=0; i<4; i++) if(slotFree[i] && !reserved[i]) freeCount++;
  
  json += "\"guidance\":\"" + String(freeCount > 0 ? "Spaces available. Drive safely." : "Parking lot full.") + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void handleOperatorDiagnosticsAPI() {
  String json = "{";
  
  json += "\"system\":{";
  json += "\"wifi_rssi\":\"" + String(WiFi.RSSI()) + " dBm\",";
  json += "\"uptime_sec\":" + String(millis() / 1000) + ",";
  json += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
  json += "\"threshold_cm\":" + String(parkingThresholdCm);
  json += "},";
  
  json += "\"gate\":{";
  json += "\"status\":\"" + String(gateOpen ? "OPEN" : "CLOSED") + "\",";
  json += "\"ir_entry\":" + String(digitalRead(IR_ENTRY) == LOW ? "true" : "false") + ",";
  json += "\"ir_exit\":" + String(digitalRead(IR_EXIT) == LOW ? "true" : "false");
  json += "},";
  
  json += "\"telemetry\":[";
  for (int i = 0; i < 4; i++) {
    json += "{";
    json += "\"slot\":" + String(i + 1) + ",";
    json += "\"raw_cm\":" + String(distance[i]) + ",";
    json += "\"status\":\"" + String(reserved[i] ? "RESERVED" : (slotFree[i] ? "FREE" : "OCCUPIED")) + "\"";
    json += "}";
    if (i < 3) json += ",";
  }
  json += "],";
  
  json += "\"logs\":[";
  for(int i=0; i<5; i++) {
    json += "\"" + systemLogs[i] + "\"";
    if(i < 4) json += ",";
  }
  json += "]";
  
  json += "}";
  server.send(200, "application/json", json);
}

// ---------------- MUTATION ROUTERS ----------------
void handleReserve() {
  if (server.hasArg("slot")) {
    int s = server.arg("slot").toInt();
    if (s >= 0 && s < 4 && slotFree[s] && !reserved[s]) {
      reserved[s] = true;
      addLog("User booked Slot S" + String(s + 1));
      server.send(200, "text/plain", "RESERVED");
      return;
    }
  }
  server.send(400, "text/plain", "REJECTED");
}

void handleUnreserve() {
  if (server.hasArg("slot")) {
    int s = server.arg("slot").toInt();
    if (s >= 0 && s < 4) {
      reserved[s] = false;
      addLog("Slot S" + String(s + 1) + " cleared.");
      server.send(200, "text/plain", "UNRESERVED");
      return;
    }
  }
  server.send(400, "text/plain", "INVALID");
}

void handleOpen() {
  gateOpen = true;
  openGateSlow();
  addLog("Operator manually forced gate OPEN.");
  server.send(200, "text/plain", "OPENED");
}

void handleClose() {
  gateOpen = false;
  closeGateSlow();
  addLog("Operator manually forced gate CLOSED.");
  server.send(200, "text/plain", "CLOSED");
}

void handleSetThreshold() {
  if (server.hasArg("val")) {
    float newVal = server.arg("val").toFloat();
    if (newVal >= 2.0 && newVal <= 100.0) {
      parkingThresholdCm = newVal;
      addLog("Sensors re-calibrated to threshold: " + String(parkingThresholdCm) + " cm");
      server.send(200, "text/plain", "UPDATED");
      return;
    }
  }
  server.send(400, "text/plain", "REJECTED_RANGE");
}

// ---------------- UNIFIED VISUAL INTERFACE (SPA) ----------------

void handleUnifiedDashboard() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Smart Parking | Unified System</title>
    <style>
        :root { --free: #10b981; --occ: #ef4444; --res: #f59e0b; --accent: #3b82f6; --text: #f8fafc; }
        
        /* Glassmorphism Background Integration */
        body { 
            font-family: 'Segoe UI', sans-serif; 
            background: linear-gradient(rgba(11, 15, 25, 0.75), rgba(11, 15, 25, 0.85)), 
                        url('https://www.rokerinc.com/wp-content/uploads/2022/05/WhatsApp-Image-2022-05-12-at-5.27.56-PM.jpeg') no-repeat center center fixed; 
            background-size: cover;
            color: var(--text); 
            margin: 0; 
            padding: 15px; 
        }
        
        .wrapper { max-width: 800px; margin: 0 auto; }
        
        /* Toggle Switch UI with blur */
        .top-bar { display: flex; justify-content: space-between; align-items: center; background: rgba(30, 41, 59, 0.7); backdrop-filter: blur(8px); padding: 10px 20px; border-radius: 10px; margin-bottom: 20px; border: 1px solid rgba(255, 255, 255, 0.1); box-shadow: 0 4px 6px rgba(0,0,0,0.3);}
        .toggle-container { display: flex; background: rgba(15, 23, 42, 0.8); border-radius: 8px; overflow: hidden; border: 1px solid rgba(255, 255, 255, 0.1); }
        .toggle-btn { padding: 8px 16px; border: none; background: transparent; color: #94a3b8; cursor: pointer; font-weight: bold; transition: 0.3s; }
        .toggle-btn.active { background: var(--accent); color: white; }
        
        /* View Containers */
        #user-view, #operator-view { display: none; }
        #user-view.active, #operator-view.active { display: block; animation: fadeIn 0.4s ease; }
        @keyframes fadeIn { from { opacity: 0; transform: translateY(10px); } to { opacity: 1; transform: translateY(0); } }

        /* User View Styles */
        .guidance-box { background: rgba(21, 31, 50, 0.7); backdrop-filter: blur(8px); padding: 15px; border-radius: 10px; margin-bottom: 20px; border-left: 4px solid var(--accent); font-size: 1.1rem; font-weight: bold; box-shadow: 0 4px 6px rgba(0,0,0,0.3);}
        .parking-lot { display: grid; grid-template-columns: 1fr 1fr; gap: 15px; }
        .slot-card { text-align: center; background: rgba(21, 31, 50, 0.7); backdrop-filter: blur(8px); padding: 15px; border-radius: 10px; border: 1px solid rgba(255, 255, 255, 0.1); box-shadow: 0 4px 6px rgba(0,0,0,0.3);}
        .free { border-top: 3px solid var(--free); color: var(--free); }
        .occupied { border-top: 3px solid var(--occ); color: var(--occ); }
        .reserved { border-top: 3px solid var(--res); color: var(--res); }
        .btn { margin-top: 10px; width: 100%; padding: 10px; border-radius: 6px; border: none; font-weight: bold; cursor: pointer; color: white; transition: 0.2s;}
        .btn:hover { filter: brightness(1.1); }
        .btn-book { background: var(--free); }
        .btn-cancel { background: var(--res); }

        /* Operator View Styles */
        .panel { border: 1px solid rgba(255, 255, 255, 0.1); padding: 15px; background: rgba(15, 23, 42, 0.75); backdrop-filter: blur(8px); margin-bottom: 15px; border-radius: 8px; font-family: monospace; box-shadow: 0 4px 6px rgba(0,0,0,0.3);}
        .panel-title { color: var(--accent); border-bottom: 1px solid rgba(255, 255, 255, 0.1); padding-bottom: 5px; margin-bottom: 10px; font-weight: bold; text-shadow: 0 2px 4px rgba(0,0,0,0.5);}
        table { width: 100%; border-collapse: collapse; font-size: 0.9rem; margin-bottom: 15px;}
        td, th { text-align: left; padding: 8px; border-bottom: 1px solid rgba(255, 255, 255, 0.05); }
        .log-box { background: rgba(0, 0, 0, 0.6); color: #10b981; padding: 10px; height: 110px; overflow-y: auto; font-size: 0.85rem; border-radius: 5px; border: 1px dashed rgba(255, 255, 255, 0.2);}
        
        .admin-controls { display: flex; gap: 10px; margin-top: 10px; margin-bottom: 15px;}
        .admin-btn { padding: 10px 15px; border-radius: 6px; border: none; font-weight: bold; cursor: pointer; color: white; font-family: monospace; transition: 0.2s;}
        .admin-btn:hover { filter: brightness(1.2); }
        .btn-open { background: var(--free); }
        .btn-close { background: var(--occ); }
        .btn-purge { background: rgba(51, 65, 85, 0.8); color: white; border: 1px solid rgba(255,255,255,0.2); padding: 5px 10px; border-radius: 4px; cursor: pointer;}
        .btn-purge:hover { background: var(--accent); }
        
        /* Slider Styling */
        .config-row { display: flex; align-items: center; gap: 15px; padding: 10px 0; }
        .slider { flex-grow: 1; accent-color: var(--accent); cursor: pointer; }
    </style>
</head>
<body>
    <div class="wrapper">
        <div class="top-bar">
            <h3 style="margin: 0; color: var(--accent); text-shadow: 0 2px 4px rgba(0,0,0,0.5);">Smart Parking</h3>
            <div class="toggle-container">
                <button id="btn-user" class="toggle-btn active" onclick="switchView('user')">User</button>
                <button id="btn-admin" class="toggle-btn" onclick="switchView('operator')">Operator</button>
            </div>
        </div>

        <!-- ================= USER DASHBOARD ================= -->
        <div id="user-view" class="active">
            <div class="guidance-box" id="guidance-msg">Loading availability...</div>
            <div class="parking-lot" id="lot-grid"></div>
        </div>

        <!-- ================= OPERATOR DASHBOARD ================= -->
        <div id="operator-view">
            <div class="panel">
                <div class="panel-title">> SYSTEM & GATE DIAGNOSTICS</div>
                <table>
                    <tr><td>Uptime:</td><td id="sys-uptime">-</td><td>WiFi RSSI:</td><td id="sys-rssi">-</td></tr>
                    <tr><td>Gate Status:</td><td id="gate-state" style="font-weight:bold;">-</td><td>Heap Memory:</td><td id="sys-heap">-</td></tr>
                    <tr><td>IR Entry Sensor:</td><td id="ir-entry">-</td><td>IR Exit Sensor:</td><td id="ir-exit">-</td></tr>
                </table>
                
                <div class="panel-title">> MANUAL GATE OVERRIDE</div>
                <div class="admin-controls">
                    <button class="admin-btn btn-open" onclick="fetch('/open')">FORCE OPEN GATE</button>
                    <button class="admin-btn btn-close" onclick="fetch('/close')">FORCE CLOSE GATE</button>
                </div>

                <div class="panel-title">> SENSOR DETECTION DISTANCE CALIBRATION</div>
                <div class="config-row">
                    <span>Threshold:</span>
                    <input type="range" id="threshold-slider" class="slider" min="2" max="50" step="1" onchange="sendThreshold(this.value)" oninput="updateSliderLabel(this.value)">
                    <span id="threshold-val" style="color: var(--res); font-weight: bold; min-width: 50px;">- cm</span>
                </div>
            </div>

            <div class="panel">
                <div class="panel-title">> ULTRASONIC TELEMETRY MATRIX</div>
                <table>
                    <thead><tr><th>Node</th><th>Distance</th><th>Logical State</th><th>Admin Override</th></tr></thead>
                    <tbody id="telemetry-table"></tbody>
                </table>
            </div>

            <div class="panel">
                <div class="panel-title">> LIVE EVENT TERMINAL</div>
                <div class="log-box" id="log-terminal"></div>
            </div>
        </div>
    </div>

    <script>
        let currentMode = 'user';
        let userInteractingWithSlider = false;

        function switchView(mode) {
            currentMode = mode;
            document.getElementById('btn-user').classList.toggle('active', mode === 'user');
            document.getElementById('btn-admin').classList.toggle('active', mode === 'operator');
            document.getElementById('user-view').classList.toggle('active', mode === 'user');
            document.getElementById('operator-view').classList.toggle('active', mode === 'operator');
            refreshData(); 
        }

        function updateSliderLabel(val) {
            userInteractingWithSlider = true;
            document.getElementById("threshold-val").innerText = val + " cm";
        }

        function sendThreshold(val) {
            fetch(`/set_threshold?val=${val}`).then(() => {
                userInteractingWithSlider = false;
                refreshData();
            });
        }

        function refreshData() {
            if (currentMode === 'user') {
                fetch('/api/user/status').then(r=>r.json()).then(d=>{
                    document.getElementById("guidance-msg").innerText = d.guidance;
                    const grid = document.getElementById("lot-grid"); grid.innerHTML = "";
                    d.slots.forEach((s, i) => {
                        let card = document.createElement("div"); 
                        card.className = "slot-card " + s.toLowerCase();
                        card.innerHTML = `<h4 style="margin:0; color:#cbd5e1;">Slot ${i+1}</h4><h2 style="margin:10px 0; text-shadow: 0 2px 4px rgba(0,0,0,0.5);">${s}</h2>`;
                        
                        let btn = document.createElement("button");
                        if(s === "FREE"){
                            btn.className = "btn btn-book"; btn.innerText = "Reserve Spot";
                            btn.onclick = () => fetch(`/reserve?slot=${i}`).then(()=>refreshData());
                        } else if(s === "RESERVED"){
                            btn.className = "btn btn-cancel"; btn.innerText = "Cancel Reservation";
                            btn.onclick = () => fetch(`/unreserve?slot=${i}`).then(()=>refreshData());
                        } else {
                            btn.className = "btn"; btn.innerText = "Unavailable"; btn.disabled = true;
                            btn.style.background = "rgba(51, 65, 85, 0.8)";
                            btn.style.color = "#94a3b8";
                        }
                        card.appendChild(btn); grid.appendChild(card);
                    });
                });
            } else {
                fetch('/api/operator/diagnostics').then(r=>r.json()).then(d=>{
                    document.getElementById("sys-uptime").innerText = d.system.uptime_sec + "s";
                    document.getElementById("sys-rssi").innerText = d.system.wifi_rssi;
                    document.getElementById("sys-heap").innerText = d.system.free_heap + "B";
                    document.getElementById("gate-state").innerText = d.gate.status;
                    document.getElementById("gate-state").style.color = d.gate.status === "OPEN" ? "#10b981" : "#ef4444";
                    document.getElementById("ir-entry").innerText = d.gate.ir_entry ? "DETECTED" : "CLEAR";
                    document.getElementById("ir-exit").innerText = d.gate.ir_exit ? "DETECTED" : "CLEAR";
                    
                    if (!userInteractingWithSlider) {
                        document.getElementById("threshold-slider").value = d.system.threshold_cm;
                        document.getElementById("threshold-val").innerText = d.system.threshold_cm + " cm";
                    }

                    const tbody = document.getElementById("telemetry-table"); tbody.innerHTML = "";
                    d.telemetry.forEach(t => {
                        let r = document.createElement("tr");
                        let color = t.status==='FREE' ? '#10b981' : t.status==='RESERVED' ? '#f59e0b' : '#ef4444';
                        r.innerHTML = `
                            <td>S${t.slot}</td>
                            <td>${t.raw_cm.toFixed(1)} cm</td>
                            <td style="color:${color}; font-weight:bold;">${t.status}</td>
                            <td><button class="btn-purge" onclick="fetch('/unreserve?slot=${t.slot-1}')">Clear Slot</button></td>
                        `;
                        tbody.appendChild(r);
                    });
                    
                    document.getElementById("log-terminal").innerHTML = d.logs.join("<br>");
                });
            }
        }
        
        setInterval(refreshData, 1500); 
        refreshData(); 
    </script>
</body>
</html>
)rawliteral";
  server.send(200, "text/html", html);
}

// ---------------- SETUP & LOOP ----------------
void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(300);
  Serial.println(WiFi.localIP());

  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();

  pinMode(IR_ENTRY, INPUT);
  pinMode(IR_EXIT, INPUT);
  gateServo.attach(23);
  gateServo.write(0);

  for (int i = 0; i < 4; i++) {
    pinMode(trigPins[i], OUTPUT);
    pinMode(echoPins[i], INPUT);
    pinMode(greenLED[i], OUTPUT);
    pinMode(redLED[i], OUTPUT);
  }

  server.on("/", handleUnifiedDashboard);
  server.on("/api/user/status", handleUserStatusAPI);
  server.on("/api/operator/diagnostics", handleOperatorDiagnosticsAPI);
  
  server.on("/reserve", handleReserve);
  server.on("/unreserve", handleUnreserve);
  server.on("/open", handleOpen);
  server.on("/close", handleClose);
  server.on("/set_threshold", handleSetThreshold);
  
  server.begin();
  addLog("System boot initialized successfully.");
}

void loop() {
  server.handleClient();

  if (millis() - lastBlink > 500) {
    blinkState = !blinkState;
    lastBlink = millis();
  }

  int freeSlots = 0;
  for (int i = 0; i < 4; i++) {
    distance[i] = readDistance(trigPins[i], echoPins[i]);
    
    if (reserved[i]) {
      digitalWrite(greenLED[i], blinkState);
      digitalWrite(redLED[i], blinkState);
    } else if (distance[i] > parkingThresholdCm || distance[i] == 0) { 
      slotFree[i] = true;
      digitalWrite(greenLED[i], HIGH);
      digitalWrite(redLED[i], LOW);
      freeSlots++;
    } else {
      slotFree[i] = false;
      digitalWrite(greenLED[i], LOW);
      digitalWrite(redLED[i], HIGH);
    }
  }

  bool entryIR = digitalRead(IR_ENTRY) == LOW;
  bool exitIR = digitalRead(IR_EXIT) == LOW;
  unsigned long now = millis();

  if ((entryIR && !lastEntryIR) || (exitIR && !lastExitIR)) {
    if (now - lastIRTrigger > debounceDelay) {
      if (!isParkingFull() || exitIR) {
        gateOpen = !gateOpen;
        if (gateOpen) {
          addLog(entryIR ? "Auto Entry loop triggered." : "Auto Exit loop triggered.");
          openGateSlow();
        } else {
          closeGateSlow();
        }
      } else {
        addLog("Access Denied: Parking layout full.");
      }
      lastIRTrigger = now;
    }
  }
  lastEntryIR = entryIR;
  lastExitIR = exitIR;

  lcd.setCursor(0, 0);
  lcd.print("Free: "); lcd.print(freeSlots); lcd.print("/4   ");
  lcd.setCursor(0, 1);
  if (isParkingFull()) lcd.print("PARKING FULL   ");
  else lcd.print(gateOpen ? "GATE OPEN     " : "GATE CLOSED   ");
  delay(50);
}