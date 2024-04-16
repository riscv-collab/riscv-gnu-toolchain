//Original:/testcases/core/c_dsp32alu_r_negneg/c_dsp32alu_r_negneg.dsp
// Spec Reference: dsp32alu dregs = neg / neg dregs
# mach: bfin

.include "testutils.inc"
	start




imm32 r0, 0xa5678911;
imm32 r1, 0x2789ab1d;
imm32 r2, 0x3b44b515;
imm32 r3, 0x46667717;
imm32 r4, 0x5567891b;
imm32 r5, 0x6789ab1d;
imm32 r6, 0x74445515;
imm32 r7, 0x86667777;
R0 = - R0 (V);
R1 = - R1 (V);
R2 = - R2 (V);
R3 = - R3 (V);
R4 = - R4 (V);
R5 = - R5 (V);
R6 = - R6 (V);
R7 = - R7 (V);
CHECKREG r0, 0x5A9976EF;
CHECKREG r1, 0xD87754E3;
CHECKREG r2, 0xC4BC4AEB;
CHECKREG r3, 0xB99A88E9;
CHECKREG r4, 0xAA9976E5;
CHECKREG r5, 0x987754E3;
CHECKREG r6, 0x8BBCAAEB;
CHECKREG r7, 0x799A8889;

imm32 r0, 0xa567892b;
imm32 r1, 0x2789ab2d;
imm32 r2, 0x344d5525;
imm32 r3, 0xd6667727;
imm32 r4, 0x58889929;
imm32 r5, 0x6aaabb2b;
imm32 r6, 0x7ccfdd2d;
imm32 r7, 0x8eeeffff;
R1 = - R0 (V);
R2 = - R1 (V);
R3 = - R2 (V);
R4 = - R3 (V);
R5 = - R4 (V);
R6 = - R5 (V);
R7 = - R6 (V);
R0 = - R7 (V);
CHECKREG r0, 0xA567892B;
CHECKREG r1, 0x5A9976D5;
CHECKREG r2, 0xA567892B;
CHECKREG r3, 0x5A9976D5;
CHECKREG r4, 0xA567892B;
CHECKREG r5, 0x5A9976D5;
CHECKREG r6, 0xA567892B;
CHECKREG r7, 0x5A9976D5;

imm32 r0, 0xb5678941;
imm32 r1, 0x2789ab5d;
imm32 r2, 0x34445565;
imm32 r3, 0xe6667777;
imm32 r4, 0x5567898b;
imm32 r5, 0x6789ab9d;
imm32 r6, 0xc4445505;
imm32 r7, 0x8666b777;
R2 = - R0 (V);
R3 = - R1 (V);
R4 = - R2 (V);
R5 = - R3 (V);
R6 = - R4 (V);
R7 = - R5 (V);
R0 = - R6 (V);
R1 = - R7 (V);
CHECKREG r0, 0xB5678941;
CHECKREG r1, 0x2789AB5D;
CHECKREG r2, 0x4A9976BF;
CHECKREG r3, 0xD87754A3;
CHECKREG r4, 0xB5678941;
CHECKREG r5, 0x2789AB5D;
CHECKREG r6, 0x4A9976BF;
CHECKREG r7, 0xD87754A3;



pass
