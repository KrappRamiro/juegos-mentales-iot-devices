// A0 es el sensor CNY70, en la placa original estaba a D1 y hay que cambiarlo
// D2 es el pin del MOSFET, hay que mandarle PWM al palo cuando tocan el boton
// DX es el pin del boton, en el codigo pongo D0 pero despues lo cambio a ver bien
#include "modules/utils/iot/iot_utils.hpp"
#define BUTTON_PIN D0
#define CNY70_PIN A0
#define MOSFET_PIN D2
#define THRESHOLD 40

bool current_state = false; // current state of the button
bool previous_state = false;
int cny70_reading = 0;
int debug_counter = 0;

void setup()
{
	Serial.begin(115200);
	while (!Serial)
		; // Do nothing until serial connection is opened
	connect_mqtt_broker();
	pinMode(BUTTON_PIN, INPUT);
	debugger.message("Finished configuration");
	debugger.requiered_loops = 10;
}

void loop()
{
	if (!mqttc.connected()) {
		reconnect();
	}
	// read the pushbutton input pin:
	current_state = digitalRead(BUTTON_PIN);
	// compare the current_state to its previous state
	if (current_state != previous_state) {
		previous_state = current_state;
		StaticJsonDocument<64> doc;
		char jsonBuffer[64];
		doc["switch"] = current_state;
		serializeJson(doc, jsonBuffer);
		report_reading_to_broker("switch", jsonBuffer);
	}
	cny70_reading = analogRead(CNY70_PIN);
	debugger.message_number("La lectura es ", cny70_reading, "debug", true);
	if ((cny70_reading) > THRESHOLD) {
		analogWrite(MOSFET_PIN, 255);
		debugger.message("MOSFET ON", "info", true);
	} else {
		analogWrite(MOSFET_PIN, 0);
		debugger.message("MOSFET OFF", "info", true);
	}
	local_delay(100);
	debugger.loop();
}