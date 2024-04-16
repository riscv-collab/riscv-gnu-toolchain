//Original:/testcases/core/c_logi2op_nbittst/c_logi2op_nbittst.dsp
// Spec Reference: Logi2op !bittst
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
CC = ! BITTST( R0 , 0 ); /* cc = 0 */
BITSET( R0 , 0 ); /* r0 = 0x00000001 */
R1 = CC;
CC = ! BITTST( R0 , 0 ); /* cc = 1 */
R2 = CC;
BITCLR( R0 , 0 ); /* r0 = 0x00000000 */
CC = ! BITTST( R0 , 0 ); /* cc = 1 */
R3 = CC;
BITTGL( R0 , 0 ); /* r0 = 0x00000001 */
CC = ! BITTST( R0 , 0 ); /* cc = 1 */
R4 = CC;
CHECKREG r0, 0x00000001;
CHECKREG r1, 0x00000001;
CHECKREG r2, 0x00000000;
CHECKREG r3, 0x00000001;
CHECKREG r4, 0x00000000;

CC = ! BITTST( R1 , 1 ); /* cc = 0 */
R2 = CC;
BITSET( R1 , 1 ); /* r0 = 0x00000002 */
CC = ! BITTST( R1 , 1 ); /* cc = 1 */
R3 = CC;
BITCLR( R1 , 1 ); /* r0 = 0x00000000 */
CC = ! BITTST( R1 , 1 ); /* cc = 1 */
R4 = CC;
BITTGL( R1 , 1 ); /* r0 = 0x00000002 */
CC = ! BITTST( R1 , 1 ); /* cc = 1 */
R5 = CC;
CHECKREG r1, 0x00000003;
CHECKREG r2, 0x00000001;
CHECKREG r3, 0x00000000;
CHECKREG r4, 0x00000001;
CHECKREG r5, 0x00000000;

CC = ! BITTST( R2 , 2 ); /* cc = 0 */
R3 = CC;
BITSET( R2 , 2 ); /* r0 = 0x00000004 */
CC = ! BITTST( R2 , 2 ); /* cc = 1 */
R4 = CC;
BITCLR( R2 , 2 ); /* r0 = 0x00000000 */
CC = ! BITTST( R2 , 2 ); /* cc = 1 */
R5 = CC;
BITTGL( R2 , 2 ); /* r0 = 0x00000004 */
CC = ! BITTST( R2 , 2 ); /* cc = 1 */
R6 = CC;
CHECKREG r2, 0x00000005;
CHECKREG r3, 0x00000001;
CHECKREG r4, 0x00000000;
CHECKREG r5, 0x00000001;
CHECKREG r6, 0x00000000;

CC = ! BITTST( R3 , 3 ); /* cc = 0 */
R4 = CC;
BITSET( R3 , 3 ); /* r0 = 0x00000008 */
CC = ! BITTST( R3 , 3 ); /* cc = 1 */
R5 = CC;
BITCLR( R3 , 3 ); /* r0 = 0x00000000 */
CC = ! BITTST( R3 , 3 ); /* cc = 1 */
R6 = CC;
BITTGL( R3 , 3 ); /* r0 = 0x00000008 */
CC = ! BITTST( R3 , 3 ); /* cc = 1 */
R7 = CC;
CHECKREG r0, 0x00000001;
CHECKREG r1, 0x00000003;
CHECKREG r2, 0x00000005;
CHECKREG r3, 0x00000009;
CHECKREG r4, 0x00000001;
CHECKREG r5, 0x00000000;
CHECKREG r6, 0x00000001;
CHECKREG r7, 0x00000000;

CC = ! BITTST( R4 , 4 ); /* cc = 0 */
R5 = CC;
BITSET( R4 , 4 ); /* r0 = 0x00000010 */
CC = ! BITTST( R4 , 4 ); /* cc = 1 */
R6 = CC;
BITCLR( R4 , 4 ); /* r0 = 0x00000000 */
CC = ! BITTST( R4 , 4 ); /* cc = 1 */
R7 = CC;
BITTGL( R4 , 4 ); /* r0 = 0x00000010 */
CC = ! BITTST( R4 , 4 ); /* cc = 1 */
R0 = CC;
CHECKREG r4, 0x00000011;
CHECKREG r5, 0x00000001;
CHECKREG r6, 0x00000000;
CHECKREG r7, 0x00000001;
CHECKREG r0, 0x00000000;

