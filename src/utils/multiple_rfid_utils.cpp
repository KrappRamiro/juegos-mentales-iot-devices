#include "utils/multiple_rfid_utils.hpp"

byte readedCard[NUMBER_OF_READERS][4]; // Matrix for storing UID over each reader, its 4 because the UID is stored in the first 4 bytes of the tag, think of it as a "Readed UID Storage"
const byte ssPins[] = { SS_0_PIN, SS_1_PIN, SS_2_PIN, SS_3_PIN };
MFRC522 mfrc522[NUMBER_OF_READERS]; // Create MFRC522 instances

bool getRFID(MFRC522* mfrc522, byte readern, byte readedCard[NUMBER_OF_READERS][4])
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

void printUID(byte* buffer)
//*********** Routine for print 4 byte UID to serial ******************

// For more information about why the loop stops at 4, see section 8.6.1 of https://www.nxp.com/docs/en/data-sheet/MF1S50YYX_V1.pdf, its because we need to read the NUID, which is stored in the first 4 bytes
{
	for (byte i = 0; i < 4; i++) {
		Serial.print(buffer[i] < 0x10 ? " 0" : " ");
		Serial.print(buffer[i], HEX);
	}
}

void readRFIDVersions(MFRC522* mfrc522, const byte* ssPins)
{
	for (uint8_t readerNumber = 0; readerNumber < NUMBER_OF_READERS; readerNumber++) {
		mfrc522[readerNumber].PCD_Init(ssPins[readerNumber], RST_PIN); // Init each MFRC522 card
		Serial.print(F("Reader "));
		Serial.print(readerNumber);
		Serial.print(F(": "));
		mfrc522[readerNumber].PCD_DumpVersionToSerial();
	}
}

void readMultipleRFID(MFRC522* mfrc522, byte readedCard[NUMBER_OF_READERS][4])
{
	for (uint8_t readerNumber = 0; readerNumber < NUMBER_OF_READERS; readerNumber++) {
		if (getRFID(mfrc522, readerNumber, readedCard)) {
			Serial.print(F("Reader "));
			Serial.print(readerNumber);
			Serial.print(F(": Card UID:"));
			printUID(readedCard[readerNumber]);
			Serial.println();
		}
	}
}

String getUIDFromReadingStorage(byte readedCard[NUMBER_OF_READERS][4], int readerNumber)
{
	String content = "";
	for (byte i = 0; i < 4; i++) {
		content.concat(String(readedCard[readerNumber][i] < 0x10 ? " 0" : " "));
		content.concat(String(readedCard[readerNumber][i], HEX));
	}
	content.toUpperCase();
	String theUID = content.substring(1);
	return theUID;
}