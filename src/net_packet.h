#pragma once

#include "plantio.h"

/**
 * @brief General packet declaration. The packets are defined by a
 * specific type, e.g. RREQ packet, ACK, etc... The source of the 
 * packet is given by an array of Node ID. The destination, i.e.
 * the path, is defined by an array. A data pointer is given to 
 * reference the payload.
 * 
 */
struct
{
    uint8_t type;
    uint16_t src_len;
    uint16_t dest_len;
    uint16_t data_len;
    uint8_t data[];
} typedef plantio_packet_t;

/**
 * @brief Create a packet object and dynamically allocates memory
 * given by the size of the data.
 * 
 * @param type Type of packet
 * @param src Array of source addresses, i.e. node ids
 * @param src_len Length of array
 * @param dest Array of destination addresses, i.e. node ids
 * @param dest_len Length of array
 * @param data Array of payload as char array
 * @param data_size Length of dat
 * @return int Number of Bytes copied into the buffer
 */
int create_packet(uint8_t type, uint8_t *src, uint8_t src_len, uint8_t *dest, uint8_t dest_len, uint8_t *data, uint16_t data_len);

/**
 * @brief Prints packet information.
 * 
 * @param packet Instance of packet struct
 */
void print_packet(plantio_packet_t *packet);