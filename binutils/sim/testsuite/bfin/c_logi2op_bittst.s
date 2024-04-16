//Original:/testcases/core/c_logi2op_bittst/c_logi2op_bittst.dsp
// Spec Reference: Logi2op functions: bittst
# mach: bfin

.include "testutils.inc"
	start



imm32 r0, 0x00000000;
imm32 r1, 0x00000000;
imm32 r2, 0x00000000;
imm32 r3, 0x00000000;
imm32 r4, 0x00000000;
imm32 r5, 0x00000000;
imm32 r6, 0x00000000;
imm32 r7, 0x00000000;

// bit(0-7) tst set clr toggle
CC = BITTST ( R0 , 0 ); /* cc = 0 */
BITSET( R0 , 0 ); /* r0 = 0x00000001 */
R1 = CC;
CC = BITTST ( R0 , 0 ); /* cc = 1 */
R2 = CC;
BITCLR( R0 , 0 ); /* r0 = 0x00000000 */
CC = BITTST ( R0 , 0 ); /* cc = 1 */
R3 = CC;
BITTGL( R0 , 0 ); /* r0 = 0x00000001 */
CC = BITTST ( R0 , 0 ); /* cc = 1 */
R4 = CC;
CHECKREG r0, 0x00000001;
CHECKREG r1, 0x00000000;
CHECKREG r2, 0x00000001;
CHECKREG r3, 0x00000000;
CHECKREG r4, 0x00000001;

CC = BITTST ( R1 , 1 ); /* cc = 0 */
R2 = CC;
BITSET( R1 , 1 ); /* r1 = 0x00000002 */
CC = BITTST ( R1 , 1 ); /* cc = 1 */
R3 = CC;
BITCLR( R1 , 1 ); /* r1 = 0x00000000 */
CC = BITTST ( R1 , 1 ); /* cc = 1 */
R4 = CC;
BITTGL( R1 , 1 ); /* r1 = 0x00000002 */
CC = BITTST ( R1 , 1 ); /* cc = 1 */
R5 = CC;
CHECKREG r1, 0x00000002;
CHECKREG r2, 0x00000000;
CHECKREG r3, 0x00000001;
CHECKREG r4, 0x00000000;
CHECKREG r5, 0x00000001;

CC = BITTST ( R2 , 2 ); /* cc = 0 */
R3 = CC;
BITSET( R2 , 2 ); /* r2 = 0x00000004 */
CC = BITTST ( R2 , 2 ); /* cc = 1 */
R4 = CC;
BITCLR( R2 , 2 ); /* r2 = 0x00000000 */
CC = BITTST ( R2 , 2 ); /* cc = 1 */
R5 = CC;
BITTGL( R2 , 2 ); /* r2 = 0x00000004 */
CC = BITTST ( R2 , 2 ); /* cc = 1 */
R6 = CC;
CHECKREG r2, 0x00000004;
CHECKREG r3, 0x00000000;
CHECKREG r4, 0x00000001;
CHECKREG r5, 0x00000000;
CHECKREG r6, 0x00000001;

CC = BITTST ( R3 , 3 ); /* cc = 0 */
R4 = CC;
BITSET( R3 , 3 ); /* r3 = 0x00000008 */
CC = BITTST ( R3 , 3 ); /* cc = 1 */
R5 = CC;
BITCLR( R3 , 3 ); /* r3 = 0x00000000 */
CC = BITTST ( R3 , 3 ); /* cc = 1 */
R6 = CC;
BITTGL( R3 , 3 ); /* r3 = 0x00000008 */
CC = BITTST ( R3 , 3 ); /* cc = 1 */
R7 = CC;
CHECKREG r0, 0x00000001;
CHECKREG r1, 0x00000002;
CHECKREG r2, 0x00000004;
CHECKREG r3, 0x00000008;
CHECKREG r4, 0x00000000;
CHECKREG r5, 0x00000001;
CHECKREG r6, 0x00000000;
CHECKREG r7, 0x00000001;

CC = BITTST ( R4 , 4 ); /* cc = 0 */
R5 = CC;
BITSET( R4 , 4 ); /* r4 = 0x00000010 */
CC = BITTST ( R4 , 4 ); /* cc = 1 */
R6 = CC;
BITCLR( R4 , 4 ); /* r4 = 0x00000000 */
CC = BITTST ( R4 , 4 ); /* cc = 1 */
R7 = CC;
BITTGL( R4 , 4 ); /* r4 = 0x00000010 */
CC = BITTST ( R4 , 4 ); /* cc = 1 */
R0 = CC;
CHECKREG r4, 0x00000010;
CHECKREG r5, 0x00000000;
CHECKREG r6, 0x00000001;
CHECKREG r7, 0x00000000;
CHECKREG r0, 0x00000001;

