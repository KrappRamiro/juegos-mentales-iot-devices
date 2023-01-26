#include "Arduino.h"
#include "secrets/shared_secrets.h"
#include "secrets/soporte_cuchillos_secrets.h"
#include "utils/iot_utils.hpp"
#define SHADOW_GET_TOPIC "$aws/things/soporte_cuchillos/shadow/get"
#define SHADOW_GET_ACCEPTED_TOPIC "$aws/things/soporte_cuchillos/shadow/get/accepted"
#define SHADOW_UPDATE_TOPIC "$aws/things/soporte_cuchillos/shadow/update"
#define SHADOW_UPDATE_ACCEPTED_TOPIC "$aws/things/soporte_cuchillos/shadow/update/accepted"
#define SHADOW_UPDATE_DELTA_TOPIC "$aws/things/soporte_cuchillos/shadow/update/delta"
#define BUTTON_PIN D0
bool button_state = false; // current state of the button

void report_state_to_shadow()
{
	StaticJsonDocument<256> doc;
	char jsonBuffer[256];
	JsonObject state_reported = doc["state"].createNestedObject("reported");
	state_reported["estado_switch"] = button_state;
	serializeJsonPretty(doc, jsonBuffer);
	Serial.println("Reporting the following to the shadow:");
	Serial.println(jsonBuffer);
	client.publish(SHADOW_UPDATE_TOPIC, jsonBuffer);
}
void setup()
{
	Serial.begin(115200);
	connectAWS(WIFI_SSID, WIFI_PASSWORD, THINGNAME, AWS_CERT_CA, AWS_CERT_CRT, AWS_CERT_PRIVATE, AWS_IOT_ENDPOINT); // Connect to AWS
	pinMode(BUTTON_PIN, INPUT);
}
void loop()
{
	now = time(nullptr); // The NTP server uses this, if you delete this, the connection to AWS no longer works
	if (!client.connected()) {
		reconnect(THINGNAME, "soporte_cuchillos/status");
	}
	// read the pushbutton input pin:
	button_state = digitalRead(BUTTON_PIN);

	// compare the button_state to its previous state
	if (button_state == HIGH) {
		Serial.println("on");
		report_state_to_shadow();
	}

	// Delay a little bit
	local_delay(500);
}