#include "classes/generic_light.hpp"
#include "classes/light_config.hpp"
#include "classes/rgb_light.hpp"
#include "utils/iot_utils.hpp"
// ------------------ Constants --------------------- //
#define SWITCH_PIN A0
#define N_RGB_LIGHTS 4
#define MODE_TOPIC "luz/elements/mode"
#define RGB_BRIGHTNESS_TOPIC "luz/elements/rgb_brightness"
#define UV_BRIGHTNESS_TOPIC "luz/elements/uv_brightness"
#define FLICKER_MIN_TIME_TOPIC "luz/elements/flicker_min_time"
#define FLICKER_MAX_TIME_TOPIC "luz/elements/flicker_max_time"
#define BLACKOUT_MIN_TIME_TOPIC "luz/elements/blackout_min_time"
#define BLACKOUT_MAX_TIME_TOPIC "luz/elements/blackout_max_time"
//  -------------- Defaults values used for fallback ------------------ //
int rn;
unsigned long flicker_start_millis = 0; // will store last time LED was updated
unsigned long blackout_start_millis = 0; // will store last time LED was updated
unsigned long flicker_interval;
unsigned long blackout_interval;
long lastReconnectAttempt = 0;
struct Flags {
	bool mode_changed = true;
} flags;
void flicker();
void blackout();
// ------------------- Lights initialization with PIN values ----------------------- //
LightConfig light_config = LightConfig();

RGBLight rgb_lights[N_RGB_LIGHTS] = {
	RGBLight(D0, D1, D1),
	RGBLight(D2, D3, D3),
	RGBLight(D4, D5, D5),
	RGBLight(D7, D8, D8)
};

GenericLight uv_light(D6);
void messageHandler(const char* topic, byte* payload, unsigned int length)
{
	// This function retrieves the document so it can be used later by update_local_values_from_doc()
	StaticJsonDocument<128> doc;
	DeserializationError error = deserializeJson(doc, (const byte*)payload, length);
	if (error) {
		debugger.message_string("deserializeJson() failed ", error.c_str(), "error");
		return;
	}
	Serial.printf("\nMESSAGE HANDLER: Topic: %s\n", topic);
	if (strcmp(topic, MODE_TOPIC) == 0) {
		light_config.set_mode(doc["mode"]);
		flags.mode_changed = true;
	} else if (strcmp(topic, RGB_BRIGHTNESS_TOPIC) == 0) {
		int brightness = doc["rgb_brightness"];
		// If the mode is off, dont update the analog pins
		bool update_analog = (light_config.get_mode().equals("off"));
		for (int i = 0; i < N_RGB_LIGHTS; i++) {
			rgb_lights[i].set_brightness(brightness, update_analog);
		}
	} else if (strcmp(topic, UV_BRIGHTNESS_TOPIC) == 0) {
		int brightness = doc["uv_brightness"];
		uv_light.set_brightness(brightness);
	} else if (strcmp(topic, FLICKER_MIN_TIME_TOPIC) == 0) {
		light_config.set_flicker_min_time(doc["flicker_min_time"]);
	} else if (strcmp(topic, FLICKER_MAX_TIME_TOPIC) == 0) {
		light_config.set_flicker_max_time(doc["flicker_max_time"]);
	} else if (strcmp(topic, BLACKOUT_MIN_TIME_TOPIC) == 0) {
		light_config.set_blackout_min_time(doc["blackout_min_time"]);
	} else if (strcmp(topic, BLACKOUT_MAX_TIME_TOPIC) == 0) {
		light_config.set_blackout_max_time(doc["blackout_max_time"]);
	} else {
		debugger.message_string("Recieved unexpected topic: ", topic, "error");
	}
}

