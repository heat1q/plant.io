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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <contiki.h>
#include <dev/leds.h>
#include <dev/serial-line.h>
#include <lib/mmem.h>

/**
 * @brief Macro for allocating dynamic memory.
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
