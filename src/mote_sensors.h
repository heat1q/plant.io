#pragma once

#include "plantio.h"

struct process p_sensors;

/**
 * @brief Handle a button press event on the device.
 * 
 */
void on_button_pressed(void);

const uint16_t get_num_sensor_data();
void set_num_sensor_data(const uint16_t num_sensor_data);

void write_sensor_data(const uint16_t temp, const uint16_t hum, const uint16_t light);
const uint16_t fetch_sensor_data(const char *filename, const uint16_t index);
void clear_sensor_data();