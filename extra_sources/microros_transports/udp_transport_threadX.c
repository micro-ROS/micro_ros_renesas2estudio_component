#include "hal_data.h"
#include "nxd_dhcp_client.h"

#include <uxr/client/transport.h>
#include <rmw_microxrcedds_c/config.h>
#include <microros_transports.h>

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

NX_IP g_ip0;
NX_DHCP g_dhcp_client0;
NX_UDP_SOCKET socket;
NX_PACKET_POOL g_packet_pool0;

// Stack memory for g_ip0.
uint8_t g_ip0_stack_memory[G_IP0_TASK_STACK_SIZE] BSP_PLACE_IN_SECTION(".stack.g_ip0") BSP_ALIGN_VARIABLE(BSP_STACK_ALIGNMENT);

// ARP cache memory for g_ip0.
uint8_t g_ip0_arp_cache_memory[G_IP0_ARP_CACHE_SIZE] BSP_ALIGN_VARIABLE(4);

// Stack memory for g_ip0 Packet pool.
uint8_t g_packet_pool0_pool_memory[G_PACKET_POOL0_PACKET_NUM * (G_PACKET_POOL0_PACKET_SIZE + sizeof(NX_PACKET))] BSP_ALIGN_VARIABLE(4) ETHER_BUFFER_PLACE_IN_SECTION;

#define LINK_ENABLE_WAIT_TIME (TX_WAIT_FOREVER)
#define SOCKET_FIFO_SIZE G_PACKET_POOL0_PACKET_NUM

// Set TX_TIMER_TICKS_PER_SECOND to 1000 (1 ms tick) in thread conf
#define TX_MS_TO_TICKS(milliseconds) ((ULONG) ((milliseconds / 1000.0) * TX_TIMER_TICKS_PER_SECOND))

void tx_application_define_user(void *first_unused_memory);

// User resources init before ThreadX starts
void tx_application_define_user(void *first_unused_memory)
{
    (void) first_unused_memory;

    // Unique MAC address for device
	const bsp_unique_id_t * unique_id = R_BSP_UniqueIdGet();
	for(size_t i = 2; i < 6; i++)
    {
        uint32_t sum = 0;
        size_t UniqueId_offset = (i-2)*4;
        for (size_t j = 0; j < 4; j++)
        {
            sum += unique_id->unique_id_words[j+UniqueId_offset];
        }
        g_ether0_cfg.p_mac_address[i] = (uint8_t) (sum % UINT8_MAX);
    }

    // Initialize the NetX system.
    nx_system_initialize ();
}

