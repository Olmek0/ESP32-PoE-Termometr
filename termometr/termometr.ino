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
#include "web_functions.h"
#include "whatsapp_functions.h"
#include "general_functions.h"

//// INICJALIZACJA ////

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

IPAddress lastIP;

// WhatsApp 
String whatsappAPIKey = ""; 
String recipientPhone = "";
float highTempLimit = 30.0;
float lowTempLimit = 10.0;

float highTempLimitF = 86.0;
float lowTempLimitF = 50.0;

bool alertsEnabled = false;

bool testEnabled = false;

bool alertSentHigh = false;
bool alertSentLow = false;

bool alertUseFahrenheit = false;

String timezone = "CET-1CEST,M3.5.0/2,M10.5.0/3";

// Temperatura inicjalizacja 

const int Temp = 32;
OneWire oneWire(Temp);
DallasTemperature sensors(&oneWire);

const int RESET_BUTTON_PIN = 34;

//// PROGRAM

void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(RESET_BUTTON_PIN, INPUT);

  Serial.println("[BOOT] Starting...");

  if (!LittleFS.begin(true)) {
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

  loadAlertConfig(); 
  loadIPConfig();

  WiFi.onEvent(WiFiEvent);
  delay(100); 
  ETH.begin();
  
  applyNetworkConfig();

  loadTimezoneConfig(); 

  Serial.print("[ETH] Waiting for IP");
  unsigned long startAttempt = millis();
  while (!eth_connected && millis() - startAttempt < 15000) {
    delay(100);
    Serial.print(".");
  }
  Serial.println();
  
  if (!eth_connected) {
    if (!useDHCP) {
      Serial.println("\n[ETH] ⚠️ Static IP failed to connect! Reverting to DHCP...");
      useDHCP = true;
      saveIPConfig();
      delay(500);
      ESP.restart();
    } else {
      Serial.println("[TIME] ⚠️ No network on DHCP, time will be incorrect!");
    }
  } else {
    applyTimezone();
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
    DynamicJsonDocument doc(512); 

    doc["dhcp"] = useDHCP;
    doc["ip"] = useDHCP ? ETH.localIP().toString() : staticIP.toString();
    doc["gateway"] = useDHCP ? ETH.gatewayIP().toString() : gateway.toString();
    doc["subnet"] = useDHCP ? ETH.subnetMask().toString() : subnet.toString();
    doc["dns1"] = useDHCP ? ETH.dnsIP(0).toString() : dns1.toString();
    doc["dns2"] = useDHCP ? ETH.dnsIP(1).toString() : dns2.toString();

    String output;
    serializeJson(doc, output);
    server.send(200, "application/json", output);
  });

  server.on("/api/ipconfig", HTTP_POST, []() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"Bad request\"}");
    return;
  }

  String body = server.arg("plain");
  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, body);

  if (error) {
    server.send(400, "application/json", "{\"error\":\"Invalid JSON\",\"message\":\"" + String(error.c_str()) + "\"}");
    return;
  }

  bool willUseDHCP = doc["dhcp"];
  IPAddress newStaticIP, newGateway, newSubnet, newDNS1, newDNS2;

  if (!willUseDHCP) {
    if (!newStaticIP.fromString(doc["ip"].as<const char*>()) ||
        !newGateway.fromString(doc["gateway"].as<const char*>()) ||
        !newSubnet.fromString(doc["subnet"].as<const char*>()) ||
        !newDNS1.fromString(doc["dns1"].as<const char*>()) ||
        !newDNS2.fromString(doc["dns2"].as<const char*>())) {
      
      server.send(400, "application/json", "{\"error\":\"Invalid IP format\",\"message\":\"One or more IP addresses are invalid\"}");
      return;
    }
  }
  
  server.send(200, "application/json", "{\"status\":\"ok\",\"reboot\":true}");
  
  delay(100);

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

  });

  server.on("/api/history", HTTP_GET, []() {
  if (!server.hasArg("start") || !server.hasArg("end")) {
    server.send(400, "application/json", "{\"error\":\"Missing date range\"}");
    return;
  }

  sendHistoryJson(server.arg("start"), server.arg("end"));
  });

  server.on("/api/alertconfig", HTTP_GET, []() {
    DynamicJsonDocument doc(512); 

    doc["high"] = highTempLimit;
    doc["low"] = lowTempLimit;
    doc["highF"] = highTempLimitF;
    doc["lowF"] = lowTempLimitF;
    doc["phone"] = recipientPhone;
    doc["apikey"] = whatsappAPIKey;
    doc["alertsEnabled"] = alertsEnabled;
    doc["testEnabled"] = testEnabled;
    doc["useFahrenheit"] = alertUseFahrenheit;

    String jsonResponse;
    serializeJson(doc, jsonResponse);
    server.send(200, "application/json", jsonResponse);

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
    highTempLimitF = doc["highF"];
    lowTempLimitF = doc["lowF"];
    recipientPhone = doc["phone"].as<String>();
    whatsappAPIKey = doc["apikey"].as<String>();
    alertsEnabled = doc["alertsEnabled"];
    testEnabled = doc["testEnabled"];
    alertUseFahrenheit = doc["useFahrenheit"];
    
    saveAlertConfig();
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  });

  server.on("/api/testalert", HTTP_POST, []() {
    if (recipientPhone == "" || !alertsEnabled) {
      server.send(400, "application/json", "{\"error\":\"Alerts not configured\"}");
      return;
    }

    float testTempLow = 0.0;
    float testTempHigh = 0.0;

    String testUnit;

    if (alertUseFahrenheit) {
          testTempHigh = highTempLimitF;
          testTempLow = lowTempLimitF;
          testUnit = "°F";
    } else {
          testTempHigh = highTempLimit;
          testTempLow = lowTempLimit;
          testUnit = "°C";
    }

    String message = "Alert informacyjny\n"
                   "Konfiguracja alertów powiodła się!\n"
                   "Limit górny: " + String(testTempHigh, 1) + testUnit + "  qqq2\n"
                   "Limit dolny: " + String(testTempLow, 1) + testUnit;
    
    sendWhatsAppAlert(message);
    server.send(200, "application/json", "{\"status\":\"Test alert sent\"}");
  });

  server.on("/api/timezone", HTTP_GET, []() {
    DynamicJsonDocument doc(256);
    doc["timezone"] = timezone;
    doc["available_timezones"] = getAvailableTimezones();
    
    String output;
    serializeJson(doc, output);
    server.send(200, "application/json", output);
  });

  server.on("/api/timezone", HTTP_POST, []() {
    if (!server.hasArg("plain")) {
      server.send(400, "application/json", "{\"error\":\"Bad request\"}");
      return;
    }
    
    String body = server.arg("plain");
    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, body);
    
    if (error) {
      server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
      return;
    }
    
    String newTimezone = doc["timezone"].as<String>();
    
    if (newTimezone.length() < 3) {
      server.send(400, "application/json", "{\"error\":\"Invalid timezone format\"}");
      return;
    }
    
    timezone = newTimezone;
    saveTimezoneConfig();
    
    applyTimezone();
    
    server.send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Timezone updated\"}");
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
const unsigned long tempInterval = 5000; //5s
const unsigned long dbInsertInterval = 300000; // 5min 300000

