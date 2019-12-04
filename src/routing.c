#include "routing.h"

PROCESS(p_broadcast, "Broadcast Process");
PROCESS_THREAD(p_broadcast, ev, data)
{
    PROCESS_EXITHANDLER(broadcast_close(&plantio_broadcast);)
    PROCESS_BEGIN();

    // Defines the functions used as callbacks for a broadcast connection.
    const struct broadcast_callbacks plantio_broadcast_call = {broadcast_receive};

    broadcast_open(&plantio_broadcast, 129, &plantio_broadcast_call);

    // Wait forever, since we need the broadcast always open
    while (1)
    {
        PROCESS_WAIT_EVENT();
    }

    PROCESS_END();
}

void broadcast_receive(struct broadcast_conn *broadcast, const linkaddr_t *from)
{
    printf("Broadcast message received from 0x%x%x: '%s' [RSSI %d]\r\n",
           from->u8[0], from->u8[1],
           (char *)packetbuf_dataptr(),
           (int16_t)packetbuf_attr(PACKETBUF_ATTR_RSSI));
}

void init_network(void)
{
    /*
    broadcast_open(&plantio_broadcast, 129, NULL);
    packetbuf_copyfrom("Hello", 6);
    broadcast_send(&plantio_broadcast);
    broadcast_close(&plantio_broadcast);
    */
    printf("Init network\r\n");
}