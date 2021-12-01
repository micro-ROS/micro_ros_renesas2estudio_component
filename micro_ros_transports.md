<br/>
<a>
   <p align="center">
      <img width="40%" src=".images/renesas_logo.gif">
      <img width="40%" src=".images/microros_logo.png">
   </p>
</a>
<br/>

# micro-ROS transports for  Renesas e<sup>2</sup> studio

Depending on which transport is used for micro-ROS specific configurations, the following should be done.

- [micro-ROS transports for  Renesas e<sup>2</sup> studio](#micro-ros-transports-for--renesas-esup2sup-studio)
  - [USB-CDC transport](#usb-cdc-transport)
  - [Serial UART transport](#serial-uart-transport)
  - [UDP transport (FreeRTOS + TCP)](#udp-transport-freertos--tcp)
  - [UDP transport (ThreadX + NetX)](#udp-transport-threadx--netx)
  - [CAN FD transport](#can-fd-transport)

## USB-CDC transport
1. Copy the following files to the source directory:
      - `extra_sources/microros_transports/usb_transport.c`
      - `extra_sources/microros_transports/usb_descriptor.c`
2. Double click on the `configuration.xml` file of your project and go to the `Stacks` tab.
3. Select `New Stack -> Middleware -> USB -> USB PCDC driver on r_usb_pcdc`.
4. Go to `Clocks` tab and configure `UCLK` clock to match 48MHz (Match the values on the highlighted boxes):

   ![image](.images/Configure_usb_clock.png)

5. Save the modifications by clicking on `Generate Project Content`.

## Serial UART transport
1. Copy the following files to the source directory:
      - `extra_sources/microros_transports/uart_transport.c`
2. Double click on the `configuration.xml` file of your project and go to the `Stacks` tab.
3. Select `New Stack -> Driver -> Connectivity -> r_src_uart`.
4. *Optional: in order to set P411 and P410 as Tx/Rx first disable SPI1*
5. Go to the component properties and configure the Tx/Rx pinout:

   ![image](.images/Configure_serial.png)

6. Save the modifications clicking on `Generate Project Content`.

## UDP transport (FreeRTOS + TCP)

1. Copy the following files to the source directory:
      - `extra_sources/microros_transports/udp_transport_freeRTOS.c`

2. Double click on the `configuration.xml` file of your project and go to the `Stacks` tab.
3. Select `New Stack -> FreeRTOS -> Libraries -> FreeRTOS + TCP`.
4. Configure the properties of the `FreeRTOS + TCP component`:
   1. `Common -> vApplicationIPNetworkEventHook` to `Disable`.
   2. `Common -> DHCP Register Hostname` to `Disable`.
   3. *Optional: Enable DHCP `Common -> Use DHCP` to `Enable`*.
   
      *Note: If DHCP is disabled, the board network parameters can be configured on the transport source file `udp_transport_freeRTOS.c` *

   4. *Optional: Increase number of buffers avaliable to the IP stack on `Common -> Total number of avaliable network buffers`*.

   ![image](.images/FreeRTOSTCP_conf.png)

5.  Save the modifications by clicking on `Generate Project Content`.
6.  Configure micro-ROS agent IP and port passing a freeRTOS `freertos_sockaddr` struct to the `rmw_uros_set_custom_transport` function:

      ```
      struct freertos_sockaddr remote_addr;
      remote_addr.sin_family = FREERTOS_AF_INET;
      remote_addr.sin_port = FreeRTOS_htons(8888);
      remote_addr.sin_addr = FreeRTOS_inet_addr("192.168.1.185");

      rmw_uros_set_custom_transport(
         false,
         (void *) &remote_addr,
         renesas_e2_transport_open,
         renesas_e2_transport_close,
         renesas_e2_transport_write,
         renesas_e2_transport_read);
      ```

## UDP transport (ThreadX + NetX)

1. Copy the following files to the source directory:
      - `extra_sources/microros_transports/udp_transport_threadX.c`
2. Double click on the `configuration.xml` file of your project and go to the `Stacks` tab.
3. Select `New Stack -> Azure RTOS -> NetX Duo -> Protocols -> NetX Duo DHCP IPv4 Client`.
4. Click on Add NetX Duo Packet Pool and select `Use g_packet_pool0 NetX Duo Packet Pool Instance`:

   ![image](.images/ThreadX_DHCP_conf.png)

5. *Optional: Increase number of buffers avaliable to the IP stack on `g_packet_pool0` properties on `Module g_packet_pool0 NetX Duo Packet Pool Instance -> Number of Packets in Pool`*.
6.  Save the modifications by clicking on `Generate Project Content`.
7.  Configure micro-ROS agent IP and port passing a `custom_transport_args` struct to the `rmw_uros_set_custom_transport` function:

      ```
      custom_transport_args remote_addr = {
         .agent_ip_address=IP_ADDRESS(192,168,1,185),
         .agent_port=8888
      };

      rmw_uros_set_custom_transport(
         false,
         (void *) &remote_addr,
         renesas_e2_transport_open,
         renesas_e2_transport_close,
         renesas_e2_transport_write,
         renesas_e2_transport_read);
      ```

## CAN FD transport
1. Copy the following files to the source directory:
      - `extra_sources/microros_transports/canfd_transport.c`
2. Double click on the `configuration.xml` file of your project and go to the `Stacks` tab.
3. Select `New Stack -> Driver -> Connectivity -> r_can_fd`.
4. Go to `Clocks` tab:
   1. Configure `CANFDCLK` clock to match 40 MHz.
   2. Make sure `PCLKB` clock is set to 50 MHz and `PCLKA` to 100 MHz.

      *Example clock configuration:*

      ![image](.images/Configure_CAN_clock.png)

5. Configure CAN reception:
   1. `Common -> Reception -> Message Buffers -> Number of Buffers` to `0`.
   2. `Common -> Reception -> FIFOs -> FIFO 0 -> Enable` to `Enabled`
   3. `Common -> Reception -> FIFOs -> FIFO 0 -> Interrupt Mode` to `Every Frame`.
   4. `Common -> Reception -> FIFOs -> FIFO 0 -> Payload Size` to `64 bytes`.

6. Configure CAN interrupts:
   1. Enable `Module g_canfd0 CANFD Driver on r_canfd -> Transmit Interrupts -> TXMB 0`.
   2. Set `Module g_canfd0 CANFD Driver on r_canfd -> Interrupts -> Channel Interrupt Priority Level` to `Priority 3`.
   3. Enable all interrupts on `Module g_canfd0 CANFD Driver on r_canfd -> Channel Error Interrupts`.

7. Configure CAN component:
   1. `Module g_canfd0 CANFD Driver on r_canfd -> General -> Channel` to `1`.
   2. Go to the Pins tab and configure your `CANFD1` pinout. As an example to use with the integrated `TJA1042T` transceiver:

      ![image](.images/CAN_pinout.png)

8. Configure your CAN FD Bitrate:
   1. As an example:

      ![image](.images/Bitrate_CAN_example.png)

   2. Make sure the configuration matches with the CAN used on the agent. Example configuration on linux:
      ```
      sudo ip link set can0 up type can bitrate 500000 sample-point 0.75 dbitrate 2000000 fd on
      sudo ifconfig can0 txqueuelen 65536
      ```

9.  Modify micro-ROS library build options:
    1.  Set transport MTU to 64 bytes:
        ```
        {
        "names": {
            "microxrcedds_client": {
                    "cmake-args": [
                    "-DUCLIENT_CUSTOM_TRANSPORT_MTU=64",
                    ...
                    ]
            },
        }
        }
        ```


    2.  *Optional: Increase the number of stream buffers to match your message requirements:*
        ```
        {
        "names": {
            "rmw_microxrcedds": {
                    "cmake-args": [
                    "-DRMW_UXRCE_STREAM_HISTORY=8",
                    ...
                    ]
            },
        }
        }
        ```

      This parameter will control the maximum payload of a publish message:
      `RMW_UXRCE_STREAM_HISTORY * UCLIENT_CUSTOM_TRANSPORT_MTU = 512 bytes`

   3. To rebuild the micro-ROS library, clean and rebuild your project.

10. Set CAN transport configuration on the `canfd_transport.c` file:
    1. Set an unique CAN frame ID for this device (CAN_ID).
    2. *Optional: Increase reception buffer size (CAN_BUFFER_SIZE).*
    3. *Optional: Enable the BRS (bit rate switch) flag (enable_BRS).*

         *Example CAN transport configuration:*

         ![image](.images/CAN_transport_conf.png)

11. Save the modifications clicking on `Generate Project Content`.