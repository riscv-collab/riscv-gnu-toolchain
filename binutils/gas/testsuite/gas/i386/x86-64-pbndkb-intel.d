#objdump: -dw -Mintel
#name: x86_64 PBNDKB insns (Intel disassembly)
#source: x86-64-pbndkb.s

.*: +file format .*

Disassembly of section \.text:

0+ <_start>:
\s*[a-f0-9]+:\s*0f 01 c7\s+pbndkb
\s*[a-f0-9]+:\s*0f 01 c7\s+pbndkb
#pass
