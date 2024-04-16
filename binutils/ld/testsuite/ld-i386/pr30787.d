#as: --32
#ld: -melf_i386 -shared --no-warn-rwx-segments -T pr30787.t
#readelf: -d --wide

#...
 0x0+2 \(PLTRELSZ\) +8 \(bytes\)
#pass
