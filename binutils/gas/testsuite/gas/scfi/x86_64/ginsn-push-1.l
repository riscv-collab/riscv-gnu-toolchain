GAS LISTING .*


   1              	## Testcase with a variety of push.
   2              	## all push insns valid in 64-bit mode must be processed for SCFI.
   3              		.text
   4              		.globl  foo
   5              		.type   foo, @function
   5              	ginsn: SYM FUNC_BEGIN
   6              	foo:
   6              	ginsn: SYM foo
   7 0000 660FA0   		pushw   %fs
   7              	ginsn: SUB %r7, 2, %r7
   7              	ginsn: STORE %r54, \[%r7\+0\]
   8 0003 660FA8   		pushw   %gs
   8              	ginsn: SUB %r7, 2, %r7
   8              	ginsn: STORE %r55, \[%r7\+0\]
   9 0006 666A28   		pushw   \$40
   9              	ginsn: SUB %r7, 2, %r7
   9              	ginsn: STORE 0, \[%r7\+0\]
  10 0009 6641FF72 		pushw   -8\(%r10\)
  10      F8
  10              	ginsn: SUB %r7, 2, %r7
  10              	ginsn: STORE \[%r10\+0\], \[%r7\+0\]
  11 000e 42FF3415 		pushq   -8\(,%r10\)
  11      F8FFFFFF 
  11              	ginsn: SUB %r7, 8, %r7
  11              	ginsn: STORE \[%r10\+0\], \[%r7\+0\]
  12 0016 669C     		pushfw
  12              	ginsn: SUB %r7, 2, %r7
  12              	ginsn: STORE %r49, \[%r7\+0\]
  13 0018 FF342500 		push    symbol
  13      000000
  13              	ginsn: SUB %r7, 8, %r7
  13              	ginsn: STORE \[%r4\+0\], \[%r7\+0\]
  14 001f 50       		push    %rax
  14              	ginsn: SUB %r7, 8, %r7
  14              	ginsn: STORE %r0, \[%r7\+0\]
  15 0020 C3       		ret
  15              	ginsn: RET
  16              	.LFE0:
  16              	ginsn: SYM .LFE0
  17              		.size   foo, .-foo
  17              	ginsn: SYM FUNC_END
