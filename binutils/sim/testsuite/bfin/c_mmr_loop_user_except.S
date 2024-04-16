//Original:/proj/frio/dv/testcases/core/c_mmr_loop_user_except/c_mmr_loop_user_except.dsp
// Spec Reference: c_mmr_loop_user_except
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
#ifndef ITABLE
#define ITABLE 0xF0000000
#endif

GEN_INT_INIT(ITABLE) // set location for interrupt table

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

//  LD32_LABEL(r0, I13HANDLE);// IVG13 Handler
//  [p0++] = r0;

//  LD32_LABEL(r0, I14HANDLE);// IVG14 Handler
//  [p0++] = r0;

//*****************
    // wrt-rd     EVT13             = 0xFFE02034
LD32(p0, 0xFFE02034);
LD32(r0, 0xDDDDABC6);
    [ P0 ] = R0;

    // wrt-rd     EVT14             = 0xFFE02038
LD32(p0, 0xFFE02038);
LD32(r0, 0xEEEEABC6);
    [ P0 ] = R0;
//*****************
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

//  JUMP    STARTSUP;   // jump to code start for SUPERVISOR mode

RTI;            // execute this instr put us in USER mode

STARTSUP:
LD32_LABEL(p1, BEGIN);

LD32(p0, EVT15);

CLI R1;    // inhibit events during write to MMR
    [ P0 ] = P1;   // IVG15 (General) handler (Int 15) load with start
CSYNC;       // wait for it
STI R1;      // reenable events with proper imask

RAISE 15;   // after we RTI, INT 15 should be taken,& return to BEGIN in
                // USER MODE & go to different RAISE in USER mode
                // until the end of the test.

RTI;

//
// The Main Program
//
STARTUSER:
LD32_LABEL(sp, USTACK);   // setup the stack pointer
FP = SP;            // set frame pointer
//  LINK 0;     // change for how much stack frame space you need.

JUMP BEGIN;

//*********************************************************************

BEGIN:

                // COMMENT the following line for USER MODE tests
    [ -- SP ] = RETI;  // enable interrupts in supervisor mode

                // **** YOUR CODE GOES HERE ****



    // PUT YOUR TEST HERE!
                // Can't Raise 0, 3, or 4
                // Raise 1 requires some intelligence so the test
                //  doesn't loop forever - use SFTRESET bit in SEQSTAT (TBD)
//  RAISE 2;    // RTN   // exception because we execute this in USER mode
    R0 = 0;
LD32(p0, 0xFFE02034);
    P2 = 2;
LSETUP ( start1 , end1 ) LC0 = P2;
start1:
      R0 = [ P0 ++ ];   // 16 bit instr
end1: R1 = R0;

CHECKREG(r0, 0x00000000);
CHECKREG(r1, 0x00000000);
CHECKREG(r2, 0x00000000);
//CHECKREG(r3, 0x00000030);
CHECKREG(r4, 0x0000000F);
CHECKREG(r5, 0x00000012);
CHECKREG(r6, 0x00000015);
CHECKREG(r7, 0x00000018);


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
    R0 = RETN;
    R0 += 2;
RETN = r0;
RTN;

XHANDLE:            // Exception Handler 3
    R3 = RETX;
    R4 += 5;
    R5 += 6;
    R6 += 7;
    R7 += 8;
    R3 += 2;    // for resturn address
RETX = r3;
RTX;

HWHANDLE:           // HW Error Handler 5
    R2 = RETI;
    R2 += 2;
RETI = r2;
RTI;

THANDLE:            // Timer Handler 6
    R3 = RETI;
    R3 += 2;
RETI = r3;
RTI;

I7HANDLE:           // IVG 7 Handler
    R4 = RETI;
    R4 += 2;
RETI = r4;
RTI;

I8HANDLE:           // IVG 8 Handler
    R5 = RETI;
    R5 += 2;
RETI = r5;
RTI;

I9HANDLE:           // IVG 9 Handler
    R6 = RETI;
    R6 += 2;
RETI = r6;
RTI;

I10HANDLE:          // IVG 10 Handler
    R7 = RETI;
    R7 += 2;
RETI = r7;
RTI;

I11HANDLE:          // IVG 11 Handler
    I0 = R0;
    I1 = R1;
    I2 = R2;
    I3 = R3;
    M0 = R4;
    R0 = RETI;
    R0 += 2;
RETI = r0;
RTI;

I12HANDLE:          // IVG 12 Handler
    R1 = RETI;
    R1 += 2;
RETI = r1;
RTI;

I13HANDLE:          // IVG 13 Handler
    R2 = RETI;
    R2 += 2;
RETI = r2;
RTI;

I14HANDLE:          // IVG 14 Handler
    R3 = RETI;
    R3 += 2;
RETI = r3;
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
//  .space (STACKSIZE);  // adding this may solve the problem
