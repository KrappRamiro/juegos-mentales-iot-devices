#include "utils/iot_utils.hpp"
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
	if (strcmp(topic, TRACK_N_TOPIC) == 0) {
		track_n = doc["track_n"];
		config_finished_flag = true;
		should_reproduce_track = true;
	}
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
	connect_mqtt_broker();
	client.setCallback(messageHandler);
	client.subscribe(TRACK_N_TOPIC, 1);
	client.subscribe(VOL_UP_TOPIC, 1);
	client.subscribe(VOL_DOWN_TOPIC, 1);
	while (!config_finished_flag) {
		local_delay(150);
		Serial.print("*");
	}
	debug("Finished setup");
}
void loop()
{
	if (!mqttc.connected()) {
		reconnect();
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
	}
	if (vol_up_flag) {
		debug("Volume UP");
		high_low(PIN_VOL_UP);
		vol_up_flag = false;
	}
	if (vol_down_flag) {
		debug("Volume DOWN");
		high_low(PIN_VOL_DOWN);
		vol_down_flag = false;
	}
	// Delay a little bit
	local_delay(200);
}
