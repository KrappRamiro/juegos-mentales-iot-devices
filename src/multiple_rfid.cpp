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
	for (uint8_t reader = 0; reader < NUMBER_OF_READERS; reader++) {
		mfrc522[reader].PCD_Init(ssPins[reader], RST_PIN); // Init each MFRC522 card
		Serial.print(F("Reader "));
		Serial.print(reader);
		Serial.print(F(": "));
		mfrc522[reader].PCD_DumpVersionToSerial();
	}
	pinMode(RST_PIN, OUTPUT);
	digitalWrite(RST_PIN, LOW); // mfrc522 readers hard power down.
}

void loop()
{
	for (uint8_t reader = 0; reader < NUMBER_OF_READERS; reader++) {
		if (getRFID(mfrc522, reader, readedCard)) {
			Serial.print(F("Reader "));
			Serial.print(reader);
			Serial.print(F(": Card UID:"));
			printUID(readedCard[reader]);
			Serial.println();
		}
	}
	delay(2000);
}

//********************************END OF ROUTINE********************************************