bool renesas_e2_transport_open(struct uxrCustomTransport * transport){
    (void) transport;

    // Create the packet pool.
    UINT status = nx_packet_pool_create(&g_packet_pool0,
            "g_packet_pool0 Packet Pool",
            G_PACKET_POOL0_PACKET_SIZE,
            &g_packet_pool0_pool_memory[0],
            G_PACKET_POOL0_PACKET_NUM * (G_PACKET_POOL0_PACKET_SIZE + sizeof(NX_PACKET)));

    if(NX_SUCCESS != status)
    {
        return false;
    }

    // Create the ip instance.
    status = nx_ip_create(&g_ip0,
                          "g_ip0 IP Instance",
                          G_IP0_ADDRESS,
                          G_IP0_SUBNET_MASK,
                          &g_packet_pool0,
                          g_netxduo_ether_0,
                          &g_ip0_stack_memory[0],
                          G_IP0_TASK_STACK_SIZE,
                          G_IP0_TASK_PRIORITY);

    if(NX_SUCCESS != status)
    {
        return false;
    }

    // Set the gateway address if it is configured.
    if (IP_ADDRESS(0, 0, 0, 0) != G_IP0_GATEWAY_ADDRESS)
    {
        status = nx_ip_gateway_address_set (&g_ip0, G_IP0_GATEWAY_ADDRESS);
        if(NX_SUCCESS != status)
        {
            return false;
        }
    }

    // Enable Address Resolution Protocol (ARP).
    status = nx_arp_enable(&g_ip0, &g_ip0_arp_cache_memory[0], G_IP0_ARP_CACHE_SIZE);
    if(NX_SUCCESS != status)
    {
        return false;
    }

    // Enable udp.
    status = nx_udp_enable(&g_ip0);

    if(NX_SUCCESS != status)
    {
        return false;
    }

    // Configure DHCP
	status = nx_dhcp_create(&g_dhcp_client0, &g_ip0, "g_dhcp_client0");

    if(NX_SUCCESS != status)
	{
		return false;
	}

	nx_dhcp_packet_pool_set(&g_dhcp_client0, &g_packet_pool0);
    nx_dhcp_start(&g_dhcp_client0);

    // Wait for the link to be enabled.
    ULONG current_state = 0;
	ULONG needed_state = NX_IP_UDP_ENABLED | NX_IP_ARP_ENABLED | NX_IP_LINK_ENABLED | NX_IP_ADDRESS_RESOLVED;
    status = nx_ip_status_check(&g_ip0, needed_state, &current_state, LINK_ENABLE_WAIT_TIME);

    if(NX_SUCCESS != status)
    {
        return false;
    }

    status = nx_udp_socket_create(&g_ip0, &socket, "Micro socket", NX_IP_NORMAL, NX_DONT_FRAGMENT, NX_IP_TIME_TO_LIVE, SOCKET_FIFO_SIZE);

    if(NX_SUCCESS != status)
    {
        return false;
    }

    // Find first avaliable udp port and bind socket.
    UINT port;
    status = nx_udp_free_port_find(&g_ip0, 1, &port);

    if(NX_SUCCESS != status)
    {
        return false;
    }

    status = nx_udp_socket_bind(&socket, port, TX_NO_WAIT);

    if(NX_SUCCESS != status)
    {
        return false;
    }

    return true;
}

bool renesas_e2_transport_close(struct uxrCustomTransport * transport){
    (void) transport;
    
    UINT status = nx_udp_socket_unbind(&socket);
    
    if(NX_SUCCESS != status)
    {
        return false;
    }

    status = nx_udp_socket_delete(&socket);

    if(NX_SUCCESS != status)
    {
        return false;
    }

    status = nx_packet_pool_delete(&g_packet_pool0);

    if(NX_SUCCESS != status)
    {
        return false;
    }

    return true;
}

size_t renesas_e2_transport_write(struct uxrCustomTransport* transport, const uint8_t * buf, size_t len, uint8_t * err){
    custom_transport_args * args = (custom_transport_args*) transport->args;

    volatile int result = FSP_SUCCESS;
    NX_PACKET *data_packet;

    // Get free packet from g_packet_pool0
    result = nx_packet_allocate(&g_packet_pool0, &data_packet, NX_UDP_PACKET, NX_NO_WAIT);

    if (result != NX_SUCCESS)
    {
        *err = (uint8_t) result;
        return 0;
    }

    result = nx_packet_data_append(data_packet, (VOID *)buf, len, &g_packet_pool0, NX_NO_WAIT);

    if (result != NX_SUCCESS)
    {
        *err = (uint8_t) result;
        return 0;
    }

    result = nx_udp_socket_send(&socket, data_packet, args->agent_ip_address, args->agent_port);

    if (result != NX_SUCCESS)
    {
        *err = (uint8_t) result;
        return 0;
    }

    return len;
}

size_t renesas_e2_transport_read(struct uxrCustomTransport* transport, uint8_t* buf, size_t len, int timeout, uint8_t* err){
    (void) transport;
    (void) len;

    NX_PACKET *data_packet = NULL;
    ULONG bytes_read = 0;

    UINT result = nx_udp_socket_receive(&socket, &data_packet, TX_MS_TO_TICKS(timeout));

    if (result != NX_SUCCESS)
    {
        *err = (uint8_t) result;
        return 0;
    }

    // Retrieve the data from the packet
    result = nx_packet_data_retrieve(data_packet, buf, &bytes_read);

    // Release the packet
    nx_packet_release(data_packet);

    if(NX_SUCCESS != result)
    {
        *err = (uint8_t) result;
        return 0;
    }

    return bytes_read;
}
