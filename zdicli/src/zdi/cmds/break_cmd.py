from zdi.cmds.cmd import Cmd, BreakpointFields, BreakpointStatus, DATA_SIZE_24BIT
from zdi.cmds.cmd import ErrorCode
from zdi.icd_comm import IcdComm
from zdi.output import Output
from zdi.devices import default_device
from zdi.utils import int_or_hex


def handler(args):
    output = Output(args)
    prog = IcdComm("BREAK", default_device(), output.verbosity())

    try:
        if args.address is not None:
            address = int_or_hex(args.address)

            if address > DATA_SIZE_24BIT:
                output.error("BREAK", f"Invalid address: {args.address}. Address is 3 bytes long number.")
                return

        if args.enable is None and args.address is None: # Get a breakpoint data (number, enabled, address)
            status, bp_response = prog.send_data_with_response(Cmd.BREAK_READ + args.break_no.to_bytes(), 6)
        else: # Returns the same data as it was sent
            status, bp_response = prog.send_data_with_response(Cmd.BREAK_SET + BreakpointFields.NUMBER + args.break_no.to_bytes() + BreakpointFields.ENABLE +
                        (b'' if args.enable is None else args.enable.to_bytes()) +
                        (b'' if args.address is None else BreakpointFields.ADDRESS + int_or_hex(args.address).to_bytes(3, 'little')), 6)

        if status:
            if bp_response[0].to_bytes() == Cmd.ERROR:
                output.error("BREAK", f"BREAK error: {ErrorCode.get_description(bp_response[1])} ({bp_response[1]})")
                return

            bp_no = int(bp_response[1])
            bp_status = BreakpointStatus.get_description(bp_response[2])
            bp_address = int.from_bytes(bp_response[3:], byteorder='little')

            if bp_response[0].to_bytes() == Cmd.BREAK_READ:
                print(f"Breakpoint no: {bp_no}, Status: {bp_status}", end="")
                print("") if bp_status == BreakpointStatus.NOT_SET else print(f", Address: {hex(bp_address)}")
            else:
                print(f"Breakpoint no {bp_no} is successfully set", end="")
                print("") if args.address is None else print(f" to address: {args.address}")
    except ValueError as e:
        output.error("BREAK", f"{e}")


def handler_breaks(args):
    output = Output(args)
    prog = IcdComm("BREAKS", default_device(), output.verbosity())

    status, bp_response = prog.send_data_with_response(Cmd.BREAKS + args.disable.to_bytes(1), 21)

    if status:
        if bp_response[0] == Cmd.ERROR:
            output.error("BREAKS", f"BREAKS error: {ErrorCode.get_description(bp_response[1])} ({bp_response[1]})")
            return

        for i in range(0, 4):
            bp_no = int(bp_response[i * 5 + 1])
            bp_status = BreakpointStatus.get_description(bp_response[i * 5 + 2])
            bp_address = int.from_bytes(bp_response[i * 5 + 3 : i * 5 + 6], byteorder='little')

            print(f"Breakpoint no: {bp_no}, Status: {bp_status}, Address: {hex(bp_address)}")


def handler_step(args):
    output = Output(args)
    prog = IcdComm("STEP", default_device(), output.verbosity())

    status, ret_code = prog.send_data_with_response(Cmd.STEP, 2)

    if status:
        if ret_code[1] != ErrorCode.ERR_SUCCESS.id_value:
            output.error("STEP", f"{ErrorCode.get_description(ret_code[1])} ({ret_code[1]})")
