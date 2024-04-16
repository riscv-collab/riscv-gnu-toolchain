# Blackfin testcase for insns that implicitly have DISALGNEXCPT behavior
# when used in parallel insns
# mach: bfin

#include "test.h"
	.include "testutils.inc"

	start

	LINK 0x100;

	# Set up I0/I1/I2/I3 to be unaligned by 0/1/2/3 bytes
	init_l_regs 0
	init_m_regs 0
	R0 = SP;
	BITCLR (R0, 0);
	BITCLR (R0, 1);
	I0 = R0;
	B0 = R0;
	R1 = 1;
	R1 = R0 + R1;
	I1 = R1;
	B1 = R1;
	R2 = 2;
	R2 = R0 + R2;
	I2 = R2;
	B2 = R2;
	R3 = 3;
	R3 = R0 + R3;
	I3 = R3;
	B3 = R3;

#define EXP_VAL 0x12345678
#define EXP(r, n) CHECKREG (r, EXP_VAL); r = 0; I##n = B##n
	imm32 R5, EXP_VAL;
	imm32 R6, 0x9abcdef0;
	imm32 R7, 0x0a1b2c3e;
	[SP] = R5;
	[SP - 4] = R6;
	[SP + 4] = R7;

#define BYTEPACK(n) \
	R7 = BYTEPACK (R0, R1) || R4 = [I##n];         EXP (R4, n); \
	R6 = BYTEPACK (R0, R1) || R5 = [I##n ++ M##n]; EXP (R5, n); \
	R5 = BYTEPACK (R0, R1) || R6 = [I##n++];       EXP (R6, n); \
	R4 = BYTEPACK (R0, R1) || R7 = [I##n--];       EXP (R7, n);
	BYTEPACK(0)
	BYTEPACK(1)
	BYTEPACK(2)
	BYTEPACK(3)

#define BYTEUNPACK(n) \
	(R7, R5) = BYTEUNPACK R1:0 || R4 = [I##n];         EXP (R4, n); \
	(R6, R7) = BYTEUNPACK R3:2 || R5 = [I##n ++ M##n]; EXP (R5, n); \
	(R5, R4) = BYTEUNPACK R1:0 || R6 = [I##n++];       EXP (R6, n); \
	(R4, R6) = BYTEUNPACK R3:2 || R7 = [I##n--];       EXP (R7, n);
	BYTEUNPACK(0)
	BYTEUNPACK(1)
	BYTEUNPACK(2)
	BYTEUNPACK(3)

#define SAA(n) \
	SAA (R1:0, R3:2) || R4 = [I##n];         EXP (R4, n); \
	SAA (R1:0, R3:2) || R5 = [I##n ++ M##n]; EXP (R5, n); \
	SAA (R1:0, R3:2) || R6 = [I##n++];       EXP (R6, n); \
	SAA (R1:0, R3:2) || R7 = [I##n--];       EXP (R7, n);
	SAA(0)
	SAA(1)
	SAA(2)
	SAA(3)

#define BYTEOP1P(n) \
	R7 = BYTEOP1P (R1:0, R3:2) || R4 = [I##n];         EXP (R4, n); \
	R6 = BYTEOP1P (R1:0, R3:2) || R5 = [I##n ++ M##n]; EXP (R5, n); \
	R5 = BYTEOP1P (R1:0, R3:2) || R6 = [I##n++];       EXP (R6, n); \
	R4 = BYTEOP1P (R1:0, R3:2) || R7 = [I##n--];       EXP (R7, n);
	BYTEOP1P(0)
	BYTEOP1P(1)
	BYTEOP1P(2)
	BYTEOP1P(3)

#define BYTEOP2P(n) \
	R7 = BYTEOP2P (R1:0, R3:2) (TL)   || R4 = [I##n];         EXP (R4, n); \
	R6 = BYTEOP2P (R1:0, R3:2) (TH)   || R5 = [I##n ++ M##n]; EXP (R5, n); \
	R5 = BYTEOP2P (R1:0, R3:2) (RNDL) || R6 = [I##n++];       EXP (R6, n); \
	R4 = BYTEOP2P (R1:0, R3:2) (RNDH) || R7 = [I##n--];       EXP (R7, n);
	BYTEOP2P(0)
	BYTEOP2P(1)
	BYTEOP2P(2)
	BYTEOP2P(3)

#define BYTEOP3P(n) \
	R7 = BYTEOP3P (R1:0, R3:2) (LO) || R4 = [I##n];         EXP (R4, n); \
	R6 = BYTEOP3P (R1:0, R3:2) (HI) || R5 = [I##n ++ M##n]; EXP (R5, n); \
	R5 = BYTEOP3P (R1:0, R3:2) (LO) || R6 = [I##n++];       EXP (R6, n); \
	R4 = BYTEOP3P (R1:0, R3:2) (HI) || R7 = [I##n--];       EXP (R7, n);
	BYTEOP3P(0)
	BYTEOP3P(1)
	BYTEOP3P(2)
	BYTEOP3P(3)

#define BYTEOP16P(n) \
	(R7, R6) = BYTEOP16P (R1:0, R3:2) || R4 = [I##n];         EXP (R4, n); \
	(R6, R4) = BYTEOP16P (R1:0, R3:2) || R5 = [I##n ++ M##n]; EXP (R5, n); \
	(R5, R7) = BYTEOP16P (R1:0, R3:2) || R6 = [I##n++];       EXP (R6, n); \
	(R4, R6) = BYTEOP16P (R1:0, R3:2) || R7 = [I##n--];       EXP (R7, n);
	BYTEOP16P(0)
	BYTEOP16P(1)
	BYTEOP16P(2)
	BYTEOP16P(3)

#define BYTEOP16M(n) \
	(R7, R5) = BYTEOP16M (R1:0, R3:2) || R4 = [I##n];         EXP (R4, n); \
	(R6, R7) = BYTEOP16M (R1:0, R3:2) || R5 = [I##n ++ M##n]; EXP (R5, n); \
	(R5, R4) = BYTEOP16M (R1:0, R3:2) || R6 = [I##n++];       EXP (R6, n); \
	(R4, R5) = BYTEOP16M (R1:0, R3:2) || R7 = [I##n--];       EXP (R7, n);
	BYTEOP16M(0)
	BYTEOP16M(1)
	BYTEOP16M(2)
	BYTEOP16M(3)

	pass
