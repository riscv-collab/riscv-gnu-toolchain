//Original:/proj/frio/dv/testcases/core/c_seq_wb_rti_lsmmrj_mvp/c_seq_wb_rti_lsmmrj_mvp.dsp
// Spec Reference: sequencer:wb ( rti ldst mmr jump regmv pushpopmultiple)
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
#ifndef ITABLE
#define ITABLE DATA_ADDR_1
#endif

GEN_INT_INIT(ITABLE) // set location for interrupt table

//
// Reset/Bootstrap Code
//   (Here we should set the processor operating modes, initialize registers,
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
CSYNC;

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

RAISE 15;   // after we RTI, INT 15 should be taken,& return to BEGIN in
                // SUPERVISOR MODE & go to different RAISE in supervisor mode
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
// PUSH
LD32(p1, 0xFFE02034);       // wrt-rd     EVT13             = 0xFFE02034
//LD32(p2, DATA_ADDR_1);
loadsym P2, DATA;
LD32(p3, 0xab5fd490);
LD32(p4, 0xa581bd94);

LD32(r2, 0x14789232);
    [ P1 ] = R2;
        R0 = 0x01;
        R1 = 0x02;
        R2 = 0x03;
        R3 = 0x04;
        R4 = 0x05;
        R5 = 0x06;
        R6 = 0x07;
        R7 = 0x08;
        [ -- SP ] = ( R7:0 );
RAISE 2;    // RTN
    R0 = [ P2 ++ ];
    R1 = [ P1 ];
JUMP.S LABEL1;
    P3 = R7;
    R4 = P3;
        [ -- SP ] = ( R7:0 );
        R1 = 0x12;
        R2 = 0x13;
        R3 = 0x14;
        R4 = 0x15;
        R5 = 0x16;
        R6 = 0x17;
        R7 = 0x18;

LABEL1:
RAISE 5;    // RTI
    R2 = [ P2 ++ ];

    P4 = R6;
    R3 = P4;

        [ -- SP ] = ( R7:0 );

        R2 = 0x23;
        R3 = 0x24;
        R4 = 0x25;
        R5 = 0x26;
        R6 = 0x27;
        R7 = 0x28;

// wrt-rd     EVT5              = 0xFFE02034
LD32(p1, 0xFFE02034);       // wrt-rd     EVT13             = 0xFFE02034
RAISE 6;    // RTI
    R4 = [ P2 ++ ];
    R6 = [ P1 ];
JUMP.S LABEL2;
    P3 = R3;
    R5 = P3;
        [ -- SP ] = ( R7:0 );
// POP
        R0 = 0x00;
        R1 = 0x00;
        R2 = 0x00;
        R3 = 0x00;
        R4 = 0x00;
        R5 = 0x00;
        R6 = 0x00;
        R7 = 0x00;

LABEL2:
CSYNC;
CHECKREG(r0, 0x00010203);
CHECKREG(r1, 0x14789232);
CHECKREG(r2, 0x00000023);
CHECKREG(r3, 0x00000024);
CHECKREG(r4, 0x08090A0B);
CHECKREG(r5, 0x00000026);
CHECKREG(r6, 0x14789232);
RAISE 7;    // RTI
    R0 = [ P2 ++ ];
    R1 = [ P1 ];
    P4 = R4;
    R2 = P4;
        ( R7:0 ) = [ SP ++ ];



CHECKREG(r0, 0x00010203);
CHECKREG(r1, 0x14789232);
CHECKREG(r2, 0x04050607);
CHECKREG(r3, 0x00000007);
CHECKREG(r4, 0x00000005);
CHECKREG(r5, 0x00000006);
CHECKREG(r6, 0x00000007);
CHECKREG(r7, 0x00000008);
// wrt-rd     EVT13             = 0xFFE02034
LD32(p1, 0xFFE02034);
RAISE 8;    // RTI
    R0 = [ P2 ++ ];
    R1 = [ P1 ];
