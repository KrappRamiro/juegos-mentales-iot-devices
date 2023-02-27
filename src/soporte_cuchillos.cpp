#include "utils/iot_utils.hpp"
#define BUTTON_PIN D0
bool current_state = false; // current state of the button
bool previous_state = false;

void report_reading_to_broker()
{
	StaticJsonDocument<64> doc;
	char jsonBuffer[64];
	doc["switch"] = current_state;
	serializeJsonPretty(doc, jsonBuffer);
	Serial.println("Reporting the following to the broker:");
	Serial.println(jsonBuffer);
	mqttc.publish(READING_TOPIC, jsonBuffer);
}

void setup()
{
	Serial.begin(115200);
	while (!Serial)
		; // Do nothing until serial connection is opened
	connect_mqtt_broker();
	pinMode(BUTTON_PIN, INPUT);
	debug("Finished configuration");
}

void loop()
{
	if (!mqttc.connected()) {
		reconnect();
	}
	// read the pushbutton input pin:
	current_state = digitalRead(BUTTON_PIN);
	// compare the current_state to its previous state
	if (current_state != previous_state) {
		previous_state = current_state;
		report_reading_to_broker();
	}

	// Delay a little bit
	local_delay(200);
}