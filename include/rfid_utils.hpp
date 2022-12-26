#ifndef rfid_utils
#define rfid_utils
#include <MFRC522.h>
#define SS_PIN 5 // Pin de SDA del MFRC522
#define RST_PIN 22 // Pin de RST del MFRC522
#define SIZE_BUFFER 18 // Este es el tamaño del buffer con el que voy a estar trabajando.
// Por que es 18? Porque son 16 bytes de los datos del tag, y 2 bytes de checksum
#define MAX_SIZE_BLOCK 16 // algo del RFID
//  ---------------- INICIO DE Variables del MFRC522 ---------------------
// The reason why mfrc522 is both in the header and in the source: https://stackoverflow.com/questions/74729454/platformio-c-multiple-multiple-definition-of
extern MFRC522::MIFARE_Key key;
extern MFRC522::StatusCode status; // Status es el codigo de estado de autenticacion
extern MFRC522 mfrc522; // Defino los pines que van al modulo RC522
// ----------------- FIN DE Variables del MFRC522 ---------------------

String getUID(MFRC522& rfid_mfrc522);
#endif