from enum import Enum, IntFlag, auto

DATA_SIZE_24BIT = 16777215 # 16.777.216 = 2**24, address is 24 bit

class Cmd:
    ERROR = b'\x00' # This is not a command but return message type
    VERSION = b'\x01'
    STATUS = b'\x02'
    SET = b'\x03'
    GET = b'\x04'
    READ = b'\x05'
    WRITE = b'\x06'
    VERIFY = b'\x07'
    BREAK_SET = b'\x08' # Breakpoint set value
    BREAK_READ = b'\x09' # Breakpoint read value
    BREAKS = b'\x0A' # Display all breakpoints information
    STEP = b'\x0B' # Single step
    REG_SET = b'\x0C' # Read single register (8, 16 or 24 bit)
    REG_READ = b'\x0D' # Write single register
    REGS = b'\x0E' # Display all registers or exchange registers
    REGS_EXX = b'\x0F' # Exchange register set
    RUN = b'\x10'
    STOP = b'\x11'
    RESET = b'\x12'
    DISASSEMBLE = b'\x13'
    DISASSEMBLE_ADDR = b'\x14'

# State type is used by SET command and as STATUS return message
class State:
    ZDI_SPEED = b'\x01'
    ADL_MODE = b'\x02'
    ZDI_STATUS = b'\x03' # ZDI status register
    BOOT_MODE = b'\x04'

# Break message fields
class BreakpointFields:
    NUMBER = b'\x00' # Followed by one byte value (1 to 4)
    ENABLE = b'\x01' # Followed by one byte value (0 - disabled, 1 - enabled)
    ADDRESS = b'\x02' # Followed by three bytes address value (little endian)

class BreakpointStatus(Enum):
    DISABLED = (b'\x00', "Disabled")
    ENABLED = (b'\x01', "Enabled")
    NOT_SET = (b'\x02', "Not set")

    def __init__(self, id_val: int, description: str):
        self._id = id_val
        self.description = description

    @classmethod
    def get_description(cls, int_value):
        """
        Retrieves the string description for a given integer value.
        """
        for member in cls:
            if int.from_bytes(member._id) == int_value:
                return member.description
        raise ValueError(f"No enum member found with value: {int_value}")

class Registers(Enum):
    REG_A = (b'\x00', "A")
    REG_F = (b'\x01', "F")
    REG_AF = (b'\x02', "AF")
    REG_B = (b'\x03', "B")
    REG_C = (b'\x04', "C")
    REG_BC = (b'\x05', "BC")
    REG_D = (b'\x06', "D")
    REG_E = (b'\x07', "E")
    REG_DE = (b'\x08', "DE")
    REG_H = (b'\x09', "H")
    REG_L = (b'\x0A', "L")
    REG_HL = (b'\x0B', "HL")
    REG_IXH = (b'\x0C', "IXH")
    REG_IXL = (b'\x0D', "IXL")
    REG_IX = (b'\x0E', "IX")
    REG_IYH = (b'\x0F', "IYH")
    REG_IYL = (b'\x10', "IYL")
    REG_IY = (b'\x11', "IY")
    REG_SP = (b'\x12', "SP")
    REG_PC = (b'\x13', "PC")

    def __init__(self, id_val: int, reg_name: str):
        self._id = id_val
        self.reg_name = reg_name

    @classmethod
    def get_id(cls, reg_name: str):
        for member in cls:
            if member.reg_name == reg_name:
                return member._id
        raise ValueError(f"No enum member found with name: {reg_name}")

    # @classmethod
    # def get_by_id(cls, reg_id: int):
    #     for member in cls:
    #         if member._id[0] == reg_id:
    #             return member.reg_name
    #     raise ValueError(f"No enum member found with id: {reg_id}")

class ErrorCode(Enum):
    ERR_SUCCESS = (0, "Command successfully completed")
    ERR_UNKNOWN = (1, "Failed to execute command") # Generic error message
    ERR_PARAM = (2, "Wrong command parameter")
    ERR_DATA_SIZE = (3, "Wrong data size")
    ERR_TIMEOUT = (4, "Timeout error")
    ERR_CHECKSUM = (5, "Checksum error")
    ERR_KEY_INTERRUPT = (6, "Canceled with CTRL+C")
    ERR_STEP = (7, "CPU must be stopped before single stepping")
    ERR_DISASSEMBLER = (8, "CPU must be stopped before disassembling region pointed by the PC register")
    ERR_NO_TARGET = (9, "No target device connected")

    def __init__(self, id_val: int, description: str):
        self._id = id_val
        self.description = description

    @property
    def id_value(self):
        return self._id

    @classmethod
    def get_description(cls, int_value):
        """
        Retrieves the string description for a given integer value.
        """

        for member in cls:
            if member._id == int_value:
                return member.description
        raise ValueError(f"No enum member found with value: {int_value}")


class DisassemblerOptions(IntFlag): # As a bitfield
    NONE = 0
    SUFFIXED_IMM = auto() # 1 - Intel hex, 0 - Motorola hex representation (with $ prefix)
    DECIMAL_IMM = auto() # 1 - decimal number, 0 - hex number
    MNE_SPACE = auto() # 1 - space after mnemonic , 0 - tab after mnemonic
    ARG_SPACE = auto() # 1 - space between arguments, 0 - without space after ','
    COMPUTE_REL = auto()
    COMPUTE_ABS = auto()