GAS LISTING .*


.*# Testcase for DWARF regnum ginsn API
   2              		.text
   3              		.globl  foo
   4              		.type   foo, @function
   4              	ginsn: SYM FUNC_BEGIN
   5              	foo:
   5              	ginsn: SYM foo
   6 0000 0408     		add    \$8, %al
   7 0002 80C108   		add    \$8, %cl
   7              	ginsn: ADD %r2, 8, %r2
   8 0005 80C208   		add    \$8, %dl
   8              	ginsn: ADD %r1, 8, %r1
   9 0008 80C408   		add    \$8, %ah
   9              	ginsn: ADD %r0, 8, %r0
  10 000b 80C508   		add    \$8, %ch
  10              	ginsn: ADD %r2, 8, %r2
  11 000e 80C608   		add    \$8, %dh
  11              	ginsn: ADD %r1, 8, %r1
  12 0011 80C708   		add    \$8, %bh
  12              	ginsn: ADD %r3, 8, %r3
  13              	
  14 0014 4080C008 		add    \$8, %axl
  14              	ginsn: ADD %r0, 8, %r0
  15 0018 4080C408 		add    \$8, %spl
  15              	ginsn: ADD %r7, 8, %r7
  16              	
  17 001c 6683C008 		add    \$8, %ax
  17              	ginsn: ADD %r0, 8, %r0
  18 0020 664183C0 		add    \$8, %r8w
  18      08
  18              	ginsn: ADD %r8, 8, %r8
  19 0025 6683C408 		add    \$8, %sp
  19              	ginsn: ADD %r7, 8, %r7
  20              	
  21 0029 83C008   		add    \$8, %eax
  21              	ginsn: ADD %r0, 8, %r0
  22 002c 4183C008 		add    \$8, %r8d
  22              	ginsn: ADD %r8, 8, %r8
  23 0030 81C40040 		add    \$16384, %esp
  23      0000
  23              	ginsn: ADD %r7, 16384, %r7
  24              	
  25 0036 4883C508 		add    \$8, %rbp
  25              	ginsn: ADD %r6, 8, %r6
  26              	
  27 003a 488D05FE 		lea    -0x2\(%rip\), %rax
  27      FFFFFF
  27              	ginsn: ADD %r4, -2, %r0
  28 0041 67488905 		mov    %rax, 0x2\(%eip\)
  28      02000000 
  28              	ginsn: MOV %r0, \[%r4\+2\]
  29 0049 67488B05 		mov    -0x2\(%eip\), %rax
  29      FEFFFFFF 
  29              	ginsn: MOV \[%r4\+-2\], %r0
  30              	
  31 0051 C3       		ret
  31              	ginsn: RET
GAS LISTING .*


  32              	.LFE0:
  32              	ginsn: SYM .LFE0
  33              		.size   foo, .-foo
  33              	ginsn: SYM FUNC_END

#pass
