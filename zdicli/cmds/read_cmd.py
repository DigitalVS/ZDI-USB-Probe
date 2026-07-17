from cmds.cmd import Cmd, DATA_SIZE_24BIT
from icd_comm import IcdComm
from output import Output
from devices import default_device
from utils import int_or_hex, hexdump, calc_checksum


def handler(args):
    output = Output(args)

    prog = IcdComm(
        "READ", default_device(), output.verbosity()
    )

    try:
        address = int_or_hex(args.address)

        if address > DATA_SIZE_24BIT:
            output.error("WRITE", f"Invalid address: {args.address}. Address is 3 bytes long number.")
            return

        length = int_or_hex(args.length)

        if length > 255:
            output.error("READ", f"Length too long: {args.length}. Max length is 255 bytes.")
            return

        status, rd_checksum, rd_size, rd_data = prog.send_data_with_var_response_crc(Cmd.READ + address.to_bytes(3, 'little') + length.to_bytes(2, 'little'))

        if status:
            checksum = calc_checksum(rd_data)

            if length != rd_size:
                output.error("READ", f"Read data failed! Read {rd_size} of {int_or_hex(args.length)} bytes")
            elif checksum != rd_checksum:
                output.error("READ", f"Checksum error: ({checksum}/{rd_checksum})")
            else:
                #output.success("READ", f"Bytes read: {rd_size}")
                hexdump(address, rd_data)
    except ValueError as e:
        output.error("READ", f"{e}")
