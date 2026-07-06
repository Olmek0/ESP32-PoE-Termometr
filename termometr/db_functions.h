#ifndef DB_FUNCTIONS
#define DB_FUNCTIONS

#include "sqlite3.h"
#include <Arduino.h>

extern sqlite3 *db; // sql database
extern char *zErrMsg; // błąd w sql

void initDatabase();
void logTemperature(float tempC, float tempF, const String& timestamp);
void sendStatsOverWebSocket();
void sendHistoryJson(const String& start, const String& end);
void sendChartData();

#endif