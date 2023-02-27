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
void connect_mqtt_broker(const char* thingname);
void reconnect();
void NTPConnect();
void local_yield();
void local_delay(unsigned long millisecs);
void debug(const char* message, const char* topic);
#endif