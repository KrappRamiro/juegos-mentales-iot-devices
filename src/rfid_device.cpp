#include "utils/iot_utils.hpp"
#include "utils/multiple_rfid_utils.hpp"

String lastPub[NUMBER_OF_READERS]; // should be in the mixin
bool should_publish;
void messageHandler(char* topic, byte* payload, unsigned int length)
{
	if (strcmp(topic, RESET_TOPIC) == 0) {
		for (int i = 0; i < NUMBER_OF_READERS; i++) {
			lastPub[i] = "00 00 00 00";
		}
		Serial.println("Cleaning lastPub");
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
	mfrc522[0].PCD_SetAntennaGain(mfrc522[0].RxGain_max);
	mfrc522[1].PCD_SetAntennaGain(mfrc522[1].RxGain_max);
	mfrc522[2].PCD_SetAntennaGain(mfrc522[2].RxGain_max);
	mfrc522[3].PCD_SetAntennaGain(mfrc522[3].RxGain_max);
	printRFIDVersions();
	pinMode(RST_PIN, OUTPUT);
	digitalWrite(RST_PIN, LOW); // mfrc522 readers hard power down.
	mqttc.subscribe(RESET_TOPIC);
	mqttc.setCallback(messageHandler);
}
void loop() { }