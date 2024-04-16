	.text
	.irp op casp, caspa, caspal, caspl, scasp, scaspa, scaspal, scaspl
	rcw\op x0, x1, x4, x5, [x16]
	.endr
	.irp op clrp, clrpa, clrpal, clrpl, sclrp, sclrpa, sclrpal, sclrpl, swpp, swppa, swppal, swppl, sswpp, sswppa, sswppal, sswppl, setp, setpa, setpal, setpl, ssetp, ssetpa, ssetpal, ssetpl
	rcw\op x0, x5, [x16]
	.endr
