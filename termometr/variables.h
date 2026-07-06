#ifndef VARIABLES
#define VARIABLES

#include <Arduino.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

WebServer server(80); // port 80
WebSocketsServer webSocket = WebSocketsServer(81); // port 81

#endif