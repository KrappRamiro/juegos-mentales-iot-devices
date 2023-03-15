/**
 * @file rgb_light.hpp
 * @author Krapp Ramiro (krappramiro@disroot.org)
 * @brief The declaration of the RGBLight class
 * @version 1.0
 * @date 2023-03-15
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once
#include "utils/iot_utils.hpp"

class RGBLight {
	byte red_pin;
	byte blue_pin;
	byte green_pin;
	byte brightness;
	char color[20];

	void update_analog_pins(); ///< This function updates the analog pins, and is called every time the color or the brightness are changed
public:
	/**
	 * @brief Construct a new RGBLight object
	 *
	 * @param red_pin
	 * @param blue_pin
	 * @param green_pin
	 * @param color
	 * @param brightness Value between 0 and 255
	 */
	RGBLight(byte red_pin, byte blue_pin, byte green_pin, const char* color = "white", byte brightness = 150);
	/**
	 * @brief Sets the brightness, value between 0 and 255
	 *
	 * @param brightness value between 0 and 255
	 */
	void set_brightness(byte brightness);
	/**
	 * @brief Sets the color
	 *
	 * @param color "white", "cyan" or "red"
	 */
	void set_color(const char* color);
	/**
	 * @brief Get the brightness
	 *
	 * @return byte with a value between 0 and 255
	 */
	byte get_brightness();
	int get_red_pin();
	int get_green_pin();
	int get_blue_pin();
};