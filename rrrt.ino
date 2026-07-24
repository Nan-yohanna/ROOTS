//Roots firmware for embedded systems😁😁😁😁.

#include <WiFi.h>
#include <WiFiManager.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <time.h>
#include <math.h>

// FIREBASE CONFIGURATION

const char* FIREBASE_HOST = "https://root-plant-watering-default-rtdb.europe-west1.firebasedatabase.app";
const char* DEVICE_ID = "frame01";

// TIME CONFIG

const long GMT_OFFSET_SEC = 3600; // WAT, UTC+1
const int DAYLIGHT_OFFSET_SEC = 0;

// HARDWARE DESIGN SOIL + PUMP

#define SOIL_PIN 4
#define PUMP_RELAY_PIN 5
#define RELAY_ACTIVE_LOW true

// Calibrated sensor readings
#define ADC_DRY_RAW 4095
#define ADC_WET_RAW 1850

//WIFI RESET BUTTON

#define WIFI_RESET_BUTTON_PIN 15
#define WIFI_RESET_HOLD_MS 3000UL

//TEST MODE VALUES: change SENSOR_POLL_INTERVAL_MS back to (5UL*60UL*1000UL) before real deployment
#define SENSOR_POLL_INTERVAL_MS (10UL * 1000UL)
#define PUMP_CHECK_INTERVAL_MS (5UL * 1000UL)
#define SETTINGS_POLL_INTERVAL_MS (30UL * 1000UL)
#define HEARTBEAT_INTERVAL_MS (30UL * 1000UL)
#define JUMP_THRESHOLD 15.0
#define LEAD_TIME_BUFFER_SEC (30UL * 60UL)
#define MIN_VALID_POINTS_FOR_FIT 6
#define MAX_POINTS 50
#define MAX_PUMP_RUN_TIME_MS (60UL * 1000UL)

// WIFI PORTAL AFTER BEING RESET

