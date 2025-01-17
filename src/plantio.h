/**
 * @file plantio.h
 * @author Patrick Willner (patrick.willner@tum.de), Andreas Koliopoulos (ga96leh@mytum.de), Alexander Schmaus (ga96fin@mytum.de)
 * @brief 
 * @version 0.1
 * @date 2019-12-14
 * 
 * @copyright Copyright (c) 2019
 * 
 */
#pragma once

#define PACKETBUF_CONF_SIZE 256

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <contiki.h>
#include <dev/leds.h>
#include <dev/serial-line.h>
#include <lib/mmem.h>

/**
 * @brief Defines the RSSI threshold.
 * 
 * @details Broadcast packets below this thresholds are dropped.
 * 
 */
#define PLANTIO_MIN_RSSI -60

/**
 * @brief Defines the default timeout in seconds before a node transmits
 * its best route to the GUI.
 * 
 */
#define PLANTIO_RREP_TIMEOUT 3

/**
 * @brief Defines the maximum number of sensors data values to be stored.
 * 
 */
#define MAX_NUM_OF_VALUES 10

// structure: 6 * [int32_t] THRESH | LEN * [uint16_t] DATA
#define FILE_SENSORS "fsensors"
#define FILE_ROUTING "frouting"

/**
 * @brief Defines IDs for different Thresholds.
 * 
 */
enum thresh {
    TEMP_LOW,
    TEMP_HIGH,
    HUM_LOW,
    HUM_HIGH,
    LIGHT_LOW,
    LIGHT_HIGH
};

/**
 * @brief Macro for allocating dynamic memory.
 * 
 * @details Uses the contiki managed memory library.
 * 
 * @param managed_memory Managed memory struct instance name
 * @param T Datatype to be allocated
 * @param varname Name of the pointer to T
 * @param size Size of allocated memory
 */
#define plantio_malloc(managed_memory, T, varname, size) \
    struct mmem managed_memory;                          \
    mmem_alloc(&managed_memory, size);                   \
    T *varname = (T *)((struct mmem *)(&managed_memory)->ptr)

/**
 * @brief Frees the dynamic allocated memory.
 * 
 * @param managed_memory Managed memory struct instance name
 */
#define plantio_free(managed_memory) mmem_free(&managed_memory)

