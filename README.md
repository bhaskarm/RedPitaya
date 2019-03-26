
## This is the last stable version of RedPitaya bhaskarm fork. This is a fork that does high speed ADC transmission over ethernet. It also includes high speed DAC waveform generation over ethernet. The code is based on OS 0.94 and might not work on newer OS versions.


# Red Pitaya ecosystem and applications

Here you will find the sources of various software components of the
Red Pitaya system. The components are mainly contained in dedicated
directories, however, due to the nature of the Xilinx SoC "All 
Programmable" paradigm and the way several components are interrelated,
some components might be spread across many directories or found at
different places one would expect.

| directories  | contents
|--------------|----------------------------------------------------------------
| api          | `librp.so` API source code
| Applications | WEB applications (controller modules & GUI clients).
| apps-free    | WEB application for the old environment (also with controller modules & GUI clients).
| Bazaar       | Nginx server with dependencies, Bazaar module & application controller module loader.
| fpga         | FPGA design (RTL, bench, simulation and synthesis scripts)
| OS/buildroot | GNU/Linux operating system components
| patches      | Directory containing patches
| scpi-server  | SCPI server
| Test         | Command line utilities (acquire, generate, ...), tests
| shared       | `libredpitaya.so` API source code (to be deprecated soon hopefully!)

## Supported platforms

Red Pitaya is developed on Linux, so Linux (preferably 64bit Ubuntu) is also the only platform we support.

## Software requirements

You will need the following to build the Red Pitaya components:

1. Various development packages:

```bash
# Basic build env
sudo apt install build-essentials
# generic dependencies
sudo apt-get install make curl xz-utils
# U-Boot build dependencies
sudo apt-get install libssl-dev device-tree-compiler u-boot-tools
# Bunch of 32 bit versions of our libraries
sudo apt-get install libc6:i386 libstdc++6:i386
sudo apt-get install lib32z1
# Qemu for debian image compilation
sudo apt-get install debootstrap qemu-user-static
sudo ln -s /usr/bin/make /usr/bin/gmake
```

2. Xilinx [Vivado 2015.4](http://www.xilinx.com/support/download/index.html/content/xilinx/en/downloadNav/vivado-design-tools/2015-4.html) FPGA development tools. The SDK (bare metal toolchain) must also be installed, be careful during the install process to select it. Preferably use the default install location.
Download the Xilinx.lic file and load it using the license manager
```bash
sudo chown -R <username >~/.Xilinx
Chmod 755 -R ~/.Xilinx
Chgrp -R <username> ~/.Xilinx
Copy Xilinx.lic to ~/.Xilinx/
```
# Build process

Go to your preferred development directory and clone the Red Pitaya repository from GitHub.
```bash
git clone --branch adc_devel https://github.com/bhaskarm/RedPitaya.git
cd RedPitaya
```

An example script `settings.sh` is provided for setting all necessary environment variables. The script assumes some default tool install paths, so it might need editing if install paths other than the ones described above were used.
```bash
source ~/xilinx.bashrc
. settings.sh
```

Prepare a download cache for various source tarballs. This is an optional step which will speedup the build process by avoiding downloads for all but the first build. There is a default cache path defined in the `settings.sh` script, you can edit it and avoid a rebuild the next time.
```bash
mkdir -p dl
export BR2_DL_DIR=$PWD/dl
```

To build everything just run `make`.
```bash
make
```

# Partial rebuild process

The next components can be built separately.
- FPGA + device tree
- u-Boot
- Linux kernel
- Debian OS
- API
- SCPI server
- free applications

## Base system

Here *base system* represents everything before Linux user space.

### FPGA and device tree

Detailed instructions are provided for [building the FPGA](fpga/README.md#build-process) including some [device tree details](fpga/README.md#device-tree).

### U-boot

To build the U-Boot binary and boot scripts (used to select between booting into Buildroot or Debian):
```bash
make tmp/u-boot.elf
make build/u-boot.scr
```
The build process downloads the Xilinx version of U-Boot sources from Github, applies patches and starts the build process. Patches are available in the `patches/` directory.

If the files in the /boot partition are being modified (boot.bin or devicetree.dtb) there is no need to generate a new image. sftp the new files over and copy them to the /boot directory

```bash
sftp root@192.168.2.100
put boot.bin
quit
```
Make the /boot partition RW and copy the new file

```bash
sudo mount -o remount,rw /dev/mmcblk0p1 /boot
cp ~/boot.bin /boot
```
### Linux kernel

To build a Linux image:
```bash
make tmp/uImage
```
The build process downloads the Xilinx version of Linux sources from Github, applies patches and starts the build process. Patches are available in the `patches/` directory.

### Boot file

The created boot file contains FSBL, FPGA bitstream and U-Boot binary.
```bash
make tmp/boot.bin.uboot
```
Since file `tmp/boot.bin.uboot` is created it should be renamed to simply `tmp/boot.bin`. There are some preparations for creating a memory test `tmp/boot.bin.memtest` which would run from the SD card, but it did not go es easy es we would like, so it is not working.

## Linux user space

### Debian OS

```bash
sudo OS/debian/image.sh
```

Sometimes image.sh starts the qemu sshd daemon but it never stop it; this causes the final removal of ./root directory to fail. Kill the qemu sshd daemon, unmount ./root and ./boot. Remove the root 

### API

To compile the API run:
```bash
make api
```
The output of this process is the Red Pitaya `librp.so` library in `api/lib` directory.
The header file for the API is `redpitaya/rp.h` and can be found in `api/includes`.
You can install it on Red Pitaya by copying it there:
```
scp api/lib/librp.so root@192.168.0.100:/opt/redpitaya/lib/
```

### SCPI server

Scpi server README can be found [here](scpi-server/README.md).

To compile the server run:
```bash
make api
```
The compiled executable is `scpi-server/scpi-server`.
You can install it on Red Pitaya by copying it there:
```bash
scp scpi-server/scpi-server root@192.168.0.100:/opt/redpitaya/bin/
```

### Free applications

To build free applications, follow the instructions given at `apps-free`/[`README.md`](apps-free/README.md) file.

## Builds and changes done on the Red Pitaya board

The Red Pitaya board comes with gcc and g++ installed. Compilation on new code works fine althogh it can be a little slow.
Copy the librp.so from /opt/redpitaya/lib to the /lib directory once the board is booted up.

