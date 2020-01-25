/**
 * @file serial.h
 * @author Patrick Willner (patrick.willner@tum.de), Andreas Koliopoulos (ga96leh@mytum.de), Alexander Schmaus (ga96fin@mytum.de)
 * @brief Defines the serial communication between a node and the GUI.
 * @version 0.1
 * @date 2019-12-14
 * 
 * @details
 * The following Node to GUI strings are defined:
 * 
 * Output String (from mote)                                                           Description
 * 
 * <$src_id:route:$r_1:$_2:...>                                                        get optimal path to node with id $src_id
 * 
 * <$src_id:th:$temp_low:$temp_high:$hum_low:$hum_high:$light_low:$light_high>         get thresholds of node with id $src_id
 * 
 * <$src_id:sensor_data:$temp_1:...:$temp_2:$hum_1:...:$hum_max:$light_1:...:$light_max>get sensors data (temp, hum, light) of node with id $src_id with datapoints 1 to max
 * 
 * <$src_id:rt:$data>                                                                  get routing table of node with id $src_id with $data formatted equivalent to file structure
 * 
 * <$src_id:ack>                                                                       acknowledgement for received datapacket from node with id $src_id
 *
 * 
 * The following GUI to Node strings are defined:
 * 
 * Input String (from GUI)                                                             Description
 * 
 * 0:init                                                                              Start network discovery from current node
 * 
 * $dest_id:led:$args                                                                  Set LED of node $dest_id, where $args = {4: blue, 2: green, 1: red, 7: all, ...RGB Addition}
 * 
 * $dest_id:rt                                                                         Fetch routing table of node $dest_id
 * 
 * $dest_id:get_th                                                                     Fetch thresholds of node $dest_id
 * 
 * $dest_id:set_th:$temp_low:$temp_high:$hum_low:$hum_high:$light_low:$light_high      Set thresholds of node $dest_id
 * 
 * $dest_id:get_data                                                                   Get MAX_NUM_OF_VALUES values of sensor data (temp, hum, light)
 * 
 * @copyright Copyright (c) 2019
 * 
 */
#pragma once

#include "plantio.h"

/**
 * @brief Process for handling serial input.
 * 
 */
struct process p_serial;

/**
 * @brief Parse serial input events, received from the GUI.
 * 
 * @param input 
 */
void parse_serial_input(char* input);