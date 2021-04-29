# micro-ROS for Renesas e<sup>2</sup> studio

This tool aims to ease the micro-ROS integration in a Renesas e<sup>2</sup> studio project.

## Requeriments

1. Renesas e<sup>2</sup> studio for Linux
2. Install board pack on e<sup>2</sup> studio: [Guide](Install_packs.md).
3. Docker
## How to use it

micro-ROS can be used with Renesas e<sup>2</sup> studio following these steps:

1. Clone this repository in your Renesas e<sup>2</sup> studio project folder
2. Go to `Project -> Properties -> C/C++ Build -> Settings -> Build Steps Tab` and in `Pre-build steps` add:

```bash
docker pull microros/micro_ros_static_library_builder:foxy && docker run --rm -v ${TCINSTALL}:/toolchain -v ${workspace_loc:/${ProjName}}:/project --env MICROROS_LIBRARY_FOLDER=micro_ros_renesas2estudio_component microros/micro_ros_static_library_builder:foxy "${cross_toolchain_flags}"
```

3. Add micro-ROS include directory. In `Project -> Settings -> C/C++ Build -> Settings -> Tool Settings Tab -> GNU ARM Cross C Compiler -> Includes` add `"${workspace_loc:/${ProjName}/micro_ros_renesas2estudio_component/libmicroros/include}"` in `Include paths (-l)`
4. Add the micro-ROS precompiled library. In `Project -> Settings -> C/C++ Build -> Settings -> Tool Settings Tab -> GNU ARM Cross C Linker -> Libraries`
      - add `"${workspace_loc:/${ProjName}/micro_ros_renesas2estudio_component/libmicroros}"` in `Library search path (-L)`
      - add `microros` in `Libraries (-l)`
5. Add the following source code files to your project, dragging them to source folder:
      - `extra_sources/microros_time.c`
      - `extra_sources/microros_allocators.c`
      - `extra_sources/microros_transports/usb_transport.c`
      - `extra_sources/microros_transports/usb_descriptor.c`
6. Configure `g_timer0` as an `r_gpt`.
   1. Double click on the `configuration.xml` file of your project and go to the `Components` tab.
   2. Filter for `timer` and enable the `r_gpt` timer:
   
      ![image](.images/Enable_timer.png)

   3. Go to the `Stacks` tab, then select `New Stack -> Driver -> Timers -> Timer Driver on r_gpt`.
   4. Modify the clock period on the component properties (`Module g_timer0 Timer Driver on r_gpt -> General -> Period`) to `0x40000000000`
   5. Save the modification using `ctrl + s` and click on `Generate Project Content`.

7. Configure the transport: [Detail](##Micro-XRCE-DDS-transport-configuration)
8. Configure the Main stack and Heap size:
   1. On the `configuration.xml` menu, go to the `BSP` tab.
   2. Go to the `RA Common` section and set the `Main stack size (bytes)` and `Heap size (bytes)` fields to 5000:

      ![image](.images/Configure_memory.png)

   3. Save the modification using `ctrl + s` and click on `Generate Project Content`.

9. Build and run your project

## Micro XRCE-DDS transport configuration
### USB transport
1. Copy the `transport_usb.c` file to the source directory.
2. Double click on the `configuration.xml` file of your project and go to the `Components` tab.
3. Filter for `usb` and enable the `r_usb_basic` and `r_usb_pcdc` components:

   ![image](.images/Enable_usb.png)

4. Go to the `Stacks` tab, then select `New Stack -> Middleware -> USB -> USB PCDC driver on r_usb_pcdc`.
5. Go to `Clocks` tab and configure `UCLK` clock to match 48MHz (Match the values on the highlighted boxes):

   ![image](.images/Configure_usb_clock.png)

6. Save the modification using `ctrl + s` and click on `Generate Project Content`.

### Serial transport
1. Copy the `transport_serial.c` file to the source directory.
2. Double click on the `configuration.xml` file of your project and go to the `Components` tab.
3. Filter for `uart` and enable the `r_src_uart` component.
4. Go to the `Stacks` tab, then select `New Stack -> Driver -> Connectivity -> r_src_uart`.
5. Go to the component properties and configure the Tx/Rx pinout:

   ![image](.images/Configure_serial.png)

6. Save the modification using `ctrl + s` and click on `Generate Project Content`.

## Purpose of the Project

This software is not ready for production use. It has neither been developed nor
tested for a specific use case. However, the license conditions of the
applicable Open Source licenses allow you to adapt the software to your needs.
Before using it in a safety relevant setting, make sure that the software
fulfills your requirements and adjust it according to any applicable safety
standards, e.g., ISO 26262.

## License

This repository is open-sourced under the Apache-2.0 license. See the [LICENSE](LICENSE) file for details.

For a list of other open-source components included in this repository,
see the file [3rd-party-licenses.txt](3rd-party-licenses.txt).

## Known Issues/Limitations

There are no known limitations.
