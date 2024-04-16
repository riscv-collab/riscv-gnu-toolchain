//Original:/testcases/core/c_progctrl_jump_pcpr/c_progctrl_jump_pcpr.dsp
// Spec Reference: progctrl jump pc+pr
# mach: bfin

.include "testutils.inc"
	start


INIT_R_REGS 0;

ASTAT = r0;

 P2 = 0x0004;

JMP:
 JUMP ( PC + P2 );
// jump JMP;

STOP:
JUMP.S END;

LAB1:
 P2 = 0x000c;
 R1 = 0x1111 (X);
JUMP.S JMP;

LAB2:
 P2 = 0x0014;
 R2 = 0x2222 (X);
JUMP.S JMP;

LAB3:
 P2 = 0x001c;
 R3 = 0x3333 (X);
JUMP.S JMP;

LAB4:
 P2 = 0x0024;
 R4 = 0x4444 (X);
JUMP.S JMP;

LAB5:
 P2 = 0x0002;
 R5 = 0x5555 (X);
JUMP.S JMP;

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
