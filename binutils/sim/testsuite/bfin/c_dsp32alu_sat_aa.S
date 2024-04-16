//Original:/proj/frio/dv/testcases/core/c_dsp32alu_sat_aa/c_dsp32alu_sat_aa.dsp
// Spec Reference: dsp32alu sat ( a1, a0)
# mach: bfin

#include "test.h"
.include "testutils.inc"
	start

	R0 = 0;
	ASTAT = R0;
	A0 = A1 = 0;

	imm32 r0, 0xabefda8f;
	imm32 r1, 0x789abced;
	imm32 r2, 0x3b44b515;
	imm32 r3, 0x96667717;
	imm32 r4, 0x5567891b;
	imm32 r5, 0x6789ab1d;
	imm32 r6, 0xabcdef89;
	imm32 r7, 0xefadbc8a;
	A0 = R0;
	A1 = R1;
	A1 = A1 (S), A0 = A0 (S);
	R0 = ASTAT;
	R2 = A0.w;
	R3 = A1.w;

	A0 = R6;
	A1 = R7;
	A1 = A1 (S), A0 = A0 (S);
	R1 = ASTAT;
	R4 = A0.w;
	R5 = A1.w;
	CHECKREG r0, _AN;
	CHECKREG r1, _AN;
	CHECKREG r2, 0xABEFDA8F;
	CHECKREG r3, 0x789ABCED;
	CHECKREG r4, 0xABCDEF89;
	CHECKREG r5, 0xEFADBC8A;

	pass
