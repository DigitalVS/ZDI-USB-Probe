# ZDI USB Probe

This project is a Zilog debug interface (ZDI) software and hardware implementation. It enables reading from and writing to a device, debugging with breakpoints and all other ZDI functions.

Client side software is a command line interface written in Python, while firmware is made with RP Pico SDK. Hardware is either standard RP Pico development board with few additional resistors, or dedicated PCB (see the picture below) which have some advantages in comparison to the RP Pico board.

Target devices are primarily devices with Zilog eZ80, but also devices with Z8 Encore! and ZNEO MCUs.

<img src="./images/ZDI_USB_Board.png" width="640" alt="ZDI USB Board">

## Hardware Description

Hardware is based on RP2040 microcontroller and its PIO (Programmable I/O) subsystem with DMA data transfer. This ensures accurate signal waveforms and reliable communication with the target device for all supported ZDI speeds, from 1MHz to 8MHz.

As it's already mentioned, software can be used either with RP Pico board, or with a PCB made specifically for this project, named __ZDI USB Probe__. Both ways are described further in the text.

### Using RP Pico Development Board

Dedicated __ZDI USB Probe__ is based on RP2040 MCU but its functionality can be used with sole RP Pico board as well. Minimal configuration is shown on the following schematic diagram.

<img src="./images/RP_Pico_Board.png" width="640" alt="ZDI USB Board">

Voltage divider on the right side of the picture serves as target device sense pin. If the target VCC is not connected this way,
target device will not be seen as connected and most of the commands won't work.

### Using ZDI USB Probe Board

Using dedicated __ZDI USB Probe__ board instead of the RP Pico has numerous advantages:

* Contains six pin ICD connector to connect to the target device, and USB-C to connect with a PC computer
* Level shifters for ZDA and ZCL lines supporting 1.8V to 5V signal voltages
* Overvoltage protection on a ZDA and ZCL lines and ESD protection on a ZDA, ZCL and reset lines
* Tolerates target device VCC voltages to at least 6.5V
* Open drain reset signal output
* Very small target device power supply load of well below 1mA
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

The update process is straightforward. To install a UF2 file on the __ZDI USB Probe__ board, press and hold the BOOTSEL button while plugging the board (or RP Pico) into your computer via a data-capable USB cable. Release the button once a new drive named RPI-RP2 appears. Drag and drop your .uf2 file onto this drive; the Pico will automatically flash, reboot, and unmount.

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
    regs                display value for all registers or exchange register set
    disassm             disassemble a memory block
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

Read command reads data from the target device and prints it as a hex dump, like in the following example. It can read up to 256 bytes. For more than 256 bytes, you can use a download command. Start address can be decimal or hexadecimal number.

```
>zdi read 0x10000 10
0x010000 27 fd b7 ed 42 20 28 ed 55 e7                   |'...B (.U.|
```

Full read command syntax is listed here:

```text
usage: zdi read [-h] address length

positional arguments:
  address     start read address
  length      length in bytes to read (max 256 bytes)

options:
  -h, --help  show this help message and exit
```

#### Write Command

Write command sends and writes bytes to target's RAM memory. Bytes have to be provided as a hexadecimal string, like in the following example. This command can write up to 256 bytes. For more than 256 bytes, please, use upload command. Start address can be decimal or hexadecimal number.

```
>zdi write 0x60000 0x1122334455667788
```

```text
usage: zdi write [-h] address hex_string

positional arguments:
  address     start write address
  hex_string  hexadecimal string to write (max 256 bytes)

options:
  -h, --help  show this help message and exit
```

#### Upload Command

Upload command uploads binary file contents to the target device RAM memory starting from the provided address. Size of the data to upload has no limitation and is limited only by the size of target device memory.

```
>zdi upload 0x123456 D:\Projects\test.bin
UPLOAD    : 100%|████████████████████████████████████████████████████████████████████████| 32/32 [00:00<00:00, 19359.26b/s]
UPLOAD: Command successfully completed
```

```text
usage: zdi upload [-h] address filename

positional arguments:
  address     start upload address
  filename    binary file to upload

options:
  -h, --help  show this help message and exit
```

#### Download Command

Download command downloads data from the target device memory to a binary file. Size of the data to download has no limitation and is limited only by the size of target device memory.

```
>zdi download 0x12345 0x20 E:\Downloads\test.bin
DOWNLOAD  : 100%|████████████████████████████████████████████████████████████████████████| 32/32 [00:00<00:00, 9676.14b/s]
DOWNLOAD: Command successfully completed
```

Optionally, overwrite option may be added to overwrite download file of the same name if it exists.

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

