# Macros for the fake ISA.  Keep in sync with example-synacor/README.arch-spec.

# These .set macros will generate symbols in the output ELF, but it also allows
# use to use them as arguments to the insns below.  Oh well.
.set r0, 32768
.set r1, r0+1
.set r2, r0+2
.set r3, r0+3
.set r4, r0+4
.set r5, r0+5
.set r6, r0+6
.set r7, r0+7

# The target is little endian, so make sure we output the 16-bit words as such.
.macro _op op:req, more:vararg
	.byte \op & 0xff, (\op >> 8) & 0xff
	.ifnb \more
	_op \more
	.endif
.endm

.macro HALT
	_op 0
.endm

.macro SET a:req, b:req
	_op 1, \a, \b
.endm

.macro PUSH a:req
	_op 2, \a
.endm

.macro POP a:req
	_op 3, \a
.endm

.macro EQ a:req, b:req, c:req
	_op 4, \a, \b, \c
.endm

.macro GT a:req, b:req, c:req
	_op 5, \a, \b, \c
.endm

.macro JMP a:req
	_op 6, \a
.endm

.macro JT a:req, b:req
	_op 7, \a, \b
.endm

.macro JF a:req, b:req
	_op 8, \a, \b
.endm

.macro ADD a:req, b:req, c:req
	_op 9, \a, \b, \c
.endm

.macro MULT a:req, b:req, c:req
	_op 10, \a, \b, \c
.endm

.macro MOD a:req, b:req, c:req
	_op 11, \a, \b, \c
.endm

.macro AND a:req, b:req, c:req
	_op 12, \a, \b, \c
.endm

.macro OR a:req, b:req, c:req
	_op 13, \a, \b, \c
.endm

.macro NOT a:req, b:req
	_op 14, \a, \b
.endm

.macro RMEM a:req, b:req
	_op 15, \a, \b
.endm

.macro WMEM a:req, b:req
	_op 16, \a, \b
.endm

.macro CALL a:req
	_op 17, \a
.endm

.macro RET
	_op 18
.endm

.macro OUT a:req
	_op 19, \a
.endm

.macro IN a:req
	_op 20, \a
.endm

.macro NOOP
	_op 21
.endm
