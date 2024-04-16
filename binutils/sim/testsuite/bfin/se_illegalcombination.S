//Original:/proj/frio/dv/testcases/seq/se_illegalcombination/se_illegalcombination.dsp
// Description: Multi-issue Illegal Combinations
# mach: bfin
# sim: --environment operating
# xfail: "missing a few checks; hardware doesnt seem to match PRM?" *-*

#include "test.h"
.include "testutils.inc"
start

//
// Constants and Defines
//

include(gen_int.inc)
include(selfcheck.inc)
include(std.inc)
include(mmrs.inc)
include(symtable.inc)

#ifndef STACKSIZE
#define STACKSIZE 0x100   // change for how much stack you need
#endif
#ifndef ITABLE
#define ITABLE 0xF0000000
#endif

GEN_INT_INIT(ITABLE) // set location for interrupt table

//
// Reset/Bootstrap Code
//   (Here we should set the processor operating modes, initialize registers,
//    etc.)
//

BOOT:
INIT_R_REGS(0);     // initialize general purpose regs

INIT_P_REGS(0);     // initialize the pointers

INIT_I_REGS(0);     // initialize the dsp address regs
INIT_M_REGS(0);
INIT_L_REGS(0);
INIT_B_REGS(0);

CLI R1;           // inhibit events during MMR writes

LD32_LABEL(sp, USTACK);   // setup the user stack pointer
USP = SP;

LD32_LABEL(sp, KSTACK);   // setup the kernel stack pointer
FP = SP;        // and frame pointer

LD32(p0, EVT0);      // Setup Event Vectors and Handlers

    P0 += 4;            // EVT0 not used (Emulation)

    P0 += 4;            // EVT1 not used (Reset)

LD32_LABEL(r0, NHANDLE);  // NMI Handler (Int2)
    [ P0 ++ ] = R0;

LD32_LABEL(r0, XHANDLE);  // Exception Handler (Int3)
    [ P0 ++ ] = R0;

    P0 += 4;            // EVT4 not used (Global Interrupt Enable)

LD32_LABEL(r0, HWHANDLE); // HW Error Handler (Int5)
    [ P0 ++ ] = R0;

LD32_LABEL(r0, THANDLE);  // Timer Handler (Int6)
    [ P0 ++ ] = R0;

LD32_LABEL(r0, I7HANDLE); // IVG7 Handler
    [ P0 ++ ] = R0;

LD32_LABEL(r0, I8HANDLE); // IVG8 Handler
    [ P0 ++ ] = R0;

LD32_LABEL(r0, I9HANDLE); // IVG9 Handler
    [ P0 ++ ] = R0;

LD32_LABEL(r0, I10HANDLE);// IVG10 Handler
    [ P0 ++ ] = R0;

LD32_LABEL(r0, I11HANDLE);// IVG11 Handler
    [ P0 ++ ] = R0;

LD32_LABEL(r0, I12HANDLE);// IVG12 Handler
    [ P0 ++ ] = R0;

LD32_LABEL(r0, I13HANDLE);// IVG13 Handler
    [ P0 ++ ] = R0;

LD32_LABEL(r0, I14HANDLE);// IVG14 Handler
    [ P0 ++ ] = R0;

LD32_LABEL(r0, I15HANDLE);// IVG15 Handler
    [ P0 ++ ] = R0;

LD32(p0, EVT_OVERRIDE);
    R0 = 0;
    [ P0 ++ ] = R0;

    R1 = -1;     // Change this to mask interrupts (*)
CSYNC;       // wait for MMR writes to finish
STI R1;      // sync and reenable events (implicit write to IMASK)

DUMMY:

    A0 = 0;         // reset accumulators
    A1 = 0;

    R0 = 0 (Z);

LT0 = r0;       // set loop counters to something deterministic
LB0 = r0;
LC0 = r0;
LT1 = r0;
LB1 = r0;
LC1 = r0;

ASTAT = r0;     // reset other internal regs
SYSCFG = r0;
RETS = r0;      // prevent X's breaking LINK instruction

