#include "rfid_utils.hpp"
// The reason why mfrc522 is both in the header and in the source: https://stackoverflow.com/questions/74729454/platformio-c-multiple-multiple-definition-of
MFRC522 mfrc522(SS_PIN, RST_PIN); // Defino los pines que van al modulo RC522
String getUID(MFRC522& rfid_mfrc522)
{
	/*
	Esta funcion retorna el UID del tag que se esta leyendo
	Args:
		None
	Returns:
		String con el UID del tag
	*/
	// conseguido de https://randomnerdtutorials.com/security-access-using-mfrc522-rfid-reader-with-arduino/
	String content = "";
	for (byte i = 0; i < rfid_mfrc522.uid.size; i++) {
		content.concat(String(rfid_mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
		content.concat(String(rfid_mfrc522.uid.uidByte[i], HEX));
	}
	content.toUpperCase();
	String theUID = content.substring(1);
	return theUID;
}
