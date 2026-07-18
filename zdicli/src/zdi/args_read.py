import argparse

import zdi.cmds.break_cmd
import zdi.cmds.disassm_cmd
import zdi.cmds.download_cmd
import zdi.cmds.read_cmd
import zdi.cmds.reg_cmd
import zdi.cmds.runstop_cmd
import zdi.cmds.set_cmd
import zdi.cmds.status_cmd
import zdi.cmds.upload_cmd
import zdi.cmds.write_cmd

BOARD_NAME = "ZDI USB Probe"
PROG_NAME = f"{BOARD_NAME} Command Line Interface"
VERSION = "0.1.0"
FW_VERSION = zdi.cmds.status_cmd.version()

def main():
    args = _arg_parser().parse_args()
    args.func(args)

def _arg_parser():
    parser = argparse.ArgumentParser(description=f"{PROG_NAME}", formatter_class=argparse.RawTextHelpFormatter)
    group = parser.add_mutually_exclusive_group()

    parser.add_argument(
        "--version", action="version",
        version=f"{PROG_NAME} V{VERSION}\n"
                f"ZDI USB Probe firmware V{FW_VERSION}\n"
                f"Copyright (c) 2026 Vitomir Spasojević"
    )
    group.add_argument(
        "-q", "--quiet", action="store_true", help="silence almost all output"
    )
    group.add_argument(
        "-v", "--verbose", action="store_false", help="allow additional output"
    )
    group.add_argument(
        "-t", "--trace", action="store_false", help="enable debugging output",
    )

    subs = parser.add_subparsers(required=True)

    # Read
    read_act = subs.add_parser("read", help="read data from target device")
    read_act.add_argument(
        "address",
        type=str,
        help="start read address",
    )
    read_act.add_argument(
        "length",
        type=str,
        help="length in bytes to read",
    )
    read_act.set_defaults(func=zdi.cmds.read_cmd.handler)

    # Write
    write_act = subs.add_parser("write", help="write data to target device")
    # write_act.add_argument(
    #     "-v", "--verify", action="store_false", help="verify written data"
    # )
    write_act.add_argument(
        "address",
        type=str,
        help="start write address",
    )
    write_act.add_argument(
        "hex_string",
        #nargs='?',
        type=str,
        help="hexadecimal string to write (max 255 bytes)",
    )
    write_act.set_defaults(func=zdi.cmds.write_cmd.handler)

    # Upload
    upload_act = subs.add_parser("upload", help="upload binary file contents to target device")
    # upload_act.add_argument(
    #     "-v", "--verify", action="store_false", help="verify uploaded data"
    # )
    upload_act.add_argument(
        "address",
        type=str,
        help="start upload address",
    )
    upload_act.add_argument(
        "filename",
        type=str,
        help="binary file to upload",
    )
    upload_act.set_defaults(func=zdi.cmds.upload_cmd.handler)

    # Download
    download_act = subs.add_parser("download", help="download binary file contents from target device")
    download_act.add_argument(
        "-o", "--overwrite", action="store_true", help="overwrite existing file"
    )
    download_act.add_argument(
        "address",
        type=str,
        help="download start address",
    )
    download_act.add_argument(
        "length",
        type=str,
        help="download data length in bytes",
    )
    download_act.add_argument(
        "filename",
        type=str,
        help="binary file to save downloaded data",
    )
    download_act.set_defaults(func=zdi.cmds.download_cmd.handler)

    # Set
    set_act = subs.add_parser("set", help="set basic parameters (eg. ZDI speed, ADL mode, ICD boot mode)")
    set_act.add_argument(
        "-s",
        "--speed",
        type=int,
        choices=[1, 2, 4, 8],
        help="ZDI speed value (1, 2, 4 or 8)",
    )
    set_act.add_argument(
        "-a",
        "--adl",
        type=int,
        choices=[0, 1],
        help="ADL mode value (0 or 1)",
    )
    set_act.set_defaults(func=zdi.cmds.set_cmd.handler)

    # Break
    break_act = subs.add_parser("break", help="set or display breakpoint information")
    break_act.add_argument('break_no', type=int, choices=[1, 2, 3, 4], help='breakpoint number (value 1 to 4)')
    break_act.add_argument(
        "-e",
        "--enable",
        type=int,
        choices=[0, 1],
        nargs="?",
        const=1, # if argument is provided but value not set
        default=None, # if argument is not provided
        help="enable (value 1) or disable (value 0) a breakpoint",
    )
    break_act.add_argument(
        "address",
        nargs="?", # argument is optional
        type=str,
        help="breakpoint address",
    )
    break_act.set_defaults(func=zdi.cmds.break_cmd.handler)

    # Step
    break_act = subs.add_parser("step", help="single step execution")
    break_act.set_defaults(func=zdi.cmds.break_cmd.handler_step)

    # Breaks
    breaks_act = subs.add_parser("breaks", help="display information for all breakpoints")
    breaks_act.set_defaults(func=zdi.cmds.break_cmd.handler_breaks)
    breaks_act.add_argument(
        "-d", "--disable", action="store_false", help="disable all breakpoints",
    )

    # Reg
    reg_act = subs.add_parser("reg", help="set or display single register value")
    reg_group = reg_act.add_mutually_exclusive_group(required=False)
    REG_CHOICES = ['A', 'F', 'AF', 'B', 'C', 'BC', 'D', 'E', 'DE', 'H', 'L', 'HL', 'IXH', 'IXL', 'IX', 'IYH', 'IYL', 'IY', 'SP', 'PC']
    reg_group.add_argument(
        "-r",
        "--read",
        type=str,
        nargs="?",
        choices=REG_CHOICES,
        help="read and display register value",
    )
    reg_group.add_argument(
        "-w",
        "--write",
        type=str,
        nargs="?",
        choices=REG_CHOICES,
        help="set register value",
    )
    reg_act.add_argument(
        "value",
        nargs="?", # argument is optional
        type=str,
        help="new register value",
    )
    reg_act.add_argument(
        "-l", "--long", action="store_true", help="write 24-bit long value to register pair in non ADL mode",
    )
    reg_act.set_defaults(func=zdi.cmds.reg_cmd.handler)

    # Reg
    regs_act = subs.add_parser("regs", help="display value for all registers")
    regs_act.set_defaults(func=zdi.cmds.reg_cmd.handler_regs)

    # Disassemble
    disassm_act = subs.add_parser("disassm", help="disassemble a memory block")
    disassm_act.set_defaults(func=zdi.cmds.disassm_cmd.handler)
    disassm_act.add_argument(
        "address",
        nargs="?", # argument is optional
        type=str,
        help="start address (default is PC register value)",
    )
    disassm_act.add_argument(
        '-l', '--length',
        type=int_range,
        default=64,
        metavar="[16-255]",
        help="number of bytes to disassemble (default is 64)"
    )
    disassm_act.add_argument(
        "-m", "--motorola", action="store_false", help="Motorola hex number representation instead of default Intel hex representation",
    )

    # Run
    run_act = subs.add_parser("run", help="continue execution from the current address")
    run_act.set_defaults(func=zdi.cmds.runstop_cmd.handler_run)

    # Stop
    stop_act = subs.add_parser("stop", help="break on next instruction")
    stop_act.set_defaults(func=zdi.cmds.runstop_cmd.handler_stop)

    # Reset
    reset_act = subs.add_parser("reset", help="reset the CPU")
    reset_act.set_defaults(func=zdi.cmds.runstop_cmd.handler_reset)
    reset_act.add_argument(
        "-f", "--full", action="store_true", help="full target device reset instead of only CPU core reset",
    )

    # Status
    status_act = subs.add_parser("status", help="show status of target device")
    status_act.set_defaults(func=zdi.cmds.status_cmd.handler)

    # Devices list
    devices_act = subs.add_parser("devices", help=f"list {BOARD_NAME} devices")
    devices_act.set_defaults(func=zdi.devices.handler)

    return parser


def int_range(value):
    ivalue = int(value)
    if ivalue < 16 or ivalue > 255:
        raise argparse.ArgumentTypeError(f"{value} is an invalid value; must be number between 16 and 255")
    return ivalue


if __name__ == "__main__":
    main()
