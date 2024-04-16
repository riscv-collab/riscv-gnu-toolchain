//Original:/testcases/core/c_dsp32alu_rlh_rnd/c_dsp32alu_rlh_rnd.dsp
// Spec Reference: dsp32alu dreg_lo(hi) = rnd dregs
# mach: bfin

.include "testutils.inc"
	start




imm32 r0, 0x4537891b;
imm32 r1, 0x6759ab2d;
imm32 r2, 0x44555535;
imm32 r3, 0x66665747;
imm32 r4, 0x88789565;
imm32 r5, 0xaa8abb5b;
imm32 r6, 0xcc9cdd85;
imm32 r7, 0xeeaeff9f;
R0.L = R1 (RND);
R0.H = R2 (RND);
R1.L = R3 (RND);
R1.H = R4 (RND);
R2.L = R5 (RND);
R2.H = R6 (RND);
CHECKREG r0, 0x4455675A;
CHECKREG r1, 0x88796666;
CHECKREG r2, 0xCC9DAA8B;


imm32 r0, 0xe537891b;
imm32 r1, 0xf759ab2d;
imm32 r2, 0x4ef55535;
imm32 r3, 0x666b5747;
imm32 r4, 0xc8789565;
imm32 r5, 0xaa8abb5b;
imm32 r6, 0x8c9cdd85;
imm32 r7, 0x9eaeff9f;
R3.L = R0 (RND);
R3.H = R1 (RND);
R4.L = R2 (RND);
R4.H = R5 (RND);
R5.L = R6 (RND);
R5.H = R7 (RND);
CHECKREG r3, 0xF75AE538;
CHECKREG r4, 0xAA8B4EF5;
CHECKREG r5, 0x9EAF8C9D;

imm32 r0, 0x5537891b;
imm32 r1, 0x6759ab2d;
imm32 r2, 0x8ef55535;
imm32 r3, 0x666b5747;
imm32 r4, 0xc8789565;
imm32 r5, 0xea8abb5b;
imm32 r6, 0xfc9cdd85;
imm32 r7, 0x9eaeff9f;
R6.L = R0 (RND);
R6.H = R1 (RND);
R7.L = R2 (RND);
R7.H = R3 (RND);
R5.L = R4 (RND);
R5.H = R5 (RND);
CHECKREG r5, 0xEA8BC879;
CHECKREG r6, 0x675A5538;
CHECKREG r7, 0x666B8EF5;

pass