JUMP.S LABEL3;
    P1 = R5;
    R6 = P1;
        ( R7:0 ) = [ SP ++ ];
//CHECKREG(r0, 0x000000a1);  // CHECKREG can not be skipped
//CHECKREG(r1, 0x000000b2);  // so they cannot appear here
//CHECKREG(r2, 0x000000c3);
//CHECKREG(r3, 0x000000d4);
//CHECKREG(r4, 0x000000e5);
//CHECKREG(r5, 0x000000f6);
//CHECKREG(r6, 0x00000017);
//CHECKREG(r7, 0x00000028);
   R0 = 12;
   R1 = 13;
   R2 = 14;
   R3 = 15;
   R4 = 16;
   R5 = 17;
   R6 = 18;
   R7 = 19;


LABEL3:
CSYNC;
CHECKREG(r0, 0x10111213);
CHECKREG(r1, 0x14789232);
RAISE 9;    // RTI
    P3 = R6;
    R7 = P3;
        ( R7:0 ) = [ SP ++ ];

CHECKREG(r0, 0x00000001);
CHECKREG(r1, 0x00000002);
CHECKREG(r2, 0x00000003);
CHECKREG(r3, 0x00000004);
CHECKREG(r4, 0x00000005);
CHECKREG(r5, 0x00000006);
CHECKREG(r6, 0x00000007);
CHECKREG(r7, 0x00000008);
R0 = I0;
R1 = I1;
R2 = I2;
R3 = I3;
CHECKREG(r0, 0x00000006);
CHECKREG(r1, 0x00000002);
CHECKREG(r2, 0x00000002);
CHECKREG(r3, 0x00000002);


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
    I0 += 2;
RTN;

XHANDLE:            // Exception Handler 3
    R1 = 3;
RTX;

HWHANDLE:           // HW Error Handler 5
    I1 += 2;
RTI;

THANDLE:            // Timer Handler 6
    I2 += 2;
RTI;

I7HANDLE:           // IVG 7 Handler
    I3 += 2;
RTI;
    //  ***********
    R0 = [ P2 ++ ];
    R1 = [ P1 ];
JUMP.S LABEL3;
    P1 = R5;
    R6 = P1;
        ( R7:0 ) = [ SP ++ ];
    //  ***********
I8HANDLE:           // IVG 8 Handler
    I0 += 2;
RTI;

I9HANDLE:           // IVG 9 Handler
    I0 += 2;
RTI;
    //  ***********
    R0 = [ P2 ++ ];
    R1 = [ P1 ];
JUMP.S LABEL3;
    P1 = R5;
    R6 = P1;
        ( R7:0 ) = [ SP ++ ];
    //  ***********

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

.section MEM_DATA_ADDR_1,"aw"
DATA:
//  .space (0x10);
.dd 0x00010203
.dd 0x04050607
.dd 0x08090A0B
.dd 0x0C0D0E0F
.dd 0x10111213
.dd 0x14151617
.dd 0x18191A1B
.dd 0x1C1D1E1F
.dd 0x11223344
.dd 0x55667788
.dd 0x99717273
.dd 0x74757677
.dd 0x82838485
.dd 0x86878889
.dd 0x80818283
.dd 0x84858687
.dd 0x01020304
.dd 0x05060708
.dd 0x09101112
.dd 0x14151617
.dd 0x18192021


// Stack Segments (Both Kernel and User)

    .space (STACKSIZE);
KSTACK:

    .space (STACKSIZE);
USTACK:

.section MEM_DATA_ADDR_2,"aw"
.dd 0x20212223
.dd 0x24252627
.dd 0x28292A2B
.dd 0x2C2D2E2F
.dd 0x30313233
.dd 0x34353637
.dd 0x38393A3B
.dd 0x3C3D3E3F
.dd 0x91929394
.dd 0x95969798
.dd 0x99A1A2A3
.dd 0xA5A6A7A8
.dd 0xA9B0B1B2
.dd 0xB3B4B5B6
.dd 0xB7B8B9C0
