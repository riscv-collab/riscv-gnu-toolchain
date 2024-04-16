//Original:/proj/frio/dv/testcases/debug/dbg_tr_umode/dbg_tr_umode.dsp
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
#ifndef STACKSIZE
#define STACKSIZE 0x20
#endif

// This test embeds .text offsets, so pad our test so it lines up.
.space 0x64

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

// .code 0x200
START:
WR_MMR(TBUFCTL, 0x00000001, p0, r0);        // Turn ON trace Buffer
WR_MMR(TBUFCTL, 0x00000003, p0, r0);        // Turn ON trace Buffer
                                                    //   TBUFPWR   = 1
                                                    //   TBUFEN    = 1
                                                    //   TBUFOVF   = 0
                                                    //   CMPLP     = 0
NOP;
NOP;
NOP;
NOP;

// The following code sets up the test for running in USER mode

LD32_LABEL(r0, STARTUSER);// One gets to user mode by doing a
                        // ReturnFromInterrupt (RTI)
RETI = r0;      // We need to load the return address

RTI;

STARTUSER:
LD32_LABEL(sp, USTACK);   // setup the stack pointer
FP = SP;            // set frame pointer
JUMP BEGIN;

//*********************************************************************

BEGIN:


NOP;
	    NOP;
NOP;
JUMP.S label1;
        R4.L = 0x1111;
        R4.H = 0x1111;
NOP;
NOP;
NOP;
label2: R5.H = 0x7777;
        R5.L = 0x7888;
JUMP.S label3;
        R6.L = 0x1111;
        R6.H = 0x1111;
NOP;
NOP;
NOP;
NOP;
NOP;
label1: R4.H = 0x5555;
        R4.L = 0x6666;
NOP;
NOP;
NOP;
NOP;
NOP;
JUMP.S label2;
        R5.L = 0x1111;
        R5.H = 0x1111;
NOP;
NOP;
NOP;
NOP;
label3:
		NOP;
NOP;
NOP;
		NOP;
		NOP;
		NOP;
NOP;
NOP;
        // Checks the contents of the Trace Buffer

	    EXCPT 0;
		NOP; NOP; NOP; NOP;
CHECKREG(r2,    0x00000006);
CHECKREG(r1,    0x00000416);
CHECKREG(r0,    0x000002aa);
CHECKREG(r3,    0x0000029a);
CHECKREG(r4,    0x00000262);
CHECKREG(r5,    0x00000004);
CHECKREG(r6,    0x0000025a);
CHECKREG(r7,    0x00000288);
		NOP; NOP; NOP; NOP;
		NOP; NOP; NOP; NOP;

		EXCPT 1;
		NOP; NOP; NOP; NOP;
		CHECKREG(r2,    0x00000005);
CHECKREG(r1,    0x00000416);
CHECKREG(r0,    0x00000304);
CHECKREG(r3,    0x000002ac);
CHECKREG(r4,    0x00000470);
CHECKREG(r5,    0x00000003);
CHECKREG(r6,    0x00000276);
CHECKREG(r7,    0x0000024a);
		NOP; NOP; NOP; NOP;
		NOP; NOP; NOP; NOP;

		EXCPT 2;
		NOP; NOP; NOP; NOP;
		CHECKREG(r2,    0x00000004);
CHECKREG(r1,    0x00000416);
CHECKREG(r0,    0x0000035e);
CHECKREG(r3,    0x00000306);
CHECKREG(r4,    0x00000470);
CHECKREG(r5,    0x00000002);
CHECKREG(r6,    0x00000244);
CHECKREG(r7,    0x00000242);
		NOP; NOP; NOP; NOP;

		EXCPT 3;
		NOP; NOP; NOP; NOP;
		CHECKREG(r2,    0x00000003);
CHECKREG(r1,    0x00000416);
CHECKREG(r0,    0x000003b0);
CHECKREG(r3,    0x00000360);
CHECKREG(r4,    0x00000470);
CHECKREG(r5,    0x00000001);
CHECKREG(r6,    0x00000238);
CHECKREG(r7,    0x00000236);



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
    	R7 = SEQSTAT;

RD_MMR(TBUFSTAT, p0, 	r2);
RD_MMR(TBUF,    p0,     r1);
RD_MMR(TBUF,    p0,     r0);
RD_MMR(TBUF,    p0,     r3);
RD_MMR(TBUF,    p0,     r4);
RD_MMR(TBUFSTAT, p0, 	r5);
RD_MMR(TBUF,    p0,     r6);
RD_MMR(TBUF,    p0,     r7);

		NOP; NOP; NOP; NOP;

RTX;

		NOP; NOP; NOP; NOP;
		NOP; NOP; NOP; NOP;

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


    .space (STACKSIZE);
KSTACK:

    .space (STACKSIZE);
USTACK:
