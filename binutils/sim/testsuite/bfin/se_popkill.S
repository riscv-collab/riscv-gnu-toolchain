//Original:/proj/frio/dv/testcases/seq/se_popkill/se_popkill.dsp
// Description: Kill pops to sysregs in WB
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

/////////////////////////////////////////////////////////////////////////////
///////////////////////// Defines               /////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#ifndef USER_CODE_SPACE
#define USER_CODE_SPACE  CODE_ADDR_RST_1   //
#endif
#ifndef STACKSIZE
#define STACKSIZE        0x00000010
#endif
#ifndef ITABLE
#define ITABLE  CODE_ADDR_RST_2   //
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
CHECK_INIT_DEF(p5); //CHECK_INIT(p5,   0x00BFFFFC);

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

/////////////////////////////////////////////////////////////////////////////
//////////////////////// CPLB Setup     /////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

    // Setup CPLB for Data Memory starting at 0x00F0_0000;
WR_MMR(DCPLB_DATA0,     DATA_ADDR_1, p0, r0);
   //WR_MMR(DCPLB_DATA0,     0x00031005, p0, r0);    // Page Size    = 4MB
                                                    // CPLB_L1_CHLB = 1
                                                    // CPLB_USER_RD = 1
                                                    // CPLB_VALID   = 1
                                                    //

    // Setup CPLB Address to point to 0x00F0_0000
WR_MMR(DCPLB_ADDR0,     DATA_ADDR_2, p0, r0);
    //WR_MMR(DCPLB_ADDR0,     0x00F00000, p0, r0);

    // Enable CPLB's
WR_MMR(DMEM_CONTROL,    DATA_ADDR_3, p0, r0);
    //WR_MMR(DMEM_CONTROL,    0x0000000f, p0, r0);    // ENDM    = 1
                                                    // ENDCPLB = 1
                                                    // DMC     = 11
    // Sync it!
CSYNC;


    // Return to Supervisor Code
RAISE 15;

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

    R3 = SEQSTAT;
    R4 = RETX;
    R4 += 8;
RETX = R4;
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

NOP;
ASTAT = R7;
RETS = R7;
LC0 = R7;
LB0 = R7;
LT0 = R7;
LC1 = R7;
LB1 = R7;
LT1 = R7;
CYCLES = R7;
CYCLES2 = R7;
SYSCFG = R7;
RETN = R7;
RETX = R7;
RETE = R7;
NOP;
NOP;
NOP;
NOP;
NOP;
NOP;
NOP;
NOP;
EXCPT 1;
ASTAT = [ SP ++ ];
NOP;
NOP;
NOP;
NOP;
EXCPT 2;
RETS = [ SP ++ ];
NOP;
NOP;
NOP;
NOP;
EXCPT 3;
LC0 = [ SP ++ ];
NOP;
NOP;
NOP;
NOP;
EXCPT 4;
LT0 = [ SP ++ ];
NOP;
NOP;
NOP;
NOP;
EXCPT 5;
LB0 = [ SP ++ ];
NOP;
NOP;
NOP;
NOP;
EXCPT 6;
LC1 = [ SP ++ ];
NOP;
NOP;
NOP;
NOP;
EXCPT 7;
LB1 = [ SP ++ ];
NOP;
NOP;
NOP;
NOP;
EXCPT 8;
LT1 = [ SP ++ ];
NOP;
NOP;
NOP;
NOP;
EXCPT 9;
CYCLES = [ SP ++ ];
NOP;
NOP;
NOP;
NOP;
EXCPT 10;
CYCLES2 = [ SP ++ ];
NOP;
NOP;
NOP;
NOP;
EXCPT 11;
SYSCFG = [ SP ++ ];
NOP;
NOP;
NOP;
NOP;
EXCPT 12;
RETI = [ SP ++ ];
NOP;
NOP;
NOP;
NOP;
EXCPT 13;
RETX = [ SP ++ ];
NOP;
NOP;
NOP;
NOP;
EXCPT 14;
RETN = [ SP ++ ];
NOP;
NOP;
NOP;
NOP;
EXCPT 15;
RETE = [ SP ++ ];
NOP;
NOP;
NOP;
NOP;

NOP;
NOP;
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

// Define Kernal Stack
.section MEM_DATA_ADDR_1 //.data 0x00F00100,"aw"
    .space (STACKSIZE);
     KSTACK :

    .space (STACKSIZE);
     USTACK :

.section MEM_DATA_ADDR_2 //.data 0x00F00100,"aw"
.dd 0xdeadbeef;
.dd 0xdeadbeef;
.dd 0xdeadbeef;
.dd 0xdeadbeef;
.dd 0xdeadbeef;
.dd 0xdeadbeef;
.dd 0xdeadbeef;
.dd 0xdeadbeef;
.dd 0xdeadbeef;
.dd 0xdeadbeef;
.dd 0xdeadbeef;
.dd 0xdeadbeef;
.dd 0xdeadbeef;
.dd 0xdeadbeef;
.dd 0xdeadbeef;
.dd 0xdeadbeef;
.dd 0xdeadbeef;
.dd 0xdeadbeef;
.dd 0xdeadbeef;

/////////////////////////////////////////////////////////////////////////////
///////////////////////// END OF TEST           /////////////////////////////
/////////////////////////////////////////////////////////////////////////////
