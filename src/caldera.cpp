#include "Arduino.h"
#include "secrets/caldera_secrets.h"
#include "secrets/shared_secrets.h"
#include "utils/iot_utils.hpp"
#include <Servo.h>
#define SHADOW_GET_TOPIC "$aws/things/caldera/shadow/get"
#define SHADOW_GET_ACCEPTED_TOPIC "$aws/things/caldera/shadow/get/accepted"
#define SHADOW_UPDATE_TOPIC "$aws/things/caldera/shadow/update"
#define SHADOW_UPDATE_ACCEPTED_TOPIC "$aws/things/caldera/shadow/update/accepted"
#define SHADOW_UPDATE_DELTA_TOPIC "$aws/things/caldera/shadow/update/delta"

#define THRESHOLD 500
#define N_SENSORES_PROXIMIDAD 4
#define N_ATENUADORES 2

#define PIN_ELECTROIMAN_CALDERA 26
#define PIN_ELECTROIMAN_TABLERO 25

#define PIN_INTERRUPTORES_INCORRECTO 23 // Metimos logica combinacional porque no nos daba la cantidad de puertos GPIO
#define PIN_INTERRUPTORES_CORRECTO 22 // Metimos logica combinacional porque no nos daba la cantidad de puertos GPIO
#define PIN_BOTONES_INCORRECTO 2 // Metimos logica combinacional porque no nos daba la cantidad de puertos GPIO
#define PIN_BOTONES_CORRECTO 15 // Metimos logica combinacional porque no nos daba la cantidad de puertos GPIO
#define PIN_SERVO 27

bool flag_report_state_to_shadow = false;

int pines_proximidad[N_SENSORES_PROXIMIDAD] = {
	36, 39, 34, 35 // pines ADC1
};
int pines_atenuadores[N_ATENUADORES] = {
	32, 33 // pines ADC1
};

bool should_publish = false;
bool estado_electroiman_caldera = true;
bool estado_electroiman_tablero = true;

Servo servo;
int angle = 0;

