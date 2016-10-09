# DHT22 data to ThingSpeak using ESP8266
ThingSpeak IOT using DHT22 sensor with ESP8266.
Download and edit code, change WiFi SSID and password to match your local setting.  Also set your ThingSpeak channel and write API key.
Requirements:
- Arduino 1.65+
- ESP8266 board manager
- Time library https://github.com/PaulStoffregen/Time (http://playground.arduino.cc/Code/Time)
- DHT22 basic sensor library from AdaFruit https://github.com/adafruit/DHT-sensor-library
- Any ESP8266 development boards and DHT22 module

Wiring:
  ESP8266-D4--->DHT22.pin#2; pin#1->Vcc; pin#4->Gnd

