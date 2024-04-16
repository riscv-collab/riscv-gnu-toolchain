//Original:/proj/frio/dv/testcases/seq/se_lsetup_kill/se_lsetup_kill.dsp
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
#define USER_CODE_SPACE  CODE_ADDR_1   //
#endif
#ifndef STACKSIZE
#define STACKSIZE        0x00000010
#endif
#ifndef ITABLE
#define ITABLE           CODE_ADDR_2   //
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
CHECK_INIT_DEF(p5); //CHECK_INIT(p5,   0x00BFFFFC);

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

    P0 = 0x5 (Z);
    P1 = 0xa (Z);

NOP;
NOP;
RTI;

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

/////////////////////////////////////////////////////////////////////////////
// Loop 0 (Kill Lsetup in WB)
/////////////////////////////////////////////////////////////////////////////

EXCPT 0x5;
LSETUP ( L0T , L0T ) LC0 = P0;
L0T:R0 += 5;

EXCPT 0x5;
LSETUP ( L1T , L1B ) LC0 = P0;
L1T:R0 += 5;
L1B:R1 += 4;

EXCPT 0x5;
LSETUP ( L2T , L2B ) LC0 = P0;
L2T:R0 += 5;
    R1 += 4;
L2B:R2 += 3;

EXCPT 0x5;
LSETUP ( L3T , L3B ) LC0 = P0;
L3T:R0 += 5;
    R1 += 4;
    R2 += 3;
L3B:R3 += 2;

EXCPT 0x5;
LSETUP ( L4T , L4B ) LC0 = P0;
L4T:R0 += 5;
    R1 += 4;
    R2 += 3;
    R3 += 2;
L4B:R4 += 1;

/////////////////////////////////////////////////////////////////////////////
// Loop 1 (Kill Lsetup in WB)
/////////////////////////////////////////////////////////////////////////////

EXCPT 0x5;
LSETUP ( M0T , M0T ) LC1 = P0;
M0T:R0 += 5;

EXCPT 0x5;
LSETUP ( M1T , M1B ) LC1 = P0;
M1T:R0 += 5;
M1B:R1 += 4;

EXCPT 0x5;
LSETUP ( M2T , M2B ) LC1 = P0;
M2T:R0 += 5;
    R1 += 4;
M2B:R2 += 3;

EXCPT 0x5;
LSETUP ( M3T , M3B ) LC1 = P0;
M3T:R0 += 5;
    R1 += 4;
    R2 += 3;
M3B:R3 += 2;

EXCPT 0x5;
LSETUP ( M4T , M4B ) LC1 = P0;
M4T:R0 += 5;
    R1 += 4;
    R2 += 3;
    R3 += 2;
M4B:R4 += 1;

/////////////////////////////////////////////////////////////////////////////
// Loop 0 (Kill during the last iteration at each pipe stage)
/////////////////////////////////////////////////////////////////////////////

LSETUP ( N0T , N0B ) LC0 = P1;
NOP;
N0T:R0 = LC0;
CC = R0 == 1;
IF !CC JUMP N0B (BP);
    R0 += 1;
    R1 += 2;
EXCPT 0x5;
N0B:R2 += 3;

LSETUP ( N1T , N1B ) LC0 = P1;
NOP;
N1T:R0 = LC0;
    R0 += 1;
    R1 += 2;
CC = R0 == 1;
IF !CC JUMP N1B (BP);
EXCPT 0x5;
N1B:R2 += 3;

LSETUP ( N2T , N2B ) LC0 = P1;
NOP;
N2T:R0 = LC0;
CC = R0 == 1;
IF !CC JUMP N2B (BP);
    R0 += 1;
    R1 += 2;
EXCPT 0x5;
    R3 += 4;
N2B:R2 += 3;

LSETUP ( N3T , N3B ) LC0 = P1;
NOP;
N3T:R0 = LC0;
    R0 += 1;
    R1 += 2;
CC = R0 == 1;
IF !CC JUMP N3B (BP);
EXCPT 0x5;
    R3 += 4;
N3B:R2 += 3;

