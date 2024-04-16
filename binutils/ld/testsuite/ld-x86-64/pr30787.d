#as: --64
#ld: -melf_x86_64 -shared --no-warn-rwx-segments -T pr30787.t
#readelf: -d --wide

#...
 0x0+2 \(PLTRELSZ\) +24 \(bytes\)
#pass
