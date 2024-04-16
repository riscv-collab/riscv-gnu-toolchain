//Original:/proj/frio/dv/testcases/debug/dbg_jmp_src_kill/dbg_jmp_src_kill.dsp
// Description: This test checks that the trace buffer keeps track of a JUMP
// source instruction getting killed at each stage in the pipe.  The test
// consists of 8 instances of an EXCPT instruction followed by 0 to 7 NOPs
// and a JUMP, with the trace buffer enabled.
# mach: bfin
# sim: --environment operating

#include "test.h"
.include "testutils.inc"
start

/////////////////////////////////////////////////////////////////////////////
///////////////////////// Include Files         /////////////////////////////
/////////////////////////////////////////////////////////////////////////////

include(std.inc)
include(selfcheck.inc)
include(symtable.inc)
include(mmrs.inc)

/////////////////////////////////////////////////////////////////////////////
///////////////////////// Defines               /////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#ifndef USER_CODE_SPACE
#define USER_CODE_SPACE  CODE_ADDR_1  //
#endif
#ifndef STACKSIZE
#define STACKSIZE        0x00000020
#endif
#ifndef ITABLE
#define ITABLE           CODE_ADDR_2  //
#endif

/////////////////////////////////////////////////////////////////////////////
///////////////////////// RESET ISR             /////////////////////////////
/////////////////////////////////////////////////////////////////////////////

 RST_ISR :

    // Initialize Dregs
INIT_R_REGS(0);

    // Initialize Pregs
INIT_P_REGS(0);

    // Initialize ILBM Registers
INIT_I_REGS(0);
INIT_M_REGS(0);
INIT_L_REGS(0);
INIT_B_REGS(0);

    // Initialize the Address of the Checkreg data segment
    // **** THIS IS NEEDED WHENEVER CHECKREG IS USED ****
CHECK_INIT_DEF(p5);  //CHECK_INIT(p5,   0x00BFFFFC);

    // Setup User Stack
LD32_LABEL(sp, USTACK);
USP = SP;

    // Setup Kernel Stack
LD32_LABEL(sp, KSTACK);

    // Setup Frame Pointer
FP = SP;

    // Setup Event Vector Table
LD32(p0, EVT0);

LD32_LABEL(r0, EMU_ISR);    // Emulation Handler (Int0)
    [ P0 ++ ] = R0;
LD32_LABEL(r0, RST_ISR);    // Reset Handler (Int1)
    [ P0 ++ ] = R0;
LD32_LABEL(r0, NMI_ISR);    // NMI Handler (Int2)
    [ P0 ++ ] = R0;
LD32_LABEL(r0, EXC_ISR);    // Exception Handler (Int3)
    [ P0 ++ ] = R0;
    [ P0 ++ ] = R0;                // IVT4 not used
LD32_LABEL(r0, HWE_ISR);    // HW Error Handler (Int5)
    [ P0 ++ ] = R0;
LD32_LABEL(r0, TMR_ISR);    // Timer Handler (Int6)
    [ P0 ++ ] = R0;
LD32_LABEL(r0, IGV7_ISR);   // IVG7 Handler
    [ P0 ++ ] = R0;
LD32_LABEL(r0, IGV8_ISR);   // IVG8 Handler
    [ P0 ++ ] = R0;
LD32_LABEL(r0, IGV9_ISR);   // IVG9 Handler
    [ P0 ++ ] = R0;
LD32_LABEL(r0, IGV10_ISR);  // IVG10 Handler
    [ P0 ++ ] = R0;
LD32_LABEL(r0, IGV11_ISR);  // IVG11 Handler
    [ P0 ++ ] = R0;
LD32_LABEL(r0, IGV12_ISR);  // IVG12 Handler
    [ P0 ++ ] = R0;
LD32_LABEL(r0, IGV13_ISR);  // IVG13 Handler
    [ P0 ++ ] = R0;
LD32_LABEL(r0, IGV14_ISR);  // IVG14 Handler
    [ P0 ++ ] = R0;
LD32_LABEL(r0, IGV15_ISR);  // IVG15 Handler
    [ P0 ++ ] = R0;

    // Setup the EVT_OVERRIDE MMR
    R0 = 0;
