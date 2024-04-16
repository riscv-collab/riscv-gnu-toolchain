//Original:/proj/frio/dv/testcases/seq/se_loop_kill/se_loop_kill.dsp
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
    P2 = 0x0200 (Z);
    P2.H = 0x00F0;
    [ -- SP ] = P0;
    [ -- SP ] = P0;
SSYNC;

LD32_LABEL(r0, l0t);
LD32_LABEL(r1, l0b);
LT0 = r0;
LB0 = r1;
EXCPT 0x5;      // Will kill mv2lc in EX3
NOP;
LC0 = P0;
l0t:R3 += 3;
    R1 += 1;
    R4 += 4;
    R5 += 5;
    R6 += 6;
l0b:R2 += 2;

LD32_LABEL(r0, l2t);
LD32_LABEL(r1, l2b);
LT0 = r0;
LB0 = r1;
EXCPT 0x5;      // Will kill mv2lc in EX3 when stalled
LC0 = [ SP ++ ];
l2t:R3 += 3;
    R1 += 1;
    R4 += 4;
    R5 += 5;
    R6 += 6;
l2b:R2 += 2;

LD32_LABEL(r0, l1t);
LD32_LABEL(r1, l1b);
LT1 = r0;
LB1 = r1;
EXCPT 0x5;      // Will kill mv2lc in EX3 when stalled
LC1 = [ SP ++ ];
l1t:R3 += 3;
    R1 += 1;
    R4 += 4;
    R5 += 5;
    R6 += 6;
l1b:R2 += 2;

LD32_LABEL(r0, l3t);
LD32_LABEL(r1, l3b);
LT1 = r0;
LB1 = r1;
EXCPT 0x5;      // Will kill mv2lc in EX3
NOP;
LC1 = P0;
l3t:R3 += 3;
    R1 += 1;
    R4 += 4;
    R5 += 5;
    R6 += 6;
l3b:R2 += 2;

EXCPT 0x6;      // Will kill Lsetup in EX2
NOP;
NOP;
LSETUP ( l1e , l1e ) LC0 = P1;
l1e:R7 += 1;

EXCPT 0x6;      // Will kill Lsetup in EX2
NOP;
NOP;
LSETUP ( m1e , m1e ) LC1 = P1;
m1e:R7 += 1;

EXCPT 0x6;      // Will kill Lsetup in EX1
NOP;
NOP;
NOP;
LSETUP ( l2e , l2e ) LC0 = P1;
l2e:R7 += 1;

EXCPT 0x6;      // Will kill Lsetup in EX1
NOP;
NOP;
NOP;
LSETUP ( m2e , m2e ) LC1 = P1;
m2e:R7 += 1;

NOP;
NOP;
NOP;

EXCPT 0x6;      // Will kill Lsetup in EX2 when stalled
    R0 = [ P2 ++ ];
LSETUP ( l3e , l3e ) LC0 = P1;
l3e:R7 += 1;

EXCPT 0x6;      // Will kill Lsetup in EX2 when stalled
    R0 = [ P2 ++ ];
LSETUP ( m3e , m3e ) LC1 = P1;
m3e:R7 += 1;

EXCPT 0x6;      // Will kill Lsetup in EX1 when stalled
    R0 = [ P2 ++ ];
NOP;
LSETUP ( l4e , l4e ) LC0 = P1;
l4e:R7 += 1;

EXCPT 0x6;      // Will kill Lsetup in EX1 when stalled
    R0 = [ P2 ++ ];
NOP;
LSETUP ( m4e , m4e ) LC1 = P1;
m4e:R7 += 1;

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
.dd 0xdeadbeef;
.section MEM_0x00F00200,"aw"
.dd 0x01010101;
.dd 0x02020202;
.dd 0x03030303;
.dd 0x04040404;

// Define Kernal Stack
.section MEM_0x00F00210,"aw"
    .space (STACKSIZE);
     KSTACK :

    .space (STACKSIZE);
     USTACK :

/////////////////////////////////////////////////////////////////////////////
///////////////////////// END OF TEST           /////////////////////////////
/////////////////////////////////////////////////////////////////////////////
