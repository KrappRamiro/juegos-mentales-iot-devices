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
	byte brightness;
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
	 */
	void set_brightness(byte brightness);
	// ----------- Getters ----------- //
	/**
	 * @brief Get the brightness
	 *
	 * @return byte with value between 0 and 255
	 */
	byte get_brightness();
	byte get_pin();
};