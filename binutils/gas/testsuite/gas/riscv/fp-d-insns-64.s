D:
	fabs.d	f31, f0
	fabs.d	f0, f31

	fadd.d	f31, f0, f0
	fadd.d	f0, f31, f0
	fadd.d	f0, f0, f31
	fadd.d	f0, f0, f0, rne
	fadd.d	f0, f0, f0, rtz
	fadd.d	f0, f0, f0, rdn
	fadd.d	f0, f0, f0, rup
	fadd.d	f0, f0, f0, rmm

	fclass.d x31, f0
	fclass.d x0, f31

	fcvt.d.l f0, x0
	fcvt.d.l f0, x0, rne
	fcvt.d.lu f0, x0
	fcvt.d.s f31, f0
	fcvt.d.s f0, f31
	fcvt.d.w f0, x0
	fcvt.d.wu f0, x0
	fcvt.l.d x0, f0
	fcvt.lu.d x0, f0
	fcvt.s.d f31, f0
	fcvt.s.d f0, f31
	fcvt.s.d f0, f0, rne
	fcvt.w.d x0, f0
	fcvt.wu.d x0, f0

	fdiv.d	f31, f0, f0
	fdiv.d	f0, f31, f0
	fdiv.d	f0, f0, f31
	fdiv.d	f0, f0, f0, rne

	feq.d	x31, f0, f0
	feq.d	x0, f31, f0
	feq.d	x0, f0, f31

	fge.d	x31, f0, f0
	fge.d	x0, f31, f0
	fge.d	x0, f0, f31

	fgt.d	x31, f0, f0
	fgt.d	x0, f31, f0
	fgt.d	x0, f0, f31

	fld	f31, (x0)
	fld	f0, 0x7ff(x0)
	fld	f0, -0x800(x0)
	fld	f0, (x31)
	fld	f0, dval, x31

	fle.d	x31, f0, f0
	fle.d	x0, f31, f0
	fle.d	x0, f0, f31

	flt.d	x31, f0, f0
	flt.d	x0, f31, f0
	flt.d	x0, f0, f31

	fmadd.d	f31, f0, f0, f0
	fmadd.d	f0, f31, f0, f0
	fmadd.d	f0, f0, f31, f0
	fmadd.d	f0, f0, f0, f31
	fmadd.d	f0, f0, f0, f0, rne

	fmax.d	f31, f0, f0
	fmax.d	f0, f31, f0
	fmax.d	f0, f0, f31

	fmin.d	f31, f0, f0
	fmin.d	f0, f31, f0
	fmin.d	f0, f0, f31

	fmsub.d	f31, f0, f0, f0
	fmsub.d	f0, f31, f0, f0
	fmsub.d	f0, f0, f31, f0
	fmsub.d	f0, f0, f0, f31
	fmsub.d	f0, f0, f0, f0, rne

	fmul.d	f31, f0, f0
	fmul.d	f0, f31, f0
	fmul.d	f0, f0, f31
	fmul.d	f0, f0, f0, rne

	fmv.d	f31, f0
	fmv.d	f0, f31

	fmv.d.x	f0, x0
	fmv.x.d	x0, f0

	fneg.d	f31, f0
	fneg.d	f0, f31

	fnmadd.d f31, f0, f0, f0
	fnmadd.d f0, f31, f0, f0
	fnmadd.d f0, f0, f31, f0
	fnmadd.d f0, f0, f0, f31
	fnmadd.d f0, f0, f0, f0, rne

	fnmsub.d f31, f0, f0, f0
	fnmsub.d f0, f31, f0, f0
	fnmsub.d f0, f0, f31, f0
	fnmsub.d f0, f0, f0, f31
	fnmsub.d f0, f0, f0, f0, rne

	fsd	f31, (x0)
	fsd	f0, 0x1f(x0)
	fsd	f0, -0x20(x0)
	fsd	f0, (x31)
	fsd	f0, dval, x31

	fsgnj.d	f31, f0, f1
	fsgnj.d	f0, f31, f0
	fsgnj.d	f0, f0, f31
	fsgnjn.d f0, f1, f0
	fsgnjx.d f0, f1, f0

	fsqrt.d	f31, f0
	fsqrt.d	f0, f31
	fsqrt.d	f0, f0, rne

	fsub.d	f31, f0, f0
	fsub.d	f0, f31, f0
	fsub.d	f0, f0, f31
	fsub.d	f0, f0, f0, rne