CC = BITTST ( R5 , 5 ); /* cc = 0 */
R6 = CC;
BITSET( R5 , 5 ); /* r5 = 0x00000020 */
CC = BITTST ( R5 , 5 ); /* cc = 1 */
R7 = CC;
BITCLR( R5 , 5 ); /* r5 = 0x00000000 */
CC = BITTST ( R5 , 5 ); /* cc = 1 */
R0 = CC;
BITTGL( R5 , 5 ); /* r5 = 0x00000020 */
CC = BITTST ( R5 , 5 ); /* cc = 1 */
R1 = CC;
CHECKREG r5, 0x00000020;
CHECKREG r6, 0x00000000;
CHECKREG r7, 0x00000001;
CHECKREG r0, 0x00000000;
CHECKREG r1, 0x00000001;

CC = BITTST ( R6 , 6 ); /* cc = 0 */
R7 = CC;
BITSET( R6 , 6 ); /* r6 = 0x00000040 */
CC = BITTST ( R6 , 6 ); /* cc = 1 */
R0 = CC;
BITCLR( R6 , 6 ); /* r6 = 0x00000000 */
CC = BITTST ( R6 , 6 ); /* cc = 1 */
R1 = CC;
BITTGL( R6 , 6 ); /* r6 = 0x00000040 */
CC = BITTST ( R6 , 6 ); /* cc = 1 */
R2 = CC;
CHECKREG r6, 0x00000040;
CHECKREG r7, 0x00000000;
CHECKREG r0, 0x00000001;
CHECKREG r1, 0x00000000;
CHECKREG r2, 0x00000001;

CC = BITTST ( R7 , 7 ); /* cc = 0 */
R0 = CC;
BITSET( R7 , 7 ); /* r7 = 0x00000080 */
CC = BITTST ( R7 , 7 ); /* cc = 1 */
R1 = CC;
BITCLR( R7 , 7 ); /* r7 = 0x00000000 */
CC = BITTST ( R7 , 7 ); /* cc = 1 */
R2 = CC;
BITTGL( R7 , 7 ); /* r7 = 0x00000080 */
CC = BITTST ( R7 , 7 ); /* cc = 1 */
R3 = CC;
CHECKREG r0, 0x00000000;
CHECKREG r1, 0x00000001;
CHECKREG r2, 0x00000000;
CHECKREG r3, 0x00000001;

CHECKREG r4, 0x00000010;
CHECKREG r5, 0x00000020;
CHECKREG r6, 0x00000040;
CHECKREG r7, 0x00000080;

// bit(8-15) tst set clr toggle
CC = BITTST ( R0 , 8 ); /* cc = 0 */
R1 = CC;
BITSET( R0 , 8 ); /* r0 = 0x00000101 */
CC = BITTST ( R0 , 8 ); /* cc = 1 */
R2 = CC;
BITCLR( R0 , 8 ); /* r0 = 0x00000000 */
CC = BITTST ( R0 , 8 ); /* cc = 1 */
R3 = CC;
BITTGL( R0 , 8 ); /* r0 = 0x00000101 */
CC = BITTST ( R0 , 8 ); /* cc = 1 */
R4 = CC;
CHECKREG r0, 0x00000100;
CHECKREG r1, 0x00000000;
CHECKREG r2, 0x00000001;
CHECKREG r3, 0x00000000;
CHECKREG r4, 0x00000001;

CC = BITTST ( R1 , 9 ); /* cc = 0 */
R2 = CC;
BITSET( R1 , 9 ); /* r1 = 0x00000200 */
CC = BITTST ( R1 , 9 ); /* cc = 1 */
R3 = CC;
BITCLR( R1 , 9 ); /* r1 = 0x00000000 */
CC = BITTST ( R1 , 9 ); /* cc = 1 */
R4 = CC;
BITTGL( R1 , 9 ); /* r1 = 0x00000200 */
CC = BITTST ( R1 , 9 ); /* cc = 1 */
R5 = CC;
CHECKREG r1, 0x00000200;
CHECKREG r2, 0x00000000;
CHECKREG r3, 0x00000001;
CHECKREG r4, 0x00000000;
CHECKREG r5, 0x00000001;

