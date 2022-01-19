
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


typedef enum {
  STATE_WAIT_FOR_SIZE = 0,
  STATE_WAIT_FOR_DATA,
  STATE_MESSAGE_AVAILABLE
} tcp_states_t;

typedef struct {
  uint8_t buffer[UXR_CONFIG_CUSTOM_TRANSPORT_MTU];

  uint8_t length_buffer[2];

  uint16_t message_size;
  uint16_t message_size_received;

  tcp_states_t state;
} tcp_receiver_t;

static bool net_init = false;

void read_tcp_data(tcp_receiver_t * receiver);
Socket_t xSocket;

// TODO: Handle errors
bool renesas_e2_transport_open(struct uxrCustomTransport * transport) {
    custom_transport_args * args = (custom_transport_args*) transport->args;
    WIFINetworkParams_t * network_conf = (WIFINetworkParams_t *) args->network_conf;
    SocketsSockaddr_t * socket_addr = (SocketsSockaddr_t *) args->socket_addr;

    // Set network struct length values. TODO: could strlen be problematic?
    network_conf->xPassword.xWPA.ucLength = (uint8_t) strlen((char *) network_conf->xPassword.xWPA.cPassphrase);
    network_conf->ucSSIDLength            = (uint8_t) strlen((char *) network_conf->ucSSID);

    WIFIReturnCode_t wifi_err = WIFI_On();

    if (eWiFiSuccess != wifi_err)
    {
        return false;
    }

    // Connect to wifi. TODO: Check timeouts or retries
    wifi_err = WIFI_ConnectAP(network_conf);

    if (eWiFiSuccess != wifi_err)
    {
        return false;
    }

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

    // Create socket
    xSocket = SOCKETS_Socket(SOCKETS_AF_INET, SOCKETS_SOCK_STREAM, SOCKETS_IPPROTO_TCP);

    if(SOCKETS_INVALID_SOCKET == xSocket)
    {
        return false;
    }

    // Connect to agent TCP server
    sock_err = SOCKETS_Connect(xSocket, socket_addr, sizeof(SocketsSockaddr_t));

    if (SOCKETS_ERROR_NONE != sock_err)
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
    int32_t bytes_sent = 0;

    // As we are using a TCP stream connection we should indicate the size of the message with the first two bytes of the stream.
    static uint8_t buffer_size[2];
    buffer_size[0] = (uint8_t)(0x00FF & len);
    buffer_size[1] = (uint8_t)((0xFF00 & len) >> 8);
    bytes_sent = SOCKETS_Send(xSocket, (const void *) &buffer_size[0], 2, 0);

    // Then we send the payload
    if (bytes_sent == 2)
    {
        bytes_sent = SOCKETS_Send(xSocket, (const void *) &buf[0], len, 0);

        if (0 <= bytes_sent)
        {
            rv = (size_t) bytes_sent;
            *err = 0;
        }
        else
        {
          *err = 1;
        }
    }
    else
    {
      *err = 1;
    }

    return rv;
}

size_t renesas_e2_transport_read(struct uxrCustomTransport* transport, uint8_t* buf, size_t len, int timeout, uint8_t* err) {
    (void) transport;
    (void) len;

    static tcp_receiver_t receiver = {};
    size_t rv = 0;

    do
    {
        // Set read timeout
        TickType_t timeout_ticks = pdMS_TO_TICKS(timeout);
        SOCKETS_SetSockOpt(xSocket, 0, SOCKETS_SO_RCVTIMEO, &timeout_ticks, 0);

        int64_t time_init = uxr_millis();
        read_tcp_data(&receiver);
        timeout -= (int)(uxr_millis() - time_init);
    }
    while ((STATE_MESSAGE_AVAILABLE != receiver.state) && (0 < timeout));

    if (STATE_MESSAGE_AVAILABLE == receiver.state)
    {
      rv = receiver.message_size;
      *err = 0;

      memcpy(buf, receiver.buffer, rv);
      receiver.state = STATE_WAIT_FOR_SIZE;
    }
    else
    {
        *err = 1;
    }

    return rv;
}

void read_tcp_data(tcp_receiver_t * receiver) {

  int32_t bytes_received = 0;
  size_t to_read = 0;

  switch(receiver->state)
  {
    case STATE_WAIT_FOR_SIZE:
      bytes_received = SOCKETS_Recv(xSocket, (void*) &receiver->length_buffer[0], 2, 0);
      if (bytes_received >= 2)
      {
        receiver->message_size = (uint16_t)(receiver->length_buffer[0] | (receiver->length_buffer[1] << 8));
        receiver->message_size_received = 0;
        receiver->state = STATE_WAIT_FOR_DATA;
      }
      break;

    case STATE_WAIT_FOR_DATA:
      to_read = receiver->message_size - receiver->message_size_received;
      bytes_received = SOCKETS_Recv(xSocket, (void*) &receiver->buffer[receiver->message_size_received], to_read, 0);
      receiver->message_size_received += (uint16_t) bytes_received;

      if(receiver->message_size_received == receiver->message_size){
        receiver->state = STATE_MESSAGE_AVAILABLE;
      }

      break;

    case STATE_MESSAGE_AVAILABLE:
      break;
  }
}

