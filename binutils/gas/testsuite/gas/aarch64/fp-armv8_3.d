#objdump: -dr
#as: -march=armv8.3-a
#as: -march=armv8-a+jscvt

.*:     file .*

Disassembly of section \.text:

0+ <.*>:
   0:	1e7e0041 	fjcvtzs	w1, d2
   4:	1e7e00e7 	fjcvtzs	w7, d7
