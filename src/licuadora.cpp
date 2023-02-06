// A0 es el sensor CNY70, en la placa original estaba a D1 y hay que cambiarlo
// D2 es el pin del MOSFET, hay que mandarle PWM al palo cuando tocan el boton
// DX es el pin del boton, en el codigo pongo D0 pero despues lo cambio a ver bien
#include "secrets/licuadora_secrets.h"
#include "secrets/shared_secrets.h"
#include "utils/iot_utils.hpp"
#define SHADOW_GET_TOPIC "$aws/things/licuadora/shadow/get"
#define SHADOW_GET_ACCEPTED_TOPIC "$aws/things/licuadora/shadow/get/accepted"
#define SHADOW_UPDATE_TOPIC "$aws/things/licuadora/shadow/update"
#define SHADOW_UPDATE_ACCEPTED_TOPIC "$aws/things/licuadora/shadow/update/accepted"
#define SHADOW_UPDATE_DELTA_TOPIC "$aws/things/licuadora/shadow/update/delta"

#define PIN_BOTON D0 // FIXME cambiar esto
#define PIN_CNY70 A0
#define PIN_MOSFET D2
#define THRESHOLD 100

bool current_state = false; // current state of the button
bool previous_state = false;
int lectura_cny70 = 0;

void report_state_to_shadow()
{
	StaticJsonDocument<256> doc;
	char jsonBuffer[256];
	JsonObject state_reported = doc["state"].createNestedObject("reported");
	state_reported["estado_boton"] = current_state;
	serializeJsonPretty(doc, jsonBuffer);
	Serial.println("Reporting the following to the shadow:");
	Serial.println(jsonBuffer);
	client.publish(SHADOW_UPDATE_TOPIC, jsonBuffer);
}

void setup()
{
	Serial.begin(115200);
	connectAWS(WIFI_SSID, WIFI_PASSWORD, THINGNAME, AWS_CERT_CA, AWS_CERT_CRT, AWS_CERT_PRIVATE, AWS_IOT_ENDPOINT); // Connect to AWS
	pinMode(PIN_BOTON, INPUT);
}

void loop()
{
	now = time(nullptr); // The NTP server uses this, if you delete this, the connection to AWS no longer works
	if (!client.connected()) {
		reconnect(THINGNAME, "soporte_cuchillos/status");
	}
	// read the pushbutton input pin:
	current_state = digitalRead(PIN_BOTON);
	// compare the current_state to its previous state
	if (current_state != previous_state) {
		previous_state = current_state;
		report_state_to_shadow();
	}
	lectura_cny70 = analogRead(PIN_CNY70);
	Serial.print(lectura_cny70);
	Serial.print("\t");
	if ((lectura_cny70) > THRESHOLD) {
		Serial.println("\tMOSFET ON");
		analogWrite(PIN_MOSFET, 255);
	} else {
		Serial.println("\tMOSFET OFF");
		analogWrite(PIN_MOSFET, 0);
	}
	// Delay a little bit
	// -------------- THIS IS ONLY FOR DEBUGGING ----------------
	// StaticJsonDocument<128> doc;
	// char jsonBuffer[128];
	// doc["lectura_cny70"] = lectura_cny70;
	// serializeJsonPretty(doc, jsonBuffer);
	// Serial.println("Reporting the following debug:");
	// Serial.println(jsonBuffer);
	// client.publish("licuadora/debug", jsonBuffer);
	local_delay(200);
}