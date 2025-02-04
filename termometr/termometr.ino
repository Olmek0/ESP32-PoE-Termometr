
#include <OneWire.h>
#include <DallasTemperature.h>

const int Temp = 32;
OneWire oneWire(Temp);
DallasTemperature sensors(&oneWire);
String temperature;

void setup() {

    Serial.begin(115200);
    sensors.begin();
    // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:
    sensors.requestTemperatures();
    temperature = (String)sensors.getTempCByIndex(0);
    Serial.print(temperature+'\n');
    delay(5000);
}
