
# Write ch to the standard output
	.macro outch  ch
	ldk   $r0,\ch
	sta   0x10000,$r0
	.endm

# End the test with return code c
	.macro  exit c
	ldk   $r0,\c
	sta   0x1fffc,$r0
	.endm

# All assembler tests should start with this macro "start"
	.macro start
	.text

	jmp     __start
	jmp     __start
	reti

	.data
ccsave: .long   0
	.text

# Fiddling to load $cc from the following word in program memory
loadcc:
	exi     $r29,$sp,0
	lpmi    $cc,$r29,0
	add     $r29,$r29,4
	exi     $r29,$sp,0
	return

failcase:
	outch 'f'
	outch 'a'
	outch 'i'
	outch 'l'
	outch '\n'
	exit  1

__start:

	.endm

# At the end of the test, the code should reach this macro PASS
	.macro PASS
	outch 'p'
	outch 'a'
	outch 's'
	outch 's'
	outch '\n'
	exit  0
	.endm

# Confirm that reg has value, and fail immediately if not
# Preserves all registers
	.macro EXPECT reg,value
	sta   ccsave,$cc
	call  loadcc
	.long \value
	cmp   \reg,$cc
	jmpc  nz,failcase
	lda   $cc,ccsave
	.endm
