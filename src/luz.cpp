// Important note from the developer: Im sorry for this code, sometimes it gets awful, and does not follow any design pattern. Sorry
#include "Arduino.h"
#include "secrets/luz_secrets.h"
#include "secrets/shared_secrets.h"
#include "utils/iot_utils.hpp"
#define SHADOW_GET_TOPIC "$aws/things/luz/shadow/get"
#define SHADOW_GET_ACCEPTED_TOPIC "$aws/things/luz/shadow/get/accepted"
#define SHADOW_UPDATE_TOPIC "$aws/things/luz/shadow/update"
#define SHADOW_UPDATE_ACCEPTED_TOPIC "$aws/things/luz/shadow/update/accepted"
#define SHADOW_UPDATE_DELTA_TOPIC "$aws/things/luz/shadow/update/delta"
#define SWITCH_PIN D7
#define NUMBER_OF_NORMAL_LIGHTS 2
#define NUMBER_OF_RGB_LIGHTS 2
#define DEFAULT_BRIGHTNESS_LEVEL 150
#define DEFAULT_FLICKER_MIN_TIME 3000
#define DEFAULT_FLICKER_MAX_TIME 5000
#define DEFAULT_OFF_ON_MIN_TIME 15000
#define DEFAULT_OFF_ON_MAX_TIME 30000
int rn;
String mode;
unsigned long flicker_min_time = DEFAULT_FLICKER_MIN_TIME;
unsigned long flicker_max_time = DEFAULT_FLICKER_MAX_TIME;
unsigned long off_on_min_time = DEFAULT_OFF_ON_MIN_TIME;
unsigned long off_on_max_time = DEFAULT_OFF_ON_MAX_TIME;
unsigned long flicker_previousMillis = 0; // will store last time LED was updated
unsigned long flicker_interval = random(DEFAULT_FLICKER_MIN_TIME, DEFAULT_FLICKER_MAX_TIME); // interval at which to make scary light effects (milliseconds)
unsigned long off_on_previousMillis = 0; // will store last time LED was updated
unsigned long off_on_interval = random(DEFAULT_OFF_ON_MIN_TIME, DEFAULT_OFF_ON_MAX_TIME); // interval at which to make scary light effects (milliseconds)
bool should_process_mqtt = false;
bool topic_was_delta = false;
bool topic_was_get_accepted = false;
bool first_time_config_from_shadow = false;
byte fixed_brightness_level;
byte off_on_brightness_level;
bool uv_light_activate = false;
bool uv_light_already_activated = false;
bool switch_status = false;
StaticJsonDocument<1024> global_doc;

class BaseLight {
public:
	String type = "base";
	bool has_to_flicker = false;
	byte PIN_N;
	void turn_off()
	{
		digitalWrite(PIN_N, LOW);
	}
	void set_brightness_level(byte brightness_level)
	{
		this->brightness_level = brightness_level;
		analogWrite(PIN_N, brightness_level);
	}
	byte get_brightness_level()
	{
		return this->brightness_level;
	}

protected:
	byte brightness_level = DEFAULT_BRIGHTNESS_LEVEL;
};
class NormalLight : public BaseLight { // Why use public here? See public inheritance https://stackoverflow.com/a/49804797/15965186
public:
	String type = "normal";
};
class UVLight : public BaseLight {
public:
	String type = "uv";
	bool active = false;
	void flicker()
	{
		byte brightness_level_before_flickering = brightness_level;
		Serial.println("Flickering the UV light");
		for (int i = 0; i < 60; i++) {
			rn = random(brightness_level_before_flickering / 4, brightness_level_before_flickering);
			set_brightness_level(rn);
			local_yield();
			delay(50);
		}
		set_brightness_level(brightness_level_before_flickering);
		Serial.println("Stopped flickering the UV light");
	}
};
class RgbLight : public BaseLight {
public:
	String type = "rgb";
	byte RED_PIN_N;
	byte BLUE_GREEN_PIN_N;
	void set_brightness_level(byte brightness_level)
	{
		this->brightness_level = brightness_level;
		analogWrite(RED_PIN_N, brightness_level);
		analogWrite(BLUE_GREEN_PIN_N, brightness_level);
	}
	void set_red_light(byte brightness_level)
	{
		this->brightness_level_red = brightness_level;
		this->brightness_level_blue_green = 0;
		analogWrite(RED_PIN_N, brightness_level);
		digitalWrite(BLUE_GREEN_PIN_N, LOW);
	}
	void turn_off()
	{
		this->brightness_level_red = 0;
		this->brightness_level_blue_green = 0;
		digitalWrite(RED_PIN_N, LOW);
		digitalWrite(BLUE_GREEN_PIN_N, LOW);
	}

private:
	byte brightness_level_red;
	byte brightness_level_blue_green;
	byte PIN_N; // i want to disable the PIN_N because the RGB Lights use multiple pins
};

