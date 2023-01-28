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

// ------------------ Constants --------------------- //
#define SWITCH_PIN D7
#define N_RGB_LIGHTS 4
//  -------------- Defaults values used for fallback ------------------ //
#define DEFAULT_BRIGHTNESS_LEVEL 150
#define DEFAULT_FLICKER_MIN_TIME 3000
#define DEFAULT_FLICKER_MAX_TIME 5000
#define DEFAULT_BLACKOUT_MIN_TIME 15000
#define DEFAULT_BLACKOUT_MAX_TIME 30000
// -------------------- Global variables ------------------- //
int rn;
StaticJsonDocument<1024> doc;
bool flag_update_values = false; // Represents if update_values should be called
unsigned long flicker_previousMillis = 0; // will store last time LED was updated
unsigned long blackout_previousMillis = 0; // will store last time LED was updated
unsigned long flicker_interval;
unsigned long blackout_interval;
// ---------------------- Functions declarations -------------------//
void messageHandler(const char* topic, byte* payload, unsigned int length);
void report_state_to_shadow();
void update_values();
void flicker();
void blackout();

#pragma region ClassDefinition

class Config {
	String mode;
	byte fixed_brightness;
	unsigned int flicker_min_time;
	unsigned int flicker_max_time;
	unsigned int blackout_min_time;
	unsigned int blackout_max_time;

public:
	Config()
	{
		this->fixed_brightness = DEFAULT_BRIGHTNESS_LEVEL;
		this->flicker_min_time = DEFAULT_FLICKER_MIN_TIME;
		this->flicker_max_time = DEFAULT_FLICKER_MAX_TIME;
		this->blackout_min_time = DEFAULT_BLACKOUT_MIN_TIME;
		this->blackout_max_time = DEFAULT_BLACKOUT_MAX_TIME;
	}
	// ----------- Setters ----------- //
	void set_mode(String mode)
	{
		if (mode == "fixed") {
			Serial.println("Setting the mode to fixed");
		} else if (mode == "scary") {
			Serial.println("Setting the mode to scary");
		} else if (mode == "panic") {
			Serial.println("Setting the mode to panic");
		} else {
			Serial.printf("ERROR: the mode %s is not supported\n", mode);
			return;
		}
		this->mode = mode;
	}
	void set_fixed_brightness(byte brightness)
	{
		if (0 > brightness && brightness < 256) {
			Serial.println("ERROR in config: Brightness should be between 0 and 255");
			return;
		}
		Serial.printf("Setting the fixed brightness to %hhu \n", brightness); // %hhu format for byte (uint8_t) https://stackoverflow.com/a/26363608/15965186
		this->fixed_brightness = brightness;
	}
	void set_flicker_time(unsigned int min, unsigned int max)
	{
		Serial.printf("Setting the flickering time to values between %u and %u \n", min, max);
		flicker_min_time = min;
		flicker_max_time = max;
	}
	void set_blackout_time(unsigned int min, unsigned int max)
	{
		Serial.printf("Setting the blackout time to values between %u and %u \n", min, max);
		blackout_min_time = min;
		blackout_max_time = max;
	}
#pragma region getters
	// ----------- Getters ----------- //

	String get_mode()
	{
		return this->mode;
	}

	byte get_fixed_brightness()
	{
		return this->fixed_brightness;
	}

	int get_flicker_min_time()
	{
		return this->flicker_min_time;
	}

	int get_flicker_max_time()
	{
		return this->flicker_max_time;
	}

	int get_blackout_max_time()
	{
		return this->blackout_max_time;
	}

	int get_blackout_min_time()
	{
		return this->blackout_min_time;
	}
#pragma endregion getters
} config;

class BaseLight {
protected: // This variables needs to be protected so they can be accessed in the derived classes
	byte brightness;
	bool flicker;

public:
	// ----------- Setters ----------- //
	// I dont define the brightness setter in the base class becuase each derived class has its own way to set the brightness
	void set_flicker(bool flicker)
	{
		Serial.printf("Setting the flicker to %i\n", flicker);
		this->flicker = flicker;
	}
	// ----------- Getters ----------- //
	byte get_brightness()
	{
		return brightness;
	}
	bool get_flicker()
	{
		return flicker;
	}
};

