#pragma once

#include "plantio.h"

struct process p_sensors;

/**
 * @brief Handle a button press event on the device.
 * 
 */
void on_button_pressed(void);

/**
 * @brief Calculates the light sensor value in Lm.
 * 
 * @param adc_input Voltage
 * @return const uint16_t 
 */
const uint16_t get_light_sensor_value(uint16_t adc_input);

/**
 * @brief Initializes the Device sensors thresholds.
 * 
 */
void init_sensor_data(void);

/**
 * @brief Writes a tuple of sensor data to flash memory.
 * 
 * @details Appends the data to the file on flash memory and 
 * assigns the tuple a unique timestamp.
 * Only keeps the latest MAX_NUM_OF_VALUES number of values.
 * 
 * @param temp Temperatur value
 * @param hum Humidity value
 * @param light Light value
 */
void write_sensor_data(const uint16_t temp, const uint16_t hum, const uint16_t light);

/**
 * @brief Clears all sensor data from the flash memory.
 * 
 */
void clear_sensor_data(void);

/**
 * @brief Prints the senors data.
 * 
 */
void print_sensor_data(void);

/**
 * @brief Retrieve Sensor data from the flash memory.
 * 
 * @details First 6*4 bytes are thresholds.
 * Next 4*2 bytes are timestamp, temp, hum, light for maximum
 * number of MAX_NUM_VALUES
 * 
 * @param index Index starting from 6*4 + Index
 * @return const uint16_t Sensors data value
 */
const uint16_t fetch_sensor_data(const uint16_t index);

/**
 * @brief Write the sensors thresholds to the flash memory.
 * 
 * @param str Threshold as string formatted, seperated by ':'
 */
void write_thresholds(char *str);

/**
 * @brief Get sensor threshold value.
 * 
 * @param id ID to specify which threshold, defined with enum thresh
 * @return const int32_t sensors threshold value
 */
const int32_t get_threshold(enum thresh id);