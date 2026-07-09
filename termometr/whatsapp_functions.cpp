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
  
  tempStr = file.readStringUntil('\n');
  tempStr.trim();
  alertsEnabled = tempStr.toInt();

  tempStr = file.readStringUntil('\n');
  tempStr.trim();
  testEnabled = tempStr.toInt();

  tempStr = file.readStringUntil('\n');
  tempStr.trim();
  highTempLimit = tempStr.toFloat();

  tempStr = file.readStringUntil('\n');
  tempStr.trim();
  lowTempLimit = tempStr.toFloat();

  recipientPhone = file.readStringUntil('\n');
  recipientPhone.trim();
  whatsappAPIKey = file.readStringUntil('\n');
  whatsappAPIKey.trim();
  
  file.close();
  Serial.println("[Alert] Configuration loaded");
}

void checkAndSendAlerts(float tempC, float tempF, String timestamp) {
  
  if (!alertsEnabled && recipientPhone == "") {
    return;
  }
  if (tempC > highTempLimit && !alertSentHigh) {
    String message = "Wysoka temperatura!\n"
                    "Obecnie jest: " + String(tempC, 1) + "°C (" + String(tempF, 1) + "°F)\n"
                    "Limit: " + String(highTempLimit, 1) + "°C\n"
                    "Czas: " + getTimestamp();
    sendWhatsAppAlert(message);
    alertSentHigh = true;
    alertSentLow = false;
  } 
  else if (tempC < lowTempLimit && !alertSentLow) {
    String message = "Niska temperatura!\n"
                    "Obecnie jest: " + String(tempC, 1) + "°C (" + String(tempF, 1) + "°F)\n"
                    "Limit: " + String(lowTempLimit, 1) + "°C\n"
                    "Czas: " + getTimestamp();
    sendWhatsAppAlert(message);
    alertSentLow = true;
    alertSentHigh = false;
  }
  else if (tempC <= highTempLimit && tempC >= lowTempLimit) {
    alertSentHigh = false;
    alertSentLow = false;
  }
}
