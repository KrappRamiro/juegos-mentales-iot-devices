/* List of to-do's :
	- Add support to /rejected topics
 */
#include "iot_utils.hpp"
#include "leds_secrets.h"
#define SHADOW_GET_TOPIC "$aws/things/esp32_leds/shadow/get"
#define SHADOW_GET_ACCEPTED_TOPIC "$aws/things/esp32_leds/shadow/get/accepted"
#define SHADOW_UPDATE_TOPIC "$aws/things/esp32_leds/shadow/update"
#define SHADOW_UPDATE_ACCEPTED_TOPIC "$aws/things/esp32_leds/shadow/update/accepted"
#define SHADOW_UPDATE_DELTA_TOPIC "$aws/things/esp32_leds/shadow/update/delta"
#define PIN_LED_VERDE 18
#define PIN_LED_ROJO 19
#define PIN_LED_AMARILLO 21
int shadowConsultingCount = 0;
int cuenta = 0;

void messageHandler(char* topic, byte* payload, unsigned int length)
{
	Serial.println("-------------------------------");
	int ledRojo = 0;
	int ledVerde = 0;
	int ledAmarillo = 0;
	Serial.print("incoming: ");
	Serial.println(topic);
	StaticJsonDocument<2048> doc;
#pragma region // Region that gets the LED's states
	if (strcmp(topic, SHADOW_GET_ACCEPTED_TOPIC) == 0) {
		Serial.println("Getting the info from the shadow");
		deserializeJson(doc, payload); // Put the info from the payload into the JSON document
		if (doc["state"]["desired"]["ledRojo"] != nullptr) {
			ledRojo = doc["state"]["desired"]["ledRojo"];
			digitalWrite(PIN_LED_ROJO, ledRojo);
		}
		if (doc["state"]["desired"]["ledVerde"] != nullptr) {
			ledVerde = doc["state"]["desired"]["ledVerde"];
			digitalWrite(PIN_LED_VERDE, ledVerde);
		}
		if (doc["state"]["desired"]["ledAmarillo"] != nullptr) {
			ledAmarillo = doc["state"]["desired"]["ledAmarillo"];
			digitalWrite(PIN_LED_AMARILLO, ledAmarillo);
		}

	} else if (strcmp(topic, SHADOW_UPDATE_DELTA_TOPIC) == 0) {
		Serial.println("Getting the delta difference from the shadow");
		deserializeJson(doc, payload); // Put the info from the payload into the JSON document
		if (doc["state"]["ledRojo"] != nullptr) {

			digitalWrite(PIN_LED_ROJO, doc["state"]["ledRojo"]);
		}
		if (doc["state"]["ledVerde"] != nullptr) {

			digitalWrite(PIN_LED_VERDE, doc["state"]["ledVerde"]);
		}
		if (doc["state"]["ledAmarillo"] != nullptr) {

			digitalWrite(PIN_LED_AMARILLO, doc["state"]["ledAmarillo"]);
		}
	}
	// Print the LED's state for debugging info
	Serial.printf("We got the following info:\n"
				  "\tledRojo: %i\n"
				  "\tledVerde: %i\n"
				  "\tledAmarillo: %i\n",
		ledRojo, ledVerde, ledAmarillo);
#pragma endregion

#pragma region // Region that reports the current state to the shadow
	doc.clear();
	doc["state"]["reported"]["ledRojo"] = ledRojo;
	doc["state"]["reported"]["ledVerde"] = ledVerde;
	doc["state"]["reported"]["ledAmarillo"] = ledAmarillo;
	char jsonBuffer[2048];
	serializeJsonPretty(doc, jsonBuffer); // print to client
	Serial.println("Reporting the following to the shadow:");
	Serial.println(jsonBuffer);
	client.publish(SHADOW_UPDATE_TOPIC, jsonBuffer);
#pragma endregion
}

void publishMessage(const char* topic)
{
	Serial.printf("Publishing message to topic %s\n", topic);
	StaticJsonDocument<200> doc;
	char jsonBuffer[200];
	serializeJson(doc, jsonBuffer); // print to client
	bool success = client.publish(topic, jsonBuffer);
	if (success)
		Serial.println("Message publishing successful");
	else
		Serial.println("Message publishing had a problem");
	return;
}

void setup()
{
	Serial.begin(115200);
	connectAWS(WIFI_SSID, WIFI_PASSWORD, THINGNAME, AWS_CERT_CA, AWS_CERT_CRT, AWS_CERT_PRIVATE, AWS_IOT_ENDPOINT);
	client.subscribe(SHADOW_GET_ACCEPTED_TOPIC, 1); // Topic that gets the shadow document
	client.subscribe(SHADOW_UPDATE_DELTA_TOPIC, 1); // Delta topic that tells the device what has to change to match the requested state
	// Create a message handler
	client.setCallback(messageHandler);
	pinMode(PIN_LED_ROJO, OUTPUT);
	pinMode(PIN_LED_VERDE, OUTPUT);
	pinMode(PIN_LED_AMARILLO, OUTPUT);
	// For some weird reason, the ESP32 doesnt catch the get/accepted response at the first attemp
	publishMessage(SHADOW_GET_TOPIC);
	delay(1000);
	publishMessage(SHADOW_GET_TOPIC);
}
void loop()
{
	if (!client.connected()) {
		reconnect(THINGNAME, "esp32_leds/status");
	}
	client.loop(); // Loop the MQTT client
	delay(50); // Just in case
}
