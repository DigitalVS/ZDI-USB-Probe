from cmds.cmd import Cmd, DATA_SIZE_24BIT, DisassemblerOptions
from devices import default_device
from icd_comm import IcdComm
from output import Output
import struct

from utils import int_or_hex


def handler(args):
    output = Output(args)

    prog = IcdComm("DISASSM", default_device(), output.verbosity())

    try:
        flags = 0
        if args.motorola:
            flags |= DisassemblerOptions.SUFFIXED_IMM

        if args.address is not None:
            address = int_or_hex(args.address)

            if address > DATA_SIZE_24BIT:
                output.error("DISASSM", f"Invalid address: {args.address}. Address is 3 bytes long number.")
                return

            status, rd_size, rd_data = prog.send_data_with_var_response(Cmd.DISASSEMBLE_ADDR + address.to_bytes(3, 'little') + flags.to_bytes(1) + args.length.to_bytes(1))
        else:
            status, rd_size, rd_data = prog.send_data_with_var_response(Cmd.DISASSEMBLE + flags.to_bytes(1) + args.length.to_bytes(1))

        if status:
            instructions = parse_instructions(rd_data)

            for inst in instructions:
                print(f"{inst["addr"]:x}", inst["instr"])
    except ValueError as e:
        output.error("DISASSM", f"{e}")


def parse_instructions(data: bytes):
    # Read total instructions (2 bytes, little-endian)
    num_instructions = struct.unpack_from('<H', data, 0)[0]

    offset = 2
    instructions = []

    for _ in range(num_instructions):
        # Extract 3-byte address value
        three_byte_chunk = data[offset:offset + 3]
        address_val = int.from_bytes(three_byte_chunk, byteorder='little')
        offset += 3

        # Read 2-byte string length
        string_length = struct.unpack_from('<H', data, offset)[0]
        offset += 2

        # Extract and decode exactly string_length bytes as ASCII
        string_val = data[offset:offset + string_length].decode('ascii')
        offset += string_length

        instructions.append({
            "addr": address_val,
            "instr": string_val
        })

    return instructions