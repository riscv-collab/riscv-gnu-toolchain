//Original:/proj/frio/dv/testcases/seq/se_loop_mv2lc/se_loop_mv2lc.dsp
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
    P1 = 0x3 (Z);

    // Loop 0
LD32_LABEL(r0, L0T);
LD32_LABEL(r1, L0B);
LT0 = r0;
LB0 = r1;

LC0 = P0;
NOP;
JUMP.S 2;

JUMP.S 6;
NOP;
LC0 = P0;
LC0 = P1;
L0T:R2 += 3;
    R3 += 4;
    R4 += 5;
    R5 += 6;
    R6 += 7;
L0B:R7 += 8;

    // Loop 1
LD32_LABEL(r0, L1T);
LD32_LABEL(r1, L1B);
LT1 = r0;
LB1 = r1;

LC1 = P0;
NOP;
JUMP.S 2;

JUMP.S 6;
NOP;
LC1 = P0;
LC1 = P1;
L1T:R2 += 3;
    R3 += 4;
    R4 += 5;
    R5 += 6;
    R6 += 7;
L1B:R7 += 8;

    // Loop 0
LSETUP ( L2T , L2T ) LC0 = P0;
NOP;
NOP;
NOP;
LC0 = P1;
L2T:R2 += 1;
    R3 += 2;
    R4 += 3;
    R5 += 4;
    R6 += 5;
L2B:R7 += 6;

LC0 = P1;
NOP;
NOP;
NOP;
LSETUP ( L3T , L3T ) LC0 = P0;
L3T:R2 += 1;
    R3 += 2;
    R4 += 3;
    R5 += 4;
    R6 += 5;
L3B:R7 += 6;

LSETUP ( L4T , L4B ) LC0 = P0;
NOP;
NOP;
LC0 = P1;
L4T:R2 += 1;
L4B:R3 += 2;
    R4 += 3;
    R5 += 4;
    R6 += 5;
    R7 += 6;

LC0 = P1;
NOP;
NOP;
LSETUP ( L5T , L5B ) LC0 = P0;
L5T:R2 += 1;
L5B:R3 += 2;
    R4 += 3;
    R5 += 4;
    R6 += 5;
    R7 += 6;

LSETUP ( L6T , L6B ) LC0 = P0;
NOP;
LC0 = P1;
L6T:R2 += 1;
    R3 += 2;
L6B:R4 += 3;
    R5 += 4;
    R6 += 5;
    R7 += 6;

LC0 = P1;
NOP;
LSETUP ( L7T , L7B ) LC0 = P0;
L7T:R2 += 1;
    R3 += 2;
L7B:R4 += 3;
    R5 += 4;
    R6 += 5;
    R7 += 6;

LSETUP ( L8T , L8B ) LC0 = P0;
LC0 = P1;
L8T:R2 += 1;
    R3 += 2;
    R4 += 3;
L8B:R5 += 4;
    R6 += 5;
    R7 += 6;

LC0 = P1;
LSETUP ( L9T , L9B ) LC0 = P0;
L9T:R2 += 1;
    R3 += 2;
    R4 += 3;
L9B:R5 += 4;
    R6 += 5;
    R7 += 6;


    //  Loop 1
LSETUP ( M2T , M2T ) LC1 = P0;
NOP;
NOP;
NOP;
LC1 = P1;
M2T:R2 += 1;
    R3 += 2;
    R4 += 3;
    R5 += 4;
    R6 += 5;
M2B:R7 += 6;

LC1 = P1;
NOP;
NOP;
NOP;
LSETUP ( M3T , M3T ) LC1 = P0;
M3T:R2 += 1;
    R3 += 2;
    R4 += 3;
    R5 += 4;
    R6 += 5;
M3B:R7 += 6;

LSETUP ( M4T , M4B ) LC1 = P0;
NOP;
NOP;
LC1 = P1;
M4T:R2 += 1;
M4B:R3 += 2;
    R4 += 3;
    R5 += 4;
    R6 += 5;
    R7 += 6;

LC1 = P1;
NOP;
NOP;
LSETUP ( M5T , M5B ) LC1 = P0;
M5T:R2 += 1;
M5B:R3 += 2;
    R4 += 3;
    R5 += 4;
    R6 += 5;
    R7 += 6;

LSETUP ( M6T , M6B ) LC1 = P0;
NOP;
LC1 = P1;
M6T:R2 += 1;
    R3 += 2;
M6B:R4 += 3;
    R5 += 4;
    R6 += 5;
    R7 += 6;

LC1 = P1;
NOP;
LSETUP ( M7T , M7B ) LC1 = P0;
M7T:R2 += 1;
    R3 += 2;
M7B:R4 += 3;
    R5 += 4;
    R6 += 5;
    R7 += 6;

