//Original:/proj/frio/dv/testcases/seq/se_cc_kill/se_cc_kill.dsp
// Description:
//   Verify CC kill under the following condition:
//
//      (1) CC = AZ     killed in WB
//      (2) CC = AN     killed in WB
//      (3) CC = AC     killed in WB
//      (4) CC = AV0    killed in WB
//      (5) CC = AV1    killed in WB
//      (6) CC = AQ     killed in WB
# mach: bfin
# sim: --environment operating

#include "test.h"
.include "testutils.inc"
start

// ----------------------------------------------------------------
//   Include Files
// ----------------------------------------------------------------

include(std.inc)
include(selfcheck.inc)
include(symtable.inc)
include(mmrs.inc)

// ----------------------------------------------------------------
//   Defines
// ----------------------------------------------------------------

#ifndef STACKSIZE
#define STACKSIZE     0x00000010
#endif
#ifndef ITABLE
#define ITABLE        CODE_ADDR_1  //
#endif


// ----------------------------------------------------------------
//   Reset ISR
//      - set the processor operating modes
//      - initialize registers
//      - etc ...
// ----------------------------------------------------------------

RST_ISR:

    // Initialize data registers
    //INIT_R_REGS(0);
    R7 = 0;
    R6 = 0;
    R5 = 0;
    R4 = 0;
    R3 = 0;
    R2 = 0;
    R1 = 0;
    R0 = 0;

    // Initialize pointer registers
INIT_P_REGS(0);

    // Initialize address registers
INIT_I_REGS(0);
INIT_M_REGS(0);
INIT_L_REGS(0);
INIT_B_REGS(0);

    // Initialize the address of the checkreg data segment
    // **** THIS IS NEEDED WHENEVER CHECKREG IS USED ****
CHECK_INIT_DEF(p5); //CHECK_INIT(p5, 0x00BFFFFC);

    // Inhibit events during MMR writes
CLI R1;

    // Setup user stack
LD32_LABEL(sp, USTACK);
USP = SP;

    // Setup kernel stack
LD32_LABEL(sp, KSTACK);

    // Setup frame pointer
FP = SP;

    // Setup event vector table
LD32(p0, EVT0);

LD32_LABEL(r0, EMU_ISR);    // Emulation Handler (EVT0)
    [ P0 ++ ] = R0;
LD32_LABEL(r0, RST_ISR);    // Reset Handler (EVT1)
    [ P0 ++ ] = R0;
LD32_LABEL(r0, NMI_ISR);    // NMI Handler (EVT2)
    [ P0 ++ ] = R0;
LD32_LABEL(r0, EXC_ISR);    // Exception Handler (EVT3)
    [ P0 ++ ] = R0;
    [ P0 ++ ] = R0;                // EVT4 not used
LD32_LABEL(r0, HWE_ISR);    // HW Error Handler (EVT5)
    [ P0 ++ ] = R0;
LD32_LABEL(r0, TMR_ISR);    // Timer Handler (EVT6)
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

    // Set the EVT_OVERRIDE MMR
LD32(p0, EVT_OVERRIDE);
    R0 = 0;
    [ P0 ++ ] = R0;

    // Disable L1 data cache
WR_MMR(DMEM_CONTROL, 0x00000000, p0, r0);

    // Mask interrupts (*)
    R1 = -1;

    // Wait for MMR writes to finish
CSYNC;

    // Re-enable events
STI R1;

    // Reset loop counters to deterministic values
    R0 = 0 (Z);

LT0 = R0;
LB0 = R0;
LC0 = R0;
LT1 = R0;
LB1 = R0;
LC1 = R0;

    // Reset other internal regs
ASTAT = R0;
SYSCFG = R0;
RETS = R0;

    // Setup the test to run in USER mode
LD32_LABEL(r0, USER_CODE);
RETI = R0;

    // Setup the test to run in SUPERVISOR mode
    // Comment the following line for a USER mode test
JUMP.S SUPERVISOR_CODE;
RTI;

