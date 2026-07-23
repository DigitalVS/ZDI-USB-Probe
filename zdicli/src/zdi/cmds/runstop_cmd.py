from zdi.cmds.cmd import Cmd
from zdi.devices import default_device
from zdi.icd_comm import IcdComm
from zdi.output import Output


def handler_run(args):
    handler(args, Cmd.RUN, "RUN")


def handler_stop(args):
    handler(args, Cmd.STOP, "STOP")


def handler_reset(args):
    if args.stop and not args.full: # Stop option will be ignored on the target device
        output = Output(args)
        output.warn("RESET", f"Option 'stop' is ignored if full target device reset option is not also set.")

    handler(args, Cmd.RESET + int(args.full).to_bytes(1) + int(args.stop).to_bytes(1), "RESET")


def handler(args, cmd, cmd_name):
    output = Output(args)

    prog = IcdComm(cmd_name, default_device(), output.verbosity())
    prog.send_data(cmd)