# mach: bfin

.include "testutils.inc"
	start

// create two short vectors v_a, v_b
//   where each element of v_a is the index
//   where each element of v_b is 128-index
	R2 = 0;
	loadsym P0, v_a;
	loadsym P1, v_b;
	P2 = 0;
	R3 = 128 (X);
	R0 = 0;
	R1 = 128 (X);
L$1:
	W [ P0 ++ ] = R0;
	W [ P1 ++ ] = R1;
	R0 += 1;
	R1 += -1;
	CC = R0 < R3;
	IF CC JUMP L$1 (BP);

	loadsym P0, v_a;
	loadsym P1, v_b;

	CALL vecadd;

	loadsym P0, v_c;
	R2 = 0;
	R3 = 128 (X);
L$3:
	R0 = W [ P0 ++ ] (X);
	DBGA ( R0.L , 128 );
	R2 += 1;
	CC = R2 < R3;
	IF CC JUMP L$3;
	_DBG R6;
	pass

vecadd:

	loadsym I0, v_a;
	loadsym I1, v_b;
	loadsym I2, v_c;

	P5 = 128 (X);
	LSETUP ( L$2 , L$2end ) LC0 = P5 >> 1;
	R0 = [ I0 ++ ];
	R1 = [ I1 ++ ];
L$2:
	R2 = R0 +|+ R1 || R0 = [ I0 ++ ] || R1 = [ I1 ++ ];
L$2end:
	[ I2 ++ ] = R2;


	RTS;

	.data
v_a:
	.space (512);
v_b:
	.space (512);
v_c:
	.space (512);
