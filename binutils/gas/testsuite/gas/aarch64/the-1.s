	.text
	.irp op cas, casa, casal, casl, scas, scasa, scasal, scasl, clr, clra, clral, clrl, sclr, sclra, sclral, sclrl, swp, swpa, swpal, swpl, sswp, sswpa, sswpal, sswpl, set, seta, setal, setl, sset, sseta, ssetal, ssetl
        .irp reg1 x0, x16
        .irp reg2 x1, x8, x17, x25
        .irp reg3 x2, x18
	rcw\op \reg1, \reg3, [\reg2]
	.endr
	.endr
	.endr
	.endr
