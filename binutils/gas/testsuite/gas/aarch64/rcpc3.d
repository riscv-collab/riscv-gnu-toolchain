#name: RCPC3 GPR load/store
#as: -march=armv8.2-a+rcpc3
#objdump: -dr

.*:     file format .*

Disassembly of section \.text:

0+ <.*>:
   0:	d9411860 	ldiapp	x0, x1, \[x3\]
   4:	99411860 	ldiapp	w0, w1, \[x3\]
   8:	d9410860 	ldiapp	x0, x1, \[x3\], #16
   c:	99410860 	ldiapp	w0, w1, \[x3\], #8
  10:	d9011860 	stilp	x0, x1, \[x3\]
  14:	99011860 	stilp	w0, w1, \[x3\]
  18:	d9010860 	stilp	x0, x1, \[x3, #-16\]!
  1c:	99010860 	stilp	w0, w1, \[x3, #-8\]!
  20:	99c00841 	ldapr	w1, \[x2\], #4
  24:	d9c00841 	ldapr	x1, \[x2\], #8
  28:	99800841 	stlr	w1, \[x2, #-4\]!
  2c:	d9800841 	stlr	x1, \[x2, #-8\]!
