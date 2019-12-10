#pragma once

#include "plantio.h"

struct process p_serial;

/**
 * @brief Parse serial input events, received from the GUI.
 * 
 * @param input 
 */
void parse_serial_input(char* input);