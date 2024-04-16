//Original:/testcases/core/c_dsp32alu_absabs/c_dsp32alu_absabs.dsp
// Spec Reference: dsp32alu dregs = abs / abs ( dregs, dregs)
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
R0 = ABS R0 (V);
R1 = ABS R1 (V);
R2 = ABS R2 (V);
R3 = ABS R3 (V);
R4 = ABS R4 (V);
R5 = ABS R5 (V);
R6 = ABS R6 (V);
R7 = ABS R7 (V);
CHECKREG r0, 0x156776EF;
CHECKREG r1, 0x278954E3;
CHECKREG r2, 0x34445515;
CHECKREG r3, 0x46667717;
CHECKREG r4, 0x556776E5;
CHECKREG r5, 0x678954E3;
CHECKREG r6, 0x74445515;
CHECKREG r7, 0x799A7777;

imm32 r0, 0x9567892b;
imm32 r1, 0xa789ab2d;
imm32 r2, 0xb4445525;
imm32 r3, 0xc6667727;
imm32 r4, 0xd8889929;
imm32 r5, 0xeaaabb2b;
imm32 r6, 0xfcccdd2d;
imm32 r7, 0x0eeeffff;
R0 = ABS R7 (V);
R1 = ABS R6 (V);
R2 = ABS R5 (V);
R3 = ABS R4 (V);
R4 = ABS R3 (V);
R5 = ABS R2 (V);
R6 = ABS R1 (V);
R7 = ABS R0 (V);
CHECKREG r0, 0x0EEE0001;
CHECKREG r1, 0x033422D3;
CHECKREG r2, 0x155644D5;
CHECKREG r3, 0x277866D7;
CHECKREG r4, 0x277866D7;
CHECKREG r5, 0x155644D5;
CHECKREG r6, 0x033422D3;
CHECKREG r7, 0x0EEE0001;


pass
