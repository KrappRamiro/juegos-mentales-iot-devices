#ifndef multiple_rfid_utils
#define multiple_rfid_utils

#include "Arduino.h"
#include "MFRC522.h"
bool getRFID(MFRC522* mfrc522, byte readern, byte readedCard[NUMBER_OF_READERS][4]);
void printUID(byte* buffer);

#endif