#as: -mthin-add-sub
#objdump: -Dr

.*:[    ]+file format .*


Disassembly of section .text:

00000000.* <.L1>:
[ 	]+...
[ 	]+0:[ 	]+R_LARCH_32_PCREL[ 	]+.L3
[ 	]+4:[ 	]+R_LARCH_ADD32[ 	]+.L3
[ 	]+4:[ 	]+R_LARCH_SUB32[ 	]+.L1

0*00000008[ 	]+<.L2>:
[ 	]+...
[ 	]+8:[ 	]+R_LARCH_64_PCREL[ 	]+.L3
[ 	]+10:[ 	]+R_LARCH_ADD64[ 	]+.L3
[ 	]+10:[ 	]+R_LARCH_SUB64[ 	]+.L2

Disassembly[ 	]+of[ 	]+section[ 	]+sx:

0*00000000[ 	]+<.L3>:
[ 	]+0:[ 	]+fffffff4[ 	]+.word[ 	]+0xfffffff4
[ 	]+4:[ 	]+fffffff4[ 	]+.word[ 	]+0xfffffff4
[ 	]+8:[ 	]+ffffffff[ 	]+.word[ 	]+0xffffffff

0*0000000c[ 	]+<.L4>:
[ 	]+...
[ 	]+c:[ 	]+R_LARCH_ADD32[ 	]+.L4
[ 	]+c:[ 	]+R_LARCH_SUB32[ 	]+.L5
[ 	]+10:[ 	]+R_LARCH_ADD64[ 	]+.L4
[ 	]+10:[ 	]+R_LARCH_SUB64[ 	]+.L5

Disassembly[ 	]+of[ 	]+section[ 	]+sy:

0*00000000[ 	]+<.L5>:
[ 	]+...
[ 	]+0:[ 	]+R_LARCH_32_PCREL[ 	]+.L1
[ 	]+4:[ 	]+R_LARCH_32_PCREL[ 	]+.L3\+0x4
[ 	]+8:[ 	]+R_LARCH_64_PCREL[ 	]+.L1\+0x8
[ 	]+10:[ 	]+R_LARCH_64_PCREL[ 	]+.L3\+0x10

Disassembly[ 	]+of[ 	]+section[ 	]+sz:

0*00000000[ 	]+<sz>:
[ 	]+0:[ 	]+00000000[ 	]+.word[ 	]+0x00000000
[ 	]+0:[ 	]+R_LARCH_ADD32[ 	]+.L1
[ 	]+0:[ 	]+R_LARCH_SUB32[ 	]+.L2
[ 	]+4:[ 	]+fffffff4[ 	]+.word[ 	]+0xfffffff4
[ 	]+...
[ 	]+8:[ 	]+R_LARCH_ADD32[ 	]+.L3
[ 	]+8:[ 	]+R_LARCH_SUB32[ 	]+.L5
[ 	]+c:[ 	]+R_LARCH_ADD64[ 	]+.L1
[ 	]+c:[ 	]+R_LARCH_SUB64[ 	]+.L2
[ 	]+14:[ 	]+fffffff4[ 	]+.word[ 	]+0xfffffff4
[ 	]+18:[ 	]+ffffffff[ 	]+.word[ 	]+0xffffffff
[ 	]+...
[ 	]+1c:[ 	]+R_LARCH_ADD64[ 	]+.L3
[ 	]+1c:[ 	]+R_LARCH_SUB64[ 	]+.L5