NormalLight normal_lights[NUMBER_OF_NORMAL_LIGHTS];
RgbLight rgb_lights[NUMBER_OF_RGB_LIGHTS];
UVLight uv_light;
void turn_off_on()
{
	// Important info: Change "normal_lights_brightness_before_running" to DEFAULT_BRIGHTNESS_LEVEL to get a different styling
	byte normal_lights_brightness_before_running[NUMBER_OF_NORMAL_LIGHTS] = {
		normal_lights[0].get_brightness_level(),
		normal_lights[1].get_brightness_level()
	};
	byte rgb_lights_brightness_before_running[NUMBER_OF_RGB_LIGHTS] = {
		rgb_lights[0].get_brightness_level(),
		rgb_lights[1].get_brightness_level()
	};
	byte uv_light_brightness_before_running = uv_light.get_brightness_level();
	const unsigned long interval = random(3000, 7000); // interval at which to blink (milliseconds)
	Serial.println("Turning off and on the light");

	// ------------ START OF bajada de tension ------------------
	for (int i = 0; i < 60; i++) {
		rn = random(10, off_on_brightness_level - i / 2);
		for (int j = 0; j < NUMBER_OF_NORMAL_LIGHTS; j++) {
			normal_lights[j].set_brightness_level(rn);
		}
		for (int j = 0; j < NUMBER_OF_RGB_LIGHTS; j++) {
			rgb_lights[j].set_brightness_level(rn);
		}
		if (uv_light.active) {
			uv_light.set_brightness_level(rn);
		}
		ESP.wdtFeed();
		local_yield();
		delay(50);
	}
	// ------------- END OF bajada de tension ---------------------------

	// ------------ START OF luces apagadas ------------------
	// "se apagan las luces"
	for (int i = 0; i < NUMBER_OF_NORMAL_LIGHTS; i++) {
		normal_lights[i].turn_off();
	}
	for (int i = 0; i < NUMBER_OF_RGB_LIGHTS; i++) {
		rgb_lights[i].turn_off();
	}
	if (uv_light.active) {
		uv_light.turn_off();
	}
	// Serial.println("Starting the millis phase");
	// wait for some time (interval) to turn off the lights
	unsigned long currentMillis = millis();
	while (1) {
		ESP.wdtFeed();
		local_yield();
		if (millis() - currentMillis >= interval) {
			break;
		}
	}
	// Serial.println("Ending the millis phase");
	// ------------ END OF luces apagadas ------------------

	// ------------ START OF reestablecer la luz de a poco ---------
	for (int i = 0; i < 60; i++) {
		rn = random(20, off_on_brightness_level);
		for (int j = 0; j < NUMBER_OF_NORMAL_LIGHTS; j++) {
			normal_lights[j].set_brightness_level(rn);
		}
		for (int j = 0; j < NUMBER_OF_RGB_LIGHTS; j++) {
			rgb_lights[j].set_brightness_level(rn);
		}
		if (uv_light.active) {
			uv_light.set_brightness_level(rn);
		}
		ESP.wdtFeed();
		local_yield();
		delay(50);
	}
	// ------------ END OF reestablecer la luz de a poco---------

	// ------------ START OF reestablecer la luz totalmente ---------
	for (int i = 0; i < NUMBER_OF_NORMAL_LIGHTS; i++) {
		normal_lights[i].set_brightness_level(normal_lights_brightness_before_running[i]);
	}
	for (int i = 0; i < NUMBER_OF_RGB_LIGHTS; i++) {
		rgb_lights[i].set_brightness_level(rgb_lights_brightness_before_running[i]);
	}
	uv_light.set_brightness_level(uv_light_brightness_before_running);
	// ------------ END OF reestablecer la luz totalmente ---------

	Serial.println("Stopped turning off and on the light");
}
void flicker()
{
	byte normal_lights_brightness_before_running[NUMBER_OF_NORMAL_LIGHTS] = {
		normal_lights[0].get_brightness_level(),
		normal_lights[1].get_brightness_level()
	};
	byte rgb_lights_brightness_before_running[NUMBER_OF_RGB_LIGHTS] = {
		rgb_lights[0].get_brightness_level(),
		rgb_lights[1].get_brightness_level()
	};
	Serial.println("Flickering the light");
	for (int i = 0; i < 60; i++) {
		for (int j = 0; j < NUMBER_OF_NORMAL_LIGHTS; j++) {
			rn = random(normal_lights_brightness_before_running[j] / 4, normal_lights_brightness_before_running[j]);
			if (normal_lights[j].has_to_flicker) {
				normal_lights[j].set_brightness_level(rn);
			}
		}
		for (int j = 0; j < NUMBER_OF_RGB_LIGHTS; j++) {
			rn = random(rgb_lights_brightness_before_running[j] / 4, rgb_lights_brightness_before_running[j]);
			if (rgb_lights[j].has_to_flicker) {
				rgb_lights[j].set_brightness_level(rn);
			}
		}
		local_yield();
		ESP.wdtFeed();
		delay(50);
	}
	for (int i = 0; i < NUMBER_OF_NORMAL_LIGHTS; i++) {
		if (normal_lights[i].has_to_flicker) {
			normal_lights[i].set_brightness_level(normal_lights_brightness_before_running[i]);
		}
	}
	for (int i = 0; i < NUMBER_OF_RGB_LIGHTS; i++) {
		if (rgb_lights[i].has_to_flicker) {
			rgb_lights[i].set_brightness_level(rgb_lights_brightness_before_running[i]);
		}
	}
	Serial.println("Stopped flickering the light");
}
void report_state_to_shadow()
{
	StaticJsonDocument<2048> doc;
	char jsonBuffer[2048];
	JsonObject state_reported = doc["state"].createNestedObject("reported");

	state_reported["mode"] = mode;
	state_reported["flicker_min_time"] = flicker_min_time;
	state_reported["flicker_max_time"] = flicker_max_time;
	state_reported["off_on_min_time"] = off_on_min_time;
	state_reported["off_on_max_time"] = off_on_max_time;
	state_reported["uv_light_brightness_level"] = uv_light.get_brightness_level();
	state_reported["uv_light_active"] = uv_light.active;
	state_reported["fixed_brightness_level"] = fixed_brightness_level;
	state_reported["off_on_brightness_level"] = off_on_brightness_level;
	state_reported["switch_status"] = switch_status;

	JsonArray state_reported_led_lights = state_reported.createNestedArray("led_lights");
	JsonObject state_reported_led_lights_0 = state_reported_led_lights.createNestedObject();
	state_reported_led_lights_0["has_to_flicker"] = normal_lights[0].has_to_flicker;
	state_reported_led_lights_0["brightness_level"] = normal_lights[0].get_brightness_level();

	JsonObject state_reported_led_lights_1 = state_reported_led_lights.createNestedObject();
	state_reported_led_lights_1["has_to_flicker"] = normal_lights[1].has_to_flicker;
	state_reported_led_lights_1["brightness_level"] = normal_lights[1].get_brightness_level();

	JsonArray state_reported_rgb_lights = state_reported.createNestedArray("rgb_lights");
	JsonObject state_reported_rgb_lights_0 = state_reported_rgb_lights.createNestedObject();
	state_reported_rgb_lights_0["has_to_flicker"] = rgb_lights[0].has_to_flicker;
	state_reported_rgb_lights_0["brightness_level"] = rgb_lights[0].get_brightness_level();

	JsonObject state_reported_rgb_lights_1 = state_reported_rgb_lights.createNestedObject();
	state_reported_rgb_lights_1["has_to_flicker"] = rgb_lights[1].has_to_flicker;
	state_reported_rgb_lights_1["brightness_level"] = rgb_lights[1].get_brightness_level();

	serializeJsonPretty(doc, jsonBuffer);
	Serial.println("Reporting the following to the shadow:");
	Serial.println(jsonBuffer);
	String result = client.publish(SHADOW_UPDATE_TOPIC, jsonBuffer) ? "success!!" : "not successful";
	Serial.println(result);
}

