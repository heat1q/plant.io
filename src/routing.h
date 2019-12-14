/**
 * @file routing.h
 * @author Patrick Willner (patrick.willner@tum.de), Andreas Koliopoulos (ga96leh@mytum.de), Alexander Schmaus (ga96fin@mytum.de)
 * @brief 
 * @version 0.1
 * @date 2019-12-12
 * 
 * @copyright Copyright (c) 2019
 * 
 */
#pragma once

#include <net/rime/rime.h> // Establish connections.
#include <net/netstack.h>  // Wireless-stack definitions
#include <core/net/linkaddr.h>
#include <core/net/rime/broadcast.h>
#include <net/packetbuf.h>

#include "plantio.h"
#include "net_packet.h"

struct
{
    uint16_t num_routes;
    uint16_t num_hops[];
} typedef routing_hops_t;

struct
{
    uint16_t num_routes;
    uint16_t route[];
} typedef routing_t;


// Forward declaration of processes
struct process p_broadcast; // Broadcast process for Flooding & Network Discovery

struct broadcast_conn plantio_broadcast; // Creates an instance of a broadcast connection.

/**
 * @brief Callback function for Broadcast.
 * 
 * @param broadcast 
 * @param from 
 */
void broadcast_receive(struct broadcast_conn *broadcast, const linkaddr_t *from);

/**
 * @brief Starts the network discovery algorithm.
 * 
 */
void init_network(void);

/**
 * @brief Forwards the network discovery packet & updates the routing tables.
 * 
 * @param packet Pointer to instance of plantio_packet_t
 */
void forward_discover(const plantio_packet_t* packet);

/**
 * @brief Finds the optimal route given the current routing metric by applying
 * the Shortest Path Tree (SPT) algorithm.
 * 
 * @return const uint16_t Index of the optimal route in the routing table
 */
const uint16_t find_best_route(void);

/**
 * @brief Prints the routing table.
 * 
 */
void print_routing_table(void);

/**
 * @brief Clears the routing table.
 * 
 */
void clear_routing_table(void);

/**
 * @brief Write a route to the routing table on Flash memory.
 * 
 * @param route Array of Node IDs
 * @param length Length of the array, i.e. the number of hops of the route
 */
void write_routing_table(const uint8_t *route, const uint16_t length);

/**
 * @brief Get the number of hops for a route with given index in the routing table.
 * 
 * @param index Index of route in table
 * @return const uint16_t Number of hops
 */
const uint16_t get_num_hops(const uint16_t index);

/**
 * @brief Get the route for given index in the routing table.
 * 
 * @param route Pointer to the allocated memory for the route 
 * @param num_hops Number of hops
 * @param index  Index of route in table
 */
void get_route(uint8_t *route, const uint16_t num_hops, const uint16_t index);