import struct

from zdi.cmds.cmd import Cmd
from zdi.icd_comm import IcdComm
from zdi.output import Output
from zdi.devices import default_device


def handler(args):
    output = Output(args)

    prog = IcdComm("STATUS", default_device(), output.verbosity())

    status, res_data = prog.send_data_with_response_err(Cmd.STATUS, 5)

    if status:
        speed_type, speed, zdi_status_type, zdi_status = struct.unpack('<cccc', res_data)

        print(f"ZDI speed: {speed[0]}MHz")
        print(f"eZ80 status: ADL = {zdi_status[0] >> 4 & 1}, MADL = {zdi_status[0] >> 3 & 1}, ZDI active:", "Yes," if (zdi_status[0] >> 7) & 1 else "No,",
              "Halt/Sleep:", "Yes," if (zdi_status[0] >> 5) & 1 else "No,", "Interrupts enabled:", "Yes" if (zdi_status[0] >> 2) & 1 else "No")
        #print(f"Target connected:", "Yes" if target_connected[0] else "No")


def version():
    prog = IcdComm("VERSION", default_device(), 0)

    try:
        status, res_data = prog.send_data_with_response(Cmd.VERSION, 4)

        if status:
            return f"{int(res_data[1])}.{int(res_data[2])}.{res_data[3]}"
        else:
            return "x.x.x"
    except Exception as e:
        return "x.x.x"