#include "utils/iot_utils.hpp"
long lastReconnectAttempt = 0;

#include <Servo.h>

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
bool estado_electroiman_tablero_electrico = true;

Servo servo;
int angle = 0;
int solved_steps = 0;

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
	if (strcmp(topic, ELECTROIMAN_CALDERA_TOPIC) == 0) {
		bool status = doc["status"];
		if (status) {
			debugger.message("Activando electroiman caldera");
			digitalWrite(PIN_ELECTROIMAN_CALDERA, LOW);
		} else {
			debugger.message("Desactivando electroiman caldera");
			digitalWrite(PIN_ELECTROIMAN_CALDERA, HIGH);
		}

	} else if (strcmp(topic, ELECTROIMAN_TABLERO_ELECTRICO_TOPIC) == 0) {
		bool status = doc["status"];
		if (status) {
			debugger.message("Activando electroiman tablero electrico");
			digitalWrite(PIN_ELECTROIMAN_TABLERO, LOW);
		} else {
			debugger.message("Desactivando electroiman tablero electrico");
			digitalWrite(PIN_ELECTROIMAN_TABLERO, HIGH);
		}
	}
}
void setup()
{
	Serial.begin(115200);
	while (!Serial) {
		;
	}
// See the reason of this includes here: https://arduino.stackexchange.com/a/84896
#ifdef ESP32
#include "soc/rtc_cntl_reg.h"
#include "soc/soc.h"
	WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // disable brownout detector
#endif
	connect_mqtt_broker();
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
	mqttc.setCallback(messageHandler);
	mqttc.subscribe(ELECTROIMAN_CALDERA_TOPIC, 1);
	mqttc.subscribe(ELECTROIMAN_TABLERO_ELECTRICO_TOPIC, 1);
	servo.attach(PIN_SERVO);
	debugger.message("Finished configuration");
	debugger.requiered_loops = 50;
}
void loop()
{
	if (!mqttc.connected()) {
		long now = millis();
		if (now - lastReconnectAttempt > 5000) {
			lastReconnectAttempt = now;
			// Attempt to reconnect
			if (nonblocking_reconnect()) {
				lastReconnectAttempt = 0;
				Serial.println("Reconnected, YEAH!!!");
				mqttc.subscribe(ELECTROIMAN_CALDERA_TOPIC, 1);
				mqttc.subscribe(ELECTROIMAN_TABLERO_ELECTRICO_TOPIC, 1);
			} else {
				Serial.println("Disconnected from MQTT broker, now attempting a reconnection");
			}
		}
	} else {

#pragma region reading
		// Analog read has a resolution of 12 bits ---> 0 to 4095
		// -------------------- START OF READING SECTION --------------------
		for (int i = 0; i < N_SENSORES_PROXIMIDAD; i++) {
			sensores_proximidad[i].lectura = analogRead(pines_proximidad[i]);
			// Serial.printf("Analog read del sensor %i: %i\n", i, sensores_proximidad[i].lectura);
		}
		for (int i = 0; i < N_ATENUADORES; i++) {
			atenuadores[i].lectura = analogRead(pines_atenuadores[i]);
		}
		botones.actual = (digitalRead(PIN_BOTONES_CORRECTO) == true && digitalRead(PIN_BOTONES_INCORRECTO) == false) ? true : false;
		interruptores.actual = (digitalRead(PIN_INTERRUPTORES_CORRECTO) == true && digitalRead(PIN_INTERRUPTORES_INCORRECTO) == false) ? true : false;

		// ----------- Debug of the analog readings ------------ //
		debugger.message_number("La lectura del sensor de movimiento 0 es", sensores_proximidad[0].lectura, "debug", true);
		debugger.message_number("La lectura del sensor de movimiento 1 es", sensores_proximidad[1].lectura, "debug", true);
		debugger.message_number("La lectura del sensor de movimiento 2 es", sensores_proximidad[2].lectura, "debug", true);
		debugger.message_number("La lectura del sensor de movimiento 3 es", sensores_proximidad[3].lectura, "debug", true);
		// -------------------- END OF READING SECTION --------------------
#pragma endregion reading

#pragma region threshold
		// -------------------- START OF THRESHOLD SECTION --------------------
		for (int i = 0; i < N_SENSORES_PROXIMIDAD; i++) {
			sensores_proximidad[i].actual = (sensores_proximidad[i].lectura > THRESHOLD) ? true : false; // Si supera el threshold de cercania
			// Serial.printf("ON/OFF Sensor-%i:  %s\n", i, (sensores_proximidad[i].actual ? "ON" : "OFF"));
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
				debugger.message_number("Change in the sensor movimiento N", i);
				sensores_proximidad[i].save_to_previous();
				should_publish = true;
			}
		}
		for (int i = 0; i < N_ATENUADORES; i++) {
			if (atenuadores[i].state_changed()) {
				debugger.message_number("Change in the atenuador N", i);
				should_publish = true;
			}
		}
		if (interruptores.state_changed()) {
			debugger.message("Change in the interruptores");
			should_publish = true;
		}
		if (botones.state_changed()) {
			debugger.message("Change in the buttons");
			should_publish = true;
		}
		// -------------------- END OF CHECK IF STATE CHANGED SECTION --------------------
#pragma endregion changed

#pragma region should_publish
		// -------------------- START OF SAVE TO PREVIOUS SECTION AND PUBLISHING --------------------
		if (should_publish) {
			debugger.message("I should be publishing!!!");
			should_publish = false;
			for (int i = 0; i < N_SENSORES_PROXIMIDAD; i++) {
				sensores_proximidad[i].save_to_previous();
			}
			for (int i = 0; i < N_ATENUADORES; i++) {
				atenuadores[i].save_to_previous();
			}
			botones.save_to_previous();
			interruptores.save_to_previous();

			StaticJsonDocument<256> tablero_doc;
			char jsonBuffer[256];
			JsonArray array_atenuadores = tablero_doc.createNestedArray("atenuadores");
			for (Sensor atenuador : atenuadores) {
				array_atenuadores.add(atenuador.actual);
			}
			tablero_doc["botones"] = botones.actual;
			tablero_doc["interruptores"] = interruptores.actual;
			tablero_doc["electroiman_caldera"] = estado_electroiman_caldera;
			tablero_doc["electroiman_tablero"] = estado_electroiman_tablero_electrico;
			serializeJson(tablero_doc, jsonBuffer);
			report_reading_to_broker("tablero_electrico", jsonBuffer);

			//--------------------- LLAVES PASO ---------------------//
			StaticJsonDocument<128> llaves_paso_doc;
			char jsonBuffer_2[128];
			JsonArray llaves_array = llaves_paso_doc.createNestedArray("llaves_paso");
			for (Sensor sensor_proximidad : sensores_proximidad) {
				llaves_array.add(sensor_proximidad.actual);
			}
			serializeJson(llaves_paso_doc, jsonBuffer_2);
			report_reading_to_broker("llaves_paso", jsonBuffer_2);
		}
#pragma endregion should_publish
		// -------------------- END OF SAVE TO PREVIOUS SECTION AND PUBLISHING --------------------

#pragma region servo_control
		// -------------------- START OF SERVO CONTROL SECTION --------------------
		if (botones.actual == true)
			solved_steps += 1;
		if (atenuadores[0].actual == true && atenuadores[1].actual == true)
			solved_steps += 1;
		if (interruptores.actual == true)
			solved_steps += 1;
		if (sensores_proximidad[1].actual == true && sensores_proximidad[3].actual == true)
			solved_steps += 1;
		switch (solved_steps) {
		case 0:
			angle = 175;
			break;
		case 1:
			angle = 125;
			break;
		case 2:
			angle = 75;
			break;
		case 3:
			angle = 30;
			break;
		case 4:
			angle = 0;
			break;
		default:
			debugger.message_number("ERROR: Wrong solved_steps number, number is: ", solved_steps, "error");
			break;
		}
		servo.write(angle);
		solved_steps = 0;
// -------------------- END OF SERVO CONTROL SECTION --------------------
#pragma endregion servo_control
		local_delay(100); // retardo de 100ms entre lectura
		debugger.loop();
	}
}