/**
 * @file net_packet.c
 * @author Patrick Willner (patrick.willner@tum.de), Andreas Koliopoulos (ga96leh@mytum.de), Alexander Schmaus (ga96fin@mytum.de)
 * @brief 
 * @version 0.1
 * @date 2019-12-12
 * 
 * @copyright Copyright (c) 2019
 * 
 */
#include "net_packet.h"
#include "routing.h"
#include "mote_sensors.h"
#include "cfs/cfs.h"

int create_packet(const uint8_t type, const uint8_t *src, uint8_t src_len, const uint8_t *dest, uint8_t dest_len, const uint8_t *data, uint8_t data_len)
{
    plantio_malloc(mmem, plantio_packet_t, packet, sizeof(plantio_packet_t) + src_len + dest_len + data_len);

    packet->type = type;
    packet->src_len = src_len;
    packet->dest_len = dest_len;
    packet->data_len = data_len;

    uint8_t i = 0;
    while (src_len--) // copy the source addresses to the data
    {
        packet->data[i++] = *src++;
    }

    while (dest_len--) // dest addresses
    {
        packet->data[i++] = *dest++;
    }

    while (data_len--) // the rest is data
    {
        packet->data[i++] = *data++;
    }

    int bytes = packetbuf_copyfrom(packet, sizeof(plantio_packet_t) + packet->src_len + packet->dest_len + packet->data_len);
    plantio_free(mmem);
    //print_packet(packetbuf_hdrptr());
    return bytes;
}

void print_packet(const plantio_packet_t *packet)
{
    printf("Packet Type: %u [%u Bytes]\r\n", packet->type, sizeof(plantio_packet_t) + packet->src_len + packet->dest_len + packet->data_len);
    printf("Src [%u]: ", packet->src_len);
    for (int i = 0; i < packet->src_len; i++)
    {
        printf("%u ", packet->data[i]);
    }
    printf("\r\n");
    printf("Dest [%u]: ", packet->dest_len);
    for (int i = packet->src_len; i < (packet->src_len + packet->dest_len); i++)
    {
        printf("%u ", packet->data[i]);
    }
    printf("\r\n");
    printf("Data [%u]: ", packet->data_len);
    for (int i = packet->src_len + packet->dest_len; i < (packet->src_len + packet->dest_len + packet->data_len); i++)
    {
        printf("0x%x ", packet->data[i]);
    }
    printf("\r\n");
}

const uint8_t* get_packet_src(const plantio_packet_t *packet) { return packet->data; }
const uint8_t* get_packet_dest(const plantio_packet_t *packet) { return packet->data + packet->src_len; }
const uint8_t* get_packet_data(const plantio_packet_t *packet) { return packet->data + packet->src_len + packet->dest_len; }

void process_data_packet(const plantio_packet_t *packet)
{
    if (packet->type == 10)
    {
        leds_toggle(*get_packet_data(packet));
        send_ack(get_packet_src(packet), packet->src_len);
    }
    else if (packet->type == 11) // setting thresholds
    {
        write_thresholds((char*) get_packet_data(packet));
        send_ack(get_packet_src(packet), packet->src_len);
    }
    else if (packet->type == 12) // request thresholds
    {
        char str[128];
        sprintf(str, "%li:%li:%li:%li:%li:%li", get_threshold(0), get_threshold(1), get_threshold(2), get_threshold(3), get_threshold(4), get_threshold(5));

        init_data_packet(13, *get_packet_src(packet), (uint8_t*) str, strlen(str)+1, get_best_route_index());
    }
    else if (packet->type == 13) // reply for request thresholds
    {
        printf("<%u:th:%s>\r\n", get_packet_src(packet)[0], (char*) get_packet_data(packet));
    }
    else if (packet->type == 14) // request sensor data
    {
        // pack the data into byte array
        uint16_t data[MAX_NUM_OF_VALUES * 3];
        for (uint16_t i = 0; i < MAX_NUM_OF_VALUES; ++i)
        {
            for (uint8_t k = 0; k < 3; ++k) // id for temp, hum, light
            {
                data[i + k * MAX_NUM_OF_VALUES] = fetch_sensor_data(i * 4 + k + 1);
            }
        }

        init_data_packet(15, *get_packet_src(packet), (uint8_t *)data, sizeof(uint16_t) * 3 * MAX_NUM_OF_VALUES, get_best_route_index());
    }
    else if (packet->type == 15) // reply for request sensor data
    {
        uint16_t data[MAX_NUM_OF_VALUES * 3];
        memcpy(data, get_packet_data(packet), sizeof(uint16_t) * MAX_NUM_OF_VALUES * 3); // data is uint8_t array
        printf("<%u:sensor_data", *get_packet_src(packet));
        for (uint16_t i = 0; i < 3 * MAX_NUM_OF_VALUES; ++i)
        {
            printf(":%u", data[i]);
        }
        printf(">\r\n");
    }
    else if (packet->type == 16) // request for rt
    {
        uint16_t num_routes = get_num_routes();
        if (num_routes)
        {
            uint16_t all_routes_len = 0;
            for (uint16_t i = 0; i < num_routes; i++)
            {
                all_routes_len += get_num_hops(i);
            }
            uint16_t filelen = all_routes_len + 2*sizeof(uint16_t) + num_routes;
            plantio_malloc(mmem_file_data, uint8_t, data, filelen);

            int f_routing = cfs_open(FILE_ROUTING, CFS_READ);
            if (f_routing != -1)
            {
                cfs_seek(f_routing, 0, CFS_SEEK_SET);
                cfs_read(f_routing, data, filelen - sizeof(uint16_t));
                cfs_close(f_routing);
            }

            // append best route index
            data[filelen - 2] = (uint8_t)(get_best_route_index() >> 8);
            data[filelen - 1] = (uint8_t) get_best_route_index();

            init_data_packet(17, *get_packet_src(packet), data, filelen, get_best_route_index());

            plantio_free(mmem_file_data);
        }
        else
        {
            init_data_packet(17, *get_packet_src(packet), NULL, 0, get_best_route_index());
        }
    }
    else if (packet->type == 17) // reply for rt
    {
        printf("opt |  i  | Hops | Routes for Device %u\r\n", *get_packet_src(packet));
        printf("----+-----+------+------------------------\r\n");
        if (packet->data_len)
        {
            uint16_t index = (((uint16_t)get_packet_data(packet)[packet->data_len-2]) << 8) | get_packet_data(packet)[packet->data_len-1];
            uint16_t num_routes = (((uint16_t)get_packet_data(packet)[1]) << 8) | get_packet_data(packet)[0]; // in file right is msb
            for (uint16_t i = 0; i < num_routes; ++i)
            {
                uint16_t num_hops = get_packet_data(packet)[2 + i];

                if (i == index)
                {
                    printf(" x  | %3u |  %3u | ", i, num_hops);
                }
                else
                {
                    printf("    | %3u |  %3u | ", i, num_hops);
                }

                uint16_t route_index = 2 + num_routes;
                for (uint16_t j = 0; j < i; j++)
                {
                    route_index += get_packet_data(packet)[2 + j];
                }
                
                const uint8_t *route = get_packet_data(packet) + route_index;
                for (uint16_t j = 0; j < num_hops; ++j)
                {
                    printf("%i ", route[j]);
                }
                printf("\r\n");
            }
        }
    }
}