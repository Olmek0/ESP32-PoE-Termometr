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