CC = ! BITTST( R5 , 5 ); /* cc = 0 */
R6 = CC;
BITSET( R5 , 5 ); /* r0 = 0x00000020 */
CC = ! BITTST( R5 , 5 ); /* cc = 1 */
R7 = CC;
BITCLR( R5 , 5 ); /* r0 = 0x00000000 */
CC = ! BITTST( R5 , 5 ); /* cc = 1 */
R0 = CC;
BITTGL( R5 , 5 ); /* r0 = 0x00000020 */
CC = ! BITTST( R5 , 5 ); /* cc = 1 */
R1 = CC;
CHECKREG r5, 0x00000021;
CHECKREG r6, 0x00000001;
CHECKREG r7, 0x00000000;
CHECKREG r0, 0x00000001;
CHECKREG r1, 0x00000000;

CC = ! BITTST( R6 , 6 ); /* cc = 0 */
R7 = CC;
BITSET( R6 , 6 ); /* r0 = 0x00000040 */
CC = ! BITTST( R6 , 6 ); /* cc = 1 */
R0 = CC;
BITCLR( R6 , 6 ); /* r0 = 0x00000000 */
CC = ! BITTST( R6 , 6 ); /* cc = 1 */
R1 = CC;
BITTGL( R6 , 6 ); /* r0 = 0x00000040 */
CC = ! BITTST( R6 , 6 ); /* cc = 1 */
R2 = CC;
CHECKREG r6, 0x00000041;
CHECKREG r7, 0x00000001;
CHECKREG r0, 0x00000000;
CHECKREG r1, 0x00000001;
CHECKREG r2, 0x00000000;

CC = ! BITTST( R7 , 7 ); /* cc = 0 */
R0 = CC;
BITSET( R7 , 7 ); /* r0 = 0x00000080 */
CC = ! BITTST( R7 , 7 ); /* cc = 1 */
R1 = CC;
BITCLR( R7 , 7 ); /* r0 = 0x00000000 */
CC = ! BITTST( R7 , 7 ); /* cc = 1 */
R2 = CC;
BITTGL( R7 , 7 ); /* r0 = 0x00000080 */
CC = ! BITTST( R7 , 7 ); /* cc = 1 */
R3 = CC;
CHECKREG r0, 0x00000001;
CHECKREG r1, 0x00000000;
CHECKREG r2, 0x00000001;
CHECKREG r3, 0x00000000;

CHECKREG r4, 0x00000011;
CHECKREG r5, 0x00000021;
CHECKREG r6, 0x00000041;
CHECKREG r7, 0x00000081;

// bit(8-15) tst set clr toggle
CC = ! BITTST( R0 , 8 ); /* cc = 0 */
R1 = CC;
BITSET( R0 , 8 ); /* r0 = 0x00000101 */
CC = ! BITTST( R0 , 8 ); /* cc = 1 */
R2 = CC;
BITCLR( R0 , 8 ); /* r0 = 0x00000000 */
CC = ! BITTST( R0 , 8 ); /* cc = 1 */
R3 = CC;
BITTGL( R0 , 8 ); /* r0 = 0x00000101 */
CC = ! BITTST( R0 , 8 ); /* cc = 1 */
R4 = CC;
CHECKREG r0, 0x00000101;
CHECKREG r1, 0x00000001;
CHECKREG r2, 0x00000000;
CHECKREG r3, 0x00000001;
CHECKREG r4, 0x00000000;

CC = ! BITTST( R1 , 9 ); /* cc = 0 */
R2 = CC;
BITSET( R1 , 9 ); /* r0 = 0x00000202 */
CC = ! BITTST( R1 , 9 ); /* cc = 1 */
R3 = CC;
BITCLR( R1 , 9 ); /* r0 = 0x00000000 */
CC = ! BITTST( R1 , 9 ); /* cc = 1 */
R4 = CC;
BITTGL( R1 , 9 ); /* r0 = 0x00000202 */
CC = ! BITTST( R1 , 9 ); /* cc = 1 */
R5 = CC;
CHECKREG r1, 0x00000201;
CHECKREG r2, 0x00000001;
CHECKREG r3, 0x00000000;
CHECKREG r4, 0x00000001;
CHECKREG r5, 0x00000000;

