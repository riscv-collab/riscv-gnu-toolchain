#as: -EL -mdialect=normal
#source: atomic-v1.s
#objdump: -dr -M hex,v1
#name: eBPF atomic instructions, little endian

.*: +file format .*bpf.*

Disassembly of section .text:

0+ <.text>:
   0:	db 21 ef 1e 00 00 00 00 	xadddw \[%r1\+0x1eef\],%r2
   8:	c3 21 ef 1e 00 00 00 00 	xaddw \[%r1\+0x1eef\],%r2