// The following code sets up the test for running in USER mode

LD32_LABEL(r0, STARTUSER);// One gets to user mode by doing a
                        // ReturnFromInterrupt (RTI)
RETI = r0;      // We need to load the return address

// Comment the following line for a USER Mode test

JUMP    STARTSUP;   // jump to code start for SUPERVISOR mode

RTI;

STARTSUP:
LD32_LABEL(p1, BEGIN);

LD32(p0, EVT15);

CLI R1;   // inhibit events during write to MMR
    [ P0 ] = P1;  // IVG15 (General) handler (Int 15) load with start
CSYNC;      // wait for it
STI R1;     // reenable events with proper imask

RAISE 15;       // after we RTI, INT 15 should be taken

RTI;

//
// The Main Program
//

STARTUSER:

LINK 0;     // change for how much stack frame space you need.

JUMP BEGIN;

//*********************************************************************

BEGIN:

                // COMMENT the following line for USER MODE tests
    [ -- SP ] = RETI;  // enable interrupts in supervisor mode

                // **** YOUR CODE GOES HERE ****

    // PUT YOUR TEST HERE!

// Slot 0 can only be LDST LOAD with search instruction (2 instrs)


 .dw 0xcc0d	//(R0,R1)=SEARCH R2(GT)||[P0]=R3||NOP;
 .dw 0x0210
 .dw 0x9303
 .dw 0x0000
// (r0,r1) = search r2 gt, nop, r3 = [i0]; // nop supposedly ok
( R0 , R1 ) = SEARCH R2 (GT) || R4 = [ P0 ++ P1 ] || NOP;

// only nop or dspLDST allowed in slot 1 (1 instr)

  // a0 = r0, nop, [p0] = r3;
  .dw 0xCC09;   // can't assemble
  .dw 0x2000;
  .dw 0x0000;
  .dw 0x9303;

// Slot 0 illegal opcodes (1 instr)

  // a0 = r0, raise 15, nop;
  .dw 0xCC09;   // can't assemble
  .dw 0x2000;
  .dw 0x009F;
  .dw 0x0000;

// multiissue with two stores (8 instrs)


 .dw 0xcc09	//A0=R0||W[P3]=R5.L||[I0]=R4;
 .dw 0x2000
 .dw 0x8b5b
 .dw 0x9f04

 .dw 0xcc09	//A0=R0||[I2]=R2||[I0]=R4;
 .dw 0x2000
 .dw 0x9f12
 .dw 0x9f04

 .dw 0xcc09	//A0=R0||[P3]=R0||[I0]=R4;
 .dw 0x2000
 .dw 0x9318
 .dw 0x9f04

 .dw 0xcc09	//A0=R0||[P3]=P0||[I0]=R4;
 .dw 0x2000
 .dw 0x9358
 .dw 0x9f04

 .dw 0xcc09	//A0=R0||[FP+-36]=R0||[I0]=R4;
 .dw 0x2000
 .dw 0xbb70
 .dw 0x9f04

 .dw 0xcc09	//A0=R0||[FP+-48]=P0||[I0]=R4;
 .dw 0x2000
 .dw 0xbb48
 .dw 0x9f04

 .dw 0xcc09	//A0=R0||[P3+0x20]=R1||[I0]=R4;
 .dw 0x2000
 .dw 0xb219
 .dw 0x9f04

 .dw 0xcc09	//A0=R0||[P3+0x20]=P1||[I0]=R4;
 .dw 0x2000
 .dw 0xbe19
 .dw 0x9f04

