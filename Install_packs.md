# Board pack installation on Renesas e<sup>2</sup> studio

## Download

- Download the latest FSP packs zip from the renesas repository: https://github.com/renesas/fsp/releases. For example: [FSP_Packs_v2.4.0.zip](https://github.com/renesas/fsp/releases/download/v2.4.0/FSP_Packs_v2.4.0.zip)

## Install FSP packs

1. Open e<sup>2</sup> studio and go to `Help -> CMSIS Packs Management -> Renesas RA`.
2. Here you will see the internal packs folder.

![image](.images/Packs_path.png)

3. Extract the downloaded packs on the 'internal' folder path: `unzip FSP_Packs_v2.4.0.zip -d /home/username/.eclipse/com.renesas.platform_1177272026/internal`.

4. Now check on e<sup>2</sup> studio that the packs were detected and installed successfully:

![image](.images/FSP_installed.png)

Note: Check the permissions on this folder, Renesas e<sup>2</sup> studio will need to write on this directory.

## Install GNU Arm Embedded Toolchain

1. Download and extract the latest version from Arm Developer site: [link](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads). In our case: [gcc-arm-none-eabi-10-2020-q4-major-x86_64-linux.tar.bz2](https://developer.arm.com/-/media/Files/downloads/gnu-rm/10-2020q4/gcc-arm-none-eabi-10-2020-q4-major-x86_64-linux.tar.bz2?revision=ca0cbf9c-9de2-491c-ac48-898b5bbc0443&la=en&hash=68760A8AE66026BCF99F05AC017A6A50C6FD832A).

2. Open e<sup>2</sup> studio and go to `Help -> Add Renesas Toolchains -> Add...`.
3. Click on Browse and select the compiler extracted folder, the compiler should be detected automatically:

![image](.images/Compiler_install.png)
