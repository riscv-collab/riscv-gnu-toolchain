//Original:/proj/frio/dv/testcases/core/c_ccmv_ncc_pr_pr/c_ccmv_ncc_pr_pr.dsp
// Spec Reference: ccmv !cc preg = preg
# mach: bfin

.include "testutils.inc"
	start

	R0 = 0;
	ASTAT = R0;

	imm32 p1, 0xd0021053;
	imm32 p2, 0x2f041405;
	imm32 p3, 0x60b61507;
	imm32 p4, 0x50487609;
	imm32 p5, 0x3005900b;
	imm32 sp, 0x2a0c660d;
	imm32 fp, 0xd90e108f;
	IF !CC P3 = P3;
	IF !CC P1 = P3;
	CC = ! CC;
	IF !CC P2 = P5;
	IF !CC P3 = P2;
	IF !CC P4 = SP;
	IF !CC P5 = P1;
	IF !CC SP = FP;
	CC = ! CC;
	IF !CC FP = P4;
	CHECKREG p1, 0x60B61507;
	CHECKREG p2, 0x2F041405;
	CHECKREG p3, 0x60B61507;
	CHECKREG p4, 0x50487609;
	CHECKREG p5, 0x3005900B;
	CHECKREG sp, 0x2A0C660D;
	CHECKREG fp, 0x50487609;

	imm32 p1, 0xd4023053;
	imm32 p2, 0x2f041405;
	imm32 p3, 0x60f61507;
	imm32 p4, 0xd0487f09;
	imm32 p5, 0x300b900b;
	imm32 sp, 0x2a0cd60d;
	imm32 fp, 0xd90e189f;
	IF !CC P4 = P3;
	IF !CC P5 = FP;
	CC = ! CC;
	IF !CC SP = P1;
	IF !CC FP = P2;
	IF !CC P3 = SP;
	IF !CC P1 = P5;
	IF !CC P2 = P4;
	CC = ! CC;
	IF !CC P3 = P2;
	CHECKREG p1, 0xD4023053;
	CHECKREG p2, 0x2F041405;
	CHECKREG p3, 0x2F041405;
	CHECKREG p4, 0x60F61507;
	CHECKREG p5, 0xD90E189F;
	CHECKREG sp, 0x2A0CD60D;
	CHECKREG fp, 0xD90E189F;

	imm32 p1, 0xd8021053;
	imm32 p2, 0x2f041405;
	imm32 p3, 0x65b61507;
	imm32 p4, 0x59487609;
	imm32 p5, 0x3005900b;
	imm32 sp, 0x2abc660d;
	imm32 fp, 0xd90e108f;
	IF !CC P3 = P2;
	IF !CC P1 = P3;
	CC = ! CC;
	IF !CC P2 = P5;
	IF !CC P3 = FP;
	IF !CC P4 = P1;
	IF !CC P5 = P4;
	IF !CC SP = FP;
	CC = ! CC;
	IF !CC FP = SP;
	CHECKREG p1, 0x2F041405;
	CHECKREG p2, 0x2F041405;
	CHECKREG p3, 0x2F041405;
	CHECKREG p4, 0x59487609;
	CHECKREG p5, 0x3005900B;
	CHECKREG sp, 0x2ABC660D;
	CHECKREG fp, 0x2ABC660D;

	imm32 p1, 0xdb021053;
	imm32 p2, 0x2f041405;
	imm32 p3, 0x64b61507;
	imm32 p4, 0x50487609;
	imm32 p5, 0x30f5900b;
	imm32 sp, 0x2a4c660d;
	imm32 fp, 0x895e108f;
	IF !CC P4 = P3;
	IF !CC P5 = FP;
	IF !CC SP = P2;
	IF !CC FP = SP;
	CC = ! CC;
	IF !CC P3 = P1;
	IF !CC P1 = P2;
	CC = ! CC;
	IF !CC P2 = P3;
	IF !CC P3 = P4;
	CHECKREG p1, 0xDB021053;
	CHECKREG p2, 0x64B61507;
	CHECKREG p3, 0x64B61507;
	CHECKREG p4, 0x64B61507;
	CHECKREG p5, 0x895E108F;
	CHECKREG sp, 0x2F041405;
	CHECKREG fp, 0x2F041405;

	pass
