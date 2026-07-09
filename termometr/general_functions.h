#ifndef GENERAL_FUNCTIONS
#define GENERAL_FUNCTIONS

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoJson.h>

extern const int Temp;
extern OneWire oneWire;
extern DallasTemperature sensors;

struct TempPair {
  String c;
  String f;
};

String getTimestamp();
TempPair GetTemperature();
void syncTime();

#endif