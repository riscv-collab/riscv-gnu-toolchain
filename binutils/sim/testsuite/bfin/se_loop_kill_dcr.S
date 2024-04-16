//Original:/proj/frio/dv/testcases/seq/se_loop_kill_dcr/se_loop_kill_dcr.dsp
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

    P0 = 0x1 (Z);
    P1 = 0x2 (Z);
    P2 = 0x3 (Z);
    P3 = 0x4 (Z);
    P4 = 0x5 (Z);

/////////////////////////////////////////////////////////////////////////////
// Loop 0 (with Kill WB)
/////////////////////////////////////////////////////////////////////////////

    // Kill Valid Dcr in WB
LSETUP ( L0T , L0T ) LC0 = P0;
EXCPT 0x5;
L0T:R0 += 5;

    // Kill Valid Dcr in EX3
LSETUP ( L1T , L1B ) LC0 = P0;
EXCPT 0x5;
L1T:R0 += 5;
L1B:R1 += 4;

    // Kill Valid Dcr in EX2
LSETUP ( L2T , L2B ) LC0 = P0;
EXCPT 0x5;
L2T:R0 += 5;
    R1 += 4;
L2B:R2 += 3;

    // Kill Valid Dcr in EX1
LSETUP ( L3T , L3B ) LC0 = P0;
EXCPT 0x5;
L3T:R0 += 5;
    R1 += 4;
    R2 += 3;
L3B:R3 += 2;

    // Kill Valid Dcr in AC
LSETUP ( L4T , L4B ) LC0 = P0;
EXCPT 0x5;
L4T:R0 += 5;
    R1 += 4;
    R2 += 3;
    R3 += 2;
L4B:R4 += 1;

    // Kill Valid Dcr in WB, EX3
LSETUP ( L5T , L5T ) LC0 = P1;
EXCPT 0x5;
L5T:R1 += 5;

    // Kill Valid Dcr in EX3, EX2
LSETUP ( L6T , L6T ) LC0 = P1;
EXCPT 0x5;
NOP;
L6T:R2 += 5;

    // Kill Valid Dcr in EX2, EX1
LSETUP ( L7T , L7T ) LC0 = P1;
EXCPT 0x5;
NOP;
NOP;
L7T:R3 += 5;

    // Kill Valid Dcr in EX1, AC
LSETUP ( L8T , L8T ) LC0 = P1;
EXCPT 0x5;
NOP;
NOP;
NOP;
L8T:R4 += 5;

    // Kill Valid Dcr in WB, EX3, EX2
LSETUP ( L9T , L9T ) LC0 = P2;
EXCPT 0x5;
L9T:R5 += 5;

    // Kill Valid Dcr in EX3, EX2, EX1
LSETUP ( LAT , LAT ) LC0 = P2;
EXCPT 0x5;
NOP;
LAT:
	R6 += 6;

    // Kill Valid Dcr in EX2, EX1, AC
LSETUP ( LBT , LBT ) LC0 = P2;
EXCPT 0x5;
NOP;
NOP;
LBT:
	R5 += 5;

    // Kill Valid Dcr in WB, EX3, EX2, EX1
LSETUP ( LCT , LCT ) LC0 = P3;
EXCPT 0x5;
LCT:
	R7 += 7;

    // Kill Valid Dcr in EX3, EX2, EX1, AC
LSETUP ( LDT , LDT ) LC0 = P3;
EXCPT 0x5;
NOP;
LDT:
	R0 += 7;

    // Kill Valid Dcr in WB, EX3, EX2, EX1, AC
LSETUP ( LET , LET ) LC0 = P4;
EXCPT 0x5;
LET:
	R1 += 1;

    // Kill Valid Dcr in WB, EX2
LSETUP ( LFT , LFB ) LC0 = P1;
LFT:
	EXCPT 0x5;
LFB:
	R1 += 2;

    // Kill Valid Dcr in WB, EX1