CC = BITTST ( R2 , 10 ); /* cc = 0 */
R3 = CC;
BITSET( R2 , 10 ); /* r2 = 0x00000400 */
CC = BITTST ( R2 , 10 ); /* cc = 1 */
R4 = CC;
BITCLR( R2 , 10 ); /* r2 = 0x00000000 */
CC = BITTST ( R2 , 10 ); /* cc = 1 */
R5 = CC;
BITTGL( R2 , 10 ); /* r2 = 0x00000400 */
CC = BITTST ( R2 , 10 ); /* cc = 1 */
R6 = CC;
CHECKREG r2, 0x00000400;
CHECKREG r3, 0x00000000;
CHECKREG r4, 0x00000001;
CHECKREG r5, 0x00000000;
CHECKREG r6, 0x00000001;

CC = BITTST ( R3 , 11 ); /* cc = 0 */
R4 = CC;
BITSET( R3 , 11 ); /* r3 = 0x00000800 */
CC = BITTST ( R3 , 11 ); /* cc = 1 */
R5 = CC;
BITCLR( R3 , 11 ); /* r3 = 0x00000000 */
CC = BITTST ( R3 , 11 ); /* cc = 1 */
R6 = CC;
BITTGL( R3 , 11 ); /* r3 = 0x00000800 */
CC = BITTST ( R3 , 11 ); /* cc = 1 */
R7 = CC;
CHECKREG r3, 0x00000800;
CHECKREG r4, 0x00000000;
CHECKREG r5, 0x00000001;
CHECKREG r6, 0x00000000;
CHECKREG r7, 0x00000001;

CC = BITTST ( R4 , 12 ); /* cc = 0 */
R5 = CC;
BITSET( R4 , 12 ); /* r4 = 0x00001000 */
CC = BITTST ( R4 , 12 ); /* cc = 1 */
R6 = CC;
BITCLR( R4 , 12 ); /* r4 = 0x00000000 */
CC = BITTST ( R4 , 12 ); /* cc = 1 */
R7 = CC;
BITTGL( R4 , 12 ); /* r4 = 0x00001000 */
CC = BITTST ( R4 , 12 ); /* cc = 1 */
R0 = CC;
CHECKREG r4, 0x00001000;
CHECKREG r5, 0x00000000;
CHECKREG r6, 0x00000001;
CHECKREG r7, 0x00000000;
CHECKREG r0, 0x00000001;

CC = BITTST ( R5 , 13 ); /* cc = 0 */
R6 = CC;
BITSET( R5 , 13 ); /* r5 = 0x00002000 */
CC = BITTST ( R5 , 13 ); /* cc = 1 */
R7 = CC;
BITCLR( R5 , 13 ); /* r5 = 0x00000000 */
CC = BITTST ( R5 , 13 ); /* cc = 1 */
R0 = CC;
BITTGL( R5 , 13 ); /* r5 = 0x00002000 */
CC = BITTST ( R5 , 13 ); /* cc = 1 */
R1 = CC;
CHECKREG r5, 0x00002000;
CHECKREG r6, 0x00000000;
CHECKREG r7, 0x00000001;
CHECKREG r0, 0x00000000;
CHECKREG r1, 0x00000001;

CC = BITTST ( R6 , 14 ); /* cc = 0 */
R7 = CC;
BITSET( R6 , 14 ); /* r6 = 0x00004000 */
CC = BITTST ( R6 , 14 ); /* cc = 1 */
R0 = CC;
BITCLR( R6 , 14 ); /* r6 = 0x00000000 */
CC = BITTST ( R6 , 14 ); /* cc = 1 */
R1 = CC;
BITTGL( R6 , 14 ); /* r6 = 0x00004000 */
CC = BITTST ( R6 , 14 ); /* cc = 1 */
R2 = CC;
CHECKREG r6, 0x00004000;
CHECKREG r7, 0x00000000;
CHECKREG r0, 0x00000001;
CHECKREG r1, 0x00000000;
CHECKREG r2, 0x00000001;

CC = BITTST ( R7 , 15 ); /* cc = 0 */
R0 = CC;
BITSET( R7 , 15 ); /* r7 = 0x00008000 */
CC = BITTST ( R7 , 15 ); /* cc = 1 */
R1 = CC;
BITCLR( R7 , 15 ); /* r7 = 0x00000000 */
CC = BITTST ( R7 , 15 ); /* cc = 1 */
R2 = CC;
BITTGL( R7 , 15 ); /* r7 = 0x00008000 */
CC = BITTST ( R7 , 15 ); /* cc = 1 */
R3 = CC;
CHECKREG r0, 0x00000000;
CHECKREG r1, 0x00000001;
CHECKREG r2, 0x00000000;
CHECKREG r3, 0x00000001;
CHECKREG r4, 0x00001000;
CHECKREG r5, 0x00002000;
CHECKREG r6, 0x00004000;
CHECKREG r7, 0x00008000;

