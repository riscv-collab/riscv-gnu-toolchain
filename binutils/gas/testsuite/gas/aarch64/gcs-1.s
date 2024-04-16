	.text
	gcspushx
	gcspopcx
	gcspopx
	gcspopm
	gcsb dsync

	.irp op gcspushm, gcsss1, gcsss2, gcspopm
        .irp reg1 x0, x15, x30, xzr
	\op \reg1
	.endr
	.endr

	.irp op gcsstr, gcssttr
        .irp reg1 x0, x15, x30, xzr
	.irp reg2 x1, x16, sp
	\op \reg1, \reg2
	.endr
	.endr
	.endr
