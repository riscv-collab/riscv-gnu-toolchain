//Original:/proj/frio/dv/testcases/seq/se_cc2stat_haz/se_cc2stat_haz.dsp
// Description:
//   Verify CC hazards under the following condition:
//
//      (1a) cc2stat (that modifies CC) followed by  that uses CC
//      (1b) same as (1a) but kill cc2stat instruction in WB
//
//      (2a) cc2stat (that modifies CC) followed by conditional branch (predicted)
//      (2b) same as (2a) but kill cc2stat instruction in WB
//
//      (3a) cc2stat (that modifies CC) followed by conditional branch (mispredicted)
//      (3b) same as (3a) but kill cc2stat instruction in WB
//
//      (4a) cc2stat (that modifies CC) followed by testset
//      (4b) same as (4a) but kill cc2stat instruction in WB
//
//      (5a) cc2stat (that modifies CC) followed by dag instruction that modifies CC
//      (5b) same as (5a) but kill cc2stat instruction in WB
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
#define ITABLE        CODE_ADDR_1   //
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

    // Reset accumulator registers
    A0 = 0;
    A1 = 0;

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
    R0 = 0;
    R1 = 1;
    R2 = 2;

    //   Verify CC hazards under the following condition:
    //
    //      (1a) cc2stat (that modifies CC) followed by  that uses CC
    A0 = 0;
    A1 = R1;
CC = R0 < R2;
CC = AV0;
    A0 = BXORSHIFT( A0 , A1, CC );
    R7 = CC;   CHECKREG(R7, 0);
    R6 = A0;   CHECKREG(R6, 0);
    R6 = A0.X; CHECKREG(R6, 0);
    R7 = A1;   CHECKREG(R7, 1);
    R7 = A1.X; CHECKREG(R7, 0);

    //      (1b) same as (1a) but kill cc2stat instruction in WB
    A0 = R1;
    A1 = R1;
CC = R0 < R2;
EXCPT 3;
CC = AV0;
    A0 = BXORSHIFT( A0 , A1, CC );
    R7 = CC;   CHECKREG(R7, 0);
    R6 = A0;   CHECKREG(R6, 3);
    R6 = A0.X; CHECKREG(R6, 0);
    R7 = A1;   CHECKREG(R7, 1);
    R7 = A1.X; CHECKREG(R7, 0);

    //      (2a) cc2stat (that modifies CC) followed by conditional branch (predicted)
    R3 = 0;
    A0 = 0;
    A1 = R1;
CC = R0 < R2;
CC = AV0;
IF !CC JUMP INC_R3_TO_10 (BP);
    R3 += 2;
    R3 += 2;
    R3 += 2;
    R3 += 2;
    R3 += 2;
    R3 += 2;
    R3 += 2;
    R3 += 2;
    R3 += 2;
    R3 += 2;
INC_R3_TO_10:
    R3 += 1;
    R3 += 1;
    R3 += 1;
    R3 += 1;
    R3 += 1;
    R3 += 1;
    R3 += 1;
    R3 += 1;
    R3 += 1;
    R3 += 1;

    //      (2b) same as (2a) but kill cc2stat instruction in WB
    A0 = 0;
    A1 = R1;
CC = R0 < R2;
EXCPT 3;
CC = AV0;
IF !CC JUMP INC_R3_TO_20 (BP);
    R3 += 2;
    R3 += 2;
    R3 += 2;
    R3 += 2;
    R3 += 2;
    R3 += 2;
    R3 += 2;
    R3 += 2;
    R3 += 2;
    R3 += 2;
INC_R3_TO_20:
    R3 += 1;
    R3 += 1;
    R3 += 1;
    R3 += 1;
    R3 += 1;
    R3 += 1;
    R3 += 1;
    R3 += 1;
    R3 += 1;
    R3 += 1;

    //      (3a) cc2stat (that modifies CC) followed by conditional branch (mispredicted)
    A0 = 0;
    A1 = R1;
CC = R0 < R2;
CC = AV0;
IF CC JUMP INC_R3_TO_20 (BP);
    R3 += 2;
    R3 += 2;
    R3 += 2;
    R3 += 2;
    R3 += 2;
    R3 += 2;
    R3 += 2;
    R3 += 2;
    R3 += 2;
    R3 += 2;

    //      (3b) same as (3a) but kill cc2stat instruction in WB
    A0 = 0;
    A1 = R1;
CC = R0 < R2;
EXCPT 3;
CC = AV0;
IF CC JUMP INC_R3_TO_20 (BP);
    R3 += 2;
    R3 += 2;
    R3 += 2;
    R3 += 2;
    R3 += 2;
    R3 += 2;
    R3 += 2;
    R3 += 2;
    R3 += 2;
    R3 += 2;

CHECKREG(r3, 60);

dbg_pass;

    //      (4a) cc2stat (that modifies CC) followed by testset
LD32(p0, DATA_ADDR_3);  //LD32(p0, 0xff000000);
LD32(p1, DATA_ADDR_2);  //LD32(p1, 0xffe00000);
    [ P0 ] = R0;

    A0 = 0;
    A1 = R1;
CC = R0 < R2;
CC = AV0;
QUERY_0:
TESTSET ( P0 );
IF !CC JUMP QUERY_0;
    [ P0 ] = R1;
CHECKMEM32(DATA_ADDR_3, 1);  //CHECKMEM32(0xff000000, 1);
    [ P0 ] = R0;
CHECKMEM32(DATA_ADDR_3, 0);  //CHECKMEM32(0xff000000, 0);

    //      (4b) same as (4a) but kill cc2stat instruction in WB
    A0 = 0;
    A1 = R1;
CC = R0 < R2;
EXCPT 3;
CC = AV0;
QUERY_1:
TESTSET ( P0 );
IF !CC JUMP QUERY_1;
    [ P0 ] = R2;
CHECKMEM32(DATA_ADDR_3, 2);  //CHECKMEM32(0xff000000, 2);
    [ P0 ] = R0;
CHECKMEM32(DATA_ADDR_3, 0);  //CHECKMEM32(0xff000000, 0);

    //      (5a) cc2stat (that modifies CC) followed by dag instruction that modifies CC
    A0 = 0;
    A1 = R1;
CC = R0 < R2;
CC = AV0;
CC = P0 < P1;

    //      (5b) same as (5a) but kill cc2stat instruction in WB
    A0 = 0;
    A1 = R1;
CC = R0 < R2;
EXCPT 3;
CC = AV0;
CC = P0 < P1;


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
