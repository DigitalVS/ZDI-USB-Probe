from zdi.cmds.cmd import Cmd, Registers, DATA_SIZE_24BIT
from zdi.devices import default_device
from zdi.icd_comm import IcdComm
from zdi.output import Output
from zdi.utils import int_or_hex


def handler(args):
    output = Output(args)

    if args.write and args.value is None:
        output.error("REG", "Value is required for write operation")
        return

    prog = IcdComm(
        "REG", default_device(), output.verbosity()
    )

    try:
        if args.value is not None:
            value = int_or_hex(args.value)

            if value > DATA_SIZE_24BIT:
                output.error("REG", f"Invalid value: {args.value}. Value is a one to three bytes long number.")
                return

        reg_val = 0

        if args.read is not None:
            reg_id = Registers.get_id(args.read)
            status, reg_response = prog.send_data_with_response_err(Cmd.REG_READ + reg_id, 5)
        else: # Write operation
            reg_id = Registers.get_id(args.write)
            reg_val = int_or_hex(args.value)
            status, reg_response = prog.send_data_with_response_err(Cmd.REG_SET + reg_id + reg_val.to_bytes(3, 'little') + bytes(1 if args.long else 0), 5)

        if status:
            if len(reg_response) == 0:
                output.error("REG", "Response not received!")
                return

            reg_val_response = int.from_bytes(reg_response[1:], 'little')

            if args.read is not None: # Read operation
                output.success("REG", f"Register {args.read} value is 0x{reg_val_response:06X}")
            else: # Write operation
                if reg_val != reg_val_response:
                    output.error("REG", f"Wrong register value returned: 0x{reg_val_response:06X}")
                    return
                #output.success("REG", f"Register {args.write} is successfully set to value {args.value}")
    except ValueError as e:
        output.error("REG", f"{e}")

def handler_regs(args):
    output = Output(args)

    prog = IcdComm(
        "REGS", default_device(), output.verbosity()
    )

    status, reg_response = prog.send_data_with_response_err(Cmd.REGS, 25)

    if status:
        regs = ["AF", "BC", "DE", "HL", "IX", "IY", "SP", "PC"]

        for i in range(0, 8):
            reg_val = int.from_bytes(reg_response[i * 3: i * 3 + 3], byteorder='little')
            print(f"{regs[i]}: 0x{reg_val:06X}")
