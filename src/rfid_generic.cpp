#include "utils/iot_utils.hpp"
#include "utils/rfid_utils.hpp"

String lastPub[NUMBER_OF_READERS];
bool should_publish;

void messageHandler(char* topic, byte* payload, unsigned int length)
{
	if (strcmp(topic, RESET_TOPIC) == 0) {
		for (int i = 0; i < NUMBER_OF_READERS; i++) {
			lastPub[i] = "00 00 00 00";
		}
		debugger.message("Cleaning reading storage");
		clearReadingStorage();
	}
}
void setup()
{
	Serial.begin(115200); // Initialize serial communications
	while (!Serial)
		; // Do nothing until serial connection is opened
	connect_mqtt_broker();
	SPI.begin(); // Init SPI bus
	for (int i = 0; i < NUMBER_OF_READERS; i++) {
		mfrc522[i].PCD_SetAntennaGain(mfrc522[i].RxGain_max);
	}
	printRFIDVersions();
	pinMode(RST_PIN, OUTPUT);
	digitalWrite(RST_PIN, LOW); // mfrc522 readers hard power down.
	mqttc.setCallback(messageHandler);
	mqttc.subscribe(RESET_TOPIC);
	debugger.message("Finished configuration");
	debugger.requiered_loops = 10;
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
			debugger.message("New RFID detected, reporting to broker");
			StaticJsonDocument<256> doc;
			char jsonBuffer[256];
			for (int i = 0; i < NUMBER_OF_READERS; i++) {
				lastPub[i] = getUIDFromReadingStorage(i);
				Serial.printf("Adding to the doc: \"rfid_%s\" : ", String(i));
				Serial.println(lastPub[i]);
				doc[String("rfid_" + String(i))] = lastPub[i];
			}
			serializeJson(doc, jsonBuffer);
			report_reading_to_broker("rfid", jsonBuffer);
		} else {
			Serial.println("Not publishing because the state is the same as before");
		}
		newRFIDAppeared = false;
	}
	local_delay(100);
	debugger.loop();
}