void setup()
{
	Serial.begin(115200);
	pinMode(SWITCH_PIN, INPUT);
	// ------------------ Start of broker connection ----------------------
	connect_mqtt_broker();
	mqttc.setCallback(messageHandler);
	mqttc.subscribe(MODE_TOPIC, 1);
	mqttc.subscribe(RGB_BRIGHTNESS_TOPIC, 1);
	mqttc.subscribe(UV_BRIGHTNESS_TOPIC, 1);
	mqttc.subscribe(FLICKER_MIN_TIME_TOPIC, 1);
	mqttc.subscribe(FLICKER_MAX_TIME_TOPIC, 1);
	mqttc.subscribe(BLACKOUT_MIN_TIME_TOPIC, 1);
	mqttc.subscribe(BLACKOUT_MAX_TIME_TOPIC, 1);
	flicker_interval = random(light_config.get_flicker_min_time(), light_config.get_flicker_max_time());
	blackout_interval = random(light_config.get_blackout_min_time(), light_config.get_blackout_max_time());
	flicker_start_millis = millis();
	blackout_start_millis = millis();
	debugger.message("Configuration ended, starting to run the code");
	debugger.requiered_loops = 20;
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
				mqttc.subscribe(MODE_TOPIC, 1);
				mqttc.subscribe(RGB_BRIGHTNESS_TOPIC, 1);
				mqttc.subscribe(UV_BRIGHTNESS_TOPIC, 1);
				mqttc.subscribe(FLICKER_MIN_TIME_TOPIC, 1);
				mqttc.subscribe(FLICKER_MAX_TIME_TOPIC, 1);
				mqttc.subscribe(BLACKOUT_MIN_TIME_TOPIC, 1);
				mqttc.subscribe(BLACKOUT_MAX_TIME_TOPIC, 1);
			} else {
				Serial.println("Disconnected from MQTT broker, now attempting a reconnection");
			}
		}
	} else {

		int switch_reading = analogRead(SWITCH_PIN);
		// debugger.message_number("A0 (switch) reading: ", switch_reading, "debug", true);
		if (switch_reading > 500) {
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
		if (flags.mode_changed) {
			flags.mode_changed = false;
			if (light_config.get_mode() == "off") {
				debugger.message("Turning off the lights");
				for (int i = 0; i < N_RGB_LIGHTS; i++) {
					analogWrite(rgb_lights[i].get_blue_pin(), 0);
					analogWrite(rgb_lights[i].get_red_pin(), 0);
					analogWrite(rgb_lights[i].get_green_pin(), 0);
				}
				analogWrite(uv_light.get_pin(), 0);
			}
			if (light_config.get_mode() == "fixed") {
				debugger.message("Putting the lights in fixed");
				for (int i = 0; i < N_RGB_LIGHTS; i++) {
					rgb_lights[i].set_brightness(rgb_lights[i].get_brightness());
					rgb_lights[i].set_color("white");
				}
			}
			if (light_config.get_mode() == "panic") {
				debugger.message("Putting the lights in panic");
				for (int i = 0; i < N_RGB_LIGHTS; i++) {
					rgb_lights[i].set_color("red");
				}
			}
			if (light_config.get_mode() == "scary") {
				for (int i = 0; i < N_RGB_LIGHTS; i++) {
					rgb_lights[i].set_color("white");
				}
			}
		}
		// This is not in the flags.mode_changed conditional, because the scary modes needs to be constantly checking if the interval passed
		if (light_config.get_mode() == "scary") {
			if (millis() > flicker_start_millis + flicker_interval) {
				Serial.println(F("The interval for flickering has passed"));
				flicker();
				flicker_start_millis = millis();
				flicker_interval = random(light_config.get_flicker_min_time(), light_config.get_flicker_max_time());
				debugger.message_number("The flicker interval is: ", flicker_interval, "debug");
			}
			delay(50); // por las dudas
			if (millis() > blackout_start_millis + blackout_interval) {
				Serial.println("The interval for blackout has passed");
				blackout();
				blackout_start_millis = millis();
				blackout_interval = random(light_config.get_blackout_min_time(), light_config.get_blackout_max_time());
				debugger.message_number("The blackout interval is: ", blackout_interval, "debug");
			}
		}
		local_delay(100);
		debugger.loop();
	}
}

