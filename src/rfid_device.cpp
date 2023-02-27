#include "utils/iot_utils.hpp"
#include "utils/multiple_rfid_utils.hpp"

String lastPub[NUMBER_OF_READERS];
bool should_publish;

void report_rfid_to_broker()
{
	StaticJsonDocument<256> doc;
	char jsonBuffer[256];
	for (int i = 0; i < NUMBER_OF_READERS; i++) {
		lastPub[i] = getUIDFromReadingStorage(i);
		Serial.printf("Adding to the doc: \"rfid_%s\" : ", String(i));
		Serial.println(lastPub[i]);
		doc[String("rfid_" + String(i))] = lastPub[i];
	}
	serializeJsonPretty(doc, jsonBuffer);
	Serial.println("Reporting the following to the mqtt broker:");
	Serial.println(jsonBuffer);
	String result = (mqttc.publish(READING_TOPIC, jsonBuffer)) ? "success publishing" : "failed publishing";
	Serial.println(result);
}

void messageHandler(char* topic, byte* payload, unsigned int length)
{
	if (strcmp(topic, RESET_TOPIC) == 0) {
		for (int i = 0; i < NUMBER_OF_READERS; i++) {
			lastPub[i] = "00 00 00 00";
		}
		Serial.println("Cleaning lastPub");
		// debug("Cleaning lastPub")
		clearReadingStorage();
	}
}
void setup()
{
	Serial.begin(115200); // Initialize serial communications
	while (!Serial)
		; // Do nothing until serial connection is opened
	connect_mqtt_broker(THINGNAME);
	SPI.begin(); // Init SPI bus
	for (int i = 0; i < NUMBER_OF_READERS; i++) {
		mfrc522[i].PCD_SetAntennaGain(mfrc522[i].RxGain_max);
	}
	printRFIDVersions();
	pinMode(RST_PIN, OUTPUT);
	digitalWrite(RST_PIN, LOW); // mfrc522 readers hard power down.
	mqttc.subscribe(RESET_TOPIC);
	mqttc.setCallback(messageHandler);
}
void loop()
{
	if (!mqttc.connected()) {
		reconnect();
	}
	printMultipleRFID();
	if (newRFIDAppeared) {
		should_publish = false;
		for (int i = 0; i < NUMBER_OF_READERS; i++) {
			if (!(lastPub[i].equals(getUIDFromReadingStorage(i)))) {
				should_publish = true;
			}
		}
		if (should_publish) {
			report_rfid_to_broker();
		} else {
			Serial.println("Not publishing because the state is the same as before");
		}
		newRFIDAppeared = false;
	}
	local_delay(200);
}