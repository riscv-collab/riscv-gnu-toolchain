target:
	# ntl.p1   == add x0, x0, x2
	# ntl.pall == add x0, x0, x3
	# ntl.s1   == add x0, x0, x4
	# ntl.all  == add x0, x0, x5
	add x0, x0, x2
	sb	s11, 0(t0)
	add x0, x0, x3
	sb	s11, 2(t0)
	add x0, x0, x4
	sb	s11, 4(t0)
	add x0, x0, x5
	sb	s11, 6(t0)

	# c.ntl.p1   == c.add x0, x2
	# c.ntl.pall == c.add x0, x3
	# c.ntl.s1   == c.add x0, x4
	# c.ntl.all  == c.add x0, x5
	.option	push
	.option	arch, +zca
	c.add	x0, x2
	sb	s11, 8(t0)
	c.add	x0, x3
	sb	s11, 10(t0)
	c.add	x0, x4
	sb	s11, 12(t0)
	c.add	x0, x5
	sb	s11, 14(t0)
	.option	pop
