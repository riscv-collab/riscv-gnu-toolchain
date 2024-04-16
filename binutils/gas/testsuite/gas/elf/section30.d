#as: --sectname-subst
#readelf: -SW
#name: --sectname-subst plus section attr/type inherting
# Targets setting NO_PSEUDO_DOT don't allow macros of certain names.
#notarget: m681*-*-* m68hc1*-*-* s12z-*-* spu-*-* xgate-*-* z80-*-*

#...
  \[..\] \.group +GROUP +[0-9a-f]+ [0-9a-f]+ 0+c 04 +[1-9][0-9]* +[1-9][0-9]* +4
  \[..\] \.text +PROGBITS +[0-9a-f]+ [0-9a-f]+ 0+0 00  AX  0   0 +[1-9][0-9]*
  \[..\] \.data +PROGBITS +[0-9a-f]+ [0-9a-f]+ 0+0 00  WA  0   0 +[1-9][0-9]*
  \[..\] \.bss +NOBITS +[0-9a-f]+ [0-9a-f]+ 0+0 00  WA  0   0 +[1-9][0-9]*
#...
  \[..\] \.text\.func1 +PROGBITS +[0-9a-f]+ [0-9a-f]+ [0-9a-f]+ 00  AX  0   0 +[1-9][0-9]*
  \[..\] \.text\.func2 +PROGBITS +[0-9a-f]+ [0-9a-f]+ [0-9a-f]+ 00  AX  0   0 +[1-9][0-9]*
  \[..\] \.data\.data1 +PROGBITS +[0-9a-f]+ [0-9a-f]+ 0+1 00  WA  0   0  1
#...
  \[..\] \.bss\.data2 +NOBITS +[0-9a-f]+ [0-9a-f]+ 0+2 00  WA  0   0  1
  \[..\] \.rodata +PROGBITS +[0-9a-f]+ [0-9a-f]+ 0+0 00   A  0   0  1
  \[..\] \.rodata\.data3 +PROGBITS +[0-9a-f]+ [0-9a-f]+ 0+3 00   A  0   0  1
  \[..\] \.rodata\.str1\.1 +PROGBITS +[0-9a-f]+ [0-9a-f]+ 0+0 01 AMS  0   0  1
  \[..\] \.rodata\.str1\.1\.str1 +PROGBITS +[0-9a-f]+ [0-9a-f]+ 0+8 01 AMS  0   0  1
  \[..\] \.rodata\.2 +PROGBITS +[0-9a-f]+ [0-9a-f]+ 0+0 00  AL  [1-9]   0  1
  \[..\] \.rodata\.2\.data4 +PROGBITS +[0-9a-f]+ [0-9a-f]+ 0+4 00  AL  [1-9]   0  1
  \[..\] \.bss\.data5 +NOBITS +[0-9a-f]+ [0-9a-f]+ 0+5 00  WA  0   0  1
  \[..\] \.rodata\.3 +PROGBITS +[0-9a-f]+ [0-9a-f]+ 0+0 00  AG  0   0  1
  \[..\] \.rodata\.3\.data6 +PROGBITS +[0-9a-f]+ [0-9a-f]+ 0+6 00  AG  0   0  1
  \[..\] \.bss\.data7 +NOBITS +[0-9a-f]+ [0-9a-f]+ 0+7 00  WA  0   0  1
#pass
