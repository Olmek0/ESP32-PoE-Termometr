#include <OneWire.h>
#include <DallasTemperature.h>
#include <ETH.h>
#include <WebServer.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <WebSocketsServer.h>

#define FILESYSTEM LittleFS
WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

bool eth_connected = false;

const int Temp = 32;
OneWire oneWire(Temp);
DallasTemperature sensors(&oneWire);
String temperature;

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

void handleRoot(String path, String content) {
  File file = FILESYSTEM.open(path.c_str(), "r");
  if (!file) {
    server.send(500, "text/plain", "Failed to open /index.html");
    return;
  }
  server.streamFile(file, content);
  file.close();
}

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

  WiFi.onEvent(WiFiEvent);
  delay(100);
  ETH.begin();

  for (int i = 0; i < 100 && !eth_connected; i++) {
    delay(100);
    Serial.print(".");
  }
  Serial.println();

  server.on("/", HTTP_GET, []() {handleRoot("/index.html","text/html"); });
  server.on("/styl.css", HTTP_GET, []() {handleRoot("/styl.css","text/css"); });
  server.on("/skrypt.js", HTTP_GET, []() {handleRoot("/skrypt.js","application/javascript"); });

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

unsigned long lastTempRequest = 0;
const unsigned long tempInterval = 5000;

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
}
