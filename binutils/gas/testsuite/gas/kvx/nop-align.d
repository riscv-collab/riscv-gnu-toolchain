#as: -march=kv3-1
#objdump: -dr --prefix-addresses

.*: +file format .*kvx.*

Disassembly of section .text:
[0-9a-f]+ <f> nop
[0-9a-f]+ <f\+0x4> nop;;

[0-9a-f]+ <f\+0x8> addw \$r0 = \$r1, \$r0;;

[0-9a-f]+ <f\+0xc> \*\*\* invalid opcode \*\*\*

[0-9a-f]+ <f\+0x10> ret
[0-9a-f]+ <f\+0x14> addw \$r0 = \$r2, \$r0;;

[0-9a-f]+ <g> addw \$r0 = \$r1, \$r0
[0-9a-f]+ <g\+0x4> addw \$r0 = \$r2, \$r0
[0-9a-f]+ <g\+0x8> addw \$r0 = \$r2, \$r0;;

[0-9a-f]+ <g\+0xc> nop
[0-9a-f]+ <g\+0x10> nop
[0-9a-f]+ <g\+0x14> nop
[0-9a-f]+ <g\+0x18> nop;;

[0-9a-f]+ <g\+0x1c> nop
[0-9a-f]+ <g\+0x20> nop
[0-9a-f]+ <g\+0x24> nop;;

[0-9a-f]+ <g\+0x28> ret
[0-9a-f]+ <g\+0x2c> addw \$r0 = \$r2, \$r0;;