LSETUP ( N4T , N4B ) LC0 = P1;
NOP;
N4T:R0 = LC0;
CC = R0 == 1;
IF !CC JUMP N4B (BP);
    R0 += 1;
    R1 += 2;
EXCPT 0x5;
    R3 += 4;
    R4 += 5;
N4B:R2 += 3;

LSETUP ( N5T , N5B ) LC0 = P1;
NOP;
N5T:R0 = LC0;
    R0 += 1;
    R1 += 2;
CC = R0 == 1;
IF !CC JUMP N5B (BP);
EXCPT 0x5;
    R3 += 4;
    R4 += 5;
N5B:R2 += 3;

LSETUP ( N6T , N6B ) LC0 = P1;
NOP;
N6T:R0 = LC0;
CC = R0 == 1;
IF !CC JUMP N6B (BP);
    R0 += 1;
    R1 += 2;
EXCPT 0x5;
    R3 += 4;
    R4 += 5;
    R5 += 6;
N6B:R2 += 3;

LSETUP ( N7T , N7B ) LC0 = P1;
NOP;
N7T:R0 = LC0;
    R0 += 1;
    R1 += 2;
CC = R0 == 1;
IF !CC JUMP N7B (BP);
EXCPT 0x5;
    R3 += 4;
    R4 += 5;
    R5 += 6;
N7B:R2 += 3;

LSETUP ( N8T , N8B ) LC0 = P1;
NOP;
N8T:R0 = LC0;
CC = R0 == 1;
IF !CC JUMP N8B (BP);
    R0 += 1;
    R1 += 2;
EXCPT 0x5;
    R3 += 4;
    R4 += 5;
    R5 += 6;
    R6 += 7;
N8B:R2 += 3;

LSETUP ( N9T , N9B ) LC0 = P1;
NOP;
N9T:R0 = LC0;
    R0 += 1;
    R1 += 2;
CC = R0 == 1;
IF !CC JUMP N9B (BP);
EXCPT 0x5;
    R3 += 4;
    R4 += 5;
    R5 += 6;
    R6 += 7;
N9B:R2 += 3;

LSETUP ( NAT , NAB ) LC0 = P1;
NOP;
NAT:
	R0 = LC0;
CC = R0 == 1;
IF !CC JUMP NAB (BP);
    R0 += 1;
    R1 += 2;
EXCPT 0x5;
    R3 += 4;
    R4 += 5;
    R5 += 6;
    R6 += 7;
    R7 += 8;
NAB:
	R2 += 3;

LSETUP ( NBT , NBB ) LC0 = P1;
NOP;
NBT:
	R0 = LC0;
    R0 += 1;
    R1 += 2;
CC = R0 == 1;
IF !CC JUMP NBB (BP);
EXCPT 0x5;
    R3 += 4;
    R4 += 5;
    R5 += 6;
    R6 += 7;
    R7 += 8;
NBB:
	R2 += 3;


/////////////////////////////////////////////////////////////////////////////
// Loop 1 (Kill during the last iteration at each pipe stage)
/////////////////////////////////////////////////////////////////////////////

LSETUP ( O0T , O0B ) LC1 = P1;
NOP;
O0T:R0 = LC1;
CC = R0 == 1;
IF !CC JUMP O0B (BP);
    R0 += 1;
    R1 += 2;
EXCPT 0x5;
O0B:R2 += 3;

LSETUP ( O1T , O1B ) LC1 = P1;
NOP;
O1T:R0 = LC1;
    R0 += 1;
    R1 += 2;
CC = R0 == 1;
IF !CC JUMP O1B (BP);
EXCPT 0x5;
O1B:R2 += 3;

LSETUP ( O2T , O2B ) LC1 = P1;
NOP;
O2T:R0 = LC1;
CC = R0 == 1;
IF !CC JUMP O2B (BP);
    R0 += 1;
    R1 += 2;
EXCPT 0x5;
    R3 += 4;
O2B:R2 += 3;

LSETUP ( O3T , O3B ) LC1 = P1;
NOP;
O3T:R0 = LC1;
    R0 += 1;
    R1 += 2;
CC = R0 == 1;
IF !CC JUMP O3B (BP);
EXCPT 0x5;
    R3 += 4;
O3B:R2 += 3;

LSETUP ( O4T , O4B ) LC1 = P1;
NOP;
O4T:R0 = LC1;
CC = R0 == 1;
IF !CC JUMP O4B (BP);
    R0 += 1;
    R1 += 2;
EXCPT 0x5;
    R3 += 4;
    R4 += 5;
O4B:R2 += 3;

LSETUP ( O5T , O5B ) LC1 = P1;
NOP;
O5T:R0 = LC1;
    R0 += 1;
    R1 += 2;
CC = R0 == 1;
IF !CC JUMP O5B (BP);
EXCPT 0x5;
    R3 += 4;
    R4 += 5;
O5B:R2 += 3;

LSETUP ( O6T , O6B ) LC1 = P1;
NOP;
O6T:R0 = LC1;
CC = R0 == 1;
IF !CC JUMP O6B (BP);
    R0 += 1;
    R1 += 2;
EXCPT 0x5;
    R3 += 4;
    R4 += 5;
    R5 += 6;
O6B:R2 += 3;

LSETUP ( O7T , O7B ) LC1 = P1;
NOP;
O7T:R0 = LC1;
    R0 += 1;
    R1 += 2;
CC = R0 == 1;
IF !CC JUMP O7B (BP);
EXCPT 0x5;
    R3 += 4;
    R4 += 5;
    R5 += 6;
O7B:R2 += 3;

LSETUP ( O8T , O8B ) LC1 = P1;
NOP;
O8T:R0 = LC1;
CC = R0 == 1;
IF !CC JUMP O8B (BP);
    R0 += 1;
    R1 += 2;
EXCPT 0x5;
    R3 += 4;
    R4 += 5;
    R5 += 6;
    R6 += 7;
O8B:R2 += 3;

LSETUP ( O9T , O9B ) LC1 = P1;
NOP;
O9T:R0 = LC1;
    R0 += 1;
    R1 += 2;
CC = R0 == 1;
IF !CC JUMP O9B (BP);
EXCPT 0x5;
    R3 += 4;
    R4 += 5;
    R5 += 6;
    R6 += 7;
O9B:R2 += 3;

LSETUP ( OAT , OAB ) LC1 = P1;
NOP;
OAT:
	R0 = LC1;
CC = R0 == 1;
IF !CC JUMP OAB (BP);
    R0 += 1;
    R1 += 2;
EXCPT 0x5;
    R3 += 4;
    R4 += 5;
    R5 += 6;
    R6 += 7;
    R7 += 8;
OAB:
	R2 += 3;

LSETUP ( OBT , OBB ) LC1 = P1;
NOP;
OBT:
	R0 = LC1;
    R0 += 1;
    R1 += 2;
CC = R0 == 1;
IF !CC JUMP OBB (BP);
EXCPT 0x5;
    R3 += 4;
    R4 += 5;
    R5 += 6;
    R6 += 7;
    R7 += 8;
OBB:
	R2 += 3;

NOP;
NOP;
NOP;
dbg_pass;        // Call Endtest Macro

/////////////////////////////////////////////////////////////////////////////
///////////////////////// DATA MEMRORY          /////////////////////////////
/////////////////////////////////////////////////////////////////////////////

.section MEM_DATA_ADDR_1 //.data 0x00F00100,"aw"
.dd 0xdeadbeef;
.section MEM_(DATA_ADDR_1 + 0x100) //.data 0x00F00200,"aw"
.dd 0x01010101; //<< WARNING: LINE MAY NEED MANUAL TRANSLATION >>
.dd 0x02020202;
.dd 0x03030303;
.dd 0x04040404;

// Define Kernal Stack
.data
    .space (STACKSIZE); //<< WARNING: LINE MAY NEED MANUAL TRANSLATION >>
     KSTACK :

    .space (STACKSIZE);
     USTACK :

/////////////////////////////////////////////////////////////////////////////
///////////////////////// END OF TEST           /////////////////////////////
/////////////////////////////////////////////////////////////////////////////
