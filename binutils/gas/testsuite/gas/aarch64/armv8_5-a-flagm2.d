#as: -march=armv8.5-a
#as: -march=armv8-a+flagm2
# objdump: -d

.*: .*


Disassembly of section \.text:

0+0 <func>:
.*:	d500403f 	xaflag
.*:	d500405f 	axflag
