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
#include "sys/etimer.h"
#include "random.h"
#include "clock.h"

static const struct broadcast_callbacks plantio_broadcast_call = {broadcast_receive};
static const struct unicast_callbacks plantio_unicast_call = {unicast_receive};

static struct etimer et_init_reply;
static uint16_t flood_time;
static int16_t best_route_index = -1;

PROCESS(p_conn, "");
PROCESS_THREAD(p_conn, ev, data)
{
    PROCESS_EXITHANDLER({ broadcast_close(&plantio_broadcast); unicast_close(&plantio_unicast); })
    PROCESS_BEGIN();
    find_best_route();

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

PROCESS(p_init_reply_timer, "");
PROCESS_THREAD(p_init_reply_timer, ev, data)
{
    PROCESS_BEGIN();
    etimer_set(&et_init_reply, PLANTIO_RREP_TIMEOUT * CLOCK_SECOND);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et_init_reply));
    find_best_route();
    init_rreq_reply(best_route_index);

    PROCESS_END();
}

void broadcast_receive(struct broadcast_conn *broadcast, const linkaddr_t *from)
{
    int16_t rssi = (int16_t)packetbuf_attr(PACKETBUF_ATTR_RSSI);

    if (rssi > PLANTIO_MIN_RSSI)
    {
        leds_toggle(LEDS_GREEN);

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
        else if (packet->type == 2)
        {
            // a new node is integrated into the network
            // send a unicast to the node with the best route
            send_best_route(*get_packet_src(packet));
        }

        plantio_free(mmem);

        leds_toggle(LEDS_GREEN);
    }
}

void unicast_receive(struct unicast_conn *unicast, const linkaddr_t *from)
{
    leds_toggle(LEDS_BLUE);

    // copy from buffer
    plantio_packet_t *packet_ptr = (plantio_packet_t *)packetbuf_dataptr();
    plantio_malloc(mmem, plantio_packet_t, packet, sizeof(plantio_packet_t) + packet_ptr->src_len + packet_ptr->dest_len + packet_ptr->data_len);
    packetbuf_copyto(packet);

#ifdef PLANTIO_DEBUG
    printf("Unicast message received from 0x%x%x: [RSSI %d]\n", from->u8[0], from->u8[1], (int16_t)packetbuf_attr(PACKETBUF_ATTR_RSSI));
    print_packet(packet);
#endif

    leds_toggle(LEDS_BLUE);

    // check if packet is for current node
    if (get_packet_dest(packet)[packet->dest_len - 1] == linkaddr_node_addr.u8[1])
    {
        if (packet->dest_len == 1) // check if packet reached destination
        {
            if (packet->type == 1) // for gui node
            {
                printf("<"); // begin data message
                printf("%u:route", get_packet_src(packet)[0]);
                for (uint8_t i = 0; i < packet->src_len; i++)
                {
                    printf(":%u", get_packet_src(packet)[i]);
                }
                printf(">\r\n"); // end data message

                // write the table for the gui mote
                write_routing_table(get_packet_src(packet), packet->src_len);
            }
            else if (packet->type == 3) // reply with best route
            {
                receive_route(packet);
            }
            else if (packet->type == 4) // ACK
            {
                printf("<%u:ack>\r\n", get_packet_src(packet)[0]);
            }
            else if (packet->type >= 10) // data packet
            {
                process_data_packet(packet);

                if (best_route_index >= 0) // if not gui node
                {
                    // Send an ACK 
                    create_packet(4, &linkaddr_node_addr.u8[1], 1, get_packet_src(packet), packet->src_len, NULL, 0);
                    linkaddr_t next_hop;
                    next_hop.u8[0] = 0;
                    next_hop.u8[1] = get_packet_src(packet)[packet->src_len - 1];
                    unicast_send(&plantio_unicast, &next_hop);
                }
            }
            else if (packet->type >= 10) // data packet
            {
                process_data_packet(packet);
            }
        }
        else
        {
            forward_routing(packet);
        }
    }
    else
    {
        printf("Received packet with incorrect destination. Destination was %u, but Node ID is%u\r\n", get_packet_dest(packet)[packet->dest_len - 1], linkaddr_node_addr.u8[1]);
    }

    plantio_free(mmem);
}