// bit(16-23) tst set clr toggle
CC = BITTST ( R0 , 16 ); /* cc = 0 */
R1 = CC;
BITSET( R0 , 16 ); /* r0 = 0x00010000 */
CC = BITTST ( R0 , 16 ); /* cc = 1 */
R2 = CC;
BITCLR( R0 , 16 ); /* r0 = 0x00000000 */
CC = BITTST ( R0 , 16 ); /* cc = 1 */
R3 = CC;
BITTGL( R0 , 16 ); /* r0 = 0x00010000 */
CC = BITTST ( R0 , 16 ); /* cc = 1 */
R4 = CC;
CHECKREG r0, 0x00010000;
CHECKREG r1, 0x00000000;
CHECKREG r2, 0x00000001;
CHECKREG r3, 0x00000000;
CHECKREG r4, 0x00000001;

CC = BITTST ( R1 , 17 ); /* cc = 0 */
R2 = CC;
BITSET( R1 , 17 ); /* r1 = 0x00020000 */
CC = BITTST ( R1 , 17 ); /* cc = 1 */
R3 = CC;
BITCLR( R1 , 17 ); /* r1 = 0x00000000 */
CC = BITTST ( R1 , 17 ); /* cc = 1 */
R4 = CC;
BITTGL( R1 , 17 ); /* r1 = 0x00020000 */
CC = BITTST ( R1 , 17 ); /* cc = 1 */
R5 = CC;
CHECKREG r1, 0x00020000;
CHECKREG r2, 0x00000000;
CHECKREG r3, 0x00000001;
CHECKREG r4, 0x00000000;
CHECKREG r5, 0x00000001;

CC = BITTST ( R2 , 18 ); /* cc = 0 */
R3 = CC;
BITSET( R2 , 18 ); /* r2 = 0x00020000 */
CC = BITTST ( R2 , 18 ); /* cc = 1 */
R4 = CC;
BITCLR( R2 , 18 ); /* r2 = 0x00000000 */
CC = BITTST ( R2 , 18 ); /* cc = 1 */
R4 = CC;
BITTGL( R2 , 18 ); /* r2 = 0x00020000 */
CC = BITTST ( R2 , 18 ); /* cc = 1 */
R5 = CC;
CHECKREG r2, 0x00040000;
CHECKREG r3, 0x00000000;
CHECKREG r4, 0x00000000;
CHECKREG r5, 0x00000001;
CHECKREG r6, 0x00004000;

CC = BITTST ( R3 , 19 ); /* cc = 0 */
R4 = CC;
BITSET( R3 , 19 ); /* r3 = 0x00080000 */
CC = BITTST ( R3 , 19 ); /* cc = 1 */
R5 = CC;
BITCLR( R3 , 19 ); /* r3 = 0x00000000 */
CC = BITTST ( R3 , 19 ); /* cc = 1 */
R6 = CC;
BITTGL( R3 , 19 ); /* r3 = 0x00080000 */
CC = BITTST ( R3 , 19 ); /* cc = 1 */
R7 = CC;
CHECKREG r3, 0x00080000;
CHECKREG r4, 0x00000000;
CHECKREG r5, 0x00000001;
CHECKREG r6, 0x00000000;
CHECKREG r7, 0x00000001;

CC = BITTST ( R4 , 20 ); /* cc = 0 */
R5 = CC;
BITSET( R4 , 20 ); /* r4 = 0x00100000 */
CC = BITTST ( R4 , 20 ); /* cc = 1 */
R6 = CC;
BITCLR( R4 , 20 ); /* r4 = 0x00000000 */
CC = BITTST ( R4 , 20 ); /* cc = 1 */
R7 = CC;
BITTGL( R4 , 20 ); /* r4 = 0x00100000 */
CC = BITTST ( R4 , 20 ); /* cc = 1 */
R0 = CC;
CHECKREG r4, 0x00100000;
CHECKREG r5, 0x00000000;
CHECKREG r6, 0x00000001;
CHECKREG r7, 0x00000000;
CHECKREG r0, 0x00000001;

