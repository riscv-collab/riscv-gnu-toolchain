#source: mark-plt-1.s
#as: --64
#ld: -melf_x86_64 -shared -z mark-plt -z ibtplt
#readelf: -drW

#...
 0x0000000070000000 \(DT_X86_64_PLT\)      0x1020
 0x0000000070000001 \(DT_X86_64_PLTSZ\)    0x10
 0x0000000070000003 \(DT_X86_64_PLTENT\)   0x10
#...
[0-9a-f ]+R_X86_64_JUMP_SLOT +0+ +bar \+ 1024
#pass
