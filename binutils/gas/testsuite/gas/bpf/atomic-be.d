#as: -EB -mdialect=normal
#source: atomic.s
#objdump: -dr -M hex
#name: eBPF atomic instructions, big endian

.*: +file format .*bpf.*

Disassembly of section .text:

0+ <.text>:
   0:	db 12 1e ef 00 00 00 00 	aadd \[%r1\+0x1eef\],%r2
   8:	c3 12 1e ef 00 00 00 00 	aadd32 \[%r1\+0x1eef\],%r2
  10:	db 12 1e ef 00 00 00 50 	aand \[%r1\+0x1eef\],%r2
  18:	c3 12 1e ef 00 00 00 50 	aand32 \[%r1\+0x1eef\],%r2
  20:	db 12 1e ef 00 00 00 40 	aor \[%r1\+0x1eef\],%r2
  28:	c3 12 1e ef 00 00 00 40 	aor32 \[%r1\+0x1eef\],%r2
  30:	db 12 1e ef 00 00 00 a0 	axor \[%r1\+0x1eef\],%r2
  38:	c3 12 1e ef 00 00 00 a0 	axor32 \[%r1\+0x1eef\],%r2
  40:	db 12 1e ef 00 00 00 01 	afadd \[%r1\+0x1eef\],%r2
  48:	c3 12 1e ef 00 00 00 01 	afadd32 \[%r1\+0x1eef\],%r2
  50:	db 12 1e ef 00 00 00 51 	afand \[%r1\+0x1eef\],%r2
  58:	c3 12 1e ef 00 00 00 51 	afand32 \[%r1\+0x1eef\],%r2
  60:	db 12 1e ef 00 00 00 41 	afor \[%r1\+0x1eef\],%r2
  68:	c3 12 1e ef 00 00 00 41 	afor32 \[%r1\+0x1eef\],%r2
  70:	db 12 1e ef 00 00 00 a1 	afxor \[%r1\+0x1eef\],%r2
  78:	c3 12 1e ef 00 00 00 a1 	afxor32 \[%r1\+0x1eef\],%r2
  80:	db 12 00 04 00 00 00 f1 	acmp \[%r1\+0x4\],%r2
  88:	c3 23 00 04 00 00 00 f1 	acmp32 \[%r2\+0x4\],%r3
  90:	db 12 00 08 00 00 00 e1 	axchg \[%r1\+0x8\],%r2
  98:	c3 13 00 08 00 00 00 e1 	axchg32 \[%r1\+0x8\],%r3
