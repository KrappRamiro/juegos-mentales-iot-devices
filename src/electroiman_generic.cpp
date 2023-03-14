#include"modules/utils/iot/iot_utils.hpp"

void messageHandler(char* topic, byte* payload, unsigned int length)
{
	StaticJsonDocument<256> doc;
	deserializeJson(doc, payload); // Put the info from the payload into the JSON document
	Serial.printf("MESSAGE HANDLER: Topic: %s\n", topic);
	if (strcmp(topic, ELECTROIMAN_1_TOPIC) == 0) {
		bool status = doc["status"];
		if (status) {
			debugger.message("Desactivando electroiman 1");
			digitalWrite(ELECTROIMAN_PIN_1, LOW);
		} else {
			debugger.message("Activando electroiman 1");
			digitalWrite(ELECTROIMAN_PIN_1, HIGH);
		}
	}
	if (strcmp(topic, ELECTROIMAN_2_TOPIC) == 0) {
		bool status = doc["status"];
		if (status) {
			debugger.message("Desactivando electroiman 2");
			digitalWrite(ELECTROIMAN_PIN_2, LOW);
		} else {
			debugger.message("Activando electroiman 2");
			digitalWrite(ELECTROIMAN_PIN_2, HIGH);
		}
	}
	if (strcmp(topic, ELECTROIMAN_3_TOPIC) == 0) {
		bool status = doc["status"];
		if (status) {
			debugger.message("Desactivando electroiman 3");
			digitalWrite(ELECTROIMAN_PIN_3, LOW);
		} else {
			debugger.message("Activando electroiman 3");
			digitalWrite(ELECTROIMAN_PIN_3, HIGH);
		}
	}
	if (strcmp(topic, ELECTROIMAN_4_TOPIC) == 0) {
		bool status = doc["status"];
		if (status) {
			debugger.message("Desactivando electroiman 4");
			digitalWrite(ELECTROIMAN_PIN_4, LOW);
		} else {
			debugger.message("Activando electroiman 4");
			digitalWrite(ELECTROIMAN_PIN_4, HIGH);
		}
	}
}
void setup()
{
	Serial.begin(115200);
	while (!Serial)
		; // Do nothing until serial connection is opened
	pinMode(ELECTROIMAN_PIN_1, OUTPUT);
	pinMode(ELECTROIMAN_PIN_2, OUTPUT);
	pinMode(ELECTROIMAN_PIN_3, OUTPUT);
	pinMode(ELECTROIMAN_PIN_4, OUTPUT);
	connect_mqtt_broker();
	mqttc.setCallback(messageHandler);
	mqttc.subscribe(ELECTROIMAN_1_TOPIC, 1);
	mqttc.subscribe(ELECTROIMAN_2_TOPIC, 1);
	mqttc.subscribe(ELECTROIMAN_3_TOPIC, 1);
	mqttc.subscribe(ELECTROIMAN_4_TOPIC, 1);
	debugger.message("Finished setup");
	debugger.requiered_loops = 5;
}

void loop()
{
	if (!mqttc.connected()) {
		reconnect();
	}
	local_delay(200);
	debugger.loop();
}