LD32(p0, EVT_OVERRIDE);
    [ P0 ] = R0;

    // Setup Interrupt Mask
    R0 = -1;
LD32(p0, IMASK);
    [ P0 ] = R0;

    // Return to Supervisor Code
RAISE 15;
NOP;

LD32_LABEL(r0, USER_CODE);
RETI = R0;
RTI;

.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
///////////////////////// EMU ISR               /////////////////////////////
/////////////////////////////////////////////////////////////////////////////

 EMU_ISR :

RTE;

.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF

/////////////////////////////////////////////////////////////////////////////
///////////////////////// NMI ISR               /////////////////////////////
/////////////////////////////////////////////////////////////////////////////

 NMI_ISR :

RTN;

.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF

/////////////////////////////////////////////////////////////////////////////
///////////////////////// EXC ISR               /////////////////////////////
/////////////////////////////////////////////////////////////////////////////

 EXC_ISR :

    // Save all the registers used in the ISR
    [ -- SP ] = R0;
    [ -- SP ] = R1;
    [ -- SP ] = P0;
    [ -- SP ] = P1;
    [ -- SP ] = LC0;
    [ -- SP ] = LB0;
    [ -- SP ] = LT0;
    [ -- SP ] = ASTAT;

    // Get EXCAUSE bits out of SEQSTAT
    R0 = SEQSTAT;
    R0 = R0 << 26;
    R0 = R0 >> 26;

    // Check for Trace Exception
    // Load r1 with EXCAUSE for Trace Exception
    R1 = 0x0011 (Z);
    // Check for Trace Exception
CC = R0 == R1;
    // Branch to OUT if the EXCAUSE is not TRACE.
IF !CC JUMP OUT;

    // Read out the Trace Buffer.
LD32(p0, TBUFSTAT);
    // Read TBUFSTAT MMR
    P1 = [ P0 ];

    // if p1 is zero skip the loop.
CC = P1 == 0;
IF CC JUMP OUT;

    // Read out the Entire Trace Buffer.
LD32(p0, TBUF);
LSETUP ( l0s , l0e ) LC0 = P1;
l0s:R0 = [ P0 ];
l0e:R0 = [ P0 ];

OUT:
    // Check for other exception, if any.

    // Restore all saved registers.
ASTAT = [ SP ++ ];
LT0 = [ SP ++ ];
LB0 = [ SP ++ ];
LC0 = [ SP ++ ];
    P1 = [ SP ++ ];
    P0 = [ SP ++ ];
    R1 = [ SP ++ ];
    R0 = [ SP ++ ];

    // Return
RTX;

.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF

/////////////////////////////////////////////////////////////////////////////
///////////////////////// HWE ISR               /////////////////////////////
/////////////////////////////////////////////////////////////////////////////

 HWE_ISR :

RTI;

.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF

/////////////////////////////////////////////////////////////////////////////
///////////////////////// TMR ISR               /////////////////////////////
/////////////////////////////////////////////////////////////////////////////

 TMR_ISR :

RTI;

.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF

/////////////////////////////////////////////////////////////////////////////
///////////////////////// IGV7 ISR              /////////////////////////////
/////////////////////////////////////////////////////////////////////////////

 IGV7_ISR :

RTI;

.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF

/////////////////////////////////////////////////////////////////////////////
///////////////////////// IGV8 ISR              /////////////////////////////
/////////////////////////////////////////////////////////////////////////////

 IGV8_ISR :

RTI;

.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF

/////////////////////////////////////////////////////////////////////////////
///////////////////////// IGV9 ISR              /////////////////////////////
/////////////////////////////////////////////////////////////////////////////

 IGV9_ISR :

RTI;

.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF

/////////////////////////////////////////////////////////////////////////////
///////////////////////// IGV10 ISR             /////////////////////////////
/////////////////////////////////////////////////////////////////////////////

 IGV10_ISR :

RTI;

.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF

/////////////////////////////////////////////////////////////////////////////
///////////////////////// IGV11 ISR             /////////////////////////////
/////////////////////////////////////////////////////////////////////////////

 IGV11_ISR :

RTI;

.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF

