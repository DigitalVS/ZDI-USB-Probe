# ZDI USB Probe

This project is a Zilog debug interface (ZDI) software and hardware implementation. It enables reading from and writing to a device, debugging with breakpoints and all other ZDI functions.

Client side software is a command line interface written in Python, while firmware is made with RP Pico SDK. Hardware is either RP Pico development board with few additional resistors, or dedicated PCB (see the picture below) which have some advantages in comparison to the RP Pico board.

Target devices are all devices with Zilog eZ80, Z8 or ZNEO MCUs.

<img src="./images/ZDI_USB_Board.png" width="640" alt="ZDI USB Board">

## Hardware Description

Hardware is based on RP2040 microcontroller and its PIO (Programmable I/O) subsystem with DMA data transfer. This ensures accurate signal waveforms and reliable communication with the target device for all supported ZDI speeds, from 1MHz to 8MHz.

As it's already mentioned, software can be used either with RP Pico board, or with a PCB made specifically for this project, named __ZDI USB Probe__. Both ways are described further in this document.

### Using RP Pico Development Board

Dedicated __ZDI USB Probe__ is based on RP2040 MCU but its functionality can be used with sole RP Pico board as well. Minimal configuration is shown on the following schematic diagram.

<img src="./images/RP_Pico_Board.png" width="640" alt="ZDI USB Board">

### Using ZDI USB Probe Board

Using dedicated __ZDI USB Probe__ board instead of the RP Pico has numerous advantages:

* Contains six pin ICD connector to connect to the target device, and USB-C to connect with a PC computer
* Level shifters for ZDA and ZCL lines supporting 1.8V to 5V signal voltages
* Overvoltage protection on a ZDA and ZCL lines and ESD protection on a ZDA, ZCL and reset lines
* Tolerates target device VCC voltages to at least 6.5V
* Open drain reset signal output
* Very small target device power supply load of less than 1mA
* Reliable communication on all ZDI speeds (up to 8MHz)

## Software

### Command Line Interface

ZDI client interface is written in Python language and as a prerequisite needs working Python 3.9+ installation.

Installation is easiest to do with pip command:

```shell
pip install zdi-usb-probe-cli
```

This package is also possible to install from files published in the release section of this repository or to use source files directly without the installation.

### Firmware Update

Latest firmware version is published in the release section of this repository as a UF2 file.

The update process is straightforward. To install a UF2 file on the ZDI USB Probe board, press and hold the BOOTSEL button while plugging the board (or RP Pico) into your computer via a data-capable USB cable. Release the button once a new drive named RPI-RP2 appears. Drag and drop your .uf2 file onto this drive; the Pico will automatically flash, reboot, and unmount.

### ZDI Commands Usage

Command line interface consists of a number of commands to read and write data, debug assembly programs and to manage target device state. General command arguments are:

```text
> .\zdi -h
usage: zdi [-h] [--version] [-q | -v | -t] {read,write,upload,download,set,break,step,breaks,reg,regs,disassm,run,stop,reset,status,devices} ...

ZDI USB Probe Command Line Interface

positional arguments:
  {read,write,upload,download,set,break,step,breaks,reg,regs,disassm,run,stop,reset,status,devices}
    read                read data from target device
    write               write data to target device
    upload              upload binary file contents to target device
    download            download data from target device and save it to binary file
    set                 set basic parameters (eg. ZDI speed, ADL mode, probe boot mode)
    break               set a breakpoint or print breakpoint information
    step                single step execution
    breaks              display information for all breakpoints
    reg                 set or display single register value
    regs                display value for all registers
    disassm             disassemble and print a memory block
    run                 continue execution from the current address
    stop                break on next instruction
    reset               reset the CPU and optionally entire target device
    status              show status of target device
    devices             list ZDI USB Probe devices

options:
  -h, --help            show this help message and exit
  --version             show program's version number and exit
  -q, --quiet           silence almost all output
  -v, --verbose         allow additional output
  -t, --trace           enable debugging output
```

Following sections describe all the available commands. Most of the commands needs target device to be connected and if it is not connected fail with a massage:

```text
Failure: No target device connected (9).
```

#### Read Command

```text
usage: zdi read [-h] address length

positional arguments:
  address     start read address
  length      length in bytes to read

options:
  -h, --help  show this help message and exit
```

#### Write Command

```text
usage: zdi write [-h] address hex_string

positional arguments:
  address     start write address
  hex_string  hexadecimal string to write (max 255 bytes)

options:
  -h, --help  show this help message and exit
```

#### Upload Command

```text
usage: zdi upload [-h] address filename

positional arguments:
  address     start upload address
  filename    binary file to upload

options:
  -h, --help  show this help message and exit
```

