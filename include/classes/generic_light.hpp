/**
 * @file generic_light.hpp
 * @author Krapp Ramiro (krappramiro@disroot.org)
 * @brief The declaration of the LightConfig class
 * @version 1.0
 * @date 2023-03-15
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once
#include "utils/iot_utils.hpp"
/**
 * @brief The generic light, used to control a generic light
 *
 */
class GenericLight {
	int brightness;
	byte pin;

public:
	/**
	 * @brief Construct a new Generic Light object
	 *
	 * @param pin_n The pin
	 */
	GenericLight(byte pin_n);
	// ----------- Setters ----------- //
	/**
	 * @brief Set the brightness of the light
	 *
	 * @param brightness value between 0 and 255
	 * @param dont_update_analog controls if the analog pins are being updated
	 */
	void set_brightness(int brightness, bool dont_update_analog = false);
	// ----------- Getters ----------- //
	/**
	 * @brief Get the brightness
	 *
	 * @return int with value between 0 and 255
	 */
	int get_brightness();
	byte get_pin();
};