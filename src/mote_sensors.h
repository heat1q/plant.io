#pragma once

#include "plantio.h"

struct process p_sensors;

/**
 * @brief Handle a button press event on the device.
 * 
 */
void on_button_pressed(void);

const uint16_t get_light_sensor_value(float m, float b, uint16_t adc_input);
const uint16_t get_num_sensor_data();
void set_num_sensor_data(const uint16_t num_sensor_data);

void init_sensor_data();
void write_sensor_data(const uint16_t temp, const uint16_t hum, const uint16_t light);
void clear_sensor_data();
void print_sensor_data();

/**
 * @brief Retrieve Sensor data from the flash memory.
 * First 6*4 bytes are thresholds.
 * Next 4*2 bytes are timestamp, temp, hum, light for maximum
 * number of MAX_NUM_VALUES
 * 
 * @param index Index starting from 6*4 + Index
 * @return const uint16_t 
 */
const uint16_t fetch_sensor_data(const uint16_t index);

void write_thresholds(char *str);
const int32_t get_threshold(enum thresh id);