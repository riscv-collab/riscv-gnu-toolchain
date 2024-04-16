//Original:/proj/frio/dv/testcases/core/c_mmr_loop/c_mmr_loop.dsp
// Spec Reference: mmr loop  (interr control)  no exception
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
// EVTx
    // wrt-rd  EVT0: 0 bits, rw=0   = 0xFFE02000
LD32(p0, 0xFFE02000);
LD32(r0, 0x00000000);
    [ P0 ] = R0;

    // wrt-rd EVT1: 32 bits, rw=0   = 0xFFE02004
LD32(p0, 0xFFE02004);
LD32(r0, 0x00000000);
    [ P0 ] = R0;

    // wrt-rd     EVT2              = 0xFFE02008
LD32(p0, 0xFFE02008);
LD32(r0, 0xE1DE5D1C);
    [ P0 ] = R0;

    // wrt-rd     EVT3              = 0xFFE0200C
LD32(p0, 0xFFE0200C);
LD32(r0, 0x9CC20332);
    [ P0 ] = R0;

    // wrt-rd     EVT4              = 0xFFE02010
LD32(p0, 0xFFE02010);
LD32(r0, 0x00000000);   // not implemented
    [ P0 ] = R0;

    // wrt-rd     EVT5              = 0xFFE02014
LD32(p0, 0xFFE02014);
LD32(r0, 0x55552345);
    [ P0 ] = R0;

    // wrt-rd     EVT6              = 0xFFE02018
LD32(p0, 0xFFE02018);
LD32(r0, 0x66663456);
    [ P0 ] = R0;

    // wrt-rd     EVT7              = 0xFFE0201C
LD32(p0, 0xFFE0201C);
LD32(r0, 0x77774567);
    [ P0 ] = R0;

    // wrt-rd     EVT8              = 0xFFE02020
LD32(p0, 0xFFE02020);
LD32(r0, 0x88885678);
    [ P0 ] = R0;

    // wrt-rd     EVT9              = 0xFFE02024
LD32(p0, 0xFFE02024);
LD32(r0, 0x99996789);
    [ P0 ] = R0;

    // wrt-rd     EVT10             = 0xFFE02028
LD32(p0, 0xFFE02028);
LD32(r0, 0xaaaa1234);
    [ P0 ] = R0;

    // wrt-rd     EVT11             = 0xFFE0202C
LD32(p0, 0xFFE0202C);
LD32(r0, 0xBBBBABC6);
    [ P0 ] = R0;

    // wrt-rd     EVT12             = 0xFFE02030
LD32(p0, 0xFFE02030);
LD32(r0, 0xCCCCABC6);
    [ P0 ] = R0;

    // wrt-rd     EVT13             = 0xFFE02034
LD32(p0, 0xFFE02034);
LD32(r0, 0xDDDDABC6);
    [ P0 ] = R0;

    // wrt-rd     EVT14             = 0xFFE02038
LD32(p0, 0xFFE02038);
LD32(r0, 0xEEEEABC6);
    [ P0 ] = R0;

    // wrt-rd     EVT15             = 0xFFE0203C
LD32(p0, 0xFFE0203C);
LD32(r0, 0xFFFFABC6);
    [ P0 ] = R0;

    // wrt-rd  EVT_OVERRIDE:9 bits  = 0xFFE02100
LD32(p0, 0xFFE02100);
LD32(r0, 0x000001ff);
    [ P0 ] = R0;

    // wrt-rd  IMASK: 16 bits       = 0xFFE02104
LD32(p0, 0xFFE02104);
LD32(r0, 0x00000fe0);
    [ P0 ] = R0;


    // wrt-rd  IPEND: 16 bits, rw=0 = 0xFFE02108
LD32(p0, 0xFFE02108);
LD32(r0, 0x00000000);
  //[p0] = r0;
RAISE 12;
RAISE 13;

    // wrt-rd  ILAT: 16 bits, rw=0  = 0xFFE0210C
LD32(p0, 0xFFE0210C);
LD32(r0, 0x00000000);
  //[p0] = r0;
CSYNC;
//*** read ops
P1.L = DATA0;
P1.H = DATA0;

LD32(p0, 0xFFE02000);
    P2 = 16;
LSETUP ( start1 , end1 ) LC0 = P2;
start1:
      R0 = [ P0 ++ ];
end1: [ P1 ++ ] = R0;
//nop;
P1.L = DATA0;
P1.H = DATA0;
    R0 = [ P1 ++ ];
    R1 = [ P1 ++ ];
    R2 = [ P1 ++ ];
    R3 = [ P1 ++ ];
    R4 = [ P1 ++ ];
    R5 = [ P1 ++ ];
    R6 = [ P1 ++ ];
    R7 = [ P1 ++ ];
CHECKREG(r0, 0x00000000);
CHECKREG(r1, 0x00000000);
CHECKREG(r2, 0xE1DE5D1C);
CHECKREG(r3, 0x9CC20332);
CHECKREG(r4, 0x00000000);
CHECKREG(r5, 0x55552345);
CHECKREG(r6, 0x66663456);
CHECKREG(r7, 0x77774567);
    R0 = [ P1 ++ ];
    R1 = [ P1 ++ ];
    R2 = [ P1 ++ ];
    R3 = [ P1 ++ ];
    R4 = [ P1 ++ ];
    R5 = [ P1 ++ ];
    R6 = [ P1 ++ ];
    R7 = [ P1 ++ ];
CHECKREG(r0, 0x88885678);
CHECKREG(r1, 0x99996789);
CHECKREG(r2, 0xAAAA1234);
CHECKREG(r3, 0xBBBBABC6);
CHECKREG(r4, 0xCCCCABC6);
CHECKREG(r5, 0xDDDDABC6);
CHECKREG(r6, 0xEEEEABC6);
CHECKREG(r7, 0xFFFFABC6);

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
    R7 = 0x00006789 (X);
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

.section MEM_DATA_ADDR_1,"aw"
DATA0:
.dd 0x000a0000
.dd 0x000b0001
.dd 0x000c0002
.dd 0x000d0003
.dd 0x000e0004
.dd 0x000f0005
.dd 0x00100006
.dd 0x00200007
.dd 0x00300008
.dd 0x00400009
.dd 0x0050000a
.dd 0x0060000b
.dd 0x0070000c
.dd 0x0080000d
.dd 0x0090000e
.dd 0x0100000f
.dd 0x02000010
.dd 0x03000011
.dd 0x04000012
.dd 0x05000013
.dd 0x06000014
.dd 0x001a0000
.dd 0x001b0001
.dd 0x001c0002
// Stack Segments (Both Kernel and User)

    .space (STACKSIZE);
KSTACK:

    .space (STACKSIZE);
USTACK:
