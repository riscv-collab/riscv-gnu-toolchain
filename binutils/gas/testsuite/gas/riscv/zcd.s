target:
	# ZCD only compress double float instructions
	fld fa0, 0(a0)
	c.fld fa1, 0(s0)
	fld fa0, 0(sp)
	c.fldsp fa1, 0(sp)
	fsd fa0, 0(a0)
	c.fsd fa1, 0(s0)
	fsd fa0, 0(sp)
	c.fsdsp fa1, 0(sp)