CC = ! BITTST( R2 , 10 ); /* cc = 0 */
R3 = CC;
BITSET( R2 , 10 ); /* r0 = 0x00000404 */
CC = ! BITTST( R2 , 10 ); /* cc = 1 */
R4 = CC;
BITCLR( R2 , 10 ); /* r0 = 0x00000000 */
CC = ! BITTST( R2 , 10 ); /* cc = 1 */
R5 = CC;
BITTGL( R2 , 10 ); /* r0 = 0x00000404 */
CC = ! BITTST( R2 , 10 ); /* cc = 1 */
R6 = CC;
CHECKREG r2, 0x00000401;
CHECKREG r3, 0x00000001;
CHECKREG r4, 0x00000000;
CHECKREG r5, 0x00000001;
CHECKREG r6, 0x00000000;

CC = ! BITTST( R3 , 11 ); /* cc = 0 */
R4 = CC;
BITSET( R3 , 11 ); /* r0 = 0x00000808 */
CC = ! BITTST( R3 , 11 ); /* cc = 1 */
R5 = CC;
BITCLR( R3 , 11 ); /* r0 = 0x00000000 */
CC = ! BITTST( R3 , 11 ); /* cc = 1 */
R6 = CC;
BITTGL( R3 , 11 ); /* r0 = 0x00000808 */
CC = ! BITTST( R3 , 11 ); /* cc = 1 */
R7 = CC;
CHECKREG r3, 0x00000801;
CHECKREG r4, 0x00000001;
CHECKREG r5, 0x00000000;
CHECKREG r6, 0x00000001;
CHECKREG r7, 0x00000000;

CC = ! BITTST( R4 , 12 ); /* cc = 0 */
R5 = CC;
BITSET( R4 , 12 ); /* r0 = 0x00001010 */
CC = ! BITTST( R4 , 12 ); /* cc = 1 */
R6 = CC;
BITCLR( R4 , 12 ); /* r0 = 0x00000000 */
CC = ! BITTST( R4 , 12 ); /* cc = 1 */
R7 = CC;
BITTGL( R4 , 12 ); /* r0 = 0x00001010 */
CC = ! BITTST( R4 , 12 ); /* cc = 1 */
R0 = CC;
CHECKREG r4, 0x00001001;
CHECKREG r5, 0x00000001;
CHECKREG r6, 0x00000000;
CHECKREG r7, 0x00000001;
CHECKREG r0, 0x00000000;

CC = ! BITTST( R5 , 13 ); /* cc = 0 */
R6 = CC;
BITSET( R5 , 13 ); /* r0 = 0x00002020 */
CC = ! BITTST( R5 , 13 ); /* cc = 1 */
R7 = CC;
BITCLR( R5 , 13 ); /* r0 = 0x00000000 */
CC = ! BITTST( R5 , 13 ); /* cc = 1 */
R0 = CC;
BITTGL( R5 , 13 ); /* r0 = 0x00002020 */
CC = ! BITTST( R5 , 13 ); /* cc = 1 */
R1 = CC;
CHECKREG r5, 0x00002001;
CHECKREG r6, 0x00000001;
CHECKREG r7, 0x00000000;
CHECKREG r0, 0x00000001;
CHECKREG r1, 0x00000000;

CC = ! BITTST( R6 , 14 ); /* cc = 0 */
R7 = CC;
BITSET( R6 , 14 ); /* r0 = 0x00004040 */
CC = ! BITTST( R6 , 14 ); /* cc = 1 */
R0 = CC;
BITCLR( R6 , 14 ); /* r0 = 0x00000000 */
CC = ! BITTST( R6 , 14 ); /* cc = 1 */
R1 = CC;
BITTGL( R6 , 14 ); /* r0 = 0x00004040 */
CC = ! BITTST( R6 , 14 ); /* cc = 1 */
R2 = CC;
CHECKREG r6, 0x00004001;
CHECKREG r7, 0x00000001;
CHECKREG r0, 0x00000000;
CHECKREG r1, 0x00000001;
CHECKREG r2, 0x00000000;

CC = ! BITTST( R7 , 15 ); /* cc = 0 */
R0 = CC;
BITSET( R7 , 15 ); /* r0 = 0x00008080 */
CC = ! BITTST( R7 , 15 ); /* cc = 1 */
R1 = CC;
BITCLR( R7 , 15 ); /* r0 = 0x00000000 */
CC = ! BITTST( R7 , 15 ); /* cc = 1 */
R2 = CC;
BITTGL( R7 , 15 ); /* r0 = 0x00008080 */
CC = ! BITTST( R7 , 15 ); /* cc = 1 */
R3 = CC;
CHECKREG r0, 0x00000001;
CHECKREG r1, 0x00000000;
CHECKREG r2, 0x00000001;
CHECKREG r3, 0x00000000;
CHECKREG r4, 0x00001001;
CHECKREG r5, 0x00002001;
CHECKREG r6, 0x00004001;
CHECKREG r7, 0x00008001;

