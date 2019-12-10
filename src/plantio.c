#include "serial.h"
#include "routing.h"
//#include "sensors.h"

// declare and run the main process
PROCESS(p_main, "Main process");
AUTOSTART_PROCESSES(&p_main);


PROCESS_THREAD(p_main, ev, data)
{
    PROCESS_BEGIN();

    // Configure Radio Channel
    NETSTACK_CONF_RADIO.set_value(RADIO_PARAM_CHANNEL, 16);
    NETSTACK_CONF_RADIO.set_value(RADIO_PARAM_TXPOWER, 7);

    // start the processes
    process_start(&p_serial, NULL);
    process_start(&p_broadcast, NULL);
    //process_start(&p_sensors, NULL);

    PROCESS_END();
}