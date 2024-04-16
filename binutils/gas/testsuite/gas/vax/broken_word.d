#as:
#objdump: -dr

.*:     file format .*


Disassembly of section .text:

00000000 <asmfn>:
       0:	00 00       	\.word 0x0000 # Entry mask: < >
       2:	c2 04 5e    	subl2 \$0x4,sp
       5:	d0 ac 04 50 	movl 0x4\(ap\),r0
       9:	cf 50 01 02 	casel r0,\$0x1,\$0x2

0000000d <\.casetable>:
       d:	1d 00 85 7f 09 00  .*

00000013 <\.casetableend>:
      13:	31 06 00    	brw 1c <casedefault>
      16:	17 ef 87 83 	jmp 83a3 <case3>
      1a:	00 00 *

0000001c <casedefault>:
      1c:	df ef 00 00 	pushal .*
      20:	00 00 *
			1e: R_VAX_PC32	\.rodata\+0x18
      22:	fb 01 ef 00 	calls \$0x1,.*
      26:	00 00 00 *
			25: R_VAX_PC32	printf

00000029 <asmret>:
      29:	04          	ret

0000002a <case1>:
      2a:	df ef 00 00 	pushal .*
      2e:	00 00 *
			2c: R_VAX_PC32	\.rodata
      30:	fb 01 ef 00 	calls \$0x1,.*
      34:	00 00 00 *
			33: R_VAX_PC32	printf
      37:	17 af ef    	jmp 29 <asmret>
	\.\.\.

00007f92 <case2>:
    7f92:	df ef 00 00 	pushal .*
    7f96:	00 00 *
			7f94: R_VAX_PC32	\.rodata\+0x8
    7f98:	fb 01 ef 00 	calls \$0x1,.*
    7f9c:	00 00 00 *
			7f9b: R_VAX_PC32	printf
    7f9f:	17 cf 86 80 	jmp 29 <asmret>
	\.\.\.

000083a3 <case3>:
    83a3:	df ef 00 00 	pushal .*
    83a7:	00 00 *
			83a5: R_VAX_PC32	\.rodata\+0x10
    83a9:	fb 01 ef 00 	calls \$0x1,.*
    83ad:	00 00 00 *
			83ac: R_VAX_PC32	printf
    83b0:	17 ef 73 7c 	jmp 29 <asmret>
    83b4:	ff ff *
