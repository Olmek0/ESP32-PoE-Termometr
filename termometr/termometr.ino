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
#include <ArduinoJson.h>
#include <ESPmDNS.h> 

//// INICJALIZACJA ////

#define FILESYSTEM LittleFS
WebServer server(80); // port 80
WebSocketsServer webSocket = WebSocketsServer(81); // port 81
sqlite3 *db; // sql database
char *zErrMsg = 0; // błąd w sql

bool eth_connected = false;

bool useDHCP = true;
IPAddress staticIP;
IPAddress gateway;
IPAddress subnet;
IPAddress dns1;
IPAddress dns2;

// For storing configuration
#define CONFIG_FILE "/ipconfig.txt"

// Temperatura inicjalizacja //

const int Temp = 32;
OneWire oneWire(Temp);
DallasTemperature sensors(&oneWire);

struct TempPair {
  String c;
  String f;
};

//// FUNKCJE ////

void saveIPConfig() {
  File file = FILESYSTEM.open(CONFIG_FILE, "w");
  if (!file) {
    Serial.println("[IP] Failed to open config file for writing");
    return;
  }
  
  file.println(useDHCP ? "dhcp" : "static");
  if (!useDHCP) {
    file.println(staticIP.toString());
    file.println(gateway.toString());
    file.println(subnet.toString());
    file.println(dns1.toString());
    file.println(dns2.toString());
  }
  
  file.close();
  Serial.println("[IP] Configuration saved");
}

void loadIPConfig() {
  if (!FILESYSTEM.exists(CONFIG_FILE)) {
    Serial.println("[IP] No config file, using defaults");
    return;
  }
  
  File file = FILESYSTEM.open(CONFIG_FILE, "r");
  if (!file) {
    Serial.println("[IP] Failed to open config file");
    return;
  }
  
  String mode = file.readStringUntil('\n');
  mode.trim();
  useDHCP = (mode == "dhcp");
  
  if (!useDHCP) {
    staticIP.fromString(file.readStringUntil('\n'));
    gateway.fromString(file.readStringUntil('\n'));
    subnet.fromString(file.readStringUntil('\n'));
    dns1.fromString(file.readStringUntil('\n'));
    dns2.fromString(file.readStringUntil('\n'));
  }
  
  file.close();
  Serial.println("[IP] Configuration loaded");
}

IPAddress lastIP;

void checkIPChange() {
  IPAddress currentIP = ETH.localIP();
  if (currentIP != lastIP) {
    Serial.print("[ETH] IP changed to: ");
    Serial.println(currentIP);
    lastIP = currentIP;

    // If you want to save it:
    if (useDHCP) {
      staticIP = currentIP; // update stored IP to current DHCP IP
      saveIPConfig();
    }
  }
}


void applyNetworkConfig() {
  if (useDHCP) {
    ETH.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
    Serial.println("[IP] Using DHCP");
  } else {
    ETH.config(staticIP, gateway, subnet, dns1, dns2);
    Serial.println("[IP] Using static IP: " + staticIP.toString());
  }
}

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
    case ARDUINO_EVENT_ETH_LOST_IP:
      Serial.println("[ETH] Lost IP");
      eth_connected = false;
      if (useDHCP) {
        ETH.config(INADDR_NONE, INADDR_NONE, INADDR_NONE); // Re-enable DHCP
      }
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
}void sendHistoryJson(const String& start, const String& end) {
  if (sqlite3_open("/sdcard/temperature.db", &db)) {
    server.send(500, "application/json", "{\"error\":\"Failed to open database\"}");
    return;
  }

  String query = "SELECT timestamp, temperature_c, temperature_f FROM logs2 "
                 "WHERE DATE(timestamp) >= DATE('" + start + "') AND DATE(timestamp) <= DATE('" + end + "') "
                 "ORDER BY timestamp ASC;";

  sqlite3_stmt* stmt;
  int rc = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    sqlite3_close(db);
    server.send(500, "application/json", "{\"error\":\"Failed to execute query\"}");
    return;
  }
  String json = "{ \"total\":0, \"data\":[";
  bool first = true;
  int total = 0;

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    if (!first) json += ",";
    first = false;
    total++;

    const char* ts = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    float c = sqlite3_column_double(stmt, 1);
    float f = sqlite3_column_double(stmt, 2);

    json += "{\"timestamp\":\"" + String(ts) + "\",\"c\":" + String(c, 2) + ",\"f\":" + String(f, 2) + "}";
  }

  json += "], \"total\":" + String(total) + " }";

  sqlite3_finalize(stmt);
  sqlite3_close(db);

  server.send(200, "application/json", json);
}