unsigned long lastDisconnectTime = 0;

void loop() {
  checkResetButton();

  server.handleClient();
  webSocket.loop();
  
  checkIPChange();

  if (!eth_connected) {
    if (lastDisconnectTime == 0) {
      lastDisconnectTime = millis();
    }
    
    if (millis() - lastDisconnectTime > 30000) {
      Serial.println("[WATCHDOG] Ethernet connection stuck. Rebooting to recover PHY chip...");
      delay(500);
      ESP.restart();
    }
  } else {
    lastDisconnectTime = 0;
  }
  
  if (millis() - lastDbInsert >= dbInsertInterval) {
    lastDbInsert = millis();
    sensors.requestTemperatures();
    float tempValc = sensors.getTempCByIndex(0);
    float tempValf = sensors.getTempFByIndex(0);
    String timestamp = getTimestamp();
    logTemperature(tempValc, tempValf, timestamp);
    Serial.println("[DB] Logged temperature: " + String(tempValc) + " / " + String(tempValf) + " at " + timestamp);
    if (alertsEnabled) {
        float alertTemp = 0.0;
        
        if (alertUseFahrenheit) {
            alertTemp = tempValf;
        } else {
            alertTemp = tempValc;
        }
        
        checkAndSendAlerts(alertUseFahrenheit, alertTemp, timestamp);
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