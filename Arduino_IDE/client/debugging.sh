#!/bin/bash

# manual commands for openOCD and arm-none-eabi-gdb for external use
openocd -s . -f /usr/share/openocd/scripts/board/ek-lm4f120xl.cfg -c gdb_port 50000 &
~/.platformio/packages/toolchain-gccarmnoneeabi/bin/arm-none-eabi-gdb -ex "target remote :50000" --interpreter=mi2 .pioenvs/lptm4c1230c3pm/firmware.elf

# gdbgui is a web debugger, vey userful
gdbgui -g ~/.platformio/packages/toolchain-gccarmnoneeabi/bin/arm-none-eabi-gdb --gdb-args '--command debugging.gdb' .pioenvs/lptm4c1230c3pm/firmware.elf