// multiissue two instructions can't modify same ireg (6 instrs)


 .dw 0xcc09	//A0=R0||I0+=M1(BREV)||R1.L=W[I0++];
 .dw 0x2000
 .dw 0x9ee4
 .dw 0x9c21

 .dw 0xcc09	//A0=R0||I1-=M3||R0=[I1++M3];
 .dw 0x2000
 .dw 0x9e7d
 .dw 0x9de8

 .dw 0xcc09	//A0=R0||I2+=2||W[I2++]=R0.L;
 .dw 0x2000
 .dw 0x9f62
 .dw 0x9e30

 .dw 0xcc09	//A0=R0||I3-=4||[I3++M1]=R7;
 .dw 0x2000
 .dw 0x9f6f
 .dw 0x9fbf

 .dw 0xcc09	//A0=R0||R1.L=W[I1++]||W[I1++]=R2.L;
 .dw 0x2000
 .dw 0x9c29
 .dw 0x9e2a

 .dw 0xcc09	//A0=R0||[I2++M3]=R7||R6=[I2++M0];
 .dw 0x2000
 .dw 0x9ff7
 .dw 0x9d96

// multiissue two instructions can't load same dreg (9 instrs)


 .dw 0xcc09	//A0=R0||R0.L=W[P0++P2]||R0=[I0++];
 .dw 0x2000
 .dw 0x8210
 .dw 0x9c00

 .dw 0xcc09	//A0=R0||R1=W[P0++P3](X)||R1.L=W[I2];
 .dw 0x2000
 .dw 0x8e58
 .dw 0x9d31

 .dw 0xcc09	//A0=R0||R2=W[P0++P3](X)||R2=[I1++M3];
 .dw 0x2000
 .dw 0x8e98
 .dw 0x9dea

 .dw 0xcc09	//A0=R0||R3=[I0++]||R3=[I1++];
 .dw 0x2000
 .dw 0x9c03
 .dw 0x9c0b

 .dw 0xcc09	//A0=R0||R4.L=W[I2]||R4.L=W[I3];
 .dw 0x2000
 .dw 0x9d34
 .dw 0x9d3c

 .dw 0xcc09	//A0=R0||R5=[I1++M3]||R5.L=W[I2++];
 .dw 0x2000
 .dw 0x9ded
 .dw 0x9c35

 .dw 0xcc09	//A0=R0||R6=[P0]||R6=[I0++];
 .dw 0x2000
 .dw 0x9106
 .dw 0x9c06

 .dw 0xcc09	//A0=R0||R7=[FP+-56]||R7.L=W[I1];
 .dw 0x2000
 .dw 0xb927
 .dw 0x9d2f

 .dw 0xcc09	//A0=R0||R0=W[P1+0x1e](X)||R0=[I0++];
 .dw 0x2000
 .dw 0xabc8
 .dw 0x9c00

// dsp32alu instructions with one dest and slot 0 multi with same dest (1 ins)


 .dw 0xcc00	//R0=R2+|+R3||R0=W[P1+0x1e](X)||NOP;
 .dw 0x0013
 .dw 0xabc8
 .dw 0x0000
        // other slot 0 dreg cases already covered

// dsp32alu one dest and slot 1 multi with same dest (1 ins)


 .dw 0xcc18	//R1=BYTEPACK(R4,R5)||NOP||R1.L=W[I2];
 .dw 0x0225
 .dw 0x0000
 .dw 0x9d31
        // other slot 1 dreg dest cases already covered

// dsp32alu dual dests and slot 0 multi with either same dest (2 instrs)


 .dw 0xcc18	//(R2,R3)=BYTEUNPACKR1:0||R2=W[P0++P3](X)||NOP;
 .dw 0x4680
 .dw 0x8e98
 .dw 0x0000

 .dw 0xcc01	//R2=R2+|+R3,R3=R2-|-R3||R3=[P3]||NOP;
 .dw 0x0693
 .dw 0x911b
 .dw 0x0000

// dsp32alu dual dests and slot 1 multi with either same dest (2 instrs)


 .dw 0xcc18	//(R4,R5)=BYTEUNPACKR1:0||NOP||R4=[I1++M3];
 .dw 0x4b00
 .dw 0x0000
 .dw 0x9dec

 .dw 0xcc01	//R4=R2+|+R3,R5=R2-|-R3||NOP||R5.L=W[I2++];
 .dw 0x0b13
 .dw 0x0000
 .dw 0x9c35