// bit(16-23) tst set clr toggle
CC = ! BITTST( R0 , 16 ); /* cc = 0 */
R1 = CC;
BITSET( R0 , 16 ); /* r0 = 0x00000001 */
CC = ! BITTST( R0 , 16 ); /* cc = 1 */
R2 = CC;
BITCLR( R0 , 16 ); /* r0 = 0x00000000 */
CC = ! BITTST( R0 , 16 ); /* cc = 1 */
R3 = CC;
BITTGL( R0 , 16 ); /* r0 = 0x00000001 */
CC = ! BITTST( R0 , 16 ); /* cc = 1 */
R4 = CC;
CHECKREG r0, 0x00010001;
CHECKREG r1, 0x00000001;
CHECKREG r2, 0x00000000;
CHECKREG r3, 0x00000001;
CHECKREG r4, 0x00000000;

CC = ! BITTST( R1 , 17 ); /* cc = 0 */
R2 = CC;
BITSET( R1 , 17 ); /* r0 = 0x00000002 */
CC = ! BITTST( R1 , 17 ); /* cc = 1 */
R3 = CC;
BITCLR( R1 , 17 ); /* r0 = 0x00000000 */
CC = ! BITTST( R1 , 17 ); /* cc = 1 */
R4 = CC;
BITTGL( R1 , 17 ); /* r0 = 0x00000002 */
CC = ! BITTST( R1 , 17 ); /* cc = 1 */
R5 = CC;
CHECKREG r1, 0x00020001;
CHECKREG r2, 0x00000001;
CHECKREG r3, 0x00000000;
CHECKREG r4, 0x00000001;
CHECKREG r5, 0x00000000;

CC = ! BITTST( R2 , 18 ); /* cc = 0 */
R3 = CC;
BITSET( R2 , 18 ); /* r0 = 0x00000004 */
CC = ! BITTST( R2 , 18 ); /* cc = 1 */
R4 = CC;
BITCLR( R2 , 18 ); /* r0 = 0x00000000 */
CC = ! BITTST( R2 , 18 ); /* cc = 1 */
R4 = CC;
BITTGL( R2 , 18 ); /* r0 = 0x00000004 */
CC = ! BITTST( R2 , 18 ); /* cc = 1 */
R5 = CC;
CHECKREG r2, 0x00040001;
CHECKREG r3, 0x00000001;
CHECKREG r4, 0x00000001;
CHECKREG r5, 0x00000000;
CHECKREG r6, 0x00004001;

CC = ! BITTST( R3 , 19 ); /* cc = 0 */
R4 = CC;
BITSET( R3 , 19 ); /* r0 = 0x00000008 */
CC = ! BITTST( R3 , 19 ); /* cc = 1 */
R5 = CC;
BITCLR( R3 , 19 ); /* r0 = 0x00000000 */
CC = ! BITTST( R3 , 19 ); /* cc = 1 */
R6 = CC;
BITTGL( R3 , 19 ); /* r0 = 0x00000008 */
CC = ! BITTST( R3 , 19 ); /* cc = 1 */
R7 = CC;
CHECKREG r3, 0x00080001;
CHECKREG r4, 0x00000001;
CHECKREG r5, 0x00000000;
CHECKREG r6, 0x00000001;
CHECKREG r7, 0x00000000;

CC = ! BITTST( R4 , 20 ); /* cc = 0 */
R5 = CC;
BITSET( R4 , 20 ); /* r0 = 0x00000010 */
CC = ! BITTST( R4 , 20 ); /* cc = 1 */
R6 = CC;
BITCLR( R4 , 20 ); /* r0 = 0x00000000 */
CC = ! BITTST( R4 , 20 ); /* cc = 1 */
R7 = CC;
BITTGL( R4 , 20 ); /* r0 = 0x00000010 */
CC = ! BITTST( R4 , 20 ); /* cc = 1 */
R0 = CC;
CHECKREG r4, 0x00100001;
CHECKREG r5, 0x00000001;
CHECKREG r6, 0x00000000;
CHECKREG r7, 0x00000001;
CHECKREG r0, 0x00000000;

