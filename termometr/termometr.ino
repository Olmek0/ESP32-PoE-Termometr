#include <OneWire.h>
#include <DallasTemperature.h>
#include <ETH.h>
#include <WebServer.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <SD_MMC.h>
#include "FS.h"
#include "sqlite3.h"
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

struct TempPair {
  String c;
  String f;
};

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

  if (sqlite3_open("/sdcard/temperature.db", &db)) {
    Serial.printf("[DB ERROR] %s\n", sqlite3_errmsg(db));
    return;
  }

  // Create table if not exists
  const char *create_table_sql = R"sql(
    CREATE TABLE IF NOT EXISTS logs2 (
      timestamp TEXT NOT NULL,
      temperature_c REAL NOT NULL,
      temperature_f REAL NOT NULL
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

void logTemperature(float tempC, float tempF, const String& timestamp) {
  if (sqlite3_open("/sdcard/temperature.db", &db)) {
    Serial.println("[DB ERROR] Failed to open database");
    return;
  }

  String query = "INSERT INTO logs2 (timestamp, temperature_c, temperature_f) VALUES ('" + timestamp + "', " + String(tempC, 2) + "," + String(tempF, 2) + ");";

  int rc = sqlite3_exec(db, query.c_str(), NULL, 0, &zErrMsg);
  if (rc != SQLITE_OK) {
    Serial.printf("[DB ERROR] Insert failed: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
  } else {
    Serial.println("[DB] Logged: " + timestamp + ", " + String(tempC));
  }

  sqlite3_close(db);
}

//

void sendStatsOverWebSocket() {
  if (sqlite3_open("/sdcard/temperature.db", &db)) {
    Serial.println("[DB ERROR] Failed to open database for stats");
    return;
  }
  const char* Query = R"sql(
    SELECT 
      COUNT(*) AS total,
      MAX(temperature_c) AS max_temp,
      MIN(temperature_c) AS min_temp,
      MAX(temperature_f) AS max_tempf,
      MIN(temperature_f) AS min_tempf,
      DATE(MIN(timestamp)) AS first_entry
    FROM logs2;                                                                       
  )sql";

  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, Query, -1, &stmt, NULL);

  if (rc != SQLITE_OK) {
    Serial.printf("[DB ERROR] Query failed: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return;
  }

  if (sqlite3_step(stmt) == SQLITE_ROW) {
    int total = sqlite3_column_int(stmt, 0);
    float maxTemp = sqlite3_column_double(stmt, 1);
    float minTemp = sqlite3_column_double(stmt, 2);
    float maxTempf = sqlite3_column_double(stmt, 3);
    float minTempf = sqlite3_column_double(stmt, 4);
    const unsigned char* firstEntry = sqlite3_column_text(stmt, 5);

    String start = firstEntry ? String((const char*)firstEntry) : "unknown";

    String statsJson = "{\"total\":" + String(total) + ",\"max\":" + String(maxTemp) + ",\"min\":" + String(minTemp) + ",\"maxf\":" + String(maxTempf) + ",\"minf\":" + String(minTempf)
    + ",\"start\":\"" + start + "\"}";

    webSocket.broadcastTXT(statsJson);
  }

  sqlite3_finalize(stmt);
  sqlite3_close(db);
}

TempPair GetTemperature(){
    TempPair temp;
    temp.c = String(sensors.getTempCByIndex(0));
    temp.f = String(sensors.getTempFByIndex(0));
    String js = "{\"c\":" + temp.c + ",\"f\":" + temp.f + "}";
    webSocket.broadcastTXT(js);

    return temp;
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
      TempPair temp = GetTemperature();
      sendStatsOverWebSocket();

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
    TempPair temp = GetTemperature();
    Serial.println("Temperature: " + temp.c + "°C / " + temp.f + "°F");
  }
  
  if (millis() - lastDbInsert >= dbInsertInterval) {
    lastDbInsert = millis();
    sensors.requestTemperatures();
    float tempValc = sensors.getTempCByIndex(0);
    float tempValf = sensors.getTempFByIndex(0);
    String timestamp = getTimestamp();
    logTemperature(tempValc, tempValf, timestamp);
    Serial.println("[DB] Logged temperature: " + String(tempValc) + " / " + String(tempValf) + " at " + timestamp);
  }
}