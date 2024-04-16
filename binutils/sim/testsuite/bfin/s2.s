# mach: bfin

.include "testutils.inc"
	start

// Test pc relative indirect branches.
	P4 = 0;
	loadsym P1 jtab;

LL1:
	P2 = P1 + ( P4 << 1 );
	R0 = W [ P2 ] (Z);
	P0 = R0;
	R2 = P4;

jp:
	JUMP ( PC + P0 );

	DBGA ( R2.L , 0 );
	JUMP.L done;

	DBGA ( R2.L , 1 );
	JUMP.L done;

	DBGA ( R2.L , 2 );
	JUMP.L done;

	DBGA ( R2.L , 3 );
	JUMP.L done;

	DBGA ( R2.L , 4 );
	JUMP.L done;

done:
	P4 += 1;
	CC = P4 < 4 (IU);
	IF CC JUMP LL1;
	pass

	.data

jtab:
	.dw 2;	//.dw (2+0*8)
	.dw 10;	//.dw (2+1*8)
	.dw 18;	//.dw (2+2*8)
	.dw 26;	//.dw (2+3*8)
	.dw 34;	//.dw (2+4*8)
