<br/>
<a>
   <p align="center">
      <img width="40%" src=".images/renesas_logo.gif">
      <img width="40%" src=".images/microros_logo.png">
   </p>
</a>
<br/>

# micro-ROS transports for  Renesas e<sup>2</sup> studio

Depending on which transport is used for micro-ROS specific configurations should be done.

- [micro-ROS transports for  Renesas e<sup>2</sup> studio](#micro-ros-transports-for--renesas-esup2sup-studio)
  - [USB-CDC transport](#usb-cdc-transport)
  - [Serial UART transport](#serial-uart-transport)
  - [UDP transport (FreeRTOS + TCP)](#udp-transport-freertos--tcp)
  - [UDP transport (ThreadX + NetX)](#udp-transport-threadx--netx)

## USB-CDC transport
1. Copy the following files file to the source directory:
      - `extra_sources/microros_transports/usb_transport.c`
      - `extra_sources/microros_transports/usb_descriptor.c`
2. Double click on the `configuration.xml` file of your project and go to the `Components` tab.
3. Filter for `usb` and enable the `r_usb_basic` and `r_usb_pcdc` components:

   ![image](.images/Enable_usb.png)

4. Go to the `Stacks` tab, then select `New Stack -> Middleware -> USB -> USB PCDC driver on r_usb_pcdc`.
5. Go to `Clocks` tab and configure `UCLK` clock to match 48MHz (Match the values on the highlighted boxes):

   ![image](.images/Configure_usb_clock.png)

6. Save the modification using `ctrl + s` and click on `Generate Project Content`.

## Serial UART transport
1. Copy the following files file to the source directory:
      - `extra_sources/microros_transports/uart_transport.c`
2. Double click on the `configuration.xml` file of your project and go to the `Components` tab.
3. Filter for `uart` and enable the `r_sci_uart` component.
4. Go to the `Stacks` tab, then select `New Stack -> Driver -> Connectivity -> r_src_uart`.
5. *Optional: in order to set P411 and P410 as Tx/Rx first disable SPI1*
6. Go to the component properties and configure the Tx/Rx pinout:

   ![image](.images/Configure_serial.png)

7. Save the modification using `ctrl + s` and click on `Generate Project Content`.

## UDP transport (FreeRTOS + TCP)

1. Copy the following files file to the source directory:
      - `extra_sources/microros_transports/udp_transport_freeRTOS.c`

2. Go to the `Stacks` tab, then select `New Stack -> FreeRTOS -> Libraries -> FreeRTOS + TCP`.
3. Click on FreeRTOS + TCP properties and set `Common -> Use DHCP` to `Enable`.
4. Click on FreeRTOS + TCP properties and set `Common -> DHCP Register Hostname` to `Disable`.
5. Click on FreeRTOS + TCP properties and set `Common -> vApplicationIPNetworkEventHook` to `Disable`.

   ![image](.images/FreeRTOSTCP_conf.png)

## UDP transport (ThreadX + NetX)

TODO