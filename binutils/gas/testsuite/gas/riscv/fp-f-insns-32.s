F:
	fabs.s	f31, f0
	fabs.s	f0, f31

	fadd.s	f31, f0, f0
	fadd.s	f0, f31, f0
	fadd.s	f0, f0, f31
	fadd.s	f0, f0, f0, rne
	fadd.s	f0, f0, f0, rtz
	fadd.s	f0, f0, f0, rdn
	fadd.s	f0, f0, f0, rup
	fadd.s	f0, f0, f0, rmm

	fclass.s x31, f0
	fclass.s x0, f31

	fcvt.s.w f31, x0
	fcvt.s.w f0, x31
	fcvt.s.w f0, x0, rne
	fcvt.s.wu f0, x0
	fcvt.w.s x31, f0
	fcvt.w.s x0, f31
	fcvt.w.s x0, f0, rne
	fcvt.wu.s x0, f0

	fdiv.s	f31, f0, f0
	fdiv.s	f0, f31, f0
	fdiv.s	f0, f0, f31
	fdiv.s	f0, f0, f0, rne

	feq.s	x31, f0, f0
	feq.s	x0, f31, f0
	feq.s	x0, f0, f31

	fge.s	x31, f0, f0
	fge.s	x0, f31, f0
	fge.s	x0, f0, f31

	fgt.s	x31, f0, f0
	fgt.s	x0, f31, f0
	fgt.s	x0, f0, f31

	fle.s	x31, f0, f0
	fle.s	x0, f31, f0
	fle.s	x0, f0, f31

	flt.s	x31, f0, f0
	flt.s	x0, f31, f0
	flt.s	x0, f0, f31

	flw	f31, (x0)
	flw	f0, 0x7ff(x0)
	flw	f0, -0x800(x0)
	flw	f0, (x31)
	flw	f0, sval, x31

	flw	f15, (x8)
	flw	f8, 4(x8)
	flw	f8, 0x38(x8)
	flw	f8, 0x40(x8)
	flw	f8, (x15)

	flw	f31, (sp)
	flw	f0, 0x1c(sp)
	flw	f0, 0x20(sp)
	flw	f0, 0xc0(sp)

	fmadd.s	f31, f0, f0, f0
	fmadd.s	f0, f31, f0, f0
	fmadd.s	f0, f0, f31, f0
	fmadd.s	f0, f0, f0, f31
	fmadd.s	f0, f0, f0, f0, rne

	fmax.s	f31, f0, f0
	fmax.s	f0, f31, f0
	fmax.s	f0, f0, f31

	fmin.s	f31, f0, f0
	fmin.s	f0, f31, f0
	fmin.s	f0, f0, f31

	fmsub.s	f31, f0, f0, f0
	fmsub.s	f0, f31, f0, f0
	fmsub.s	f0, f0, f31, f0
	fmsub.s	f0, f0, f0, f31
	fmsub.s	f0, f0, f0, f0, rne

	fmul.s	f31, f0, f0
	fmul.s	f0, f31, f0
	fmul.s	f0, f0, f31
	fmul.s	f0, f0, f0, rne

	fmv.s	f31, f0
	fmv.s	f0, f31

	fmv.s.x	f31, x0
	fmv.s.x	f0, x31
	fmv.x.s	x31, f0
	fmv.x.s	x0, f31

	fneg.s	f31, f0
	fneg.s	f0, f31

	fnmadd.s f31, f0, f0, f0
	fnmadd.s f0, f31, f0, f0
	fnmadd.s f0, f0, f31, f0
	fnmadd.s f0, f0, f0, f31
	fnmadd.s f0, f0, f0, f0, rne

	fnmsub.s f31, f0, f0, f0
	fnmsub.s f0, f31, f0, f0
	fnmsub.s f0, f0, f31, f0
	fnmsub.s f0, f0, f0, f31
	fnmsub.s f0, f0, f0, f0, rne

	frcsr	x31
	frflags	x31
	frrm	x31

	fscsr	x31
	fscsr	x31, x1
	fscsr	x1, x31

	fsflags	x31
	fsflags	x31, x1
	fsflags	x1, x31

	fsgnj.s	f31, f0, f1
	fsgnj.s	f0, f31, f0
	fsgnj.s	f0, f0, f31
	fsgnjn.s f0, f1, f0
	fsgnjx.s f0, f1, f0

	fsqrt.s	f31, f0
	fsqrt.s	f0, f31
	fsqrt.s	f0, f0, rne

	fsrm	x31
	fsrm	x31, x1
	fsrm	x1, x31

	fsub.s	f31, f0, f0
	fsub.s	f0, f31, f0
	fsub.s	f0, f0, f31
	fsub.s	f0, f0, f0, rne

	fsw	f31, (x0)
	fsw	f0, 0x1f(x0)
	fsw	f0, -0x20(x0)
	fsw	f0, (x31)
	fsw	f0, sval, x31

	fsw	f15, (x8)
	fsw	f8, 4(x8)
	fsw	f8, 0x38(x8)
	fsw	f8, 0x40(x8)
	fsw	f8, (x15)

	fsw	f31, (sp)
	fsw	f0, 0x1c(sp)
	fsw	f0, 0x20(sp)
	fsw	f0, 0xc0(sp)