void flicker()
{
	// REMEMBER: For this functions, we dont modify the .brightness class member, we just use analogWrite with the PIN_N
	debugger.message("START flickering");
	for (int i = 0; i < 40; i++) {
		for (int j = 0; j < N_RGB_LIGHTS; j++) {
			rn = random(rgb_lights[j].get_brightness() / 4, rgb_lights[j].get_brightness());
			analogWrite(rgb_lights[j].get_blue_pin(), rn);
			analogWrite(rgb_lights[j].get_red_pin(), rn);
			analogWrite(rgb_lights[j].get_green_pin(), rn);
		}
		delay(50);
		ESP.wdtFeed();
	}

	for (int i = 0; i < N_RGB_LIGHTS; i++) {
		analogWrite(rgb_lights[i].get_blue_pin(), rgb_lights[i].get_brightness());
		analogWrite(rgb_lights[i].get_red_pin(), rgb_lights[i].get_brightness());
		analogWrite(rgb_lights[i].get_green_pin(), rgb_lights[i].get_brightness());
	}
	debugger.message("END Flickering");
}

void blackout()
{
	// REMEMBER: For this functions, we dont modify the .brightness class member, we just use analogWrite with the PIN_N
	debugger.message("START blackout");
	const unsigned long interval = random(3000, 4000); // duration of the blackout

	// ------------ START OF bajada de tension ------------------
	for (int i = 0; i < 20; i++) {
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
		yield();
	}
	// ------------- END OF bajada de tension ---------------------------

	// ------------ START OF luces apagadas ------------------
	// "se apagan las luces"
	for (int i = 0; i < N_RGB_LIGHTS; i++) {
		analogWrite(rgb_lights[i].get_blue_pin(), 0);
		analogWrite(rgb_lights[i].get_red_pin(), 0);
		analogWrite(rgb_lights[i].get_green_pin(), 0);
	}
	debugger.message("START millis phase", "debug");
	// wait for some time (interval) to turn off the lights
	unsigned long currentMillis = millis();
	while (1) {
		if (millis() - currentMillis >= interval) {
			break;
		}
		delay(50);
		ESP.wdtFeed();
		yield();
		Serial.print("~");
	}
	Serial.print("\n");
	debugger.message("END millis phase", "debug");
	// ------------ END OF luces apagadas ------------------

	// ------------ START OF reestablecer la luz de a poco ---------
	debugger.message("START reestablecer de a poco", "debug");
	for (int i = 0; i < 40; i++) {
		rn = random(20, DEFAULT_BRIGHTNESS_LEVEL);
		for (int j = 0; j < N_RGB_LIGHTS; j++) {
			analogWrite(rgb_lights[j].get_blue_pin(), rn);
			analogWrite(rgb_lights[j].get_red_pin(), rn);
			analogWrite(rgb_lights[j].get_green_pin(), rn);
		}
		delay(50);
		ESP.wdtFeed();
		yield();
		Serial.print("~");
	}
	Serial.print("\n");
	debugger.message("END reestablecer de a poco", "debug");
	// ------------ END OF reestablecer la luz de a poco---------

	// ------------ START OF reestablecer la luz totalmente ---------
	for (int i = 0; i < N_RGB_LIGHTS; i++) {
		analogWrite(rgb_lights[i].get_blue_pin(), rgb_lights[i].get_brightness());
		analogWrite(rgb_lights[i].get_red_pin(), rgb_lights[i].get_brightness());
		analogWrite(rgb_lights[i].get_green_pin(), rgb_lights[i].get_brightness());
	}

	// ------------ END OF reestablecer la luz totalmente ---------

	debugger.message("END blackout");
}
