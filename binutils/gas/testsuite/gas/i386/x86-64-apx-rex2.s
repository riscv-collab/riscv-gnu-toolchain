# Check 64bit instructions with rex2 prefix encoding

	.allow_index_reg
	.text
_start:
         test	$0x7, %r24b
         test	$0x7, %r24d
         test	$0x7, %r24
         test	$0x7, %r24w
## REX2.M bit
         imull	%eax, %r15d
         imull	%eax, %r16d
         punpckldq (%r18), %mm2
## REX2.R4 bit
         leal	(%rax), %r16d
         leal	(%rax), %r17d
         leal	(%rax), %r18d
         leal	(%rax), %r19d
         leal	(%rax), %r20d
         leal	(%rax), %r21d
         leal	(%rax), %r22d
         leal	(%rax), %r23d
         leal	(%rax), %r24d
         leal	(%rax), %r25d
         leal	(%rax), %r26d
         leal	(%rax), %r27d
         leal	(%rax), %r28d
         leal	(%rax), %r29d
         leal	(%rax), %r30d
         leal	(%rax), %r31d
## REX2.X4 bit
         leal	(,%r16), %eax
         leal	(,%r17), %eax
         leal	(,%r18), %eax
         leal	(,%r19), %eax
         leal	(,%r20), %eax
         leal	(,%r21), %eax
         leal	(,%r22), %eax
         leal	(,%r23), %eax
         leal	(,%r24), %eax
         leal	(,%r25), %eax
         leal	(,%r26), %eax
         leal	(,%r27), %eax
         leal	(,%r28), %eax
         leal	(,%r29), %eax
         leal	(,%r30), %eax
         leal	(,%r31), %eax
## REX2.B4 bit
         leal	(%r16), %eax
         leal	(%r17), %eax
         leal	(%r18), %eax
         leal	(%r19), %eax
         leal	(%r20), %eax
         leal	(%r21), %eax
         leal	(%r22), %eax
         leal	(%r23), %eax
         leal	(%r24), %eax
         leal	(%r25), %eax
         leal	(%r26), %eax
         leal	(%r27), %eax
         leal	(%r28), %eax
         leal	(%r29), %eax
         leal	(%r30), %eax
         leal	(%r31), %eax
## REX2.W bit
         leaq	(%rax), %r15
         leaq	(%rax), %r16
         leaq	(%r15), %rax
         leaq	(%r16), %rax
         leaq	(,%r15), %rax
         leaq	(,%r16), %rax
## REX2.R3 bit
         add    (%r16), %r8
         add    (%r16), %r15
## REX2.X3 bit
         mov    (,%r9), %r16
         mov    (,%r14), %r16
## REX2.B3 bit
	 sub   (%r10), %r31
	 sub   (%r13), %r31
## SIB
         leal	1(%r16, %r20), %eax
         leal	1(%r16, %r28), %r31d
         leal	129(%r20, %r8), %eax
         leal	129(%r28, %r8), %r31d
