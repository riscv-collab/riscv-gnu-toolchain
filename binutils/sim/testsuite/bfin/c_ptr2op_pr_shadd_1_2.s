//Original:/proj/frio/dv/testcases/core/c_ptr2op_pr_shadd_1_2/c_ptr2op_pr_shadd_1_2.dsp
// Spec Reference:  ptr2op shadd  preg, pregs, 1 (2)
# mach: bfin

.include "testutils.inc"
	start

	R0 = 0;
	ASTAT = R0;

// check p-reg to p-reg move

	imm32 p1, 0xf0921203;
	imm32 p2, 0xbe041305;
	imm32 p3, 0xd0d61407;
	imm32 p4, 0xa00a1089;
	imm32 p5, 0x400a300b;
	imm32 sp, 0xe07c180d;
	imm32 fp, 0x206e109f;
	P1 = ( P1 + P1 ) << 2;
	P2 = ( P2 + P1 ) << 2;
	P3 = ( P3 + P1 ) << 2;
	P4 = ( P4 + P1 ) << 1;
	P5 = ( P5 + P1 ) << 2;
	SP = ( SP + P1 ) << 2;
	FP = ( FP + P1 ) << 1;
	CHECKREG p1, 0x84909018;
	CHECKREG p2, 0x0A528C74;
	CHECKREG p3, 0x559A907C;
	CHECKREG p4, 0x49354142;
	CHECKREG p5, 0x126B008C;
	CHECKREG sp, 0x9432A094;
	CHECKREG fp, 0x49FD416E;

	imm32 p1, 0x50021003;
	imm32 p2, 0x26041005;
	imm32 p3, 0x60761007;
	imm32 p4, 0x20081009;
	imm32 p5, 0xf00a900b;
	imm32 sp, 0xb00c1a0d;
	imm32 fp, 0x200e10bf;
	P1 = ( P1 + P2 ) << 1;
	P2 = ( P2 + P2 ) << 2;
	P3 = ( P3 + P2 ) << 1;
	P4 = ( P4 + P2 ) << 2;
	P5 = ( P5 + P2 ) << 2;
	SP = ( SP + P2 ) << 1;
	FP = ( FP + P2 ) << 2;
	CHECKREG p1, 0xEC0C4010;
	CHECKREG p2, 0x30208028;
	CHECKREG p3, 0x212D205E;
	CHECKREG p4, 0x40A240C4;
	CHECKREG p5, 0x80AC40CC;
	CHECKREG sp, 0xC059346A;
	CHECKREG fp, 0x40BA439C;

	imm32 p1, 0x30026003;
	imm32 p2, 0x40051005;
	imm32 p3, 0x20e65057;
	imm32 p4, 0x2d081089;
	imm32 p5, 0xf00ab07b;
	imm32 sp, 0x200c1b0d;
	imm32 fp, 0x200e100f;
	P1 = ( P1 + P3 ) << 2;
	P2 = ( P2 + P3 ) << 1;
	P3 = ( P3 + P3 ) << 2;
	P4 = ( P4 + P3 ) << 2;
	P5 = ( P5 + P3 ) << 2;
	SP = ( SP + P3 ) << 1;
	FP = ( FP + P3 ) << 2;
	CHECKREG p1, 0x43A2C168;
	CHECKREG p2, 0xC1D6C0B8;
	CHECKREG p3, 0x073282B8;
	CHECKREG p4, 0xD0EA4D04;
	CHECKREG p5, 0xDCF4CCCC;
	CHECKREG sp, 0x4E7D3B8A;
	CHECKREG fp, 0x9D024B1C;

	imm32 p1, 0xa0021003;
	imm32 p2, 0x2c041005;
	imm32 p3, 0x40b61007;
	imm32 p4, 0x250d1009;
	imm32 p5, 0x260ae00b;
	imm32 sp, 0x700c110d;
	imm32 fp, 0x900e104f;
	P1 = ( P1 + P4 ) << 1;
	P2 = ( P2 + P4 ) << 2;
	P3 = ( P3 + P4 ) << 2;
	P4 = ( P4 + P4 ) << 2;
	P5 = ( P5 + P4 ) << 1;
	SP = ( SP + P4 ) << 2;
	FP = ( FP + P4 ) << 2;
	CHECKREG p1, 0x8A1E4018;
	CHECKREG p2, 0x44448038;
	CHECKREG p3, 0x970C8040;
	CHECKREG p4, 0x28688048;
	CHECKREG p5, 0x9CE6C0A6;
	CHECKREG sp, 0x61D24554;
	CHECKREG fp, 0xE1DA425C;

	imm32 p1, 0xae021003;
	imm32 p2, 0x22041705;
	imm32 p3, 0x20361487;
	imm32 p4, 0x90743009;
	imm32 p5, 0xa60aa00b;
	imm32 sp, 0xb00c1b0d;
	imm32 fp, 0x200e10cf;
	P1 = ( P1 + P5 ) << 2;
	P2 = ( P2 + P5 ) << 2;
	P3 = ( P3 + P5 ) << 2;
	P4 = ( P4 + P5 ) << 2;
	P5 = ( P5 + P5 ) << 1;
	SP = ( SP + P5 ) << 2;
	FP = ( FP + P5 ) << 2;
	CHECKREG p1, 0x5032C038;
	CHECKREG p2, 0x203ADC40;
	CHECKREG p3, 0x1902D248;
	CHECKREG p4, 0xD9FB4050;
	CHECKREG p5, 0x982A802C;
	CHECKREG sp, 0x20DA6CE4;
	CHECKREG fp, 0xE0E243EC;

	imm32 p1, 0x50021003;
	imm32 p2, 0x62041005;
	imm32 p3, 0x70e61007;
	imm32 p4, 0x290f1009;
	imm32 p5, 0x700ab00b;
	imm32 sp, 0x2a0c1d0d;
	imm32 fp, 0xb00e1e0f;
	P1 = ( P1 + SP ) << 2;
	P2 = ( P2 + SP ) << 1;
	P3 = ( P3 + SP ) << 2;
	P4 = ( P4 + SP ) << 2;
	P5 = ( P5 + SP ) << 2;
	SP = ( SP + SP ) << 1;
	FP = ( FP + SP ) << 2;
	CHECKREG p1, 0xE838B440;
	CHECKREG p2, 0x18205A24;
	CHECKREG p3, 0x6BC8B450;
	CHECKREG p4, 0x4C6CB458;
	CHECKREG p5, 0x685B3460;
	CHECKREG sp, 0xA8307434;
	CHECKREG fp, 0x60FA490C;

	imm32 p1, 0x32002003;
	imm32 p2, 0x24004005;
	imm32 p3, 0xe0506007;
	imm32 p4, 0xd0068009;
	imm32 p5, 0x230ae00b;
	imm32 sp, 0x205c1f0d;
	imm32 fp, 0x200e10bf;
	P1 = ( P1 + FP ) << 2;
	P2 = ( P2 + FP ) << 1;
	P3 = ( P3 + FP ) << 2;
	P4 = ( P4 + FP ) << 2;
	P5 = ( P5 + FP ) << 2;
	SP = ( SP + FP ) << 2;
	FP = ( FP + FP ) << 2;
	CHECKREG p1, 0x4838C308;
	CHECKREG p2, 0x881CA188;
	CHECKREG p3, 0x0179C318;
	CHECKREG p4, 0xC0524320;
	CHECKREG p5, 0x0C63C328;
	CHECKREG sp, 0x01A8BF30;
	CHECKREG fp, 0x007085F8;

	pass