/////////////////////////////////////////////////////////////////////////////
///////////////////////// IGV12 ISR             /////////////////////////////
/////////////////////////////////////////////////////////////////////////////

 IGV12_ISR :

RTI;

.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF

/////////////////////////////////////////////////////////////////////////////
///////////////////////// IGV13 ISR             /////////////////////////////
/////////////////////////////////////////////////////////////////////////////

 IGV13_ISR :

RTI;

.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF

/////////////////////////////////////////////////////////////////////////////
///////////////////////// IGV14 ISR             /////////////////////////////
/////////////////////////////////////////////////////////////////////////////

 IGV14_ISR :

RTI;

.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF

/////////////////////////////////////////////////////////////////////////////
///////////////////////// IGV15 ISR             /////////////////////////////
/////////////////////////////////////////////////////////////////////////////

 IGV15_ISR :

WR_MMR(TBUFCTL, 0x7, p0, r0);       // Enable trace buffer & overflow

CSYNC;      // Wait for MMR write to complete

EXCPT 1;
JUMP 4;     // Jump gets killed in WB stage
NOP;
NOP;

EXCPT 2;
NOP;
JUMP 4;     // Jump gets killed in EX3 stage
NOP;
NOP;

EXCPT 3;
NOP;
NOP;
JUMP 4;     // Jump gets killed in EX2 stage
NOP;
NOP;

EXCPT 4;
NOP;
NOP;
NOP;
JUMP 4;     // Jump gets killed in EX1 stage
NOP;
NOP;

EXCPT 5;
NOP;
NOP;
NOP;
NOP;
JUMP 4;     // Jump gets killed in AC stage
NOP;
NOP;

EXCPT 6;
NOP;
NOP;
NOP;
NOP;
NOP;
JUMP 4;     // Jump gets killed in DEC stage
NOP;
NOP;

EXCPT 7;
NOP;
NOP;
NOP;
NOP;
NOP;
NOP;
JUMP 4;     // Jump gets killed in IF2 stage
NOP;
NOP;

EXCPT 8;
NOP;
NOP;
NOP;
NOP;
NOP;
NOP;
NOP;
JUMP 4;     // Jump gets killed in IF1 stage
NOP;
NOP;

    // Read out the Rest of the Trace Buffer.
LD32(p0, TBUFSTAT);
    // Read TBUFSTAT MMR
    P1 = [ P0 ];

    // if p1 is zero skip the loop.
CC = P1 == 0;
IF CC JUMP OUT1;

    // Read out the Entire Trace Buffer.
LD32(p0, TBUF);
LSETUP ( l1s , l1e ) LC0 = P1;
l1s:R0 = [ P0 ];
l1e:R0 = [ P0 ];

    // Don't RTI if you never wish to go to User Mode
    // use END_TEST instead.

OUT1:
dbg_pass;

//  rti;

.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF

/////////////////////////////////////////////////////////////////////////////
///////////////////////// USER CODE             /////////////////////////////
/////////////////////////////////////////////////////////////////////////////


 USER_CODE :

    // YOUR USER CODE GOES HERE.

dbg_pass;        // Call Endtest Macro

/////////////////////////////////////////////////////////////////////////////
///////////////////////// DATA MEMRORY          /////////////////////////////
/////////////////////////////////////////////////////////////////////////////

.section MEM_DATA_ADDR_1 //.data 0x00F00100,"aw"
.dd 0x01010101;
.dd 0x02020202;
.dd 0x03030303;
.dd 0x04040404;
.dd 0x05050505;
.dd 0x06060606;
.dd 0x07070707;
.dd 0x08080808;
.dd 0x09090909;
.dd 0x0a0a0a0a;
.dd 0x0b0b0b0b;
.dd 0x0c0c0c0c;
.dd 0x0d0d0d0d;
.dd 0x0e0e0e0e;
.dd 0x0f0f0f0f;

// Define Kernal Stack
.section MEM_DATA_ADDR_2 //.data 0x00F00210,"aw"
    .space (STACKSIZE);
     KSTACK :

    .space (STACKSIZE);
     USTACK :

/////////////////////////////////////////////////////////////////////////////
///////////////////////// END OF TEST           /////////////////////////////
/////////////////////////////////////////////////////////////////////////////
