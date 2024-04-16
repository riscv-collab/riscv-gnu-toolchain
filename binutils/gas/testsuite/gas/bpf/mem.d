#as: -EL -mdialect=normal
#objdump: -dr -M hex
#source: mem.s
#name: eBPF MEM instructions, modulus lddw, normal syntax

.*: +file format .*bpf.*

Disassembly of section .text:

0+ <.text>:
   0:	20 00 00 00 ef be 00 00 	ldabsw 0xbeef
   8:	28 00 00 00 ef be 00 00 	ldabsh 0xbeef
  10:	30 00 00 00 ef be 00 00 	ldabsb 0xbeef
  18:	38 00 00 00 ef be 00 00 	ldabsdw 0xbeef
  20:	40 30 00 00 ef be 00 00 	ldindw %r3,0xbeef
  28:	48 50 00 00 ef be 00 00 	ldindh %r5,0xbeef
  30:	50 70 00 00 ef be 00 00 	ldindb %r7,0xbeef
  38:	58 90 00 00 ef be 00 00 	ldinddw %r9,0xbeef
  40:	61 12 ef 7e 00 00 00 00 	ldxw %r2,\[%r1\+0x7eef\]
  48:	69 12 ef 7e 00 00 00 00 	ldxh %r2,\[%r1\+0x7eef\]
  50:	71 12 ef 7e 00 00 00 00 	ldxb %r2,\[%r1\+0x7eef\]
  58:	79 12 fe ff 00 00 00 00 	ldxdw %r2,\[%r1\+0xfffe\]
  60:	63 21 ef 7e 00 00 00 00 	stxw \[%r1\+0x7eef\],%r2
  68:	6b 21 ef 7e 00 00 00 00 	stxh \[%r1\+0x7eef\],%r2
  70:	73 21 ef 7e 00 00 00 00 	stxb \[%r1\+0x7eef\],%r2
  78:	7b 21 fe ff 00 00 00 00 	stxdw \[%r1\+0xfffe\],%r2
  80:	72 01 ef 7e 44 33 22 11 	stb \[%r1\+0x7eef\],0x11223344
  88:	6a 01 ef 7e 44 33 22 11 	sth \[%r1\+0x7eef\],0x11223344
  90:	62 01 ef 7e 44 33 22 11 	stw \[%r1\+0x7eef\],0x11223344
  98:	7a 01 fe ff 44 33 22 11 	stdw \[%r1\+0xfffe\],0x11223344
  a0:	81 12 ef 7e 00 00 00 00 	ldxsw %r2,\[%r1\+0x7eef\]
  a8:	89 12 ef 7e 00 00 00 00 	ldxsh %r2,\[%r1\+0x7eef\]
  b0:	91 12 ef 7e 00 00 00 00 	ldxsb %r2,\[%r1\+0x7eef\]
  b8:	99 12 ef 7e 00 00 00 00 	ldxsdw %r2,\[%r1\+0x7eef\]
  c0:	79 12 00 00 00 00 00 00 	ldxdw %r2,\[%r1\+0x0\]
  c8:	40 30 00 00 00 00 00 00 	ldindw %r3,0x0
