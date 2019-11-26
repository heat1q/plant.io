#include "plantio.h"

#include "serial.h"

// declare and run the main process
PROCESS(p_main, "Main process");
AUTOSTART_PROCESSES(&p_main);


PROCESS_THREAD(p_main, ev, data)
{
    PROCESS_BEGIN();
    // start the serial listener
    process_start(&p_serial, NULL);

    while (1)
    {
        PROCESS_WAIT_EVENT();
    }

    PROCESS_END();
}