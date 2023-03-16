/**
 * @file light_config.hpp
 * @author Krapp Ramiro (krappramiro@disroot.org)
 * @brief The declaration of the LightConfig class
 * @version 1.0
 * @date 2023-03-14
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once
#include "utils/iot_utils.hpp"
#define DEFAULT_BRIGHTNESS_LEVEL 150
#define DEFAULT_FLICKER_MIN_TIME 30000
#define DEFAULT_FLICKER_MAX_TIME 50000
#define DEFAULT_BLACKOUT_MIN_TIME 600000
#define DEFAULT_BLACKOUT_MAX_TIME 800000
#define DEFAULT_MODE "off"
/**
 LightConfig class stores the configuration to be used in the lights, such as:
 - The light mode
 - The fixed brightness level
 - The scary brightness level
 - The flickering time
 - The blackout time
*/
class LightConfig {
	char mode[20]; ///< The mode for the light, can be "off", "fixed", "scary", or "panic"
	unsigned int flicker_min_time; ///< The minimum time for the flickering period
	unsigned int flicker_max_time; ///< The maximum time for the flickering period
	unsigned int blackout_min_time; ///< The minimum time for the blackout period
	unsigned int blackout_max_time; ///< The maximum time for the blackout period

public:
	/**
	 * @brief Default constructor
	 */
	LightConfig();
	/**
	 * @brief Set the light mode
	 *
	 * @param mode Can be:
	 * - "scary" (lights flicker and blackout).
	 * - "fixed" (lights have a fixed brightness).
	 * - "off" (lights are off)
	 * - "panic" (red lights)
	 */
	void set_mode(const char* mode);
	/**
	 * @brief Set the flicker min time
	 *
	 * @param time time in millis
	 */
	void set_flicker_min_time(unsigned int time);
	/**
	 * @brief Set the flicker max time
	 *
	 * @param time time in millis
	 */
	void set_flicker_max_time(unsigned int time);
	/**
	 * @brief Set the blackout min time
	 *
	 * @param time time in millis
	 */
	void set_blackout_min_time(unsigned int time);
	/**
	 * @brief Set the blackout max time
	 *
	 * @param time time in millis
	 */
	void set_blackout_max_time(unsigned int time);
	/**
	 * @brief Get the mode
	 *
	 * @returns String with the mode
	 */
	String get_mode();
	/**
	 * @brief Get the flicker min time
	 *
	 * @return int with the flicker_min_time
	 */
	int get_flicker_min_time();
	/**
	 * @brief Get the flicker max time
	 *
	 * @return int with the flicker_max_time
	 */
	int get_flicker_max_time();
	/**
	 * @brief Get the blackout min time
	 *
	 * @return int with the blackout_min_time
	 */
	int get_blackout_min_time();
	/**
	 * @brief Get the blackout max time
	 *
	 * @return int with the blackout_max_time
	 */
	int get_blackout_max_time();
};