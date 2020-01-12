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
    }
    else if (packet->type == 11) // setting thresholds
    {
        write_thresholds((char*) get_packet_data(packet));
    }
    else if (packet->type == 12) // request thresholds
    {
        char str[128];
        sprintf(str, "%li:%li:%li:%li:%li:%li", get_threshold(0), get_threshold(1), get_threshold(2), get_threshold(3), get_threshold(4), get_threshold(5));

        init_data_packet(13, *get_packet_src(packet), (uint8_t*) str, strlen(str)+1, get_best_route_index());
    }
    else if (packet->type == 13) // reply for request thresholds
    {
        printf("<%u:th:%s>", get_packet_src(packet)[0], (char*) get_packet_data(packet));
    }
}