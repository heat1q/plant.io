/**
 * @file plantio.c
 * @author Patrick Willner (patrick.willner@tum.de), Andreas Koliopoulos (ga96leh@mytum.de), Alexander Schmaus (ga96fin@mytum.de)
 * @brief 
 * @version 0.1
 * @date 2019-12-14
 * 
 * @copyright Copyright (c) 2019
 * 
 */

#include "serial.h"
#include "routing.h"
#include "mote_sensors.h"

// declare and run the main process
PROCESS(p_main, "Main process");
AUTOSTART_PROCESSES(&p_main);


PROCESS_THREAD(p_main, ev, data)
{
    PROCESS_BEGIN();
    mmem_init(); // Initialize managed memory library

    // Configure Radio Channel
    NETSTACK_CONF_RADIO.set_value(RADIO_PARAM_CHANNEL, 16);
    NETSTACK_CONF_RADIO.set_value(RADIO_PARAM_TXPOWER, RADIO_CONST_TXPOWER_MAX);

    // start the processes
    process_start(&p_serial, NULL);
    process_start(&p_conn, NULL);
    process_start(&p_sensors, NULL);

    PROCESS_END();
}