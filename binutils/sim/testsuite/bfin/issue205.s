# mach: bfin

.include "testutils.inc"
	start


	R0 = 0;	R1 = 0; R2 = 0; R3 = 0; R4 = 0; R5 = 0; R6 = 0; R7 = 0;
	P0 = 0;	P1 = 0; P2 = 0; P4 = 0; P5 = 0;
	I0 = 0 (X);	I1 = 0 (X); I2 = 0 (X); I3 = 0 (X);
	M0 = 0 (X);	M1 = 0 (X); M2 = 0 (X); M3 = 0 (X);
	L0 = 0 (X);	L1 = 0 (X); L2 = 0 (X); L3 = 0 (X);
	B0 = 0 (X);	B1 = 0 (X); B2 = 0 (X); B3 = 0 (X);

	R0 = -1;
	R1 = 0x1234 (X);
	R2 = -2000 (X);
	R3 = 2000 (X);
	R4 = 0;
	R5 = 1;
	R6 = 5555 (X);
	R7 = -1000 (X);

	loadsym P1, tmp0;
	loadsym P2, tmp1;
	loadsym P4, tmp2;

	I1 = P1;
	I2 = P2;
	I3 = P4;


	R0.L = 0x0017;
	R0.H = 0xffff;
	R0.L = EXPADJ( R2 , R1.L ) || [ P2 ] = R0 || NOP;
	R6 = [ P2 ];
	DBGA ( R6.L , 0x17 );
	DBGA ( R6.H , 0xffff );

	DBGA ( R0.L , 0x1234 );
	DBGA ( R0.H , 0xffff );

	pass

	.data
tmp0:
	.dd 0x12345678  // 0x1000
	.dd 0x10101010  // 0x1004
	.dd 0x55555555  // 0x1008
	.dd 0xaaaaaaaa  // 0x100c
	.dd 0xffffffff  // 0x1010

	.data
tmp1:
	.dd 0xabcdefef  // 0x2000
	.dd 0x12121212  // 0x2004
	.dd 0x45454545  // 0x2008
	.dd 0xabababab  // 0x200c
	.dd 0x0f0f0f0f  // 0x2010

	.data
tmp2:
	.dd 0xff00ff00  // 0x3000
	.dd 0x02020202  // 0x3004
	.dd 0x4f4f4f45  // 0x3008
	.dd 0xafafafaf  // 0x300c
	.dd 0x1f1f1f1f  // 0x3010
