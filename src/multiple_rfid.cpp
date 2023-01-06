#include "utils/multiple_rfid_utils.hpp"
void setup()
{
	Serial.begin(115200); // Initialize serial communications
	while (!Serial)
		; // Do nothing until serial connection is opened
	SPI.begin(); // Init SPI bus
	readRFIDVersions(mfrc522, ssPins);
	pinMode(RST_PIN, OUTPUT);
	digitalWrite(RST_PIN, LOW); // mfrc522 readers hard power down.
}

void loop()
{
	readMultipleRFID(mfrc522, readedCard);
	delay(2000);
}