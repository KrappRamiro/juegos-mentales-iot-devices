#ifndef multiple_rfid_utils
#define multiple_rfid_utils

#include "Arduino.h"
#include <MFRC522.h>
#include <SPI.h>
#include <string.h>
bool getRFID(MFRC522* mfrc522, byte readern, byte readedCard[NUMBER_OF_READERS][4]);
void printUID(byte* buffer);
void readRFIDVersions(MFRC522* mfrc522, const byte* ssPins);
void readMultipleRFID(MFRC522* mfrc522, byte readedCard[NUMBER_OF_READERS][4]);
String getUIDFromReadingStorage(byte readedCard[NUMBER_OF_READERS][4], int readerNumber);
#endif