class UVLight : public BaseLight {
	byte pin;

public:
	UVLight(byte pin)
	{
		this->pin = pin;
	}
	// ----------- Setters ----------- //
	void set_brightness(byte brightness)
	{
		if (0 > brightness && brightness < 256) {
			Serial.println("ERROR in uv light: Brightness should be between 0 and 255");
			return;
		}
		// Note: Only the UVLight sets the analog values via the set_brightness function
		this->brightness = brightness;
		analogWrite(pin, brightness);
	}
};

class RGBLight : public BaseLight {
	byte red_pin;
	byte blue_pin;
	byte green_pin;
	char* color;

public:
	RGBLight() {};
	RGBLight(byte red_pin, byte blue_pin, byte green_pin, char* color = "white", byte brightness = DEFAULT_BRIGHTNESS_LEVEL, bool flicker = true) // Constructor
	{
		if (0 > brightness && brightness < 256) {
			Serial.println("ERROR in rgb light constructor: Brightness should be between 0 and 255");
			return;
		}
		this->red_pin = red_pin;
		this->blue_pin = blue_pin;
		this->green_pin = green_pin;
		this->brightness = brightness;
		this->flicker = flicker;
		this->color = color;
		update_analog_pins();
	}
	// ----------- Setters ----------- //
	void set_brightness(byte brightness) // Sets the brightness
	{
		if (0 > brightness && brightness < 256) {
			Serial.println("ERROR in rgb light: Brightness should be between 0 and 255");
			return;
		}
		Serial.printf("Setting the RGBLight brightness to: %i\n", brightness);
		this->brightness = brightness;
		update_analog_pins();
	}
	void set_color(char* color) // Sets the color and updates the analog pins
	{
		if ((strcmp(color, "red") == 0) || (strcmp(color, "cyan") == 0) || (strcmp(color, "white") == 0)) {
			Serial.printf("Setting the color to %s\n", color);
			this->color = color;
		} else {
			Serial.printf("ERROR! The color %s is not a valid color!", color);
		}
		update_analog_pins();
	}
	void update_analog_pins()
	{
		if (strcmp(this->color, "red") == 0) {
			// Set the color to red
			analogWrite(red_pin, this->brightness);
			analogWrite(blue_pin, 0);
			analogWrite(green_pin, 0);
		} else if (strcmp(this->color, "cyan") == 0) {
			// Set the color to red
			analogWrite(red_pin, 0);
			analogWrite(blue_pin, this->brightness);
			analogWrite(green_pin, this->brightness);
		} else if (strcmp(this->color, "white") == 0) {
			// Set the color to red
			analogWrite(red_pin, this->brightness);
			analogWrite(blue_pin, this->brightness);
			analogWrite(green_pin, this->brightness);
		}
	}

	// ----------- Getters ----------- //
	String get_color()
	{
		return this->color;
	}
	int get_red_pin()
	{
		return this->red_pin;
	}
	int get_green_pin()
	{
		return this->green_pin;
	}
	int get_blue_pin()
	{
		return this->blue_pin;
	}
};

#pragma endregion class_definition
// ------------------- Lights initialization with PIN values ----------------------- //
RGBLight rgb_lights[N_RGB_LIGHTS] = {
	RGBLight(D0, D1, D1),
	RGBLight(D2, D3, D3),
	RGBLight(D4, D5, D5),
	RGBLight(D7, D8, D8)
};
UVLight uv_light(D6);

