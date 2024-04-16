//Original:/proj/frio/dv/testcases/seq/se_ssync/se_ssync.dsp
// Description: Test SSYNC by writing a bunch of MMRs and verifying read
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
include(mmrs.inc)
include(symtable.inc)

#ifndef STACKSIZE
#define STACKSIZE 0x10   // change for how much stack you need
#endif

LD32(p0, EVT5);
LD32(r0, 0x55555555);
LD32(p1, EVT6);
LD32(r1, 0xAAAAAAAA);
LD32(p2, EVT7);
LD32(r2, 0xBABEFACE);
LD32(p3, EVT8);
LD32(r3, 0xCFCFCFCF);
LD32(p4, EVT9);
LD32(r4, 0xDEADBEEF);
LD32(p5, EVT10);
LD32(r5, 0xBAD1BAD1);

    [ P0 ] = R0;      // write the MMRS
    [ P1 ] = R1;
    [ P2 ] = R2;
    [ P3 ] = R3;
    [ P4 ] = R4;
    [ P5 ] = R5;

SSYNC;          // wait for it

    R7 = [ P5 ];      // read back MMRs
    R6 = [ P4 ];      // should be updated
    R5 = [ P3 ];
    R4 = [ P2 ];
    R3 = [ P1 ];
    R2 = [ P0 ];

CHECK_INIT_DEF(p0); //CHECK_INIT(p0, 0xFF7FFFFC);

CHECKREG(r2, 0x55555555);
CHECKREG(r3, 0xAAAAAAAA);
CHECKREG(r4, 0xBABEFACE);
CHECKREG(r5, 0xCFCFCFCF);
CHECKREG(r6, 0xDEADBEEF);
CHECKREG(r7, 0xBAD1BAD1);

dbg_pass;
