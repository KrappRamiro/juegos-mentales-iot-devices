#ifndef multiple_rfid_utils
#define multiple_rfid_utils

#include "Arduino.h"
#include <MFRC522.h>
#include <SPI.h>
#include <string.h>

extern const byte ssPins[];
extern byte readedCard[NUMBER_OF_READERS][4]; // Matrix for storing UID over each reader, its 4 because the UID is stored in the first 4 bytes of the tag, think of it as a "Readed UID Storage"
extern MFRC522 mfrc522[NUMBER_OF_READERS]; // Create MFRC522 instances

bool getRFID(MFRC522* mfrc522, byte readern, byte readedCard[NUMBER_OF_READERS][4]);
void printUID(byte* buffer);
void readRFIDVersions(MFRC522* mfrc522, const byte* ssPins);
void readMultipleRFID(MFRC522* mfrc522, byte readedCard[NUMBER_OF_READERS][4]);
String getUIDFromReadingStorage(byte readedCard[NUMBER_OF_READERS][4], int readerNumber);
#endif