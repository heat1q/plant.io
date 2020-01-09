#include "mote_sensors.h"
#include "routing.h"

PROCESS(p_sensors, "Sensors Event Listener");
PROCESS_THREAD(p_sensors, ev, data)
{
    PROCESS_BEGIN();
    leds_off(LEDS_ALL);

    while (1)
    {
        PROCESS_WAIT_EVENT();

        if (ev == sensors_event)
        {
            if (data == &button_sensor)
            {
                on_button_pressed();
            }
        }
    }
    PROCESS_END();
}

void on_button_pressed(void)
{
    if (button_sensor.value(0) == 0) // pressed
    {
        //init_network(); // for testing
        //init_rreq_reply(find_best_route());
        add_device_to_network();
    }

    /*
    else if (button_sensor.value(0) == 8) // released
    {
        leds_off(LEDS_ALL);
    }
    */
}