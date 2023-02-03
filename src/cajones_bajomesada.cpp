#include "Arduino.h"
#include "secrets/cajones_bajomesada_secrets.h"
#include "secrets/shared_secrets.h"
#include "utils/iot_utils.hpp"
#define SHADOW_GET_TOPIC "$aws/things/cajones_bajomesada/shadow/get"
#define SHADOW_GET_ACCEPTED_TOPIC "$aws/things/cajones_bajomesada/shadow/get/accepted"
#define SHADOW_UPDATE_TOPIC "$aws/things/cajones_bajomesada/shadow/update"
#define SHADOW_UPDATE_ACCEPTED_TOPIC "$aws/things/cajones_bajomesada/shadow/update/accepted"
#define SHADOW_UPDATE_DELTA_TOPIC "$aws/things/cajones_bajomesada/shadow/update/delta"
#define PIN_ELECTROIMAN_1 D1
#define PIN_ELECTROIMAN_2 D2
#define PIN_ELECTROIMAN_3 D3

bool estado_electroiman_1 = true;
bool estado_electroiman_2 = true;
bool estado_electroiman_3 = true;

void messageHandler(char* topic, byte* payload, unsigned int length)
{
	StaticJsonDocument<256> doc;
	JsonObject state_desired;
	deserializeJson(doc, payload); // Put the info from the payload into the JSON document
	Serial.printf("MESSAGE HANDLER: Topic: %s\n", topic);
	// ------------ RETRIEVING THE SHADOW DOCUMENT FROM AWS -------------//
	if (strcmp(topic, SHADOW_GET_ACCEPTED_TOPIC) == 0)
		state_desired = doc["state"]["desired"];
	else if (strcmp(topic, SHADOW_UPDATE_DELTA_TOPIC) == 0)
		state_desired = doc["state"];
	estado_electroiman_1 = state_desired["electroiman_1"] | estado_electroiman_1;
	estado_electroiman_2 = state_desired["electroiman_2"] | estado_electroiman_2;
	estado_electroiman_3 = state_desired["electroiman_3"] | estado_electroiman_3;

	Serial.printf("Estado elctroiman 1: %s\n", estado_electroiman_1 ? "true" : "false");
	Serial.printf("Estado electroiman 2: %s\n", estado_electroiman_2 ? "true" : "false");
	Serial.printf("Estado electroiman 3: %s\n", estado_electroiman_3 ? "true" : "false");

	digitalWrite(PIN_ELECTROIMAN_1, estado_electroiman_1);
	digitalWrite(PIN_ELECTROIMAN_2, estado_electroiman_2);
	digitalWrite(PIN_ELECTROIMAN_3, estado_electroiman_3);

	// ------------------------------------------------------------------//

#pragma region // Region that reports the current state to the shadow
	doc.clear(); // Clear the JSON document so it can be used to publish the current state
	char jsonBuffer[256];
	JsonObject state_reported = doc["state"].createNestedObject("reported");
	state_reported["electroiman_1"] = estado_electroiman_1;
	state_reported["electroiman_2"] = estado_electroiman_2;
	state_reported["electroiman_3"] = estado_electroiman_3;

	serializeJsonPretty(doc, jsonBuffer);
	Serial.println("Reporting the following to the shadow:");
	Serial.println(jsonBuffer);
	client.publish(SHADOW_UPDATE_TOPIC, jsonBuffer);
#pragma endregion
}

void setup()
{
	Serial.begin(115200);
	connectAWS(WIFI_SSID, WIFI_PASSWORD, THINGNAME, AWS_CERT_CA, AWS_CERT_CRT, AWS_CERT_PRIVATE, AWS_IOT_ENDPOINT); // Connect to AWS
	client.subscribe(SHADOW_GET_ACCEPTED_TOPIC, 1); // Subscribe to the topic that gets the initial state
	client.subscribe(SHADOW_UPDATE_DELTA_TOPIC, 1); // Subscribe to the topic that updates the state every time it changes
	client.setCallback(messageHandler);
	pinMode(PIN_ELECTROIMAN_1, OUTPUT);
	pinMode(PIN_ELECTROIMAN_2, OUTPUT);
	pinMode(PIN_ELECTROIMAN_3, OUTPUT);

	// ----------- Get the Shadow document --------------//
	StaticJsonDocument<8> doc;
	char jsonBuffer[8];
	serializeJson(doc, jsonBuffer);
	client.publish(SHADOW_GET_TOPIC, jsonBuffer);
	// --------------------------------------------------//
}
void loop()
{
	now = time(nullptr); // The NTP server uses this, if you delete this, the connection to AWS no longer works
	if (!client.connected()) {
		reconnect(THINGNAME, "cajones_bajomesada/status");
	}
	local_delay(100);
}