void init_network(void)
{
    // clear own routing table
    clear_routing_table();

    // add timestamps to flooding packets
    flood_time = (uint16_t)clock_time(); // the 16 lsb should be enough
    // convert to byte array
    uint8_t timestamp[2];
    timestamp[0] = (uint8_t)(flood_time >> 8);
    timestamp[1] = (uint8_t)flood_time;

    // broadcast packet with type 0, node id as src
    create_packet(0, &linkaddr_node_addr.u8[1], 1, NULL, 0, timestamp, 2);
    broadcast_send(&plantio_broadcast);

#ifdef PLANTIO_DEBUG
    printf("Initialized Network with timestamp %u\r\n", flood_time);
#endif
}

void forward_discover(const plantio_packet_t *packet)
{
    uint16_t timestamp = (((uint16_t)get_packet_data(packet)[0]) << 8) | get_packet_data(packet)[1];

    // check first if the routing tables need to be cleared in case of new network discovery packet
    if (get_num_routes()) // only if table is non-empty
    {
        //uint8_t ref;
        //get_route(&ref, 1, 0); // just get the first id in the first route for reference
        // if src of current packet doesnt match src of table OR new timestamp
        if (/*ref != *get_packet_src(packet) || */ flood_time != timestamp)
        {
            // clear if network discovery is started from different mote
            clear_routing_table();
        }
    }

    flood_time = timestamp; // update timestamp

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
        if (get_num_routes() == 0)
        {
            process_start(&p_init_reply_timer, NULL);
        }
        else if (!etimer_expired(&et_init_reply))
        {
            PROCESS_CONTEXT_BEGIN(&p_init_reply_timer);
            etimer_set(&et_init_reply, PLANTIO_RREP_TIMEOUT * CLOCK_SECOND);
            PROCESS_CONTEXT_END(&p_init_reply_timer);
        }

        // append to the routing table & write table
        write_routing_table(get_packet_src(packet), packet->src_len);

        // append own id to src
        // new memory needs to be allocated since the packerbuf is
        // cleared before adding a new packet
        uint8_t src_len_new = packet->src_len + 1;
        plantio_malloc(mmem, uint8_t, src_new, sizeof(uint8_t) * (packet->src_len + 1));
        memcpy(src_new, get_packet_src(packet), sizeof(uint8_t) * packet->src_len); // copy old arr to new
        src_new[packet->src_len] = linkaddr_node_addr.u8[1];                        // append node id

        // transmit the timestamp further
        uint8_t time[2];
        time[0] = (uint8_t)(flood_time >> 8);
        time[1] = (uint8_t)flood_time;

        create_packet(0, src_new, src_len_new, NULL, 0, time, 2);
        broadcast_send(&plantio_broadcast);
        plantio_free(mmem);
    }
}

void find_best_route(void)
{
    uint16_t num_routes = get_num_routes();
    if (num_routes)
    {
        // for now, just return the shortest (hops) route
        uint16_t min = 1 << 15;
        for (uint16_t i = 0; i < get_num_routes(); ++i)
        {
            uint16_t num_hops = get_num_hops(i);
            if (num_hops < min)
            {
                min = num_hops;
                best_route_index = (int16_t) i;
            }
        }
    }
}

const int16_t get_best_route_index()
{
    return best_route_index;
}

void print_routing_table(void)
{
    uint16_t index = (uint16_t) best_route_index;
    printf("opt |  i  | Hops | Routes for Device %u\r\n", linkaddr_node_addr.u8[1]);
    printf("----+-----+------+------------------------\r\n");
    for (uint16_t i = 0; i < get_num_routes(); ++i)
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
    cfs_remove(FILE_ROUTING);
}

