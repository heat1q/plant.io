/**
 * @file routing.c
 * @author Patrick Willner (patrick.willner@tum.de), Andreas Koliopoulos (ga96leh@mytum.de), Alexander Schmaus (ga96fin@mytum.de)
 * @brief 
 * @version 0.1
 * @date 2019-12-12
 * 
 * @copyright Copyright (c) 2019
 * 
 */
#include "routing.h"

const struct broadcast_callbacks plantio_broadcast_call = {broadcast_receive};

static uint16_t num_routes;
static uint16_t *num_hops;
static uint8_t **route;

PROCESS(p_broadcast, "Broadcast Process");
PROCESS_THREAD(p_broadcast, ev, data)
{
    PROCESS_EXITHANDLER({broadcast_close(plantio_broadcast); free(plantio_broadcast); })
    PROCESS_BEGIN();

    plantio_broadcast = malloc(sizeof(struct broadcast_conn));

    // Defines the functions used as callbacks for a broadcast connection.
    broadcast_open(plantio_broadcast, 129, &plantio_broadcast_call);

    // Wait forever, since we need the broadcast always open
    while (1)
    {
        PROCESS_WAIT_EVENT();
    }

    PROCESS_END();
}

void broadcast_receive(struct broadcast_conn *broadcast, const linkaddr_t *from)
{
    leds_on(LEDS_GREEN);

    printf("Broadcast message received from 0x%x%x: [RSSI %d]\r\n", from->u8[0], from->u8[1], (int16_t)packetbuf_attr(PACKETBUF_ATTR_RSSI));

    // copy from buffer
    plantio_packet_t *packet_ptr = (plantio_packet_t *)packetbuf_dataptr();
    plantio_packet_t *packet = malloc(sizeof(plantio_packet_t) + packet_ptr->src_len + packet_ptr->dest_len + packet_ptr->data_len);
    packetbuf_copyto(packet);

    print_packet(packet);

    // network discovery packet which has to be forwarded
    if (packet->type == 0)
    {
        forward_discover(packet);
    }

    free(packet);

    leds_off(LEDS_GREEN);
}

void init_network(void)
{
    leds_on(LEDS_RED);

    // broadcast packet with type 0, node id as src
    create_packet(0, &linkaddr_node_addr.u8[1], 1, NULL, 0, NULL, 0);
    broadcast_send(plantio_broadcast);

    leds_off(LEDS_RED);
}

void forward_discover(const plantio_packet_t *packet)
{
    // check first if the routing tables need to be cleared in case of new network discovery packet
    if (num_routes) // only if table is non-empty
    {
        if (**route != *get_packet_src(packet)) // if src of current packet doesnt match src of table
        {
            // clear if network discovery is started from different mote
            clear_routing_table();
        }
    }

    // Find node id in src array
    uint16_t tmp = packet->src_len;
    const uint8_t *src = get_packet_src(packet);
    while (tmp)
    {
        if (*src++ == linkaddr_node_addr.u8[1])
        {
            break;
        }
        --tmp;
    }

    if (!tmp) // Proceed only if node id was not found in src
    {
        // append to the routing table
        ++num_routes;
        route = realloc(route, sizeof(uint8_t *) * num_routes);
        num_hops = realloc(num_hops, sizeof(uint16_t) * num_routes);

        num_hops[num_routes - 1] = packet->src_len; // length of the current route
        route[num_routes - 1] = malloc(sizeof(uint8_t) * packet->src_len);
        // copy the data from src to respective routing entry
        memcpy(route[num_routes - 1], get_packet_src(packet), sizeof(uint8_t) * packet->src_len);

        // append own id to src
        // new memory needs to be allocated since the packerbuf is
        // cleared before adding a new packet
        uint16_t src_len_new = packet->src_len + 1;
        uint8_t *src_new = malloc((packet->src_len + 1) * sizeof(uint8_t));
        memcpy(src_new, get_packet_src(packet), sizeof(uint8_t) * packet->src_len); // copy old arr to new
        src_new[packet->src_len] = linkaddr_node_addr.u8[1];                        // append node id

        create_packet(0, src_new, src_len_new, NULL, 0, NULL, 0);
        broadcast_send(plantio_broadcast);
        free(src_new);
    }
}

const uint16_t find_best_route(void)
{
    // for now, just return the shortest (hops) route
    uint16_t min = 1 << 15;
    uint16_t index = 0;
    for (uint16_t i = 0; i < num_routes; ++i)
    {
        if (num_hops[i] < min)
        {
            min = num_hops[i];
            index = i;
        }
    }
    
    return index;
}

void print_routing_table(void)
{
    uint16_t index = find_best_route();
    printf("opt |  i  | Routes for Device %u\r\n", linkaddr_node_addr.u8[1]);
    printf("----+-----+--------------------------\r\n");
    for (uint16_t i = 0; i < num_routes; ++i)
    {
        if (i == index)
        {
            printf(" x  | %3u | ", i);
        }
        else
        {
            printf("    | %3u | ", i);
        }
        for (uint8_t j = 0; j < num_hops[i]; ++j)
        {
            printf("%i ", route[i][j]);
        }
        printf("\r\n");
    }
}

void clear_routing_table(void)
{
    for (uint32_t i = 0; i < num_routes; ++i)
    {
        free(route[i]);
    }
    free(num_hops);
    free(route);
    // set to zero
    num_routes = 0;

    // set to NULL to be consistent with the static definition
    num_hops = NULL;
    route = NULL;
}
