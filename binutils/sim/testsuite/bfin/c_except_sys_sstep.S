//Original:/proj/frio/dv/testcases/core/c_except_sys_sstep/c_except_sys_sstep.dsp
// Spec Reference: Single Step Supervisor Exception Test (NO REGTRACE!)
# mach: bfin
# sim: --environment operating

#include "test.h"
.include "testutils.inc"
start

include(gen_int.inc)
include(selfcheck.inc)
include(std.inc)

#ifndef STACKSIZE
#define STACKSIZE 0x10
#endif
#ifndef EVT
#define EVT  0xFFE02000
#endif
#ifndef EVT15
#define EVT15  0xFFE0203C
#endif
#ifndef EVT_OVERRIDE
#define EVT_OVERRIDE 0xFFE02100
#endif
//

////MY_GEN_INT_INIT(0xF0000000) // set location for interrupt table

//
// Reset/Bootstrap Code
//   (Here we should set the processor operating modes, initialize registers,
//    etc.)
//

BOOT:
INIT_R_REGS(0);     // initialize general purpose regs

INIT_P_REGS(0);     // initialize the pointers

INIT_I_REGS(0);     // initialize the dsp address regs
INIT_M_REGS(0);
INIT_L_REGS(0);
INIT_B_REGS(0);
//    CHECK_INIT(p2, 0x2000);
include(symtable.inc)
CHECK_INIT_DEF(p2);


LD32_LABEL(sp, KSTACK);   // setup the stack pointer
FP = SP;        // and frame pointer

LD32(p0, EVT);      // Setup Event Vectors and Handlers

LD32_LABEL(r0, EHANDLE);  // Emulation Handler (Int0)
    [ P0 ++ ] = R0;

LD32_LABEL(r0, RHANDLE);  // Reset Handler (Int1)
    [ P0 ++ ] = R0;

LD32_LABEL(r0, NHANDLE);  // NMI Handler (Int2)
    [ P0 ++ ] = R0;

LD32_LABEL(r0, XHANDLE);  // Exception Handler (Int3)
    [ P0 ++ ] = R0;

    [ P0 ++ ] = R0;        // IVT4 not used

LD32_LABEL(r0, HWHANDLE); // HW Error Handler (Int5)
    [ P0 ++ ] = R0;

LD32_LABEL(r0, THANDLE);  // Timer Handler (Int6)
    [ P0 ++ ] = R0;

LD32_LABEL(r0, I7HANDLE); // IVG7 Handler
    [ P0 ++ ] = R0;

LD32_LABEL(r0, I8HANDLE); // IVG8 Handler
    [ P0 ++ ] = R0;

LD32_LABEL(r0, I9HANDLE); // IVG9 Handler
    [ P0 ++ ] = R0;

LD32_LABEL(r0, I10HANDLE);// IVG10 Handler
    [ P0 ++ ] = R0;

LD32_LABEL(r0, I11HANDLE);// IVG11 Handler
    [ P0 ++ ] = R0;

LD32_LABEL(r0, I12HANDLE);// IVG12 Handler
    [ P0 ++ ] = R0;

LD32_LABEL(r0, I13HANDLE);// IVG13 Handler
    [ P0 ++ ] = R0;

LD32_LABEL(r0, I14HANDLE);// IVG14 Handler
    [ P0 ++ ] = R0;

LD32_LABEL(r0, I15HANDLE);// IVG15 Handler
    [ P0 ++ ] = R0;

LD32(p0, EVT_OVERRIDE);
    R0 = 0;
    [ P0 ++ ] = R0;
    R0 = -1;     // Change this to mask interrupts (*)
    [ P0 ] = R0;   // IMASK

DUMMY:

    R0 = 0 (Z);

LT0 = r0;       // set loop counters to something deterministic
LB0 = r0;
LC0 = r0;
LT1 = r0;
LB1 = r0;
LC1 = r0;

ASTAT = r0;     // reset other internal regs
    R0 = 1;
SYSCFG = r0;    // Enable Supervisor Single Step
    R4 = 0;

LD32_LABEL(r0, START);
RETI = r0;      // We need to load the return address

RTI;


START:
LD32_LABEL(sp, USTACK);   // setup the stack pointer
FP = SP;            // set frame pointer
JUMP BEGIN;

//*********************************************************************

BEGIN:

    R0 = 0;
    R1 = 1;
    R2 = 2;
    R3 = 3;
    R5 = 5;
    R6 = 6;
    R7 = 7;

EXCPT 3;            // turn off single step via handler

CHECKREG(r4, 0x0b); // 11 instrs are executed before single step = disabled
CHECKREG(r0, 0x00);
CHECKREG(r1, 0x03);
CHECKREG(r2, 0x10);
CHECKREG(r3, 0x04);
CHECKREG(r5, 0x09);
CHECKREG(r6, 0x06);
CHECKREG(r7, 0x07);


    // PUT YOUR TEST HERE!


END:
dbg_pass;            // End the test

//*********************************************************************

//
// Handlers for Events
//

EHANDLE:            // Emulation Handler 0
RTE;

RHANDLE:            // Reset Handler 1
RTI;

NHANDLE:            // NMI Handler 2
RTN;

XHANDLE:            // Exception Handler 3
    [ -- SP ] = ASTAT; // save ASTAT
    R1 = SEQSTAT;
    R1 <<= 26;
    R1 >>= 26;      // only want EXCAUSE
    R2 = 0x10;      // EXCAUSE 0x10 means Single Step (exception)
CC = r1 == r2;
IF CC JUMP SSCOUNT;  // Go to Single Step Handler

SYSCFG = r0;      // otherwise must be an EXCPT, so turn off singlestep
    R3 += 1;

JUMP.S EXIT;

SSCOUNT:
    R4 += 1;        // R4 counts single step events

EXIT:
ASTAT = [sp++];
    R5 += 1;

RTX;

HWHANDLE:           // HW Error Handler 5
RTI;

THANDLE:            // Timer Handler 6
RTI;

I7HANDLE:           // IVG 7 Handler
RTI;

I8HANDLE:           // IVG 8 Handler
RTI;

I9HANDLE:           // IVG 9 Handler
RTI;

I10HANDLE:          // IVG 10 Handler
RTI;

I11HANDLE:          // IVG 11 Handler
RTI;

I12HANDLE:          // IVG 12 Handler
RTI;

I13HANDLE:          // IVG 13 Handler
RTI;

I14HANDLE:          // IVG 14 Handler
RTI;

I15HANDLE:          // IVG 15 Handler
RTI;

NOP;NOP;NOP;NOP;NOP;NOP;NOP; // needed for icache bug

//
// Data Segment
//

.data
DATA:
    .space (0x10);

// Stack Segments (Both Kernel and User)

    .space (STACKSIZE);
KSTACK:

    .space (STACKSIZE);
USTACK:
