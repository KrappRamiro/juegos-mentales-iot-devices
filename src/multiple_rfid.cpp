#include "Arduino.h"
#include <MFRC522.h>
#include <SPI.h>
#include <string.h>
#define D3 0
#define D8 15
#define D2 4
#define D1 5
#define D0 16

const uint8_t NUMBER_OF_READERS = 4;
const uint8_t RST_PIN = D3;
const uint8_t SS_0_PIN = D8;
const uint8_t SS_1_PIN = D2;
const uint8_t SS_2_PIN = D1;
const uint8_t SS_3_PIN = D0;

const byte ssPins[] = { SS_0_PIN, SS_1_PIN, SS_2_PIN, SS_3_PIN };
byte readedCard[NUMBER_OF_READERS][4]; // Matrix for storing UID over each reader, its 4 because the UID is stored in the first 4 bytes of the tag
MFRC522 mfrc522[NUMBER_OF_READERS]; // Create MFRC522 instances

bool getRFID(byte readern)
{
	bool isPICCpresent = false;
	digitalWrite(RST_PIN, HIGH); // Get RC522 reader out of hard low power mode
	mfrc522[readern].PCD_Init(); // Init the reader
	//****** Trying to set antenna gain to max, erratic functioning of the readers ***********
	// mfrc522[readern].PCD_SetRegisterBitMask(mfrc522[readern].RFCfgReg, (0x07<<4));
	// mfrc522[readern].PCD_SetAntennaGain(0x04);
	// mfrc522[readern].PCD_ClearRegisterBitMask(mfrc522[readern].RFCfgReg, (0x07<<4));
	// mfrc522[readern].PCD_SetRegisterBitMask(mfrc522[readern].RFCfgReg, 0x07);
	// delay(50);
	if (mfrc522[readern].PICC_IsNewCardPresent() && mfrc522[readern].PICC_ReadCardSerial()) {
		memcpy(readedCard[readern], mfrc522[readern].uid.uidByte, 4);
		isPICCpresent = true;
	}
	mfrc522[readern].PICC_HaltA();
	mfrc522[readern].PCD_StopCrypto1();
	digitalWrite(RST_PIN, LOW); // return to hard low power mode
	return isPICCpresent; // returns TRUE if PICC is detected, false if not
}

//*********** Routine for print 4 byte UID to serial ******************

// For more information about why the loop stops at 4, see section 8.6.1 of https://www.nxp.com/docs/en/data-sheet/MF1S50YYX_V1.pdf, its because we need to read the NUID, which is stored in the first 4 bytes
void printUID(byte* buffer)
{
	for (byte i = 0; i < 4; i++) {
		Serial.print(buffer[i] < 0x10 ? " 0" : " ");
		Serial.print(buffer[i], HEX);
	}
}
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
		if (getRFID(reader)) {
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
