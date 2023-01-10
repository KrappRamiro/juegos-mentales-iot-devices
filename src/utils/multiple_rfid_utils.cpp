#include "utils/multiple_rfid_utils.hpp"

byte readingStorage[NUMBER_OF_READERS][4]; // Matrix for storing UID over each reader, its 4 because the UID is stored in the first 4 bytes of the tag, think of it as a "Readed UID Storage"
const byte ssPins[] = { SS_0_PIN, SS_1_PIN, SS_2_PIN, SS_3_PIN };
MFRC522 mfrc522[NUMBER_OF_READERS]; // Create MFRC522 instances
bool newRFIDAppeared;

bool getRFID(byte readerNumber)
{
	bool isPICCpresent = false;
	digitalWrite(RST_PIN, HIGH); // Get RC522 reader out of hard low power mode
	mfrc522[readerNumber].PCD_Init(); // Init the reader
	//****** Trying to set antenna gain to max, erratic functioning of the readers ***********
	// mfrc522[readerNumber].PCD_SetRegisterBitMask(mfrc522[readerNumber].RFCfgReg, (0x07<<4));
	// mfrc522[readerNumber].PCD_SetAntennaGain(0x04);
	// mfrc522[readerNumber].PCD_ClearRegisterBitMask(mfrc522[readerNumber].RFCfgReg, (0x07<<4));
	// mfrc522[readerNumber].PCD_SetRegisterBitMask(mfrc522[readerNumber].RFCfgReg, 0x07);
	// delay(50);
	if (mfrc522[readerNumber].PICC_IsNewCardPresent() && mfrc522[readerNumber].PICC_ReadCardSerial()) {
		memcpy(readingStorage[readerNumber], mfrc522[readerNumber].uid.uidByte, 4);
		isPICCpresent = true;
	}
	mfrc522[readerNumber].PICC_HaltA();
	mfrc522[readerNumber].PCD_StopCrypto1();
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

void printRFIDVersions()
{
	for (uint8_t readerNumber = 0; readerNumber < NUMBER_OF_READERS; readerNumber++) {
		mfrc522[readerNumber].PCD_Init(ssPins[readerNumber], RST_PIN); // Init each MFRC522 card
		Serial.print(F("Reader "));
		Serial.print(readerNumber);
		Serial.print(F(": "));
		mfrc522[readerNumber].PCD_DumpVersionToSerial();
	}
}

void printMultipleRFID()
{
	for (uint8_t readerNumber = 0; readerNumber < NUMBER_OF_READERS; readerNumber++) {
		if (getRFID(readerNumber)) {
			Serial.print(F("Reader "));
			Serial.print(readerNumber);
			Serial.print(F(": Card UID:"));
			printUID(readingStorage[readerNumber]);
			Serial.println();
			newRFIDAppeared = true;
		}
	}
}

String getUIDFromReadingStorage(int readerNumber)
{
	String content = "";
	for (byte i = 0; i < 4; i++) {
		content.concat(String(readingStorage[readerNumber][i] < 0x10 ? " 0" : " "));
		content.concat(String(readingStorage[readerNumber][i], HEX));
	}
	content.toUpperCase();
	String theUID = content.substring(1);
	return theUID;
}