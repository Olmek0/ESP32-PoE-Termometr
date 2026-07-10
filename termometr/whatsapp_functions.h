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

extern float highTempLimitF;
extern float lowTempLimitF;

extern bool alertUseFahrenheit;

extern bool alertsEnabled;
extern bool testEnabled;

extern bool alertSentHigh;
extern bool alertSentLow;

void sendWhatsAppAlert(String message);
void saveAlertConfig();
void loadAlertConfig();
void checkAndSendAlerts(bool useFahrenheit, float currentTemp, String timestamp);

#endif