/**
 * @file serial.h
 * @author Patrick Willner (patrick.willner@tum.de), Andreas Koliopoulos (ga96leh@mytum.de), Alexander Schmaus (ga96fin@mytum.de)
 * @brief 
 * @version 0.1
 * @date 2019-12-14
 * 
 * @copyright Copyright (c) 2019
 * 
 */
#pragma once

#include "plantio.h"

struct process p_serial;

/**
 * @brief Parse serial input events, received from the GUI.
 * 
 * @param input 
 */
void parse_serial_input(char* input);