import os
import string
import serial
import sys

import serial.serialutil
from zdi.cmds.cmd import Cmd
from zdi.cmds.cmd import ErrorCode
from tqdm import tqdm

from zdi.utils import calc_checksum

if sys.stdout.isatty():
    cbWhite = "\x1b[1;37m"
    cbRed = "\x1b[1;31m"
    cbGreen = "\x1b[1;32m"
    cbYellow = "\x1b[1;33m"
    cReset = "\x1b[0m"
else:
    cbWhite = ""
    cbRed = ""
    cbGreen = ""
    cbYellow = ""
    cReset = ""

class IcdComm(object):

    _translate_str_1 = "".join(
        [(chr(x) in string.printable) and chr(x) or "." for x in range(256)]
    )
    _translate_str = bytes(_translate_str_1, "ascii")

    PACKET_SIZE = 1024 # TODO Change packet size from 1024 to 2048 or 4096 bytes

    def __init__(
        self,
        operation: str,
        device: str,
        verbosity=0,
        reuse_serial=None,
    ):
        self._operation = operation
        self._device = device
        self._serial = reuse_serial
        self._file_size = 0
        self._error_code = ErrorCode.ERR_SUCCESS
        self._verbosity = verbosity

    def init_programmer(self):
        if self._serial is None:
            self._serial = serial.Serial(port=self._device, timeout=5, write_timeout=5)

        self._serial.flushInput()
        self._serial.flushOutput()

    def serial(self):
        return self._serial

    def send_data(self, data):
        """
        Send command without response message
        :param data:
        :return: True on success, False on failure
        """
        try:
            self.init_programmer()
        except serial.SerialException:
            if self._verbosity > 0:
                print(f"{cbRed}Failure{cReset}: Device not found, or communication failure")
            return False

        self._serial.write(data)
        return True

    def send_data_with_response_err(self, data, return_data_size):
        """
        Send command and receive fixed size message or two bytes error response
        :param data:
        :param return_data_size:
        :return: True, Data on success, False, None on failure
        """
        try:
            self.init_programmer()
        except serial.SerialException:
            if self._verbosity > 0:
                print(f"{cbRed}Failure{cReset}: Device not found, or communication failure")
            return False, None

        self._serial.write(data)
        rd_response = self._serial.read(1)  # Read only one byte, the message type

        if rd_response == Cmd.ERROR:
            err_code = self._serial.read(1)[0]
            print(f"{cbRed}Failure{cReset}: {ErrorCode.get_description(err_code)} ({err_code}).")
            return False, None
        elif rd_response[0] != data[0]: # Check message type
            print(f"{cbRed}Failure{cReset}: Unexpected response message received: {rd_response[0]}")
            return False, None

        return True, self._serial.read(return_data_size - 1)

    def send_data_with_response(self, data, return_data_size):
        """
        Send command and receive fixed size message
        :param data:
        :param return_data_size:
        :return: True, Data on success, False, None on failure
        """
        try:
            self.init_programmer()
        except serial.SerialException:
            if self._verbosity > 0:
                print(f"{cbRed}Failure{cReset}: Device not found, or communication failure")
            return False, None

        self._serial.write(data)
        return True, self._serial.read(return_data_size)

    def send_data_with_var_response_crc(self, data):
        """
        Send command and receive variable size message
        :param data:
        :return: True, CRC8 value, Size, Data on success, False, 0, 0, None on failure
        """
        status, rd_response = self.send_data_with_response(data, 1) # Read only one byte, the message type

        if status:
            if rd_response == Cmd.ERROR:
                err_code = self._serial.read(1)[0]
                print(f"{cbRed}Failure{cReset}: {ErrorCode.get_description(err_code)} ({err_code}).")
                return False, 0, 0, None
            elif rd_response[0] != data[0]:
                print(f"{cbRed}Failure{cReset}: Unexpected response message received: {rd_response[0]}")
                return False, 0, 0, None

            # Read actual data
            tmp_data = self._serial.read(3) # Read data size, two bytes long
            rd_size = int.from_bytes(tmp_data[0:1], byteorder='little')
            checksum = tmp_data[2]
            return True, checksum, rd_size, self._serial.read(rd_size)

        return False, 0, 0, None

    def send_data_with_var_response(self, data):
        """
        Send command and receive variable size message
        :param data:
        :return: True, Size, Data on success, False, 0, None on failure
        """
        status, rd_response = self.send_data_with_response(data, 1) # Read only one byte, the message type

        if status:
            if rd_response == Cmd.ERROR:
                err_code = self._serial.read(1)[0]
                print(f"{cbRed}Failure{cReset}: {ErrorCode.get_description(err_code)} ({err_code}).")
                return False, 0, None
            elif rd_response[0] != data[0]:
                print(f"{cbRed}Failure{cReset}: Unexpected response message received: {rd_response[0]}")
                return False, 0, None

            # Read actual data
            tmp_data = self._serial.read(2) # Read data size, two bytes long
            rd_size = int.from_bytes(tmp_data[0:1], byteorder='little')
            return True, rd_size, self._serial.read(rd_size)

        return False, 0, None

    def send_file(self, address, fd) -> ErrorCode:
        self._file_size = os.fstat(fd.fileno()).st_size
        bytes_written = 0
        progress = None
        if self._verbosity > 0:
            progress = tqdm(
                total = self._file_size,
                desc = f"{cbWhite}%-10s{cReset}" % self._operation,
                unit = "b",
            )

        try:
            while bytes_written < self._file_size:
                file_data = fd.read(self.PACKET_SIZE) # Read a file section
                length = len(file_data)

                # Read a response message
                status, wr_response = self.send_data_with_response(
                    Cmd.WRITE + address.to_bytes(3, 'little') + length.to_bytes(2, byteorder='little') + file_data, 2)

                if status:
                    if wr_response[0] == 0 and self._verbosity > 0:
                        self._error_code = wr_response[1]
                        #print(f"{cbRed}Failure{cReset}: {self._error_code}")
                        break

                    #wr_size = int.from_bytes(wr_response[1:2], byteorder='little')
                    wr_checksum = wr_response[1]
                    checksum = calc_checksum(file_data)
                    if checksum != wr_checksum:
                        print(f"{cbRed}Failure{cReset}: Checksum error ({checksum}/{wr_checksum})")
                        self._error_code = ErrorCode.ERR_CHECKSUM
                        break

                    if self._verbosity > 0:
                        progress.update(n=length)

                bytes_written += length
                address += len(file_data) # Next packet address

            if self._verbosity > 1:
                progress.close()

            return self._error_code
        except KeyboardInterrupt:
            return ErrorCode.ERR_KEY_INTERRUPT

    def receive_file(self, address, length, fd):
        self._file_size = length
        bytes_read = 0
        progress = None
        if self._verbosity > 0:
            progress = tqdm(
                total = self._file_size,
                desc = f"{cbWhite}%-10s{cReset}" % self._operation,
                unit = "b",
            )

        try:
            while bytes_read < self._file_size:
                if bytes_read + self.PACKET_SIZE > self._file_size:
                    length = self._file_size - bytes_read
                else:
                    length = self.PACKET_SIZE

                status, rd_checksum, rd_size, rd_data = self.send_data_with_var_response_crc(Cmd.READ + address.to_bytes(3, 'little') + length.to_bytes(2, byteorder='little'))

                if status:
                    if length != rd_size:
                        self._error_code = ErrorCode.ERR_DATA_SIZE
                        break
                else:
                    self._error_code = ErrorCode.ERR_UNKNOWN
                    break

                checksum = calc_checksum(rd_data)
                if checksum != rd_checksum:
                    print(f"{cbRed}Failure{cReset}: Checksum error ({checksum}/{rd_checksum})")
                    self._error_code = ErrorCode.ERR_CHECKSUM
                    break

                if self._verbosity > 0:
                    progress.update(n=length)

                fd.write(rd_data)
                bytes_read += length
                address += length

            if self._verbosity > 1:
                progress.close()

            return self._error_code
        except KeyboardInterrupt:
            return ErrorCode.ERR_KEY_INTERRUPT