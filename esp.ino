#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <LittleFS.h> // Include LittleFS for file handling

// ---------------- WIFI ----------------
const char* ssid = "Winterfell";
const char* password = "@kyouma254";

// ---------------- SERVER ----------------
WebServer server(80);

// ---------------- LCD ----------------
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ---------------- SERVO ----------------
Servo gateServo;

// ---------------- IR ----------------
#define IR_ENTRY 19
#define IR_EXIT 34

// ---------------- ULTRASONIC ----------------
int trigPins[4] = {5, 17, 4, 2};
int echoPins[4] = {18, 16, 0, 15};

// ---------------- LED ----------------
int greenLED[4] = {26, 14, 25, 32};
int redLED[4]   = {27, 12, 33, 13};

// ---------------- STATE ----------------
float distance[4];
bool slotFree[4];
bool gateOpen = false;

unsigned long lastIRTrigger = 0;
const int debounceDelay = 200;

// -------- IR EDGE TRACKING --------
bool lastEntryIR = false;
bool lastExitIR = false;

// ---------------- DISTANCE ----------------
float readDistance(int trig, int echo) {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  long duration = pulseIn(echo, HIGH, 30000);
  return duration * 0.034 / 2;
}

// ---------------- JSON ----------------
void handleData() {
  String json = "{";
  json += "\"slots\":[";

  for (int i = 0; i < 4; i++) {
    json += slotFree[i] ? "\"FREE\"" : "\"OCCUPIED\"";
    if (i < 3) json += ",";
  }

  json += "],";
  json += "\"gate\":\"";
  json += (gateOpen ? "OPEN" : "CLOSED");
  json += "\"}";

  server.send(200, "application/json", json);
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);

  // Initialize LittleFS
  if (!LittleFS.begin(true)) {
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(300);

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

  // Route for root / web page - Serve the file directly from flash!
  server.serveStatic("/", LittleFS, "/index.html");

  // Route for the CSS file!
  server.serveStatic("/style.css", LittleFS, "/style.css");

  // Route for the JavaScript file!
  server.serveStatic("/script.js", LittleFS, "/script.js");

  server.on("/data", handleData);
  server.begin();
}

// ---------------- LOOP ----------------
void loop() {
  server.handleClient();

  int freeSlots = 0;

  // -------- SLOT CHECK --------
  for (int i = 0; i < 4; i++) {
    distance[i] = readDistance(trigPins[i], echoPins[i]);

    if (distance[i] > 10) {
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

  // -------- IR READ --------
  bool entryIR = digitalRead(IR_ENTRY) == LOW;
  bool exitIR  = digitalRead(IR_EXIT) == LOW;

  unsigned long now = millis();

  // -------- EDGE DETECTION --------
  bool entryTrigger = (entryIR && !lastEntryIR);
  bool exitTrigger  = (exitIR && !lastExitIR);

  lastEntryIR = entryIR;
  lastExitIR = exitIR;

  // -------- TOGGLE GATE --------
  if ((entryTrigger || exitTrigger) && (now - lastIRTrigger > debounceDelay)) {
    gateOpen = !gateOpen;

    if (gateOpen) {
      gateServo.write(90);
      Serial.println("Gate OPEN");
    } else {
      gateServo.write(0);
      Serial.println("Gate CLOSED");
    }

    lastIRTrigger = now;
  }

  // -------- LCD --------
  lcd.setCursor(0, 0);
  lcd.print("Free:");
  lcd.print(freeSlots);
  lcd.print("/4   ");

  lcd.setCursor(0, 1);
  lcd.print(gateOpen ? "GATE OPEN   " : "GATE CLOSED ");

  delay(120);
}