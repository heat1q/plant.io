#include "routing.h"
#include "net_packet.h"

const struct broadcast_callbacks plantio_broadcast_call = {broadcast_receive};

PROCESS(p_broadcast, "Broadcast Process");
PROCESS_THREAD(p_broadcast, ev, data)
{
    PROCESS_EXITHANDLER({broadcast_close(plantio_broadcast); free(plantio_broadcast);})
    PROCESS_BEGIN();

    plantio_broadcast = malloc(sizeof(struct broadcast_conn));

    // Defines the functions used as callbacks for a broadcast connection.
    broadcast_open(plantio_broadcast, 129, &plantio_broadcast_call);

    // Wait forever, since we need the broadcast always open
    while (1)
    {
        PROCESS_WAIT_EVENT();
    }

    PROCESS_END();
}

void broadcast_receive(struct broadcast_conn *broadcast, const linkaddr_t *from)
{
    leds_on(LEDS_GREEN);

    printf("Broadcast message received from 0x%x%x: [RSSI %d]\r\n", from->u8[0], from->u8[1], (int16_t)packetbuf_attr(PACKETBUF_ATTR_RSSI));

    plantio_packet_t* packet = (plantio_packet_t*)packetbuf_dataptr();
    print_packet(packet);    

    leds_off(LEDS_GREEN);
}

void init_network(void)
{
    leds_on(LEDS_RED);
    
    uint8_t test_src[10];
    uint8_t test_dest[10];
    for (size_t i = 0; i < 10; i++)
    {
        test_src[i] = i;
        test_dest[i] = rand() % 10;
    }
    
    plantio_packet_t* packet = create_packet(0, test_src, 10, test_dest, 10, "Default Load", 14);
    broadcast_send(plantio_broadcast);
    
    leds_off(LEDS_RED);
}