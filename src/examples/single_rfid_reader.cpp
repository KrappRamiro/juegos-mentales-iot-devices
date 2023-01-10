#include "secrets/rfid_secrets.h"
#include "utils/iot_utils.hpp"
#include "utils/single_rfid_utils.hpp"
#define AWS_IOT_PUBLISH_TOPIC "esp32/rfid"

void publishMessage(String uid)
{
	Serial.println("Message publishing started!!!");
	StaticJsonDocument<200> doc;
	doc["rfid"] = uid;
	char jsonBuffer[512];
	serializeJson(doc, jsonBuffer); // print to client
	bool result = client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
	Serial.println(result);
}
void setup()
{
	Serial.begin(115200);
	connectAWS(WIFI_SSID, WIFI_PASSWORD, THINGNAME, AWS_CERT_CA, AWS_CERT_CRT, AWS_CERT_PRIVATE, AWS_IOT_ENDPOINT);
	SPI.begin(); // Inicio el bus SPI
	mfrc522.PCD_Init(); // Inicio el MFRC522
	Serial.println(F("Acerca tu tarjeta RFID\n"));
}
void loop()
{
	if (!client.connected()) {
		reconnect(THINGNAME, "esp32_rfid/status");
	}
	client.loop();
	if (!mfrc522.PICC_IsNewCardPresent()) { // Se espera a que se acerque un tag
		return;
	}
	if (!mfrc522.PICC_ReadCardSerial()) { // Se espera a que se lean los datos
		return;
	}
	String uid = getUID(mfrc522); // Consigo la UID de la tarjeta como un string
	Serial.print("La UID leida es: ");
	Serial.println(uid);
	publishMessage(uid);
	mfrc522.PICC_HaltA(); // Le dice al PICC que se vaya a un estado de STOP cuando esta activo (o sea, lo haltea)

	// Esto "para" la encriptación del PCD (proximity coupling device).  Tiene que ser llamado si o si despues de la comunicacion con una autenticación exitosa, en otro caso no se va a poder iniciar otra comunicación.
	mfrc522.PCD_StopCrypto1();
}