void sendChartData() {
  if (sqlite3_open("/sdcard/temperature.db", &db)) {
    Serial.println("[DB ERROR] Cannot open DB for chart");
    return;
  }

  // Query for last 24 hours, grouped by hour
  const char *query24h = R"sql(
    SELECT 
      strftime('%Y-%m-%d %H:00', timestamp) AS hour,
      AVG(temperature_c) AS avg_c,
      AVG(temperature_f) AS avg_f
    FROM logs2
    WHERE timestamp >= datetime('now', '-24 hours')
    GROUP BY hour
    ORDER BY hour ASC;
  )sql";

  // Query for last 30 days, grouped by day
  const char *query30d = R"sql(
    SELECT 
      strftime('%Y-%m-%d', timestamp) AS day,
      AVG(temperature_c) AS avg_c,
      AVG(temperature_f) AS avg_f
    FROM logs2
    WHERE timestamp >= date('now', '-30 days')
    GROUP BY day
    ORDER BY day ASC;
  )sql";

  String json = "{\"type\":\"chart\"";

  // 24h data
  sqlite3_stmt *stmt24;
  if (sqlite3_prepare_v2(db, query24h, -1, &stmt24, NULL) == SQLITE_OK) {
    json += ",\"data\":[";
    bool first = true;
    while (sqlite3_step(stmt24) == SQLITE_ROW) {
      if (!first) json += ",";
      first = false;

      const char *hour = reinterpret_cast<const char *>(sqlite3_column_text(stmt24, 0));
      float avg_c = sqlite3_column_double(stmt24, 1);
      float avg_f = sqlite3_column_double(stmt24, 2);

      json += "{\"hour\":\"" + String(hour) + "\",\"avg_c\":" + String(avg_c, 2) + ",\"avg_f\":" + String(avg_f, 2) + "}";
    }
    json += "]";
    sqlite3_finalize(stmt24);
  } else {
    Serial.printf("[DB ERROR] 24h query failed: %s\n", sqlite3_errmsg(db));
  }

  // 30d data
  sqlite3_stmt *stmt30;
  if (sqlite3_prepare_v2(db, query30d, -1, &stmt30, NULL) == SQLITE_OK) {
    json += ",\"monthly\":[";
    bool first = true;
    while (sqlite3_step(stmt30) == SQLITE_ROW) {
      if (!first) json += ",";
      first = false;

      const char *day = reinterpret_cast<const char *>(sqlite3_column_text(stmt30, 0));
      float avg_c = sqlite3_column_double(stmt30, 1);
      float avg_f = sqlite3_column_double(stmt30, 2);

      json += "{\"day\":\"" + String(day) + "\",\"avg_c\":" + String(avg_c, 2) + ",\"avg_f\":" + String(avg_f, 2) + "}";
    }
    json += "]";
    sqlite3_finalize(stmt30);
  } else {
    Serial.printf("[DB ERROR] 30d query failed: %s\n", sqlite3_errmsg(db));
  }

  json += "}";

  sqlite3_close(db);

  webSocket.broadcastTXT(json);
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

  loadIPConfig();

  WiFi.onEvent(WiFiEvent);
  delay(100); 
  ETH.begin();
  
  applyNetworkConfig();

  for (int i = 0; i < 100 && !eth_connected; i++) {
    delay(100);
    Serial.print(".");
  }
  Serial.println();
  
  configTzTime("CET-1CEST,M3.5.0/2,M10.5.0/3", "pool.ntp.org");

  if (MDNS.begin("esp32-poe")) {
    Serial.println("[MDNS] mDNS responder started: esp32-poe.local");
  } else {
    Serial.println("[MDNS] Error setting up mDNS responder!");
  }

  server.on("/", HTTP_GET, []() {handleRoot("/index.html","text/html"); });
  server.on("/styl.css", HTTP_GET, []() {handleRoot("/styl.css","text/css"); });
  server.on("/skrypt.js", HTTP_GET, []() {handleRoot("/skrypt.js","application/javascript"); });

  server.on("/history.html", HTTP_GET, []() {handleRoot("/history.html", "text/html");});

  server.on("/skrypt2.js", HTTP_GET, []() {handleRoot("/skrypt2.js", "application/javascript");});

  server.on("/set.html", HTTP_GET, []() { handleRoot("/set.html", "text/html"); });

  server.on("/api/ipconfig", HTTP_GET, []() {
    String json = "{";
    json += "\"dhcp\":" + String(useDHCP ? "true" : "false") + ",";
    json += "\"ip\":\"" + (useDHCP ? ETH.localIP().toString() : staticIP.toString()) + "\",";
    json += "\"gateway\":\"" + (useDHCP ? ETH.gatewayIP().toString() : gateway.toString()) + "\",";
    json += "\"subnet\":\"" + (useDHCP ? ETH.subnetMask().toString() : subnet.toString()) + "\",";
    json += "\"dns1\":\"" + (useDHCP ? ETH.dnsIP(0).toString() : dns1.toString()) + "\",";
    json += "\"dns2\":\"" + (useDHCP ? ETH.dnsIP(1).toString() : dns2.toString()) + "\"";
    json += "}";
    server.send(200, "application/json", json);
  });

  server.on("/api/ipconfig", HTTP_POST, []() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"Bad request\"}");
    return;
  }

  String body = server.arg("plain");
  DynamicJsonDocument doc(512);
  deserializeJson(doc, body);

  bool willUseDHCP = doc["dhcp"];
  IPAddress newStaticIP, newGateway, newSubnet, newDNS1, newDNS2;

  if (!willUseDHCP) {
    newStaticIP.fromString(doc["ip"].as<const char*>());
    newGateway.fromString(doc["gateway"].as<const char*>());
    newSubnet.fromString(doc["subnet"].as<const char*>());
    newDNS1.fromString(doc["dns1"].as<const char*>());
    newDNS2.fromString(doc["dns2"].as<const char*>());
  }

  // Get future IP address (either static or what DHCP likely assigns)

  // Send success JSON BEFORE disconnecting
  server.send(200, "application/json", "{\"status\":\"ok\",\"reboot\":true}");

  // Delay to allow browser to finish receiving response
  delay(500);

  // Now apply the new network config
  useDHCP = willUseDHCP;
  if (!willUseDHCP) {
    staticIP = newStaticIP;
    gateway = newGateway;
    subnet = newSubnet;
    dns1 = newDNS1;
    dns2 = newDNS2;
  }

  saveIPConfig();
  applyNetworkConfig();  // This may restart Ethernet

  // Optional: delay to stabilize after change
  delay(100);
});


  server.on("/api/history", HTTP_GET, []() {
  if (!server.hasArg("start") || !server.hasArg("end")) {
    server.send(400, "application/json", "{\"error\":\"Missing date range\"}");
    return;
  }

  sendHistoryJson(server.arg("start"), server.arg("end"));
  });

  // websockety

  webSocket.begin();
  webSocket.onEvent([](uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
    if (type == WStype_CONNECTED) {
      Serial.println("[WS] Client connected");
      TempPair temp = GetTemperature();
      sendStatsOverWebSocket();
      sendChartData();
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
  
  checkIPChange();
  
  if (millis() - lastDbInsert >= dbInsertInterval) {
    lastDbInsert = millis();
    sensors.requestTemperatures();
    float tempValc = sensors.getTempCByIndex(0);
    float tempValf = sensors.getTempFByIndex(0);
    String timestamp = getTimestamp();
    logTemperature(tempValc, tempValf, timestamp);
    Serial.println("[DB] Logged temperature: " + String(tempValc) + " / " + String(tempValf) + " at " + timestamp);
    sendChartData();
    sendStatsOverWebSocket();
  }
  if (millis() - lastTempRequest >= tempInterval) {
    lastTempRequest = millis();
    sensors.requestTemperatures();
    TempPair temp = GetTemperature();
    Serial.println("Temperature: " + temp.c + "°C / " + temp.f + "°F");
  }
}