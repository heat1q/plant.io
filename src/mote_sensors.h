#pragma once

#include "plantio.h"

#include <dev/button-sensor.h>

struct process p_sensors;

/**
 * @brief Handle a button press event on the device.
 * 
 */
void on_button_pressed(void);