// dsp32shift one dest and slot 0 multi with same dest (1 instruction)


 .dw 0xce0d	//R6=ALIGN8(R4,R5)||R6=[P0]||NOP;
 .dw 0x0c2c
 .dw 0x9106
 .dw 0x0000

// dsp32shift one dest and slot 1 multi with same dest (1 instruction)


 .dw 0xce00	//R7.L=ASHIFTR0.HBYR7.L||NOP||R7.L=W[I1];
 .dw 0x1e38
 .dw 0x0000
 .dw 0x9d2f

// dsp32shift two dests and slot 0 multi with either same dest (2 instrs)


 .dw 0xce08	//BITMUX(R0,R1,A0)(ASR)||R0.L=W[P0++P2]||NOP;
 .dw 0x0001
 .dw 0x8210
 .dw 0x0000

 .dw 0xce08	//BITMUX(R2,R3,A0)(ASL)||R3=[I0++]||NOP;
 .dw 0x4013
 .dw 0x9c03
 .dw 0x0000

// dsp32shift two dests and slot 1 multi with either same dest (2 instrs)


 .dw 0xce08	//BITMUX(R4,R5,A0)(ASR)||NOP||R4.H=W[I3];
 .dw 0x0025
 .dw 0x0000
 .dw 0x9d5c

 .dw 0xce08	//BITMUX(R6,R7,A0)(ASL)||NOP||R7.L=W[I1];
 .dw 0x4037
 .dw 0x0000
 .dw 0x9d2f

// dsp32shiftimm one dest and slot 0 with same dest (1 instr)


 .dw 0xce80	//R1.L=R0.H<<0x7||R1=W[P0++P3](X)||NOP;
 .dw 0x1238
 .dw 0x8e58
 .dw 0x0000

// dsp32shiftimm one dest and slot 1 with same dest (1 instr)


 .dw 0xce81	//R5=R2<<0x9(V)||NOP||R5.L=W[I2++];
 .dw 0x0a4a
 .dw 0x0000
 .dw 0x9c35

// dsp32mac one dest and slot 0 multi with same dest (1 inst)


 .dw 0xc805	//A0+=R1.H*R0.L,R6.H=(A1+=R1.L*R0.H)||R6=W[P0++P3](X)||NOP;
 .dw 0x4d88
 .dw 0x8f98
 .dw 0x0000

// dsp32mult one dest and slot 0 multi with same dest (1 inst)


 .dw 0xca04	//R7.H=R3.L*R4.H||R7=[FP+-56]||NOP;
 .dw 0x41dc
 .dw 0xb927
 .dw 0x0000

// dsp32 mac one dest and slot 1 multi with same dest (1 inst)


 .dw 0xc805	//A0+=R1.H*R0.L,R0.H=(A1+=R1.L*R0.H)||NOP||R0=[I0++];
 .dw 0x4c08
 .dw 0x0000
 .dw 0x9c00

// dsp32mult one dest and slot 1 multi with same dest (1 inst)


 .dw 0xca04	//R1.H=R3.L*R4.H||NOP||R1.H=W[I1];
 .dw 0x405c
 .dw 0x0000
 .dw 0x9d49

// dsp32mac write to register pair and slot 0 same dest - even (1 instr)


 .dw 0xc80d	//R3=(A1+=R1.L*R0.H),R2=(A0+=R1.H*R0.L)||R2=W[P0++P3](X)||NOP;
 .dw 0x6c88
 .dw 0x8e98
 .dw 0x0000

// dsp32mult write to register pair and slot 0 same dest - even (1 instr)


 .dw 0xca0c	//R5=R1.L*R0.H,R4=R1.H*R0.L||R4=[P0++P1]||NOP;
 .dw 0x6508
 .dw 0x8108
 .dw 0x0000

// dsp32mac write to register pair and slot 1 same dest - even (1 instr)


 .dw 0xc80d	//R3=(A1+=R1.L*R0.H),R2=(A0+=R1.H*R0.L)||NOP||R2=[I1++M3];
 .dw 0x6c88
 .dw 0x0000
 .dw 0x9dea

