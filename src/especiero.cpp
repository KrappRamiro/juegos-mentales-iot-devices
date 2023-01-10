#include "secrets/especiero_secrets.h"
#include "utils/iot_utils.hpp"
#include "utils/multiple_rfid_utils.hpp"
String lastPub[NUMBER_OF_READERS];
bool should_publish;
void publishMessage()
{
	Serial.println("Message publishing started!!!");
	StaticJsonDocument<200> doc;
	for (int i = 0; i < NUMBER_OF_READERS; i++) {
		lastPub[i] = getUIDFromReadingStorage(i);
		Serial.printf("Adding to the doc: \"%s\" : ", String(i));
		Serial.println(lastPub[i]);
		doc[String(i)] = lastPub[i];
	}
	char jsonBuffer[512];
	serializeJson(doc, jsonBuffer); // print to client
	String result = (client.publish("especiero", jsonBuffer)) ? "success publishing" : "failed publishing";
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
}

void loop()
{
	now = time(nullptr);
	if (!client.connected()) {
		reconnect(THINGNAME, "especiero/status");
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
			publishMessage();
		} else {
			Serial.println("Not publishing because the state is the same as before");
		}
		newRFIDAppeared = false;
	}
	delay(500);
}