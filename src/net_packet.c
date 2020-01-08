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
    printf("%s\r\n", packet->data + packet->src_len + packet->dest_len);
}

const uint8_t* get_packet_src(const plantio_packet_t *packet) { return packet->data; }
const uint8_t* get_packet_dest(const plantio_packet_t *packet) { return packet->data + packet->src_len; }
const uint8_t* get_packet_data(const plantio_packet_t *packet) { return packet->data + packet->src_len + packet->dest_len; }