#include "FreeRTOS_IP.h"
#include "FreeRTOS_IP_Private.h"
#include "FreeRTOS_Sockets.h"
#include <uxr/client/transport.h>
#include <uxr/client/util/time.h>

#include <rmw_microxrcedds_c/config.h>
#include <microros_transports.h>

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// FreeRTOS MAC will be copied from g_ether0 configured value.
static uint8_t ucMACAddress[ 6 ]       = {0x00};

// Default IP configuration.
// - This values will be used as the board static IP address if DHCP is disabled.
// - If DHCP is enabled, they will be override with the received IP address.
static uint8_t ucIPAddress[ 4 ]        = {192, 168, 1, 180};
static uint8_t ucNetMask[ 4 ]          = {255, 255, 255, 0};
static uint8_t ucGatewayAddress[ 4 ]   = {0, 0, 0, 0};
static uint8_t ucDNSServerAddress[ 4 ] = {10, 60, 1, 2};

struct freertos_sockaddr *remote_addr;
Socket_t xSocket;

static bool net_init = false;

bool renesas_e2_transport_open(struct uxrCustomTransport * transport){
    (void) transport;
    bool rv = false;

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

    memcpy(ucMACAddress, g_ether0_cfg.p_mac_address, sizeof(ucMACAddress));

    if(!net_init){
        net_init = true;
        FreeRTOS_IPInit(ucIPAddress,
                        ucNetMask,
                        ucGatewayAddress,
                        ucDNSServerAddress,
                        ucMACAddress);
    }

    xSocket = FreeRTOS_socket(FREERTOS_AF_INET, FREERTOS_SOCK_DGRAM, FREERTOS_IPPROTO_UDP);
    if (FREERTOS_INVALID_SOCKET != xSocket)
    {
        // Set write timeout
        TickType_t timeout_ticks = pdMS_TO_TICKS(WRITE_TIMEOUT);
        FreeRTOS_setsockopt(xSocket, 0, FREERTOS_SO_RCVTIMEO, &timeout_ticks, 0);
        rv = true;
    }

    return rv;
}

bool renesas_e2_transport_close(struct uxrCustomTransport * transport){
    (void) transport;
    (void) FreeRTOS_shutdown(xSocket, FREERTOS_SHUT_RDWR);
    (void) FreeRTOS_closesocket(xSocket);
    return true;
}

size_t renesas_e2_transport_write(struct uxrCustomTransport* transport, const uint8_t * buf, size_t len, uint8_t * err){
    remote_addr = (struct freertos_sockaddr *) transport->args;
    size_t rv = 0;

    BaseType_t bytes_sent = FreeRTOS_sendto(xSocket, buf, len, 0, remote_addr, sizeof(struct freertos_sockaddr));

    if (0 <= bytes_sent)
    {
        rv = (size_t)bytes_sent;
        *err = 0;
    }
    else
    {
        *err = 1;
    }

    return rv;
}

size_t renesas_e2_transport_read(struct uxrCustomTransport* transport, uint8_t* buf, size_t len, int timeout, uint8_t* err){
    (void) transport;
    size_t rv = 0;

    // Set read timeout
    TickType_t timeout_ticks = pdMS_TO_TICKS(timeout);
    FreeRTOS_setsockopt(xSocket, 0, FREERTOS_SO_RCVTIMEO, &timeout_ticks, 0);

    int32_t bytes_received = FreeRTOS_recvfrom(xSocket, (void*)buf, len, 0, NULL, NULL);
    if (0 <= bytes_received)
    {
        rv = (size_t)bytes_received;
        *err = 0;
    }
    else
    {
        *err = 1;
    }

    return rv;
}

