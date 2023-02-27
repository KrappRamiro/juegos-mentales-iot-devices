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
void connectAWS(const char* wifi_ssid, const char* wifi_password, const char* thingname, const char* aws_cert_ca, const char* aws_cert_crt, const char* aws_cert_private, const char* aws_cert_endpoint);
void connect_mqtt_broker(const char* thingname);
void reconnect(const char* thingname, const char* aws_iot_publish_topic);
void NTPConnect();
void local_yield();
void local_delay(unsigned long millisecs);
#endif