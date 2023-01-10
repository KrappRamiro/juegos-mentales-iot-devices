#include "Arduino.h"
#include "secrets/heladera_secrets.h"
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
byte colPins[n_rows] = { D4, D5, D6, D7 };
byte rowPins[n_cols] = { D0, D1, D2, D3 };
Keypad myKeypad = Keypad(makeKeymap(keys), rowPins, colPins, n_rows, n_cols);
// ----------- END OF KEYPAD CREATION ------------- //

void messageHandler(char* topic, byte* payload, unsigned int length)
{
	bool electroiman = 0;
	StaticJsonDocument<256> doc;
	Serial.printf("MESSAGE HANDLER: Topic: %s\n", topic);
	if (strcmp(topic, SHADOW_GET_ACCEPTED_TOPIC) == 0) {
		deserializeJson(doc, payload); // Put the info from the payload into the JSON document
		if (doc["state"]["desired"]["electroiman"] != nullptr) {
			electroiman = doc["state"]["desired"]["electroiman"];
			digitalWrite(PIN_ELECTROIMAN, electroiman);
		}
	}
	if (strcmp(topic, SHADOW_UPDATE_DELTA_TOPIC) == 0) {
		deserializeJson(doc, payload); // Put the info from the payload into the JSON document
		if (doc["state"]["electroiman"] != nullptr) {
			electroiman = doc["state"]["electroiman"];
			digitalWrite(PIN_ELECTROIMAN, electroiman);
		}
	}

#pragma region // Region that reports the current state to the shadow
	doc.clear();
	electroiman = digitalRead(PIN_ELECTROIMAN);
	doc["state"]["reported"]["electroiman"] = electroiman;
	char jsonBuffer[256];
	serializeJsonPretty(doc, jsonBuffer); // print to client
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
		serializeJson(doc, jsonBuffer); // print to client
		result = (client.publish("heladera/teclado", jsonBuffer)) ? "success publishing" : "failed publishing";
	}
	Serial.println(result);
}

void setup()
{
	Serial.begin(115200);
	connectAWS(WIFI_SSID, WIFI_PASSWORD, THINGNAME, AWS_CERT_CA, AWS_CERT_CRT, AWS_CERT_PRIVATE, AWS_IOT_ENDPOINT);
	client.subscribe(SHADOW_GET_ACCEPTED_TOPIC, 1);
	client.subscribe(SHADOW_UPDATE_DELTA_TOPIC, 1);
	client.setCallback(messageHandler);
	pinMode(PIN_ELECTROIMAN, OUTPUT);
	// For some weird reason, the ESP32 doesnt catch the get/accepted response at the first attemp

	StaticJsonDocument<8> doc;
	char jsonBuffer[8];
	serializeJson(doc, jsonBuffer); // print to client
	client.publish(SHADOW_GET_TOPIC, jsonBuffer);
	delay(100);
	client.publish(SHADOW_GET_TOPIC, jsonBuffer);
}

void loop()
{
	now = time(nullptr);
	if (!client.connected()) {
		reconnect(THINGNAME, "heladera/status");
	}
	client.loop();
	char myKey = myKeypad.getKey();
	if (myKey != NULL) {
		Serial.print("Key pressed: ");
		Serial.println(myKey);
		publishMessage("heladera/teclado", myKey);
	}
}