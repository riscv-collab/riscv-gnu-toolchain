//Original:/proj/frio/dv/testcases/debug/dbg_tr_tbuf0/dbg_tr_tbuf0.dsp
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



 START :

WR_MMR(TBUFCTL, 0x00000001, p0, r0);        // Turn ON trace Buffer
WR_MMR(TBUFCTL, 0x0000000b, p0, r0);        // Turn ON trace Buffer
                                                    //   TBUFPWR   = 1
                                                    //   TBUFEN    = 1
                                                    //   TBUFOVF   = 0
                                                    //   CMPLP     = 01
NOP;
NOP;
NOP;
		NOP;
		NOP;
		R6 = 0;
		R7 = 10;

JMP:
	   	JUMP.S LABEL0;
		NOP;
		NOP;

LABEL0:
	 P1 = 0x0006;
		JUMP (PC+P1);

LABEL1:
	 	 LD32(R3, 0xBADD); //<< WARNING: LINE MAY NEED MANUAL TRANSLATION >>

LABEL2:
	  CC = R7 == R6;
		 IF CC JUMP END;
		 R6 += 1;
		 JUMP LABEL2;

LABEL3:
	  NOP;

LABEL4:
	  	  LD32(R4, 0xBADD); //<< WARNING: LINE MAY NEED MANUAL TRANSLATION >>




END:
        R0 = 1;
	    NOP;
	    NOP;
		NOP;

CHECKREG(r3,    0x00000000);
CHECKREG(r4,    0x00000000);
        // Read the contents of the Trace Buffer

RD_MMR(TBUFSTAT, p0, r0);
CHECKREG(r0,    0x00000004);

        // Read last entry of the Trace Buffer
RD_MMR(TBUF,    p0,     r1);
CHECKREG(r1,    0x00000256);

RD_MMR(TBUF,    p0,     r2);
CHECKREG(r2,    0x00000246);

RD_MMR(TBUFSTAT, p0, r0);
CHECKREG(r0,    0x00000003);

        // Read last entry of the Trace Buffer
RD_MMR(TBUF,    p0,     r1);
CHECKREG(r1,    0x00000245);

RD_MMR(TBUF,    p0,     r2);
CHECKREG(r2,    0x0000024a);

RD_MMR(TBUFSTAT, p0, r0);
CHECKREG(r0,    0x00000002);

        // Read last entry of the Trace Buffer
RD_MMR(TBUF,    p0,     r1);
CHECKREG(r1,    0x00000240);

RD_MMR(TBUF,    p0,     r2);
CHECKREG(r2,    0x0000023a);

RD_MMR(TBUFSTAT, p0, r0);
CHECKREG(r0,    0x00000001);

        // Read last entry of the Trace Buffer
RD_MMR(TBUF,    p0,     r1);
CHECKREG(r1,    0x00000238);

RD_MMR(TBUF,    p0,     r2);
CHECKREG(r2,    0x00000232);



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
	NOP;NOP;NOP;NOP;NOP;
	NOP;NOP;NOP;NOP;NOP;

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
