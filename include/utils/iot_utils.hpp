#ifndef iot_utils
#define iot_utils
#ifdef ESP32
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <time.h>
// The reason why net and client are both in the header and in the source: https://stackoverflow.com/questions/74729454/platformio-c-multiple-multiple-definition-of
extern time_t now;
extern time_t nowish;
extern WiFiClient esp_client;
extern PubSubClient mqttc;

class MQTTDebug {
	// my variables declaration here
	int loop_counter = 0; // this starts at 0, and should count the number of loops
	bool should_debug_polling = false;

public:
	int requiered_loops = 0;
	void loop(); // should be called ONCE at the end of void loop
	MQTTDebug(); // The constructor here
	// function declarations here
	void message(const char* message, const char* subtopic = "info", bool polling = false);
	void message_number(const char* message, unsigned long number, const char* subtopic = "info", bool polling = false);
	void message_string(const char* message, const char* string, const char* subtopic = "info", bool polling = false);
};
extern MQTTDebug debugger;

void connect_mqtt_broker();
void reconnect();
void NTPConnect();
void local_yield();
void local_delay(unsigned long millisecs);
void report_reading_to_broker(const char* subtopic, char* jsonBuffer);
bool nonblocking_reconnect();
#endif