//Original:/proj/frio/dv/testcases/core/c_mmr_timer/c_mmr_timer.dsp
// Spec Reference: mmr timer
# mach: bfin
# sim: --environment operating

#include "test.h"
.include "testutils.inc"
start

include(gen_int.inc)
include(selfcheck.inc)
include(std.inc)
include(mmrs.inc)

#ifndef STACKSIZE
#define STACKSIZE 0x10
#endif
//

////MY_GEN_INT_INIT(0xF0000000) // set location for interrupt table

//
// Reset/Bootstrap Code
//   (Here we set the processor operating modes, initialize registers
//    etc.)
//

BOOT:

INIT_R_REGS(0);
INIT_P_REGS(0);
INIT_I_REGS(0);     // initialize the dsp address regs
INIT_M_REGS(0);
INIT_L_REGS(0);
INIT_B_REGS(0);
    //CHECK_INIT(p5, 0xe0000000);
include(symtable.inc)
CHECK_INIT_DEF(p5);

CLI R1;           // inhibit events during MMR writes

LD32_LABEL(sp, USTACK);   // setup the user stack pointer
USP = SP;                  // and frame pointer

LD32_LABEL(sp, KSTACK);   // setup the stack pointer
FP = SP;                  // and frame pointer

LD32(p0, EVT0);      // Setup Event Vectors and Handlers
LD32_LABEL(r0, EHANDLE);  // Emulation Handler (Int0)
    [ P0 ++ ] = R0;

LD32_LABEL(r0, RHANDLE);  // Reset Handler (Int1)
    [ P0 ++ ] = R0;

LD32_LABEL(r0, NHANDLE);  // NMI Handler (Int2)
    [ P0 ++ ] = R0;

LD32_LABEL(r0, XHANDLE);  // Exception Handler (Int3)
    [ P0 ++ ] = R0;

    [ P0 ++ ] = R0;          // EVT4 not used global Interr Enable (INT4)

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

    R1 = -1;     // Change this to mask interrupts (*)
CSYNC;       // wait for MMR writes to finish
STI R1;      // sync and reenable events (implicit write to IMASK)

DUMMY:

    R0 = 0 (Z);

LT0 = r0;       // set loop counters to something deterministic
LB0 = r0;
LC0 = r0;
LT1 = r0;
LB1 = r0;
LC1 = r0;

ASTAT = r0;     // reset other internal regs
SYSCFG = r0;
RETS = r0;      // prevent X's breaking LINK instruction

// The following code sets up the test for running in USER mode

LD32_LABEL(r0, STARTUSER);// One gets to user mode by doing a
                        // ReturnFromInterrupt (RTI)
RETI = r0;      // We need to load the return address

// Comment the following line for a USER Mode test

JUMP    STARTSUP;   // jump to code start for SUPERVISOR mode

RTI;

STARTSUP:
LD32_LABEL(p1, BEGIN);

LD32(p0, EVT15);

CLI R1;    // inhibit events during write to MMR
    [ P0 ] = P1;   // IVG15 (General) handler (Int 15) load with start
CSYNC;       // wait for it
STI R1;      // reenable events with proper imask

RAISE 15;       // after we RTI, INT 15 should be taken

RTI;

//
// The Main Program
//
STARTUSER:
LINK 0;     // change for how much stack frame space you need.

JUMP BEGIN;


//*********************************************************************

BEGIN:

                // COMMENT the following line for USER MODE tests
    [ -- SP ] = RETI;  // enable interrupts in supervisor mode

                // **** YOUR CODE GOES HERE ****

    // TCNTL: 4 bits, rw=1          = 0xFFE03000
LD32(p0, 0xFFE03000);
LD32(r0, 0x0000000D);
    [ P0 ] = R0;
CSYNC;                  // without this it read out zero
    R1 = [ P0 ];

    // TPERIOD: 32 bits, rw=1       = 0xFFE03004
LD32(p0, 0xFFE03004);
LD32(r0, 0x11112222);
    [ P0 ] = R0;
CSYNC;                  // without this it read out zero
    R2 = [ P0 ];

    // TSCALE: 8 bits, rw=1         = 0xFFE03008
LD32(p0, 0xFFE03008);
LD32(r0, 0x00000050);
    [ P0 ] = R0;
CSYNC;                  // without this it read out zero
    R3 = [ P0 ];


    //  TCOUNT: 32 bits, rw=1       = 0xFFE0300C
LD32(p0, 0xFFE0300C);
LD32(r0, 0x00000100);
    [ P0 ] = R0;
CSYNC;                  // without this it read out zero
    R4 = [ P0 ];


CHECKREG(r1, 0x0000000D);
CHECKREG(r2, 0x11112222);
CHECKREG(r3, 0x00000050);
CHECKREG(r4, 0x00000100);

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
    R0 = 2;
RTN;

XHANDLE:            // Exception Handler 3

RTX;

HWHANDLE:           // HW Error Handler 5
    R2 = 5;
RTI;

THANDLE:            // Timer Handler 6
    R3 = 6;
RTI;

I7HANDLE:           // IVG 7 Handler
    R4 = 7;
RTI;

I8HANDLE:           // IVG 8 Handler
    R5 = 8;
RTI;

I9HANDLE:           // IVG 9 Handler
    R6 = 9;
RTI;

I10HANDLE:          // IVG 10 Handler
    R7 = 10;
RTI;

I11HANDLE:          // IVG 11 Handler
    R0 = 11;
RTI;

I12HANDLE:          // IVG 12 Handler
    R1 = 12;
RTI;

I13HANDLE:          // IVG 13 Handler
    R2 = 13;
RTI;

I14HANDLE:          // IVG 14 Handler
    R3 = 14;
RTI;

I15HANDLE:          // IVG 15 Handler
    R4 = 15;
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

//.data 0xFFE03000
//.dd 0x00000000
