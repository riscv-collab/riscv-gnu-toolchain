//Original:/testcases/core/c_ujump/c_ujump.dsp
// Spec Reference: ujump
# mach: bfin

.include "testutils.inc"
	start


INIT_R_REGS 0;

ASTAT = r0;

JUMP.S LAB1;


STOP:
JUMP.S END;

LAB1:
 R1 = 0x1111 (X);
JUMP.S LAB5;
 R6 = 0x6666 (X);

LAB2:
 R2 = 0x2222 (X);
JUMP.S STOP;

LAB3:
 R3 = 0x3333 (X);
JUMP.S LAB2;
 R7 = 0x7777 (X);

LAB4:
 R4 = 0x4444 (X);
JUMP.S LAB3;

LAB5:
 R5 = 0x5555 (X);
JUMP.S LAB4;

END:

CHECKREG r0, 0x00000000;
CHECKREG r1, 0x00001111;
CHECKREG r2, 0x00002222;
CHECKREG r3, 0x00003333;
CHECKREG r4, 0x00004444;
CHECKREG r5, 0x00005555;
CHECKREG r6, 0x00000000;
CHECKREG r7, 0x00000000;

pass
