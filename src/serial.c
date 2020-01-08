/**
 * @file serial.c
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
#include "net_packet.h"

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
            parse_serial_input((char *)data);
        }
    }
    PROCESS_END();
}

/**
 * @brief Parse a serial input char array
 * 
 * @param input 
 */
void parse_serial_input(char *input)
{
    printf("Serial Input Received :: %s \r\n", input);
    //format ID:TASK:ARGS

    char *tmp = strtok(input, ":");
    // check id
    int id = atoi(tmp);

    tmp = strtok(NULL, ":");
    // check task
    char *task = tmp;

    tmp = strtok(NULL, ":");
    //args
    char *args = tmp;

    // continue with correct format
    if (strcmp(task, "led") == 0)
    {
        int led_id;
        if (strcmp(args, "blue") == 0)
        {
            led_id = LEDS_BLUE;
        }
        else if (strcmp(args, "green") == 0)
        {
            led_id = LEDS_GREEN;
        }
        else if (strcmp(args, "red") == 0)
        {
            led_id = LEDS_RED;
        }
        else
        {
            led_id = LEDS_ALL;
        }

        if (id == linkaddr_node_addr.u8[1])
        {
            leds_toggle(led_id);
        }
        else // send to the right node
        {
            
        }
        
    }
    else if (strcmp(task, "init") == 0)
    {
        // start network discovery
        leds_off(LEDS_ALL);
        init_network();
    }
    else if (strcmp(task ,"rt") == 0)
    {
        print_routing_table();
    }
    else if (strcmp(task ,"reply") == 0)
    {
        init_rreq_reply(atol(args));
    }
}