#### Download Command

```text
usage: zdi download [-h] [-o] address length filename

positional arguments:
  address          download start address
  length           download data length in bytes
  filename         binary file to save downloaded data

options:
  -h, --help       show this help message and exit
  -o, --overwrite  overwrite existing file
```

#### Set Command

Set command can set the ZDI communication speed and turn-on/off ADL mode. For example, command ```zdi set -s 2``` will set ZDI speed to 2MHz. After ZDI USB Probe starts, speed is always 1MHz but you can set the higher speed which will be active until the next restart.

Highest possible communication speed depends on a target system clock frequency. Use next table to determine the maximum speed for your case.

| System Clock Frequency | ZDI Clock Frequency
| ------ | ------
| 3–10 MHz | 1 MHz
| 8–16 MHz | 2 MHz
| 12–24 MHz | 4 MHz
| 20–50 MHz | 8 MHz

This command does not return any values.

Command syntax is as follows:

```text
usage: zdi set [-h] [-s {1,2,4,8}] [-a {0,1}]

options:
  -h, --help            show this help message and exit
  -s, --speed {1,2,4,8}
                        ZDI speed value (1, 2, 4 or 8)
  -a, --adl {0,1}       ADL mode value (0 or 1)
```

#### Break Command

```text
usage: zdi break [-h] [-e [{0,1}]] {1,2,3,4} [address]

positional arguments:
  {1,2,3,4}             breakpoint number (value 1 to 4)
  address               breakpoint address

options:
  -h, --help            show this help message and exit
  -e, --enable [{0,1}]  enable (value 1) or disable (value 0) a breakpoint
```

#### Step Command

```text
usage: zdi step [-h]

options:
  -h, --help  show this help message and exit
```

#### Breaks Command

```text
usage: zdi breaks [-h] [-d]

options:
  -h, --help     show this help message and exit
  -d, --disable  disable all breakpoints
```

#### Reg Command

```text
usage: zdi reg [-h] [-r [{A,F,AF,B,C,BC,D,E,DE,H,L,HL,IXH,IXL,IX,IYH,IYL,IY,SP,PC}] | -w [{A,F,AF,B,C,BC,D,E,DE,H,L,HL,IXH,IXL,IX,IYH,IYL,IY,SP,PC}]] [-l] [value]

positional arguments:
  value                 new register value

options:
  -h, --help            show this help message and exit
  -r, --read [{A,F,AF,B,C,BC,D,E,DE,H,L,HL,IXH,IXL,IX,IYH,IYL,IY,SP,PC}]
                        read and display register value
  -w, --write [{A,F,AF,B,C,BC,D,E,DE,H,L,HL,IXH,IXL,IX,IYH,IYL,IY,SP,PC}]
                        set register value
  -l, --long            write 24-bit long value to register pair in non ADL mode
```

#### Regs Command

```text
usage: zdi regs [-h]

options:
  -h, --help  show this help message and exit
```

#### Disassm Command

```text
usage: zdi disassm [-h] [-l [16-255]] [-m] [address]

positional arguments:
  address               start address (default is PC register value)

options:
  -h, --help            show this help message and exit
  -l, --length [16-255]
                        number of bytes to disassemble (default is 64)
  -m, --motorola        Motorola hex number representation instead of default Intel hex representation
```

#### Run Command

```text
usage: zdi run [-h]

options:
  -h, --help  show this help message and exit
```

#### Stop Command

```text
usage: zdi stop [-h]

options:
  -h, --help  show this help message and exit
```

#### Reset Command

```text
usage: zdi reset [-h] [-f] [-s]

options:
  -h, --help  show this help message and exit
  -f, --full  full target device reset instead of only CPU core reset
  -s, --stop  stop CPU on first instruction following the reset
```

#### Status Command

This command prints ZDI speed and status of the target MCU, like it is shown in the next example:

```
ZDI speed: 1MHz
eZ80 status: ADL = 0, MADL = 0, ZDI active: No, Halt/Sleep: No, Interrupts enabled: No
```
ZDI is active if target CPU is in ZDI mode, otherwise it is not active.

Command does not have parameters other then ```--help```.

```text
usage: zdi status [-h]

options:
  -h, --help  show this help message and exit
```

#### Devices Command

Lists all virtual serial ports where ZDI USB Probe is connected. For example, in Windows it will print ```COM7``` if ZDI USB Probe is connected to COM port number 7.

If more probes are connected simultaneously, this command will list all of them. Other commands will automatically recognize connected probes and use first from the list.

One only possible command parameter is a ```--help``` parameter.

```text
usage: zdi devices [-h]

options:
  -h, --help  show this help message and exit
```
