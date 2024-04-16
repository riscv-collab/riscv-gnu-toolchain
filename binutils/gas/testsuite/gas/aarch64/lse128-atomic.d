#objdump: -dr
#as:-march=armv9-a+lse128

.*:     file format .*

Disassembly of section \.text:

0+ <.*>:
   0:	19211040 	ldclrp	x0, x1, \[x2\]
   4:	192313e2 	ldclrp	x2, x3, \[sp\]
   8:	19a11040 	ldclrpa	x0, x1, \[x2\]
   c:	19a313e2 	ldclrpa	x2, x3, \[sp\]
  10:	19e11040 	ldclrpal	x0, x1, \[x2\]
  14:	19e313e2 	ldclrpal	x2, x3, \[sp\]
  18:	19611040 	ldclrpl	x0, x1, \[x2\]
  1c:	196313e2 	ldclrpl	x2, x3, \[sp\]
  20:	19213040 	ldsetp	x0, x1, \[x2\]
  24:	192333e2 	ldsetp	x2, x3, \[sp\]
  28:	19a13040 	ldsetpa	x0, x1, \[x2\]
  2c:	19a333e2 	ldsetpa	x2, x3, \[sp\]
  30:	19e13040 	ldsetpal	x0, x1, \[x2\]
  34:	19e333e2 	ldsetpal	x2, x3, \[sp\]
  38:	19613040 	ldsetpl	x0, x1, \[x2\]
  3c:	196333e2 	ldsetpl	x2, x3, \[sp\]
  40:	19218040 	swpp	x0, x1, \[x2\]
  44:	192383e2 	swpp	x2, x3, \[sp\]
  48:	19a18040 	swppa	x0, x1, \[x2\]
  4c:	19a383e2 	swppa	x2, x3, \[sp\]
  50:	19e18040 	swppal	x0, x1, \[x2\]
  54:	19e383e2 	swppal	x2, x3, \[sp\]
  58:	19618040 	swppl	x0, x1, \[x2\]
  5c:	196383e2 	swppl	x2, x3, \[sp\]