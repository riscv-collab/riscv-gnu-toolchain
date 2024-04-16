.text

	ldiapp x0, x1, [x3]
	ldiapp w0, w1, [x3]
	ldiapp x0, x1, [x3], #16
	ldiapp w0, w1, [x3], #8

	stilp x0, x1, [x3]
	stilp w0, w1, [x3]
	stilp x0, x1, [x3, #-16]!
	stilp w0, w1, [x3, #-8]!

	ldapr w1, [x2], #4
	ldapr x1, [x2], #8

	stlr w1, [x2, #-4]!
	stlr x1, [x2, #-8]!
