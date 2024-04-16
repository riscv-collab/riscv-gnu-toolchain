GAS LISTING .*


   1              	## Testcase with a variety of add.
   2              	## Some add insns valid in 64-bit mode may not be processed for SCFI.
   3              		.text
   4              		.globl foo
   5              		.type foo, @function
   5              	ginsn: SYM FUNC_BEGIN
   6              	foo:
   6              	ginsn: SYM foo
   7 0000 54       		push %rsp
   7              	ginsn: SUB %r7, 8, %r7
   7              	ginsn: STORE %r7, \[%r7\+0\]
   8 0001 4889E5   		movq %rsp, %rbp
   8              	ginsn: MOV %r7, %r6
   9              	
  10 0004 48010425 		addq %rax, symbol
  10      00000000 
  11 000c 03042500 		add symbol, %eax
  11      000000
  12 0013 670320   		add \(%eax\), %esp
  12              	ginsn: ADD \[%r0\+0\], %r7, %r7
  13 0016 67012405 		add %esp, \(,%eax\)
  13      00000000 
  14 001e 67032405 		add \(,%eax\), %esp
  14      00000000 
  14              	ginsn: ADD \[%r0\+0\], %r7, %r7
  15              	
  16 0026 4801C3   		addq %rax, %rbx
  16              	ginsn: ADD %r0, %r3, %r3
  17 0029 01C3     		add %eax, %ebx
  17              	ginsn: ADD %r0, %r3, %r3
  18              	
  19 002b 4883D408 		adc \$8, %rsp
  19              	ginsn: OTH 0, 0, %r7
  20              	
  21 002f 488345F0 		addq \$1, -16\(%rbp\)
  21      01
  22              	
  23 0034 4803D8   		\{load\} addq %rax, %rbx
  23              	ginsn: ADD %r0, %r3, %r3
  24              	
  25 0037 C3       		ret
  25              	ginsn: RET
  26              	.LFE0:
  26              	ginsn: SYM .LFE0
  27              		.size foo, .-foo
  27              	ginsn: SYM FUNC_END
