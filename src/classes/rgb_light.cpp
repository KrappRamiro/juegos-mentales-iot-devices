/**
 * @file rgb_light.cpp
 * @author Krapp Ramiro (krappramiro@disroot.org)
 * @brief The definition of the RGBLight class
 * @version 1.0
 * @date 2023-03-15
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "classes/rgb_light.hpp"

RGBLight::RGBLight(byte red_pin, byte blue_pin, byte green_pin, const char* color, byte brightness)
{
	if (0 > brightness && brightness < 256) {
		Serial.println("ERROR in rgb light constructor: Brightness should be between 0 and 255");
		return;
	}
	this->red_pin = red_pin;
	this->blue_pin = blue_pin;
	this->green_pin = green_pin;
	this->brightness = brightness;
	strcpy(this->color, color);
	update_analog_pins();
}

void RGBLight::set_brightness(byte brightness) // Sets the brightness
{
	if (0 > brightness && brightness < 256) {
		debugger.message_number("ERROR setting RGB light brightness: Brightness should be between 0 and 255, but recieved: ", brightness, "error");
		return;
	}
	debugger.message_number("Setting the generic light brightness to ", brightness);
	this->brightness = brightness;
	update_analog_pins();
}
void RGBLight::set_color(const char* color) // Sets the color and updates the analog pins
{
	if ((strcmp(color, "red") == 0) || (strcmp(color, "cyan") == 0) || (strcmp(color, "white") == 0)) {
		debugger.message_string("Setting the RGB light color to ", color);
		strcpy(this->color, color);
	} else {
		debugger.message_string("Error setting the RGB light color: the following color is not valid: ", color, "error");
	}
	update_analog_pins();
}
void RGBLight::update_analog_pins()
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

byte RGBLight::get_brightness()
{
	return brightness;
}
int RGBLight::get_red_pin()
{
	return this->red_pin;
}
int RGBLight::get_green_pin()
{
	return this->green_pin;
}
int RGBLight::get_blue_pin()
{
	return this->blue_pin;
}