void messageHandler(const char* topic, byte* payload, unsigned int length)
{

	///////////////////////////////////////////////////////
	// -------------- README IMPORTANT INFO ------------ //
	// Please do the less amount of things here,
	// its a very unstable enviroment
	///////////////////////////////////////////////////////

	Serial.printf("MESSAGE HANDLER: Topic: %s\n", topic);
	// ------------ RETRIEVING THE SHADOW DOCUMENT FROM AWS -------------//
	if (strcmp(topic, SHADOW_GET_ACCEPTED_TOPIC) == 0) {
		StaticJsonDocument<64> filter;
		filter["state"]["desired"] = true;
		DeserializationError error = deserializeJson(global_doc, (const byte*)payload, length, DeserializationOption::Filter(filter));
		if (error) {
			Serial.print(F("deserializeJson() failed: "));
			Serial.println(error.f_str());
			return;
		}
		should_process_mqtt = true;
		topic_was_get_accepted = true;
		first_time_config_from_shadow = true;
	}
	// ------------------------------------------------------------------//

	// ----------------- RETRIEVING THE DELTA FROM AWS ------------------//
	if (strcmp(topic, SHADOW_UPDATE_DELTA_TOPIC) == 0) {
		DeserializationError error = deserializeJson(global_doc, (const byte*)payload, length);
		if (error) {
			Serial.print(F("deserializeJson() failed: "));
			Serial.println(error.f_str());
			return;
		}
		should_process_mqtt = true;
		topic_was_delta = true;
	}
	// ------------------------------------------------------------------//

	Serial.println("The message handler ends here");
}

