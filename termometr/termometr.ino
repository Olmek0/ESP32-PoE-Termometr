#include <OneWire.h>
#include <DallasTemperature.h>
#include <ETH.h>
#include <WebServer.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <SD_MMC.h>
#include "FS.h"
#include "sqlite3.h"
#include <ezTime.h>
#include <WebSocketsServer.h>


//// INICJALIZACJA ////

#define FILESYSTEM LittleFS
WebServer server(80); // port 80
WebSocketsServer webSocket = WebSocketsServer(81); // port 81
sqlite3 *db; // sql database
char *zErrMsg = 0; // błąd w sql

bool eth_connected = false;

// Temperatura inicjalizacja //

const int Temp = 32;
OneWire oneWire(Temp);
DallasTemperature sensors(&oneWire);
String temperature;

//// FUNKCJE ////

// Utworzenie servera

void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      Serial.println("[ETH] Started");
      ETH.setHostname("esp32-poe");
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      Serial.println("[ETH] Connected");
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      Serial.print("[ETH] Got IP: ");
      Serial.println(ETH.localIP());
      eth_connected = true;
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Serial.println("[ETH] Disconnected");
      eth_connected = false;
      break;
    case ARDUINO_EVENT_ETH_STOP:
      Serial.println("[ETH] Stopped");
      eth_connected = false;
      break;
    default:
      break;
  }
}

// Parse plików servera

void handleRoot(String path, String content) {
  File file = FILESYSTEM.open(path.c_str(), "r");
  if (!file) {
    server.send(500, "text/plain", "Failed to open /index.html");
    return;
  }
  server.streamFile(file, content);
  file.close();
}

// baza danych, tworzona jeśli nie istnieje

void initDatabase() {
  String dbPath = "/sdcard/temperature.db";

  // Open database
  if (sqlite3_open("/sdcard/temperature.db", &db)) {
    Serial.printf("[DB ERROR] %s\n", sqlite3_errmsg(db));
    return;
  }

  // Create table if not exists
  const char *create_table_sql = R"sql(
    CREATE TABLE IF NOT EXISTS logs (
      timestamp TEXT NOT NULL,
      temperature REAL NOT NULL
    );
  )sql";

  int rc = sqlite3_exec(db, create_table_sql, NULL, 0, &zErrMsg);
  if (rc != SQLITE_OK) {
    Serial.printf("[DB ERROR] SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
  } else {
    Serial.println("[DB] Table ready");
  }

  sqlite3_close(db);
}

// podanie daty

String getTimestamp() {
  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);
  char buf[20];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &timeinfo);
  return String(buf);
}

// 

void logTemperature(float tempC, const String& timestamp) {
  if (sqlite3_open("/sdcard/temperature.db", &db)) {
    Serial.println("[DB ERROR] Failed to open database");
    return;
  }

  String query = "INSERT INTO logs (timestamp, temperature) VALUES ('" + timestamp + "', " + String(tempC, 2) + ");";

  int rc = sqlite3_exec(db, query.c_str(), NULL, 0, &zErrMsg);
  if (rc != SQLITE_OK) {
    Serial.printf("[DB ERROR] Insert failed: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
  } else {
    Serial.println("[DB] Logged: " + timestamp + ", " + String(tempC));
  }

  sqlite3_close(db);
}

//// PROGRAM

void setup() {
  Serial.begin(115200);
  delay(200);

  Serial.println("[BOOT] Starting...");

  if (!FILESYSTEM.begin(true)) {
    Serial.println("[ERROR] LittleFS Mount Failed");
    return;
  }
  Serial.println("[FS] LittleFS mounted");

  sensors.begin();

  if (!SD_MMC.begin("/sdcard", true)) {  // true = 1-bit mode
    Serial.println("[ERROR] SD_MMC mount failed!");
    return;
  }

  Serial.println("[OK] SD_MMC mounted!");

  initDatabase();

  WiFi.onEvent(WiFiEvent);
  delay(100); 
  ETH.begin();

  for (int i = 0; i < 100 && !eth_connected; i++) {
    delay(100);
    Serial.print(".");
  }
  Serial.println();
  

  configTzTime("CET-1CEST,M3.5.0/2,M10.5.0/3", "pool.ntp.org");

  server.on("/", HTTP_GET, []() {handleRoot("/index.html","text/html"); });
  server.on("/styl.css", HTTP_GET, []() {handleRoot("/styl.css","text/css"); });
  server.on("/skrypt.js", HTTP_GET, []() {handleRoot("/skrypt.js","application/javascript"); });

  // websockety

  webSocket.begin();
  webSocket.onEvent([](uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
    if (type == WStype_CONNECTED) {
      Serial.println("[WS] Client connected");
      
      String initialTemp = String(sensors.getTempCByIndex(0));
      webSocket.sendTXT(num, initialTemp);
    } else if (type == WStype_DISCONNECTED) {
      Serial.println("[WS] Client disconnected");
    }
});

  server.begin();
  Serial.println("[HTTP] Server started");
}

// Przerwa w odczytach na bieżąco

unsigned long lastTempRequest = 0;
unsigned long lastDbInsert = 0;
const unsigned long tempInterval = 5000;
const unsigned long dbInsertInterval = 300000;

void loop() {
  server.handleClient();
  webSocket.loop();
  if (millis() - lastTempRequest >= tempInterval) {
    lastTempRequest = millis();
    sensors.requestTemperatures();
    temperature = String(sensors.getTempCByIndex(0));
    webSocket.broadcastTXT(temperature);
    Serial.println("Temperature: " + temperature + "°C");
  }
  
  if (millis() - lastDbInsert >= dbInsertInterval) {
    lastDbInsert = millis();
    sensors.requestTemperatures();
    float tempVal = sensors.getTempCByIndex(0);
    String timestamp = getTimestamp();
    logTemperature(tempVal, timestamp);
    Serial.println("[DB] Logged temperature: " + String(tempVal) + " at " + timestamp);
  }
}
