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
#include "mote_sensors.h"

PROCESS(p_serial, "Serial Event listener");
PROCESS_THREAD(p_serial, ev, data)
{
    PROCESS_BEGIN();

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
    uint8_t id = (uint8_t) atoi(tmp);

    uint8_t len = strlen(tmp) + 1; // the length of the id + delim

    tmp = strtok(NULL, ":");
    // check task
    char *task = tmp;
    len += strlen(task) + 1;

    //args
    char *args = input + len;

    // continue with correct format
    if (strcmp(task, "led") == 0)
    {
        //RED: 1, GREEN: 2, BLUE: 4
        uint8_t led_id = (uint8_t) atoi(args);

        if (id == linkaddr_node_addr.u8[1] || id == 0)
        {
            leds_toggle(led_id);
        }
        else // send to the right node
        {
            init_data_packet(10, id, &led_id, 1, -1);
        }
    }
    else if (strcmp(task, "init") == 0)
    {
        // start network discovery
        init_network();
    }
    else if (strcmp(task ,"rt") == 0)
    {
        if (id == linkaddr_node_addr.u8[1] || id == 0)
        {
            print_routing_table();
        }
        else // send to the right node
        {
            init_data_packet(16, id, NULL, 0, -1);
        }
    }
    else if (strcmp(task ,"set_th") == 0)
    {
        if (id == linkaddr_node_addr.u8[1] || id == 0)
        {
            write_thresholds(args);
        }
        else // send to the right node
        {
            init_data_packet(11, id, (uint8_t*) args, strlen(args)+1, -1);
        }
    }
    else if (strcmp(task ,"get_th") == 0)
    {
        if (id == linkaddr_node_addr.u8[1] || id == 0)
        {
            printf("<%u:th:%li:%li:%li:%li:%li:%li>\r\n", linkaddr_node_addr.u8[1], get_threshold(0), get_threshold(1), get_threshold(2), get_threshold(3), get_threshold(4), get_threshold(5));
        }
        else 
        {
            init_data_packet(12, id, NULL, 0, -1);
        }
    }
    else if (strcmp(task ,"get_data") == 0)
    {
        if (id == linkaddr_node_addr.u8[1] || id == 0)
        {
            printf("<%u:sensor_data", linkaddr_node_addr.u8[1]);
            for (uint8_t k = 0; k < 3; ++k) // id for temp, hum, light
            {
                for (uint16_t i = 0; i < MAX_NUM_OF_VALUES; ++i)
                {
                    printf(":%u", fetch_sensor_data(i * 4 + k + 1));
                }
            }
            printf(">\r\n");
        }
        else // send to the right node
        {
            init_data_packet(14, id, NULL, 0, -1);
        }
    }
}
