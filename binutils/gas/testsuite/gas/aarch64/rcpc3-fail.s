.text

	ldiapp w0, w1, [x3, #8]
	ldiapp x0, x1, [x3, #16]

	stilp w0, w1, [x3, #8]
	stilp x0, x1, [x3, #16]

	stilp w0, w1, [x3], #8
	stilp x0, x1, [x3], #16

	ldiapp w0, w1, [x3, #-8]!
	ldiapp x0, x1, [x3, #-16]!
