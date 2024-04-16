//Original:/testcases/core/c_regmv_acc_acc/c_regmv_acc_acc.dsp
// Spec Reference: regmv acc-acc
# mach: bfin

.include "testutils.inc"
	start

	imm32 r0, 0xa9627911;
	imm32 r1, 0xd0158978;
	imm32 r2, 0xc1234567;
	imm32 r3, 0x10060007;
	imm32 r4, 0x02080009;
	imm32 r5, 0x003a000b;
	imm32 r6, 0x0004000d;
	imm32 r7, 0x000e500f;
	A0 = R0;

	A1 = A0;
	R2 = A1.w;
	R3 = A1.x;

	A1.x = A0.w;
	A1.w = A0.w;
	A0.x = A0.w;
	A0.w = A0.w;
	R4 = A0.w;
	R5 = A0.x;
	R6 = A1.w;
	R7 = A1.x;

	CHECKREG r0, 0xA9627911;
	CHECKREG r1, 0xD0158978;
	CHECKREG r2, 0xA9627911;
	CHECKREG r3, 0xFFFFFFFF;
	CHECKREG r4, 0xA9627911;
	CHECKREG r5, 0x00000011;
	CHECKREG r6, 0xA9627911;
	CHECKREG r7, 0x00000011;

	imm32 r0, 0x90ba7911;
	imm32 r1, 0xe3458978;
	imm32 r2, 0xc1234567;
	imm32 r3, 0x10060007;
	imm32 r4, 0x56080009;
	imm32 r5, 0x783a000b;
	imm32 r6, 0xf247890d;
	imm32 r7, 0x489e534f;
	A1 = R0;

	A0 = A1;
	R2 = A0.w;
	R3 = A0.x;

	A0.x = A1.w;
	A0.w = A1.w;
	A1.x = A1.w;
	A1.w = A1.w;
	R4 = A0.w;
	R5 = A0.x;
	R6 = A1.w;
	R7 = A1.x;
	CHECKREG r0, 0x90BA7911;
	CHECKREG r1, 0xE3458978;
	CHECKREG r2, 0x90BA7911;
	CHECKREG r3, 0xFFFFFFFF;
	CHECKREG r4, 0x90BA7911;
	CHECKREG r5, 0x00000011;
	CHECKREG r6, 0x90BA7911;
	CHECKREG r7, 0x00000011;

	imm32 r0, 0xf9627911;
	imm32 r1, 0xd0158978;
	imm32 r2, 0xc1234567;
	imm32 r3, 0x10060007;
	imm32 r4, 0x02080009;
	imm32 r5, 0x003a000b;
	imm32 r6, 0xf247890d;
	imm32 r7, 0x789e534f;
	A0 = R0;

	A0.x = A0.x;
	A0.w = A0.x;
	A1.w = A0.x;
	A1.x = A0.x;
	R4 = A0.w;
	R5 = A0.x;
	R6 = A1.w;
	R7 = A1.x;
	CHECKREG r0, 0xF9627911;
	CHECKREG r1, 0xD0158978;
	CHECKREG r2, 0xC1234567;
	CHECKREG r3, 0x10060007;
	CHECKREG r4, 0xFFFFFFFF;
	CHECKREG r5, 0xFFFFFFFF;
	CHECKREG r6, 0xFFFFFFFF;
	CHECKREG r7, 0xFFFFFFFF;

	imm32 r0, 0x90ba7911;
	imm32 r1, 0xe3458978;
	imm32 r2, 0xc1234567;
	imm32 r3, 0x10060007;
	imm32 r4, 0x56080009;
	imm32 r5, 0x783a000b;
	imm32 r6, 0xf247890d;
	imm32 r7, 0x489e534f;
	A1 = R0;

	A0.x = A1.x;
	A0.w = A1.x;
	A1.w = A1.x;
	A1.x = A1.x;
	R4 = A0.w;
	R5 = A0.x;
	R6 = A1.w;
	R7 = A1.x;
	CHECKREG r0, 0x90BA7911;
	CHECKREG r1, 0xE3458978;
	CHECKREG r2, 0xC1234567;
	CHECKREG r3, 0x10060007;
	CHECKREG r4, 0xFFFFFFFF;
	CHECKREG r5, 0xFFFFFFFF;
	CHECKREG r6, 0xFFFFFFFF;
	CHECKREG r7, 0xFFFFFFFF;

	pass