Set command can set the ZDI communication speed, turn-on/off ADL mode and set Probe into the USB bootloader mode. For example, command ```zdi set -s 2``` will set ZDI speed to 2MHz. After __ZDI USB Probe__ starts, speed is always 1MHz, but it can be set the higher speed which will be active until the next restart.

Highest possible communication speed depends on a target system clock frequency. Use next table to determine the maximum speed for your case.

| System Clock Frequency | ZDI Clock Frequency
| ------ | ------
| 3–10 MHz | 1 MHz
| 8–16 MHz | 2 MHz
| 12–24 MHz | 4 MHz
| 20–50 MHz | 8 MHz

ZDI communication speed does not have much influence to the most of the commands because they are already short and various other factors are also important (e.g. USB communication speed via virtual serial port), maybe even more than ZDI clock frequency. 
However, both upload and download commands can benefit from higher ZDI speeds, especially if the amount of data to upload or download is larger.

Boot mode option restarts ZDI Probe in the USB bootloader mode which does the same as plugging
USB cable while BOOTSEL button is pressed. This way ZDI Probe can be put in the USB bootloader mode via
software instead of using physical button. This is useful when ZDI Probe PCB is encased in an enclosure
and button is not accessible.

This command does not return any values.

Command syntax is as follows:

```text
usage: zdi set [-h] [-s {1,2,4,8}] [-a {0,1}] [-b]

options:
  -h, --help            show this help message and exit
  -s, --speed {1,2,4,8}
                        set ZDI speed (1, 2, 4 or 8)
  -a, --adl {0,1}       set ADL mode value (0 or 1)
  -b, --bootusb         boot in USB bootloader mode
```

#### Break Command

Break command sets or prints single breakpoint information.

First example sets first breakpoint address to hex value 0x1122aa. Optionally, it can be 
set breakpoint as enabled, with option -e, or disabled with option -d.

```
>zdi break 1 0x1122aa
Breakpoint no 1 is successfully set to address: 0x1122aa
```
Next example prints information about the first breakpoint. Note that its status is not 
yet set, meaning it's initially neither enabled nor disabled. Second example below shows the enabled breakpoint.

```
>zdi break 1
Breakpoint no: 1, Status: Not set, Address: 0x1122aa
```

```
>zdi break 1
Breakpoint no: 1, Status: Enabled, Address: 0x1122aa
```

Command has the following syntax:

```text
usage: zdi break [-h] [-e | -d] {1,2,3,4} [address]

positional arguments:
  {1,2,3,4}      breakpoint number (value 1 to 4)
  address        breakpoint address

options:
  -h, --help     show this help message and exit
  -e, --enable   enable a breakpoint
  -d, --disable  disable a breakpoint
```

#### Step Command

This command single-steps program execution by asserting a BREAK after the next instruction. To execute step command, CPU must be in stopped state. If it is running, error message is printed and command aborts.

```text
usage: zdi step [-h]

options:
  -h, --help  show this help message and exit
```

#### Breaks Command

This command prints information about all breakpoints and can, optionally, disable all the breakpoints. 
```
>zdi breaks
Breakpoint no: 1, Status: Enabled, Address: 0x1122aa
Breakpoint no: 2, Status: Not set, Address: 0x0
Breakpoint no: 3, Status: Not set, Address: 0x0
Breakpoint no: 4, Status: Not set, Address: 0x0
```

```text
usage: zdi breaks [-h] [-d]

options:
  -h, --help     show this help message and exit
  -d, --disable  disable all breakpoints
```

#### Reg Command

Reg command reads from and writes to target device CPU registers.

Register read always displays three byte value. Next example shows read command for the BC register.
Lowest byte value (right side byte) is a C register value 0x11. Next byte (middle byte)
is B register value 0xC4. The highest byte is used only in ADL mode and represents upper BC pair byte (BCU).
In Z80 mode this third byte is not used and that value is ignored.

```
>zdi reg -r BC
REG: Register BC value is 0x0BC411
```
Because *read* command always prints three bytes, command ```zdi reg -r B``` actually prints
the same value as command that reads BC register pair.

Next table shows byte order for all eZ80 registers. In Z80 mode, MBASE determines which 64KB page of 16MB
address space is used. In other words, it is the 64KB window into the 16MB address space.

| Register | Highest byte | Middle byte | Lowest byte |
|----------|-|-------------|------------|
| AF       | MBASE | F           | A          |
| BC       | BCU | B           | C          |
| DE       | DEU | D           | E          |
| HL       | HLU | H           | L          |
| IX       | IXU | IXH         | IXL        |
| IY       | IYU | IYH         | IYL        |
| PC       | PC[23:16] | PC[15:8]    | PC[7:0]    |

