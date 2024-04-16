//Original:/testcases/core/c_dsp32alu_bytepack/c_dsp32alu_bytepack.dsp
// Spec Reference: dsp32alu bytepack
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
R4 = BYTEPACK ( R0 , R0 );
R5 = BYTEPACK ( R0 , R1 );
R6 = BYTEPACK ( R0 , R2 );
R7 = BYTEPACK ( R0 , R3 );
CHECKREG r4, 0x67116711;
CHECKREG r5, 0x891D6711;
CHECKREG r6, 0x44156711;
CHECKREG r7, 0x66176711;

imm32 r0, 0x1567892b;
imm32 r1, 0x2789ab2d;
imm32 r2, 0x34445525;
imm32 r3, 0x46667727;
imm32 r4, 0x58889929;
imm32 r5, 0x6aaabb2b;
imm32 r6, 0x7cccdd2d;
imm32 r7, 0x8eeeffff;
R4 = BYTEPACK ( R1 , R4 );
R5 = BYTEPACK ( R1 , R5 );
R6 = BYTEPACK ( R1 , R6 );
R7 = BYTEPACK ( R1 , R7 );
CHECKREG r4, 0x8829892D;
CHECKREG r5, 0xAA2B892D;
CHECKREG r6, 0xCC2D892D;
CHECKREG r7, 0xEEFF892D;

imm32 r0, 0x416789ab;
imm32 r1, 0x6289abcd;
imm32 r2, 0x43445555;
imm32 r3, 0x64667777;
imm32 r0, 0x456789ab;
imm32 r1, 0x6689abcd;
imm32 r2, 0x47445555;
imm32 r3, 0x68667777;
R4 = BYTEPACK ( R2 , R0 );
R5 = BYTEPACK ( R2 , R1 );
R6 = BYTEPACK ( R2 , R2 );
R7 = BYTEPACK ( R2 , R3 );
CHECKREG r4, 0x67AB4455;
CHECKREG r5, 0x89CD4455;
CHECKREG r6, 0x44554455;
CHECKREG r7, 0x66774455;

imm32 r0, 0x496789ab;
imm32 r1, 0x6489abcd;
imm32 r2, 0x4b445555;
imm32 r3, 0x6c647777;
imm32 r4, 0x8d889999;
imm32 r5, 0xaeaa4bbb;
imm32 r6, 0xcfccd44d;
imm32 r7, 0xe1eefff4;
R4 = BYTEPACK ( R3 , R4 );
R5 = BYTEPACK ( R3 , R5 );
R6 = BYTEPACK ( R3 , R6 );
R7 = BYTEPACK ( R3 , R7 );
CHECKREG r4, 0x88996477;
CHECKREG r5, 0xAABB6477;
CHECKREG r6, 0xCC4D6477;
CHECKREG r7, 0xEEF46477;


pass
