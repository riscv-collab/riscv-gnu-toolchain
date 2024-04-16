//  Test result extraction of mac instructions.
//  Test basic edge values
//  SIGNED FRACTIONAL mode into SINGLE destination register
//  test ops: "+="
# mach: bfin

.include "testutils.inc"
	start


// load r0=0x80007fff
// load r1=0x80007fff
// load r2=0xf0000000
// load r3=0x0000007f
// load r4=0x00000080
	loadsym p0, data0;
	R0 = [ P0 ++ ];
	R1 = [ P0 ++ ];
	R2 = [ P0 ++ ];
	R3 = [ P0 ++ ];
	R4 = [ P0 ++ ];

// simple extraction with no saturation
// 0x7fff * 0x7fff = 0x007ffe0002 -> 0x7ffe
	R7 = 0;
	ASTAT = R7;
	A1 = A0 = 0;
	R5.H = (A1 += R0.L * R1.L), R5.L = (A0 += R0.L * R1.L);
	DBGA ( R5.L , 0x7ffe );
	DBGA ( R5.H , 0x7ffe );
	_DBG ASTAT;
	R7 = ASTAT;
	DBGA (R7.H, 0x0);
	DBGA (R7.L, 0x0);

// positive saturation at 32 bits
// 0x0 * 0x0 + 0x7ff0000000 -> 0x7fff
	R7 = 0;
	ASTAT = R7;
	A1 = A0 = 0;
	A1.w = R2;
	A1.x = R3.L;
	A0.x = R3.L;
	A0.w = R2;
	R5.H = (A1 += R0.L * R2.L), R5.L = (A0 += R0.L * R2.L);
	_DBG A1;
	_DBG A0;
	DBGA ( R5.L , 0x7fff );
	DBGA ( R5.H , 0x7fff );
	_DBG ASTAT;
	R7 = ASTAT;
	_DBG R7;
	DBGA (R7.H, 0x300);
	DBGA (R7.L, 0x8);

// positive saturation at 32 bits
// 0x7fff * 0x7fff + 0x7ff0000000 -> 0x7fff
	R7 = 0;
	ASTAT = R7;
	A1 = A0 = 0;
	A1.w = R2;
	A1.x = R3.L;
	A0.w = R2;
	A0.x = R3.L;
	R5.H = (A1 += R0.L * R1.L), R5.L = (A0 += R0.L * R1.L);
	DBGA ( R5.L , 0x7fff );
	DBGA ( R5.H , 0x7fff );
	_DBG ASTAT;
	R7 = ASTAT;
	DBGA (R7.H, 0x30f);
	DBGA (R7.L, 0x8);

// negative saturation at 32 bits
// 0x0 * 0x0 + 0x80f0000000 -> 0x8000
	R7 = 0;
	ASTAT = R7;
	A1 = A0 = 0;
	A1.w = R2;
	A1.x = R4.L;
	A0.w = R2;
	A0.x = R4.L;
	R5.H = (A1 += R0.L * R2.L), R5.L = (A0 += R0.L * R2.L);
	DBGA ( R5.L , 0x8000 );
	DBGA ( R5.H , 0x8000 );
	_DBG A1;
	_DBG A0;
	_DBG ASTAT;
	R7=ASTAT;
	_DBG R7;
	DBGA (R7.H, 0x300);
	DBGA (R7.L, 0x0008);

// negative saturation at 32 bits
// 0x7fff * 0x8000 + 0x80f0000000 -> 0x8000
	R7 = 0;
	ASTAT = R7;
	A1 = A0 = 0;
	A1.w = R2;
	A1.x = R4.L;
	A0.w = R2;
	A0.x = R4.L;
	R5.H = (A1 += R0.H * R1.L), R5.L = (A0 += R0.H * R1.L);
	DBGA ( R5.L , 0x8000 );
	DBGA ( R5.H , 0x8000 );
	R7=ASTAT;
	_DBG ASTAT;
	DBGA (R7.H, 0x300);
	DBGA (R7.L, 0x0008);

// negative saturation at 32 bits on MAC only
// 0x7fff * 0x8000 + 0x80f0000000 -> 0x8000
	R7 = 0;
	ASTAT = R7;
	A1 = A0 = 0;
	A0.w = R2;
	A0.x = R4.L;
	_DBG ASTAT;
	R5.H = A1, R5.L = (A0 += R0.H * R1.L);
	_DBG A0;
	DBGA ( R5.L , 0x8000 );
	DBGA ( R5.H , 0x0000 );
	R7=ASTAT;
	_DBG ASTAT;
	DBGA (R7.H, 0x300);
	DBGA (R7.L, 0x0009);

// 0x0100 * 0x0100 = 0x00020000 -> 0x0002
	R7 = 0;
	ASTAT = R7;
	R0.L = 0x0100;
	R1.L = 0x0100;
	A1 = A0 = 0;
	R5.H = (A1 = R0.L * R1.L), R5.L = (A0 = R0.L * R1.L) (T);
	DBGA ( R5.L , 0x0002 );
	DBGA ( R5.H , 0x0002 );
	R7 = ASTAT;
	DBGA (R7.H, 0x000);
	DBGA (R7.L, 0x000);

	pass

	.data
data0:
	.dw 0x7fff
	.dw 0x8000
	.dw 0x7fff
	.dw 0x8000
	.dw 0x0000
	.dw 0xf000
	.dw 0x007f
	.dw 0x0000
	.dw 0x0080
	.dw 0x0000
