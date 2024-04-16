#as: -EL -mdialect=pseudoc
#objdump: -dr -M hex,pseudoc
#source: mem-pseudoc.s
#name: eBPF MEM instructions, modulus lddw, pseudo-c syntax

.*: +file format .*bpf.*

Disassembly of section .text:

0+ <.text>:
   0:	20 00 00 00 ef be 00 00 	r0=\*\(u32\*\)skb\[0xbeef\]
   8:	28 00 00 00 ef be 00 00 	r0=\*\(u16\*\)skb\[0xbeef\]
  10:	30 00 00 00 ef be 00 00 	r0=\*\(u8\*\)skb\[0xbeef\]
  18:	38 00 00 00 ef be 00 00 	r0=\*\(u64\*\)skb\[0xbeef\]
  20:	40 30 00 00 ef be 00 00 	r0=\*\(u32\*\)skb\[r3\+0xbeef\]
  28:	48 50 00 00 ef be 00 00 	r0=\*\(u16\*\)skb\[r5\+0xbeef\]
  30:	50 70 00 00 ef be 00 00 	r0=\*\(u8\*\)skb\[r7\+0xbeef\]
  38:	58 90 00 00 ef be 00 00 	r0=\*\(u64\*\)skb\[r9\+0xbeef\]
  40:	61 12 ef 7e 00 00 00 00 	r2=\*\(u32\*\)\(r1\+0x7eef\)
  48:	69 12 ef 7e 00 00 00 00 	r2=\*\(u16\*\)\(r1\+0x7eef\)
  50:	71 12 ef 7e 00 00 00 00 	r2=\*\(u8\*\)\(r1\+0x7eef\)
  58:	79 12 fe ff 00 00 00 00 	r2=\*\(u64\*\)\(r1\+0xfffe\)
  60:	63 21 ef 7e 00 00 00 00 	\*\(u32\*\)\(r1\+0x7eef\)=r2
  68:	6b 21 ef 7e 00 00 00 00 	\*\(u16\*\)\(r1\+0x7eef\)=r2
  70:	73 21 ef 7e 00 00 00 00 	\*\(u8\*\)\(r1\+0x7eef\)=r2
  78:	7b 21 fe ff 00 00 00 00 	\*\(u64\*\)\(r1\+0xfffe\)=r2
  80:	72 01 ef 7e 44 33 22 11 	\*\(u8\*\)\(r1\+0x7eef\)=0x11223344
  88:	6a 01 ef 7e 44 33 22 11 	\*\(u16\*\)\(r1\+0x7eef\)=0x11223344
  90:	62 01 ef 7e 44 33 22 11 	\*\(u32\*\)\(r1\+0x7eef\)=0x11223344
  98:	7a 01 fe ff 44 33 22 11 	\*\(u64\*\)\(r1\+0xfffe\)=0x11223344
  a0:	81 12 ef 7e 00 00 00 00 	r2=\*\(s32\*\)\(r1\+0x7eef\)
  a8:	89 12 ef 7e 00 00 00 00 	r2=\*\(s16\*\)\(r1\+0x7eef\)
  b0:	91 12 ef 7e 00 00 00 00 	r2=\*\(s8\*\)\(r1\+0x7eef\)
  b8:	99 12 ef 7e 00 00 00 00 	r2=\*\(s64\*\)\(r1\+0x7eef\)
  c0:	58 50 00 00 00 00 00 00 	r0=\*\(u64\*\)skb\[r5\+0x0\]
  c8:	61 12 00 00 00 00 00 00 	r2=\*\(u32\*\)\(r1\+0x0\)
