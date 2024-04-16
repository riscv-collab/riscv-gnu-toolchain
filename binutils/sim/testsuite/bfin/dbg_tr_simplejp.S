//Original:/proj/frio/dv/testcases/debug/dbg_tr_simplejp/dbg_tr_simplejp.dsp
// Description: This test performs simple jumps and verifies the trace buffer
//                          recording for simple jumps.
# mach: bfin
# sim: --environment operating

#include "test.h"
.include "testutils.inc"
start

include(std.inc)
include(mmrs.inc)
include(selfcheck.inc)
include(symtable.inc)

#ifndef ITABLE
#define ITABLE  CODE_ADDR_1   //
#endif

// This test embeds .text offsets, so pad our test so it lines up.
.space 0x5e

// Boot code

 BOOT :
INIT_R_REGS(0);                             // Initialize Dregs
INIT_P_REGS(0);                             // Initialize Pregs

CHECK_INIT_DEF(p5); // CHECK_INIT(p5,   0x00BFFFFC);

LD32(p0, EVT0);              // Setup Event Vectors and Handlers

LD32_LABEL(r0, EHANDLE);    // Emulation Handler (Int0)
        [ P0 ++ ] = R0;

LD32_LABEL(r0, RHANDLE);    // Reset Handler (Int1)
        [ P0 ++ ] = R0;

LD32_LABEL(r0, NHANDLE);    // NMI Handler (Int2)
        [ P0 ++ ] = R0;

LD32_LABEL(r0, XHANDLE);    // Exception Handler (Int3)
        [ P0 ++ ] = R0;

        [ P0 ++ ] = R0;                // IVT4 not used

LD32_LABEL(r0, HWHANDLE);   // HW Error Handler (Int5)
        [ P0 ++ ] = R0;

LD32_LABEL(r0, THANDLE);    // Timer Handler (Int6)
        [ P0 ++ ] = R0;

LD32_LABEL(r0, I7HANDLE);   // IVG7 Handler
        [ P0 ++ ] = R0;

LD32_LABEL(r0, I8HANDLE);   // IVG8 Handler
        [ P0 ++ ] = R0;

LD32_LABEL(r0, I9HANDLE);   // IVG9 Handler
        [ P0 ++ ] = R0;

LD32_LABEL(r0, I10HANDLE);  // IVG10 Handler
        [ P0 ++ ] = R0;

LD32_LABEL(r0, I11HANDLE);  // IVG11 Handler
        [ P0 ++ ] = R0;

LD32_LABEL(r0, I12HANDLE);  // IVG12 Handler
        [ P0 ++ ] = R0;

LD32_LABEL(r0, I13HANDLE);  // IVG13 Handler
        [ P0 ++ ] = R0;

LD32_LABEL(r0, I14HANDLE);  // IVG14 Handler
        [ P0 ++ ] = R0;

LD32_LABEL(r0, I15HANDLE);  // IVG15 Handler
        [ P0 ++ ] = R0;

LD32(p0, EVT_OVERRIDE);
        R0 = 0;
        [ P0 ++ ] = R0;
        R0 = -1;     // Change this to mask interrupts (*)
        [ P0 ] = R0;   // IMASK

LD32_LABEL(p1, START);

LD32(p0, EVT15);
        [ P0 ] = P1;   // IVG15 (General) handler (Int 15) load with start

LD32_LABEL(r7, DUMMY);
RETI = r7;
RAISE 15;    // after we RTI, INT 15 should be taken

NOP;        // Workaround for Bug 217
RTI;
NOP;
NOP;
NOP;
DUMMY:
	  NOP;
NOP;
NOP;
NOP;



 START :
WR_MMR(TBUFCTL, 0x00000003, p0, r0);        // Turn ON trace Buffer
                                                    //   TBUFPWR   = 1
                                                    //   TBUFEN    = 1
                                                    //   TBUFOVF   = 0
                                                    //   CMPLP     = 0
NOP;
NOP;
NOP;
NOP;
NOP;
NOP;
NOP;
NOP;
NOP;
JUMP.S label1;                                                            // 0x0224
        R4.L = 0x1111;                             // Will be killed
        R4.H = 0x1111;                             // Will be killed
NOP;
NOP;
NOP;
label2: R5.H = 0x7777;           // 0x0234
        R5.L = 0x7888;
JUMP.S label3;            //0x023c
        R6.L = 0x1111;                             // Will be killed
        R6.H = 0x1111;                             // Will be killed
NOP;
NOP;
NOP;
NOP;
NOP;
label1: R4.H = 0x5555;           // 0x0250
        R4.L = 0x6666;
NOP;
JUMP.S label2;            // 0x0258
        R5.L = 0x1111;     // Will be killed
        R5.H = 0x1111;     // Will be killed
NOP;
NOP;
NOP;
NOP;
label3: R6.H = 0x7999;           //0x026c
        R6.L = 0x7aaa;
NOP;
NOP;
NOP;
NOP;

WR_MMR(TBUFCTL, 0x00000001, p0, r0);        // Turn OFF trace Buffer

NOP;
NOP;
NOP;
NOP;
NOP;
        // Read the contents of the Trace Buffer

RD_MMR(TBUFSTAT, p0, r2);
CHECKREG(r2,    0x00000003);

        // Read 3rd Entry of the Trace Buffer
RD_MMR(TBUF,    p0,     r0);
CHECKREG(r0,    0x0000026c);

RD_MMR(TBUFSTAT, p0, r2);
CHECKREG(r2,    0x00000003);

RD_MMR(TBUF,    p0,     r1);
CHECKREG(r1,    0x0000023c);

RD_MMR(TBUFSTAT, p0, r2);
CHECKREG(r2,    0x00000002);

        // Read 2nd Entry of the Trace Buffer
RD_MMR(TBUF,    p0,     r0);
CHECKREG(r0,    0x00000234);

RD_MMR(TBUFSTAT, p0, r2);
CHECKREG(r2,    0x00000002);

RD_MMR(TBUF,    p0,     r1);
CHECKREG(r1,    0x0000025a);

RD_MMR(TBUFSTAT, p0, r2);
CHECKREG(r2,    0x00000001);

        // Read ist Entry of the Trace Buffer
RD_MMR(TBUF,    p0,     r0);
CHECKREG(r0,    0x00000250);

RD_MMR(TBUFSTAT, p0, r2);
CHECKREG(r2,    0x00000001);

RD_MMR(TBUF,    p0,     r1);
CHECKREG(r1,    0x00000224);

RD_MMR(TBUFSTAT, p0, r2);
CHECKREG(r2,    0x00000000);

WR_MMR(TBUFCTL, 0x00000000, p0, r0);        // Turn OFF trace Buffer Power

NOP;
NOP;
NOP;
NOP;
NOP;
NOP;
dbg_pass;        // Call Endtest Macro



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
