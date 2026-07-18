import os
from pathlib import Path

from zdi.cmds.cmd import ErrorCode, DATA_SIZE_24BIT
from zdi.icd_comm import IcdComm
from zdi.output import Output
from zdi.devices import default_device
from zdi.utils import int_or_hex


def handler(args):
    output = Output(args)

    prog = IcdComm(
        "DOWNLOAD", default_device(), output.verbosity()
    )

    if int_or_hex(args.address) > DATA_SIZE_24BIT:
        output.error("DOWNLOAD", f"Invalid address: {args.address}. Address is 3 bytes long number.")
        return

    file_path = Path(args.filename)

    if file_path.suffix != ".bin":
        output.error("DOWNLOAD", f"Invalid file type: {args.filename}. Filename must end with '.bin'")
        return

    if not args.overwrite and os.path.exists(args.filename):
        output.error("DOWNLOAD", f"File '{args.filename}' already exists.")
        return

    try:
        with open(args.filename, 'wb') as file:
            ret = prog.receive_file(int_or_hex(args.address), int_or_hex(args.length), file)

            if ret == ErrorCode.ERR_SUCCESS:
                output.success("DOWNLOAD", f"{ret.description}")
            else:
                output.error("DOWNLOAD", f"{ret.description}")

            file.close()
    except OSError as e:
        output.error("DOWNLOAD", f"An I/O error occurred: {e}")