/**
 * @file iot_utils.cpp
 * @author Krapp Ramiro (krappramiro@disroot.org)
 * @brief File used for defining all the utils function that are used in the usage of MQTT, also used to define the MQTTDebug class.
 * Here a WifiClient instance, a PubSubClient instance and a MQTTDebug instance are also created.
 * @version 1.0
 * @date 2023-03-13
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "utils/iot_utils.hpp"
#include "secrets/shared_secrets.h"
#ifdef ESP32
#include "iot_utils.hpp"
#include "soc/rtc_wdt.h"
#endif
time_t now;
time_t nowish = 1510592825;
// The reason why net and client are both in the header and in the source: https://stackoverflow.com/questions/74729454/platformio-c-multiple-multiple-definition-of
WiFiClient esp_client = WiFiClient();
PubSubClient mqttc(esp_client);
MQTTDebug debugger = MQTTDebug();

#pragma region MQTTDebug_class
/// @brief The default constructor for MQTTDebug class
MQTTDebug::MQTTDebug()
{
}
/// @brief This function should be called at the end of every loop in your code
void MQTTDebug::loop()
{
	loop_counter += 1;
	if (loop_counter == requiered_loops) {
		// if the loop counter has reached the point of requiered loops, debug for the following loop
		should_debug_polling = true;
	} else if (loop_counter > requiered_loops) {
		// if the loop counter has passed the point of requiered loops, stop debugging and reset the counter
		should_debug_polling = false;
		loop_counter = 0;
	}
}
/// @brief Generates a message that is published to the MQTT broker and Serial printed. This will be published at <THINGNAME>/debug/subtopic
/// @param message The message that is going to be published
/// @param subtopic The subtopic where you want to publish the message. Optional parameter, defaults to "info"
void MQTTDebug::message(const char* message, const char* subtopic, bool polling)
{
	// Debug para solamente texto
	if (polling && !should_debug_polling) {
		return;
	}
	Serial.printf("%s: %s\n", subtopic, message);
	// THINGNAME / debug / subtopic
	char topic[100] = "";
	strcat(topic, THINGNAME);
	strcat(topic, "/debug/");
	strcat(topic, subtopic);
	mqttc.publish(topic, message);
}

void MQTTDebug::message_number(const char* message, int number, const char* subtopic, bool polling)
{
	if (polling && !should_debug_polling) {
		return;
	}
	// Debug que soporta printear un numero
	// Appendeo de number a message
	char integer_string[32];
	char new_msg[128];
	strcpy(new_msg, message); // copy the content of message to a new string
	sprintf(integer_string, "%i", number); // put the number inside an string
	strcat(new_msg, integer_string); // message + number

	Serial.printf("%s: %s\n", subtopic, new_msg);
	// THINGNAME / debug / subtopic
	char topic[100] = "";
	strcat(topic, THINGNAME);
	strcat(topic, "/debug/");
	strcat(topic, subtopic);
	mqttc.publish(topic, new_msg);
}
#pragma endregion MQTTDebug_class

void connect_mqtt_broker()
{
	/**
	 * @brief Connects to the MQTT broker. Expects the following to be defined via includes or compile flags (-D platformIO flag):
	 * WIFI_SSID
	 * THINGNAME
	 * WIFI_PASSWORD
	 * MQTT_BROKER
	 * MQTT_PORT
	 *
	 */
	Serial.printf("Connecting to Wi-Fi %s\n", WIFI_SSID);
	WiFi.mode(WIFI_STA);
	char hostname[40] = "ESP_";
	strcat(hostname, THINGNAME);
	WiFi.setHostname(hostname);
	WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
	while (WiFi.status() != WL_CONNECTED) {
		delay(250);
		Serial.print(".");
	}
	mqttc.setServer(MQTT_BROKER, MQTT_PORT);
	mqttc.setBufferSize(6000); // See https://github.com/knolleary/pubsubclient/issues/485#issuecomment-435236670
	mqttc.setKeepAlive(150);
	mqttc.setSocketTimeout(150);
	Serial.println("Connecting to MQTT broker");
	while (!mqttc.connected()) {
		Serial.printf("The client %s attempts to connect to the mqtt broker\n", THINGNAME);
		if (mqttc.connect(THINGNAME)) {
			Serial.println("Successfully connected to MQTT broker");
			debugger.message("Connected to the broker!");
		} else {
			Serial.printf("failed with rc=%i\n", mqttc.state());
			delay(2000);
		}
	}
}

void reconnect()
{
	// Loop until we're reconnected
	while (!mqttc.connected()) {
		Serial.print("Attempting MQTT connection...");
		// Attempt to connect
		if (mqttc.connect(THINGNAME)) {
			Serial.println("connected");
		} else {
			Serial.print("failed, rc=");
			Serial.print(mqttc.state());
			Serial.println(" try again in 3 seconds");
			// Wait 3 seconds before retrying
			delay(3000);
		}
	}
}

void NTPConnect(void)
{
	Serial.print("Setting time using SNTP");
	configTime(-3 * 3600, 0 * 3600, "pool.ntp.org", "time.nist.gov");
	now = time(nullptr);
	while (now < nowish) {
		delay(500);
		Serial.print(".");
		now = time(nullptr);
	}
	Serial.println("done!");
	struct tm timeinfo;
	gmtime_r(&now, &timeinfo);
	Serial.print("Current time: ");
	Serial.print(asctime(&timeinfo));
}

void local_yield()
// See https://sigmdel.ca/michel/program/esp8266/arduino/watchdogs_en.html
{
#ifdef ESP32
	//	rtc_wdt_feed();
	yield();
#elif defined(ESP8266)
	ESP.wdtFeed();
#endif
	mqttc.loop();
}

void local_delay(unsigned long millisecs)
{
	unsigned long start = millis();
	local_yield();
	if (millisecs > 0) {
		while (millis() < millisecs + start) {
			local_yield();
		}
	}
}
void report_reading_to_broker(const char* subtopic, char* jsonBuffer)
{
	// This function serializes the JsonDocument into the JsonBuffer and then publishes it to <THINGNAME>/readings/<subtopic>
	char topic[100] = "";
	strcat(topic, THINGNAME);
	strcat(topic, "/readings/");
	strcat(topic, subtopic);
	const char* result = (mqttc.publish(topic, jsonBuffer)) ? "success publishing" : "failed publishing";
	Serial.println(result);
}
