#ifndef WHATSAPP_FUNCTIONS
#define WHATSAPP_FUNCTIONS

#include <Arduino.h>
#include <LittleFS.h>
#include "FS.h"
#include <HTTPClient.h>
#include <UrlEncode.h>

#define ALERT_CONFIG_FILE "/alertconfig.txt"

extern String whatsappAPIKey; 
extern String recipientPhone;
extern float highTempLimit;
extern float lowTempLimit;
extern bool alertsEnabled;
extern bool alertSentHigh;
extern bool alertSentLow;

void sendWhatsAppAlert(String message);
void saveAlertConfig();
void loadAlertConfig();
void checkAndSendAlerts(float tempC, float tempF, String timestamp);

#endif