void setup()
{
	Serial.begin(115200);
	ESP.wdtDisable();
	ESP.wdtEnable(10000);
	connectAWS(WIFI_SSID, WIFI_PASSWORD, THINGNAME, AWS_CERT_CA, AWS_CERT_CRT, AWS_CERT_PRIVATE, AWS_IOT_ENDPOINT); // Connect to AWS
	client.subscribe(SHADOW_GET_ACCEPTED_TOPIC, 1); // Subscribe to the topic that gets the initial state
	client.subscribe(SHADOW_UPDATE_DELTA_TOPIC, 1); // Subscribe to the topic that updates the state every time it changes
	client.setCallback(messageHandler);
	// ----------- Get the Shadow document --------------//
	StaticJsonDocument<8> doc;
	char jsonBuffer[8];
	serializeJson(doc, jsonBuffer);
	client.publish(SHADOW_GET_TOPIC, jsonBuffer);
	// --------------------------------------------------//

	// --------------- SETTING UP THE LIGHTS ------------//
	uv_light.PIN_N = D6;
	normal_lights[0].PIN_N = D5;
	normal_lights[1].PIN_N = D4;
	rgb_lights[0].BLUE_GREEN_PIN_N = D3;
	rgb_lights[0].RED_PIN_N = D2;
	rgb_lights[1].BLUE_GREEN_PIN_N = D1;
	rgb_lights[1].RED_PIN_N = D0;
	pinMode(SWITCH_PIN, INPUT);
	// --------------------------------------------------//
}
void loop()
{
	now = time(nullptr); // The NTP server uses this, if you delete this, the connection to AWS no longer works
	if (!client.connected()) {
		reconnect(THINGNAME, "luz/status");
	}
	client.loop();
	while (!first_time_config_from_shadow) {
		Serial.println(F("Waiting until we get the shadow document from AWS"));
		while (1) {
			Serial.print(F("."));
			local_delay(150);
			if (first_time_config_from_shadow) {
				Serial.println(F("Got the shadow document, starting to run!!!"));
				break;
			}
		}
	}
	if (should_process_mqtt) {
		int i = 0;
		JsonObject state_desired;
		if (topic_was_delta) {
			state_desired = global_doc["state"];
			topic_was_delta = false;
		}
		if (topic_was_get_accepted) {
			state_desired = global_doc["state"]["desired"];
			topic_was_get_accepted = false;
		}
		if (state_desired["mode"] != nullptr) {
			mode = state_desired["mode"].as<String>();
		}
		flicker_min_time = state_desired["flicker_min_time"] | flicker_min_time;
		flicker_max_time = state_desired["flicker_max_time"] | flicker_max_time;
		off_on_min_time = state_desired["off_on_min_time"] | off_on_min_time;
		off_on_max_time = state_desired["off_on_max_time"] | off_on_max_time;
		fixed_brightness_level = state_desired["fixed_brightness_level"] | fixed_brightness_level;
		off_on_brightness_level = state_desired["off_on_brightness_level"] | off_on_brightness_level;

		uv_light.set_brightness_level(state_desired["uv_light_brightness_level"] | uv_light.get_brightness_level());
		uv_light.active = state_desired["uv_light_active"] | false;
		if (uv_light.active && !uv_light_already_activated) {
			uv_light_activate = true;
		}
		for (JsonObject led_light : state_desired["led_lights"].as<JsonArray>()) {
			normal_lights[i].has_to_flicker = led_light["has_to_flicker"]; // true, true
			normal_lights[i].set_brightness_level(led_light["brightness_level"]); // 160, 160
			i++;
		}
		i = 0;
		for (JsonObject rgb_light : state_desired["rgb_lights"].as<JsonArray>()) {
			rgb_lights[i].has_to_flicker = rgb_light["has_to_flicker"]; // true, true
			rgb_lights[i].set_brightness_level(rgb_light["brightness_level"]);
			i++;
		}
		i = 0;

		should_process_mqtt = false;
		global_doc.clear();
		report_state_to_shadow();
	}

	if (uv_light_activate) {
		uv_light.flicker();
		uv_light_activate = false;
		uv_light_already_activated = true;
	}
	// if (digitalRead(SWITCH_PIN) == HIGH) { //HACK Uncomment this when uploading to the real board
	if (0) {
		Serial.println(F("Detected the switch"));
		if (switch_status)
			switch_status = false;
		else
			switch_status = true;
		report_state_to_shadow();
		local_delay(300); // I delay it a little bit so the player cant turn it on-off by holding the switch
	}

	if (mode == "fixed") {
		// When the lights are off, the mode should be also fixed and with brightness levels of 0
		normal_lights[0].set_brightness_level(fixed_brightness_level);
		normal_lights[1].set_brightness_level(fixed_brightness_level);
		rgb_lights[0].set_brightness_level(fixed_brightness_level);
		rgb_lights[1].set_brightness_level(fixed_brightness_level);
	} else if (mode == "scary") {
		unsigned long startMillis = millis();
		if (startMillis - flicker_previousMillis >= flicker_interval) {
			flicker_previousMillis = startMillis;
			Serial.println(F("The interval for flickering has passed"));
			flicker();
			flicker_interval = random(flicker_min_time, flicker_max_time);
		}
		if (startMillis - off_on_previousMillis >= off_on_interval) {
			off_on_previousMillis = startMillis;
			Serial.println("The interval for off_on has passed");
			turn_off_on();
			off_on_interval = random(off_on_min_time, off_on_max_time);
		}
	} else if (mode == "panic") {
		normal_lights[0].turn_off();
		normal_lights[1].turn_off();
		rgb_lights[0].set_red_light(255);
		rgb_lights[1].set_red_light(255);
	}
	delay(100);
}