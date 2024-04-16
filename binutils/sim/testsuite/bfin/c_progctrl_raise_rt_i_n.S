//Original:/proj/frio/dv/testcases/core/c_progctrl_raise_rt_i_n/c_progctrl_raise_rt_i_n.dsp
// Spec Reference: progctrl raise rti rtn
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
CHECK_INIT(p5, 0xe0000000);

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
#ifndef ITABLE
#define ITABLE 0xF0000000
#endif

GEN_INT_INIT(ITABLE) // set location for interrupt table

//
// Reset/Bootstrap Code
//   (Here we should set the processor operating modes, initialize registers,
//    etc.)
//

BOOT:


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

JUMP    STARTSUP;   // jump to code start for SUPERVISOR mode

RTI;

STARTSUP:
LD32_LABEL(p1, BEGIN);

LD32(p0, EVT15);
    [ P0 ] = P1;   // IVG15 (General) handler (Int 15) load with start

RAISE 15;       // after we RTI, INT 15 should be taken

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
                // Raise 1 requires some intelligence so the test
                //  doesn't loop forever - use SFTRESET bit in SEQSTAT (TBD)
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
RAISE 15;   // RTI

CHECKREG(r0, 0x0000000B);
CHECKREG(r1, 0x0000000C);
CHECKREG(r2, 0x0000000D);
CHECKREG(r3, 0x0000000E);
CHECKREG(r4, 0x00000007);
CHECKREG(r5, 0x00000008);
CHECKREG(r6, 0x00000009);
CHECKREG(r7, 0x0000000A);
R0 = I0;
R1 = I1;
R2 = I2;
R3 = I3;
R4 = M0;
CHECKREG(r0, 0x00000002);
CHECKREG(r1, 0x00000000);
CHECKREG(r2, 0x00000005);
CHECKREG(r3, 0x00000006);
CHECKREG(r4, 0x00000007);


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
    R0 = 2;
RTN;

XHANDLE:            // Exception Handler 3
    R1 = 3;
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
    I0 = R0;
    I1 = R1;
    I2 = R2;
    I3 = R3;
    M0 = R4;
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
