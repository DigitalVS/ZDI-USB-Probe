from zdi.cmds.cmd import Cmd, ErrorCode, DATA_SIZE_24BIT
from zdi.icd_comm import IcdComm
from zdi.output import Output
from zdi.devices import default_device
from zdi.utils import int_or_hex, calc_checksum


def handler(args):
    output = Output(args)

    prog = IcdComm(
        "WRITE", default_device(), output.verbosity()
    )

    try:
        address = int_or_hex(args.address)

        if address > DATA_SIZE_24BIT:
            output.error("WRITE", f"Invalid address: {args.address}. Address is 3 bytes long number.")
            return

        data = bytearray.fromhex(args.hex_string.replace('0x', ''))
        length = len(data)

        if length > 255:
            output.error("WRITE", f"Length too long: {length}. Max length is 255 bytes.")
            return

        status, wr_response = prog.send_data_with_response(Cmd.WRITE + address.to_bytes(3, 'little') + length.to_bytes(2, byteorder='little') +  data, 2)

        if status:
            if wr_response[0] == Cmd.ERROR:
                output.error("WRITE", f"Write error: {ErrorCode.get_description(wr_response[1])} ({wr_response[1]})")
                return

            wr_checksum = wr_response[1]
            checksum = calc_checksum(data)

            if checksum != wr_checksum:
                output.error("WRITE", f"Checksum error: ({checksum}/{wr_checksum})")
            # else:
            #     output.success("WRITE", f"Bytes written: {wr_size}")
    except ValueError as e:
        output.error("WRITE", f"{e}")
