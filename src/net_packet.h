/**
 * @file net_packet.h
 * @author Patrick Willner (patrick.willner@tum.de), Andreas Koliopoulos (ga96leh@mytum.de), Alexander Schmaus (ga96fin@mytum.de)
 * @brief 
 * @version 0.1
 * @date 2019-12-12
 * 
 * @copyright Copyright (c) 2019
 * 
 */
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
struct __attribute__((__packed__)) //disable padding
{
    uint8_t type;
    uint8_t src_len;
    uint8_t dest_len;
    uint8_t data_len;
    uint8_t data[];
} typedef plantio_packet_t;

/**
 * @brief Create a packet object and dynamically allocates memory
 * given by the size of the data.
 * 
 * @details The following packet types are defined:
 * 0 ==> [Broadcast] Network discovery packet
 * 
 * 1 ==> [Unicast] Route Request (RREQ) Reply packet
 * 
 * 2 ==> [Broadcast] Request for the best route of the neighbouring nodes
 * 
 * 3 ==> [Unicast] Reply with the best route to new integrated node 
 * 
 * >=10 ==> [Unicast] General Data 
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
int create_packet(const uint8_t type, const uint8_t *src, uint8_t src_len, const uint8_t *dest, uint8_t dest_len, const uint8_t *data, uint8_t data_len);

/**
 * @brief Prints packet information.
 * 
 * @param packet Instance of packet struct
 */
void print_packet(const plantio_packet_t *packet);

const uint8_t* get_packet_src(const plantio_packet_t *packet);
const uint8_t* get_packet_dest(const plantio_packet_t *packet);
const uint8_t* get_packet_data(const plantio_packet_t *packet);

/**
 * @brief Processes packets with id >= 10
 * 
 * @param packet Instance of packet struct
 */
void process_data_packet(const plantio_packet_t *packet);
