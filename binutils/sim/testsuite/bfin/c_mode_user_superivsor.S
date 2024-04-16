//Original:/proj/frio/dv/testcases/core/c_mode_user_superivsor/c_mode_user_superivsor.dsp
// Spec Reference: mode_user_supervisor
# mach: bfin
# sim: --environment operating

#include "test.h"
.include "testutils.inc"
start

include(std.inc)
include(selfcheck.inc)
include(gen_int.inc)
INIT_R_REGS(0);
INIT_P_REGS(0);
INIT_I_REGS(0);     // initialize the dsp address regs
INIT_M_REGS(0);
INIT_L_REGS(0);
INIT_B_REGS(0);
//CHECK_INIT(p5, 0xe0000000);
include(symtable.inc)
CHECK_INIT_DEF(p5);

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

                              // in reset mode now
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
    [ P0 ] = P1;   // IVG15 (General) handler (Int 15) load with start

RAISE 15;   // after we RTI, INT 15 should be taken,& return to BEGIN in
                // USER MODE & go to different RAISE in USER mode
                // until the end of the test.

NOP;    // Workaround for Bug 217
RTI;

//
// The Main Program
//
STARTUSER:
LD32_LABEL(sp, USTACK);   // setup the stack pointer
FP = SP;            // set frame pointer
JUMP BEGIN;

//*********************************************************************

BEGIN:

                // COMMENT the following line for USER MODE tests
    [ -- SP ] = RETI;  // enable interrupts in supervisor mode

                // **** YOUR CODE GOES HERE ****



    // PUT YOUR TEST HERE!
                // Can't Raise 0, 3, or 4
RAISE 2;    // RTN
RAISE 5;    // RTI
RAISE 6;    // RTI
RAISE 7;    // RTI
RAISE 8;    // RTI
RAISE 9;    // RTI
RAISE 10;   // RTI
RAISE 11;   // RTI
RAISE 12;   // RTI
RAISE 13;   // RTI
RAISE 14;   // RTI

R0 = I0;
R1 = I1;
R2 = I2;
R3 = I3;
R4 = M0;
R5 = M1;
R6 = M2;
R7 = M3;

CHECKREG(r0, 0x00000018);
CHECKREG(r1, 0x00000018);
CHECKREG(r2, 0x00000000);
CHECKREG(r3, 0x00000018);
CHECKREG(r4, 0x00000000);
CHECKREG(r5, 0x00000000);
CHECKREG(r6, 0x00000000);
CHECKREG(r7, 0x00000000);


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
    I0 += 2;
    I1 += 2;
    I2 += 2;
    I3 += 2;
    R0 += 2;
RETN = r0;
RTN;

XHANDLE:            // Exception Handler 3
    R0 = RETX;
    I0 += 2;
    I1 += 2;
    I3 += 2;
    R0 += 2;
RETX = r0;
RTX;

HWHANDLE:           // HW Error Handler 5
    R0 = RETI;
    I0 += 2;
    I1 += 2;
    I2 += 2;
    R0 += 2;
RETI = r0;
RTI;

THANDLE:            // Timer Handler 6
    R0 = RETI;
    R0 += 2;
RETI = r0;
RTI;

I7HANDLE:           // IVG 7 Handler
    R0 = RETI;
    I0 += 2;
    I1 += 2;
    I3 += 2;
    R0 += 2;
RETI = r0;
RTI;

I8HANDLE:           // IVG 8 Handler
    R0 = RETI;
    I0 += 2;
    I1 += 2;
    I2 += 2;
    I3 += 2;
    M0 = I0;
    M1 = I1;
    M2 = I2;
    M3 = I3;
    R0 += 2;
RETI = r0;
RTI;

I9HANDLE:           // IVG 9 Handler
    R0 = RETI;
    I0 += 2;
    I1 += 2;
    I2 += 2;
    I3 += 2;
    R0 += 2;
RETI = r0;
RTI;

I10HANDLE:          // IVG 10 Handler
    R0 = RETI;
    I0 += 2;
    I1 += 2;
    I2 += 2;
    I3 += 2;
    R0 += 2;
RETI = r0;
RTI;

I11HANDLE:          // IVG 11 Handler
    I0 = R0;
    I1 = R1;
    I2 = R2;
    M0 = R4;
    R0 = RETI;
    I0 += 2;
    I1 += 2;
    I2 += 2;
    I3 += 2;
    R0 += 2;
RETI = r0;
RTI;

I12HANDLE:          // IVG 12 Handler
    R0 = RETI;
    I0 += 2;
    I1 += 2;
    I2 += 2;
    I3 += 2;
    R0 += 2;
RETI = r0;
RTI;

I13HANDLE:          // IVG 13 Handler
    R0 = RETI;
    I0 += 2;
    I1 += 2;
    I2 += 2;
    I3 += 2;
    R0 += 2;
RETI = r0;
RTI;

I14HANDLE:          // IVG 14 Handler
    R0 = RETI;
    I1 += 2;
    I2 += 2;
    I3 += 2;
    R0 += 2;
RETI = r0;
RTI;

I15HANDLE:          // IVG 15 Handler
    R4 = 15;
    I1 += 2;
    I2 += 2;
RTI;

//  nop;nop;nop;nop;nop;nop;nop; // needed for icache bug

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