void write_routing_table(const uint8_t *route, const uint8_t length)
{
    //set_num_routes(get_num_routes() + 1);
    uint16_t num_routes = get_num_routes();
    // get current file length
    if (num_routes)
    {
        uint16_t all_routes_len = 0;
        for (uint16_t i = 0; i < num_routes; i++)
        {
            all_routes_len += get_num_hops(i);
        }

        // temporarly load data to memory
        plantio_malloc(mmem_file_data, uint8_t, data, all_routes_len + sizeof(uint16_t) + num_routes);

        int f_routing = cfs_open(FILE_ROUTING, CFS_READ);
        if (f_routing != -1)
        {
            cfs_seek(f_routing, 0, CFS_SEEK_SET);
            cfs_read(f_routing, data, all_routes_len + sizeof(uint16_t) + num_routes);
            cfs_close(f_routing);
        }
        cfs_remove(FILE_ROUTING);

        ++num_routes;

        f_routing = cfs_open(FILE_ROUTING, CFS_WRITE + CFS_APPEND);    
        if (f_routing != -1)
        {
            int n = cfs_write(f_routing, &num_routes, sizeof(uint16_t));
            n += cfs_write(f_routing, data+sizeof(uint16_t), num_routes - 1); // old num_hops array
            n += cfs_write(f_routing, &length, sizeof(uint8_t)); // new num_hops
            n += cfs_write(f_routing, data+sizeof(uint16_t)+num_routes-1, all_routes_len); // old routes
            n += cfs_write(f_routing, route, length); // new route

            cfs_close(f_routing);

            #ifdef PLANTIO_DEBUG
            printf("f_routing: successfully written to routing table. wrote %i bytes\r\n", n);
            #endif
        }
        else
        {
            #ifdef PLANTIO_DEBUG
            printf("f_routing: ERROR: could not write routing table.\r\n");
            #endif
        }

        plantio_free(mmem_file_data);
    }
    else // table was emtpy
    {
        ++num_routes;

        int f_routing = cfs_open(FILE_ROUTING, CFS_WRITE + CFS_APPEND);
        if (f_routing != -1)
        {
            int n = cfs_write(f_routing, &num_routes, sizeof(uint16_t));
            n += cfs_write(f_routing, &length, sizeof(uint8_t)); // new num_hops
            n += cfs_write(f_routing, route, length); // new route

            cfs_close(f_routing);
            #ifdef PLANTIO_DEBUG
            printf("f_routing: successfully written to routing table. wrote %i bytes\r\n", n);
            #endif
        }
        else
        {
            #ifdef PLANTIO_DEBUG
            printf("f_routing: ERROR: could not write routing table.\r\n");
            #endif
        }
    }
}

const uint16_t get_num_routes()
{
    uint16_t num_routes = 0;
    int f_routing = cfs_open(FILE_ROUTING, CFS_READ);
    if (f_routing != -1)
    {
        cfs_seek(f_routing, 0, CFS_SEEK_SET); // num routes is at index 0-1
        cfs_read(f_routing, &num_routes, sizeof(uint16_t));
        cfs_close(f_routing);
    }
    // if file cannot be read, return 0
    return num_routes;
}

const uint8_t get_num_hops(const uint16_t index)
{
    uint16_t num_routes = get_num_routes();
    uint8_t num_hops = 0;
    if (num_routes)
    {
        int f_hops = cfs_open(FILE_ROUTING, CFS_READ);
        if (f_hops != -1)
        {
            cfs_seek(f_hops, sizeof(uint16_t) + index, CFS_SEEK_SET); // num_hops array starts at 2
            cfs_read(f_hops, &num_hops, sizeof(uint8_t));
            cfs_close(f_hops);
        }
    }
    return num_hops;
}

void get_route(uint8_t *route, const uint16_t num_hops, const uint16_t index)
{
    uint16_t num_routes = get_num_routes();
    if (num_routes)
    {
        //uint8_t num_hops = get_num_hops(index);
        uint16_t route_index = 0;
        // find the right index in the route array
        for (uint16_t i = 0; i < index; i++)
        {
            route_index += get_num_hops(i);
        }

        route_index += sizeof(uint16_t) + num_routes; // route is at 2 + num_routes + sum(num_hops of previous routes)

        int f_route = cfs_open(FILE_ROUTING, CFS_READ);
        if (f_route != -1)
        {
            cfs_seek(f_route, route_index, CFS_SEEK_SET); // jump to right position
            // the next 2 bytes are the right value
            cfs_read(f_route, route, sizeof(uint8_t) * num_hops);
            cfs_close(f_route);
        }
    }
}

