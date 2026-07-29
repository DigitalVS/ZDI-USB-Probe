#include <stdio.h>

#include "hardware/pio.h"
#include "cmd_version.h"

CmdVersion::CmdVersion(CmdId id, cbuf_handle_t cbuf) : Cmd(id) {
}

bool CmdVersion::execute() {
  return true;
}

ResponseBuf CmdVersion::getResponse() {
  // Create a response message
  Cmd::outBuffer[0] = VERSION; // Message type
  Cmd::outBuffer[1] = 0; // Major version number
  Cmd::outBuffer[2] = 2; // Minor version number
  Cmd::outBuffer[3] = 0; // Revision number

  return ResponseBuf { .startAddr = Cmd::outBuffer, .size = 4 };
}