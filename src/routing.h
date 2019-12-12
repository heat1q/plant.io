#pragma once

#include <net/rime/rime.h> // Establish connections.
#include <net/netstack.h>  // Wireless-stack definitions
#include <core/net/linkaddr.h>
#include <core/net/rime/broadcast.h>
#include <net/packetbuf.h>

#include "plantio.h"

// Forward declaration of processes
struct process p_broadcast; // Broadcast process for Flooding & Network Discovery

struct broadcast_conn* plantio_broadcast; // Creates an instance of a broadcast connection.

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