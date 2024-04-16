# mach: crisv32
# requires: simoption --cris-900000xx
# sim: --cris-900000xx --memory-region 0x90000000,0x10
# xerror:
# output: /core/cris_900000xx: memory map 0:0x90000000..0x900000ff (256 bytes) overlaps 0:0x90000000..0x9000000f (16 bytes)\nQuit Simulator\n

; Check that I/O region overlap is detected.

 .include "nopv32t.ms"
