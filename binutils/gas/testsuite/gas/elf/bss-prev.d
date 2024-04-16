#name: .bss / .previous interaction
#as: --no-pad-sections
#readelf: -S --wide

There are [0-9]+ section headers, starting at offset 0x[0-9a-f]+:

Section Headers:
 +\[Nr\] Name +Type +Addr(ess|) +Off +Size .*
#...
 *\[ [1-9]\] *\.text +PROGBITS +0*0 +0[0-9a-f]* 0+ .*
 *\[ [1-9]\] *\.data +PROGBITS +0*0 +0[0-9a-f]* 0*1 .*
 *\[ [1-9]\] *\.bss +NOBITS +0*0 +0[0-9a-f]* 0*1 .*
#pass