Stack pointer (SP) register is a 24-bit SPL register in ADL mode, and 16-bit SPS register in non-ADL mode.

*Write* command can set any 8-bit register, including IXL, IXH, IYL and IYH. In ADL mode, register pairs (BC, DE, HL) and 
IX and IY registers are written as 24-bit registers. In Z80 mode, register pairs and IX and IY registers are processed
as 16-bit registers, unless *--long* option is used when they are assumed as 24-bit registers. This *--long* option is convenient
to use to set MBASE value. Of course, MBASE value can be set in ADL mode as well, which is also the official way to set it. 

Next example shows setting B register to value 0x11.

```
>zdi reg -w B 0x11
```

Command has the following syntax:

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

This command displays values of all the registers. All registers are always printed as a three byte value.
What each byte represents is described in the table in a **REG** command description.

> Note that for AF value, A register value is in the lowest byte (first from the right), and F register is in the middle byte. 
> On the other side, for register pairs order is different. For example, for BC register pair, C register value 
> is in the lowest byte and B value is in the middle byte.

> Highest byte of the AF value (first from the left) represents the MBASE value.

```
>zdi regs
AF: 0x004201
BC: 0x0BC437
DE: 0x0BFF79
HL: 0x008A8D
IX: 0x0BFF7A
IY: 0x0BCF0D
SP: 0x0BFF79
PC: 0x008A98
```

If used with **--exx** option, this command exchanges target CPU register set without displaying
a response message. Such command exchanges both AF and all register pairs (BC, DE, HL).

```text
usage: zdi regs [-h] [-x]

options:
  -h, --help  show this help message and exit
  -x, --exx   exchange CPU register set
```

#### Disassm Command

This command disassembles target device memory starting from the specified address and 
print it to the client's screen (see next example). By default, it uses Intel 
notation to display hexadecimal numbers but, optionally, it can display them in a Motorola's 
notation.

```
>zdi disassm 0x3000 -l 20
3000 jp 0003164h
3004 ld hl,(ix-003h)
3007 ld a,(hl)
3008 ld b,a
3009 rla
300a sbc hl,hl
300c ld l,b
300d ld bc,0000072h
3011 or a,a
3012 sbc hl,bc
```

```text
usage: zdi disassm [-h] [-l [16-255]] [-m] [address]

positional arguments:
  address               start address (default is PC register value)

options:
  -h, --help            show this help message and exit
  -l, --length [16-255]
                        number of bytes to disassemble (default is 32)
  -m, --motorola        Motorola hex number representation instead of default Intel hex representation
```

#### Run Command

This command set the CPU to continue execution from current PC register value (releases the break state). If CPU is already running, this command has no effect.

Command does not print any response massage. Status command can be used to check if CPU is running or it is stopped.

```text
usage: zdi run [-h]

options:
  -h, --help  show this help message and exit
```

#### Stop Command

This command stops the CPU (set it in break state). If CPU is already in stopped state, this command has no effect.

Command does not print any response massage. Status command can be used to check if CPU is running or it is stopped.

```text
usage: zdi stop [-h]

options:
  -h, --help  show this help message and exit
```

#### Reset Command

This command resets the CPU, and when issued with *--full* option, resets, not only the CPU, but the whole target device. Optionally, additional *--stop* option stops the CPU on the first instruction after the reset. 
Stop option has effect only when used with full target reset.

```
>zdi reset
```

```
>zdi reset -f
```

```text
usage: zdi reset [-h] [-f] [-s]

options:
  -h, --help  show this help message and exit
  -f, --full  full target device reset instead of only CPU core reset
  -s, --stop  stop CPU on first instruction following the reset
```

#### Status Command

This command prints ZDI speed and status of the target CPU, like shown in the next example:

```
ZDI speed: 1MHz
eZ80 status: ADL = 0, MADL = 0, ZDI active: No, Halt/Sleep: No, Interrupts enabled: No
```
ZDI is active if target CPU is stopped, otherwise it is not active.

Command does not have parameters other than ```--help```.

```text
usage: zdi status [-h]

options:
  -h, --help  show this help message and exit
```

#### Devices Command

Lists all virtual serial ports where ZDI USB Probe is connected. For example, in Windows it will print ```COM7``` if __ZDI USB Probe__ is connected to COM port number 7.

If more probes are connected simultaneously, this command will list all of them. Other commands will automatically recognize connected probes and use first from the list.

One only possible command parameter is a *--help* parameter.

```text
usage: zdi devices [-h]

options:
  -h, --help  show this help message and exit
```

