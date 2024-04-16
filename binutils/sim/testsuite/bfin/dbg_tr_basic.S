//Original:/proj/frio/dv/testcases/debug/dbg_tr_basic/dbg_tr_basic.dsp
// Description: Verify the basic functionality of TBUFPWR and TBUFEN in
//              Supervisor mode
# mach: bfin
# sim: --environment operating

#include "test.h"
.include "testutils.inc"
start

include(std.inc)
include(mmrs.inc)
include(selfcheck.inc)

#ifndef ITABLE
#define ITABLE           0xF0000000
#endif

// This test embeds .text offsets, so pad our test so it lines up.
.space 0x70

// Boot code

 BOOT :
INIT_R_REGS(0);                             // Initialize Dregs
INIT_P_REGS(0);                             // Initialize Pregs

CHECK_INIT(p5,   0x00BFFFFC);

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
WR_MMR(TBUFCTL, 0x00000000, p0, r0);        // Turn ON trace Buffer
                                                    //   TBUFPWR   = 0
                                                    //   TBUFEN    = 0
                                                    //   TBUFOVF   = 0
                                                    //   CMPLP     = 0
NOP;
NOP;
NOP;
NOP;
NOP;
	    NOP;
NOP;
JUMP.S label1;								//
        R4.L = 0x1111;                             // Will be killed
        R4.H = 0x1111;                             // Will be killed
NOP;
NOP;
NOP;
label2: R5.H = 0x7777;		//
        R5.L = 0x7888;
JUMP.S label3;		//
        R6.L = 0x1111;                             // Will be killed
        R6.H = 0x1111;                             // Will be killed
NOP;
NOP;
NOP;
NOP;
NOP;
label1: R4.H = 0x5555;		//
        R4.L = 0x6666;
NOP;
WR_MMR(TBUFCTL, 0x00000002, p0, r0);        //
                                                    //   TBUFPWR   = 0
                                                    //   TBUFEN    = 1
                                                    //   TBUFOVF   = 0
                                                    //   CMPLP     = 0
NOP;
NOP;
NOP;
NOP;
JUMP.S label2; 		//
        R5.L = 0x1111;     // Will be killed
        R5.H = 0x1111;     // Will be killed
NOP;
NOP;
NOP;
NOP;
label3: R6.H = 0x7999;		//
        R6.L = 0x7aaa;
NOP;
NOP;
WR_MMR(TBUFCTL, 0x00000001, p0, r0);
		NOP;
		NOP;
		NOP;
WR_MMR(TBUFCTL, 0x00000003, p0, r0);        // Turn ON trace Buffer
                                                    //   TBUFPWR   = 1
                                                    //   TBUFEN    = 1
                                                    //   TBUFOVF   = 0
                                                    //   CMPLP     = 0
NOP;
NOP;
NOP;
NOP;
JUMP.S label4; 		//
        R5.L = 0x1111;     // Will be killed
        R5.H = 0x1111;     // Will be killed
NOP;
NOP;
NOP;
NOP;

label4: R6.H = 0x1aaa;		//
        R6.L = 0x2222;
NOP;
NOP;
NOP;
NOP;

WR_MMR(TBUFCTL, 0x00000001, p0, r0);        // Turn OFF trace Buffer

NOP;
NOP;
NOP;
NOP;
        // Read the contents of the Trace Buffer

RD_MMR(TBUFSTAT, p0, r2);
CHECKREG(r2,    0x00000001);

        // Read 3rd Entry of the Trace Buffer
RD_MMR(TBUF,    p0,     r0);
CHECKREG(r0,    0x000002d2);

RD_MMR(TBUFSTAT, p0, r2);
CHECKREG(r2,    0x00000001);

RD_MMR(TBUF,    p0,     r1);
CHECKREG(r1,    0x000002c0);

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
