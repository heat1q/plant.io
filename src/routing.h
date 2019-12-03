#pragma once

#include "plantio.h"

#include "net/rime/rime.h" // Establish connections.
#include "net/netstack.h"  // Wireless-stack definitions
#include "core/net/linkaddr.h"

// Forward declaration of processes 
struct process p_broadcast; // Broadcast process for Flooding & Network Discovery

static struct broadcast_conn plantio_broadcast; // Creates an instance of a broadcast connection.

static void broadcast_receive(struct broadcast_conn *broadcast, const linkaddr_t *from);

// Defines the functions used as callbacks for a broadcast connection.
static const struct broadcast_callbacks plantio_broadcast_call = { broadcast_receive };


static void init_network(void);