#name: RCPC3 fp load/store
#as: -march=armv8.2-a+rcpc3
#objdump: -dr

.*:     file format .*

Disassembly of section \.text:

0+ <.*>:
   0:	0d4187e1 	ldap1	{v1.d}\[0], \[sp]
   4:	4d418422 	ldap1	{v2.d}\[1], \[x1]
   8:	0d018443 	stl1	{v3.d}\[0], \[x2]
   c:	4d018464 	stl1	{v4.d}\[1], \[x3]
  10:	1d400be1 	ldapur	b1, \[sp\]
  14:	1d500be1 	ldapur	b1, \[sp, #-256\]
  18:	1d4ffbe1 	ldapur	b1, \[sp, #255\]
  1c:	5d400842 	ldapur	h2, \[x2\]
  20:	9d400863 	ldapur	s3, \[x3\]
  24:	dd400884 	ldapur	d4, \[x4\]
  28:	1dc00be1 	ldapur	q1, \[sp\]
  2c:	1d000be1 	stlur	b1, \[sp\]
  30:	1d100be1 	stlur	b1, \[sp, #-256\]
  34:	1d0ffbe1 	stlur	b1, \[sp, #255\]
  38:	9d000863 	stlur	s3, \[x3\]
  3c:	dd000884 	stlur	d4, \[x4\]
  40:	1d800be1 	stlur	q1, \[sp\]
