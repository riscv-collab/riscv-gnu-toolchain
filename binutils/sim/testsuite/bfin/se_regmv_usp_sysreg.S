//Original:/proj/frio/dv/testcases/seq/se_regmv_usp_sysreg/se_regmv_usp_sysreg.dsp
// Description: RegMV USP to SYSREG
# mach: bfin
# sim: --environment operating

#include "test.h"
.include "testutils.inc"
start

//
// Constants and Defines
//

include(selfcheck.inc)
include(std.inc)
include(symtable.inc)

//*********************************************************************

BEGIN:

    // KLUDGE: from perl script must place cycles 2 write before cycles
    // write, and cycles 2 read AFTER cycles read

    // PUT YOUR TEST HERE!
    R0 = 0;
SP = R0;
SYSCFG = R0;

CHECK_INIT_DEF(p0); //CHECK_INIT(p0, 0xFF7FFFFC);

    R0 = 0x59c4 (Z);
    R0.H = 0x95a6;
USP = R0;
ASTAT = USP;
    R1 = ASTAT;

    R0 = 0xd4a4 (Z);
    R0.H = 0xd16c;
USP = R0;
RETS = USP;
    R1 = RETS;
CHECKREG(r1, 3513570468);

    R0 = 0x2bca (Z);
    R0.H = 0x6ad8;
USP = R0;
LC0 = USP;
    R1 = LC0;
CHECKREG(r1, 1792551882);

    R0 = 0x6d4a (Z);
    R0.H = 0xada2;
USP = R0;
LT0 = USP;
    R1 = LT0;
CHECKREG(r1, 2913103178);

    R0 = 0x6b18 (Z);
    R0.H = 0x931c;
USP = R0;
LB0 = USP;
    R1 = LB0;
CHECKREG(r1, 2468113176);

    R0 = 0x62da (Z);
    R0.H = 0x16ee;
USP = R0;
LC1 = USP;
    R1 = LC1;
CHECKREG(r1, 384721626);

    R0 = 0x7c60 (Z);
    R0.H = 0xf7c8;
USP = R0;
LT1 = USP;
    R1 = LT1;
CHECKREG(r1, 4157111392);

    R0 = 0x182 (Z);
    R0.H = 0x942;
USP = R0;
LB1 = USP;
    R1 = LB1;
CHECKREG(r1, 155320706);

    R0 = 0xd5a2 (Z);
    R0.H = 0x8782;
USP = R0;
CYCLES2 = USP;
                    // KLUDGE - moved read after that for cycles

    R0 = 0x297c (Z);
    R0.H = 0x9d06;
USP = R0;
CYCLES = USP;
    R1 = CYCLES;
CHECKREG(r1, 2634426748);
    R1 = CYCLES2;   // KLUDGE moved read after that for cycles
CHECKREG(r1, 2273498530);

    R0 = 0x8c66 (Z);
    R0.H = 0x3d64;
USP = R0;
SEQSTAT = USP;
    R1 = SEQSTAT;

    R0 = 0x3b8c (Z);
    R0.H = 0xdcd4;
USP = R0;
SYSCFG = USP;
    R1 = SYSCFG;

    R0 = 0xb1ae (Z);
    R0.H = 0x6f6;
USP = R0;
RETI = USP;
    R1 = RETI;
CHECKREG(r1, 116830638);

    R0 = 0x32b0 (Z);
    R0.H = 0x9b7e;
USP = R0;
RETX = USP;
    R1 = RETX;
CHECKREG(r1, 2608738992);

    R0 = 0xea72 (Z);
    R0.H = 0x11ea;
USP = R0;
RETN = USP;
    R1 = RETN;
CHECKREG(r1, 300608114);

    R0 = 0x2c58 (Z);
    R0.H = 0xb13a;
USP = R0;
RETE = USP;
    R1 = RETE;
CHECKREG(r1, 2973379672);

// Sanity check
USP = R0;
USP = R1;
USP = R2;
USP = R3;
USP = R4;
USP = R5;
USP = R6;
USP = R7;
USP = P0;
USP = P1;
USP = P2;
USP = P3;
USP = P4;
USP = P5;
USP = SP;
USP = FP;
USP = A0.X;
USP = A0.W;
USP = A1.X;
USP = A1.W;
A0.X = USP;
A0.W = USP;
A1.X = USP;
A1.W = USP;

END:
dbg_pass;            // End the test

//*********************************************************************
