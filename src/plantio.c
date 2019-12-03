#include "serial.h"
//#include "routing.h"

// declare and run the main process
PROCESS(p_main, "Main process");
AUTOSTART_PROCESSES(&p_main);

PROCESS_THREAD(p_main, ev, data)
{
    PROCESS_BEGIN();

    // Configure Radio Channel
    //NETSTACK_CONF_RADIO.set_value(RADIO_PARAM_CHANNEL, 16);

    // start the serial listener
    process_start(&p_serial, NULL);
    //process_start(&p_broadcast, NULL);

    PROCESS_END();
}