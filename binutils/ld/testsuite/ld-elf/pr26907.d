#ld: -T pr26907.ld --no-warn-rwx-segments
#readelf: -lW
#xfail: dlx-*-* ft32-*-* h8300-*-* ip2k-*-* m32r*-*-elf* m32r*-*-rtems*
#xfail: moxie-*-* msp430-*-* mt-*-* pru*-*-* visium-*-*

#failif
#...
 +LOAD +0x[0-9a-f]+ 0x[0-9a-f]+ 0x[0-9a-f]+ 0x0+ 0x0+ .*
#...
