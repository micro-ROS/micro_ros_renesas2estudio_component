
#include <micro_ros_thread.h>

#include <uxr/client/transport.h>
#include <uxr/client/util/time.h>

#include <rmw_microxrcedds_c/config.h>
#include <microros_transports.h>

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <string.h>

#include "aws_secure_sockets_config.h"
#include "iot_secure_sockets.h"


static bool net_init = false;

Socket_t xSocket;

// TODO: Handle errors
bool renesas_e2_transport_open(struct uxrCustomTransport * transport) {
    custom_transport_args * args = (custom_transport_args*) transport->args;
    WIFINetworkParams_t * network_conf = (WIFINetworkParams_t *) args->network_conf;
    SocketsSockaddr_t * socket_addr = (SocketsSockaddr_t *) args->socket_addr;

    // Set network struct length values. TODO: could strlen be problematic?
    network_conf->xPassword.xWPA.ucLength = (uint8_t) strlen(network_conf->xPassword.xWPA.cPassphrase);
    network_conf->ucSSIDLength            = (uint8_t) strlen(network_conf->ucSSID);

    WIFIReturnCode_t wifi_err = WIFI_On();

    if (eWiFiSuccess != wifi_err)
    {
        return false;
    }

    // Connect to wifi. TODO: Check timeouts or retries
    WIFI_ConnectAP(&network_conf);

    // Initialize the Socket Interface
    BaseType_t sock_err;
    if(!net_init)
    {
        sock_err = SOCKETS_Init();

        if (pdPASS != sock_err)
        {
            // TODO: use assert(pdPASS == sock_err); ?
            return false;
        }

        net_init = true;
    }

    // Create TCP socket
    xSocket = SOCKETS_Socket(SOCKETS_AF_INET, SOCKETS_SOCK_STREAM, SOCKETS_IPPROTO_TCP);

    if(SOCKETS_INVALID_SOCKET != xSocket)
    {
        return false;
    }

    // Connect to agent TCP server
    sock_err = SOCKETS_Connect(xSocket, socket_addr, sizeof(SocketsSockaddr_t));

    if (pdPASS != sock_err)
    {
        return false;
    }

    return true;
}

bool renesas_e2_transport_close(struct uxrCustomTransport * transport) {
    (void) transport;

    (void) SOCKETS_Close(xSocket);
    (void) WIFI_Off();

    // TODO: Add checks?
    // 0 == SOCKETS_Close(xSocket);
    // eWiFiSuccess == WIFI_Off();

    return true;
}

size_t renesas_e2_transport_write(struct uxrCustomTransport* transport, const uint8_t * buf, size_t len, uint8_t * err) {
    (void) transport;

    size_t rv = 0;

    int32_t bytes_sent = SOCKETS_Send(xSocket, (const void *) buf, len, 0);

    if (0 <= bytes_sent)
    {
        rv = (size_t) bytes_sent;
        *err = 0;
    }
    else
    {
        *err = 1;
    }

    return rv;
}

size_t renesas_e2_transport_read(struct uxrCustomTransport* transport, uint8_t* buf, size_t len, int timeout, uint8_t* err) {
    (void) transport;

    size_t rv = 0;

    // Set read timeout
    TickType_t timeout_ticks = pdMS_TO_TICKS(timeout);
    SOCKETS_SetSockOpt(xSocket, 0, SOCKETS_SO_RCVTIMEO, &timeout_ticks, 0);

    int32_t bytes_received = SOCKETS_Recv(xSocket, (void*) buf, len, 0);

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

