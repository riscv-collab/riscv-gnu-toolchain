#as:
#objdump: -dw -Mintel
#name: x86_64 APX_F JMPABS insns (Intel disassembly)
#source: x86-64-apx-jmpabs.s

.*: +file format .*

Disassembly of section \.text:

0+ <_start>:
\s*[a-f0-9]+:\s*d5 00 a1 02 00 00 00 00 00 00 00[	 ]+jmpabs 0x2
#pass
