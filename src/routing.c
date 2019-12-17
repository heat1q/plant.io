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
#include "cfs/cfs.h"

static const struct broadcast_callbacks plantio_broadcast_call = {broadcast_receive};
static const struct unicast_callbacks plantio_unicast_call = {unicast_receive};

static uint8_t network_discover_ctr; // synchronized network discovery id for all motes to check for repeated network discovery
static uint16_t num_routes;
static const char *filename_hops = "routing_hops";
static const char *filename_routing = "routing";

PROCESS(p_conn, "");
PROCESS_THREAD(p_conn, ev, data)
{
    PROCESS_EXITHANDLER({ broadcast_close(&plantio_broadcast); unicast_close(&plantio_unicast); })
    PROCESS_BEGIN();

    // Defines the functions used as callbacks for a broadcast connection.
    broadcast_open(&plantio_broadcast, 129, &plantio_broadcast_call);
    unicast_open(&plantio_unicast, 128, &plantio_unicast_call);

    // Wait forever, since we need the broadcast always open
    while (1)
    {
        PROCESS_WAIT_EVENT();
    }

    PROCESS_END();
}

PROCESS(p_unicast, "");
PROCESS_THREAD(p_unicast, ev, data)
{
    PROCESS_BEGIN();

    PROCESS_END();
}

void broadcast_receive(struct broadcast_conn *broadcast, const linkaddr_t *from)
{
    leds_on(LEDS_GREEN);
    int16_t rssi = (int16_t)packetbuf_attr(PACKETBUF_ATTR_RSSI);

    if (rssi > PLANTIO_MIN_RSSI)
    {
        // copy from buffer
        plantio_packet_t *packet_ptr = (plantio_packet_t *)packetbuf_dataptr();
        plantio_malloc(mmem, plantio_packet_t, packet, sizeof(plantio_packet_t) + packet_ptr->src_len + packet_ptr->dest_len + packet_ptr->data_len);
        packetbuf_copyto(packet);

#ifdef PLANTIO_DEBUG
        printf("Broadcast message received from 0x%x%x: [RSSI %d]\r\n", from->u8[0], from->u8[1], rssi);
        print_packet(packet);
#endif

        // network discovery packet which has to be forwarded
        if (packet->type == 0)
        {
            forward_discover(packet);
        }

        plantio_free(mmem);

        leds_off(LEDS_GREEN);
    }
}

void unicast_receive(struct unicast_conn *unicast, const linkaddr_t *from)
{
    leds_on(LEDS_BLUE);

    // copy from buffer
    plantio_packet_t *packet_ptr = (plantio_packet_t *)packetbuf_dataptr();
    plantio_malloc(mmem, plantio_packet_t, packet, sizeof(plantio_packet_t) + packet_ptr->src_len + packet_ptr->dest_len + packet_ptr->data_len);
    packetbuf_copyto(packet);

#ifdef PLANTIO_DEBUG
    printf("Unicast message received from 0x%x%x: [RSSI %d]\n", from->u8[0], from->u8[1], (int16_t)packetbuf_attr(PACKETBUF_ATTR_RSSI));
    print_packet(packet);
#endif

    // check if packet is for current node
    if (get_packet_dest(packet)[packet->dest_len - 1] == linkaddr_node_addr.u8[1])
    {
        forward_routing(packet);
    }
    else
    {
        printf("Received packet with incorrect destination. Destination was %u, but Node ID is%u\r\n", get_packet_dest(packet)[packet->dest_len - 1], linkaddr_node_addr.u8[1]);
    }

    plantio_free(mmem);

    leds_off(LEDS_BLUE);
}

void init_network(void)
{
    leds_on(LEDS_RED);

    // increase the counter to check if network discovery has been restarted
    network_discover_ctr++; // if this overflows, we start at 0

    // broadcast packet with type 0, node id as src
    create_packet(0, &linkaddr_node_addr.u8[1], 1, NULL, 0, &network_discover_ctr, 1);
    broadcast_send(&plantio_broadcast);

    leds_off(LEDS_RED);
}

void forward_discover(const plantio_packet_t *packet)
{
    // check first if the routing tables need to be cleared in case of new network discovery packet
    if (num_routes) // only if table is non-empty
    {
        uint8_t ref;
        get_route(&ref, 1, 0); // just get the first id in the first route for reference
        // if src of current packet doesnt match src of table OR ctr doesnt match the current packet
        if (ref != *get_packet_src(packet) || network_discover_ctr != *get_packet_data(packet))
        {
            // clear if network discovery is started from different mote
            clear_routing_table();
        }
    }

    network_discover_ctr = *get_packet_data(packet); // set the id

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
        // write table
        write_routing_table(get_packet_src(packet), packet->src_len);

        // append own id to src
        // new memory needs to be allocated since the packerbuf is
        // cleared before adding a new packet
        uint8_t src_len_new = packet->src_len + 1;
        plantio_malloc(mmem, uint8_t, src_new, sizeof(uint8_t) * (packet->src_len + 1));
        memcpy(src_new, get_packet_src(packet), sizeof(uint8_t) * packet->src_len); // copy old arr to new
        src_new[packet->src_len] = linkaddr_node_addr.u8[1];                        // append node id

        create_packet(0, src_new, src_len_new, NULL, 0, &network_discover_ctr, 1);
        broadcast_send(&plantio_broadcast);
        plantio_free(mmem);
    }
}

