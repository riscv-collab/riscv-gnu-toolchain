//Original:/proj/frio/dv/testcases/core/c_progctrl_except_rtx/c_progctrl_except_rtx.dsp
// Spec Reference: c_progctrl_except_rtx
# mach: bfin
# sim: --environment operating

#include "test.h"
.include "testutils.inc"
start

include(std.inc)
include(selfcheck.inc)
INIT_R_REGS(0);
INIT_P_REGS(0);
//CHECK_INIT(p5, 0xe0000000);
include(symtable.inc)
CHECK_INIT_DEF(p5);
// load address of exception handler

P0 = 0x200C (Z);              // 0xFFE0200C EVT3 EXCEPTION
P0.H = 0xFFE0;
R0 = exception_handler (Z);   // wr address of exception handler to MMR EVT3
R0.H = exception_handler;
[ P0 ] = R0;

//  Jump to User mode and enable exceptions

R0 = MidUserCode (Z);
R0.H = MidUserCode;
RETI = R0;
RTI;            // cause it to go to Midusercode, .dd cause exception

BeginUserCode:
P1 = 1;
P2 = 2;
P3 = 3;
P4 = 4;

CHECKREG(r0, 0x00000000);
CHECKREG(r1, 0x00000001);
CHECKREG(r2, 0x00000002);
CHECKREG(r3, 0x00000003);
CHECKREG(r5, 0x00000000);
CHECKREG(r6, 0x00000000);
CHECKREG(r7, 0x00000000);
CHECKREG(p1, 0x00000001);
CHECKREG(p2, 0x00000002);
CHECKREG(p3, 0x00000003);
CHECKREG(p4, 0x00000004);

dbg_pass;
//jump 2;
//jump -2;
.dd 0xFFFFFFFF
.dd 0xFFFFFFFF
.dd 0xFFFFFFFF
.dd 0xFFFFFFFF
.dd 0xFFFFFFFF
.dd 0xFFFFFFFF
.dd 0xFFFFFFFF
.dd 0xFFFFFFFF

//dbg_pass;

MidUserCode:
.dd 0xFFFFFFFF
R0 = 0;
R1 = 1;
R2 = 2;
R3 = 3;
CC = R0;
IF !CC JUMP BeginUserCode;

.dd 0xFFFFFFFF
.dd 0xFFFFFFFF
.dd 0xFFFFFFFF
.dd 0xFFFFFFFF
.dd 0xFFFFFFFF
.dd 0xFFFFFFFF
.dd 0xFFFFFFFF
.dd 0xFFFFFFFF


exception_handler:
R4 = RETX;   // error handler: RETX has the address of the same Illegal instr
R1 += 1;
R2 += 2;
R3 += 3;
R1 += 1;
R4 += 4;     // we have to add 4 to point to next instr after return
RETX = R4;

RTX;  // return from exception

.section MEM_DATA_ADDR_1,"aw"
.dd 0xDEADBEEF
.dd 0xBAD00BAD
