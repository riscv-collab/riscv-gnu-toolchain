//Original:/proj/frio/dv/testcases/seq/se_loop_mv2lt_stall/se_loop_mv2lt_stall.dsp
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

/////////////////////////////////////////////////////////////////////////////
///////////////////////// Defines               /////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#ifndef USER_CODE_SPACE
#define USER_CODE_SPACE  0x00000500
#endif
#ifndef STACKSIZE
#define STACKSIZE        0x00000010
#endif
#ifndef ITABLE
#define ITABLE           0xF0000000
#endif
#ifndef EVT
#define EVT              0xFFE02000
#endif
#ifndef EVT_OVERRIDE
#define EVT_OVERRIDE     0xFFE02100
#endif
#ifndef IMASK
#define IMASK            0xFFE02104
#endif
#ifndef DMEM_CONTROL
#define DMEM_CONTROL     0xFFE00004
#endif
#ifndef DCPLB_ADDR0
#define DCPLB_ADDR0      0xFFE00100
#endif
#ifndef DCPLB_DATA0
#define DCPLB_DATA0      0xFFE00200
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
CHECK_INIT(p5,   0x00BFFFFC);

    // Setup User Stack
LD32_LABEL(sp, USTACK);
USP = SP;

    // Setup Kernel Stack
LD32_LABEL(sp, KSTACK);

    // Setup Frame Pointer
FP = SP;

    // Setup Event Vector Table
LD32(p0, EVT);

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
    P2 = 0x0100 (Z);
    P2.H = 0x00f0;

    // Loop 0
LD32_LABEL(r0, L0T);
LD32_LABEL(r1, L0B);
LC0 = p1;
LB0 = r1;
    R5 = [ P2 ++ ];
LT0 = r0;
L0T:R3 += 4;
    R2 += 3;
    R4 += 5;
    R5 += 6;
    R6 += 7;
L0B:R7 += 8;

    // Loop 0
LD32_LABEL(r0, L1T);
LD32_LABEL(r1, L1B);
LB0 = r1;
LC0 = p1;
    R5 = [ P2 ++ ];
NOP;
LT0 = r0;
L1T:R4 += 5;
    R2 += 3;
    R3 += 4;
    R5 += 6;
    R6 += 7;
L1B:R7 += 8;

    // Loop 0
LD32_LABEL(r0, L2T);
LD32_LABEL(r1, L2B);
LB0 = r1;
LC0 = p1;
    R5 = [ P2 ++ ];
NOP;
NOP;
LT0 = r0;
L2T:R5 += 6;
    R2 += 3;
    R3 += 4;
    R4 += 5;
    R6 += 7;
L2B:R7 += 8;

    // Loop 0
LD32_LABEL(r0, L3T);
LD32_LABEL(r1, L3B);
LB0 = r1;
LC0 = p1;
    R5 = [ P2 ++ ];
NOP;
NOP;
NOP;
LT0 = r0;
L3T:R2 += 3;
    R5 += 6;
    R6 += 7;
    R3 += 4;
    R4 += 5;
L3B:R7 += 8;

    // Loop 0
LD32_LABEL(r0, L4T);
LD32_LABEL(r1, L4B);
LB0 = r1;
LC0 = p1;
    R5 = [ P2 ++ ];
NOP;
NOP;
NOP;
NOP;
LT0 = r0;
L4T:R2 += 3;
    R3 += 4;
    R5 += 6;
    R6 += 7;
    R4 += 5;
L4B:R7 += 8;

    // Loop 0
LD32_LABEL(r0, L5T);
LD32_LABEL(r1, L5B);
    [ -- SP ] = R0;
SSYNC;
LB0 = r1;
LC0 = p0;
    R5 = [ P2 ++ ];
LT0 = [sp++];
L5T:R2 += 3;
    R3 += 4;
    R5 += 6;
    R6 += 7;
    R4 += 5;
L5B:R7 += 8;


    // Loop 1
LD32_LABEL(r0, M0T);
LD32_LABEL(r1, M0B);
LB1 = r1;
LC1 = p1;
    R5 = [ P2 ++ ];
LT1 = r0;
M0T:R3 += 4;
    R2 += 3;
    R4 += 5;
    R5 += 6;
    R6 += 7;
M0B:R7 += 8;

    // Loop 1
LD32_LABEL(r0, M1T);
LD32_LABEL(r1, M1B);
LB1 = r1;
LC1 = p1;
    R5 = [ P2 ++ ];
NOP;
LT1 = r0;
M1T:R4 += 5;
    R2 += 3;
    R3 += 4;
    R5 += 6;
    R6 += 7;
M1B:R7 += 8;

    // Loop 1
LD32_LABEL(r0, M2T);
LD32_LABEL(r1, M2B);
LB1 = r1;
LC1 = p1;
    R5 = [ P2 ++ ];
NOP;
NOP;
LT1 = r0;
M2T:R5 += 6;
    R2 += 3;
    R3 += 4;
    R4 += 5;
    R6 += 7;
M2B:R7 += 8;

    // Loop 1
LD32_LABEL(r0, M3T);
LD32_LABEL(r1, M3B);
LB1 = r1;
LC1 = p1;
    R5 = [ P2 ++ ];
NOP;
NOP;
NOP;
LT1 = r0;
M3T:R2 += 3;
    R5 += 6;
    R6 += 7;
    R3 += 4;
    R4 += 5;
M3B:R7 += 8;

    // Loop 1
LD32_LABEL(r0, M4T);
LD32_LABEL(r1, M4B);
LB1 = r1;
LC1 = p1;
    R5 = [ P2 ++ ];
NOP;
NOP;
NOP;
NOP;
LT1 = r0;
M4T:R2 += 3;
    R3 += 4;
    R5 += 6;
    R6 += 7;
    R4 += 5;
M4B:R7 += 8;

    // Loop 1
LD32_LABEL(r0, M5T);
LD32_LABEL(r1, M5B);
    [ -- SP ] = R0;
SSYNC;
LB1 = r1;
LC1 = p0;
    R5 = [ P2 ++ ];
LT1 = [sp++];
M5T:R2 += 3;
    R3 += 4;
    R5 += 6;
    R6 += 7;
    R4 += 5;
M5B:R7 += 8;

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

.section MEM_0x00F00100,"aw"
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
.section MEM_0x00F00210,"aw"
    .space (STACKSIZE);
     KSTACK :

    .space (STACKSIZE);
     USTACK :

/////////////////////////////////////////////////////////////////////////////
///////////////////////// END OF TEST           /////////////////////////////
/////////////////////////////////////////////////////////////////////////////
