.macro	INSN_SEQ
	ntl.p1
	sb	s11, 0(t0)
	ntl.pall
	sb	s11, 2(t0)
	ntl.s1
	sb	s11, 4(t0)
	ntl.all
	sb	s11, 6(t0)
.endm

.macro	INSN_SEQ_C
	c.ntl.p1
	sb	s11, 8(t0)
	c.ntl.pall
	sb	s11, 10(t0)
	c.ntl.s1
	sb	s11, 12(t0)
	c.ntl.all
	sb	s11, 14(t0)
.endm

target:
	INSN_SEQ	# RV32I_Zihintntl

	# 'Zcb' is chosen to test complex cases to enable
	# compressed instructions.
	.option	push
	.option	arch, +zcb
	INSN_SEQ	# RV32I_Zihintntl_Zca_Zcb (auto compression without prefix)
	INSN_SEQ_C	# RV32I_Zihintntl_Zca_Zcb (with compressed prefix)
	.option pop
