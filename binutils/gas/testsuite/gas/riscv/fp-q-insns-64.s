Q:
	fabs.q	f31, f0
	fabs.q	f0, f31

	fadd.q	f31, f0, f0
	fadd.q	f0, f31, f0
	fadd.q	f0, f0, f31
	fadd.q	f0, f0, f0, rne
	fadd.q	f0, f0, f0, rtz
	fadd.q	f0, f0, f0, rdn
	fadd.q	f0, f0, f0, rup
	fadd.q	f0, f0, f0, rmm

	fclass.q x31, f0
	fclass.q x0, f31

	fcvt.d.q f31, f0
	fcvt.d.q f0, f31
	fcvt.d.q f0, f0, rne
	fcvt.l.q x0, f0
	fcvt.l.q x0, f0, rne
	fcvt.lu.q x0, f0
	fcvt.q.d f31, f0
	fcvt.q.d f0, f31
	fcvt.q.l f0, x0
	fcvt.q.lu f0, x0
	fcvt.q.s f31, f0
	fcvt.q.s f0, f31
	fcvt.q.w f31, x0
	fcvt.q.w f0, x31
	fcvt.q.wu f0, x0
	fcvt.s.q f31, f0
	fcvt.s.q f0, f31
	fcvt.s.q f0, f0, rne
	fcvt.w.q x31, f0
	fcvt.w.q x0, f31
	fcvt.w.q x0, f0, rne
	fcvt.wu.q x0, f0

	fdiv.q	f31, f0, f0
	fdiv.q	f0, f31, f0
	fdiv.q	f0, f0, f31
	fdiv.q	f0, f0, f0, rne

	feq.q	x31, f0, f0
	feq.q	x0, f31, f0
	feq.q	x0, f0, f31

	fge.q	x31, f0, f0
	fge.q	x0, f31, f0
	fge.q	x0, f0, f31

	fgt.q	x31, f0, f0
	fgt.q	x0, f31, f0
	fgt.q	x0, f0, f31

	fle.q	x31, f0, f0
	fle.q	x0, f31, f0
	fle.q	x0, f0, f31

	flq	f31, (x0)
	flq	f0, 0x7ff(x0)
	flq	f0, -0x800(x0)
	flq	f0, (x31)
	flq	f0, qvar, x31

	flt.q	x31, f0, f0
	flt.q	x0, f31, f0
	flt.q	x0, f0, f31

	fmadd.q	f31, f0, f0, f0
	fmadd.q	f0, f31, f0, f0
	fmadd.q	f0, f0, f31, f0
	fmadd.q	f0, f0, f0, f31
	fmadd.q	f0, f0, f0, f0, rne

	fmax.q	f31, f0, f0
	fmax.q	f0, f31, f0
	fmax.q	f0, f0, f31

	fmin.q	f31, f0, f0
	fmin.q	f0, f31, f0
	fmin.q	f0, f0, f31

	fmsub.q	f31, f0, f0, f0
	fmsub.q	f0, f31, f0, f0
	fmsub.q	f0, f0, f31, f0
	fmsub.q	f0, f0, f0, f31
	fmsub.q	f0, f0, f0, f0, rne

	fmul.q	f31, f0, f0
	fmul.q	f0, f31, f0
	fmul.q	f0, f0, f31
	fmul.q	f0, f0, f0, rne

	fmv.q	f31, f0
	fmv.q	f0, f31

	fneg.q	f31, f0
	fneg.q	f0, f31

	fnmadd.q f31, f0, f0, f0
	fnmadd.q f0, f31, f0, f0
	fnmadd.q f0, f0, f31, f0
	fnmadd.q f0, f0, f0, f31
	fnmadd.q f0, f0, f0, f0, rne

	fnmsub.q f0, f0, f0, f0
	fnmsub.q f0, f31, f0, f0
	fnmsub.q f0, f0, f31, f0
	fnmsub.q f0, f0, f0, f31
	fnmsub.q f0, f0, f0, f0, rne

	fsgnj.q	f31, f0, f1
	fsgnj.q	f0, f31, f0
	fsgnj.q	f0, f0, f31
	fsgnjn.q f0, f1, f0
	fsgnjx.q f0, f1, f0

	fsq	f31, (x0)
	fsq	f0, 0x1f(x0)
	fsq	f0, -0x20(x0)
	fsq	f0, (x31)
	fsq	f0, qvar, x31

	fsqrt.q	f31, f0
	fsqrt.q	f0, f31
	fsqrt.q	f0, f0, rne

	fsub.q	f31, f0, f0
	fsub.q	f0, f31, f0
	fsub.q	f0, f0, f31
	fsub.q	f0, f0, f0, rne
