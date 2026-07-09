#ifndef WEB_FUNCTIONS
#define WEB_FUNCTIONS

#include <Arduino.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <WiFi.h>
#include "FS.h"
#include <LittleFS.h>
#include <ETH.h>

#define CONFIG_FILE "/ipconfig.txt"

extern WebServer server; // port 80
extern WebSocketsServer webSocket; // port 81

extern bool eth_connected;

extern bool useDHCP;
extern IPAddress staticIP;
extern IPAddress gateway;
extern IPAddress subnet;
extern IPAddress dns1;
extern IPAddress dns2;
extern IPAddress lastIP;

void saveIPConfig();
void loadIPConfig();
void checkIPChange();
void applyNetworkConfig();
void WiFiEvent(WiFiEvent_t event);
void handleRoot(String path, String content);

#endif