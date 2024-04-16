GAS LISTING .*


   1              	## Testcase with a variety of pop.
   2              	## all pop insns valid in 64-bit mode must be processed for SCFI.
   3              		.text
   4              		.globl  foo
   5              		.type   foo, @function
   5              	ginsn: SYM FUNC_BEGIN
   6              	foo:
   6              	ginsn: SYM foo
   7 0000 660FA1   		popw    %fs
   7              	ginsn: LOAD \[%r7\+0\], %r54
   7              	ginsn: ADD %r7, 2, %r7
   8 0003 660FA9   		popw    %gs
   8              	ginsn: LOAD \[%r7\+0\], %r55
   8              	ginsn: ADD %r7, 2, %r7
   9 0006 669D     		popfw
   9              	ginsn: LOAD \[%r7\+0\], %r49
   9              	ginsn: ADD %r7, 2, %r7
  10 0008 66418F42 		popw    -8\(%r10\)
  10      F8
  10              	ginsn: LOAD \[%r7\+0\], \[%r10\+0\]
  10              	ginsn: ADD %r7, 2, %r7
  11 000d 428F0415 		popq    -8\(,%r10\)
  11      F8FFFFFF 
  11              	ginsn: LOAD \[%r7\+0\], \[%r10\+0\]
  11              	ginsn: ADD %r7, 8, %r7
  12 0015 8F042500 		pop     symbol
  12      000000
  12              	ginsn: LOAD \[%r7\+0\], \[%r4\+0\]
  12              	ginsn: ADD %r7, 8, %r7
  13 001c 58       		popq    %rax
  13              	ginsn: LOAD \[%r7\+0\], %r0
  13              	ginsn: ADD %r7, 8, %r7
  14 001d C3       		ret
  14              	ginsn: RET
  15              	.LFE0:
  15              	ginsn: SYM .LFE0
  16              		.size   foo, .-foo
  16              	ginsn: SYM FUNC_END
