#include "utils/multiple_rfid_utils.hpp"
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