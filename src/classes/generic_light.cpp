/**
 * @file generic_light.cpp
 * @author Krapp Ramiro (krappramiro@disroot.org)
 * @brief The definition of the LightConfig class
 * @version 1.0
 * @date 2023-03-15
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "classes/generic_light.hpp"
GenericLight::GenericLight(byte pin_n)
{
	pin = pin_n;
}
// ----------- Setters ----------- //
void GenericLight::set_brightness(int brightness, bool dont_update_analog)
{
	if (brightness < 0 || brightness > 255) {
		debugger.message_number("ERROR setting generic light brightness: Brightness should be between 0 and 255, but recieved: ", brightness, "error");
		return;
	}
	this->brightness = brightness;
	if (dont_update_analog) {
		debugger.message_number("Setting the generic light brightness WITHOUT UPDATING ANALOG to ", brightness);
	} else {
		debugger.message_number("Setting the generic light brightness to ", brightness);
		analogWrite(pin, brightness);
	}
}
// ----------- Getters ----------- //
int GenericLight::get_brightness()
{
	return brightness;
}
byte GenericLight::get_pin()
{
	return pin;
}