void setup()
{
	Serial.begin(115200);
	pinMode(SWITCH_PIN, INPUT);
	// ------------------ Start of AWS connection ----------------------
	connectAWS(WIFI_SSID, WIFI_PASSWORD, THINGNAME, AWS_CERT_CA, AWS_CERT_CRT, AWS_CERT_PRIVATE, AWS_IOT_ENDPOINT); // Connect to AWS
	client.subscribe(SHADOW_GET_ACCEPTED_TOPIC, 1); // Subscribe to the topic that gets the initial state
	client.subscribe(SHADOW_UPDATE_DELTA_TOPIC, 1); // Subscribe to the topic that updates the state every time it changes
	client.setCallback(messageHandler);
	StaticJsonDocument<8> doc;
	char jsonBuffer[8];
	serializeJson(doc, jsonBuffer);
	client.publish(SHADOW_GET_TOPIC, jsonBuffer);
	Serial.print("Waiting for the initial config");
	while (!flag_update_values) {
		Serial.print(".");
		local_delay(100);
	}
	// ------------------ End of AWS connection ----------------------
	// ---------------------- Set up the lights ------------------------ //
	update_values();
	for (int i = 0; i < N_RGB_LIGHTS; i++) {
		rgb_lights[i].update_analog_pins();
	}
	report_state_to_shadow();
	flicker_interval = random(config.get_flicker_min_time(), config.get_flicker_max_time());
	blackout_interval = random(config.get_blackout_min_time(), config.get_blackout_max_time());
	Serial.println("Configuration ended, starting to run the code");
}
void loop()
{
	if (flag_update_values) {
		flag_update_values = false;
		update_values();
		report_state_to_shadow();
	}
	if (config.get_mode() == "fixed") {
		for (int i = 0; i < N_RGB_LIGHTS; i++) {
			// README: I use analogWrite because using set_brightness would modify the value that is going to be reported to the shadow and that would create a mess
			analogWrite(rgb_lights[i].get_red_pin(), config.get_fixed_brightness());
			analogWrite(rgb_lights[i].get_blue_pin(), config.get_fixed_brightness());
			analogWrite(rgb_lights[i].get_green_pin(), config.get_fixed_brightness());
		}
	}
	if (config.get_mode() == "panic") {
		for (int i = 0; i < N_RGB_LIGHTS; i++) {
			// README: I use analogWrite because using set_brightness would modify the value that is going to be reported to the shadow and that would create a mess
			rgb_lights[i].set_color("red");
		}
	}
	if (config.get_mode() == "scary") {
		unsigned long startMillis = millis();
		if (startMillis - flicker_previousMillis >= flicker_interval) {
			flicker_previousMillis = startMillis;
			Serial.println(F("The interval for flickering has passed"));
			flicker();
			flicker_interval = random(config.get_flicker_min_time(), config.get_flicker_max_time());
		}
		if (startMillis - blackout_previousMillis >= blackout_interval) {
			blackout_previousMillis = startMillis;
			Serial.println("The interval for blackout has passed");
			blackout();
			blackout_interval = random(config.get_blackout_min_time(), config.get_blackout_max_time());
		}
	}

	local_delay(500);
}

void update_values()
{
	Serial.println("Updating the values in the document so they can be published later");
	char jsonBuffer[1024];
	serializeJson(doc, jsonBuffer);
	Serial.println("----------------------");
	Serial.println("----------------------\n");

	Serial.println(jsonBuffer);

	Serial.println("\n----------------------");
	Serial.println("----------------------");
	// This functions get the values from the global defined StaticJsonDocument doc variable, and saves those values to the Config, RGBLight and UVLight object
	JsonObject state_desired;
	if (!doc["state"]["desired"].isNull()) { // Check if there is ["state"]["desired"] in the document
		Serial.println("The document is a get/accepted type");
		state_desired = doc["state"]["desired"];
	} else {
		Serial.println("The document is a update/delta type");
		state_desired = doc["state"];
	}
	JsonObject doc_config = state_desired["config"];
	// config.set_mode(doc_config["mode"] | config.get_mode()); // WARNING! Dont do this, for some reason it generates an error in ArduinoJson
	// ---------------- Getting the config --------------------- //
	if (doc_config["mode"] != nullptr) {
		config.set_mode(doc_config["mode"].as<String>());
	}
	config.set_fixed_brightness(
		doc_config["fixed_brightness"] | config.get_fixed_brightness());
	config.set_flicker_time(
		doc_config["flicker_min_time"] | config.get_flicker_min_time(),
		doc_config["flicker_max_time"] | config.get_flicker_max_time());
	config.set_blackout_time(
		doc_config["blackout_min_time"] | config.get_blackout_min_time(),
		doc_config["blackout_max_time"] | config.get_blackout_max_time());
	// --------------------- Getting the values for the RGB Lights ------------------- //
	int i = 0;
	for (JsonObject rgb_light : state_desired["rgb_lights"].as<JsonArray>()) {
		rgb_lights[i].set_brightness(
			rgb_light["brightness"] | rgb_lights[i].get_brightness());
		rgb_lights[i].set_flicker(rgb_light["flicker"] | rgb_lights[i].get_flicker());
		i++;
	}
	// --------------------- Getting the values for the UV light ------------------- //
	uv_light.set_brightness(state_desired["uv_light"]["brightness"] | uv_light.get_brightness());
	uv_light.set_flicker(state_desired["uv_light"]["flicker"] | uv_light.get_flicker());
	doc.clear(); // IMPORTANT: Remember to clear the Json Document each time the values are updated
}