void init_rreq_reply(const uint16_t index)
{
    if (get_num_routes()) // only if table is non-empty
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

#ifdef PLANTIO_DEBUG
        printf("Initialized Route Request Reply\r\n");
#endif
    }
}

void forward_routing(const plantio_packet_t *packet)
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

void init_data_packet(const uint8_t type, const uint8_t dest, const uint8_t *data, const uint8_t data_len, int index)
{
    if (index < 0) // index in rt is not specified
    {
        // find the route to destination in table
        uint16_t min = 1 << 15;
        for (uint16_t i = 0; i < get_num_routes(); ++i)
        {
            // find the route with the correct dest & min num_hops
            uint8_t dest_ref;
            get_route(&dest_ref, 1, i);
            if (dest_ref == dest) 
            { 
                if (get_num_hops(i) < min)
                {
                    index = i;
                    min = get_num_hops(i);
                }
            }
            //if (index >= 0) { break; }
        }
    }

    if (index >= 0)
    {
        const uint8_t num_hops = get_num_hops(index);

        plantio_malloc(mmem_route, uint8_t, route, sizeof(uint8_t) * num_hops);
        get_route(route, num_hops, index);

        create_packet(type, &linkaddr_node_addr.u8[1], 1, route, num_hops, data, data_len);
        linkaddr_t next_hop;
        next_hop.u8[0] = 0;
        next_hop.u8[1] = route[num_hops - 1];
        unicast_send(&plantio_unicast, &next_hop);

        plantio_free(mmem_route);
    }
}

void add_device_to_network(void)
{
    clear_routing_table();

    // broadcast packet with type 2, node id as src
    create_packet(2, &linkaddr_node_addr.u8[1], 1, NULL, 0, NULL, 0);
    broadcast_send(&plantio_broadcast);
}

void send_best_route(const uint8_t dest)
{
    if (best_route_index >= 0)
    {
        uint8_t num_hops = get_num_hops(best_route_index);

        plantio_malloc(mmem_route, uint8_t, route, sizeof(uint8_t) * (num_hops + 1));
        get_route(route, num_hops, best_route_index);

        for (uint8_t i = 0; i < num_hops; i++)
        {
            // if dest id is in the selected route, cut the route
            // to prevent loops
            if (route[i] == dest)
            {
                num_hops = i;
            }
        }

        route[num_hops] = linkaddr_node_addr.u8[1]; // append own id to src

        create_packet(3, route, num_hops + 1, &dest, 1, NULL, 0);
        linkaddr_t next_hop;
        next_hop.u8[0] = 0;
        next_hop.u8[1] = dest;
        unicast_send(&plantio_unicast, &next_hop);

        plantio_free(mmem_route);
    }
    else // the table is either empty or the current node is the GUI node
    {
        create_packet(3, &linkaddr_node_addr.u8[1], 1, &dest, 1, NULL, 0);
        linkaddr_t next_hop;
        next_hop.u8[0] = 0;
        next_hop.u8[1] = dest;
        unicast_send(&plantio_unicast, &next_hop);
    }
}

void receive_route(const plantio_packet_t *packet)
{
    if (get_num_routes() == 0)
    {
        process_start(&p_init_reply_timer, NULL);
    }
    else if (!etimer_expired(&et_init_reply))
    {
        PROCESS_CONTEXT_BEGIN(&p_init_reply_timer);
        etimer_set(&et_init_reply, PLANTIO_RREP_TIMEOUT/2 * CLOCK_SECOND);
        PROCESS_CONTEXT_END(&p_init_reply_timer);
    }

    // save the route to the routing table
    write_routing_table(get_packet_src(packet), packet->src_len);
}