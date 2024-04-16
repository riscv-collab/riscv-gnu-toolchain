#as: -EL -mdialect=normal
#objdump: -dr -M hex
#source: atomic.s
#name: eBPF atomic instructions, normal syntax

.*: +file format .*bpf.*

Disassembly of section .text:

0+ <.text>:
   0:	db 21 ef 1e 00 00 00 00 	aadd \[%r1\+0x1eef\],%r2
   8:	c3 21 ef 1e 00 00 00 00 	aadd32 \[%r1\+0x1eef\],%r2
  10:	db 21 ef 1e 50 00 00 00 	aand \[%r1\+0x1eef\],%r2
  18:	c3 21 ef 1e 50 00 00 00 	aand32 \[%r1\+0x1eef\],%r2
  20:	db 21 ef 1e 40 00 00 00 	aor \[%r1\+0x1eef\],%r2
  28:	c3 21 ef 1e 40 00 00 00 	aor32 \[%r1\+0x1eef\],%r2
  30:	db 21 ef 1e a0 00 00 00 	axor \[%r1\+0x1eef\],%r2
  38:	c3 21 ef 1e a0 00 00 00 	axor32 \[%r1\+0x1eef\],%r2
  40:	db 21 ef 1e 01 00 00 00 	afadd \[%r1\+0x1eef\],%r2
  48:	c3 21 ef 1e 01 00 00 00 	afadd32 \[%r1\+0x1eef\],%r2
  50:	db 21 ef 1e 51 00 00 00 	afand \[%r1\+0x1eef\],%r2
  58:	c3 21 ef 1e 51 00 00 00 	afand32 \[%r1\+0x1eef\],%r2
  60:	db 21 ef 1e 41 00 00 00 	afor \[%r1\+0x1eef\],%r2
  68:	c3 21 ef 1e 41 00 00 00 	afor32 \[%r1\+0x1eef\],%r2
  70:	db 21 ef 1e a1 00 00 00 	afxor \[%r1\+0x1eef\],%r2
  78:	c3 21 ef 1e a1 00 00 00 	afxor32 \[%r1\+0x1eef\],%r2
  80:	db 21 04 00 f1 00 00 00 	acmp \[%r1\+0x4\],%r2
  88:	c3 32 04 00 f1 00 00 00 	acmp32 \[%r2\+0x4\],%r3
  90:	db 21 08 00 e1 00 00 00 	axchg \[%r1\+0x8\],%r2
  98:	c3 31 08 00 e1 00 00 00 	axchg32 \[%r1\+0x8\],%r3