void report_state_to_shadow()
{
	char jsonBuffer[1024];
	JsonObject state_reported = doc["state"].createNestedObject("reported");
	JsonObject doc_config = state_reported.createNestedObject("config");
	doc_config["mode"] = config.get_mode();
	doc_config["fixed_brightness"] = config.get_fixed_brightness();
	doc_config["flicker_min_time"] = config.get_flicker_min_time();
	doc_config["flicker_max_time"] = config.get_flicker_max_time();
	doc_config["blackout_min_time"] = config.get_blackout_min_time();
	doc_config["blackout_max_time"] = config.get_blackout_max_time();

	JsonArray doc_rgb_lights = state_reported.createNestedArray("rgb_lights");

	JsonObject rgb_lights_0 = doc_rgb_lights.createNestedObject();
	rgb_lights_0["brightness"] = rgb_lights[0].get_brightness();
	rgb_lights_0["flicker"] = rgb_lights[0].get_flicker();

	JsonObject rgb_lights_1 = doc_rgb_lights.createNestedObject();
	rgb_lights_1["brightness"] = rgb_lights[1].get_brightness();
	rgb_lights_1["flicker"] = rgb_lights[1].get_flicker();

	JsonObject rgb_lights_2 = doc_rgb_lights.createNestedObject();
	rgb_lights_2["brightness"] = rgb_lights[2].get_brightness();
	rgb_lights_2["flicker"] = rgb_lights[2].get_flicker();

	JsonObject rgb_lights_6 = doc_rgb_lights.createNestedObject();
	rgb_lights_6["brightness"] = rgb_lights[3].get_brightness();
	rgb_lights_6["flicker"] = rgb_lights[3].get_flicker();

	JsonObject doc_uv_light = state_reported.createNestedObject("uv_light");
	doc_uv_light["brightness"] = uv_light.get_brightness();
	doc_uv_light["flicker"] = uv_light.get_flicker();
	serializeJsonPretty(doc, jsonBuffer);
	Serial.println("Reporting the following to the shadow:");
	Serial.println(jsonBuffer);
	const char* result = client.publish(SHADOW_UPDATE_TOPIC, jsonBuffer) ? "Shadow reporting success!!" : "Shadow reporting not successful";
	Serial.println(result);
}

void messageHandler(const char* topic, byte* payload, unsigned int length)
{
	Serial.printf("\nMESSAGE HANDLER: Topic: %s\n", topic);
	// ------------ RETRIEVING THE SHADOW DOCUMENT FROM AWS -------------//
	if (strcmp(topic, SHADOW_GET_ACCEPTED_TOPIC) == 0) {
		StaticJsonDocument<64> filter;
		filter["state"]["desired"] = true;
		DeserializationError error = deserializeJson(doc, (const byte*)payload, length, DeserializationOption::Filter(filter));
		if (error) {
			Serial.print(F("deserializeJson() failed: "));
			Serial.println(error.f_str());
			return;
		}
	}
	// ------------------------------------------------------------------//

	// ----------------- RETRIEVING THE DELTA FROM AWS ------------------//
	if (strcmp(topic, SHADOW_UPDATE_DELTA_TOPIC) == 0) {
		StaticJsonDocument<64> filter;
		filter["state"] = true;
		DeserializationError error = deserializeJson(doc, (const byte*)payload, length, DeserializationOption::Filter(filter));
		if (error) {
			Serial.print(F("deserializeJson() failed: "));
			Serial.println(error.f_str());
			return;
		}
	}
	// ------------------------------------------------------------------//
	flag_update_values = true;
}

void flicker()
{
	// REMEMBER: For this functions, we dont modify the .brightness class member, we just use analogWrite with the PIN_N
	Serial.println("Flickering");
}

void blackout()
{
	// REMEMBER: For this functions, we dont modify the .brightness class member, we just use analogWrite with the PIN_N
	Serial.println("Blackouting");
}
