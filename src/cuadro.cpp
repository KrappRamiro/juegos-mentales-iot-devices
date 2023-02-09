#include "secrets/cuadro_secrets.h"
#include "secrets/shared_secrets.h"
#include "utils/iot_utils.hpp"
#include "utils/multiple_rfid_utils.hpp"
#define SHADOW_GET_TOPIC "$aws/things/cuadro/shadow/get"
#define SHADOW_GET_ACCEPTED_TOPIC "$aws/things/cuadro/shadow/get/accepted"
#define SHADOW_UPDATE_TOPIC "$aws/things/cuadro/shadow/update"
#define SHADOW_UPDATE_ACCEPTED_TOPIC "$aws/things/cuadro/shadow/update/accepted"
#define SHADOW_UPDATE_DELTA_TOPIC "$aws/things/cuadro/shadow/update/delta"
#define RESET_TOPIC "cuadro/reset"
String lastPub[NUMBER_OF_READERS]; // should be in the mixin
bool should_publish;

void messageHandler(char* topic, byte* payload, unsigned int length)
{
	if (strcmp(topic, RESET_TOPIC) == 0) {
		for (int i = 0; i < NUMBER_OF_READERS; i++) {
			lastPub[i] = "00 00 00 00";
		}
		Serial.println("Cleaning lastPub");
	}
}

void report_state_to_shadow()
{
	StaticJsonDocument<256> doc;
	char jsonBuffer[256];
	JsonObject state_reported = doc["state"].createNestedObject("reported");
	for (int i = 0; i < NUMBER_OF_READERS; i++) {
		lastPub[i] = getUIDFromReadingStorage(i);
		Serial.printf("Adding to the doc: \"rfid_%s\" : ", String(i));
		Serial.println(lastPub[i]);
		state_reported[String("rfid_" + String(i))] = lastPub[i];
	}
	serializeJsonPretty(doc, jsonBuffer);
	Serial.println("Reporting the following to the shadow:");
	Serial.println(jsonBuffer);
	String result = (client.publish(SHADOW_UPDATE_TOPIC, jsonBuffer)) ? "success publishing" : "failed publishing";
	Serial.println(result);
}
void setup()
{
	Serial.begin(115200); // Initialize serial communications
	while (!Serial)
		; // Do nothing until serial connection is opened
	connectAWS(WIFI_SSID, WIFI_PASSWORD, THINGNAME, AWS_CERT_CA, AWS_CERT_CRT, AWS_CERT_PRIVATE, AWS_IOT_ENDPOINT);
	SPI.begin(); // Init SPI bus
	printRFIDVersions();
	pinMode(RST_PIN, OUTPUT);
	digitalWrite(RST_PIN, LOW); // mfrc522 readers hard power down.
	client.subscribe(RESET_TOPIC);
	client.setCallback(messageHandler);
}

void loop()
{
	now = time(nullptr);
	if (!client.connected()) {
		reconnect(THINGNAME, "cuadro/status");
	}
	client.loop();
	printMultipleRFID();
	if (newRFIDAppeared) {
		should_publish = false;
		for (int i = 0; i < NUMBER_OF_READERS; i++) {
			if (!(lastPub[i].equals(getUIDFromReadingStorage(i)))) {
				should_publish = true;
			}
		}
		if (should_publish) {
			report_state_to_shadow();
		} else {
			Serial.println("Not publishing because the state is the same as before");
		}
		newRFIDAppeared = false;
	}
	local_delay(200);
}