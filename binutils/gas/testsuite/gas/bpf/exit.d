#as: -EL -mdialect=normal
#objdump: -dr -M hex
#name: eBPF EXIT instruction

.*: +file format .*bpf.*

Disassembly of section .text:

0+ <.text>:
   0:	95 00 00 00 00 00 00 00 	exit