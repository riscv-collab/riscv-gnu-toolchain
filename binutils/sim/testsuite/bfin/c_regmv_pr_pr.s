//Original:/testcases/core/c_regmv_pr_pr/c_regmv_pr_pr.dsp
// Spec Reference: regmv preg-to-preg
# mach: bfin

.include "testutils.inc"
	start

// check p-reg to p-reg move
	imm32 p1, 0x20021003;
	imm32 p2, 0x20041005;
	imm32 p4, 0x20081009;
	imm32 p5, 0x200a100b;
	imm32 fp, 0x200e100f;

	imm32 p1, 0x20021003;
	imm32 p2, 0x20041005;
	imm32 p4, 0x20081009;
	imm32 p5, 0x200a100b;
	imm32 fp, 0x200e100f;
	P1 = P1;
	P2 = P1;
	P4 = P1;
	P5 = P1;
	FP = P1;
	CHECKREG p1, 0x20021003;
	CHECKREG p2, 0x20021003;
	CHECKREG p4, 0x20021003;
	CHECKREG p5, 0x20021003;
	CHECKREG fp, 0x20021003;

	imm32 p1, 0x20021003;
	imm32 p2, 0x20041005;
	imm32 p4, 0x20081009;
	imm32 p5, 0x200a100b;
	imm32 fp, 0x200e100f;
	P1 = P2;
	P2 = P2;
	P4 = P2;
	P5 = P2;
	FP = P2;
	CHECKREG p1, 0x20041005;
	CHECKREG p2, 0x20041005;
	CHECKREG p4, 0x20041005;
	CHECKREG p5, 0x20041005;
	CHECKREG fp, 0x20041005;

	imm32 p1, 0x20021003;
	imm32 p2, 0x20041005;
	imm32 p4, 0x20081009;
	imm32 p5, 0x200a100b;
	imm32 fp, 0x200e100f;
	P1 = P4;
	P2 = P4;
	P4 = P4;
	P5 = P4;
	FP = P4;
	CHECKREG p1, 0x20081009;
	CHECKREG p2, 0x20081009;
	CHECKREG p4, 0x20081009;
	CHECKREG p5, 0x20081009;
	CHECKREG fp, 0x20081009;

	imm32 p1, 0x20021003;
	imm32 p2, 0x20041005;
	imm32 p4, 0x20081009;
	imm32 p5, 0x200a100b;
	imm32 fp, 0x200e100f;
	P1 = P5;
	P2 = P5;
	P4 = P5;
	P5 = P5;
	FP = P5;
	CHECKREG p1, 0x200a100b;
	CHECKREG p2, 0x200a100b;
	CHECKREG p4, 0x200a100b;
	CHECKREG p5, 0x200a100b;
	CHECKREG fp, 0x200a100b;

	imm32 p1, 0x20021003;
	imm32 p2, 0x20041005;
	imm32 p4, 0x20081009;
	imm32 p5, 0x200a100b;
	imm32 fp, 0x200e100f;
	P1 = FP;
	P2 = FP;
	P4 = FP;
	P5 = FP;
	FP = FP;
	CHECKREG p1, 0x200e100f;
	CHECKREG p2, 0x200e100f;
	CHECKREG p4, 0x200e100f;
	CHECKREG p5, 0x200e100f;
	CHECKREG fp, 0x200e100f;

	pass