LSETUP ( M8T , M8B ) LC1 = P0;
LC1 = P1;
M8T:R2 += 1;
    R3 += 2;
    R4 += 3;
M8B:R5 += 4;
    R6 += 5;
    R7 += 6;

LC1 = P1;
LSETUP ( M9T , M9B ) LC1 = P0;
M9T:R2 += 1;
    R3 += 2;
    R4 += 3;
M9B:R5 += 4;
    R6 += 5;
    R7 += 6;

    // Loop 0
LSETUP ( N2T , N2B ) LC0 = P0 >> 1;
NOP;
NOP;
NOP;
LC0 = P1;
N2T:R2 += 1;
    R3 += 2;
    R4 += 3;
    R5 += 4;
    R6 += 5;
N2B:R7 += 6;

LC0 = P1;
NOP;
NOP;
NOP;
LSETUP ( N3T , N3B ) LC0 = P0 >> 1;
N3T:R2 += 1;
    R3 += 2;
    R4 += 3;
    R5 += 4;
    R6 += 5;
N3B:R7 += 6;

LSETUP ( N4T , N4B ) LC0 = P0 >> 1;
NOP;
NOP;
LC0 = P1;
N4T:R2 += 1;
    R3 += 2;
    R4 += 3;
    R5 += 4;
N4B:R6 += 5;
    R7 += 6;

LC0 = P1;
NOP;
NOP;
LSETUP ( N5T , N5B ) LC0 = P0 >> 1;
N5T:R2 += 1;
    R3 += 2;
    R4 += 3;
    R5 += 4;
N5B:R6 += 5;
    R7 += 6;

LSETUP ( N6T , N6B ) LC0 = P0 >> 1;
NOP;
LC0 = P1;
N6T:R2 += 1;
    R3 += 2;
    R4 += 3;
N6B:R5 += 4;
    R6 += 5;
    R7 += 6;

LC0 = P1;
NOP;
LSETUP ( N7T , N7B ) LC0 = P0 >> 1;
N7T:R2 += 1;
    R3 += 2;
    R4 += 3;
N7B:R5 += 4;
    R6 += 5;
    R7 += 6;

LSETUP ( N8T , N8T ) LC0 = P0 >> 1;
LC0 = P1;
N8T:R2 += 1;
    R3 += 2;
N8B:R4 += 3;
    R5 += 4;
    R6 += 5;
    R7 += 6;

LC0 = P1;
LSETUP ( N9T , N9T ) LC0 = P0 >> 1;
N9T:R2 += 1;
    R3 += 2;
N9B:R4 += 3;
    R5 += 4;
    R6 += 5;
    R7 += 6;


    //  Loop 1
LSETUP ( O2T , O2B ) LC1 = P0 >> 1;
NOP;
NOP;
NOP;
LC1 = P1;
O2T:R2 += 1;
    R3 += 2;
    R4 += 3;
    R5 += 4;
    R6 += 5;
O2B:R7 += 6;

LC1 = P1;
NOP;
NOP;
NOP;
LSETUP ( O3T , O3B ) LC1 = P0 >> 1;
O3T:R2 += 1;
    R3 += 2;
    R4 += 3;
    R5 += 4;
    R6 += 5;
O3B:R7 += 6;

LSETUP ( O4T , O4B ) LC1 = P0 >> 1;
NOP;
NOP;
LC1 = P1;
O4T:R2 += 1;
    R3 += 2;
    R4 += 3;
    R5 += 4;
O4B:R6 += 5;
    R7 += 6;

LC1 = P1;
NOP;
NOP;
LSETUP ( O5T , O5B ) LC1 = P0 >> 1;
O5T:R2 += 1;
    R3 += 2;
    R4 += 3;
    R5 += 4;
O5B:R6 += 5;
    R7 += 6;

LSETUP ( O6T , O6B ) LC1 = P0 >> 1;
NOP;
LC1 = P1;
O6T:R2 += 1;
    R3 += 2;
    R4 += 3;
O6B:R5 += 4;
    R6 += 5;
    R7 += 6;

LC1 = P1;
NOP;
LSETUP ( O7T , O7B ) LC1 = P0 >> 1;
O7T:R2 += 1;
    R3 += 2;
    R4 += 3;
O7B:R5 += 4;
    R6 += 5;
    R7 += 6;

LSETUP ( O8T , O8T ) LC1 = P0 >> 1;
LC1 = P1;
O8T:R2 += 1;
    R3 += 2;
O8B:R4 += 3;
    R5 += 4;
    R6 += 5;
    R7 += 6;

LC1 = P1;
LSETUP ( O9T , O9T ) LC1 = P0 >> 1;
O9T:R2 += 1;
    R3 += 2;
O9B:R4 += 3;
    R5 += 4;
    R6 += 5;
    R7 += 6;


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

NOP;
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
