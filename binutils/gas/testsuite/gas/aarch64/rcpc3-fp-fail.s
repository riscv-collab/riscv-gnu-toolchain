.text

	stl1	{ v1.d }[-1], [x1]	// BAD
	stl1	{ v1.d }[0], [x1]	// OK
	stl1	{ v1.d }[1], [x1]	// OK
	stl1	{ v1.d }[2], [x1]	// BAD

	ldap1	{ v2.d }[-1], [sp]	// BAD
	ldap1	{ v2.d }[0], [sp]	// OK
	ldap1	{ v2.d }[1], [sp]	// OK
	ldap1	{ v2.d }[2], [sp]	// BAD

	ldapur	b1, [x1, #-257]	// BAD
	ldapur	b1, [x1, #-256]	// OK
	ldapur	b1, [x1, #255]		// OK
	ldapur	b1, [x1, #256]		// BAD

	stlur	q1, [x1, #-257]	// BAD
	stlur	q1, [x1, #-256]	// OK
	stlur	q1, [x1, #255]		// OK
	stlur	q1, [x1, #256]		// BAD

	ldapur	b1, [x1], #255		// BAD
	ldapur	b1, [x1, #-255]!	// BAD

	stlur	b1, [x1], #255		// BAD
	stlur	b1, [x1, #-255]!	// BAD
