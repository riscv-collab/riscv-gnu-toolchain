# trap.s
#

	.text
	.global _trap_entry
	.type	_trap_entry, @function

	.align 4 # Trap entry must be 4-byte aligned for mtvec CSR

_trap_entry:
	
	# Save current context to the stack.
	# 
	#   struct trap_context {
	#       uint64_t x[32]; // x[0] unused
	#       uint64_t mstatus;
	#       uint64_t mepc;
	#   };
	# 

	addi	sp, sp, -34*8

	nop			# x0 is zero
	sd      x1, 1*8(sp)	# x1 is ra
	nop			# x2 is sp
	sd      x3, 3*8(sp)	# x3 is gp
	sd      x4, 4*8(sp)	# x4 is tp
	sd      x5, 5*8(sp)	# x4 is t0
	sd      x6, 6*8(sp)	# x6 is t1
	sd      x7, 7*8(sp)	# x7 is t2
	sd      x8, 8*8(sp)	# x8 is s0/fp
	sd      x9, 9*8(sp)	# x9 is s1
	sd	x11, 11*8(sp)	# x11 is a1
	sd	x12, 12*8(sp)	# x12 is a2
	sd	x13, 13*8(sp)	# x13 is a3
	sd	x14, 14*8(sp)	# x14 is a4
	sd	x15, 15*8(sp)	# x15 is a5
	sd	x16, 16*8(sp)	# x16 is a6
	sd	x17, 17*8(sp)	# x17 is a7
	sd	x18, 18*8(sp)	# x18 is s2
	sd	x19, 19*8(sp)	# x19 is s3
	sd	x20, 20*8(sp)	# x20 is s4
	sd	x21, 21*8(sp)	# x21 is s5
	sd	x22, 22*8(sp)	# x22 is s6
	sd	x23, 23*8(sp)	# x23 is s7
	sd	x24, 24*8(sp)	# x24 is s8
	sd	x25, 25*8(sp)	# x25 is s9
	sd	x26, 26*8(sp)	# x26 is s10
	sd	x27, 27*8(sp)	# x27 is s11
	sd	x28, 28*8(sp)	# x28 is t3
	sd	x29, 29*8(sp)	# x29 is t4
	sd	x30, 30*8(sp)	# x30 is t5
	sd	x31, 31*8(sp)	# x31 is t6

	# Save mstatus and mepc

	csrr	t0, mstatus
	sd	t0, 32*8(sp)
	csrr	t0, mepc
	sd	t0, 33*8(sp)

	# Dispatch to fault_handler or one of the interrupt service routines.
	# 
	# To return from a trap handler, we need to execute mret. We arrange
	# for all of the above handlers to return to the instruction following
	# the call instruction below. The call instruction will place the
	# address of that instruction into ra and continue at label 1 below.

	call	1f

	# ALL HANDLERS RETURN HERE

	# Restore saved context.

	ld	t0, 33*8(sp)
	csrw	mepc, t0
	ld	t0, 32*8(sp)
	csrw	mstatus, t0

	ld	x31, 31*8(sp)	# x31 is t6
	ld	x30, 30*8(sp)	# x30 is t5
	ld	x29, 29*8(sp)	# x29 is t4
	ld	x28, 28*8(sp)	# x28 is t3
	ld	x27, 27*8(sp)	# x27 is s11
	ld	x26, 26*8(sp)	# x26 is s10
	ld	x25, 25*8(sp)	# x25 is s9
	ld	x24, 24*8(sp)	# x24 is s8
	ld	x23, 23*8(sp)	# x23 is s7
	ld	x22, 22*8(sp)	# x22 is s6
	ld	x21, 21*8(sp)	# x21 is s5
	ld	x20, 20*8(sp)	# x20 is s4
	ld	x19, 19*8(sp)	# x19 is s3
	ld	x18, 18*8(sp)	# x18 is s2
	ld	x17, 17*8(sp)	# x17 is a7
	ld	x16, 16*8(sp)	# x16 is a6
	ld	x15, 15*8(sp)	# x15 is a5
	ld	x14, 14*8(sp)	# x14 is a4
	ld	x13, 13*8(sp)	# x13 is a3
	ld	x12, 12*8(sp)	# x12 is a2
	ld	x11, 11*8(sp)	# x11 is a1
	ld	x10, 10*8(sp)	# x10 is sp
	ld	x9, 9*8(sp)	# x9 is s1
	ld	x8, 8*8(sp)	# x8 is s0/fp
	ld	x7, 7*8(sp)	# x7 is t2
	ld	x6, 6*8(sp)	# x6 is t1
	ld	x5, 5*8(sp)	# x5 is t0
	ld	x4, 4*8(sp)	# x4 is tp
	ld	x3, 3*8(sp)	# x3 is gp
	nop			# x2 is sp
	ld	x1, 1*8(sp)	# x1 is ra
	nop			# x0 is zero

	addi	sp, sp, 34*8
	
	mret

1:	# _trap_entry continues here from call instruction above

	# Dispatch to fault_handler if mcause[63] = 0. Otherwise, dispatch to
	# a specific interrupt handler. The fault handler gets a pointer to a
	# struct trap_frame as its first argument and the mcause value as the
	# second argument.

	csrr	a0, mcause
	mv	a1, sp
	bgez	a0, fault_handler	# in halt.c (for now)
	
	slli	a0, a0, 1		# Clear most significant bit so that 
	srli	a0, a0, 1		# a0 contains the "exception code."

	j	intr_handler		# in intr.c

	.end

