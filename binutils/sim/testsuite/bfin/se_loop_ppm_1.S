//Original:/proj/frio/dv/testcases/seq/se_loop_ppm_1/se_loop_ppm_1.dsp
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

LSETUP ( l0s , l0s ) LC0 = P0;
    R0 += 1;
    R4 += 3;
    R5 += 5;
l0s:[ -- SP ] = ( R7:5 );

LSETUP ( l1s , l1e ) LC0 = P0;
    R0 += 1;
    R4 += 3;
    R5 += 5;
l1s:R5 += 1;
l1e:[ -- SP ] = ( R7:5 );

LSETUP ( l2s , l2e ) LC0 = P0;
    R0 += 1;
    R4 += 3;
    R5 += 5;
l2s:R5 += 1;
    R6 += 2;
l2e:[ -- SP ] = ( R7:5 );

LSETUP ( l3s , l3e ) LC0 = P0;
    R0 += 1;
    R4 += 3;
    R5 += 5;
l3s:R5 += 1;
    R6 += 2;
    R7 += 3;
l3e:[ -- SP ] = ( R7:5 );

LSETUP ( l4s , l4e ) LC0 = P0;
    R0 += 1;
    R4 += 3;
    R5 += 5;
l4s:R5 += 1;
    R6 += 2;
    R7 += 3;
    R4 += 4;
l4e:[ -- SP ] = ( R7:4 );

LSETUP ( l5s , l5e ) LC0 = P0;
    R0 += 1;
    R4 += 3;
    R5 += 5;
l5s:R5 += 1;
    R6 += 2;
    R7 += 3;
    R4 += 4;
    R5 += 3;
l5e:[ -- SP ] = ( R7:4 );

LSETUP ( l6s , l6e ) LC1 = P0;
    R0 += 1;
    R4 += 3;
    R5 += 5;
l6s:R5 += 1;
    R6 += 2;
    R7 += 3;
    R4 += 4;
    R5 += 3;
    R7 += 5;
l6e:[ -- SP ] = ( R7:4 );

NOP;

LSETUP ( m0s , m0s ) LC1 = P0;
    R0 += 1;
    R4 += 3;
    R5 += 5;
m0s:[ -- SP ] = ( R7:5 );

LSETUP ( m1s , m1e ) LC1 = P0;
    R0 += 1;
    R4 += 3;
    R5 += 5;
m1s:R5 += 1;
m1e:[ -- SP ] = ( R7:5 );

LSETUP ( m2s , m2e ) LC1 = P0;
    R0 += 1;
    R4 += 3;
    R5 += 5;
m2s:R5 += 1;
    R6 += 2;
m2e:[ -- SP ] = ( R7:5 );

LSETUP ( m3s , m3e ) LC1 = P0;
    R0 += 1;
    R4 += 3;
    R5 += 5;
m3s:R5 += 1;
    R6 += 2;
    R7 += 3;
m3e:[ -- SP ] = ( R7:5 );

LSETUP ( m4s , m4e ) LC1 = P0;
    R0 += 1;
    R4 += 3;
    R5 += 5;
m4s:R5 += 1;
    R6 += 2;
    R7 += 3;
    R4 += 4;
m4e:[ -- SP ] = ( R7:4 );

LSETUP ( m5s , m5e ) LC1 = P0;
    R0 += 1;
    R4 += 3;
    R5 += 5;
m5s:R5 += 1;
    R6 += 2;
    R7 += 3;
    R4 += 4;
    R5 += 3;
m5e:[ -- SP ] = ( R7:4 );

LSETUP ( m6s , m6e ) LC1 = P0;
    R0 += 1;
    R4 += 3;
    R5 += 5;
m6s:R5 += 1;
    R6 += 2;
    R7 += 3;
    R4 += 4;
    R5 += 3;
    R7 += 5;
m6e:[ -- SP ] = ( R7:4 );

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
