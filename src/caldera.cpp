#include "Arduino.h"
#include "secrets/caldera_secrets.h"
#include "secrets/shared_secrets.h"
#include "utils/iot_utils.hpp"
#define SHADOW_GET_TOPIC "$aws/things/caldera/shadow/get"
#define SHADOW_GET_ACCEPTED_TOPIC "$aws/things/caldera/shadow/get/accepted"
#define SHADOW_UPDATE_TOPIC "$aws/things/caldera/shadow/update"
#define SHADOW_UPDATE_ACCEPTED_TOPIC "$aws/things/caldera/shadow/update/accepted"
#define SHADOW_UPDATE_DELTA_TOPIC "$aws/things/caldera/shadow/update/delta"

///////////////////////////////////////////////////////
///////////////////////////////////////////////////////
/////////////////      README       ///////////////////
// THIS CODE NEEDS TO BE REDONE TO SUPPORT MULTIPLEXERS
///////////////////////////////////////////////////////
///////////////////////////////////////////////////////

#define THRESHOLD 1500
#define N_LLAVES_PASO 4
#define N_ATENUADORES 2
#define N_INTERRUPTORES 10
#define N_BOTONES 9
#define PIN_ELECTROIMAN_CALDERA 23 // TODO: Definir esto con barla
#define PIN_ELECTROIMAN_TABLERO 22 // TODO: Definir esto con barla

int pines_proximidad[] = {
	36, 39, 34, 35 // pines ADC1
};
int pines_atenuador[] = {
	32, 33 // pines ADC1
};
int pines_interruptores[] = {
	// 25,26,27 --> MUX pins
	// 14,13 -----> Normal pins
	25, 26, 27, 14, 13
};
int pines_botones[] = {
	// 21, 19, 18 ---> MUX pins
	// 5 ------------> Normal pins
	21, 19, 18, 5
};

bool estado_electroiman = true;
struct Sensor {
	int nivel;
	bool actual;
	bool previous;
	bool state_changed()
	{
		return actual != previous;
	}
	void save_to_previous()
	{
		previous = actual;
	}
} sensores_proximidad[4], atenuadores[2], interruptores[10];

void report_state_to_shadow()
{
	StaticJsonDocument<512> doc;
	char jsonBuffer[512];

	JsonObject state_reported = doc["state"].createNestedObject("reported");

	JsonArray state_reported_interruptores = state_reported.createNestedArray("interruptores");
	state_reported_interruptores.add(true);
	state_reported_interruptores.add(false);
	state_reported_interruptores.add(true);
	state_reported_interruptores.add(true);
	state_reported_interruptores.add(false);
	state_reported_interruptores.add(true);
	state_reported_interruptores.add(false);
	state_reported_interruptores.add(true);
	state_reported_interruptores.add(false);
	state_reported_interruptores.add(true);

	JsonArray state_reported_llaves_paso = state_reported.createNestedArray("llaves_paso");
	state_reported_llaves_paso.add(true);
	state_reported_llaves_paso.add(false);
	state_reported_llaves_paso.add(true);
	state_reported_llaves_paso.add(true);

	JsonArray state_reported_atenuadores = state_reported.createNestedArray("atenuadores");
	state_reported_atenuadores.add(false);
	state_reported_atenuadores.add(true);
	state_reported["botones_presionados"] = true;
	serializeJsonPretty(doc, jsonBuffer);
	Serial.println("Reporting the following to the shadow:");
	Serial.println(jsonBuffer);
	client.publish(SHADOW_UPDATE_TOPIC, jsonBuffer);
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
	pinMode(PIN_INTERRUPTOR_1, INPUT);
	pinMode(PIN_INTERRUPTOR_2, INPUT);
	pinMode(PIN_ELECTROIMAN, OUTPUT);
}
void loop()
{
	now = time(nullptr); // The NTP server uses this, if you delete this, the connection to AWS no longer works
	// Analog read has a resolution of 12 bits ---> 0 to 4095
	proximidad_1.actual = (analogRead(PIN_SENSOR_1) > THRESHOLD) ? true : false;
	proximidad_2.actual = (analogRead(PIN_SENSOR_2) > THRESHOLD) ? true : false;
	proximidad_3.actual = (analogRead(PIN_SENSOR_3) > THRESHOLD) ? true : false;
	proximidad_4.actual = (analogRead(PIN_SENSOR_4) > THRESHOLD) ? true : false;
	atenuador_1.nivel = analogRead(PIN_ATENUADOR_1);
	atenuador_2.nivel = analogRead(PIN_ATENUADOR_2);
	atenuador_1.actual = (1800 < atenuador_1.nivel && atenuador_1.nivel < 2200) ? true : false;
	atenuador_2.actual = (800 < atenuador_2.nivel && atenuador_2.nivel < 1200) ? true : false;
	interruptor_1.actual = digitalRead(PIN_INTERRUPTOR_1);
	interruptor_2.actual = digitalRead(PIN_INTERRUPTOR_2);

	if (proximidad_1.state_changed() || proximidad_2.state_changed() || proximidad_3.state_changed() || proximidad_4.state_changed() || atenuador_1.state_changed() || atenuador_2.state_changed() || interruptor_1.state_changed() || interruptor_2.state_changed()) {
		Serial.println("Cambio!");
		proximidad_1.save_to_previous();
		proximidad_2.save_to_previous();
		proximidad_3.save_to_previous();
		proximidad_4.save_to_previous();
		atenuador_1.save_to_previous();
		atenuador_2.save_to_previous();
		interruptor_1.save_to_previous();
		interruptor_2.save_to_previous();
		report_state_to_shadow();
	}

	Serial.printf("Sensor 1:%i \nSensor 2:%i \nSensor 3:%i \nSensor 4:%i \n", analogRead(PIN_SENSOR_1), analogRead(PIN_SENSOR_2), analogRead(PIN_SENSOR_3), analogRead(PIN_SENSOR_4));
	Serial.printf("Atenuador 1:%i \nAtenuador 2:%i\n", atenuador_1.nivel, atenuador_2.nivel);
	Serial.println("-----------------------------------------------------");
	local_delay(1000); // retardo de 2 segundos entre lectura
}