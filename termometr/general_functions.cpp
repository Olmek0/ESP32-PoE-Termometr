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
