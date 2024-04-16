//Original:/testcases/core/c_logi2op_bitclr/c_logi2op_bitclr.dsp
// Spec Reference: Logi2op functions: bitclr
# mach: bfin

.include "testutils.inc"
	start



imm32 r0, 0xffffffff;
imm32 r1, 0xffffffff;
imm32 r2, 0xffffffff;
imm32 r3, 0xffffffff;
imm32 r4, 0xffffffff;
imm32 r5, 0xffffffff;
imm32 r6, 0xffffffff;
imm32 r7, 0xffffffff;

// bit clr
BITCLR( R0 , 0 ); /* r0 = 0x00000001 */
BITCLR( R1 , 1 ); /* r1 = 0x00000002 */
BITCLR( R2 , 2 ); /* r2 = 0x00000004 */
BITCLR( R3 , 3 ); /* r3 = 0x00000008 */
BITCLR( R4 , 4 ); /* r4 = 0x00000010 */
BITCLR( R5 , 5 ); /* r5 = 0x00000020 */
BITCLR( R6 , 6 ); /* r6 = 0x00000040 */
BITCLR( R7 , 7 ); /* r7 = 0x00000080 */
CHECKREG r0, 0xfffffffe;
CHECKREG r1, 0xfffffffd;
CHECKREG r2, 0xfffffffb;
CHECKREG r3, 0xfffffff7;
CHECKREG r4, 0xffffffef;
CHECKREG r5, 0xffffffdf;
CHECKREG r6, 0xffffffbf;
CHECKREG r7, 0xffffff7f;

// bit clr
BITCLR( R0 , 8 ); /* r0 = 0x00000100 */
BITCLR( R1 , 9 ); /* r1 = 0x00000200 */
BITCLR( R2 , 10 ); /* r2 = 0x00000400 */
BITCLR( R3 , 11 ); /* r3 = 0x00000800 */
BITCLR( R4 , 12 ); /* r4 = 0x00001000 */
BITCLR( R5 , 13 ); /* r5 = 0x00002000 */
BITCLR( R6 , 14 ); /* r6 = 0x00004000 */
BITCLR( R7 , 15 ); /* r7 = 0x00008000 */
CHECKREG r0, 0xfffffefe;
CHECKREG r1, 0xfffffdfd;
CHECKREG r2, 0xfffffbfb;
CHECKREG r3, 0xfffff7f7;
CHECKREG r4, 0xffffefef;
CHECKREG r5, 0xffffdfdf;
CHECKREG r6, 0xffffbfbf;
CHECKREG r7, 0xffff7f7f;

// bit clr
BITCLR( R0 , 16 ); /* r0 = 0x00000100 */
BITCLR( R1 , 17 ); /* r1 = 0x00000200 */
BITCLR( R2 , 18 ); /* r2 = 0x00000400 */
BITCLR( R3 , 19 ); /* r3 = 0x00000800 */
BITCLR( R4 , 20 ); /* r4 = 0x00001000 */
BITCLR( R5 , 21 ); /* r5 = 0x00002000 */
BITCLR( R6 , 22 ); /* r6 = 0x00004000 */
BITCLR( R7 , 23 ); /* r7 = 0x00008000 */
CHECKREG r0, 0xfffefefe;
CHECKREG r1, 0xfffdfdfd;
CHECKREG r2, 0xfffbfbfb;
CHECKREG r3, 0xfff7f7f7;
CHECKREG r4, 0xffefefef;
CHECKREG r5, 0xffdfdfdf;
CHECKREG r6, 0xffbfbfbf;
CHECKREG r7, 0xff7f7f7f;

// bit clr
BITCLR( R0 , 24 ); /* r0 = 0x00000100 */
BITCLR( R1 , 25 ); /* r1 = 0x00000200 */
BITCLR( R2 , 26 ); /* r2 = 0x00000400 */
BITCLR( R3 , 27 ); /* r3 = 0x00000800 */
BITCLR( R4 , 28 ); /* r4 = 0x00001000 */
BITCLR( R5 , 29 ); /* r5 = 0x00002000 */
BITCLR( R6 , 30 ); /* r6 = 0x00004000 */
BITCLR( R7 , 31 ); /* r7 = 0x00008000 */
CHECKREG r0, 0xfefefefe;
CHECKREG r1, 0xfdfdfdfd;
CHECKREG r2, 0xfbfbfbfb;
CHECKREG r3, 0xf7f7f7f7;
CHECKREG r4, 0xefefefef;
CHECKREG r5, 0xdfdfdfdf;
CHECKREG r6, 0xbfbfbfbf;
CHECKREG r7, 0x7f7f7f7f;


pass
