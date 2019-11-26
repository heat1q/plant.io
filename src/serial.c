#include "serial.h"

PROCESS(p_serial, "Serial Event listener");

PROCESS_THREAD(p_serial, ev, data)
{
    PROCESS_BEGIN();
    leds_off(LEDS_ALL);

    while (1)
    {
        PROCESS_WAIT_EVENT();

        if (ev == serial_line_event_message)
        {
            parse_serial_input((const char*) data);
        }
    }
    PROCESS_END();
}


static void parse_serial_input(const char* input)
{
    printf("Serial Input Received :: %s \r\n", input);
    //format ID:TASK:ARGS

    char* tmp = strtok(input, ":");
    // check id
    int id = atoi(tmp);

    tmp = strtok(NULL, ":");
    // check task
    char* task = tmp;

    tmp = strtok(NULL, ":");
    //args
    char* args = tmp;

    // continue with correct format
    if (strcmp(task, "led") == 0)
    {
        int led_id;
        if (strcmp(args, "blue") == 0) { led_id=LEDS_BLUE; }
        else if (strcmp(args, "green") == 0) { led_id=LEDS_GREEN; }
        else if (strcmp(args, "red") == 0) { led_id=LEDS_RED; }
        else {led_id=LEDS_ALL; }
        leds_toggle(led_id);
    }


}