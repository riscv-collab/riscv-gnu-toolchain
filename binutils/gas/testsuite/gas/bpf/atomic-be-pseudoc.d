#as: -EB -mdialect=pseudoc
#objdump: -dr -M hex,pseudoc
#source: atomic-pseudoc.s
#name: eBPF atomic instructions, pseudoc syntax, big endian

.*: +file format .*bpf.*

Disassembly of section .text:

0+ <.text>:
   0:	db 12 1e ef 00 00 00 00 	lock \*\(u64\*\)\(r1\+0x1eef\)\+=r2
   8:	c3 12 1e ef 00 00 00 00 	lock \*\(u32\*\)\(r1\+0x1eef\)\+=w2
  10:	db 12 1e ef 00 00 00 00 	lock \*\(u64\*\)\(r1\+0x1eef\)\+=r2
  18:	c3 12 1e ef 00 00 00 00 	lock \*\(u32\*\)\(r1\+0x1eef\)\+=w2
  20:	db 12 1e ef 00 00 00 50 	lock \*\(u64\*\)\(r1\+0x1eef\)\&=r2
  28:	c3 12 1e ef 00 00 00 50 	lock \*\(u32\*\)\(r1\+0x1eef\)\&=w2
  30:	db 12 1e ef 00 00 00 40 	lock \*\(u64\*\)\(r1\+0x1eef\)\|=r2
  38:	c3 12 1e ef 00 00 00 40 	lock \*\(u32\*\)\(r1\+0x1eef\)\|=w2
  40:	db 12 1e ef 00 00 00 a0 	lock \*\(u64\*\)\(r1\+0x1eef\)\^=r2
  48:	c3 12 1e ef 00 00 00 a0 	lock \*\(u32\*\)\(r1\+0x1eef\)\^=w2
  50:	db 12 1e ef 00 00 00 01 	r2=atomic_fetch_add\(\(u64\*\)\(r1\+0x1eef\),r2\)
  58:	c3 12 1e ef 00 00 00 01 	w2=atomic_fetch_add\(\(u32\*\)\(r1\+0x1eef\),w2\)
  60:	db 12 1e ef 00 00 00 51 	r2=atomic_fetch_and\(\(u64\*\)\(r1\+0x1eef\),r2\)
  68:	c3 12 1e ef 00 00 00 51 	w2=atomic_fetch_and\(\(u32\*\)\(r1\+0x1eef\),w2\)
  70:	db 12 1e ef 00 00 00 41 	r2=atomic_fetch_or\(\(u64\*\)\(r1\+0x1eef\),r2\)
  78:	c3 12 1e ef 00 00 00 41 	w2=atomic_fetch_or\(\(u32\*\)\(r1\+0x1eef\),w2\)
  80:	db 12 1e ef 00 00 00 a1 	r2=atomic_fetch_xor\(\(u64\*\)\(r1\+0x1eef\),r2\)
  88:	c3 12 1e ef 00 00 00 a1 	w2=atomic_fetch_xor\(\(u32\*\)\(r1\+0x1eef\),w2\)
  90:	db 12 00 04 00 00 00 f1 	r0=cmpxchg_64\(r1\+0x4,r0,r2\)
  98:	c3 23 00 04 00 00 00 f1 	w0=cmpxchg32_32\(r2\+0x4,w0,w3\)
  a0:	db 12 00 08 00 00 00 e1 	r2=xchg_64\(r1\+0x8,r2\)
  a8:	c3 13 00 08 00 00 00 e1 	w3=xchg32_32\(r1\+0x8,w3\)
