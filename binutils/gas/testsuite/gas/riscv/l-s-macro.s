L:
	lb	a0, bval
	lbu	a0, bval
	lh	a0, hval
	lhu	a0, hval
	lw	a0, wval
	lwu	a0, wval
	ld	a0, dval

S:
	sb	a0, bval, t0
	sh	a0, hval, t0
	sw	a0, wval, t0
	sd	a0, dval, t0
