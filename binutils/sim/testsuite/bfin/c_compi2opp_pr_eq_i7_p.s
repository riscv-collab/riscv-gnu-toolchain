//Original:/proj/frio/dv/testcases/core/c_compi2opp_pr_eq_i7_p/c_compi2opp_pr_eq_i7_p.dsp
// Spec Reference: compi2opd pregs = imm7 positive
# mach: bfin

.include "testutils.inc"
	start

//R0  = 0;
	P1 = 1;
	P2 = 2;
	P3 = 3;
	P4 = 4;
	P5 = 5;
	SP = 6;
	FP = 7;
	CHECKREG p1,  1;
	CHECKREG p2,  2;
	CHECKREG p3,  3;
	CHECKREG p4,  4;
	CHECKREG p5,  5;
	CHECKREG sp,  6;
	CHECKREG fp,  7;

	P1 = 9;
	P2 = 10;
	P3 = 11;
	P4 = 12;
	P5 = 13;
	SP = 14;
	FP = 15;
	CHECKREG p1,  9;
	CHECKREG p2,  10;
	CHECKREG p3,  11;
	CHECKREG p4,  12;
	CHECKREG p5,  13;
	CHECKREG sp,  14;
	CHECKREG fp,  15;

	P1 = 17;
	P2 = 18;
	P3 = 19;
	P4 = 20;
	P5 = 21;
	SP = 22;
	FP = 23;
	CHECKREG p1,  17;
	CHECKREG p2,  18;
	CHECKREG p3,  19;
	CHECKREG p4,  20;
	CHECKREG p5,  21;
	CHECKREG sp,  22;
	CHECKREG fp,  23;

	P1 = 25;
	P2 = 26;
	P3 = 27;
	P4 = 28;
	P5 = 29;
	SP = 30;
	FP = 31;
	CHECKREG p1,  25;
	CHECKREG p2,  26;
	CHECKREG p3,  27;
	CHECKREG p4,  28;
	CHECKREG p5,  29;
	CHECKREG sp,  30;
	CHECKREG fp,  31;

	R0 = 32;
	P1 = 33;
	P2 = 34;
	P3 = 35;
	P4 = 36;
	P5 = 37;
	SP = 38;
	FP = 39;
	CHECKREG r0,  32;
	CHECKREG p1,  33;
	CHECKREG p2,  34;
	CHECKREG p3,  35;
	CHECKREG p4,  36;
	CHECKREG p5,  37;
	CHECKREG sp,  38;
	CHECKREG fp,  39;

	P1 = 41;
	P2 = 42;
	P3 = 43;
	P4 = 44;
	P5 = 45;
	SP = 46;
	FP = 47;
	CHECKREG p1,  41;
	CHECKREG p2,  42;
	CHECKREG p3,  43;
	CHECKREG p4,  44;
	CHECKREG p5,  45;
	CHECKREG sp,  46;
	CHECKREG fp,  47;

	P1 = 49;
	P2 = 50;
	P3 = 51;
	P4 = 52;
	P5 = 53;
	SP = 54;
	FP = 55;
	CHECKREG p1,  49;
	CHECKREG p2,  50;
	CHECKREG p3,  51;
	CHECKREG p4,  52;
	CHECKREG p5,  53;
	CHECKREG sp,  54;
	CHECKREG fp,  55;

	P1 = 57;
	P2 = 58;
	P3 = 59;
	P4 = 60;
	P5 = 61;
	SP = 62;
	FP = 63;
	CHECKREG p1,  57;
	CHECKREG p2,  58;
	CHECKREG p3,  59;
	CHECKREG p4,  60;
	CHECKREG p5,  61;
	CHECKREG sp,  62;
	CHECKREG fp,  63;

	pass