CC = BITTST ( R5 , 21 ); /* cc = 0 */
R6 = CC;
BITSET( R5 , 21 ); /* r5 = 0x00200000 */
CC = BITTST ( R5 , 21 ); /* cc = 1 */
R7 = CC;
BITCLR( R5 , 21 ); /* r5 = 0x00000000 */
CC = BITTST ( R5 , 21 ); /* cc = 1 */
R0 = CC;
BITTGL( R5 , 21 ); /* r5 = 0x00200000 */
CC = BITTST ( R5 , 21 ); /* cc = 1 */
R1 = CC;
CHECKREG r5, 0x00200000;
CHECKREG r6, 0x00000000;
CHECKREG r7, 0x00000001;
CHECKREG r0, 0x00000000;
CHECKREG r1, 0x00000001;

CC = BITTST ( R6 , 22 ); /* cc = 0 */
R7 = CC;
BITSET( R6 , 22 ); /* r6 = 0x00400000 */
CC = BITTST ( R6 , 22 ); /* cc = 1 */
R0 = CC;
BITCLR( R6 , 22 ); /* r6 = 0x00000000 */
CC = BITTST ( R6 , 22 ); /* cc = 1 */
R1 = CC;
BITTGL( R6 , 22 ); /* r6 = 0x00400000 */
CC = BITTST ( R6 , 22 ); /* cc = 1 */
R2 = CC;
CHECKREG r6, 0x00400000;
CHECKREG r7, 0x00000000;
CHECKREG r0, 0x00000001;
CHECKREG r1, 0x00000000;
CHECKREG r2, 0x00000001;

CC = BITTST ( R7 , 23 ); /* cc = 0 */
R0 = CC;
BITSET( R7 , 23 ); /* r7 = 0x00800000 */
CC = BITTST ( R7 , 23 ); /* cc = 1 */
R1 = CC;
BITCLR( R7 , 23 ); /* r7 = 0x00000000 */
CC = BITTST ( R7 , 23 ); /* cc = 1 */
R2 = CC;
BITTGL( R7 , 23 ); /* r7 = 0x00800000 */
CC = BITTST ( R7 , 23 ); /* cc = 1 */
R3 = CC;
CHECKREG r0, 0x00000000;
CHECKREG r1, 0x00000001;
CHECKREG r2, 0x00000000;
CHECKREG r3, 0x00000001;
CHECKREG r4, 0x00100000;
CHECKREG r5, 0x00200000;
CHECKREG r6, 0x00400000;
CHECKREG r7, 0x00800000;

// bit(24-31) tst set clr toggle
CC = BITTST ( R0 , 24 ); /* cc = 0 */
R1 = CC;
BITSET( R0 , 24 ); /* r0 = 0x00000101 */
CC = BITTST ( R0 , 24 ); /* cc = 1 */
R2 = CC;
BITCLR( R0 , 24 ); /* r0 = 0x01000000 */
CC = BITTST ( R0 , 24 ); /* cc = 1 */
R3 = CC;
BITTGL( R0 , 24 ); /* r0 = 0x01000000 */
CC = BITTST ( R0 , 24 ); /* cc = 1 */
R4 = CC;
CHECKREG r0, 0x01000000;
CHECKREG r1, 0x00000000;
CHECKREG r2, 0x00000001;
CHECKREG r3, 0x00000000;
CHECKREG r4, 0x00000001;

CC = BITTST ( R1 , 25 ); /* cc = 0 */
R2 = CC;
BITSET( R1 , 25 ); /* r1 = 0x02000000 */
CC = BITTST ( R1 , 25 ); /* cc = 1 */
R3 = CC;
BITCLR( R1 , 25 ); /* r1 = 0x00000000 */
CC = BITTST ( R1 , 25 ); /* cc = 1 */
R4 = CC;
BITTGL( R1 , 25 ); /* r1 = 0x02000000 */
CC = BITTST ( R1 , 25 ); /* cc = 1 */
R5 = CC;
CHECKREG r1, 0x02000000;
CHECKREG r2, 0x00000000;
CHECKREG r3, 0x00000001;
CHECKREG r4, 0x00000000;
CHECKREG r5, 0x00000001;

CC = BITTST ( R2 , 26 ); /* cc = 0 */
R3 = CC;
BITSET( R2 , 26 ); /* r2 = 0x04000000 */
CC = BITTST ( R2 , 26 ); /* cc = 1 */
R4 = CC;
BITCLR( R2 , 26 ); /* r2 = 0x00000000 */
CC = BITTST ( R2 , 26 ); /* cc = 1 */
R5 = CC;
BITTGL( R2 , 26 ); /* r2 = 0x04000000 */
CC = BITTST ( R2 , 26 ); /* cc = 1 */
R6 = CC;
CHECKREG r2, 0x04000000;
CHECKREG r3, 0x00000000;
CHECKREG r4, 0x00000001;
CHECKREG r5, 0x00000000;
CHECKREG r6, 0x00000001;

