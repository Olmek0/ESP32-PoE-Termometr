#include "web_functions.h"

void saveIPConfig() {
  File file = LittleFS.open(CONFIG_FILE, "w");
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
  if (!LittleFS.exists(CONFIG_FILE)) {
    Serial.println("[IP] No config file, using defaults");
    return;
  }
  
  File file = LittleFS.open(CONFIG_FILE, "r");
  if (!file) {
    Serial.println("[IP] Failed to open config file");
    return;
  }
  
  String mode = file.readStringUntil('\n');
  mode.trim();
  useDHCP = (mode == "dhcp");
  
  if (!useDHCP) {
    String ipStr = file.readStringUntil('\n');
    ipStr.trim();
    staticIP.fromString(ipStr);
    
    String gwStr = file.readStringUntil('\n');
    gwStr.trim();
    gateway.fromString(gwStr);
    
    String subStr = file.readStringUntil('\n');
    subStr.trim();
    subnet.fromString(subStr);
    
    String dns1Str = file.readStringUntil('\n');
    dns1Str.trim();
    dns1.fromString(dns1Str);
    
    String dns2Str = file.readStringUntil('\n');
    dns2Str.trim();
    dns2.fromString(dns2Str);
  }
  
  file.close();
  Serial.println("[IP] Configuration loaded");
}

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
  File file = LittleFS.open(path.c_str(), "r");
  if (!file) {
    server.send(500, "text/plain", "Failed to open /index.html");
    return;
  }
  server.streamFile(file, content);
  file.close();
}
