#include "Arduino.h"
#include "utils/multiple_rfid_utils.hpp"
#include <MFRC522.h>
#include <SPI.h>
#include <string.h>

const byte ssPins[] = { SS_0_PIN, SS_1_PIN, SS_2_PIN, SS_3_PIN };
byte readedCard[NUMBER_OF_READERS][4]; // Matrix for storing UID over each reader, its 4 because the UID is stored in the first 4 bytes of the tag
MFRC522 mfrc522[NUMBER_OF_READERS]; // Create MFRC522 instances

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