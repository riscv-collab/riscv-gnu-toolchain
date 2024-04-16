//Original:/testcases/core/c_pushpopmultiple_preg/c_pushpopmultiple_preg.dsp
// Spec Reference: pushpopmultiple preg
# mach: bfin

.include "testutils.inc"
	start

	FP = SP;

	imm32 r0, 0x00000000;
	ASTAT = r0;

	P1 = 0xa1 (X);
	P2 = 0xa2 (X);
	P3 = 0xa3 (X);
	P4 = 0xa4 (X);
	P5 = 0xa5 (X);
	[ -- SP ] = ( P5:1 );
	P1 = 0;
	P2 = 0;
	P3 = 0;
	P4 = 0;
	P5 = 0;
	( P5:1 ) = [ SP ++ ];
	CHECKREG p1, 0x000000a1;
	CHECKREG p2, 0x000000a2;
	CHECKREG p3, 0x000000a3;
	CHECKREG p4, 0x000000a4;
	CHECKREG p5, 0x000000a5;

	P2 = 0xb2 (X);
	P3 = 0xb3 (X);
	P4 = 0xb4 (X);
	P5 = 0xb5 (X);
	[ -- SP ] = ( P5:2 );
	P2 = 0;
	P3 = 0;
	P4 = 0;
	P5 = 0;
	( P5:2 ) = [ SP ++ ];
	CHECKREG p1, 0x000000a1;
	CHECKREG p2, 0x000000b2;
	CHECKREG p3, 0x000000b3;
	CHECKREG p4, 0x000000b4;
	CHECKREG p5, 0x000000b5;

	P3 = 0xc3 (X);
	P4 = 0xc4 (X);
	P5 = 0xc5 (X);
	[ -- SP ] = ( P5:3 );
	P3 = 0;
	P4 = 0;
	P5 = 0;
	( P5:3 ) = [ SP ++ ];
	CHECKREG p1, 0x000000a1;
	CHECKREG p2, 0x000000b2;
	CHECKREG p3, 0x000000c3;
	CHECKREG p4, 0x000000c4;
	CHECKREG p5, 0x000000c5;

	P4 = 0xd4 (X);
	P5 = 0xd5 (X);
	[ -- SP ] = ( P5:4 );
	P4 = 0;
	P5 = 0;
	( P5:4 ) = [ SP ++ ];
	CHECKREG p1, 0x000000a1;
	CHECKREG p2, 0x000000b2;
	CHECKREG p3, 0x000000c3;
	CHECKREG p4, 0x000000d4;
	CHECKREG p5, 0x000000d5;

	P5 = 0xe5 (X);
	[ -- SP ] = ( P5:5 );
	P5 = 0;
	( P5:5 ) = [ SP ++ ];
	CHECKREG p1, 0x000000a1;
	CHECKREG p2, 0x000000b2;
	CHECKREG p3, 0x000000c3;
	CHECKREG p4, 0x000000d4;
	CHECKREG p5, 0x000000e5;

	pass
