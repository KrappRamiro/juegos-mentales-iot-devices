#include "classes/light_config.hpp"
#include "utils/iot_utils.hpp"
// ------------------ Constants --------------------- //
#define SWITCH_PIN A0
#define N_RGB_LIGHTS 4
//  -------------- Defaults values used for fallback ------------------ //
int rn;
unsigned long flicker_start_millis = 0; // will store last time LED was updated
unsigned long blackout_start_millis = 0; // will store last time LED was updated
unsigned long flicker_interval;
unsigned long blackout_interval;

// LightConfig light_config = LightConfig();
MQTTDebug deb = MQTTDebug();

void setup() { }
void loop() { }
/*
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
	// ----------- Getters ----------- //
	byte get_pin()
	{
		return this->pin;
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

void flicker()
{
	// REMEMBER: For this functions, we dont modify the .brightness class member, we just use analogWrite with the PIN_N
	Serial.println("Flickering");
	for (int i = 0; i < 60; i++) {
		for (int j = 0; j < N_RGB_LIGHTS; j++) {
			rn = random(rgb_lights[j].get_brightness() / 4, rgb_lights[j].get_brightness());
			if (rgb_lights[j].get_flicker()) {
				analogWrite(rgb_lights[j].get_blue_pin(), rn);
				analogWrite(rgb_lights[j].get_red_pin(), rn);
				analogWrite(rgb_lights[j].get_green_pin(), rn);
			}
		}
		delay(50);
		ESP.wdtFeed();
	}

	for (int i = 0; i < N_RGB_LIGHTS; i++) {
		if (rgb_lights[i].get_flicker()) {
			analogWrite(rgb_lights[i].get_blue_pin(), rgb_lights[i].get_brightness());
			analogWrite(rgb_lights[i].get_red_pin(), rgb_lights[i].get_brightness());
			analogWrite(rgb_lights[i].get_green_pin(), rgb_lights[i].get_brightness());
		}
	}
	Serial.println("Stopped Flickering");
}

void blackout()
{
	// REMEMBER: For this functions, we dont modify the .brightness class member, we just use analogWrite with the PIN_N
	Serial.println("Blackouting");
	const unsigned long interval = random(4000, 7000); // interval at which to blink (milliseconds)

	// ------------ START OF bajada de tension ------------------
	for (int i = 0; i < 60; i++) {
		rn = random(10, DEFAULT_BRIGHTNESS_LEVEL - i / 2);
		for (int j = 0; j < N_RGB_LIGHTS; j++) {
			analogWrite(rgb_lights[j].get_blue_pin(), rn);
			analogWrite(rgb_lights[j].get_red_pin(), rn);
			analogWrite(rgb_lights[j].get_green_pin(), rn);
		}
		if (uv_light.get_brightness() > 0) {
			analogWrite(uv_light.get_pin(), rn);
		}
		delay(50);
		ESP.wdtFeed();
	}
	// ------------- END OF bajada de tension ---------------------------

	// ------------ START OF luces apagadas ------------------
	// "se apagan las luces"
	for (int i = 0; i < N_RGB_LIGHTS; i++) {
		analogWrite(rgb_lights[i].get_blue_pin(), 0);
		analogWrite(rgb_lights[i].get_red_pin(), 0);
		analogWrite(rgb_lights[i].get_green_pin(), 0);
	}
	if (uv_light.get_brightness() > 0) {
		analogWrite(uv_light.get_pin(), 0);
	}
	Serial.println("Starting the millis phase");
	// wait for some time (interval) to turn off the lights
	unsigned long currentMillis = millis();
	while (1) {
		if (millis() - currentMillis >= interval) {
			break;
		}
		delay(50);
		ESP.wdtFeed();
	}
	Serial.println("Ending the millis phase");
	// ------------ END OF luces apagadas ------------------

	// ------------ START OF reestablecer la luz de a poco ---------
	Serial.println("Start of reestablecer de a poco");
	for (int i = 0; i < 60; i++) {
		rn = random(20, DEFAULT_BRIGHTNESS_LEVEL);
		for (int j = 0; j < N_RGB_LIGHTS; j++) {
			analogWrite(rgb_lights[i].get_blue_pin(), rn);
			analogWrite(rgb_lights[i].get_red_pin(), rn);
			analogWrite(rgb_lights[i].get_green_pin(), rn);
		}
		if (uv_light.get_brightness() > 0) {
			analogWrite(uv_light.get_pin(), 0);
		}
		delay(50);
		ESP.wdtFeed();
		Serial.print("A");
	}
	Serial.println("\nEnd of reestablecer de a poco");
	// ------------ END OF reestablecer la luz de a poco---------

	// ------------ START OF reestablecer la luz totalmente ---------
	for (int i = 0; i < N_RGB_LIGHTS; i++) {
		analogWrite(rgb_lights[i].get_blue_pin(), rgb_lights[i].get_brightness());
		analogWrite(rgb_lights[i].get_red_pin(), rgb_lights[i].get_brightness());
		analogWrite(rgb_lights[i].get_green_pin(), rgb_lights[i].get_brightness());
	}
	analogWrite(uv_light.get_pin(), uv_light.get_brightness());

	// ------------ END OF reestablecer la luz totalmente ---------

	Serial.println("Stopped blackout");
}

void update_local_values_from_doc()
{
	// This functions get the values from the global defined StaticJsonDocument doc variable, and saves those values to the Config, RGBLight and UVLight object
	debugger.message("Updating the values in the document so they can be published later");
	// ---------------- Getting the config --------------------- //
	if (doc["mode"] != nullptr) {
		config.set_mode(doc["mode"].as<String>());
	}
	config.set_fixed_brightness(
		doc["fixed_brightness"] | config.get_fixed_brightness());
	config.set_flicker_time(
		doc["flicker_min_time"] | config.get_flicker_min_time(),
		doc["flicker_max_time"] | config.get_flicker_max_time());
	config.set_blackout_time(
		doc["blackout_min_time"] | config.get_blackout_min_time(),
		doc["blackout_max_time"] | config.get_blackout_max_time());
	// --------------------- Getting the values for the RGB Lights ------------------- //
	for (int i = 0; i < N_RGB_LIGHTS; i++) {
		rgb_lights[i].set_brightness(
			doc["scary_brightness"] | rgb_lights[i].get_brightness());
		rgb_lights[i].set_flicker(true);
	}
	// --------------------- Getting the values for the UV light ------------------- //
	uv_light.set_brightness(doc["uv_light_active"] ? 255 : 0 | uv_light.get_brightness());
	uv_light.set_flicker(true | uv_light.get_flicker());
	doc.clear(); // IMPORTANT: Remember to clear the Json Document each time the values are updated
}

void messageHandler(const char* topic, byte* payload, unsigned int length)
{
	// This function retrieves the document so it can be used later by update_local_values_from_doc()
	Serial.printf("\nMESSAGE HANDLER: Topic: %s\n", topic);
	if (strcmp(topic, LUZ_CONFIG_TOPIC) == 0) {
		DeserializationError error = deserializeJson(doc, (const byte*)payload, length);
		if (error) {
			Serial.print(F("deserializeJson() failed: "));
			Serial.println(error.f_str());
			return;
		}
	}
	flag_update_local_values_from_doc = true;
}

void setup()
{
	Serial.begin(115200);
	pinMode(SWITCH_PIN, INPUT);
	// ------------------ Start of broker connection ----------------------
	connect_mqtt_broker();
	mqttc.subscribe(LUZ_CONFIG_TOPIC, 1);
	mqttc.setCallback(messageHandler);
	Serial.print("Waiting for the initial config");
	while (!flag_update_local_values_from_doc) {
		Serial.print(".");
		local_delay(100);
	}
	// ------------------ End of broker connection ----------------------
	// ---------------------- Set up the lights ------------------------ //
	update_local_values_from_doc();
	flag_update_local_values_from_doc = false;
	for (int i = 0; i < N_RGB_LIGHTS; i++) {
		rgb_lights[i].update_analog_pins();
	}
	flicker_interval = random(config.get_flicker_min_time(), config.get_flicker_max_time());
	blackout_interval = random(config.get_blackout_min_time(), config.get_blackout_max_time());
	flicker_start_millis = millis();
	blackout_start_millis = millis();
	debugger.message("Configuration ended, starting to run the code");
	debugger.requiered_loops = 10;
}
void loop()
{
	if (flag_update_local_values_from_doc) {
		flag_update_local_values_from_doc = false;
		update_local_values_from_doc();
	}
	Serial.println(analogRead(SWITCH_PIN));
	if (analogRead(SWITCH_PIN) > 500) {
		debugger.message("Detected the switch");
		// --------------- Report switch status ------------------ //
		StaticJsonDocument<32> switchDoc;
		char jsonBuffer[32];
		switchDoc["switch"] = true;
		serializeJson(switchDoc, jsonBuffer);
		report_reading_to_broker("switch", jsonBuffer);
		// ---------------------------------------------------------------------------- //
		local_delay(200); // I delay it a little bit so the player cant turn it on-off by holding the switch
	}
	if (config.get_mode() == "off") {
		for (int i = 0; i < N_RGB_LIGHTS; i++) {
			// README: I use analogWrite because using set_brightness would modify the value that is going to be reported to the shadow and that would create a mess
			analogWrite(rgb_lights[i].get_red_pin(), 0);
			analogWrite(rgb_lights[i].get_blue_pin(), 0);
			analogWrite(rgb_lights[i].get_green_pin(), 0);
		}
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
		if (millis() > flicker_start_millis + flicker_interval) {
			Serial.println(F("The interval for flickering has passed"));
			flicker();
			flicker_start_millis = millis();
			flicker_interval = random(config.get_flicker_min_time(), config.get_flicker_max_time());
		}
		delay(50); // por las dudas
		if (millis() > blackout_start_millis + blackout_interval) {
			Serial.println("The interval for blackout has passed");
			blackout();
			blackout_start_millis = millis();
			blackout_interval = random(config.get_blackout_min_time(), config.get_blackout_max_time());
		}
	}
	local_delay(100);
	debugger.loop();
}

*/