CC = ! BITTST( R5 , 21 ); /* cc = 0 */
R6 = CC;
BITSET( R5 , 21 ); /* r0 = 0x00000020 */
CC = ! BITTST( R5 , 21 ); /* cc = 1 */
R7 = CC;
BITCLR( R5 , 21 ); /* r0 = 0x00000000 */
CC = ! BITTST( R5 , 21 ); /* cc = 1 */
R0 = CC;
BITTGL( R5 , 21 ); /* r0 = 0x00000020 */
CC = ! BITTST( R5 , 21 ); /* cc = 1 */
R1 = CC;
CHECKREG r5, 0x00200001;
CHECKREG r6, 0x00000001;
CHECKREG r7, 0x00000000;
CHECKREG r0, 0x00000001;
CHECKREG r1, 0x00000000;

CC = ! BITTST( R6 , 22 ); /* cc = 0 */
R7 = CC;
BITSET( R6 , 22 ); /* r0 = 0x00000040 */
CC = ! BITTST( R6 , 22 ); /* cc = 1 */
R0 = CC;
BITCLR( R6 , 22 ); /* r0 = 0x00000000 */
CC = ! BITTST( R6 , 22 ); /* cc = 1 */
R1 = CC;
BITTGL( R6 , 22 ); /* r0 = 0x00000040 */
CC = ! BITTST( R6 , 22 ); /* cc = 1 */
R2 = CC;
CHECKREG r6, 0x00400001;
CHECKREG r7, 0x00000001;
CHECKREG r0, 0x00000000;
CHECKREG r1, 0x00000001;
CHECKREG r2, 0x00000000;

CC = ! BITTST( R7 , 23 ); /* cc = 0 */
R0 = CC;
BITSET( R7 , 23 ); /* r0 = 0x00000080 */
CC = ! BITTST( R7 , 23 ); /* cc = 1 */
R1 = CC;
BITCLR( R7 , 23 ); /* r0 = 0x00000000 */
CC = ! BITTST( R7 , 23 ); /* cc = 1 */
R2 = CC;
BITTGL( R7 , 23 ); /* r0 = 0x00000080 */
CC = ! BITTST( R7 , 23 ); /* cc = 1 */
R3 = CC;
CHECKREG r0, 0x00000001;
CHECKREG r1, 0x00000000;
CHECKREG r2, 0x00000001;
CHECKREG r3, 0x00000000;
CHECKREG r4, 0x00100001;
CHECKREG r5, 0x00200001;
CHECKREG r6, 0x00400001;
CHECKREG r7, 0x00800001;

// bit(24-31) tst set clr toggle
CC = ! BITTST( R0 , 24 ); /* cc = 0 */
R1 = CC;
BITSET( R0 , 24 ); /* r0 = 0x00000101 */
CC = ! BITTST( R0 , 24 ); /* cc = 1 */
R2 = CC;
BITCLR( R0 , 24 ); /* r0 = 0x00000000 */
CC = ! BITTST( R0 , 24 ); /* cc = 1 */
R3 = CC;
BITTGL( R0 , 24 ); /* r0 = 0x00000101 */
CC = ! BITTST( R0 , 24 ); /* cc = 1 */
R4 = CC;
CHECKREG r0, 0x01000001;
CHECKREG r1, 0x00000001;
CHECKREG r2, 0x00000000;
CHECKREG r3, 0x00000001;
CHECKREG r4, 0x00000000;

CC = ! BITTST( R1 , 25 ); /* cc = 0 */
R2 = CC;
BITSET( R1 , 25 ); /* r0 = 0x00000202 */
CC = ! BITTST( R1 , 25 ); /* cc = 1 */
R3 = CC;
BITCLR( R1 , 25 ); /* r0 = 0x00000000 */
CC = ! BITTST( R1 , 25 ); /* cc = 1 */
R4 = CC;
BITTGL( R1 , 25 ); /* r0 = 0x00000202 */
CC = ! BITTST( R1 , 25 ); /* cc = 1 */
R5 = CC;
CHECKREG r1, 0x02000001;
CHECKREG r2, 0x00000001;
CHECKREG r3, 0x00000000;
CHECKREG r4, 0x00000001;
CHECKREG r5, 0x00000000;

CC = ! BITTST( R2 , 26 ); /* cc = 0 */
R3 = CC;
BITSET( R2 , 26 ); /* r0 = 0x00000404 */
CC = ! BITTST( R2 , 26 ); /* cc = 1 */
R4 = CC;
BITCLR( R2 , 26 ); /* r0 = 0x00000000 */
CC = ! BITTST( R2 , 26 ); /* cc = 1 */
R5 = CC;
BITTGL( R2 , 26 ); /* r0 = 0x00000404 */
CC = ! BITTST( R2 , 26 ); /* cc = 1 */
R6 = CC;
CHECKREG r2, 0x04000001;
CHECKREG r3, 0x00000001;
CHECKREG r4, 0x00000000;
CHECKREG r5, 0x00000001;
CHECKREG r6, 0x00000000;

