/**
 * @file light_config.cpp
 * @author Krapp Ramiro (krappramiro@disroot.org)
 * @brief The definition of the LightConfig class functions and constructor
 * @version 1.0
 * @date 2023-03-14
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "classes/light_config.hpp"
#include "utils/iot_utils.hpp"
#define DEFAULT_BRIGHTNESS_LEVEL 150
#define DEFAULT_FLICKER_MIN_TIME 30000
#define DEFAULT_FLICKER_MAX_TIME 50000
#define DEFAULT_BLACKOUT_MIN_TIME 600000
#define DEFAULT_BLACKOUT_MAX_TIME 800000
LightConfig::LightConfig()
{
	fixed_brightness = DEFAULT_BRIGHTNESS_LEVEL;
	flicker_min_time = DEFAULT_FLICKER_MIN_TIME;
	flicker_max_time = DEFAULT_FLICKER_MAX_TIME;
	blackout_min_time = DEFAULT_BLACKOUT_MIN_TIME;
	blackout_max_time = DEFAULT_BLACKOUT_MAX_TIME;
}
void LightConfig::set_mode(const char* desired_mode)
{
	if (desired_mode != "fixed" || desired_mode != "scary" || desired_mode != "panic" || desired_mode != "off") {
		debugger.message_string("ERROR setting mode: the following mode is not supported: ", desired_mode, "error");
		return;
	}
	debugger.message_string("Setting the mode to", desired_mode);
	strcpy(mode, desired_mode);
}
void LightConfig::set_fixed_brightness(byte brightness)
{
	if (0 > brightness && brightness < 256) {
		debugger.message_number("ERROR setting fixed brightness: Brightness should be between 0 and 255, but recieved: ", brightness, "error");
		return;
	}
	debugger.message_number("Setting the fixed brightness to ", brightness);
	fixed_brightness = brightness;
}
void LightConfig::set_flicker_min_time(unsigned int time)
{
	debugger.message_number("Setting the flickering min time to ", time);
	flicker_min_time = time;
}
void LightConfig::set_flicker_max_time(unsigned int time)
{
	debugger.message_number("Setting the flickering min time to ", time);
	flicker_min_time = time;
}
void LightConfig::set_blackout_min_time(unsigned int time)
{
	debugger.message_number("Setting the blackout min time to ", time);
	blackout_min_time = time;
}
void LightConfig::set_blackout_max_time(unsigned int time)
{
	debugger.message_number("Setting the blackout max time to ", time);
	blackout_max_time = time;
}
String LightConfig::get_mode()
{
	return mode;
}

byte LightConfig::get_fixed_brightness()
{
	return fixed_brightness;
}

int LightConfig::get_flicker_min_time()
{
	return flicker_min_time;
}

int LightConfig::get_flicker_max_time()
{
	return flicker_max_time;
}

int LightConfig::get_blackout_max_time()
{
	return blackout_max_time;
}

int LightConfig::get_blackout_min_time()
{
	return blackout_min_time;
}