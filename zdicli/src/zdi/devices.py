import serial.tools.list_ports
import zdi.args_read

OUR_VID = 0x2E8A
OUR_PID = 0x0009


def handler(args):
    devices = find_devices()

    if not devices:
        print(f"No {zdi.args_read.BOARD_NAME} device(s) found")
    else:
        for device in devices:
            print(device["Device"])


def find_devices() -> list[dict[str, str | None]]:
    all_progs = serial.tools.list_ports.comports(False)
    return [
        {
            "Device": prog.device,
            "Product": prog.product,
            "Manufacturer": prog.manufacturer,
        }
        for prog in all_progs
        if prog.vid == OUR_VID and prog.pid == OUR_PID
    ]


def default_device() -> str | None:
    for dev in serial.tools.list_ports.comports(False):
        if dev.vid == OUR_VID and dev.pid == OUR_PID:
            return dev.device

    return None
