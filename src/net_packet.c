#include "net_packet.h"
#include "routing.h"

int create_packet(uint8_t type, uint8_t *src, uint8_t src_len, uint8_t *dest, uint8_t dest_len, uint8_t *data, uint16_t data_len)
{
    plantio_packet_t *packet = malloc(sizeof(plantio_packet_t) + src_len + dest_len + 0);

    packet->type = type;
    packet->src_len = src_len;
    packet->dest_len = dest_len;
    packet->data_len = data_len;

    uint16_t i = 0;
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
    free(packet);
    //print_packet(packetbuf_hdrptr());
    return bytes;
}

void print_packet(plantio_packet_t *packet)
{
    printf("Packet Type: %u\r\n", packet->type);
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