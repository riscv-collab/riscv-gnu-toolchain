.text

	ldap1 	{v1.d}[0], [sp]
	ldap1 	{v2.d}[1], [x1]
	stl1 	{v3.d}[0], [x2]
	stl1 	{v4.d}[1], [x3]

	ldapur	b1, [sp]
	ldapur	b1, [sp, #-256]
	ldapur	b1, [sp, #255]
	ldapur	h2, [x2]
	ldapur	s3, [x3]
	ldapur	d4, [x4]
	ldapur	q1, [sp]

	stlur	b1, [sp]
	stlur	b1, [sp, #-256]
	stlur	b1, [sp, #255]
	stlur	s3, [x3]
	stlur	d4, [x4]
	stlur	q1, [sp]