const uint16_t find_best_route(void)
{
    // for now, just return the shortest (hops) route
    uint16_t min = 1 << 15;
    uint16_t index = 0;
    for (uint16_t i = 0; i < num_routes; ++i)
    {
        uint16_t num_hops = get_num_hops(i);
        if (num_hops < min)
        {
            min = num_hops;
            index = i;
        }
    }

    return index;
}

void print_routing_table(void)
{
    uint16_t index = find_best_route();
    printf("opt |  i  | Hops | Routes for Device %u\r\n", linkaddr_node_addr.u8[1]);
    printf("----+-----+------+------------------------\r\n");
    for (uint16_t i = 0; i < num_routes; ++i)
    {
        uint16_t num_hops = get_num_hops(i);

        if (i == index)
        {
            printf(" x  | %3u |  %3u | ", i, num_hops);
        }
        else
        {
            printf("    | %3u |  %3u | ", i, num_hops);
        }

        plantio_malloc(mmem_route, uint8_t, route, sizeof(uint8_t) * num_hops);
        get_route(route, num_hops, i);

        for (uint16_t j = 0; j < num_hops; ++j)
        {
            printf("%i ", route[j]);
        }
        printf("\r\n");

        plantio_free(mmem_route);
    }
}

void clear_routing_table(void)
{
    cfs_remove(filename_hops);
    cfs_remove(filename_routing);
    num_routes = 0;
}

void write_routing_table(const uint8_t *route, const uint16_t length)
{
    int f_hops = cfs_open(filename_hops, CFS_WRITE + CFS_APPEND);
    if (f_hops != -1)
    {
        int n = cfs_write(f_hops, &length, sizeof(uint16_t));
        cfs_close(f_hops);
#ifdef PLANTIO_DEBUG
        printf("f_hops: successfully written to routing table. wrote %i bytes\r\n", n);
#endif
    }
    else
    {
#ifdef PLANTIO_DEBUG
        printf("f_hops: ERROR: could not write routing table.\r\n");
#endif
    }

    int f_routes = cfs_open(filename_routing, CFS_WRITE + CFS_APPEND);
    if (f_routes != -1)
    {
        int n = cfs_write(f_routes, route, sizeof(uint8_t) * length);
        cfs_close(f_routes);
#ifdef PLANTIO_DEBUG
        printf("f_routes: successfully written to routing table. wrote %i bytes\r\n", n);
#endif
    }
    else
    {
#ifdef PLANTIO_DEBUG
        printf("f_routes: ERROR: could not write routing table.\r\n");
#endif
    }
}

const uint16_t get_num_hops(const uint16_t index)
{
    uint16_t num_hops = 0;
    int f_hops = cfs_open(filename_hops, CFS_READ);
    if (f_hops != -1)
    {
        cfs_seek(f_hops, sizeof(uint16_t) * index, CFS_SEEK_SET); // jump to right position
        // the next 2 bytes are the right value
        cfs_read(f_hops, &num_hops, sizeof(uint16_t));
        cfs_close(f_hops);
    }
    return num_hops;
}

void get_route(uint8_t *route, const uint16_t num_hops, const uint16_t index)
{
    //plantio_malloc(mmem_hops, uint16_t, hops, num_routes);

    uint16_t route_index = 0;
    // find the right index in the route array
    for (uint16_t i = 0; i < index; i++)
    {
        route_index += get_num_hops(i);
    }

    int f_route = cfs_open(filename_routing, CFS_READ);
    if (f_route != -1)
    {
        cfs_seek(f_route, route_index, CFS_SEEK_SET); // jump to right position
        // the next 2 bytes are the right value
        cfs_read(f_route, route, sizeof(uint8_t) * num_hops);
        cfs_close(f_route);
    }
}

void init_rreq_reply(const uint16_t index)
{
    if (num_routes) // only if table is non-empty
    {
        const uint16_t num_hops = get_num_hops(index);

        // get the route
        plantio_malloc(mmem_route, uint8_t, route, num_hops);
        get_route(route, num_hops, index);

        create_packet(1, &linkaddr_node_addr.u8[1], 1, route, num_hops, NULL, 0);
        linkaddr_t next_hop;
        next_hop.u8[0] = 0;
        next_hop.u8[1] = route[num_hops - 1];
        unicast_send(&plantio_unicast, &next_hop);

        plantio_free(mmem_route);
    }
}

void forward_routing(const plantio_packet_t *packet)
{
    if (packet->dest_len > 1) // check if destination is already reached
    {
        // append own id to src
        plantio_malloc(mmem_src, uint8_t, src_new, packet->src_len + 1);
        memcpy(src_new, get_packet_src(packet), packet->src_len);
        src_new[packet->src_len] = linkaddr_node_addr.u8[1];

        // remove last id of dest, i.e. copy dest_len-1 bytes
        create_packet(packet->type, src_new, packet->src_len + 1, get_packet_dest(packet), packet->dest_len - 1, get_packet_data(packet), packet->data_len);

        linkaddr_t next_hop;
        next_hop.u8[0] = 0;
        next_hop.u8[1] = get_packet_dest(packet)[packet->dest_len - 2]; // get the next hop in the route
        unicast_send(&plantio_unicast, &next_hop);

        plantio_free(mmem_src);
    }
}
