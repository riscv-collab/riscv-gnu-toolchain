#name: Check indirect call stub to BTI stub relaxation.
#source: bti-far-3a.s
#source: bti-far-3b.s
#source: bti-far-3c.s
#ld: -shared --hash-style=sysv -T bti-far-3.ld
#objdump: -dr

[^:]*: *file format elf64-.*aarch64


Disassembly of section \.plt:

0000000000020000 <\.plt>:
   20000:	d503245f 	bti	c
   20004:	a9bf7bf0 	stp	x16, x30, \[sp, #-16\]!
   20008:	900fff10 	adrp	x16, 20000000 <_GLOBAL_OFFSET_TABLE_>
   2000c:	f9400e11 	ldr	x17, \[x16, #24\]
   20010:	91006210 	add	x16, x16, #0x18
   20014:	d61f0220 	br	x17
   20018:	d503201f 	nop
   2001c:	d503201f 	nop

0000000000020020 <extern_func@plt>:
   20020:	900fff10 	adrp	x16, 20000000 <_GLOBAL_OFFSET_TABLE_>
   20024:	f9401211 	ldr	x17, \[x16, #32\]
   20028:	91008210 	add	x16, x16, #0x20
   2002c:	d61f0220 	br	x17
   20030:	14000004 	b	20040 .*
   20034:	d503201f 	nop

0000000000020038 <__extern_func_bti_veneer>:
   20038:	d503245f 	bti	c
   2003c:	17fffff9 	b	20020 <extern_func@plt>

Disassembly of section \.text:

0000000000030000 <a_func>:
       30000:	15c00004 	b	7030010 <__b_func_veneer>
       30004:	17ffc007 	b	20020 <extern_func@plt>
	\.\.\.
     7030008:	1400000a 	b	7030030 .*
     703000c:	d503201f 	nop

0000000007030010 <__b_func_veneer>:
     7030010:	90040010 	adrp	x16, f030000 <b_func\+0x6ffffd0>
     7030014:	91018210 	add	x16, x16, #0x60
     7030018:	d61f0200 	br	x16
	\.\.\.

0000000007030028 <__a_func_bti_veneer>:
     7030028:	d503245f 	bti	c
     703002c:	163ffff5 	b	30000 <a_func>
	\.\.\.

0000000008030030 <b_func>:
     8030030:	15c0000a 	b	f030058 <__c_func_veneer>
     8030034:	15c00003 	b	f030040 <__a_func_veneer>
	\.\.\.
     f030038:	14000012 	b	f030080 .*
     f03003c:	d503201f 	nop

000000000f030040 <__a_func_veneer>:
     f030040:	90fc0010 	adrp	x16, 7030000 <a_func\+0x7000000>
     f030044:	9100a210 	add	x16, x16, #0x28
     f030048:	d61f0200 	br	x16
	\.\.\.

000000000f030058 <__c_func_veneer>:
     f030058:	d503245f 	bti	c
     f03005c:	14400009 	b	10030080 <c_func>

000000000f030060 <__b_func_bti_veneer>:
     f030060:	d503245f 	bti	c
     f030064:	163ffff3 	b	8030030 <b_func>

000000000f030068 <__extern_func_veneer>:
     f030068:	90f87f90 	adrp	x16, 20000 <\.plt>
     f03006c:	9100e210 	add	x16, x16, #0x38
     f030070:	d61f0200 	br	x16
	\.\.\.

0000000010030080 <c_func>:
    10030080:	17bffff0 	b	f030040 <__a_func_veneer>
    10030084:	17bffff7 	b	f030060 <__b_func_bti_veneer>
    10030088:	17bffff8 	b	f030068 <__extern_func_veneer>
