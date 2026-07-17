
def int_or_hex(value):
    """
    Converts an integer or hexadecimal string to an integer.
    :param value: Integer or hex string
    :return: Integer value
    """
    try:
        # Try converting as a decimal integer
        return int(value)
    except ValueError:
        try:
            # If decimal conversion fails, try converting as a hexadecimal integer
            return int(value, 16)
        except ValueError:
            # If both fail, raise an error
            raise ValueError(f"Invalid value: '{value}' is not a valid integer or hexadecimal number.")

def to_hex_str(value):
    """
    Converts an integer or hexadecimal string to hexadecimal string.
    :param value: Integer or hex string
    :return: Hexadecimal string with even number of digits and without 0x at the beginning
    """
    res = hex(int_or_hex(value))
    if len(res) % 2 == 1:
        res = res.replace('0x', '0') # Add one 0 digit to the beginning because it has to have even number of digits
    else:
        res = res.replace('0x', '')
    return res

def is_integer(s):
    try:
        int(s)
        return True
    except ValueError:
        return False

def is_hexadecimal(s):
    try:
        int(s, 16)
        return True
    except ValueError:
        return False


def hexdump(address: int,  data: bytes, bytes_per_line=16):
    """
    Generates a formatted hex dump of the given bytes data.

    address (int): Start address for data.
    data (bytes): The bytes object to dump.
    bytes_per_line (int): The number of bytes to display per line.
    """

    def to_printable_ascii(byte):
        """Converts a byte to its printable ASCII character or a dot."""
        return chr(byte) if 32 <= byte <= 126 else "."

    offset = 0
    while offset < len(data):
        chunk = data[offset: offset + bytes_per_line]

        # Format hex values
        hex_values = " ".join(f"{byte:02x}" for byte in chunk)

        # Format ASCII values
        ascii_values = "".join(to_printable_ascii(byte) for byte in chunk)

        # Print the formatted line
        address = address + offset
        print(f"0x{address:06x} {hex_values:<{bytes_per_line * 3 - 1}} |{ascii_values}|")
        offset += bytes_per_line

def calc_checksum(data):
    summary = 0
    for byte in data:
        summary += byte
    return (-summary) & 0xFF