CC = BITTST ( R3 , 27 ); /* cc = 0 */
R4 = CC;
BITSET( R3 , 27 ); /* r3 = 0x08000000 */
CC = BITTST ( R3 , 27 ); /* cc = 1 */
R5 = CC;
BITCLR( R3 , 27 ); /* r3 = 0x00000000 */
CC = BITTST ( R3 , 27 ); /* cc = 1 */
R6 = CC;
BITTGL( R3 , 27 ); /* r3 = 0x08000000 */
CC = BITTST ( R3 , 27 ); /* cc = 1 */
R7 = CC;
CHECKREG r3, 0x08000000;
CHECKREG r4, 0x00000000;
CHECKREG r5, 0x00000001;
CHECKREG r6, 0x00000000;
CHECKREG r7, 0x00000001;

CC = BITTST ( R4 , 28 ); /* cc = 0 */
R5 = CC;
BITSET( R4 , 28 ); /* r4 = 0x10000000 */
CC = BITTST ( R4 , 28 ); /* cc = 1 */
R6 = CC;
BITCLR( R4 , 28 ); /* r4 = 0x00000000 */
CC = BITTST ( R4 , 28 ); /* cc = 1 */
R7 = CC;
BITTGL( R4 , 28 ); /* r4 = 0x10000000 */
CC = BITTST ( R4 , 28 ); /* cc = 1 */
R0 = CC;
CHECKREG r4, 0x10000000;
CHECKREG r5, 0x00000000;
CHECKREG r6, 0x00000001;
CHECKREG r7, 0x00000000;
CHECKREG r0, 0x00000001;

CC = BITTST ( R5 , 29 ); /* cc = 0 */
R6 = CC;
BITSET( R5 , 29 ); /* r5 = 0x20000000 */
CC = BITTST ( R5 , 29 ); /* cc = 1 */
R7 = CC;
BITCLR( R5 , 29 ); /* r5 = 0x00000000 */
CC = BITTST ( R5 , 29 ); /* cc = 1 */
R0 = CC;
BITTGL( R5 , 29 ); /* r5 = 0x20000000 */
CC = BITTST ( R5 , 29 ); /* cc = 1 */
R1 = CC;
CHECKREG r5, 0x20000000;
CHECKREG r6, 0x00000000;
CHECKREG r7, 0x00000001;
CHECKREG r0, 0x00000000;
CHECKREG r1, 0x00000001;

CC = BITTST ( R6 , 30 ); /* cc = 0 */
R7 = CC;
BITSET( R6 , 30 ); /* r6 = 0x40000000 */
CC = BITTST ( R6 , 30 ); /* cc = 1 */
R0 = CC;
BITCLR( R6 , 30 ); /* r6 = 0x00000000 */
CC = BITTST ( R6 , 30 ); /* cc = 1 */
R1 = CC;
BITTGL( R6 , 30 ); /* r6 = 0x40000000 */
CC = BITTST ( R6 , 30 ); /* cc = 1 */
R2 = CC;
CHECKREG r6, 0x40000000;
CHECKREG r7, 0x00000000;
CHECKREG r0, 0x00000001;
CHECKREG r1, 0x00000000;
CHECKREG r2, 0x00000001;

CC = BITTST ( R7 , 31 ); /* cc = 0 */
R0 = CC;
BITSET( R7 , 31 ); /* r7 = 0x80000000 */
CC = BITTST ( R7 , 31 ); /* cc = 1 */
R1 = CC;
BITCLR( R7 , 31 ); /* r7 = 0x00000000 */
CC = BITTST ( R7 , 31 ); /* cc = 1 */
R2 = CC;
BITTGL( R7 , 31 ); /* r7 = 0x80000000 */
CC = BITTST ( R7 , 31 ); /* cc = 1 */
R3 = CC;
CHECKREG r0, 0x00000000;
CHECKREG r1, 0x00000001;
CHECKREG r2, 0x00000000;
CHECKREG r3, 0x00000001;
CHECKREG r4, 0x10000000;
CHECKREG r5, 0x20000000;
CHECKREG r6, 0x40000000;
CHECKREG r7, 0x80000000;

pass
