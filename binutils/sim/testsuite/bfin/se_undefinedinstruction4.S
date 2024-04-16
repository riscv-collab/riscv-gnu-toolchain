//Original:/proj/frio/dv/testcases/seq/se_undefinedinstruction4/se_undefinedinstruction4.dsp
// Description: 64 bit special cases Undefined Instructions in Supervisor Mode
# mach: bfin
# sim: --environment operating

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
#define STACKSIZE 0x10   // change for how much stack you need
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
// Starting 64bit section  COUNT = 6406
    .dw 0xCF00 ;
    .dw 0xFA4D ;
    .dw 0x4973 ;
    .dw 0x434D ;
    .dw 0xCF01 ;
    .dw 0x3CAF ;
    .dw 0xE7F7 ;
    .dw 0xACAD ;
    .dw 0xCF02 ;
    .dw 0xC9A3 ;
    .dw 0x705D ;
    .dw 0x8EFF ;
    .dw 0xCF03 ;
    .dw 0x242D ;
    .dw 0x26ED ;
    .dw 0x1C67 ;
    .dw 0xCF04 ;
    .dw 0xBC83 ;
    .dw 0x18BB ;
    .dw 0xEF95 ;
    .dw 0xCF05 ;
    .dw 0xDFA7 ;
    .dw 0x6AD9 ;
    .dw 0x7FEF ;
    .dw 0xCF06 ;
    .dw 0x71F3 ;
    .dw 0x19CB ;
    .dw 0x1F69 ;
    .dw 0xCF07 ;
    .dw 0xA117 ;
    .dw 0x23ED ;
    .dw 0xE509 ;
    .dw 0xCF08 ;
    .dw 0x4DF9 ;
    .dw 0x31C3 ;
    .dw 0x5207 ;
    .dw 0xCF09 ;
    .dw 0xF35D ;
    .dw 0x998F ;
    .dw 0xC1A7 ;
    .dw 0xCF0A ;
    .dw 0xA7FF ;
    .dw 0x73D ;
    .dw 0x4ACB ;
    .dw 0xCF0B ;
    .dw 0xEE29 ;
    .dw 0xAAE7 ;
    .dw 0x3FD3 ;
    .dw 0xCF0C ;
    .dw 0xD3B5 ;
    .dw 0x5549 ;
    .dw 0xBCB7 ;
    .dw 0xCF0D ;
    .dw 0xF0B7 ;
    .dw 0xB91B ;
    .dw 0xC01F ;
    .dw 0xCF0E ;
    .dw 0xC169 ;
    .dw 0x3D1F ;
    .dw 0xB96B ;
    .dw 0xCF0F ;
    .dw 0x7CD3 ;
    .dw 0xFD95 ;
    .dw 0x2EA1 ;
    .dw 0xCF10 ;
    .dw 0x8907 ;
    .dw 0x6013 ;
    .dw 0x467D ;
    .dw 0xCF11 ;
    .dw 0x7F67 ;
    .dw 0xFC1F ;
    .dw 0x6611 ;
    .dw 0xCF12 ;
    .dw 0x1BB3 ;
    .dw 0xCFE1 ;
    .dw 0xF609 ;
    .dw 0xCF13 ;
    .dw 0x6AF1 ;
    .dw 0xC229 ;
    .dw 0x8009 ;
    .dw 0xCF14 ;
    .dw 0xF619 ;
    .dw 0xF2C9 ;
    .dw 0xF8C7 ;
    .dw 0xCF15 ;
    .dw 0xE413 ;
    .dw 0x99F3 ;
    .dw 0x7919 ;
    .dw 0xCF16 ;
    .dw 0x5E8B ;
    .dw 0xCA1 ;
    .dw 0xED71 ;
    .dw 0xCF17 ;
    .dw 0x3FBB ;
    .dw 0x221B ;
    .dw 0xDA89 ;
    .dw 0xCF18 ;
    .dw 0xDFED ;
    .dw 0x1565 ;
    .dw 0x12DB ;
    .dw 0xCF19 ;
    .dw 0x95FD ;
    .dw 0xB71F ;
    .dw 0xB9B ;
    .dw 0xCF1A ;
    .dw 0xAB8F ;
    .dw 0xC14F ;
    .dw 0xD777 ;
    .dw 0xCF1B ;
    .dw 0x9427 ;
    .dw 0x2E69 ;
    .dw 0x5F23 ;
    .dw 0xCF1C ;
    .dw 0xB9F1 ;
    .dw 0xFE17 ;
    .dw 0x6AA1 ;
    .dw 0xCF1D ;
    .dw 0x642B ;
    .dw 0x676B ;
    .dw 0xCA2B ;
    .dw 0xCF1E ;
    .dw 0x4399 ;
    .dw 0x8C55 ;
    .dw 0x5187 ;
    .dw 0xCF1F ;
    .dw 0xCED5 ;
    .dw 0x9163 ;
    .dw 0x4B95 ;
    .dw 0xCF20 ;
    .dw 0xE0F9 ;
    .dw 0xA3AF ;
    .dw 0x72EB ;
    .dw 0xCF21 ;
    .dw 0x120B ;
    .dw 0x9161 ;
    .dw 0x4C73 ;
    .dw 0xCF22 ;
    .dw 0xA97F ;
    .dw 0x9BC3 ;
    .dw 0xF2A9 ;
    .dw 0xCF23 ;
    .dw 0x9B6F ;
    .dw 0x15F5 ;
    .dw 0x83F3 ;
    .dw 0xCF24 ;
    .dw 0x67D3 ;
    .dw 0x4385 ;
    .dw 0xEF37 ;
    .dw 0xCF25 ;
    .dw 0xD3A3 ;
    .dw 0xFB5B ;
    .dw 0x119D ;
    .dw 0xCF26 ;
    .dw 0xCA67 ;
    .dw 0xC3F5 ;
    .dw 0x2109 ;
    .dw 0xCF27 ;
    .dw 0x459B ;
    .dw 0xC69 ;
    .dw 0x6BD3 ;
    .dw 0xCF28 ;
    .dw 0xBD4B ;
    .dw 0x82E1 ;
    .dw 0xDD07 ;
    .dw 0xCF29 ;
    .dw 0x9131 ;
    .dw 0x4A0B ;
    .dw 0x503B ;
    .dw 0xCF2A ;
    .dw 0x3383 ;
    .dw 0x55B5 ;
    .dw 0x7107 ;
    .dw 0xCF2B ;
    .dw 0x9F5D ;
    .dw 0x14B3 ;
    .dw 0xF6FF ;
    .dw 0xCF2C ;
    .dw 0xF3B1 ;
    .dw 0x53DF ;
    .dw 0x9A93 ;
    .dw 0xCF2D ;
    .dw 0x5A59 ;
    .dw 0x3879 ;
    .dw 0x41AD ;
    .dw 0xCF2E ;
    .dw 0xDD63 ;
    .dw 0x9BEF ;
    .dw 0x55B3 ;
    .dw 0xCF2F ;
    .dw 0x9B01 ;
    .dw 0x563D ;
    .dw 0x598B ;
    .dw 0xCF30 ;
    .dw 0xF1E3 ;
    .dw 0x45E1 ;
    .dw 0xD327 ;
    .dw 0xCF31 ;
    .dw 0xF0C7 ;
    .dw 0xD19D ;
    .dw 0x110D ;
    .dw 0xCF32 ;
    .dw 0x94B7 ;
    .dw 0x68CF ;
    .dw 0x6ADB ;
    .dw 0xCF33 ;
    .dw 0x4083 ;
    .dw 0xAD23 ;
    .dw 0x3F8B ;
    .dw 0xCF34 ;
    .dw 0x55D3 ;
    .dw 0x6969 ;
    .dw 0x38D9 ;
    .dw 0xCF35 ;
    .dw 0xD261 ;
    .dw 0xF353 ;
    .dw 0x1595 ;
    .dw 0xCF36 ;
    .dw 0x8897 ;
    .dw 0x9A6D ;
    .dw 0x2093 ;
    .dw 0xCF37 ;
    .dw 0x2673 ;
    .dw 0xD509 ;
    .dw 0xF435 ;
    .dw 0xCF38 ;
    .dw 0x5093 ;
    .dw 0x6F8F ;
    .dw 0x93D9 ;
    .dw 0xCF39 ;
    .dw 0xAAE1 ;
    .dw 0xE2F1 ;
    .dw 0x807F ;
    .dw 0xCF3A ;
    .dw 0x64D ;
    .dw 0xFEF7 ;
    .dw 0x103D ;
    .dw 0xCF3B ;
    .dw 0x1665 ;
    .dw 0x1959 ;
    .dw 0x608F ;
    .dw 0xCF3C ;
    .dw 0x43D9 ;
    .dw 0x2CDD ;
    .dw 0x2F3F ;
    .dw 0xCF3D ;
    .dw 0x950B ;
    .dw 0x3B49 ;
    .dw 0x2681 ;
    .dw 0xCF3E ;
    .dw 0xEA9D ;
    .dw 0x8053 ;
    .dw 0xC311 ;
    .dw 0xCF3F ;
    .dw 0x4D3 ;
    .dw 0x9311 ;
    .dw 0x498B ;
    .dw 0xCF40 ;
    .dw 0x6909 ;
    .dw 0x27C3 ;
    .dw 0x2B45 ;
    .dw 0xCF41 ;
    .dw 0x1347 ;
    .dw 0xFC37 ;
    .dw 0x8C9D ;
    .dw 0xCF42 ;
    .dw 0xD08F ;
    .dw 0xFF4B ;
    .dw 0x3223 ;
    .dw 0xCF43 ;
    .dw 0x485 ;
    .dw 0x7C05 ;
    .dw 0xB5BB ;
    .dw 0xCF44 ;
    .dw 0x49BB ;
    .dw 0x5A71 ;
    .dw 0xBD1B ;
    .dw 0xCF45 ;
    .dw 0x27D9 ;
    .dw 0x39B ;
    .dw 0xE099 ;
    .dw 0xCF46 ;
    .dw 0x85AF ;
    .dw 0xC637 ;
    .dw 0xC7EF ;
    .dw 0xCF47 ;
    .dw 0x5D7B ;
    .dw 0x9FAF ;
    .dw 0xE277 ;
    .dw 0xCF48 ;
    .dw 0x51C9 ;
    .dw 0xD04B ;
    .dw 0xE427 ;
    .dw 0xCF49 ;
    .dw 0x747B ;
    .dw 0xB7F5 ;
    .dw 0x4E5 ;
    .dw 0xCF4A ;
    .dw 0xCBDF ;
    .dw 0xFB21 ;
    .dw 0x2B5B ;
    .dw 0xCF4B ;
    .dw 0x6F59 ;
    .dw 0x716D ;
    .dw 0xB07B ;
    .dw 0xCF4C ;
    .dw 0x42CB ;
    .dw 0x46CB ;
    .dw 0x9CD5 ;
    .dw 0xCF4D ;
    .dw 0xC98B ;
    .dw 0x2C5D ;
    .dw 0x57FF ;
    .dw 0xCF4E ;
    .dw 0xF097 ;
    .dw 0xF96D ;
    .dw 0x9C45 ;
    .dw 0xCF4F ;
    .dw 0x8743 ;
    .dw 0xD053 ;
    .dw 0xF01F ;
    .dw 0xCF50 ;
    .dw 0xD12D ;
    .dw 0x79ED ;
    .dw 0x18D7 ;
    .dw 0xCF51 ;
    .dw 0xCB3 ;
    .dw 0x860F ;
    .dw 0x5F57 ;
    .dw 0xCF52 ;
    .dw 0x41B7 ;
    .dw 0xFB03 ;
    .dw 0x2985 ;
    .dw 0xCF53 ;
    .dw 0x514F ;
    .dw 0x6F ;
    .dw 0x74F1 ;
    .dw 0xCF54 ;
    .dw 0x32AF ;
    .dw 0x4413 ;
    .dw 0x4F1 ;
    .dw 0xCF55 ;
    .dw 0xDF13 ;
    .dw 0xEB77 ;
    .dw 0xFDC7 ;
    .dw 0xCF56 ;
    .dw 0xE7BF ;
    .dw 0xF8FB ;
    .dw 0x8881 ;
    .dw 0xCF57 ;
    .dw 0xD71 ;
    .dw 0xE18B ;
    .dw 0x58E1 ;
    .dw 0xCF58 ;
    .dw 0xE66B ;
    .dw 0x396B ;
    .dw 0x6441 ;
    .dw 0xCF59 ;
    .dw 0xEAE5 ;
    .dw 0xC4B9 ;
    .dw 0x5D65 ;
    .dw 0xCF5A ;
    .dw 0x2DA9 ;
    .dw 0x2BBB ;
    .dw 0xD621 ;
    .dw 0xCF5B ;
    .dw 0x2FD1 ;
    .dw 0xEB81 ;
    .dw 0x56F3 ;
    .dw 0xCF5C ;
    .dw 0x7E67 ;
    .dw 0xE6E1 ;
    .dw 0x907 ;
    .dw 0xCF5D ;
    .dw 0x40A3 ;
    .dw 0x95B3 ;
    .dw 0x3501 ;
    .dw 0xCF5E ;
    .dw 0xBE25 ;
    .dw 0x12A5 ;
    .dw 0x96D ;
    .dw 0xCF5F ;
    .dw 0x94C9 ;
    .dw 0xF7F7 ;
    .dw 0xA553 ;
    .dw 0xCF60 ;
    .dw 0xB291 ;
    .dw 0x5C7D ;
    .dw 0x32ED ;
    .dw 0xCF61 ;
    .dw 0xABB5 ;
    .dw 0x3987 ;
    .dw 0x90FB ;
    .dw 0xCF62 ;
    .dw 0xDE61 ;
    .dw 0x6B43 ;
    .dw 0x5F83 ;
    .dw 0xCF63 ;
    .dw 0xF03D ;
    .dw 0x61AF ;
    .dw 0x3713 ;
    .dw 0xCF64 ;
    .dw 0x854D ;
    .dw 0x2B4B ;
    .dw 0x5ACB ;
    .dw 0xCF65 ;
    .dw 0x669B ;
    .dw 0xC7A9 ;
    .dw 0xC7B5 ;
    .dw 0xCF66 ;
    .dw 0x2E5D ;
    .dw 0xFFE5 ;
    .dw 0x8929 ;
    .dw 0xCF67 ;
    .dw 0xA089 ;
    .dw 0x8151 ;
    .dw 0xCD41 ;
    .dw 0xCF68 ;
    .dw 0xC17F ;
    .dw 0x7ECF ;
    .dw 0xB3F9 ;
    .dw 0xCF69 ;
    .dw 0x1689 ;
    .dw 0xEA61 ;
    .dw 0xC17B ;
    .dw 0xCF6A ;
    .dw 0xF6A1 ;
    .dw 0xB5D1 ;
    .dw 0xE1D5 ;
    .dw 0xCF6B ;
    .dw 0x8CEB ;
    .dw 0xFA5 ;
    .dw 0xBF9B ;
    .dw 0xCF6C ;
    .dw 0x9A11 ;
    .dw 0x79DB ;
    .dw 0x6B09 ;
    .dw 0xCF6D ;
    .dw 0x769B ;
    .dw 0xEED1 ;
    .dw 0x3BE3 ;
    .dw 0xCF6E ;
    .dw 0x8B95 ;
    .dw 0xC2E9 ;
    .dw 0x782D ;
    .dw 0xCF6F ;
    .dw 0x3763 ;
    .dw 0x756B ;
    .dw 0xE4B1 ;
    .dw 0xCF70 ;
    .dw 0xB2F5 ;
    .dw 0x7F09 ;
    .dw 0x2A1B ;
    .dw 0xCF71 ;
    .dw 0x9A79 ;
    .dw 0x5685 ;
    .dw 0x30BF ;
    .dw 0xCF72 ;
    .dw 0xCE41 ;
    .dw 0x72D1 ;
    .dw 0x301B ;
    .dw 0xCF73 ;
    .dw 0xAA27 ;
    .dw 0x909B ;
    .dw 0x818D ;
    .dw 0xCF74 ;
    .dw 0x5BB9 ;
    .dw 0x8C95 ;
    .dw 0xEA9F ;
    .dw 0xCF75 ;
    .dw 0x3079 ;
    .dw 0x3273 ;
    .dw 0x87F ;
    .dw 0xCF76 ;
    .dw 0x5297 ;
    .dw 0x639B ;
    .dw 0xC64B ;
    .dw 0xCF77 ;
    .dw 0x6883 ;
    .dw 0xF731 ;
    .dw 0xA8DF ;
    .dw 0xCF78 ;
    .dw 0x4387 ;
    .dw 0x53CB ;
    .dw 0x9CA1 ;
    .dw 0xCF79 ;
    .dw 0xAB55 ;
    .dw 0xF8B ;
    .dw 0xC01D ;
    .dw 0xCF7A ;
    .dw 0x3335 ;
    .dw 0xA1EB ;
    .dw 0xFD35 ;
    .dw 0xCF7B ;
    .dw 0xB3D ;
    .dw 0x3F6B ;
    .dw 0xF1A1 ;
    .dw 0xCF7C ;
    .dw 0x6EA9 ;
    .dw 0x33F3 ;
    .dw 0xAB8B ;
    .dw 0xCF7D ;
    .dw 0xBB41 ;
    .dw 0xBCB7 ;
    .dw 0xAA7D ;
    .dw 0xCF7E ;
    .dw 0x1ABD ;
    .dw 0x8C9F ;
    .dw 0xBBA9 ;
    .dw 0xCF7F ;
    .dw 0xB089 ;
    .dw 0x55A3 ;
    .dw 0xED41 ;
    .dw 0xCF80 ;
    .dw 0xB59D ;
    .dw 0xC0AD ;
    .dw 0xE873 ;
    .dw 0xCF81 ;
    .dw 0xFEA7 ;
    .dw 0xB265 ;
    .dw 0xF55F ;
    .dw 0xCF82 ;
    .dw 0x8A87 ;
    .dw 0xE7F9 ;
    .dw 0x64D3 ;
    .dw 0xCF83 ;
    .dw 0xE769 ;
    .dw 0x6783 ;
    .dw 0x4547 ;
    .dw 0xCF84 ;
    .dw 0x9597 ;
    .dw 0xFBE9 ;
    .dw 0xE1DD ;
    .dw 0xCF85 ;
    .dw 0x5239 ;
    .dw 0x6397 ;
    .dw 0x99C1 ;
    .dw 0xCF86 ;
    .dw 0xE6FF ;
    .dw 0x84B ;
    .dw 0x31C7 ;
    .dw 0xCF87 ;
    .dw 0x3E93 ;
    .dw 0x6CDD ;
    .dw 0xE883 ;
    .dw 0xCF88 ;
    .dw 0x9A81 ;
    .dw 0xEB3D ;
    .dw 0x310B ;
    .dw 0xCF89 ;
    .dw 0xA8AF ;
    .dw 0x405D ;
    .dw 0xDFC7 ;
    .dw 0xCF8A ;
    .dw 0x515B ;
    .dw 0x7C13 ;
    .dw 0xD483 ;
    .dw 0xCF8B ;
    .dw 0x1EE3 ;
    .dw 0xD5E9 ;
    .dw 0x2FAD ;
    .dw 0xCF8C ;
    .dw 0x2A93 ;
    .dw 0xB0E1 ;
    .dw 0xC4C1 ;
    .dw 0xCF8D ;
    .dw 0xD1DD ;
    .dw 0xB1E7 ;
    .dw 0x1E29 ;
    .dw 0xCF8E ;
    .dw 0xD6ED ;
    .dw 0x1DB1 ;
    .dw 0x2C7F ;
    .dw 0xCF8F ;
    .dw 0x1935 ;
    .dw 0x6711 ;
    .dw 0x618D ;
    .dw 0xCF90 ;
    .dw 0xFB4D ;
    .dw 0xD003 ;
    .dw 0xB185 ;
    .dw 0xCF91 ;
    .dw 0x1969 ;
    .dw 0xD80F ;
    .dw 0xDD13 ;
    .dw 0xCF92 ;
    .dw 0xFDE7 ;
    .dw 0xF487 ;
    .dw 0x54AB ;
    .dw 0xCF93 ;
    .dw 0x4FDB ;
    .dw 0xCA39 ;
    .dw 0x7EAF ;
    .dw 0xCF94 ;
    .dw 0xF805 ;
    .dw 0xC4BF ;
    .dw 0x8F77 ;
    .dw 0xCF95 ;
    .dw 0x24E3 ;
    .dw 0x5055 ;
    .dw 0x491 ;
    .dw 0xCF96 ;
    .dw 0x37A9 ;
    .dw 0xCD9D ;
    .dw 0xD301 ;
    .dw 0xCF97 ;
    .dw 0x2379 ;
    .dw 0xDD89 ;
    .dw 0xBC7B ;
    .dw 0xCF98 ;
    .dw 0xE1F3 ;
    .dw 0x977F ;
    .dw 0xED8B ;
    .dw 0xCF99 ;
    .dw 0xF983 ;
    .dw 0xCE75 ;
    .dw 0x3E75 ;
    .dw 0xCF9A ;
    .dw 0x4081 ;
    .dw 0xF3D5 ;
    .dw 0x3185 ;
    .dw 0xCF9B ;
    .dw 0xCB77 ;
    .dw 0x47AD ;
    .dw 0x97E9 ;
    .dw 0xCF9C ;
    .dw 0x71AF ;
    .dw 0x93E1 ;
    .dw 0xE25B ;
    .dw 0xCF9D ;
    .dw 0x9139 ;
    .dw 0xCE65 ;
    .dw 0x33C3 ;
    .dw 0xCF9E ;
    .dw 0xF4F5 ;
    .dw 0xEF8D ;
    .dw 0xC8D5 ;
    .dw 0xCF9F ;
    .dw 0x1E1 ;
    .dw 0x59A7 ;
    .dw 0xE7A1 ;
    .dw 0xCFA0 ;
    .dw 0x4241 ;
    .dw 0xCB25 ;
    .dw 0x4265 ;
    .dw 0xCFA1 ;
    .dw 0xE769 ;
    .dw 0x27E1 ;
    .dw 0xCD97 ;
    .dw 0xCFA2 ;
    .dw 0xA491 ;
    .dw 0xB5C1 ;
    .dw 0x427 ;
    .dw 0xCFA3 ;
    .dw 0x6AD7 ;
    .dw 0xC611 ;
    .dw 0xD5AB ;
    .dw 0xCFA4 ;
    .dw 0x4DA9 ;
    .dw 0x8A15 ;
    .dw 0x83DD ;
    .dw 0xCFA5 ;
    .dw 0xE503 ;
    .dw 0xCB71 ;
    .dw 0x2189 ;
    .dw 0xCFA6 ;
    .dw 0x6A27 ;
    .dw 0x2EBB ;
    .dw 0xE6D9 ;
    .dw 0xCFA7 ;
    .dw 0xDF6B ;
    .dw 0x35E5 ;
    .dw 0x288D ;
    .dw 0xCFA8 ;
    .dw 0x42DD ;
    .dw 0x6A67 ;
    .dw 0xD7F1 ;
    .dw 0xCFA9 ;
    .dw 0x143B ;
    .dw 0x70F9 ;
    .dw 0x319D ;
    .dw 0xCFAA ;
    .dw 0x919B ;
    .dw 0x7C3B ;
    .dw 0x1B7B ;
    .dw 0xCFAB ;
    .dw 0x4413 ;
    .dw 0x42CB ;
    .dw 0xC3FF ;
    .dw 0xCFAC ;
    .dw 0x7D61 ;
    .dw 0x27AB ;
    .dw 0x818B ;
    .dw 0xCFAD ;
    .dw 0x839F ;
    .dw 0x7FB1 ;
    .dw 0x27A3 ;
    .dw 0xCFAE ;
    .dw 0x932D ;
    .dw 0xE719 ;
    .dw 0x5449 ;
    .dw 0xCFAF ;
    .dw 0x1289 ;
    .dw 0xDED7 ;
    .dw 0xC905 ;
    .dw 0xCFB0 ;
    .dw 0xE641 ;
    .dw 0xDFAD ;
    .dw 0xF1A5 ;
    .dw 0xCFB1 ;
    .dw 0xC0D1 ;
    .dw 0xF7BD ;
    .dw 0x3423 ;
    .dw 0xCFB2 ;
    .dw 0xAC39 ;
    .dw 0xDC73 ;
    .dw 0x4545 ;
    .dw 0xCFB3 ;
    .dw 0x3F39 ;
    .dw 0xB1D9 ;
    .dw 0x3DA7 ;
    .dw 0xCFB4 ;
    .dw 0x86A1 ;
    .dw 0xE663 ;
    .dw 0xB105 ;
    .dw 0xCFB5 ;
    .dw 0x52A1 ;
    .dw 0xA52D ;
    .dw 0xB8C7 ;
    .dw 0xCFB6 ;
    .dw 0x9D8B ;
    .dw 0xE251 ;
    .dw 0xFFB3 ;
    .dw 0xCFB7 ;
    .dw 0xA225 ;
    .dw 0x7425 ;
    .dw 0xA407 ;
    .dw 0xCFB8 ;
    .dw 0x13C3 ;
    .dw 0xD553 ;
    .dw 0x9F8F ;
    .dw 0xCFB9 ;
    .dw 0x9ABF ;
    .dw 0x6487 ;
    .dw 0xE63D ;
    .dw 0xCFBA ;
    .dw 0x971B ;
    .dw 0xEBCD ;
    .dw 0xF725 ;
    .dw 0xCFBB ;
    .dw 0x8B4F ;
    .dw 0xCED3 ;
    .dw 0x691B ;
    .dw 0xCFBC ;
    .dw 0x3C89 ;
    .dw 0xFE7B ;
    .dw 0x9105 ;
    .dw 0xCFBD ;
    .dw 0x86D9 ;
    .dw 0xC0CD ;
    .dw 0x75A5 ;
    .dw 0xCFBE ;
    .dw 0xD961 ;
    .dw 0xF4C1 ;
    .dw 0x7801 ;
    .dw 0xCFBF ;
    .dw 0xAAA3 ;
    .dw 0xC993 ;
    .dw 0x92C5 ;
    .dw 0xCFC0 ;
    .dw 0x8D ;
    .dw 0xEAB5 ;
    .dw 0xCF55 ;
    .dw 0xCFC1 ;
    .dw 0xF94D ;
    .dw 0xB307 ;
    .dw 0xA575 ;
    .dw 0xCFC2 ;
    .dw 0x140F ;
    .dw 0x4CE7 ;
    .dw 0xD78B ;
    .dw 0xCFC3 ;
    .dw 0xF359 ;
    .dw 0x4DE7 ;
    .dw 0x958B ;
    .dw 0xCFC4 ;
    .dw 0xD893 ;
    .dw 0xBA3 ;
    .dw 0x8A5D ;
    .dw 0xCFC5 ;
    .dw 0x5149 ;
    .dw 0xCB4B ;
    .dw 0x21E3 ;
    .dw 0xCFC6 ;
    .dw 0xA65 ;
    .dw 0x7A85 ;
    .dw 0x2571 ;
    .dw 0xCFC7 ;
    .dw 0xA2DF ;
    .dw 0xC7F9 ;
    .dw 0xB9AF ;
    .dw 0xCFC8 ;
    .dw 0xF8A3 ;
    .dw 0x491D ;
    .dw 0xBD37 ;
    .dw 0xCFC9 ;
    .dw 0xFA7B ;
    .dw 0x8B45 ;
    .dw 0xCD ;
    .dw 0xCFCA ;
    .dw 0x84F3 ;
    .dw 0x1C97 ;
    .dw 0xA6C7 ;
    .dw 0xCFCB ;
    .dw 0x1349 ;
    .dw 0x6CD9 ;
    .dw 0xF7E3 ;
    .dw 0xCFCC ;
    .dw 0x738D ;
    .dw 0x9209 ;
    .dw 0x90F9 ;
    .dw 0xCFCD ;
    .dw 0x6C31 ;
    .dw 0x3A3D ;
    .dw 0x7921 ;
    .dw 0xCFCE ;
    .dw 0x18E5 ;
    .dw 0xB46F ;
    .dw 0xE29B ;
    .dw 0xCFCF ;
    .dw 0x812D ;
    .dw 0x2E4B ;
    .dw 0xB56B ;
    .dw 0xCFD0 ;
    .dw 0x87E5 ;
    .dw 0x18D5 ;
    .dw 0xC509 ;
    .dw 0xCFD1 ;
    .dw 0x8005 ;
    .dw 0xFAA1 ;
    .dw 0x7DC1 ;
    .dw 0xCFD2 ;
    .dw 0xCCC5 ;
    .dw 0xBEE7 ;
    .dw 0x87FB ;
    .dw 0xCFD3 ;
    .dw 0x6D11 ;
    .dw 0xE40B ;
    .dw 0x47C5 ;
    .dw 0xCFD4 ;
    .dw 0xDE9F ;
    .dw 0x6351 ;
    .dw 0x24DB ;
    .dw 0xCFD5 ;
    .dw 0x8803 ;
    .dw 0x690D ;
    .dw 0xE3F5 ;
    .dw 0xCFD6 ;
    .dw 0x22C9 ;
    .dw 0x505 ;
    .dw 0xF573 ;
    .dw 0xCFD7 ;
    .dw 0xC055 ;
    .dw 0xB295 ;
    .dw 0xA7D3 ;
    .dw 0xCFD8 ;
    .dw 0x305 ;
    .dw 0xD61D ;
    .dw 0x933B ;
    .dw 0xCFD9 ;
    .dw 0xC59 ;
    .dw 0x8CD1 ;
    .dw 0x3D47 ;
    .dw 0xCFDA ;
    .dw 0x9095 ;
    .dw 0x8C21 ;
    .dw 0xAA23 ;
    .dw 0xCFDB ;
    .dw 0x5D97 ;
    .dw 0x376F ;
    .dw 0x3C85 ;
    .dw 0xCFDC ;
    .dw 0xDC49 ;
    .dw 0xE393 ;
    .dw 0xB31B ;
    .dw 0xCFDD ;
    .dw 0x9871 ;
    .dw 0x61FF ;
    .dw 0xCF1 ;
    .dw 0xCFDE ;
    .dw 0xEC8D ;
    .dw 0xD8B ;
    .dw 0x683D ;
    .dw 0xCFDF ;
    .dw 0x449D ;
    .dw 0x82F5 ;
    .dw 0x24FF ;
    .dw 0xCFE0 ;
    .dw 0x708D ;
    .dw 0x8629 ;
    .dw 0xB5D3 ;
    .dw 0xCFE1 ;
    .dw 0x7FA3 ;
    .dw 0xC4EB ;
    .dw 0x80C7 ;
    .dw 0xCFE2 ;
    .dw 0xD88F ;
    .dw 0x5DBF ;
    .dw 0x5113 ;
    .dw 0xCFE3 ;
    .dw 0xF1BD ;
    .dw 0x6797 ;
    .dw 0xEA3B ;
    .dw 0xCFE4 ;
    .dw 0xB965 ;
    .dw 0x2E63 ;
    .dw 0x56ED ;
    .dw 0xCFE5 ;
    .dw 0x15B ;
    .dw 0x733 ;
    .dw 0x5599 ;
    .dw 0xCFE6 ;
    .dw 0xB249 ;
    .dw 0xAAFB ;
    .dw 0xC29B ;
    .dw 0xCFE7 ;
    .dw 0x20C1 ;
    .dw 0x26A9 ;
    .dw 0x39 ;
    .dw 0xCFE8 ;
    .dw 0xD1E5 ;
    .dw 0xCC2D ;
    .dw 0x8D6D ;
    .dw 0xCFE9 ;
    .dw 0xB4C3 ;
    .dw 0xF651 ;
    .dw 0xF25 ;
    .dw 0xCFEA ;
    .dw 0x10F3 ;
    .dw 0xFB75 ;
    .dw 0x3E79 ;
    .dw 0xCFEB ;
    .dw 0x9B55 ;
    .dw 0x2A7 ;
    .dw 0xFEAB ;
    .dw 0xCFEC ;
    .dw 0x4623 ;
    .dw 0x1BCD ;
    .dw 0xFA9B ;
    .dw 0xCFED ;
    .dw 0xA3E3 ;
    .dw 0x9B9B ;
    .dw 0x2B6F ;
    .dw 0xCFEE ;
    .dw 0x58A9 ;
    .dw 0xD303 ;
    .dw 0x2287 ;
    .dw 0xCFEF ;
    .dw 0x3AF1 ;
    .dw 0xBEFF ;
    .dw 0xF90B ;
    .dw 0xCFF0 ;
    .dw 0xCC47 ;
    .dw 0xDE4D ;
    .dw 0x9E43 ;
    .dw 0xCFF1 ;
    .dw 0xFE51 ;
    .dw 0x7DC7 ;
    .dw 0x79BD ;
    .dw 0xCFF2 ;
    .dw 0x6B1D ;
    .dw 0x6835 ;
    .dw 0x7AD9 ;
    .dw 0xCFF3 ;
    .dw 0xC635 ;
    .dw 0x955D ;
    .dw 0xDE57 ;
    .dw 0xCFF4 ;
    .dw 0x2F0B ;
    .dw 0x2555 ;
    .dw 0xD887 ;
    .dw 0xCFF5 ;
    .dw 0xCB59 ;
    .dw 0xAC01 ;
    .dw 0x3CEB ;
    .dw 0xCFF6 ;
    .dw 0xFDF5 ;
    .dw 0x510D ;
    .dw 0xB54D ;
    .dw 0xCFF7 ;
    .dw 0xD1DB ;
    .dw 0xA867 ;
    .dw 0x482F ;
    .dw 0xCFF8 ;
    .dw 0xB1C9 ;
    .dw 0x5AA7 ;
    .dw 0x4121 ;
    .dw 0xCFF9 ;
    .dw 0x83A1 ;
    .dw 0x5A65 ;
    .dw 0x4161 ;
    .dw 0xCFFA ;
    .dw 0x9E7F ;
    .dw 0xF1F ;
    .dw 0x7E8F ;
    .dw 0xCFFB ;
    .dw 0x4D1F ;
    .dw 0x7C11 ;
    .dw 0xA17B ;
    .dw 0xCFFC ;
    .dw 0xB5FD ;
    .dw 0x2AF7 ;
    .dw 0x5C2B ;
    .dw 0xCFFD ;
    .dw 0xFA4F ;
    .dw 0x580D ;
    .dw 0x8E77 ;
    .dw 0xCFFE ;
    .dw 0xEB0B ;
    .dw 0x633B ;
    .dw 0x9099 ;
    .dw 0xCFFF ;
    .dw 0xE1A1 ;
    .dw 0x7B5F ;
    .dw 0xC9B ;
// COUNT = 6662



    // count of UI's will be in r5, which was initialized to 0 by header

CHECK_INIT_DEF(p0); //CHECK_INIT(p0, 0xFF7FFFFC);
                    // Xhandler counts all EXCAUSE = 0x21;
CHECKREG(r5, 256); // count of all 16 bit UI's.

END:
dbg_pass;            // End the test

//*********************************************************************

//
// Handlers for Events
//

NHANDLE:            // NMI Handler 2
RTN;

XHANDLE:            // Exception Handler 3
                    // 64 bit illegal opcode handler - skips bad instruction

    [ -- SP ] = ASTAT; // save what we damage
    [ -- SP ] = ( R7:6 );
    R7 = SEQSTAT;
    R7 <<= 26;
    R7 >>= 26;      // only want EXCAUSE
    R6 = 0x21;      // EXCAUSE 0x21 means I-Fetch Undefined Instruction
CC = r7 == r6;
IF CC JUMP UNDEFINEDINSTRUCTION;   // If EXCAUSE != 0x21 then leave

dbg_fail;

UNDEFINEDINSTRUCTION:
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
