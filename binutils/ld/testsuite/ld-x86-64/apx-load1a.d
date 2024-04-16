#source: apx-load1.s
#as: --64 -mrelax-relocations=yes
#ld: -melf_x86_64 -z max-page-size=0x200000 -z noseparate-code
#objdump: -dw --sym

.*: +file format .*

SYMBOL TABLE:
#...
0+6001d0 l     O .data	0+1 bar
#...
0+6001d1 g     O .data	0+1 foo
#...

Disassembly of section .text:

0+4000b0 <_start>:
 +[a-f0-9]+:	d5 10 81 d0 d0 01 60 00 	adc    \$0x6001d0,%r16d
 +[a-f0-9]+:	d5 10 81 c1 d0 01 60 00 	add    \$0x6001d0,%r17d
 +[a-f0-9]+:	d5 10 81 e2 d0 01 60 00 	and    \$0x6001d0,%r18d
 +[a-f0-9]+:	d5 10 81 fb d0 01 60 00 	cmp    \$0x6001d0,%r19d
 +[a-f0-9]+:	d5 10 81 cc d0 01 60 00 	or     \$0x6001d0,%r20d
 +[a-f0-9]+:	d5 10 81 dd d0 01 60 00 	sbb    \$0x6001d0,%r21d
 +[a-f0-9]+:	d5 10 81 ee d0 01 60 00 	sub    \$0x6001d0,%r22d
 +[a-f0-9]+:	d5 10 81 f7 d0 01 60 00 	xor    \$0x6001d0,%r23d
 +[a-f0-9]+:	d5 11 f7 c0 d0 01 60 00 	test   \$0x6001d0,%r24d
 +[a-f0-9]+:	d5 18 81 d0 d0 01 60 00 	adc    \$0x6001d0,%r16
 +[a-f0-9]+:	d5 18 81 c1 d0 01 60 00 	add    \$0x6001d0,%r17
 +[a-f0-9]+:	d5 18 81 e2 d0 01 60 00 	and    \$0x6001d0,%r18
 +[a-f0-9]+:	d5 18 81 fb d0 01 60 00 	cmp    \$0x6001d0,%r19
 +[a-f0-9]+:	d5 18 81 cc d0 01 60 00 	or     \$0x6001d0,%r20
 +[a-f0-9]+:	d5 18 81 dd d0 01 60 00 	sbb    \$0x6001d0,%r21
 +[a-f0-9]+:	d5 18 81 ee d0 01 60 00 	sub    \$0x6001d0,%r22
 +[a-f0-9]+:	d5 18 81 f7 d0 01 60 00 	xor    \$0x6001d0,%r23
 +[a-f0-9]+:	d5 19 f7 c0 d0 01 60 00 	test   \$0x6001d0,%r24
 +[a-f0-9]+:	d5 10 81 d0 d1 01 60 00 	adc    \$0x6001d1,%r16d
 +[a-f0-9]+:	d5 10 81 c1 d1 01 60 00 	add    \$0x6001d1,%r17d
 +[a-f0-9]+:	d5 10 81 e2 d1 01 60 00 	and    \$0x6001d1,%r18d
 +[a-f0-9]+:	d5 10 81 fb d1 01 60 00 	cmp    \$0x6001d1,%r19d
 +[a-f0-9]+:	d5 10 81 cc d1 01 60 00 	or     \$0x6001d1,%r20d
 +[a-f0-9]+:	d5 10 81 dd d1 01 60 00 	sbb    \$0x6001d1,%r21d
 +[a-f0-9]+:	d5 10 81 ee d1 01 60 00 	sub    \$0x6001d1,%r22d
 +[a-f0-9]+:	d5 10 81 f7 d1 01 60 00 	xor    \$0x6001d1,%r23d
 +[a-f0-9]+:	d5 11 f7 c0 d1 01 60 00 	test   \$0x6001d1,%r24d
 +[a-f0-9]+:	d5 18 81 d0 d1 01 60 00 	adc    \$0x6001d1,%r16
 +[a-f0-9]+:	d5 18 81 c1 d1 01 60 00 	add    \$0x6001d1,%r17
 +[a-f0-9]+:	d5 18 81 e2 d1 01 60 00 	and    \$0x6001d1,%r18
 +[a-f0-9]+:	d5 18 81 fb d1 01 60 00 	cmp    \$0x6001d1,%r19
 +[a-f0-9]+:	d5 18 81 cc d1 01 60 00 	or     \$0x6001d1,%r20
 +[a-f0-9]+:	d5 18 81 dd d1 01 60 00 	sbb    \$0x6001d1,%r21
 +[a-f0-9]+:	d5 18 81 ee d1 01 60 00 	sub    \$0x6001d1,%r22
 +[a-f0-9]+:	d5 18 81 f7 d1 01 60 00 	xor    \$0x6001d1,%r23
 +[a-f0-9]+:	d5 19 f7 c0 d1 01 60 00 	test   \$0x6001d1,%r24
#pass
