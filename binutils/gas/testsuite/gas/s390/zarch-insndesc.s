.text
foo:
	cdfbr	%f6,%r9
	brxh	%r6,%r9,.
	srnm	4095(%r5)
	lngfr	%r9,%r6
	rnsbgt	%r6,%r7,18,28,38
	risbhgz	%r6,%r7,12,13,14
	cxfbra %f5,3,%r9,7
	risbgnz	%r6,%r7,12,20,14
	lochhino	%r6,-32765
	cgijnl	%r6,-42,.