SUPERVISOR_CODE:
    // Load IVG15 general handler (Int15) with MAIN_CODE
LD32_LABEL(p1, MAIN_CODE);

LD32(p0, EVT15);

CLI R1;
    [ P0 ] = P1;
CSYNC;
STI R1;

    // Take Int15 which branch to MAIN_CODE after RTI
RAISE 15;
RTI;

USER_CODE:
    // Setup the stack pointer and the frame pointer
LD32_LABEL(sp, USTACK);
FP = SP;
JUMP.S MAIN_CODE;

.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF

// ----------------------------------------------------------------
//   ISR Table
// ----------------------------------------------------------------


// ----------------------------------------------------------------
//   EMU ISR
// ----------------------------------------------------------------

 EMU_ISR :

RTE;

.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF

// ----------------------------------------------------------------
//   NMI ISR
// ----------------------------------------------------------------

 NMI_ISR :

RTN;

.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF

// ----------------------------------------------------------------
//   EXC ISR
// ----------------------------------------------------------------

 EXC_ISR :

RTX;

.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF

// ----------------------------------------------------------------
//   HWE ISR
// ----------------------------------------------------------------

 HWE_ISR :

RTI;

.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF

// ----------------------------------------------------------------
//   TMR ISR
// ----------------------------------------------------------------

 TMR_ISR :

RTI;

.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF

// ----------------------------------------------------------------
//   IGV7 ISR
// ----------------------------------------------------------------

 IGV7_ISR :

RTI;

.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF

// ----------------------------------------------------------------
//   IGV8 ISR
// ----------------------------------------------------------------

 IGV8_ISR :

RTI;

.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF

// ----------------------------------------------------------------
//   IGV9 ISR
// ----------------------------------------------------------------

 IGV9_ISR :

RTI;

.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF

// ----------------------------------------------------------------
//   IGV10 ISR
// ----------------------------------------------------------------

 IGV10_ISR :

RTI;

.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF

// ----------------------------------------------------------------
//   IGV11 ISR
// ----------------------------------------------------------------

 IGV11_ISR :

RTI;

.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF

// ----------------------------------------------------------------
//   IGV12 ISR
// ----------------------------------------------------------------

 IGV12_ISR :

RTI;

.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF

// ----------------------------------------------------------------
//   IGV13 ISR
// ----------------------------------------------------------------

 IGV13_ISR :

RTI;

.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF

// ----------------------------------------------------------------
//   IGV14 ISR
// ----------------------------------------------------------------

 IGV14_ISR :

RTI;

.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF

// ----------------------------------------------------------------
//   IGV15 ISR
// ----------------------------------------------------------------

 IGV15_ISR :

RTI;

.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF
.dw 0xFFFF

// ----------------------------------------------------------------
//   Main Code
// ----------------------------------------------------------------


MAIN_CODE:
    // Enable interrupts in SUPERVISOR mode
    // Comment the following line for a USER mode test
    [ -- SP ] = RETI;

    // Start of the program code

    //   Verify CC kill under the following condition:

    //      (1) CC = AZ     killed in WB
CC = R2 < R3;
EXCPT 3;
CC = AZ;

    //      (2) CC = AN     killed in WB
CC = R2 == R3;
EXCPT 3;
CC = AN;

    //      (3) CC = AC     killed in WB
CC = R2 < R3;
EXCPT 3;
CC = AC0;

    //      (4) CC = AV0    killed in WB
CC = R2 == R3;
EXCPT 3;
CC = AV0;

    //      (5) CC = AV1    killed in WB
CC = R2 == R3;
EXCPT 3;
CC = AV1;

    //      (6) CC = AQ     killed in WB
CC = R2 == R3;
EXCPT 3;
CC = AQ;


END:
dbg_pass;

// ----------------------------------------------------------------
//   Data Segment
//      - define kernel and user stacks
// ----------------------------------------------------------------

.data
    DATA:
    .space (STACKSIZE);

    .space (STACKSIZE);
    KSTACK:

    .space (STACKSIZE);
    USTACK:
