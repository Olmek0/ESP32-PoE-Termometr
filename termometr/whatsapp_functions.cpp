#include "whatsapp_functions.h"
#include "general_functions.h"

//#include <LittleFS.h>
//#include <FS.h>

// Alerty WhatsApp

void sendWhatsAppAlert(String message) {
  if (recipientPhone == "" || !alertsEnabled) return;

  String formattedPhone = recipientPhone;
  
  String encodedMessage = urlEncode(message);
  
  String url = "https://api.callmebot.com/whatsapp.php?phone=" + formattedPhone + 
               "&text=" + encodedMessage + "&apikey=" + whatsappAPIKey;

  HTTPClient http;
  http.begin(url);
  
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    Serial.println("[WhatsApp] Alert sent successfully");
  } else {
    Serial.printf("[WhatsApp] Error sending alert: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}

// Config alertów WhatsApp - zapisz

void saveAlertConfig() {
  File file = LittleFS.open(ALERT_CONFIG_FILE, "w");
  if (!file) {
    Serial.println("[Alert] Failed to open config file for writing");
    return;
  }
  
  file.println(alertsEnabled ? "1" : "0");
  file.println(testEnabled ? "1" : "0");
  file.println(highTempLimit);
  file.println(lowTempLimit);
  file.println(highTempLimitF);
  file.println(lowTempLimitF);
  file.println(alertUseFahrenheit ? "1" : "0"); // <-- Added
  file.println(recipientPhone);
  file.println(whatsappAPIKey);
  
  file.close();
  Serial.println("[Alert] Configuration saved");
}

// Config alertów WhatsApp - wczytanie

void loadAlertConfig() {
  if (!LittleFS.exists(ALERT_CONFIG_FILE)) {
    Serial.println("[Alert] No config file, using defaults");
    return;
  }
  
  File file = LittleFS.open(ALERT_CONFIG_FILE, "r");
  if (!file) {
    Serial.println("[Alert] Failed to open config file");
    return;
  }
  String tempStr;
  
  tempStr = file.readStringUntil('\n'); tempStr.trim(); 
  alertsEnabled = tempStr.toInt();

  tempStr = file.readStringUntil('\n'); tempStr.trim(); 
  testEnabled = tempStr.toInt();

  tempStr = file.readStringUntil('\n'); tempStr.trim(); 
  highTempLimit = tempStr.toFloat();

  tempStr = file.readStringUntil('\n'); tempStr.trim(); 
  lowTempLimit = tempStr.toFloat();
  tempStr = file.readStringUntil('\n'); tempStr.trim(); 
  highTempLimitF = tempStr.toFloat();
  tempStr = file.readStringUntil('\n'); tempStr.trim(); 
  lowTempLimitF = tempStr.toFloat();
  
  tempStr = file.readStringUntil('\n'); tempStr.trim(); 
  alertUseFahrenheit = tempStr.toInt();

  recipientPhone = file.readStringUntil('\n'); recipientPhone.trim();
  whatsappAPIKey = file.readStringUntil('\n'); whatsappAPIKey.trim();
  
  file.close();
  Serial.println("[Alert] Configuration loaded");
}

void checkAndSendAlerts(bool useFahrenheit, float currentTemp, String timestamp) {
  if (!alertsEnabled || recipientPhone == "") return;

  // Select the appropriate limits based on the configuration unit
  float highLimit = useFahrenheit ? highTempLimitF : highTempLimit;
  float lowLimit  = useFahrenheit ? lowTempLimitF : lowTempLimit;
  String unitStr  = useFahrenheit ? "°F" : "°C";

  if (currentTemp > highLimit && !alertSentHigh) {
    String message = "Wysoka temperatura!\n"
                     "Obecnie jest: " + String(currentTemp, 1) + unitStr + "\n"
                     "Limit: " + String(highLimit, 1) + unitStr + "\n"
                     "Czas: " + timestamp;
    sendWhatsAppAlert(message);
    alertSentHigh = true;
    alertSentLow = false;
  } 
  else if (currentTemp < lowLimit && !alertSentLow) {
    String message = "Niska temperatura!\n"
                     "Obecnie jest: " + String(currentTemp, 1) + unitStr + "\n"
                     "Limit: " + String(lowLimit, 1) + unitStr + "\n"
                     "Czas: " + timestamp;
    sendWhatsAppAlert(message);
    alertSentLow = true;
    alertSentHigh = false;
  }
  else if (currentTemp <= highLimit && currentTemp >= lowLimit) {
    alertSentHigh = false;
    alertSentLow = false;
  }
}
