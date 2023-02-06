/*
Cuando en el server aprete un boton te pone por 500ms un HIGH en un PIN
PINES:
	D3 Track 1
	D4 Track 2
	D5 Track 3
	D6 Vol -
	D7 Vol +
*/
#include "Arduino.h"
#include "secrets/shared_secrets.h"
#include "secrets/sistema_audio_secrets.h"
#include "utils/iot_utils.hpp"
#define SHADOW_GET_TOPIC "$aws/things/sistema_audio/shadow/get"
#define SHADOW_GET_ACCEPTED_TOPIC "$aws/things/sistema_audio/shadow/get/accepted"
#define SHADOW_UPDATE_TOPIC "$aws/things/sistema_audio/shadow/update"
#define SHADOW_UPDATE_ACCEPTED_TOPIC "$aws/things/sistema_audio/shadow/update/accepted"
#define SHADOW_UPDATE_DELTA_TOPIC "$aws/things/sistema_audio/shadow/update/delta"
#define VOL_UP_TOPIC "sistema_audio/vol_up"
#define VOL_DOWN_TOPIC "sistema_audio/vol_down"

#define PIN_TRACK_1 D3
#define PIN_TRACK_2 D4
#define PIN_PAUSA D5 // track_n 0
#define PIN_VOL_DOWN D6
#define PIN_VOL_UP D7

bool config_finished_flag = false;
bool vol_up_flag = false;
bool vol_down_flag = false;
bool should_reproduce_track = false;
int track_n = 0;
void messageHandler(char* topic, byte* payload, unsigned int length)
{
	StaticJsonDocument<256> doc;
	JsonObject state_desired;
	deserializeJson(doc, payload); // Put the info from the payload into the JSON document
	Serial.printf("MESSAGE HANDLER: Topic: %s\n", topic);
	// ------------ RETRIEVING THE SHADOW DOCUMENT FROM AWS -------------//
	if (strcmp(topic, VOL_UP_TOPIC) == 0) {
		vol_up_flag = true;
		return;
	}
	if (strcmp(topic, VOL_DOWN_TOPIC) == 0) {
		vol_down_flag = true;
		return;
	}
	if (strcmp(topic, SHADOW_GET_ACCEPTED_TOPIC) == 0) {

		state_desired = doc["state"]["desired"];
		config_finished_flag = true;
	} else if (strcmp(topic, SHADOW_UPDATE_DELTA_TOPIC) == 0)
		state_desired = doc["state"];
	track_n = state_desired["track_n"];
	should_reproduce_track = true;
}
void report_state_to_shadow()
{
	StaticJsonDocument<256> doc;
	char jsonBuffer[256];
	JsonObject state_reported = doc["state"].createNestedObject("reported");
	state_reported["track_n"] = track_n;
	serializeJsonPretty(doc, jsonBuffer);
	Serial.println("Reporting the following to the shadow:");
	Serial.println(jsonBuffer);
	client.publish(SHADOW_UPDATE_TOPIC, jsonBuffer);
}
void high_low(int pin_n)
{
	Serial.printf("High-lowing the pin %i\n", pin_n);
	digitalWrite(pin_n, HIGH);
	delay(300);
	digitalWrite(pin_n, LOW);
}
void setup()
{
	Serial.begin(115200);
	pinMode(PIN_TRACK_1, OUTPUT);
	pinMode(PIN_TRACK_2, OUTPUT);
	pinMode(PIN_PAUSA, OUTPUT);
	pinMode(PIN_VOL_UP, OUTPUT);
	pinMode(PIN_VOL_DOWN, OUTPUT);
	digitalWrite(PIN_TRACK_1, LOW);
	digitalWrite(PIN_TRACK_2, LOW);
	digitalWrite(PIN_PAUSA, LOW);
	digitalWrite(PIN_VOL_UP, LOW);
	digitalWrite(PIN_VOL_DOWN, LOW);
	connectAWS(WIFI_SSID, WIFI_PASSWORD, THINGNAME, AWS_CERT_CA, AWS_CERT_CRT, AWS_CERT_PRIVATE, AWS_IOT_ENDPOINT); // Connect to AWS
	client.setCallback(messageHandler);
	client.subscribe(SHADOW_GET_ACCEPTED_TOPIC, 1);
	client.subscribe(SHADOW_UPDATE_DELTA_TOPIC, 1);
	client.subscribe(VOL_UP_TOPIC, 1);
	client.subscribe(VOL_DOWN_TOPIC, 1);
	StaticJsonDocument<8> doc;
	char jsonBuffer[8];
	serializeJson(doc, jsonBuffer);
	client.publish(SHADOW_GET_TOPIC, jsonBuffer);
	while (!config_finished_flag) {
		local_delay(150);
		Serial.print("*");
	}
	Serial.println("Finished setup");
}
void loop()
{
	now = time(nullptr); // The NTP server uses this, if you delete this, the connection to AWS no longer works
	if (!client.connected()) {
		reconnect(THINGNAME, "sistema_audio/status");
	}
	if (should_reproduce_track) {
		should_reproduce_track = false;
		Serial.printf("Reproducing track %i\n", track_n);
		if (track_n == 1)
			high_low(PIN_TRACK_1);
		else if (track_n == 2)
			high_low(PIN_TRACK_2);
		else if (track_n == 0)
			high_low(PIN_PAUSA);
		report_state_to_shadow();
	}
	if (vol_up_flag) {
		Serial.println("Volume UP");
		high_low(PIN_VOL_UP);
		vol_up_flag = false;
	}
	if (vol_down_flag) {
		Serial.println("Volume DOWN");
		high_low(PIN_VOL_DOWN);
		vol_down_flag = false;
	}

	// Delay a little bit
	local_delay(100);
}