LSETUP ( LGT , LGB ) LC0 = P1;
LGT:
	R2 += 3;
EXCPT 0x5;
LGB:
	R1 += 2;

    // Kill Valid Dcr in WB, AC
LSETUP ( LHT , LHB ) LC0 = P1;
LHT:
	R2 += 3;
    R3 += 4;
EXCPT 0x5;
LHB:
	R1 += 2;

    // Kill Valid Dcr in EX3, EX1
LSETUP ( LIT , LIB ) LC0 = P1;
EXCPT 0x5;
LIT:
	R2 += 1;
LIB:
	R1 += 2;

    // Kill Valid Dcr in EX3, AC
LSETUP ( LJT , LJB ) LC0 = P1;
LJT:
	EXCPT 0x5;
    R2 += 1;
LJB:
	R1 += 2;

    // Kill Valid Dcr in EX2, AC
LSETUP ( LKT , LKB ) LC0 = P1;
EXCPT 0x5;
NOP;
LKT:
	R2 += 1;
LKB:
	R1 += 2;

    // Kill Valid Dcr in WB, EX2, AC
LSETUP ( LLT , LLB ) LC0 = P2;
LLT:
	EXCPT 0x5;
LLB:
	R2 += 2;


/////////////////////////////////////////////////////////////////////////////
// Loop 1 (with Kill WB)
/////////////////////////////////////////////////////////////////////////////

    // Kill Valid Dcr in WB
LSETUP ( M0T , M0T ) LC1 = P0;
EXCPT 0x5;
M0T:R0 += 5;

    // Kill Valid Dcr in EX3
LSETUP ( M1T , M1B ) LC1 = P0;
EXCPT 0x5;
M1T:R0 += 5;
M1B:R1 += 4;

    // Kill Valid Dcr in EX2
LSETUP ( M2T , M2B ) LC1 = P0;
EXCPT 0x5;
M2T:R0 += 5;
    R1 += 4;
M2B:R2 += 3;

    // Kill Valid Dcr in EX1
LSETUP ( M3T , M3B ) LC1 = P0;
EXCPT 0x5;
M3T:R0 += 5;
    R1 += 4;
    R2 += 3;
M3B:R3 += 2;

    // Kill Valid Dcr in AC
LSETUP ( M4T , M4B ) LC1 = P0;
EXCPT 0x5;
M4T:R0 += 5;
    R1 += 4;
    R2 += 3;
    R3 += 2;
M4B:R4 += 1;

    // Kill Valid Dcr in WB, EX3
LSETUP ( M5T , M5T ) LC1 = P1;
EXCPT 0x5;
M5T:R1 += 5;

    // Kill Valid Dcr in EX3, EX2
LSETUP ( M6T , M6T ) LC1 = P1;
EXCPT 0x5;
NOP;
M6T:R2 += 5;

    // Kill Valid Dcr in EX2, EX1
LSETUP ( M7T , M7T ) LC1 = P1;
EXCPT 0x5;
NOP;
NOP;
M7T:R3 += 5;

    // Kill Valid Dcr in EX1, AC
LSETUP ( M8T , M8T ) LC1 = P1;
EXCPT 0x5;
NOP;
NOP;
NOP;
M8T:R4 += 5;

    // Kill Valid Dcr in WB, EX3, EX2
LSETUP ( M9T , M9T ) LC1 = P2;
EXCPT 0x5;
M9T:R5 += 5;

    // Kill Valid Dcr in EX3, EX2, EX1
LSETUP ( MAT , MAT ) LC1 = P2;
EXCPT 0x5;
NOP;
MAT:
	R6 += 6;

    // Kill Valid Dcr in EX2, EX1, AC
LSETUP ( MBT , MBT ) LC1 = P2;
EXCPT 0x5;
NOP;
NOP;
MBT:
	R5 += 5;

    // Kill Valid Dcr in WB, EX3, EX2, EX1
