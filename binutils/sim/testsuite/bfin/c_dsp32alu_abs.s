//Original:/testcases/core/c_dsp32alu_abs/c_dsp32alu_abs.dsp
// Spec Reference: dsp32alu dregs = abs ( dregs, dregs)
# mach: bfin

.include "testutils.inc"
	start




imm32 r0, 0x15678911;
imm32 r1, 0x2789ab1d;
imm32 r2, 0x34445515;
imm32 r3, 0x46667717;
imm32 r4, 0x5567891b;
imm32 r5, 0x6789ab1d;
imm32 r6, 0x74445515;
imm32 r7, 0x86667777;
R0 = ABS R0;
R1 = ABS R1;
R2 = ABS R2;
R3 = ABS R3;
R4 = ABS R4;
R5 = ABS R5;
R6 = ABS R6;
R7 = ABS R7;
CHECKREG r0, 0x15678911;
CHECKREG r1, 0x2789AB1D;
CHECKREG r2, 0x34445515;
CHECKREG r3, 0x46667717;
CHECKREG r4, 0x5567891B;
CHECKREG r5, 0x6789AB1D;
CHECKREG r6, 0x74445515;
CHECKREG r7, 0x79998889;

imm32 r0, 0x9567892b;
imm32 r1, 0xa789ab2d;
imm32 r2, 0xb4445525;
imm32 r3, 0xc6667727;
imm32 r4, 0xd8889929;
imm32 r5, 0xeaaabb2b;
imm32 r6, 0xfcccdd2d;
imm32 r7, 0x0eeeffff;
R0 = ABS R7;
R1 = ABS R6;
R2 = ABS R5;
R3 = ABS R4;
R4 = ABS R3;
R5 = ABS R2;
R6 = ABS R1;
R7 = ABS R0;
CHECKREG r0, 0x0EEEFFFF;
CHECKREG r1, 0x033322D3;
CHECKREG r2, 0x155544D5;
CHECKREG r3, 0x277766D7;
CHECKREG r4, 0x277766D7;
CHECKREG r5, 0x155544D5;
CHECKREG r6, 0x033322D3;
CHECKREG r7, 0x0EEEFFFF;


pass
