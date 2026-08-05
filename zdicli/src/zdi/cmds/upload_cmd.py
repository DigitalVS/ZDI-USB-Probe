from pathlib import Path

from zdi.cmds.cmd import ErrorCode, DATA_SIZE_24BIT
from zdi.icd_comm import IcdComm
from zdi.output import Output
from zdi.devices import default_device
from zdi.utils import int_or_hex


def handler(args):
    output = Output(args)

    prog = IcdComm(
        "UPLOAD", default_device(), output.verbosity()
    )

    if int_or_hex(args.address) > DATA_SIZE_24BIT:
        output.error("UPLOAD", f"Invalid address: {args.address}. Address is 3 bytes long number.")
        return

    file_path = Path(args.filename)

    if file_path.suffix != ".bin":
        output.error("UPLOAD", f"Invalid file type: {args.filename}. Filename must end with '.bin'")
        return

    try:
        with open(args.filename, 'rb') as file:
            ret = prog.send_file(int_or_hex(args.address), file)

            if ret == ErrorCode.ERR_SUCCESS:
                output.success("UPLOAD", f"{ret.description}")
            else:
                output.error("UPLOAD", f"{ret.description}")

            file.close()
    except FileNotFoundError:
        output.error("Error", "The file was not found.")