LSETUP ( MCT , MCT ) LC1 = P3;
EXCPT 0x5;
MCT:
	R7 += 7;

    // Kill Valid Dcr in EX3, EX2, EX1, AC
LSETUP ( MDT , MDT ) LC1 = P3;
EXCPT 0x5;
NOP;
MDT:
	R0 += 7;

    // Kill Valid Dcr in WB, EX3, EX2, EX1, AC
LSETUP ( MET , MET ) LC1 = P4;
EXCPT 0x5;
MET:
	R1 += 1;

    // Kill Valid Dcr in WB, EX2
LSETUP ( MFT , MFB ) LC1 = P1;
MFT:
	EXCPT 0x5;
MFB:
	R1 += 2;

    // Kill Valid Dcr in WB, EX1
LSETUP ( MGT , MGB ) LC1 = P1;
MGT:
	R2 += 3;
EXCPT 0x5;
MGB:
	R1 += 2;

    // Kill Valid Dcr in WB, AC
LSETUP ( MHT , MHB ) LC1 = P1;
MHT:
	R2 += 3;
    R3 += 4;
EXCPT 0x5;
MHB:
	R1 += 2;

    // Kill Valid Dcr in EX3, EX1
LSETUP ( MIT , MIB ) LC1 = P1;
EXCPT 0x5;
MIT:
	R2 += 1;
MIB:
	R1 += 2;

    // Kill Valid Dcr in EX3, AC
LSETUP ( MJT , MJB ) LC1 = P1;
MJT:
	EXCPT 0x5;
    R2 += 1;
MJB:
	R1 += 2;

    // Kill Valid Dcr in EX2, AC
LSETUP ( MKT , MKB ) LC1 = P1;
EXCPT 0x5;
NOP;
MKT:
	R2 += 1;
MKB:
	R1 += 2;

    // Kill Valid Dcr in WB, EX2, AC
LSETUP ( MLT , MLB ) LC1 = P2;
MLT:
	EXCPT 0x5;
MLB:
	R2 += 2;

/////////////////////////////////////////////////////////////////////////////
// Loop 0 (with Kill EX3)
/////////////////////////////////////////////////////////////////////////////

    // Kill Valid Dcr in EX3
LSETUP ( N1T , N1T ) LC0 = P0;
CSYNC;
N1T:R0 += 5;

    // Kill Valid Dcr in EX2
LSETUP ( N2T , N2B ) LC0 = P0;
CSYNC;
N2T:R0 += 5;
N2B:R2 += 3;

    // Kill Valid Dcr in EX1
LSETUP ( N3T , N3B ) LC0 = P0;
CSYNC;
N3T:R0 += 5;
    R2 += 3;
N3B:R3 += 2;

    // Kill Valid Dcr in AC
LSETUP ( N4T , N4B ) LC0 = P0;
CSYNC;
N4T:R0 += 5;
    R2 += 3;
    R3 += 2;
N4B:R4 += 1;

    // Kill Valid Dcr in EX3, EX2
LSETUP ( N6T , N6T ) LC0 = P1;
CSYNC;
N6T:R2 += 5;

    // Kill Valid Dcr in EX2, EX1
LSETUP ( N7T , N7T ) LC0 = P1;
CSYNC;
NOP;
N7T:R3 += 5;

    // Kill Valid Dcr in EX1, AC
LSETUP ( N8T , N8T ) LC0 = P1;
CSYNC;
NOP;
NOP;
N8T:R4 += 5;

    // Kill Valid Dcr in EX3, EX2, EX1
LSETUP ( NAT , NAT ) LC0 = P2;
CSYNC;
NAT:
	R6 += 6;

    // Kill Valid Dcr in EX2, EX1, AC
LSETUP ( NBT , NBT ) LC0 = P2;
CSYNC;
NOP;
NBT:
	R5 += 5;

    // Kill Valid Dcr in EX3, EX2, EX1, AC
