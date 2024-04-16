#as:
#objdump: -dw -Mintel
#name: x86_64 APX_F pushp popp insns (Intel disassembly)
#source: x86-64-apx-pushp-popp.s

.*: +file format .*

Disassembly of section \.text:

0+ <_start>:
\s*[a-f0-9]+:\s*d5 08 50[	 ]+pushp  rax
\s*[a-f0-9]+:\s*d5 19 57[	 ]+pushp  r31
\s*[a-f0-9]+:\s*d5 08 58[	 ]+popp   rax
\s*[a-f0-9]+:\s*d5 19 5f[	 ]+popp   r31
#pass