CC = ! BITTST( R3 , 27 ); /* cc = 0 */
R4 = CC;
BITSET( R3 , 27 ); /* r0 = 0x00000808 */
CC = ! BITTST( R3 , 27 ); /* cc = 1 */
R5 = CC;
BITCLR( R3 , 27 ); /* r0 = 0x00000000 */
CC = ! BITTST( R3 , 27 ); /* cc = 1 */
R6 = CC;
BITTGL( R3 , 27 ); /* r0 = 0x00000808 */
CC = ! BITTST( R3 , 27 ); /* cc = 1 */
R7 = CC;
CHECKREG r3, 0x08000001;
CHECKREG r4, 0x00000001;
CHECKREG r5, 0x00000000;
CHECKREG r6, 0x00000001;
CHECKREG r7, 0x00000000;

CC = ! BITTST( R4 , 28 ); /* cc = 0 */
R5 = CC;
BITSET( R4 , 28 ); /* r0 = 0x00001010 */
CC = ! BITTST( R4 , 28 ); /* cc = 1 */
R6 = CC;
BITCLR( R4 , 28 ); /* r0 = 0x00000000 */
CC = ! BITTST( R4 , 28 ); /* cc = 1 */
R7 = CC;
BITTGL( R4 , 28 ); /* r0 = 0x00001010 */
CC = ! BITTST( R4 , 28 ); /* cc = 1 */
R0 = CC;
CHECKREG r4, 0x10000001;
CHECKREG r5, 0x00000001;
CHECKREG r6, 0x00000000;
CHECKREG r7, 0x00000001;
CHECKREG r0, 0x00000000;

CC = ! BITTST( R5 , 29 ); /* cc = 0 */
R6 = CC;
BITSET( R5 , 29 ); /* r0 = 0x00002020 */
CC = ! BITTST( R5 , 29 ); /* cc = 1 */
R7 = CC;
BITCLR( R5 , 29 ); /* r0 = 0x00000000 */
CC = ! BITTST( R5 , 29 ); /* cc = 1 */
R0 = CC;
BITTGL( R5 , 29 ); /* r0 = 0x00002020 */
CC = ! BITTST( R5 , 29 ); /* cc = 1 */
R1 = CC;
CHECKREG r5, 0x20000001;
CHECKREG r6, 0x00000001;
CHECKREG r7, 0x00000000;
CHECKREG r0, 0x00000001;
CHECKREG r1, 0x00000000;

CC = ! BITTST( R6 , 30 ); /* cc = 0 */
R7 = CC;
BITSET( R6 , 30 ); /* r0 = 0x00004040 */
CC = ! BITTST( R6 , 30 ); /* cc = 1 */
R0 = CC;
BITCLR( R6 , 30 ); /* r0 = 0x00000000 */
CC = ! BITTST( R6 , 30 ); /* cc = 1 */
R1 = CC;
BITTGL( R6 , 30 ); /* r0 = 0x00004040 */
CC = ! BITTST( R6 , 30 ); /* cc = 1 */
R2 = CC;
CHECKREG r6, 0x40000001;
CHECKREG r7, 0x00000001;
CHECKREG r0, 0x00000000;
CHECKREG r1, 0x00000001;
CHECKREG r2, 0x00000000;

CC = ! BITTST( R7 , 31 ); /* cc = 0 */
R0 = CC;
BITSET( R7 , 31 ); /* r0 = 0x00008080 */
CC = ! BITTST( R7 , 31 ); /* cc = 1 */
R1 = CC;
BITCLR( R7 , 31 ); /* r0 = 0x00000000 */
CC = ! BITTST( R7 , 31 ); /* cc = 1 */
R2 = CC;
BITTGL( R7 , 31 ); /* r0 = 0x80808080 */
CC = ! BITTST( R7 , 31 ); /* cc = 1 */
R3 = CC;
CHECKREG r0, 0x00000001;
CHECKREG r1, 0x00000000;
CHECKREG r2, 0x00000001;
CHECKREG r3, 0x00000000;
CHECKREG r4, 0x10000001;
CHECKREG r5, 0x20000001;
CHECKREG r6, 0x40000001;
CHECKREG r7, 0x80000001;

pass
