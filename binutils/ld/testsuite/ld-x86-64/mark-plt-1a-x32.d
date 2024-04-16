#source: mark-plt-1.s
#as: --x32
#ld: -melf32_x86_64 -shared -z mark-plt
#readelf: -drW

#...
 0x70000000 \(DT_X86_64_PLT\)              0x1000
 0x70000001 \(DT_X86_64_PLTSZ\)            0x20
 0x70000003 \(DT_X86_64_PLTENT\)           0x10
#...
[0-9a-f ]+R_X86_64_JUMP_SLOT +0+ +bar \+ 1010
#pass
