//Original:/testcases/core/c_logi2op_alshft_mix/c_logi2op_alshft_mix.dsp
// Spec Reference: Logi2op >>>=, >>=, <<=
# mach: bfin

.include "testutils.inc"
	start

// Arithmetic >>>= : positive data
imm32 r0, 0x40000000;
imm32 r1, 0x01111111;
imm32 r2, 0x22222222;
imm32 r3, 0x33333333;
imm32 r4, 0x44444444;
imm32 r5, 0x55555555;
imm32 r6, 0x66666666;
imm32 r7, 0x77777777;
R0 >>>= 1; /* r0 = 0x20000000 */
R1 >>>= 1; /* r1 = 0x00888888 */
R2 >>>= 2; /* r2 = 0x08888888 */
R3 >>>= 8; /* r3 = 0x00333333 */
R4 >>>= 1; /* r4 = 0x22222222 */
R5 >>>= 27; /* r5 = 0x0000000a */
R6 >>>= 30; /* r5 = 0x00000001 */
R7 >>>= 31; /* r5 = 0x00000000 */
CHECKREG r0, 0x20000000;
CHECKREG r1, 0x00888888;
CHECKREG r2, 0x08888888;
CHECKREG r3, 0x00333333;
CHECKREG r4, 0x22222222;
CHECKREG r5, 0x0000000a;
CHECKREG r6, 0x00000001;
CHECKREG r7, 0x00000000;



// Arithmetic >>>= : negative data ,
imm32 r0, 0x80000000;
imm32 r1, 0x81111111;
imm32 r2, 0xa2222222;
imm32 r3, 0xb3333333;
imm32 r4, 0xc4444444;
imm32 r5, 0xd5555555;
imm32 r6, 0xe6666666;
imm32 r7, 0xf7777777;
R0 >>>= 1; /* r0 = 0xc0000000 */
R1 >>>= 1; /* r1 = 0xc0888888 */
R2 >>>= 2; /* r2 = 0xe8888888 */
R3 >>>= 8; /* r3 = 0x00333333 */
R4 >>>= 1; /* r4 = 0x22222222 */
R5 >>>= 27; /* r5 = 0x0000000a */
R6 >>>= 30; /* r5 = 0x00000001 */
R7 >>>= 31; /* r5 = 0x00000000 */
CHECKREG r0, 0xc0000000;
CHECKREG r1, 0xc0888888;
CHECKREG r2, 0xe8888888;
CHECKREG r3, 0xffb33333;
CHECKREG r4, 0xe2222222;
CHECKREG r5, 0xfffffffa;
CHECKREG r6, 0xffffffff;
CHECKREG r7, 0xffffffff;


// Logical >>>= : positive data
imm32 r0, 0x40000000;
imm32 r1, 0x01111111;
imm32 r2, 0x22222222;
imm32 r3, 0x33333333;
imm32 r4, 0x44444444;
imm32 r5, 0x55555555;
imm32 r6, 0x66666666;
imm32 r7, 0x77777777;
R0 >>= 1; /* r0 = 0x20000000 */
R1 >>= 1; /* r1 = 0x00888888 */
R2 >>= 2; /* r2 = 0x08888888 */
R3 >>= 8; /* r3 = 0x00333333 */
R4 >>= 1; /* r4 = 0x22222222 */
R5 >>= 27; /* r5 = 0x0000000a */
R6 >>= 30; /* r5 = 0x00000001 */
R7 >>= 31; /* r5 = 0x00000000 */
CHECKREG r0, 0x20000000;
CHECKREG r1, 0x00888888;
CHECKREG r2, 0x08888888;
CHECKREG r3, 0x00333333;
CHECKREG r4, 0x22222222;
CHECKREG r5, 0x0000000a;
CHECKREG r6, 0x00000001;
CHECKREG r7, 0x00000000;

// Logical >>= : negative data ,
imm32 r0, 0x80000000;
imm32 r1, 0x81111111;
imm32 r2, 0xa2222222;
imm32 r3, 0xb3333333;
imm32 r4, 0xc4444444;
imm32 r5, 0xd5555555;
imm32 r6, 0xe6666666;
imm32 r7, 0xf7777777;
R0 >>= 1; /* r0 = 0x40000000 */
R1 >>= 1; /* r1 = 0x40888888 */
R2 >>= 2; /* r2 = 0x48888888 */
R3 >>= 8; /* r3 = 0x40333333 */
R4 >>= 1; /* r4 = 0xa2222222 */
R5 >>= 27; /* r5 = 0x0000000a */
R6 >>= 30; /* r5 = 0x00000001 */
R7 >>= 31; /* r5 = 0x00000000 */
CHECKREG r0, 0x40000000;
CHECKREG r1, 0x40888888;
CHECKREG r2, 0x28888888;
CHECKREG r3, 0x00b33333;
CHECKREG r4, 0x62222222;
CHECKREG r5, 0x0000001a;
CHECKREG r6, 0x00000003;
CHECKREG r7, 0x00000001;


// Logical <<= : negative data ,
imm32 r0, 0x80000000;
imm32 r1, 0x81111111;
imm32 r2, 0xa2222222;
imm32 r3, 0xb3333333;
imm32 r4, 0xc4444444;
imm32 r5, 0xd5555555;
imm32 r6, 0xe6666666;
imm32 r7, 0xf7777777;
R0 <<= 1; /* r0 = 0x00000000 */
R1 <<= 1; /* r1 = 0x40888888 */
R2 <<= 2; /* r2 = 0x88888888 */
R3 <<= 8; /* r3 = 0x33333300 */
R4 <<= 1; /* r4 = 0x88888888 */
R5 <<= 27; /* r5 = 0xa8000000 */
R6 <<= 30; /* r5 = 0x80000000 */
R7 <<= 31; /* r5 = 0x80000000 */
CHECKREG r0, 0x00000000;
CHECKREG r1, 0x02222222;
CHECKREG r2, 0x88888888;
CHECKREG r3, 0x33333300;
CHECKREG r4, 0x88888888;
CHECKREG r5, 0xa8000000;
CHECKREG r6, 0x80000000;
CHECKREG r7, 0x80000000;
 // hlt;

pass
