#include "Arduino.h"
#include "secrets/heladera_secrets.h"
#include "secrets/shared_secrets.h"
#include "utils/iot_utils.hpp"
#include <Keypad.h>
#define SHADOW_GET_TOPIC "$aws/things/heladera/shadow/get"
#define SHADOW_GET_ACCEPTED_TOPIC "$aws/things/heladera/shadow/get/accepted"
#define SHADOW_UPDATE_TOPIC "$aws/things/heladera/shadow/update"
#define SHADOW_UPDATE_ACCEPTED_TOPIC "$aws/things/heladera/shadow/update/accepted"
#define SHADOW_UPDATE_DELTA_TOPIC "$aws/things/heladera/shadow/update/delta"
#define PIN_ELECTROIMAN D8

// -------------- KEYPAD CREATION ----------------- //
const byte n_rows = 4;
const byte n_cols = 4;
char keys[n_rows][n_cols] = {
	{ '1', '2', '3', 'A' },
	{ '4', '5', '6', 'B' },
	{ '7', '8', '9', 'C' },
	{ '*', '0', '#', 'D' }
};
byte rowPins[n_rows] = { D7, D6, D5, D0 };
byte colPins[n_cols] = { D1, D2, D3, D4 };
Keypad myKeypad = Keypad(makeKeymap(keys), rowPins, colPins, n_rows, n_cols);
// ----------- END OF KEYPAD CREATION ------------- //

void messageHandler(char* topic, byte* payload, unsigned int length)
{
	bool electroiman = 0;
	StaticJsonDocument<256> doc;
	Serial.printf("MESSAGE HANDLER: Topic: %s\n", topic);
	// ------------ RETRIEVING THE SHADOW DOCUMENT FROM AWS -------------//
	if (strcmp(topic, SHADOW_GET_ACCEPTED_TOPIC) == 0) {
		deserializeJson(doc, payload); // Put the info from the payload into the JSON document
		if (doc["state"]["desired"]["electroiman"] != nullptr) {
			electroiman = doc["state"]["desired"]["electroiman"];
			digitalWrite(PIN_ELECTROIMAN, electroiman);
		}
	}
	// ------------------------------------------------------------------//
	// ----------------- RETRIEVING THE DELTA FROM AWS ------------------//
	if (strcmp(topic, SHADOW_UPDATE_DELTA_TOPIC) == 0) {
		deserializeJson(doc, payload); // Put the info from the payload into the JSON document
		if (doc["state"]["electroiman"] != nullptr) {
			electroiman = doc["state"]["electroiman"];
			digitalWrite(PIN_ELECTROIMAN, electroiman);
		}
	}
	// ------------------------------------------------------------------//

#pragma region // Region that reports the current state to the shadow
	doc.clear(); // Clear the JSON document so it can be used to publish the current state
	electroiman = digitalRead(PIN_ELECTROIMAN);
	doc["state"]["reported"]["electroiman"] = electroiman;
	char jsonBuffer[256];
	serializeJsonPretty(doc, jsonBuffer);
	Serial.println("Reporting the following to the shadow:");
	Serial.println(jsonBuffer);
	client.publish(SHADOW_UPDATE_TOPIC, jsonBuffer);
#pragma endregion
}

void publishMessage(const char* topic, char myKey = ' ')
{
	String result = "No result";
	if (strcmp(topic, "heladera/teclado") == 0) {
		String myKeyStr = String(myKey);
		Serial.printf("Message publishing started for the topic %s\n", topic);
		StaticJsonDocument<200> doc;
		char jsonBuffer[200];
		doc["key"] = myKeyStr;
		serializeJson(doc, jsonBuffer);
		result = (client.publish("heladera/teclado", jsonBuffer)) ? "success publishing" : "failed publishing";
	}
	Serial.println(result);
}

void setup()
{
	Serial.begin(115200);
	connectAWS(WIFI_SSID, WIFI_PASSWORD, THINGNAME, AWS_CERT_CA, AWS_CERT_CRT, AWS_CERT_PRIVATE, AWS_IOT_ENDPOINT); // Connect to AWS
	client.subscribe(SHADOW_GET_ACCEPTED_TOPIC, 1); // Subscribe to the topic that gets the initial state
	client.subscribe(SHADOW_UPDATE_DELTA_TOPIC, 1); // Subscribe to the topic that updates the state every time it changes
	client.setCallback(messageHandler);
	pinMode(PIN_ELECTROIMAN, OUTPUT);

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
		reconnect(THINGNAME, "heladera/status");
	}
	client.loop();
	char myKey = myKeypad.getKey();
	if (myKey != NULL) {
		Serial.print("Key pressed: ");
		Serial.println(myKey);
		publishMessage("heladera/teclado", myKey); // Publish the pressed key to AWS
	}
	delay(100);
}