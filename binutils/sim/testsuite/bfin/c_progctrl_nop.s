//Original:/testcases/core/c_progctrl_nop/c_progctrl_nop.dsp
// Spec Reference: progctrl nop
# mach: bfin

.include "testutils.inc"
	start


INIT_R_REGS 0;


I0 = 0x1122 (Z);
NOP;
R0 = I0;

I1 = 0x3344 (Z);
NOP;
R1 = I1;

I2 = 0x5566 (Z);
NOP;
R2 = I2;

I3 = 0x7788 (Z);
NOP;
R3 = I3;


P2 = 0x99aa (Z);
NOP; NOP;
R4 = P2;

P3 = 0xbbcc (Z);
NOP; NOP;
R5 = P3;

P4 = 0xddee (Z);
NOP; NOP;
R6 = P4;

P5 = 0x1234 (Z);
NOP; NOP;
R7 = P5;

CHECKREG r0, 0x00001122;
CHECKREG r1, 0x00003344;
CHECKREG r2, 0x00005566;
CHECKREG r3, 0x00007788;
CHECKREG r4, 0x000099AA;
CHECKREG r5, 0x0000BBCC;
CHECKREG r6, 0x0000DDEE;
CHECKREG r7, 0x00001234;


pass
