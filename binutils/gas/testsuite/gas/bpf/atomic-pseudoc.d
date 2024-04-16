#as: -EL -mdialect=pseudoc
#objdump: -dr -M hex,pseudoc
#source: atomic-pseudoc.s
#name: eBPF atomic instructions, pseudoc syntax

.*: +file format .*bpf.*

Disassembly of section .text:

0+ <.text>:
   0:	db 21 ef 1e 00 00 00 00 	lock \*\(u64\*\)\(r1\+0x1eef\)\+=r2
   8:	c3 21 ef 1e 00 00 00 00 	lock \*\(u32\*\)\(r1\+0x1eef\)\+=w2
  10:	db 21 ef 1e 00 00 00 00 	lock \*\(u64\*\)\(r1\+0x1eef\)\+=r2
  18:	c3 21 ef 1e 00 00 00 00 	lock \*\(u32\*\)\(r1\+0x1eef\)\+=w2
  20:	db 21 ef 1e 50 00 00 00 	lock \*\(u64\*\)\(r1\+0x1eef\)\&=r2
  28:	c3 21 ef 1e 50 00 00 00 	lock \*\(u32\*\)\(r1\+0x1eef\)\&=w2
  30:	db 21 ef 1e 40 00 00 00 	lock \*\(u64\*\)\(r1\+0x1eef\)\|=r2
  38:	c3 21 ef 1e 40 00 00 00 	lock \*\(u32\*\)\(r1\+0x1eef\)\|=w2
  40:	db 21 ef 1e a0 00 00 00 	lock \*\(u64\*\)\(r1\+0x1eef\)\^=r2
  48:	c3 21 ef 1e a0 00 00 00 	lock \*\(u32\*\)\(r1\+0x1eef\)\^=w2
  50:	db 21 ef 1e 01 00 00 00 	r2=atomic_fetch_add\(\(u64\*\)\(r1\+0x1eef\),r2\)
  58:	c3 21 ef 1e 01 00 00 00 	w2=atomic_fetch_add\(\(u32\*\)\(r1\+0x1eef\),w2\)
  60:	db 21 ef 1e 51 00 00 00 	r2=atomic_fetch_and\(\(u64\*\)\(r1\+0x1eef\),r2\)
  68:	c3 21 ef 1e 51 00 00 00 	w2=atomic_fetch_and\(\(u32\*\)\(r1\+0x1eef\),w2\)
  70:	db 21 ef 1e 41 00 00 00 	r2=atomic_fetch_or\(\(u64\*\)\(r1\+0x1eef\),r2\)
  78:	c3 21 ef 1e 41 00 00 00 	w2=atomic_fetch_or\(\(u32\*\)\(r1\+0x1eef\),w2\)
  80:	db 21 ef 1e a1 00 00 00 	r2=atomic_fetch_xor\(\(u64\*\)\(r1\+0x1eef\),r2\)
  88:	c3 21 ef 1e a1 00 00 00 	w2=atomic_fetch_xor\(\(u32\*\)\(r1\+0x1eef\),w2\)
  90:	db 21 04 00 f1 00 00 00 	r0=cmpxchg_64\(r1\+0x4,r0,r2\)
  98:	c3 32 04 00 f1 00 00 00 	w0=cmpxchg32_32\(r2\+0x4,w0,w3\)
  a0:	db 21 08 00 e1 00 00 00 	r2=xchg_64\(r1\+0x8,r2\)
  a8:	c3 31 08 00 e1 00 00 00 	w3=xchg32_32\(r1\+0x8,w3\)