LSETUP ( NDT , NDT ) LC0 = P3;
CSYNC;
NDT:
	R0 += 7;

    // Kill Valid Dcr in EX3, EX1
LSETUP ( NIT , NIB ) LC0 = P1;
NIT:
	CSYNC;
NIB:
	R1 += 2;

    // Kill Valid Dcr in EX3, AC
LSETUP ( NJT , NJB ) LC0 = P1;
NJT:
	R2 += 1;
CSYNC;
NJB:
	R1 += 2;

    // Kill Valid Dcr in EX2, AC
LSETUP ( NKT , NKB ) LC0 = P1;
CSYNC;
NKT:
	R2 += 1;
NKB:
	R1 += 2;

/////////////////////////////////////////////////////////////////////////////
// Loop 1 (with Kill EX3)
/////////////////////////////////////////////////////////////////////////////

    // Kill Valid Dcr in EX3
LSETUP ( O1T , O1T ) LC1 = P0;
CSYNC;
O1T:R0 += 5;

    // Kill Valid Dcr in EX2
LSETUP ( O2T , O2B ) LC1 = P0;
CSYNC;
O2T:R0 += 5;
O2B:R2 += 3;

    // Kill Valid Dcr in EX1
LSETUP ( O3T , O3B ) LC1 = P0;
CSYNC;
O3T:R0 += 5;
    R2 += 3;
O3B:R3 += 2;

    // Kill Valid Dcr in AC
LSETUP ( O4T , O4B ) LC1 = P0;
CSYNC;
O4T:R0 += 5;
    R2 += 3;
    R3 += 2;
O4B:R4 += 1;

    // Kill Valid Dcr in EX3, EX2
LSETUP ( O6T , O6T ) LC1 = P1;
CSYNC;
O6T:R2 += 5;

    // Kill Valid Dcr in EX2, EX1
LSETUP ( O7T , O7T ) LC1 = P1;
CSYNC;
NOP;
O7T:R3 += 5;

    // Kill Valid Dcr in EX1, AC
LSETUP ( O8T , O8T ) LC1 = P1;
CSYNC;
NOP;
NOP;
O8T:R4 += 5;

    // Kill Valid Dcr in EX3, EX2, EX1
LSETUP ( OAT , OAT ) LC1 = P2;
CSYNC;
OAT:
	R6 += 6;

    // Kill Valid Dcr in EX2, EX1, AC
LSETUP ( OBT , OBT ) LC1 = P2;
CSYNC;
NOP;
OBT:
	R5 += 5;

    // Kill Valid Dcr in EX3, EX2, EX1, AC
LSETUP ( ODT , ODT ) LC1 = P3;
CSYNC;
ODT:
	R0 += 7;

    // Kill Valid Dcr in EX3, EX1
LSETUP ( OIT , OIB ) LC1 = P1;
OIT:
	CSYNC;
OIB:
	R1 += 2;

    // Kill Valid Dcr in EX3, AC
LSETUP ( OJT , OJB ) LC1 = P1;
OJT:
	R2 += 1;
CSYNC;
OJB:
	R1 += 2;

    // Kill Valid Dcr in EX2, AC
LSETUP ( OKT , OKB ) LC1 = P1;
CSYNC;
OKT:
	R2 += 1;
OKB:
	R1 += 2;

/////////////////////////////////////////////////////////////////////////////
// Loop 0 (with Kill AC)
/////////////////////////////////////////////////////////////////////////////

    // Kill Valid Dcr in AC
LSETUP ( P4T , P4T ) LC0 = P0;
JUMP.S 2;
P4T:R0 += 5;

/////////////////////////////////////////////////////////////////////////////
// Loop 1 (with Kill AC)
/////////////////////////////////////////////////////////////////////////////

    // Kill Valid Dcr in AC
LSETUP ( Q4T , Q4T ) LC1 = P0;
JUMP.S 2;
Q4T:R0 += 5;

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
