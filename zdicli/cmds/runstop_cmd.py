from cmds.cmd import Cmd
from devices import default_device
from icd_comm import IcdComm
from output import Output


def handler_run(args):
    handler(args, Cmd.RUN, "RUN")


def handler_stop(args):
    handler(args, Cmd.STOP, "STOP")


def handler_reset(args):
    handler(args, Cmd.RESET + int(args.full).to_bytes(1), "RESET")


def handler(args, cmd, cmd_name):
    output = Output(args)

    prog = IcdComm(
        cmd_name, default_device(), output.verbosity()
    )

    prog.send_data(cmd)