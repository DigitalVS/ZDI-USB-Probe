from zdi.cmds.cmd import Cmd
from zdi.cmds.cmd import State
from zdi.icd_comm import IcdComm
from zdi.output import Output
from zdi.devices import default_device

def handler(args):
    output = Output(args)

    if args.speed is None and args.adl is None:
        output.error("SET", "At least one of --speed or --adl must be provided.")
        return

    output.output(2,f"Speed: {args.speed}, ADL mode: {args.adl}")

    prog = IcdComm(
        "SET", default_device(), output.verbosity()
    )

    prog.send_data(Cmd.SET +
                   (b'' if args.speed is None else State.ZDI_SPEED + args.speed.to_bytes()) +
                   (b'' if args.adl is None else State.ADL_MODE + args.adl.to_bytes()))
