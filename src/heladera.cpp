#include "utils/iot_utils.hpp"
#include <Keypad.h>
#define PIN_ELECTROIMAN D8
#define ELECTROIMAN_TOPIC "heladera/elements/electroiman"

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
char myKey;
// ----------- END OF KEYPAD CREATION ------------- //

void messageHandler(char* topic, byte* payload, unsigned int length)
{
	StaticJsonDocument<64> doc;
	Serial.printf("MESSAGE HANDLER: Topic: %s\n", topic);
	if (strcmp(topic, ELECTROIMAN_TOPIC) == 0) {
		deserializeJson(doc, payload); // Put the info from the payload into the JSON document
		bool status = doc["status"];
		digitalWrite(PIN_ELECTROIMAN, status);
	}
}

void setup()
{
	Serial.begin(115200);
	while (!Serial)
		; // Do nothing until serial connection is opened
	pinMode(PIN_ELECTROIMAN, OUTPUT);
	connect_mqtt_broker();
	mqttc.setCallback(messageHandler);
	mqttc.subscribe(ELECTROIMAN_TOPIC, 1); // Subscribe to the topic that updates the state every time it changes
	debug("Finished configuration");
}

void loop()
{
	if (!mqttc.connected()) {
		reconnect();
	}
	myKey = myKeypad.getKey();
	if (myKey != NULL) {
		Serial.printf("Key pressed: %c", myKey);
		StaticJsonDocument<32> doc;
		char jsonBuffer[32];
		doc["key"] = myKey;
		serializeJson(doc, jsonBuffer);
		report_reading_to_broker("keypad", jsonBuffer); // Publish the pressed key to AWS
	}
	local_delay(50);
}