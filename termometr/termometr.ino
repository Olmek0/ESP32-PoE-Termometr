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
#include <HTTPClient.h>
#include <UrlEncode.h>
#include "db_functions.h"
#include "variables.h"

//// INICJALIZACJA ////

#define FILESYSTEM LittleFS

WebServer server(80);
WebSocketsServer webSocket(81);
sqlite3 *db;
char *zErrMsg;

bool eth_connected = false;

bool useDHCP = true;
IPAddress staticIP;
IPAddress gateway;
IPAddress subnet;
IPAddress dns1;
IPAddress dns2;

// WhatsApp 
String whatsappAPIKey = ""; 
String recipientPhone = "";
float highTempLimit = 30.0;
float lowTempLimit = 10.0;
bool alertsEnabled = false;
bool alertSentHigh = false;
bool alertSentLow = false;
#define ALERT_CONFIG_FILE "/alertconfig.txt"

// plik konfiguracji
#define CONFIG_FILE "/ipconfig.txt"

// Temperatura inicjalizacja 

const int Temp = 32;
OneWire oneWire(Temp);
DallasTemperature sensors(&oneWire);

struct TempPair {
  String c;
  String f;
};

//// FUNKCJE ////

void syncTime() {
  Serial.println("[TIME] Syncing with NTP...");
  
  // Set timezone (Poland)
  configTzTime("CET-1CEST,M3.5.0/2,M10.5.0/3", "pool.ntp.org", "time.google.com", "time.windows.com");
  
  // Wait for time to sync (with timeout)
  int attempts = 0;
  while (attempts < 20) {  // 20 seconds timeout
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 1000)) {
      Serial.printf("[TIME] Synced: %04d-%02d-%02d %02d:%02d:%02d\n", 
                    timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                    timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
      return;
    }
    Serial.print(".");
    attempts++;
  }
  
  Serial.println("\n[TIME] ⚠️ Time sync failed! Will retry later.");
}

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

    if (useDHCP) {
      staticIP = currentIP;
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
        ETH.config(INADDR_NONE, INADDR_NONE, INADDR_NONE); 
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

TempPair GetTemperature(){
    TempPair temp;
    temp.c = String(sensors.getTempCByIndex(0));
    temp.f = String(sensors.getTempFByIndex(0));
    String js = "{\"c\":" + temp.c + ",\"f\":" + temp.f + "}";
    webSocket.broadcastTXT(js);

    return temp;
}

// Alerty WhatsApp

void sendWhatsAppAlert(String message) {
  if (recipientPhone == "" || !alertsEnabled) return;

  String formattedPhone = recipientPhone;
  
  String encodedMessage = urlEncode(message);
  
  String url = "https://api.callmebot.com/whatsapp.php?phone=" + formattedPhone + 
               "&text=" + encodedMessage + "&apikey=" + whatsappAPIKey;

  HTTPClient http;
  http.begin(url);
  
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    Serial.println("[WhatsApp] Alert sent successfully");
  } else {
    Serial.printf("[WhatsApp] Error sending alert: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}

// Config alertów WhatsApp - zapisz

void saveAlertConfig() {
  File file = FILESYSTEM.open(ALERT_CONFIG_FILE, "w");
  if (!file) {
    Serial.println("[Alert] Failed to open config file for writing");
    return;
  }
  
  file.println(alertsEnabled ? "1" : "0");
  file.println(highTempLimit);
  file.println(lowTempLimit);
  file.println(recipientPhone);
  file.println(whatsappAPIKey);
  
  file.close();
  Serial.println("[Alert] Configuration saved");
}

// Config alertów WhatsApp - wczytanie

void loadAlertConfig() {
  if (!FILESYSTEM.exists(ALERT_CONFIG_FILE)) {
    Serial.println("[Alert] No config file, using defaults");
    return;
  }
  
  File file = FILESYSTEM.open(ALERT_CONFIG_FILE, "r");
  if (!file) {
    Serial.println("[Alert] Failed to open config file");
    return;
  }
  
  alertsEnabled = file.readStringUntil('\n').toInt();
  highTempLimit = file.readStringUntil('\n').toFloat();
  lowTempLimit = file.readStringUntil('\n').toFloat();
  recipientPhone = file.readStringUntil('\n');
  recipientPhone.trim();
  whatsappAPIKey = file.readStringUntil('\n');
  whatsappAPIKey.trim();
  
  file.close();
  Serial.println("[Alert] Configuration loaded");
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

  if (!SD_MMC.begin("/sdcard", true)) {
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
  
 if (eth_connected) {
  // We have internet, sync time
  syncTime();
  } else {
  Serial.println("[TIME] ⚠️ No network, time will be incorrect!");
  }

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

  // JSON - status ok
  server.send(200, "application/json", "{\"status\":\"ok\",\"reboot\":true}");

  delay(500);

  useDHCP = willUseDHCP;
  if (!willUseDHCP) {
    staticIP = newStaticIP;
    gateway = newGateway;
    subnet = newSubnet;
    dns1 = newDNS1;
    dns2 = newDNS2;
  }

  saveIPConfig();
  applyNetworkConfig();

  delay(100);
});


  server.on("/api/history", HTTP_GET, []() {
  if (!server.hasArg("start") || !server.hasArg("end")) {
    server.send(400, "application/json", "{\"error\":\"Missing date range\"}");
    return;
  }

  sendHistoryJson(server.arg("start"), server.arg("end"));
  });

  server.on("/api/alertconfig", HTTP_GET, []() {
    String json = "{";
    json += "\"high\":" + String(highTempLimit, 1) + ",";
    json += "\"low\":" + String(lowTempLimit, 1) + ",";
    json += "\"phone\":\"" + recipientPhone + "\",";
    json += "\"apikey\":\"" + whatsappAPIKey + "\",";
    json += "\"alertsEnabled\":" + String(alertsEnabled ? "true" : "false");
    json += "}";
    server.send(200, "application/json", json);
  });

  server.on("/api/alertconfig", HTTP_POST, []() {
    if (!server.hasArg("plain")) {
      server.send(400, "application/json", "{\"error\":\"Bad request\"}");
      return;
    }

    String body = server.arg("plain");
    DynamicJsonDocument doc(512);
    deserializeJson(doc, body);

    highTempLimit = doc["high"];
    lowTempLimit = doc["low"];
    recipientPhone = doc["phone"].as<String>();
    whatsappAPIKey = doc["apikey"].as<String>();
    alertsEnabled = doc["alertsEnabled"];
    
    saveAlertConfig();
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  });

  server.on("/api/testalert", HTTP_POST, []() {
    if (recipientPhone == "" || !alertsEnabled) {
      server.send(400, "application/json", "{\"error\":\"Alerts not configured\"}");
      return;
    }
    
    String message = "Alert informacyjny\n"
                   "Konfiguracja alertów powiodła się!\n"
                   "Limit górny: " + String(highTempLimit, 1) + "°C\n"
                   "Limit dolny: " + String(lowTempLimit, 1) + "°C";
    
    sendWhatsAppAlert(message);
    server.send(200, "application/json", "{\"status\":\"Test alert sent\"}");
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
        if (alertsEnabled && recipientPhone != "") {
      if (tempValc > highTempLimit && !alertSentHigh) {
        String message = "⚠️ Wysoka temperatura!\n"
                       "Obecnie jest: " + String(tempValc, 1) + "°C (" + String(tempValf, 1) + "°F)\n"
                       "Limit: " + String(highTempLimit, 1) + "°C\n"
                       "Czas: " + getTimestamp();
        sendWhatsAppAlert(message);
        alertSentHigh = true;
        alertSentLow = false;
      } 
      else if (tempValc < lowTempLimit && !alertSentLow) {
        String message = "⚠️ Niska temperatura!\n"
                       "Obecnie jest: " + String(tempValc, 1) + "°C (" + String(tempValf, 1) + "°F)\n"
                       "Limit: " + String(lowTempLimit, 1) + "°C\n"
                       "Czas: " + getTimestamp();
        sendWhatsAppAlert(message);
        alertSentLow = true;
        alertSentHigh = false;
      }
      else if (tempValc <= highTempLimit && tempValc >= lowTempLimit) {
        alertSentHigh = false;
        alertSentLow = false;
      }
    }

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