// dsp32mult write to register pair and slot 1 same dest - even (1 instr)


 .dw 0xca0c	//R5=R1.L*R0.H,R4=R1.H*R0.L||NOP||R4=[I1++M3];
 .dw 0x6508
 .dw 0x0000
 .dw 0x9dec

// dsp32mac write to register pair and slot 0 same dest - odd (1 instr)


 .dw 0xc80d	//A0+=R1.H*R0.L,R3=(A1+=R1.L*R0.H)||R3=W[P0++P3](X)||NOP;
 .dw 0x4c88
 .dw 0x8ed8
 .dw 0x0000

// dsp32mult write to register pair and slot 0 same dest - odd (1 instr)


 .dw 0xca0c	//R5=R1.L*R0.H,R4=R1.H*R0.L||R5=[P0++P1]||NOP;
 .dw 0x6508
 .dw 0x8148
 .dw 0x0000

// dsp32mac write to register pair and slot 1 same dest - odd (1 instr)


 .dw 0xc80d	//A0+=R1.H*R0.L,R3=(A1+=R1.L*R0.H)||NOP||R3=[I1++M3];
 .dw 0x4c88
 .dw 0x0000
 .dw 0x9deb

// dsp32mult write to register pair and slot 1 same dest - odd (1 instr)


 .dw 0xca0c	//R5=R1.L*R0.H,R4=R1.H*R0.L||NOP||R5=[I1++M3];
 .dw 0x6508
 .dw 0x0000
 .dw 0x9ded

// CHECKER

CHECK_INIT_DEF(p0); //CHECK_INIT(p0, 0xFF7FFFFC);
                    // Xhandler counts all EXCAUSE = 0x22;
CHECKREG(r5, 53); // count of all Illegal Combination Exceptions.

END:
dbg_pass;            // End the test

//*********************************************************************

//
// Handlers for Events
//

NHANDLE:            // NMI Handler 2
RTN;

XHANDLE:            // Exception Handler 3
                    // 16 bit illegal opcode handler - skips bad instruction

    [ -- SP ] = ASTAT; // save what we damage
    [ -- SP ] = ( R7:6 );
    R7 = SEQSTAT;
    R7 <<= 26;
    R7 >>= 26;      // only want EXCAUSE
    R6 = 0x22;      // EXCAUSE 0x22 means I-Fetch Undefined Instruction
CC = r7 == r6;
IF CC JUMP ILLEGALCOMBINATION;   // If EXCAUSE != 0x22 then leave

dbg_fail;
JUMP.S OUT;       // if the EXCAUSE is wrong the test will infinite loop

ILLEGALCOMBINATION:
    R7 = RETX;      // Fix up return address

    R7 += 8;        // skip offending 64 bit instruction

RETX = r7;      // and put back in RETX

    R5 += 1;        // Increment global counter

OUT:
    ( R7:6 ) = [ SP ++ ];
ASTAT = [sp++];

RTX;

HWHANDLE:           // HW Error Handler 5
RTI;

THANDLE:            // Timer Handler 6
RTI;

I7HANDLE:           // IVG 7 Handler
RTI;

I8HANDLE:           // IVG 8 Handler
RTI;

I9HANDLE:           // IVG 9 Handler
RTI;

I10HANDLE:          // IVG 10 Handler
RTI;

I11HANDLE:          // IVG 11 Handler
RTI;

I12HANDLE:          // IVG 12 Handler
RTI;

I13HANDLE:          // IVG 13 Handler
RTI;

I14HANDLE:          // IVG 14 Handler
RTI;

I15HANDLE:          // IVG 15 Handler
RTI;


    // padding for the icache

EXCPT 0; EXCPT 0; EXCPT 0; EXCPT 0; EXCPT 0; EXCPT 0; EXCPT 0;

//
// Data Segment
//

.data
DATA:
    .space (0x10);

// Stack Segments (Both Kernel and User)

    .space (STACKSIZE);
KSTACK:

    .space (STACKSIZE);
USTACK:
