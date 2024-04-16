SYS_exit = 1
SYS_write = 4

	.bss
	.space 64
_stack:

	.data
pass_text:
	.string "pass\n"
fail_text:
	.string "fail\n"

	.text
	.global	_start
_start:
	movhi   hi(_stack),	r0,	sp
	movea   lo(_stack),	sp,	sp
	jr	start_test

	.macro	seti	val reg
	movhi	hi(\val),r0,\reg
	movea	lo(\val),\reg,\reg
	.endm

_pass_1:
	mov	SYS_write,r6
	mov	1,r7
	seti	pass_text,r8
	mov	5,r9
	trap	31

	mov	0, r7
	jr	_exit

_fail_1:
	mov	SYS_write,r6
	mov	1,r7
	seti	fail_text,r8
	mov	5,r9
	trap	31

	mov	1, r7
	jr	_exit

_exit:
	mov	SYS_exit, r6
	mov	0, r8
	mov	0, r9
	trap	31

_pass:
	jr	_pass_1

_fail:
	jr	_fail_1

	.macro	pass
	jr	_pass
	.endm
	.macro	fail
	jr	_fail
	.endm

	# These pass or fail if the given flag is set or not set
	# Currently, it assumed that the code of any test is going to
	# be less than 256 bytes.  Else, we'll have to use a
	# branch-around-jump design instead.

	.macro	pass_c
	bc	_pass
	.endm
	.macro	fail_c
	bc	_fail
	.endm
	.macro	pass_nc
	bnc	_pass
	.endm
	.macro	fail_nc
	bnc	_fail
	.endm

	.macro	pass_z
	bz	_pass
	.endm
	.macro	fail_z
	bz	_fail
	.endm
	.macro	pass_nz
	bnz	_pass
	.endm
	.macro	fail_nz
	bnz	_fail
	.endm

	.macro	pass_v
	bv	_pass
	.endm
	.macro	fail_v
	bv	_fail
	.endm
	.macro	pass_nv
	bnv	_pass
	.endm
	.macro	fail_nv
	bnv	_fail
	.endm

	.macro	pass_s
	bn	_pass
	.endm
	.macro	fail_s
	bn	_fail
	.endm
	.macro	pass_ns
	bp	_pass
	.endm
	.macro	fail_ns
	bp	_fail
	.endm

	.macro	pass_sat
	bsa	_pass
	.endm
	.macro	fail_sat
	bsa	_fail
	.endm
	.macro	pass_nsat
	bsa	1f
	br	_pass
1:
	.endm
	.macro	fail_nsat
	bsa	1f
	br	_fail
1:
	.endm

	# These pass/fail if the given register has/hasn't the specified value in it.

	.macro	pass_req	reg val
	seti	\val,r10
	cmp	r10,\reg
	be	_pass
	.endm

	.macro	pass_rne	reg val
	seti	\val,r10
	cmp	r10,\reg
	bne	_pass
	.endm

	.macro	fail_req	reg val
	seti	\val,r10
	cmp	r10,\reg
	be	_fail
	.endm

	.macro	fail_rne	reg val
	seti	\val,r10
	cmp	r10,\reg
	bne	_fail
	.endm

# convenience version
	.macro	reg	reg val
	seti	\val,r10
	cmp	r10,\reg
	bne	_fail
	.endm

z    = 1
nz   = 0
s    = 2
ns   = 0
v    = 4
nv   = 0
c    = 8
nc   = 0
sat  = 16
nsat = 0

# sat c v s z

	.macro	flags	fval
	stsr	psw, r10
	movea	+(\fval), r0, r9
	andi	31, r10, r10
	cmp	r9, r10
	bne	_fail
	.endm

	.macro	noflags
	stsr	psw, r10
	andi	~0x1f, r10, r10
	ldsr	r10, psw
	.endm

	.macro	allflags
	stsr	psw, r10
	ori	0x1f, r10, r10
	ldsr	r10, psw
	.endm

start_test:
