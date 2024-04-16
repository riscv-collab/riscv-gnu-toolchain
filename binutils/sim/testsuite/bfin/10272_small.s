# mach: bfin

.include "testutils.inc"
	start

	loadsym P5, tmp0;

	r6=0xFF (Z);
	W[p5+0x6] = r6;

	r0.l=0x0808;
	r0.h=0xffff;

	R1 = W[P5 + 0x6 ] (X);
	R0 = DEPOSIT(R1, R0);
	W[P5+0x6] = R0;

	R5=W[P5+0x6] (X);
	DBGA(r5.l,0xffff);

	/* This instruction order fails to successfully write R0 back */
	r0.l=0x0808;
	r0.h=0xffff;

	loadsym P5, tmp0;

	r6=0xFF (Z);
	W[p5+0x6] = r6;
	R1 = W[P5 + 0x6 ] (X);
	R0 = DEPOSIT(R1, R0);
	W[P5+0x6] = R0;

	R5=W[P5+0x6] (X);
	DBGA(r5.l,0xffff);

	r4=1;
	loadsym P5, tmp0;
	r6=0xFF (Z);
	W[p5+0x6] = r6;
	R1 = W[P5 + 0x6 ] (X);
	R0 = R1+R4;
	W[P5+0x6] = R0;

	R5=W[P5+0x6] (X);
	DBGA(r5.l,0x100);

	pass;

	.data
tmp0:
	.space (0x10);
