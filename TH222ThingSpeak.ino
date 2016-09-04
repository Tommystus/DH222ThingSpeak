/* DHTServer - ESP8266 Webserver with a DHT sensor as an input

   Based on ESP8266Webserver, DHTexample, and BlinkWithoutDelay (thank you)

   Version 1.0  5/3/2014  Version 1.0   Mike Barela for Adafruit Industries
*/
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <DHT.h>
#include <TimeLib.h>
#include "ThingSpeak.h"

#define DHTTYPE DHT22

const int DHTPIN = D4;

const char* ssid     = "myssid";
const char* password = "passwd";

ESP8266WebServer server(80);

 /*============ ThingSpeak setup ==================*/
// Note:  Each channel has its own number and write API key
// API key is what get used - wrong channel number doesn't matter

// Temperature Humidity Channel
static unsigned long myChannelNumber = 123456;
static const char 	*myWriteAPIKey = "XXXXXXXXXXXXXXXX";

/*============ End ThingSpeak setup ==============*/


// Initialize DHT sensor 
// NOTE: For working with a faster than ATmega328p 16 MHz Arduino chip, like an ESP8266,
// you need to increase the threshold for cycle counts considered a 1 or 0.
// You can do this by passing a 3rd parameter for this threshold.  It's a bit
// of fiddling to find the right value, but in general the faster the CPU the
// higher the value.  The default for a 16mhz AVR is a value of 6.  For an
// Arduino Due that runs at 84mhz a value of 30 works.
// This is for the ESP8266 processor on ESP-01 
DHT dht(DHTPIN, DHTTYPE, 11); // 11 works fine for ESP8266
 
float lastHumdty, lastTempF;  // Values read from sensor
static int nextSampleMinute = 15;
static int minuteSampleInterval = 15;

// Generally, you should use "unsigned long" for variables that hold time
unsigned long previousMillis = 0;        // will store last temp was read
const long interval = 2000;              // interval at which to read sensor

void getTemperature() {
  // Wait at least 2 seconds seconds between measurements.
  // if the difference between the current time and last time you read
  // the sensor is bigger than the interval you set, read the sensor
  // Works better than delay for things happening elsewhere also
  unsigned long currentMillis = millis();
 
  if(currentMillis - previousMillis >= interval) {
    // save the last time you read the sensor 
    previousMillis = currentMillis;   

    // Reading temperature for humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (it's a very slow sensor)
    lastHumdty = dht.readHumidity();          // Read humidity (percent)
    lastTempF = dht.readTemperature(true);     // Read temperature as Fahrenheit
    // Check if any reads failed and exit early (to try again).
    if (isnan(lastHumdty) || isnan(lastTempF)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }
  }
}

static int uploadData(void)
{
	ThingSpeak.setField( 1, lastTempF);
	ThingSpeak.setField( 2, lastHumdty);
	int rc = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
//	Serial.println(String("Post rc=")+rc);
	return rc;
}

 
const String HTML_TYPE = "text/html";

void handle_null() {
    Serial.println("Handle null");
	server.send(200,HTML_TYPE,"");
	delay(100);
}
void handleRoot()
{
	String wstr = 
	"<p>Hello from the weather esp8266, read from /temp or /humidity"
	"<br>set <a href=/set?minuteSampleInterval=5>5 minute</a> sample interval"
	", <a href=/set?nextSampleMinute=5>sample at 15 </a> minute"
	"<br><a href=/sample>Sample Data</a>"
	"<br><a href=/temp>Get Temperature</a>"
	"<br><a href=/humidity>Get Humidity</a>";
	wstr += "<br>Last temperature: " + String(lastTempF)+"F";
	wstr += " humidity: " + String(lastHumdty)+"%";
	wstr += "<br>Next sample time: " + String(nextSampleMinute)+"minute";
	server.send(200,HTML_TYPE, wstr);
	delay(100);
}
void doSample()
{
	Serial.println("DoSample");
	getTemperature();
	int rc = uploadData();
	String wstr = "<p>Sample data:  ";
	wstr += "<br>Temperature " + String(lastTempF);
	wstr += "<br>Humidity " + String(lastHumdty);
	wstr += "<br>Upload thingspeak rc=" + String(rc);
	server.send(200,HTML_TYPE, wstr);
}
 
// Web Service Definition structure
// =================================
typedef struct s_WebServiceDef
{
	const char *urlName;
	void (*doit)(void);
} WebServiceDef;

/* =======================================================================
	Define web service definition here
	Each entry has a URL name and a lambda function to service the request
   =======================================================================
*/
static WebServiceDef wsd[] = {
	{ "/", handleRoot },
	{ "/favicon", handle_null },
	{ "/temp",			// url name
		[] ()				// lambda function
		{
			getTemperature();       // read sensor
			String wstr="Temperature: "+String((int)lastTempF)+" F";
			server.send(200, "text/plain", wstr);   // send to someones browser when asked
		}
	},
	{ "/humidity", []()
		{ 
			getTemperature();
			String wstr="Humidity: "+String((int)lastHumdty)+"%";
			server.send(200, "text/plain", wstr);
		}
	},
	{ "/sample", doSample },
	{ "/set", []()
		{
			String n = String("minuteSampleInterval");
			if (server.hasArg(n))
			{
				int v = server.arg(n).toInt();
				if (v)
					minuteSampleInterval = v;
			}
			n = String("nextSampleMinute");
			if (server.hasArg(n))
			{
				int v = server.arg(n).toInt();
				if (v)
					nextSampleMinute = v;
			}
		}
	},
 	{0,0}
};

static void setupServerHandler(void)
{
	for (int i; wsd[i].urlName; i++)
	{
		server.on( wsd[i].urlName, wsd[i].doit);
	}
	server.onNotFound(handleRoot);
}

static int nextAlarmMin(int sampleInterval)
{
	int nextMin = minute(now());
	nextMin = ((nextMin + sampleInterval) % 60);	// alarm every 5 min
	return nextMin;
}



void setup(void)
{
	// You can open the Arduino IDE Serial Monitor window to see what the code is doing
	Serial.begin(115200);  // Serial connection from ESP-01 via 3.3v console cable
	dht.begin();           // initialize temperature sensor

	// Connect to WiFi network
	WiFi.begin(ssid, password);
	Serial.print("\n\r \n\rWorking to connect");

	// Wait for connection
	while (WiFi.status() != WL_CONNECTED) {
	delay(500);
	Serial.print(".");
	}
	Serial.println("");
	Serial.println("DHT Weather Reading Server");
	Serial.print("Connected to ");
	Serial.println(ssid);
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());

	setupServerHandler();

	server.begin();
	Serial.println("HTTP server started");
}
 
void loop(void)
{
	int currentMinute = minute(now());

	if (currentMinute == nextSampleMinute)
	{
		// trigger sample at set interval
		Serial.println("Regular sample");
		getTemperature();
		int rc = uploadData();
		Serial.println("rc=" + String(rc));

		nextSampleMinute = nextAlarmMin(minuteSampleInterval);
		delay(1000);
	}

	server.handleClient();
} 

