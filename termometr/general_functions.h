#ifndef GENERAL_FUNCTIONS
#define GENERAL_FUNCTIONS

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoJson.h>

extern const int Temp;
extern OneWire oneWire;
extern DallasTemperature sensors;
extern String timezone;

extern const int RESET_BUTTON_PIN;

struct TempPair {
  String c;
  String f;
};

String getTimestamp();
TempPair GetTemperature();
void syncTime();

void checkResetButton();

void saveTimezoneConfig();
void loadTimezoneConfig();
void applyTimezone();
String getAvailableTimezones();

#endif