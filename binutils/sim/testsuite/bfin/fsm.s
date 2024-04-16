# mach: bfin

.include "testutils.inc"
	start


	R1 = 0;
	R0 = R1;
	R7 = 7;
L$10:
	CC = R0 == 1;
	IF CC JUMP L$14;
	CC = R0 <= 1;
	IF !CC JUMP L$30;
	CC = R0 == 0;
	IF CC JUMP L$12;
	JUMP.S L$25;
L$30:
	CC = R0 == R7;
	IF CC JUMP L$16;
	R5 = 17;
	CC = R0 == R5;
	IF CC JUMP L$23;
	JUMP.S L$25;
L$12:
	R1 += 5;
	R0 = 1;
	JUMP.S L$8;
L$14:
	R1 <<= 4;
	R0 = 4;
	JUMP.S L$8;
L$16:
	CC = BITTST ( R1 , 3 );
	IF CC JUMP L$17;
	BITSET( R1 , 3 );
	R0 = 4;
	JUMP.S L$20;
L$17:
	BITSET( R1 , 5 );
	R0 = 14;
L$20:
	JUMP.S L$8;
L$23:
	R5 = 13;
	R1 = R1 ^ R5;
	R0 = 20;
	JUMP.S L$8;
L$25:
	R1 += 1;
	R0 += 1;
L$8:
	R5 = 19;
	CC = R0 <= R5;
	IF CC JUMP L$10 (BP);
	DBGA ( R0.L , 20 );	DBGA ( R1.L , 140 );
	pass