struct Sensor {
	int lectura; // WARNING: Only use for ANALOG READINGS, for digitalReadings, use actual
	bool actual;
	bool previous = false;
	int min; // WARNING: Only use for atenuadores
	int max; // WARNING: Only use for atenuadores
	bool state_changed()
	{
		// Serial.printf("actual is: %i and previous is %i\n", actual, previous);
		if (actual != previous) {
			// Serial.println("State has changed");
			return true;
		} else {
			// Serial.println("State has not changed");
			return false;
		}
	}
	void save_to_previous()
	{
		// Serial.println("Saving to previous");
		previous = actual;
	}
} sensores_proximidad[N_SENSORES_PROXIMIDAD], atenuadores[N_ATENUADORES], interruptores, botones;
void messageHandler(char* topic, byte* payload, unsigned int length)
{
	StaticJsonDocument<256> doc;
	JsonObject state_desired;
	deserializeJson(doc, payload); // Put the info from the payload into the JSON document
	Serial.printf("MESSAGE HANDLER: Topic: %s\n", topic);
	// ------------ RETRIEVING THE SHADOW DOCUMENT FROM AWS -------------//
	if (strcmp(topic, SHADOW_GET_ACCEPTED_TOPIC) == 0)
		state_desired = doc["state"]["desired"];
	else if (strcmp(topic, SHADOW_UPDATE_DELTA_TOPIC) == 0)
		state_desired = doc["state"];

	estado_electroiman_caldera = state_desired["electroiman_caldera"] | estado_electroiman_caldera;
	estado_electroiman_tablero = state_desired["electroiman_tablero"] | estado_electroiman_tablero;

	Serial.printf("Estado electroiman caldera: %s\n", estado_electroiman_caldera ? "true" : "false");
	Serial.printf("Estado electroiman tablero: %s\n", estado_electroiman_tablero ? "true" : "false");
	digitalWrite(PIN_ELECTROIMAN_CALDERA, estado_electroiman_caldera);
	digitalWrite(PIN_ELECTROIMAN_TABLERO, estado_electroiman_tablero);
	flag_report_state_to_shadow = true;
	// ------------------------------------------------------------------//
}
void report_state_to_shadow()
{
	StaticJsonDocument<512> doc;
	char jsonBuffer[512];

	JsonObject state_reported = doc["state"].createNestedObject("reported");

	JsonArray state_reported_llaves_paso = state_reported.createNestedArray("llaves_paso");
	for (Sensor sensor_proximidad : sensores_proximidad) {
		state_reported_llaves_paso.add(sensor_proximidad.actual);
	}

	JsonArray state_reported_atenuadores = state_reported.createNestedArray("atenuadores");
	for (Sensor atenuador : atenuadores) {
		state_reported_atenuadores.add(atenuador.actual);
	}
	state_reported["botones"] = botones.actual;
	state_reported["interruptores"] = interruptores.actual;
	state_reported["electroiman_caldera"] = estado_electroiman_caldera;
	state_reported["electroiman_tablero"] = estado_electroiman_tablero;
	serializeJsonPretty(doc, jsonBuffer);
	Serial.println("Reporting the following to the shadow:");
	Serial.println(jsonBuffer);
	client.publish(SHADOW_UPDATE_TOPIC, jsonBuffer);
	flag_report_state_to_shadow = false;
}
void setup()
{
	Serial.begin(115200);
// See the reason of this includes here: https://arduino.stackexchange.com/a/84896
#ifdef ESP32
#include "soc/rtc_cntl_reg.h"
#include "soc/soc.h"
	WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // disable brownout detector
#endif
	connectAWS(WIFI_SSID, WIFI_PASSWORD, THINGNAME, AWS_CERT_CA, AWS_CERT_CRT, AWS_CERT_PRIVATE, AWS_IOT_ENDPOINT); // Connect to AWS
	pinMode(PIN_ELECTROIMAN_CALDERA, OUTPUT);
	pinMode(PIN_ELECTROIMAN_TABLERO, OUTPUT);
	pinMode(PIN_BOTONES_CORRECTO, INPUT);
	pinMode(PIN_BOTONES_INCORRECTO, INPUT);
	pinMode(PIN_INTERRUPTORES_CORRECTO, INPUT);
	pinMode(PIN_INTERRUPTORES_INCORRECTO, INPUT);
	atenuadores[0].min = 1800;
	atenuadores[0].max = 2200;
	atenuadores[1].min = 1200;
	atenuadores[1].max = 1600;
	client.subscribe(SHADOW_GET_ACCEPTED_TOPIC, 1); // Subscribe to the topic that gets the initial state
	client.subscribe(SHADOW_UPDATE_DELTA_TOPIC, 1); // Subscribe to the topic that updates the state every time it changes
	client.setCallback(messageHandler);
	// ----------- Get the Shadow document --------------//
	StaticJsonDocument<8> doc;
	char jsonBuffer[8];
	serializeJson(doc, jsonBuffer);
	client.publish(SHADOW_GET_TOPIC, jsonBuffer);
	// --------------------------------------------------//
	servo.attach(PIN_SERVO);
}
void loop()
{
	now = time(nullptr); // The NTP server uses this, if you delete this, the connection to AWS no longer works
	if (flag_report_state_to_shadow) {
		report_state_to_shadow();
	}
#pragma region reading
	// Analog read has a resolution of 12 bits ---> 0 to 4095
	// -------------------- START OF READING SECTION --------------------
	for (int i = 0; i < N_SENSORES_PROXIMIDAD; i++) {
		sensores_proximidad[i].lectura = analogRead(pines_proximidad[i]);
		Serial.printf("Analog read del sensor %i: %i\n", i, sensores_proximidad[i].lectura);
	}
	for (int i = 0; i < N_ATENUADORES; i++) {
		atenuadores[i].lectura = analogRead(pines_atenuadores[i]);
	}
	botones.actual = (digitalRead(PIN_BOTONES_CORRECTO) == true && digitalRead(PIN_BOTONES_INCORRECTO) == false) ? true : false;
	interruptores.actual = (digitalRead(PIN_INTERRUPTORES_CORRECTO) == true && digitalRead(PIN_INTERRUPTORES_INCORRECTO) == false) ? true : false;

	// ----------- Debug of the analog readings ------------ //
	StaticJsonDocument<256> debugDoc;
	char jsonBuffer[256];
	debugDoc["sensor-0"] = sensores_proximidad[0].lectura;
	debugDoc["sensor-1"] = sensores_proximidad[1].lectura;
	debugDoc["sensor-2"] = sensores_proximidad[2].lectura;
	debugDoc["sensor-3"] = sensores_proximidad[3].lectura;
	serializeJson(debugDoc, jsonBuffer);
	client.publish("caldera/debug", jsonBuffer);
	// -------------------- END OF READING SECTION --------------------
#pragma endregion reading

#pragma region threshold
	// -------------------- START OF THRESHOLD SECTION --------------------
	for (int i = 0; i < N_SENSORES_PROXIMIDAD; i++) {
		sensores_proximidad[i].actual = (sensores_proximidad[i].lectura > THRESHOLD) ? true : false; // Si supera el threshold de cercania
		Serial.printf("ON/OFF Sensor-%i:  %s\n", i, (sensores_proximidad[i].actual ? "ON" : "OFF"));
	}
	for (int i = 0; i < N_ATENUADORES; i++) {
		// Si el potenciometro esta entre los valores establecidos
		atenuadores[i].actual = (atenuadores[i].min < atenuadores[i].lectura && atenuadores[i].lectura < atenuadores[i].max) ? true : false;
	}
	// -------------------- END OF THRESHOLD SECTION --------------------
#pragma endregion threshold

#pragma region changed
	// -------------------- START OF CHECK IF STATE CHANGED SECTION --------------------
	for (int i = 0; i < N_SENSORES_PROXIMIDAD; i++) {
		if (sensores_proximidad[i].state_changed()) {
			Serial.printf("The state of the sensor_proximidad with index %i has changed\n", i);
			sensores_proximidad[i].save_to_previous();
			should_publish = true;
		}
	}
	for (int i = 0; i < N_ATENUADORES; i++) {
		if (atenuadores[i].state_changed()) {
			Serial.printf("The state of the atenuador with index %i has changed\n", i);
			should_publish = true;
		}
	}
	if (interruptores.state_changed()) {
		Serial.println("The state of the interruptores has changed");
		should_publish = true;
	}
	if (botones.state_changed()) {
		Serial.println("The state of the buttons has changed");
		should_publish = true;
	}

	// -------------------- END OF CHECK IF STATE CHANGED SECTION --------------------
#pragma endregion changed

#pragma region should_publish
	// -------------------- START OF SAVE TO PREVIOUS SECTION --------------------
	if (should_publish) {
		should_publish = false;
		Serial.println("Guardando cambios!!");
		for (int i = 0; i < N_SENSORES_PROXIMIDAD; i++) {
			sensores_proximidad[i].save_to_previous();
		}
		for (int i = 0; i < N_ATENUADORES; i++) {
			atenuadores[i].save_to_previous();
		}
		botones.save_to_previous();
		interruptores.save_to_previous();
		report_state_to_shadow();
	}
#pragma endregion should_publish
	// -------------------- END OF SAVE TO PREVIOUS SECTION --------------------

#pragma region servo_control
	// -------------------- START OF SERVO CONTROL SECTION --------------------
	if (botones.actual == true) {
		angle += 45;
		servo.write(angle);
	} else {
		angle -= 45;
		servo.write(angle);
	}
	if (atenuadores[0].actual == true && atenuadores[1].actual == true) {
		angle += 45;
		servo.write(angle);
	} else {
		angle -= 45;
		servo.write(angle);
	}
	if (interruptores.actual == true) {
		angle += 45;
		servo.write(angle);
	} else {
		angle -= 45;
		servo.write(angle);
	}
	if (sensores_proximidad[0].actual == true && sensores_proximidad[1].actual == true && sensores_proximidad[2].actual == true && sensores_proximidad[3].actual == true) {
		angle += 45;
		servo.write(angle);
	} else {
		angle -= 45;
		servo.write(angle);
	}
	// -------------------- END OF SERVO CONTROL SECTION --------------------
#pragma endregion servo_control
	Serial.println("-------------------------------------------------------------------;");
	Serial.println("-------------------------------------------------------------------;");
	Serial.println("-------------------------------------------------------------------;");
	local_delay(1000); // retardo de 1 segundo entre lectura
}