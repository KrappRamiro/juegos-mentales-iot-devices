#include "secrets/especiero_secrets.h"
#include "utils/iot_utils.hpp"
#include "utils/multiple_rfid_utils.hpp"

void publishMessage(String uid)
{
	Serial.println("Message publishing started!!!");
	StaticJsonDocument<200> doc;
	doc["rfid"] = uid;
	char jsonBuffer[512];
	serializeJson(doc, jsonBuffer); // print to client
	bool result = client.publish("especiero", jsonBuffer);
	Serial.println(result);
}

void setup()
{
	Serial.begin(115200); // Initialize serial communications
	while (!Serial)
		; // Do nothing until serial connection is opened
	Serial.println("A");
	connectAWS(WIFI_SSID, WIFI_PASSWORD, THINGNAME, AWS_CERT_CA, AWS_CERT_CRT, AWS_CERT_PRIVATE, AWS_IOT_ENDPOINT);
	Serial.println("B");
	SPI.begin(); // Init SPI bus
	readRFIDVersions(mfrc522, ssPins);
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
	readMultipleRFID(mfrc522, readedCard);
	publishMessage((getUIDFromReadingStorage(readedCard, 0)));
	delay(500);
}