#include "Arduino.h"
#include "secrets/luz_secrets.h"
#include "utils/iot_utils.hpp"
#define SHADOW_GET_TOPIC "$aws/things/luz/shadow/get"
#define SHADOW_GET_ACCEPTED_TOPIC "$aws/things/luz/shadow/get/accepted"
#define SHADOW_UPDATE_TOPIC "$aws/things/luz/shadow/update"
#define SHADOW_UPDATE_ACCEPTED_TOPIC "$aws/things/luz/shadow/update/accepted"
#define SHADOW_UPDATE_DELTA_TOPIC "$aws/things/luz/shadow/update/delta"
#define LED D8
byte brightness_level = 150;
int rn;
String mode;
unsigned long flicker_previousMillis = 0; // will store last time LED was updated
unsigned long flicker_interval = 0; // interval at which to make scary light effects (milliseconds)
unsigned long off_on_previousMillis = 0; // will store last time LED was updated
unsigned long off_on_interval = 0; // interval at which to make scary light effects (milliseconds)

void turn_off_on()
{
	const long interval = random(3000, 7000); // interval at which to blink (milliseconds)

	Serial.println("Turning off and on the light");
	for (int i = 0; i < 60; i++) {
		rn = random(10, brightness_level - i / 2);
		analogWrite(LED, rn);
		ESP.wdtFeed();
		delay(50);
	}
	digitalWrite(LED, LOW);
	// Serial.println("Starting the millis phase");
	unsigned long currentMillis = millis();
	while (1) {
		ESP.wdtFeed();
		if (millis() - currentMillis >= interval) {
			break;
		}
	}
	// Serial.println("Ending the millis phase");
	for (int i = 0; i < 60; i++) {
		rn = random(20, brightness_level);
		analogWrite(LED, rn);
		ESP.wdtFeed();
		delay(50);
	}
	analogWrite(LED, brightness_level);
	Serial.println("Stopped turning off and on the light");
}

void flicker()
{
	Serial.println("Flickering the light");
	for (int i = 0; i < 60; i++) {
		rn = random(brightness_level / 4, brightness_level);
		analogWrite(LED, rn);
		ESP.wdtFeed();
		delay(50);
	}
	analogWrite(LED, brightness_level);
	Serial.println("Stopped flickering the light");
}
void messageHandler(char* topic, byte* payload, unsigned int length)
{
	Serial.printf("MESSAGE HANDLER: Topic: %s\n", topic);
	StaticJsonDocument<256> doc;
	// ------------ RETRIEVING THE SHADOW DOCUMENT FROM AWS -------------//
	if (strcmp(topic, SHADOW_GET_ACCEPTED_TOPIC) == 0) {
		deserializeJson(doc, payload); // Put the info from the payload into the JSON document
		if (doc["state"]["desired"]["brightness_level"] != nullptr) {
			brightness_level = doc["state"]["desired"]["brightness_level"];
		}
		if (doc["state"]["desired"]["mode"] != nullptr) {
			// const char* the_mode = doc["state"]["desired"]["mode"];
			// strcpy(mode, the_mode); // This is done this way because it crashes when assigning the value to a char* instead of a const char*
			mode = doc["state"]["desired"]["mode"].as<String>();
		}
	}
	// ------------------------------------------------------------------//
	// ----------------- RETRIEVING THE DELTA FROM AWS ------------------//
	if (strcmp(topic, SHADOW_UPDATE_DELTA_TOPIC) == 0) {
		deserializeJson(doc, payload); // Put the info from the payload into the JSON document
		if (doc["state"]["brightness_level"] != nullptr) {
			brightness_level = doc["state"]["brightness_level"];
			analogWrite(LED, brightness_level);
		}
		if (doc["state"]["mode"] != nullptr) {
			// const char* the_mode = doc["state"]["mode"];
			// strcpy(mode, the_mode);
			mode = doc["state"]["mode"].as<String>();
		}
	}
	// ------------------------------------------------------------------//

#pragma region // Region that reports the current state to the shadow
	doc.clear(); // Clear the JSON document so it can be used to publish the current state
	doc["state"]["reported"]["brightness_level"] = brightness_level;
	doc["state"]["reported"]["mode"] = mode;
	char jsonBuffer[256];
	serializeJsonPretty(doc, jsonBuffer);
	Serial.println("Reporting the following to the shadow:");
	Serial.println(jsonBuffer);
	client.publish(SHADOW_UPDATE_TOPIC, jsonBuffer);
#pragma endregion
}

void setup()
{
	Serial.begin(115200);
	ESP.wdtDisable();
	ESP.wdtEnable(10000);
	pinMode(LED, OUTPUT);

	connectAWS(WIFI_SSID, WIFI_PASSWORD, THINGNAME, AWS_CERT_CA, AWS_CERT_CRT, AWS_CERT_PRIVATE, AWS_IOT_ENDPOINT); // Connect to AWS
	client.subscribe(SHADOW_GET_ACCEPTED_TOPIC, 1); // Subscribe to the topic that gets the initial state
	client.subscribe(SHADOW_UPDATE_DELTA_TOPIC, 1); // Subscribe to the topic that updates the state every time it changes
	client.setCallback(messageHandler);
	pinMode(LED, OUTPUT);
	// ----------- Get the Shadow document --------------//
	StaticJsonDocument<8> doc;
	char jsonBuffer[8];
	serializeJson(doc, jsonBuffer);
	// For some weird reason, the ESP8266 doesnt catch the get/accepted response at the first attemp, so it needs to be done twice
	client.publish(SHADOW_GET_TOPIC, jsonBuffer);
	delay(1000);
	client.publish(SHADOW_GET_TOPIC, jsonBuffer);
	// --------------------------------------------------//
}
void loop()
{
	now = time(nullptr); // The NTP server uses this, if you delete this, the connection to AWS no longer works
	if (!client.connected()) {
		reconnect(THINGNAME, "luz/status");
	}
	client.loop();
	if (mode == "fixed") {
		analogWrite(LED, brightness_level);
	} else if (mode == "scary") {
		unsigned long currentMillis = millis();
		if (currentMillis - flicker_previousMillis >= flicker_interval) {
			flicker_previousMillis = currentMillis;
			Serial.println("The interval for flickering has passed");
			flicker();
			flicker_interval = random(30000, 40000);
		}
		if (currentMillis - off_on_previousMillis >= off_on_interval) {
			off_on_previousMillis = currentMillis;
			Serial.println("The interval for off_on has passed");
			turn_off_on();
			off_on_interval = random(70000, 90000);
		}
	}
	delay(100);
}