target:
	# ZCF only compress single float instructions with RV32
	flw fa0, 0(a0)
	c.flw fa1, 0(s0)
	flw fa0, 0(sp)
	c.flwsp fa1, 0(sp)
	fsw fa0, 0(a0)
	c.fsw fa1, 0(s0)
	fsw fa0, 0(sp)
	c.fswsp fa1, 0(sp)
