//Original:/proj/frio/dv/testcases/core/c_mode_user/c_mode_user.dsp
// Spec Reference: mode_user
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

    A0 = 0;         // reset accumulators
    A1 = 0;

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
A1 = A0 = 0;
ASTAT = R0;


// R-reg to P-reg to R reg: stall
LD32(r0, 0x1357bdad);
LD32(r1, 0x02dfe804);
LD32(r2, 0x12345679);
LD32(r3, 0x34751975);
LD32(r4, 0x08810990);
LD32(r5, 0x01a1b0b0);
LD32(r6, 0x01c1dd00);
LD32(r7, 0x01e1fff0);
R5 = R3.L * R1.L, R4 = R3.L * R1.L;            // dsp32mult_pair
P4 = R5;
R6 = P4;
R1 = ( A1 += R5.L * R6.H ), A0 = R5.H * R6.L;   // dsp32mac_pair
P3 = A0.w;
P4 = A1.w;
A1 = A1 (S), A0 = A0 (S);                               // dsp32alu_sat_aa
R6 = A0.w;
R7 = A1.w;
R0 = R7;
R2 = R0;                          // regmv
R2 >>>= R3;                         // c_alu2op_arith_r_sft.dsp
R4 = R2 - R1;
R5.L = ASHIFT R4.L BY R3.L;
R6 += -3;                                  //c_compi2opd_dr_add_i7_n.dsp
I2 = R6;
I2 += 2;
I2 += M1;
R7 = I2;


CHECKREG(r0, 0x015AF820);
CHECKREG(r2, 0x00000000);
CHECKREG(r3, 0x34751975);
CHECKREG(r4, 0xFEA507E0);
CHECKREG(r5, 0xFB3A0000);
CHECKREG(r6, 0x015AF81D);
CHECKREG(r7, 0x015AF81F);
R0 = I0;
R1 = I1;
R2 = I2;
R3 = I3;
CHECKREG(r0, 0x00000000);
CHECKREG(r1, 0x00000000);
CHECKREG(r2, 0x015AF81F);
CHECKREG(r3, 0x00000000);
CHECKREG(r4, 0xFEA507E0);


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
    R1 = RETX;
    R0 += 1;
    R1 += 2;
    R2 += 1;
    R3 += 1;
    R4 += 1;
    R5 += 1;
    R6 += 1;
    R7 += 1;
RETX = r1;
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
