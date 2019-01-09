target remote localhost:3333
set arm abi APCS
monitor reset halt
file .pioenvs/lptm4c1230c3pm/firmware.elf
load
monitor reset