# mach: bfin

.include "testutils.inc"
	start


	r0.l = 0x1111;
	r0.h = 0x0011;
	r1.l = 0x2222;
	r1.h = 0x0022;
	r2.l = 0x3333;
	r2.h = 0x0033;
	r3.l = 0x4444;
	r3.h = 0x0044;
	r4.l = 0x5555;
	r4.h = 0x0055;
	r5.l = 0x6666;
	r5.h = 0x0066;
	r6.l = 0x7777;
	r6.h = 0x0077;
	r7.l = 0x8888;
	r7.h = 0x0088;
	p1.l = 0x5a5a;
	p1.h = 0x005a;
	p2.l = 0x6363;
	p2.h = 0x0063;
	p3.l = 0x7777;
	p3.h = 0x0077;
	p4.l = 0x7878;
	p4.h = 0x0078;
	p5.l = 0x3e3e;
	p5.h = 0x003e;
	sp = 0x4000(x);

	jump.s prog_start;

	nop;
	nop;	// ADD reg update to roll back
	nop;

prog_start:
	nop;
	[--sp] = r0;
	[--sp] = r1;
	[--sp] = r2;
	[--sp] = r3;
	[--sp] = r4;
	[--sp] = r5;
	[--sp] = r6;
	[--sp] = r7;
	[--sp] = p0;
	[--sp] = p1;
	[--sp] = p2;
	[--sp] = p3;
	[--sp] = p4;
	[--sp] = p5;

	nop;
	nop;
	nop;
	nop;
	r0.l = 0xdead;
	r0.h = 0xdead;
	r1.l = 0xdead;
	r1.h = 0xdead;
	r2.l = 0xdead;
	r2.h = 0xdead;
	r3.l = 0xdead;
	r3.h = 0xdead;
	r4.l = 0xdead;
	r4.h = 0xdead;
	r5.l = 0xdead;
	r5.h = 0xdead;
	r6.l = 0xdead;
	r6.h = 0xdead;
	r7.l = 0xdead;
	r7.h = 0xdead;
	p1.l = 0xdead;
	p1.h = 0xdead;
	p2.l = 0xdead;
	p2.h = 0xdead;
	p3.l = 0xdead;
	p3.h = 0xdead;
	p4.l = 0xdead;
	p4.h = 0xdead;
	p5.l = 0xdead;
	p5.h = 0xdead;
	nop;
	nop;
	nop;
	r0 = [sp++];
	r1 = [sp++];
	r2 = [sp++];
	r3 = [sp++];
	r4 = [sp++];
	r5 = [sp++];
	r6 = [sp++];
	r7 = [sp++];
	p0 = [sp++];
	p1 = [sp++];
	p2 = [sp++];
	p3 = [sp++];
	p4 = [sp++];
	p5 = [sp++];

	nop;
	nop;
	nop;
	nop;
	nop;
	nop;
	nop;
_tp1:
	nop;
	nop;
	nop;
	nop;
	nop;
	nop;
	nop;
	[--sp] = r0;
	[--sp] = r1;
	[--sp] = r2;
	[--sp] = r3;
	[--sp] = r4;
	[--sp] = r5;
	[--sp] = r6;
	[--sp] = r7;
	[--sp] = p0;
	[--sp] = p1;
	[--sp] = p2;
	[--sp] = p3;
	[--sp] = p4;
	[--sp] = p5;

	nop;
	nop;
	nop;
	nop;
	r0.l = 0xdead;
	r0.h = 0xdead;
	r1.l = 0xdead;
	r1.h = 0xdead;
	r2.l = 0xdead;
	r2.h = 0xdead;
	r3.l = 0xdead;
	r3.h = 0xdead;
	r4.l = 0xdead;
	r4.h = 0xdead;
	r5.l = 0xdead;
	r5.h = 0xdead;
	r6.l = 0xdead;
	r6.h = 0xdead;
	r7.l = 0xdead;
	r7.h = 0xdead;
	p1.l = 0xdead;
	p1.h = 0xdead;
	p2.l = 0xdead;
	p2.h = 0xdead;
	p3.l = 0xdead;
	p3.h = 0xdead;
	p4.l = 0xdead;
	p4.h = 0xdead;
	p5.l = 0xdead;
	p5.h = 0xdead;
	nop;
	nop;
	nop;
	r0 = [sp++];
	r1 = [sp++];
	r2 = [sp++];
	r3 = [sp++];
	r4 = [sp++];
	r5 = [sp++];
	r6 = [sp++];
	r7 = [sp++];
	p0 = [sp++];
	p1 = [sp++];
	a0.x = [sp++];

	a1.w = r0;	//preserve r0

	r0 = a0.x;
	DBGA(r0.l,0x0063);

	a0.w = [sp++];
	r0 = a0.w;
	DBGA(r0.l,0x7777);
	DBGA(r0.h,0x0077);

	a0 = a1;	//perserver r0, still

	a1.x = [sp++];
	r0 = a1.x;
	DBGA(r0.l,0x0078);

	a1.w = [sp++];
	r0 = a1.w;
	DBGA(r0.l,0x3e3e);
	DBGA(r0.h,0x003e);

	r0 = a0.w;	//restore r0

	nop;
	nop;
	nop;
	nop;
	nop;
	nop;
	nop;
_tp2:
	nop;
	nop;
	nop;
	[--sp] = r0;
	[--sp] = r1;
	[--sp] = r2;
	[--sp] = r3;
	[--sp] = a0.x;
	[--sp] = a0.w;
	[--sp] = a1.x;
	[--sp] = a1.w;
	[--sp] = p0;
	[--sp] = p1;
	[--sp] = p2;
	[--sp] = p3;
	[--sp] = p4;
	[--sp] = p5;

	nop;
	nop;
	nop;
	nop;
	r0.l = 0xdead;
	r0.h = 0xdead;
	r1.l = 0xdead;
	r1.h = 0xdead;
	r2.l = 0xdead;
	r2.h = 0xdead;
	r3.l = 0xdead;
	r3.h = 0xdead;
	r4.l = 0xdead;
	r4.h = 0xdead;
	r5.l = 0xdead;
	r5.h = 0xdead;
	r6.l = 0xdead;
	r6.h = 0xdead;
	r7.l = 0xdead;
	r7.h = 0xdead;
	p1.l = 0xdead;
	p1.h = 0xdead;
	p2.l = 0xdead;
	p2.h = 0xdead;
	p3.l = 0xdead;
	p3.h = 0xdead;
	p4.l = 0xdead;
	p4.h = 0xdead;
	p5.l = 0xdead;
	p5.h = 0xdead;
	nop;
	nop;
	nop;
	r0 = [sp++];
	r1 = [sp++];
	r2 = [sp++];
	r3 = [sp++];
	r4 = [sp++];
	r5 = [sp++];
	r6 = [sp++];
	r7 = [sp++];
	p0 = [sp++];
	p1 = [sp++];
	p2 = [sp++];
	p3 = [sp++];
	p4 = [sp++];
	p5 = [sp++];

	nop;
	nop;
	nop;
	nop;
	nop;
	nop;
	nop;
_tp3:
	nop;
	nop;
	nop;
	nop;
	nop;
_halt:
	pass;
