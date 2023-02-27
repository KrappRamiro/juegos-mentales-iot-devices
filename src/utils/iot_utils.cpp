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

void connect_mqtt_broker(const char* thingname)
{
	Serial.printf("Connecting to Wi-Fi %s\n", WIFI_SSID);
	WiFi.mode(WIFI_STA);
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
		} else {
			Serial.print("failed with rc=");
			Serial.print(mqttc.state());
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
void debug(const char* message, const char* subtopic = "info")
{
	Serial.printf("%s: %s\n", subtopic, message);
	// THINGNAME / debug / subtopic
	char topic[100] = "";
	strcat(topic, THINGNAME);
	strcat(topic, "/debug/");
	strcat(topic, subtopic);
	mqttc.publish(topic, message);
}