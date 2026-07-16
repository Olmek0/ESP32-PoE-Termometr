#include "general_functions.h"
#include "web_functions.h"

// 

String getTimestamp() {
  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);
  char buf[20];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &timeinfo);
  return String(buf);
}

TempPair GetTemperature(){
    TempPair temp;
    temp.c = String(sensors.getTempCByIndex(0));
    temp.f = String(sensors.getTempFByIndex(0));
    String js = "{\"c\":" + temp.c + ",\"f\":" + temp.f + "}";
    webSocket.broadcastTXT(js);

    return temp;
}

void syncTime() {
  Serial.println("[TIME] Syncing with NTP using timezone: " + timezone);
  
  configTzTime(timezone.c_str(), "pool.ntp.org", "time.google.com", "time.windows.com");
  
  int attempts = 0;
  while (attempts < 20) { 
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 1000)) {
      Serial.printf("[TIME] Synced: %04d-%02d-%02d %02d:%02d:%02d\n", 
                    timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                    timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
      return;
    }
    Serial.print(".");
    attempts++;
  }
  
  Serial.println("\n[TIME] ⚠️ Time sync failed! Will retry later.");
}


void checkResetButton() {
  if (digitalRead(RESET_BUTTON_PIN) == LOW) {
    Serial.println("[RESET] Button detected! Hold for 5 seconds to reset network settings...");
    
    unsigned long pressStartTime = millis();
    bool fullyPressed = true;

    while (millis() - pressStartTime < 5000) {
      delay(50);
      if (digitalRead(RESET_BUTTON_PIN) == HIGH) {
        Serial.println("[RESET] Button released too early. Reset aborted.");
        fullyPressed = false;
        break;
      }
    }

    if (fullyPressed) {
      Serial.println("\n[RESET] Wiping network config...");
      
      useDHCP = true;
      saveIPConfig();
      
      Serial.println("[RESET] Done. Rebooting device...");
      delay(1000);
      ESP.restart();
    }
  }
}

void saveTimezoneConfig() {
  File file = LittleFS.open("/timezone.cfg", "w");
  if (!file) {
    Serial.println("[ERROR] Failed to open timezone.cfg for writing");
    return;
  }
  file.println(timezone);
  file.close();
  Serial.println("[CONFIG] Timezone saved: " + timezone);
}

void loadTimezoneConfig() {
  File file = LittleFS.open("/timezone.cfg", "r");
  if (!file) {
    Serial.println("[CONFIG] No timezone config found, using default");
    return;
  }
  String tz = file.readStringUntil('\n');
  tz.trim();
  if (tz.length() > 0) {
    timezone = tz;
    Serial.println("[CONFIG] Loaded timezone: " + timezone);
  }
  file.close();
}

void applyTimezone() {
  if (eth_connected) {
    Serial.println("[TIME] Applying timezone: " + timezone);
    configTzTime(timezone.c_str(), "pool.ntp.org", "time.google.com", "time.windows.com");
  }
}

String getAvailableTimezones() {
  DynamicJsonDocument doc(2048);
  JsonArray arr = doc.to<JsonArray>();
  
  arr.add("CET-1CEST,M3.5.0/2,M10.5.0/3"); // Central European Time
  arr.add("GMT0BST,M3.5.0/1,M10.5.0/2");   // UK/London
  arr.add("WET0WEST,M3.5.0/1,M10.5.0/2");  // Western European Time
  arr.add("EET-2EEST,M3.5.0/3,M10.5.0/4"); // Eastern European Time
  arr.add("CST6CDT");                       // US Central
  arr.add("EST5EDT");                       // US Eastern
  arr.add("MST7MDT");                       // US Mountain
  arr.add("PST8PDT");                       // US Pacific
  arr.add("AST4ADT");                       // Atlantic
  arr.add("NZST-12NZDT,M9.5.0/2,M4.1.0/3"); // New Zealand
  arr.add("JST-9");                         // Japan
  arr.add("AEST-10AEDT,M10.1.0/2,M4.1.0/3"); // Australia East
  arr.add("CST-8");                         // China
  arr.add("IST-5:30");                      // India
  
  String output;
  serializeJson(doc, output);
  return output;
}

