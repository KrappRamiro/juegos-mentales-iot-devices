#include "utils/iot_utils.hpp"
#define TRACK_1_PIN D3
#define TRACK_2_PIN D4
#define PAUSE_PIN D5
#define VOL_DOWN_PIN D6
#define VOL_UP_PIN D7

bool config_finished_flag = false;
bool vol_up_flag = false;
bool vol_down_flag = false;
bool pause_flag = false;
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
	if (strcmp(topic, PAUSE_TOPIC) == 0) {
		pause_flag = true;
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
	debugger.message_number("High-lowing the pin ", pin_n, "debug");
	digitalWrite(pin_n, HIGH);
	delay(300);
	digitalWrite(pin_n, LOW);
}
void setup()
{
	Serial.begin(115200);
	while (!Serial)
		; // Do nothing until serial connection is opened
	pinMode(TRACK_1_PIN, OUTPUT);
	pinMode(TRACK_2_PIN, OUTPUT);
	pinMode(PAUSE_PIN, OUTPUT);
	pinMode(VOL_UP_PIN, OUTPUT);
	pinMode(VOL_DOWN_PIN, OUTPUT);
	digitalWrite(TRACK_1_PIN, LOW);
	digitalWrite(TRACK_2_PIN, LOW);
	digitalWrite(PAUSE_PIN, LOW);
	digitalWrite(VOL_UP_PIN, LOW);
	digitalWrite(VOL_DOWN_PIN, LOW);
	connect_mqtt_broker();
	mqttc.setCallback(messageHandler);
	mqttc.subscribe(TRACK_N_TOPIC, 1);
	mqttc.subscribe(VOL_UP_TOPIC, 1);
	mqttc.subscribe(VOL_DOWN_TOPIC, 1);
	while (!config_finished_flag) {
		local_delay(150);
		Serial.print("*");
	}
	debugger.message("Finished setup");
	debugger.requiered_loops = 5;
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
			high_low(TRACK_1_PIN);
		else if (track_n == 2)
			high_low(TRACK_2_PIN);
		else if (track_n == 0)
			high_low(PAUSE_PIN);
	}
	if (vol_up_flag) {
		debugger.message("Volume UP");
		high_low(VOL_UP_PIN);
		vol_up_flag = false;
	}
	if (vol_down_flag) {
		debugger.message("Volume DOWN");
		high_low(VOL_DOWN_PIN);
		vol_down_flag = false;
	}
	if (pause_flag) {
		debugger.message("Pausing the music");
		high_low(PAUSE_PIN);
		pause_flag = false;
	}
	// Delay a little bit
	local_delay(200);
	debugger.loop();
}