const char* WIFI_PORTAL_CSS = R"rawliteral(
<style>
body {
background: #0f1712 !important;
color: #e8fff0 !important;
font-family: -apple-system, Arial, sans-serif !important;
}
h1, h2, h3, .msg b {
color: #39ff88 !important;
}
button, input[type='submit'], input[type='button'] {
background: #1f9d5c !important;
color: #04150b !important;
border: none !important;
border-radius: 8px !important;
padding: 10px 16px !important;
font-weight: 600 !important;
}
input[type='text'], input[type='password'] {
background: #1a2620 !important;
color: #e8fff0 !important;
border: 1px solid #2a3b30 !important;
border-radius: 8px !important;
padding: 8px !important;
}
a { color: #39ff88 !important; }
.msg { background: #1a2620 !important; border-radius: 8px !important; padding: 10px !important; }
hr { border-color: #2a3b30 !important; }
</style>
)rawliteral";



WiFiManager wm; // WIFI MANAGER MAKES WIFI WORK

// STATE OF SENSORS

struct Reading {
uint32_t t;
float moisture;
};

Reading cycleData[MAX_POINTS];
int cyclePointCount = 0;
uint32_t wateringStartTime = 0;

float currentMoisture = 0;
float prevMoisture = 0;
float M_dry = 20.0;
float tau = -1;
float lastGoodTau = -1;
float tauHistory[5] = {-1,-1,-1,-1,-1};
int tauHistoryIndex = 0;

float minMoisture = 25.0;
float maxMoisture = 65.0;

unsigned long lastPollTime = 0;
unsigned long lastSettingsPoll = 0;
unsigned long lastHeartbeat = 0;
unsigned long pumpStartedAt = 0;
bool pumpRunning = false;

unsigned long buttonPressStart = 0;
bool buttonHeld = false;

String lastEventMessage = "System started.";

// HELPERS FOR SENSORS

float readMoisturePercent() {
const int SAMPLES = 5;
long sum = 0;
for (int i = 0; i < SAMPLES; i++) {
sum += analogRead(SOIL_PIN);
delay(10);
}
float raw = sum / (float)SAMPLES;

float pct = (ADC_DRY_RAW - raw) / (float)(ADC_DRY_RAW - ADC_WET_RAW) * 100.0;
if (pct < 0) pct = 0;
if (pct > 100) pct = 100;
return pct;
}

bool wateringDetected(float prev, float curr) {
return (curr - prev) > JUMP_THRESHOLD;
}

// HELPERS FOR TIME PREDICTION

String getDateString() {
time_t now = time(nullptr);
struct tm timeinfo;
localtime_r(&now, &timeinfo);
char buf[11];
strftime(buf, sizeof(buf), "%Y-%m-%d", &timeinfo);
return String(buf);
}

String getTimeString() {
time_t now = time(nullptr);
struct tm timeinfo;
localtime_r(&now, &timeinfo);
char buf[6];
strftime(buf, sizeof(buf), "%H:%M", &timeinfo);
return String(buf);
}

// FIREBASE HELPERS

bool firebasePut(const String& path, const String& jsonValue) {
if (WiFi.status() != WL_CONNECTED) return false;
WiFiClientSecure client;
client.setInsecure();
HTTPClient http;
String url = String(FIREBASE_HOST) + path + ".json";
http.begin(client, url);
http.addHeader("Content-Type", "application/json");
int code = http.PUT(jsonValue);
bool ok = (code >= 200 && code < 300);
if (!ok) {
Serial.print("Firebase PUT failed, code: ");
Serial.println(code);
}
http.end();
return ok;
}

bool firebasePost(const String& path, const String& jsonBody) {
if (WiFi.status() != WL_CONNECTED) return false;
WiFiClientSecure client;
client.setInsecure();
HTTPClient http;
String url = String(FIREBASE_HOST) + path + ".json";
http.begin(client, url);
http.addHeader("Content-Type", "application/json");
int code = http.POST(jsonBody);
bool ok = (code >= 200 && code < 300);
if (!ok) {
Serial.print("Firebase POST failed, code: ");
Serial.println(code);
}
http.end();
return ok;
}

String firebaseGet(const String& path) {
if (WiFi.status() != WL_CONNECTED) return "";
WiFiClientSecure client;
client.setInsecure();
HTTPClient http;
String url = String(FIREBASE_HOST) + path + ".json";
http.begin(client, url);
int code = http.GET();
String payload = "";
if (code == 200) {
payload = http.getString();
} else {
Serial.print("Firebase GET failed, code: ");
Serial.println(code);
}
http.end();
return payload;
}

void pushMoisture(float moisture) {
firebasePut("/devices/" + String(DEVICE_ID) + "/moisture", String(moisture, 1));
}

void pushHistoryEntry(float moisture) {
String path = "/history/" + getDateString();
String json = "{";
json += "\"time\":\"" + getTimeString() + "\",";
json += "\"moisture\":" + String(moisture, 1);
json += "}";
firebasePost(path, json);
}

void pushPumpStatus(bool on) {
firebasePut("/devices/" + String(DEVICE_ID) + "/pumpStatus", on ? "\"on\"" : "\"off\"");
}

void pushLastEvent(const String& text) {
String safe = text;
safe.replace("\"", "'");
firebasePut("/devices/" + String(DEVICE_ID) + "/lastEvent", "\"" + safe + "\"");
}

void pushHeartbeat() {
time_t now = time(nullptr);
firebasePut("/devices/" + String(DEVICE_ID) + "/lastSeen", String((long)now));
}

void pushLogEntry(const String& event, float moisture) {
String path = "/logs/" + getDateString();
String json = "{";
json += "\"time\":\"" + getTimeString() + "\",";
json += "\"event\":\"" + event + "\",";
json += "\"moisture\":" + String(moisture, 1);
json += "}";
firebasePost(path, json);
}

float extractJsonNumber(const String& json, const String& key, float fallback) {
String searchKey = "\"" + key + "\":";
int idx = json.indexOf(searchKey);
if (idx < 0) return fallback;
int start = idx + searchKey.length();
int end = start;
while (end < (int)json.length() && (isDigit(json[end]) || json[end] == '.' || json[end] == '-')) {
end++;
}
if (end == start) return fallback;
return json.substring(start, end).toFloat();
}

void pullSettings() {
String path = "/devices/" + String(DEVICE_ID) + "/settings";
String json = firebaseGet(path);
if (json.length() == 0 || json == "null") return;

float newMin = extractJsonNumber(json, "minMoisture", minMoisture);
float newMax = extractJsonNumber(json, "maxMoisture", maxMoisture);

if (newMax > newMin + 5 && newMin >= 0 && newMax <= 100) {
if (newMin != minMoisture || newMax != maxMoisture) {
Serial.print("Settings updated — min: ");
Serial.print(newMin);
Serial.print(", max: ");
Serial.println(newMax);
}
minMoisture = newMin;
maxMoisture = newMax;
}
}

// WIFI RESET BUTTON

void checkWifiResetButton() {
bool pressed = (digitalRead(WIFI_RESET_BUTTON_PIN) == LOW);

if (pressed && !buttonHeld) {
buttonHeld = true;
buttonPressStart = millis();
} else if (!pressed && buttonHeld) {
buttonHeld = false;
}

if (buttonHeld && (millis() - buttonPressStart >= WIFI_RESET_HOLD_MS)) {
Serial.println("\n>>> WiFi reset button held — erasing saved WiFi and restarting into setup mode <<<\n");
wm.resetSettings();
delay(500);
ESP.restart();
}
}

// PUMP CONTROL

void setPumpRelay(bool on) {
bool level = RELAY_ACTIVE_LOW ? !on : on;
digitalWrite(PUMP_RELAY_PIN, level ? HIGH : LOW);
}

void startPump(const char* reason) {
setPumpRelay(true);
pumpRunning = true;
pumpStartedAt = millis();

lastEventMessage = String("Pump started (") + reason + ") at moisture " +
String(currentMoisture, 1) + "%.";
Serial.println(lastEventMessage);

pushPumpStatus(true);
pushLastEvent(lastEventMessage);
pushLogEntry(String("Pump turned on (") + reason + ")", currentMoisture);
}

void stopPump(const char* reason) {
setPumpRelay(false);
pumpRunning = false;

lastEventMessage = String("Pump stopped (") + reason + ") at moisture " +
String(currentMoisture, 1) + "%.";
Serial.println(lastEventMessage);

pushPumpStatus(false);
pushLastEvent(lastEventMessage);
pushLogEntry(String("Pump turned off (") + reason + ")", currentMoisture);
}

void servicePump() {
if (!pumpRunning) return;

static unsigned long lastPumpCheck = 0;
unsigned long nowMs = millis();
if (nowMs - lastPumpCheck < PUMP_CHECK_INTERVAL_MS) return;
lastPumpCheck = nowMs;

float moisture = readMoisturePercent();
currentMoisture = moisture;
pushMoisture(moisture);

Serial.print("Pump running, moisture: ");
Serial.print(moisture);
Serial.println("%");

if (moisture >= maxMoisture) {
stopPump("target reached");
return;
}

if (nowMs - pumpStartedAt >= MAX_PUMP_RUN_TIME_MS) {
stopPump("safety cutoff — check sensor");
}
}

// DRY CURVE REGRESSION FOR SOI1 MOISTURE

float estimateTau(Reading* points, int n, float mDry) {
float sumT = 0, sumY = 0, sumTY = 0, sumTT = 0;
int validN = 0;
for (int i = 0; i < n; i++) {
float diff = points[i].moisture - mDry;
if (diff <= 0.01) continue;
float y = log(diff);
float t = (float)points[i].t;
sumT += t; sumY += y; sumTY += t * y; sumTT += t * t;
validN++;
}
if (validN < MIN_VALID_POINTS_FOR_FIT) return -1;
float denom = (validN * sumTT - sumT * sumT);
if (fabs(denom) < 0.0001) return -1;
float slope = (validN * sumTY - sumT * sumY) / denom;
if (slope >= 0) return -1;
float result = -1.0 / slope;
if (result <= 0 || result > (10.0 * 24 * 3600)) return -1;
return result;
}

void recordTau(float newTau) {
lastGoodTau = newTau;
tauHistory[tauHistoryIndex] = newTau;
tauHistoryIndex = (tauHistoryIndex + 1) % 5;
}

float averagedTau() {
float sum = 0;
int count = 0;
for (int i = 0; i < 5; i++) {
if (tauHistory[i] > 0) { sum += tauHistory[i]; count++; }
}
return (count > 0) ? (sum / count) : -1;
}

// MAIN MOISTURE CHECK CYCLE

void onSensorRead(float moisture) {
Serial.print("Moisture reading: ");
Serial.print(moisture);
Serial.println("%");

if (moisture < M_dry) M_dry = moisture;

if (wateringDetected(prevMoisture, moisture)) {
wateringStartTime = millis();
cyclePointCount = 0;
tau = -1;
lastEventMessage = "Watering event detected, starting new drying cycle.";
Serial.println(lastEventMessage);
pushLastEvent(lastEventMessage);
}

if (cyclePointCount < MAX_POINTS) {
cycleData[cyclePointCount].t = (millis() - wateringStartTime) / 1000;
cycleData[cyclePointCount].moisture = moisture;
cyclePointCount++;
}

prevMoisture = moisture;
currentMoisture = moisture;
pushMoisture(moisture);
pushHistoryEntry(moisture);

if (cyclePointCount >= MIN_VALID_POINTS_FOR_FIT) {
float fit = estimateTau(cycleData, cyclePointCount, M_dry);
if (fit > 0) {
recordTau(fit);
tau = averagedTau();
}
}

if (!pumpRunning && moisture <= minMoisture) {
Serial.println(">>> Moisture at/below minimum — triggering pump");
startPump("below minimum");
}
}

// SETUP, LOOP

void setup() {
Serial.begin(115200);
delay(1000);
Serial.println("\n--- Roots firmware starting ---");

pinMode(PUMP_RELAY_PIN, OUTPUT);
setPumpRelay(false);

pinMode(WIFI_RESET_BUTTON_PIN, INPUT_PULLUP);

wm.setTitle("roots");
wm.setCustomHeadElement(WIFI_PORTAL_CSS);

Serial.println("Starting WiFi (WiFiManager)...");
Serial.println("If no saved network connects, look for a WiFi network called 'Root-Setup' on your phone.");
Serial.println("Hold the reset button for 3+ seconds at any time to forget WiFi and re-enter setup.");

bool connected = wm.autoConnect("Root-Setup");

if (!connected) {
Serial.println("Failed to connect and setup timed out. Restarting...");
delay(2000);
ESP.restart();
}

Serial.print("Connected. IP: ");
Serial.println(WiFi.localIP());

configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, "pool.ntp.org", "time.nist.gov");
Serial.print("Syncing time");
time_t now = time(nullptr);
while (now < 100000) {
delay(500);
Serial.print(".");
now = time(nullptr);
}
Serial.println();
Serial.println("Time synced.");

pullSettings();
Serial.print("Loaded thresholds — min: ");
Serial.print(minMoisture);
Serial.print(", max: ");
Serial.println(maxMoisture);

prevMoisture = readMoisturePercent();
currentMoisture = prevMoisture;
wateringStartTime = millis();

pushMoisture(currentMoisture);
pushPumpStatus(false);
pushLastEvent("Device connected and ready.");
pushHeartbeat();

Serial.println("--- Setup complete, entering loop ---\n");
}

void loop() {
checkWifiResetButton();

servicePump();

if (millis() - lastSettingsPoll >= SETTINGS_POLL_INTERVAL_MS) {
lastSettingsPoll = millis();
pullSettings();
}

if (millis() - lastHeartbeat >= HEARTBEAT_INTERVAL_MS) {
lastHeartbeat = millis();
pushHeartbeat();
}

if (!pumpRunning && millis() - lastPollTime >= SENSOR_POLL_INTERVAL_MS) {
lastPollTime = millis();
float moisture = readMoisturePercent();
onSensorRead(moisture);
}
}
