//Original:/proj/frio/dv/testcases/seq/se_kills2/se_kills2.dsp
// Description: Test se_kill for all supported types of RTL1 instructions
# mach: bfin
# sim: --environment operating

#include "test.h"
.include "testutils.inc"
start

//
// Constants and Defines
//

include(selfcheck.inc)
include(std.inc)
include(symtable.inc)

#ifndef STACKSIZE
#define STACKSIZE 0x10
#endif

//
// Reset/Bootstrap Code
//   (Here we should set the processor operating modes, initialize registers,
//    etc.)
//

BOOT:
SP = 0 (Z);
SP.L = KSTACK;     // setup the stack pointer
SP.H = KSTACK;
FP = SP;        // and frame pointer

INIT_R_REGS(0);     // initialize general purpose regs

INIT_P_REGS(0);     // initialize the pointers

INIT_I_REGS(0);     // initialize the dsp address regs
INIT_M_REGS(0);
INIT_L_REGS(0);
INIT_B_REGS(0);

ASTAT = r0;     // reset sequencer registers

//
// The Main Program
//

START:

                // **** YOUR CODE GOES HERE ****
 // CHECK_INIT(p0, 0xFF7FFFFC);   // original
CHECK_INIT_DEF(p0);

    R0 = 0;
    R1 = 1;
    R2 = 2;
    R3 = 3;
    R4 = 4;
    R5 = 5;
    R6 = 6;
    R7 = 7;
    P1 = 11;
                // Assume CC is reset to 0.
IF !CC JUMP NEXT1;      // following instruction should be killed
RAISE 13;

NEXT1:
	  IF !CC JUMP NEXT2;
EXCPT 15;

NEXT2:
	  IF !CC JUMP NEXT3;
    ( R7:0, P5:0 ) = [ SP ++ ];

NEXT3:
	  IF !CC JUMP NEXT4;
    [ -- SP ] = ( R7:0, P5:0 );

NEXT4:
	  IF !CC JUMP NEXT5;
EMUEXCPT;

NEXT5:
	  IF !CC JUMP NEXT6;
.dd 0xFACEBABE

NEXT6:
	  IF !CC JUMP NEXT7;
LINK 12;

NEXT7:
	  IF !CC JUMP NEXT8;
UNLINK;

NEXT8:
	  IF !CC JUMP NEXT9;
LSETUP (NEXT10, NEXT11) lc0 = p0;

NEXT9:
	  IF !CC JUMP NEXT10;

NEXT10:
	 IF !CC JUMP NEXT11;

NEXT11:
	 IF !CC JUMP NEXT12;

NEXT12:
	 IF !CC JUMP NEXT13;

NEXT13:
	 IF !CC JUMP NEXT14;

NEXT14:
	 IF !CC JUMP NEXT15;

NEXT15:
	 IF !CC JUMP NEXT16;

NEXT16:

END:
CHECKREG(r0, 0);
CHECKREG(r1, 1);
CHECKREG(r2, 2);
CHECKREG(r3, 3);
CHECKREG(r4, 4);
CHECKREG(r5, 5);
CHECKREG(r6, 6);
CHECKREG(r7, 7);

dbg_pass;       // Call Endtest Macro

//*********************************************************************
//
// Data Segment
//

//.data 0xF0000000
.data
DATA:
    .space (0x010); // Some data space

// Stack Segments

    .space (STACKSIZE);
KSTACK:
