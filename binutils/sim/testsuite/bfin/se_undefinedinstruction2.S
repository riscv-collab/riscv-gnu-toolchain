//Original:/proj/frio/dv/testcases/seq/se_undefinedinstruction2/se_undefinedinstruction2.dsp
// Description: 16 bit special cases Undefined Instructions in Supervisor Mode
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
r4 = p1;

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

    // count of UI's will be in r5, which was initialized to 0 by header

// 16 bit special cases  COUNT = 830
    .dw 0x10E ;
    .dw 0x124 ;
.ifndef BFIN_HW
	// XXX: hardware doesnt trigger illegal exception ?
    .dw 0x125 ;
.endif
    .dw 0x164 ;
.ifndef BFIN_HW
	// XXX: hardware doesnt trigger illegal exception ?
    .dw 0x165 ;
.endif
    .dw 0x128 ;
    .dw 0x129 ;
    .dw 0x12A ;
    .dw 0x12B ;
    .dw 0x12C ;
    .dw 0x12D ;
    .dw 0x12E ;
    .dw 0x12F ;
    .dw 0x168 ;
    .dw 0x169 ;
    .dw 0x16A ;
    .dw 0x16B ;
    .dw 0x16C ;
    .dw 0x16D ;
    .dw 0x16E ;
    .dw 0x16F ;
#if 0
	// EMUDAT = [SP++]; is valid
    .dw 0x13F ;
	// [SP++] = EMUDAT; is valid
    .dw 0x17F ;
#endif
    .dw 0x486 ;
    .dw 0x487 ;
    .dw 0x210 ;
    .dw 0x211 ;
    .dw 0x212 ;
    .dw 0x213 ;
    .dw 0x214 ;
    .dw 0x215 ;
    .dw 0x216 ;
    .dw 0x217 ;
    .dw 0x305 ;
#if 0
	// Not documented, but hardware takes them
	// CC = <reserved astat>
    .dw 0x307 ;
    .dw 0x308 ;
    .dw 0x309 ;
    .dw 0x30A ;
    .dw 0x30B ;
    .dw 0x30C ;
    .dw 0x30D ;
    .dw 0x30E ;
    .dw 0x30F ;
    .dw 0x310 ;
    .dw 0x311 ;
    .dw 0x312 ;
    .dw 0x313 ;
    .dw 0x314 ;
    .dw 0x315 ;
    .dw 0x316 ;
    .dw 0x317 ;
    .dw 0x318 ;
    .dw 0x319 ;
    .dw 0x31A ;
    .dw 0x31B ;
    .dw 0x31C ;
    .dw 0x31D ;
    .dw 0x31E ;
    .dw 0x31F ;
#endif
    .dw 0x325 ;
#if 0
	// Not documented, but hardware takes them
	// CC |= <reserved astat>
    .dw 0x327 ;
    .dw 0x328 ;
    .dw 0x329 ;
    .dw 0x32A ;
    .dw 0x32B ;
    .dw 0x32C ;
    .dw 0x32D ;
    .dw 0x32E ;
    .dw 0x32F ;
    .dw 0x330 ;
    .dw 0x331 ;
    .dw 0x332 ;
    .dw 0x333 ;
    .dw 0x334 ;
    .dw 0x335 ;
    .dw 0x336 ;
    .dw 0x337 ;
    .dw 0x338 ;
    .dw 0x339 ;
    .dw 0x33A ;
    .dw 0x33B ;
    .dw 0x33C ;
    .dw 0x33D ;
    .dw 0x33E ;
    .dw 0x33F ;
#endif
    .dw 0x345 ;
#if 0
	// Not documented, but hardware takes them
	// CC &= <reserved astat>
    .dw 0x347 ;
    .dw 0x348 ;
    .dw 0x349 ;
    .dw 0x34A ;
    .dw 0x34B ;
    .dw 0x34C ;
    .dw 0x34D ;
    .dw 0x34E ;
    .dw 0x34F ;
    .dw 0x350 ;
    .dw 0x351 ;
    .dw 0x352 ;
    .dw 0x353 ;
    .dw 0x354 ;
    .dw 0x355 ;
    .dw 0x356 ;
    .dw 0x357 ;
    .dw 0x358 ;
    .dw 0x359 ;
    .dw 0x35A ;
    .dw 0x35B ;
    .dw 0x35C ;
    .dw 0x35D ;
    .dw 0x35E ;
    .dw 0x35F ;
#endif
    .dw 0x365 ;
#if 0
	// Not documented, but hardware takes them
	// CC ^= <reserved astat>
    .dw 0x367 ;
    .dw 0x368 ;
    .dw 0x369 ;
    .dw 0x36A ;
    .dw 0x36B ;
    .dw 0x36C ;
    .dw 0x36D ;
    .dw 0x36E ;
    .dw 0x36F ;
    .dw 0x370 ;
    .dw 0x371 ;
    .dw 0x372 ;
    .dw 0x373 ;
    .dw 0x374 ;
    .dw 0x375 ;
    .dw 0x376 ;
    .dw 0x377 ;
    .dw 0x378 ;
    .dw 0x379 ;
    .dw 0x37A ;
    .dw 0x37B ;
    .dw 0x37C ;
    .dw 0x37D ;
    .dw 0x37E ;
    .dw 0x37F ;
#endif
    .dw 0x385 ;
#if 0
	// Not documented, but hardware takes them
	// <reserved astat> = CC
    .dw 0x387 ;
    .dw 0x388 ;
    .dw 0x389 ;
    .dw 0x38A ;
    .dw 0x38B ;
    .dw 0x38C ;
    .dw 0x38D ;
    .dw 0x38E ;
    .dw 0x38F ;
    .dw 0x390 ;
    .dw 0x391 ;
    .dw 0x392 ;
    .dw 0x393 ;
    .dw 0x394 ;
    .dw 0x395 ;
    .dw 0x396 ;
    .dw 0x397 ;
    .dw 0x398 ;
    .dw 0x399 ;
    .dw 0x39A ;
    .dw 0x39B ;
    .dw 0x39C ;
    .dw 0x39D ;
    .dw 0x39E ;
    .dw 0x39F ;
#endif
    .dw 0x3A5 ;
#if 0
	// Not documented, but hardware takes them
	// <reserved astat> |= CC
    .dw 0x3A7 ;
    .dw 0x3A8 ;
    .dw 0x3A9 ;
    .dw 0x3AA ;
    .dw 0x3AB ;
    .dw 0x3AC ;
    .dw 0x3AD ;
    .dw 0x3AE ;
    .dw 0x3AF ;
    .dw 0x3B0 ;
    .dw 0x3B1 ;
    .dw 0x3B2 ;
    .dw 0x3B3 ;
    .dw 0x3B4 ;
    .dw 0x3B5 ;
    .dw 0x3B6 ;
    .dw 0x3B7 ;
    .dw 0x3B8 ;
    .dw 0x3B9 ;
    .dw 0x3BA ;
    .dw 0x3BB ;
    .dw 0x3BC ;
    .dw 0x3BD ;
    .dw 0x3BE ;
    .dw 0x3BF ;
#endif
    .dw 0x3C5 ;
#if 0
	// Not documented, but hardware takes them
	// <reserved astat> &= CC
    .dw 0x3C7 ;
    .dw 0x3C8 ;
    .dw 0x3C9 ;
    .dw 0x3CA ;
    .dw 0x3CB ;
    .dw 0x3CC ;
    .dw 0x3CD ;
    .dw 0x3CE ;
    .dw 0x3CF ;
    .dw 0x3D0 ;
    .dw 0x3D1 ;
    .dw 0x3D2 ;
    .dw 0x3D3 ;
    .dw 0x3D4 ;
    .dw 0x3D5 ;
    .dw 0x3D6 ;
    .dw 0x3D7 ;
    .dw 0x3D8 ;
    .dw 0x3D9 ;
    .dw 0x3DA ;
    .dw 0x3DB ;
    .dw 0x3DC ;
    .dw 0x3DD ;
    .dw 0x3DE ;
    .dw 0x3DF ;
#endif
    .dw 0x3E5 ;
#if 0
	// Not documented, but hardware takes them
	// <reserved astat> ^= CC
    .dw 0x3E7 ;
    .dw 0x3E8 ;
    .dw 0x3E9 ;
    .dw 0x3EA ;
    .dw 0x3EB ;
    .dw 0x3EC ;
    .dw 0x3ED ;
    .dw 0x3EE ;
    .dw 0x3EF ;
    .dw 0x3F0 ;
    .dw 0x3F1 ;
    .dw 0x3F2 ;
    .dw 0x3F3 ;
    .dw 0x3F4 ;
    .dw 0x3F5 ;
    .dw 0x3F6 ;
    .dw 0x3F7 ;
    .dw 0x3F8 ;
    .dw 0x3F9 ;
    .dw 0x3FA ;
    .dw 0x3FB ;
    .dw 0x3FC ;
    .dw 0x3FD ;
    .dw 0x3FE ;
    .dw 0x3FF ;
#endif
    .dw 0x3A00 ;
    .dw 0x3A01 ;
    .dw 0x3A02 ;
    .dw 0x3A03 ;
    .dw 0x3A04 ;
    .dw 0x3A05 ;
    .dw 0x3A06 ;
    .dw 0x3A07 ;
    .dw 0x3A08 ;
    .dw 0x3A09 ;
    .dw 0x3A0A ;
    .dw 0x3A0B ;
    .dw 0x3A0C ;
    .dw 0x3A0D ;
    .dw 0x3A0E ;
    .dw 0x3A0F ;
    .dw 0x3A10 ;
    .dw 0x3A11 ;
    .dw 0x3A12 ;
    .dw 0x3A13 ;
    .dw 0x3A14 ;
    .dw 0x3A15 ;
    .dw 0x3A16 ;
    .dw 0x3A17 ;
    .dw 0x3A18 ;
    .dw 0x3A19 ;
    .dw 0x3A1A ;
    .dw 0x3A1B ;
    .dw 0x3A1C ;
    .dw 0x3A1D ;
    .dw 0x3A1E ;
    .dw 0x3A1F ;
    .dw 0x3A20 ;
    .dw 0x3A21 ;
    .dw 0x3A22 ;
    .dw 0x3A23 ;
    .dw 0x3A24 ;
    .dw 0x3A25 ;
    .dw 0x3A26 ;
    .dw 0x3A27 ;
    .dw 0x3A28 ;
    .dw 0x3A29 ;
    .dw 0x3A2A ;
    .dw 0x3A2B ;
    .dw 0x3A2C ;
    .dw 0x3A2D ;
    .dw 0x3A2E ;
    .dw 0x3A2F ;
    .dw 0x3A30 ;
    .dw 0x3A31 ;
    .dw 0x3A32 ;
    .dw 0x3A33 ;
    .dw 0x3A34 ;
    .dw 0x3A35 ;
    .dw 0x3A36 ;
    .dw 0x3A37 ;
    .dw 0x3A38 ;
    .dw 0x3A39 ;
    .dw 0x3A3A ;
    .dw 0x3A3B ;
    .dw 0x3A3C ;
    .dw 0x3A3D ;
    .dw 0x3A3E ;
    .dw 0x3A3F ;
    .dw 0x3A40 ;
    .dw 0x3A41 ;
    .dw 0x3A42 ;
    .dw 0x3A43 ;
    .dw 0x3A44 ;
    .dw 0x3A45 ;
    .dw 0x3A46 ;
    .dw 0x3A47 ;
    .dw 0x3A48 ;
    .dw 0x3A49 ;
    .dw 0x3A4A ;
    .dw 0x3A4B ;
    .dw 0x3A4C ;
    .dw 0x3A4D ;
    .dw 0x3A4E ;
    .dw 0x3A4F ;
    .dw 0x3A50 ;
    .dw 0x3A51 ;
    .dw 0x3A52 ;
    .dw 0x3A53 ;
    .dw 0x3A54 ;
    .dw 0x3A55 ;
    .dw 0x3A56 ;
    .dw 0x3A57 ;
    .dw 0x3A58 ;
    .dw 0x3A59 ;
    .dw 0x3A5A ;
    .dw 0x3A5B ;
    .dw 0x3A5C ;
    .dw 0x3A5D ;
    .dw 0x3A5E ;
    .dw 0x3A5F ;
    .dw 0x3A60 ;
    .dw 0x3A61 ;
    .dw 0x3A62 ;
    .dw 0x3A63 ;
    .dw 0x3A64 ;
    .dw 0x3A65 ;
    .dw 0x3A66 ;
    .dw 0x3A67 ;
    .dw 0x3A68 ;
    .dw 0x3A69 ;
    .dw 0x3A6A ;
    .dw 0x3A6B ;
    .dw 0x3A6C ;
    .dw 0x3A6D ;
    .dw 0x3A6E ;
    .dw 0x3A6F ;
    .dw 0x3A70 ;
    .dw 0x3A71 ;
    .dw 0x3A72 ;
    .dw 0x3A73 ;
    .dw 0x3A74 ;
    .dw 0x3A75 ;
    .dw 0x3A76 ;
    .dw 0x3A77 ;
    .dw 0x3A78 ;
    .dw 0x3A79 ;
    .dw 0x3A7A ;
    .dw 0x3A7B ;
    .dw 0x3A7C ;
    .dw 0x3A7D ;
    .dw 0x3A7E ;
    .dw 0x3A7F ;
    .dw 0x3A80 ;
    .dw 0x3A81 ;
    .dw 0x3A82 ;
    .dw 0x3A83 ;
    .dw 0x3A84 ;
    .dw 0x3A85 ;
    .dw 0x3A86 ;
    .dw 0x3A87 ;
    .dw 0x3A88 ;
    .dw 0x3A89 ;
    .dw 0x3A8A ;
    .dw 0x3A8B ;
    .dw 0x3A8C ;
    .dw 0x3A8D ;
    .dw 0x3A8E ;
    .dw 0x3A8F ;
    .dw 0x3A90 ;
    .dw 0x3A91 ;
    .dw 0x3A92 ;
    .dw 0x3A93 ;
    .dw 0x3A94 ;
    .dw 0x3A95 ;
    .dw 0x3A96 ;
    .dw 0x3A97 ;
    .dw 0x3A98 ;
    .dw 0x3A99 ;
    .dw 0x3A9A ;
    .dw 0x3A9B ;
    .dw 0x3A9C ;
    .dw 0x3A9D ;
    .dw 0x3A9E ;
    .dw 0x3A9F ;
    .dw 0x3AA0 ;
    .dw 0x3AA1 ;
    .dw 0x3AA2 ;
    .dw 0x3AA3 ;
    .dw 0x3AA4 ;
    .dw 0x3AA5 ;
    .dw 0x3AA6 ;
    .dw 0x3AA7 ;
    .dw 0x3AA8 ;
    .dw 0x3AA9 ;
    .dw 0x3AAA ;
    .dw 0x3AAB ;
    .dw 0x3AAC ;
    .dw 0x3AAD ;
    .dw 0x3AAE ;
    .dw 0x3AAF ;
    .dw 0x3AB0 ;
    .dw 0x3AB1 ;
    .dw 0x3AB2 ;
    .dw 0x3AB3 ;
    .dw 0x3AB4 ;
    .dw 0x3AB5 ;
    .dw 0x3AB6 ;
    .dw 0x3AB7 ;
    .dw 0x3AB8 ;
    .dw 0x3AB9 ;
    .dw 0x3ABA ;
    .dw 0x3ABB ;
    .dw 0x3ABC ;
    .dw 0x3ABD ;
    .dw 0x3ABE ;
    .dw 0x3ABF ;
    .dw 0x3AC0 ;
    .dw 0x3AC1 ;
    .dw 0x3AC2 ;
    .dw 0x3AC3 ;
    .dw 0x3AC4 ;
    .dw 0x3AC5 ;
    .dw 0x3AC6 ;
    .dw 0x3AC7 ;
    .dw 0x3AC8 ;
    .dw 0x3AC9 ;
    .dw 0x3ACA ;
    .dw 0x3ACB ;
    .dw 0x3ACC ;
    .dw 0x3ACD ;
    .dw 0x3ACE ;
    .dw 0x3ACF ;
    .dw 0x3AD0 ;
    .dw 0x3AD1 ;
    .dw 0x3AD2 ;
    .dw 0x3AD3 ;
    .dw 0x3AD4 ;
    .dw 0x3AD5 ;
    .dw 0x3AD6 ;
    .dw 0x3AD7 ;
    .dw 0x3AD8 ;
    .dw 0x3AD9 ;
    .dw 0x3ADA ;
    .dw 0x3ADB ;
    .dw 0x3ADC ;
    .dw 0x3ADD ;
    .dw 0x3ADE ;
    .dw 0x3ADF ;
    .dw 0x3AE0 ;
    .dw 0x3AE1 ;
    .dw 0x3AE2 ;
    .dw 0x3AE3 ;
    .dw 0x3AE4 ;
    .dw 0x3AE5 ;
    .dw 0x3AE6 ;
    .dw 0x3AE7 ;
    .dw 0x3AE8 ;
    .dw 0x3AE9 ;
    .dw 0x3AEA ;
    .dw 0x3AEB ;
    .dw 0x3AEC ;
    .dw 0x3AED ;
    .dw 0x3AEE ;
    .dw 0x3AEF ;
    .dw 0x3AF0 ;
    .dw 0x3AF1 ;
    .dw 0x3AF2 ;
    .dw 0x3AF3 ;
    .dw 0x3AF4 ;
    .dw 0x3AF5 ;
    .dw 0x3AF6 ;
    .dw 0x3AF7 ;
    .dw 0x3AF8 ;
    .dw 0x3AF9 ;
    .dw 0x3AFA ;
    .dw 0x3AFB ;
    .dw 0x3AFC ;
    .dw 0x3AFD ;
    .dw 0x3AFE ;
    .dw 0x3AFF ;
    .dw 0x3B00 ;
    .dw 0x3B01 ;
    .dw 0x3B02 ;
    .dw 0x3B03 ;
    .dw 0x3B04 ;
    .dw 0x3B05 ;
    .dw 0x3B06 ;
    .dw 0x3B07 ;
    .dw 0x3B08 ;
    .dw 0x3B09 ;
    .dw 0x3B0A ;
    .dw 0x3B0B ;
    .dw 0x3B0C ;
    .dw 0x3B0D ;
    .dw 0x3B0E ;
    .dw 0x3B0F ;
    .dw 0x3B10 ;
    .dw 0x3B11 ;
    .dw 0x3B12 ;
    .dw 0x3B13 ;
    .dw 0x3B14 ;
    .dw 0x3B15 ;
    .dw 0x3B16 ;
    .dw 0x3B17 ;
    .dw 0x3B18 ;
    .dw 0x3B19 ;
    .dw 0x3B1A ;
    .dw 0x3B1B ;
    .dw 0x3B1C ;
    .dw 0x3B1D ;
    .dw 0x3B1E ;
    .dw 0x3B1F ;
    .dw 0x3B20 ;
    .dw 0x3B21 ;
    .dw 0x3B22 ;
    .dw 0x3B23 ;
    .dw 0x3B24 ;
    .dw 0x3B25 ;
    .dw 0x3B26 ;
    .dw 0x3B27 ;
    .dw 0x3B28 ;
    .dw 0x3B29 ;
    .dw 0x3B2A ;
    .dw 0x3B2B ;
    .dw 0x3B2C ;
    .dw 0x3B2D ;
    .dw 0x3B2E ;
    .dw 0x3B2F ;
    .dw 0x3B30 ;
    .dw 0x3B31 ;
    .dw 0x3B32 ;
    .dw 0x3B33 ;
    .dw 0x3B34 ;
    .dw 0x3B35 ;
    .dw 0x3B36 ;
    .dw 0x3B37 ;
    .dw 0x3B38 ;
    .dw 0x3B39 ;
    .dw 0x3B3A ;
    .dw 0x3B3B ;
    .dw 0x3B3C ;
    .dw 0x3B3D ;
    .dw 0x3B3E ;
    .dw 0x3B3F ;
    .dw 0x3B40 ;
    .dw 0x3B41 ;
    .dw 0x3B42 ;
    .dw 0x3B43 ;
    .dw 0x3B44 ;
    .dw 0x3B45 ;
    .dw 0x3B46 ;
    .dw 0x3B47 ;
    .dw 0x3B48 ;
    .dw 0x3B49 ;
    .dw 0x3B4A ;
    .dw 0x3B4B ;
    .dw 0x3B4C ;
    .dw 0x3B4D ;
    .dw 0x3B4E ;
    .dw 0x3B4F ;
    .dw 0x3B50 ;
    .dw 0x3B51 ;
    .dw 0x3B52 ;
    .dw 0x3B53 ;
    .dw 0x3B54 ;
    .dw 0x3B55 ;
    .dw 0x3B56 ;
    .dw 0x3B57 ;
    .dw 0x3B58 ;
    .dw 0x3B59 ;
    .dw 0x3B5A ;
    .dw 0x3B5B ;
    .dw 0x3B5C ;
    .dw 0x3B5D ;
    .dw 0x3B5E ;
    .dw 0x3B5F ;
    .dw 0x3B60 ;
    .dw 0x3B61 ;
    .dw 0x3B62 ;
    .dw 0x3B63 ;
    .dw 0x3B64 ;
    .dw 0x3B65 ;
    .dw 0x3B66 ;
    .dw 0x3B67 ;
    .dw 0x3B68 ;
    .dw 0x3B69 ;
    .dw 0x3B6A ;
    .dw 0x3B6B ;
    .dw 0x3B6C ;
    .dw 0x3B6D ;
    .dw 0x3B6E ;
    .dw 0x3B6F ;
    .dw 0x3B70 ;
    .dw 0x3B71 ;
    .dw 0x3B72 ;
    .dw 0x3B73 ;
    .dw 0x3B74 ;
    .dw 0x3B75 ;
    .dw 0x3B76 ;
    .dw 0x3B77 ;
    .dw 0x3B78 ;
    .dw 0x3B79 ;
    .dw 0x3B7A ;
    .dw 0x3B7B ;
    .dw 0x3B7C ;
    .dw 0x3B7D ;
    .dw 0x3B7E ;
    .dw 0x3B7F ;
    .dw 0x3B80 ;
    .dw 0x3B81 ;
    .dw 0x3B82 ;
    .dw 0x3B83 ;
    .dw 0x3B84 ;
    .dw 0x3B85 ;
    .dw 0x3B86 ;
    .dw 0x3B87 ;
    .dw 0x3B88 ;
    .dw 0x3B89 ;
    .dw 0x3B8A ;
    .dw 0x3B8B ;
    .dw 0x3B8C ;
    .dw 0x3B8D ;
    .dw 0x3B8E ;
    .dw 0x3B8F ;
    .dw 0x3B90 ;
    .dw 0x3B91 ;
    .dw 0x3B92 ;
    .dw 0x3B93 ;
    .dw 0x3B94 ;
    .dw 0x3B95 ;
    .dw 0x3B96 ;
    .dw 0x3B97 ;
    .dw 0x3B98 ;
    .dw 0x3B99 ;
    .dw 0x3B9A ;
    .dw 0x3B9B ;
    .dw 0x3B9C ;
    .dw 0x3B9D ;
    .dw 0x3B9E ;
    .dw 0x3B9F ;
    .dw 0x3BA0 ;
    .dw 0x3BA1 ;
    .dw 0x3BA2 ;
    .dw 0x3BA3 ;
    .dw 0x3BA4 ;
    .dw 0x3BA5 ;
    .dw 0x3BA6 ;
    .dw 0x3BA7 ;
    .dw 0x3BA8 ;
    .dw 0x3BA9 ;
    .dw 0x3BAA ;
    .dw 0x3BAB ;
    .dw 0x3BAC ;
    .dw 0x3BAD ;
    .dw 0x3BAE ;
    .dw 0x3BAF ;
    .dw 0x3BB0 ;
    .dw 0x3BB1 ;
    .dw 0x3BB2 ;
    .dw 0x3BB3 ;
    .dw 0x3BB4 ;
    .dw 0x3BB5 ;
    .dw 0x3BB6 ;
    .dw 0x3BB7 ;
    .dw 0x3BB8 ;
    .dw 0x3BB9 ;
    .dw 0x3BBA ;
    .dw 0x3BBB ;
    .dw 0x3BBC ;
    .dw 0x3BBD ;
    .dw 0x3BBE ;
    .dw 0x3BBF ;
    .dw 0x3BC0 ;
    .dw 0x3BC1 ;
    .dw 0x3BC2 ;
    .dw 0x3BC3 ;
    .dw 0x3BC4 ;
    .dw 0x3BC5 ;
    .dw 0x3BC6 ;
    .dw 0x3BC7 ;
    .dw 0x3BC8 ;
    .dw 0x3BC9 ;
    .dw 0x3BCA ;
    .dw 0x3BCB ;
    .dw 0x3BCC ;
    .dw 0x3BCD ;
    .dw 0x3BCE ;
    .dw 0x3BCF ;
    .dw 0x3BD0 ;
    .dw 0x3BD1 ;
    .dw 0x3BD2 ;
    .dw 0x3BD3 ;
    .dw 0x3BD4 ;
    .dw 0x3BD5 ;
    .dw 0x3BD6 ;
    .dw 0x3BD7 ;
    .dw 0x3BD8 ;
    .dw 0x3BD9 ;
    .dw 0x3BDA ;
    .dw 0x3BDB ;
    .dw 0x3BDC ;
    .dw 0x3BDD ;
    .dw 0x3BDE ;
    .dw 0x3BDF ;
    .dw 0x3BE0 ;
    .dw 0x3BE1 ;
    .dw 0x3BE2 ;
    .dw 0x3BE3 ;
    .dw 0x3BE4 ;
    .dw 0x3BE5 ;
    .dw 0x3BE6 ;
    .dw 0x3BE7 ;
    .dw 0x3BE8 ;
    .dw 0x3BE9 ;
    .dw 0x3BEA ;
    .dw 0x3BEB ;
    .dw 0x3BEC ;
    .dw 0x3BED ;
    .dw 0x3BEE ;
    .dw 0x3BEF ;
    .dw 0x3BF0 ;
    .dw 0x3BF1 ;
    .dw 0x3BF2 ;
    .dw 0x3BF3 ;
    .dw 0x3BF4 ;
    .dw 0x3BF5 ;
    .dw 0x3BF6 ;
    .dw 0x3BF7 ;
    .dw 0x3BF8 ;
    .dw 0x3BF9 ;
    .dw 0x3BFA ;
    .dw 0x3BFB ;
    .dw 0x3BFC ;
    .dw 0x3BFD ;
    .dw 0x3BFE ;
    .dw 0x3BFF ;
    .dw 0x3140 ;
    .dw 0x3141 ;
    .dw 0x3142 ;
    .dw 0x3143 ;
    .dw 0x3144 ;
    .dw 0x3145 ;
    .dw 0x3146 ;
    .dw 0x3147 ;
    .dw 0x3148 ;
    .dw 0x3149 ;
    .dw 0x314A ;
    .dw 0x314B ;
    .dw 0x314C ;
    .dw 0x314D ;
    .dw 0x314E ;
    .dw 0x314F ;
    .dw 0x3150 ;
    .dw 0x3151 ;
    .dw 0x3152 ;
    .dw 0x3153 ;
    .dw 0x3154 ;
    .dw 0x3155 ;
    .dw 0x3156 ;
    .dw 0x3157 ;
    .dw 0x3158 ;
    .dw 0x3159 ;
    .dw 0x315A ;
    .dw 0x315B ;
    .dw 0x315C ;
    .dw 0x315D ;
    .dw 0x315E ;
    .dw 0x315F ;
    .dw 0x3160 ;
    .dw 0x3161 ;
    .dw 0x3162 ;
    .dw 0x3163 ;
    .dw 0x3164 ;
    .dw 0x3165 ;
    .dw 0x3166 ;
    .dw 0x3167 ;
    .dw 0x3168 ;
    .dw 0x3169 ;
    .dw 0x316A ;
    .dw 0x316B ;
    .dw 0x316C ;
    .dw 0x316D ;
    .dw 0x316E ;
    .dw 0x316F ;
    .dw 0x3170 ;
    .dw 0x3171 ;
    .dw 0x3172 ;
    .dw 0x3173 ;
    .dw 0x3174 ;
    .dw 0x3175 ;
    .dw 0x3176 ;
    .dw 0x3177 ;
    .dw 0x3178 ;
    .dw 0x3179 ;
    .dw 0x317A ;
    .dw 0x317B ;
    .dw 0x317C ;
    .dw 0x317D ;
    .dw 0x317E ;
    .dw 0x317F ;
    .dw 0x3340 ;
    .dw 0x3341 ;
    .dw 0x3342 ;
    .dw 0x3343 ;
    .dw 0x3344 ;
    .dw 0x3345 ;
    .dw 0x3346 ;
    .dw 0x3347 ;
    .dw 0x3348 ;
    .dw 0x3349 ;
    .dw 0x334A ;
    .dw 0x334B ;
    .dw 0x334C ;
    .dw 0x334D ;
    .dw 0x334E ;
    .dw 0x334F ;
    .dw 0x3350 ;
    .dw 0x3351 ;
    .dw 0x3352 ;
    .dw 0x3353 ;
    .dw 0x3354 ;
    .dw 0x3355 ;
    .dw 0x3356 ;
    .dw 0x3357 ;
    .dw 0x3358 ;
    .dw 0x3359 ;
    .dw 0x335A ;
    .dw 0x335B ;
    .dw 0x335C ;
    .dw 0x335D ;
    .dw 0x335E ;
    .dw 0x335F ;
    .dw 0x3360 ;
    .dw 0x3361 ;
    .dw 0x3362 ;
    .dw 0x3363 ;
    .dw 0x3364 ;
    .dw 0x3365 ;
    .dw 0x3366 ;
    .dw 0x3367 ;
    .dw 0x3368 ;
    .dw 0x3369 ;
    .dw 0x336A ;
    .dw 0x336B ;
    .dw 0x336C ;
    .dw 0x336D ;
    .dw 0x336E ;
    .dw 0x336F ;
    .dw 0x3370 ;
    .dw 0x3371 ;
    .dw 0x3372 ;
    .dw 0x3373 ;
    .dw 0x3374 ;
    .dw 0x3375 ;
    .dw 0x3376 ;
    .dw 0x3377 ;
    .dw 0x3378 ;
    .dw 0x3379 ;
    .dw 0x337A ;
    .dw 0x337B ;
    .dw 0x337C ;
    .dw 0x337D ;
    .dw 0x337E ;
    .dw 0x337F ;
    .dw 0x3540 ;
    .dw 0x3541 ;
    .dw 0x3542 ;
    .dw 0x3543 ;
    .dw 0x3544 ;
    .dw 0x3545 ;
    .dw 0x3546 ;
    .dw 0x3547 ;
    .dw 0x3548 ;
    .dw 0x3549 ;
    .dw 0x354A ;
    .dw 0x354B ;
    .dw 0x354C ;
    .dw 0x354D ;
    .dw 0x354E ;
    .dw 0x354F ;
    .dw 0x3550 ;
    .dw 0x3551 ;
    .dw 0x3552 ;
    .dw 0x3553 ;
    .dw 0x3554 ;
    .dw 0x3555 ;
    .dw 0x3556 ;
    .dw 0x3557 ;
    .dw 0x3558 ;
    .dw 0x3559 ;
    .dw 0x355A ;
    .dw 0x355B ;
    .dw 0x355C ;
    .dw 0x355D ;
    .dw 0x355E ;
    .dw 0x355F ;
    .dw 0x3560 ;
    .dw 0x3561 ;
    .dw 0x3562 ;
    .dw 0x3563 ;
    .dw 0x3564 ;
    .dw 0x3565 ;
    .dw 0x3566 ;
    .dw 0x3567 ;
    .dw 0x3568 ;
    .dw 0x3569 ;
    .dw 0x356A ;
    .dw 0x356B ;
    .dw 0x356C ;
    .dw 0x356D ;
    .dw 0x356E ;
    .dw 0x356F ;
    .dw 0x3570 ;
    .dw 0x3571 ;
    .dw 0x3572 ;
    .dw 0x3573 ;
    .dw 0x3574 ;
    .dw 0x3575 ;
    .dw 0x3576 ;
    .dw 0x3577 ;
    .dw 0x3578 ;
    .dw 0x3579 ;
    .dw 0x357A ;
    .dw 0x357B ;
    .dw 0x357C ;
    .dw 0x357D ;
    .dw 0x357E ;
    .dw 0x357F ;
    .dw 0x3740 ;
    .dw 0x3741 ;
    .dw 0x3742 ;
    .dw 0x3743 ;
    .dw 0x3744 ;
    .dw 0x3745 ;
    .dw 0x3746 ;
    .dw 0x3747 ;
    .dw 0x3748 ;
    .dw 0x3749 ;
    .dw 0x374A ;
    .dw 0x374B ;
    .dw 0x374C ;
    .dw 0x374D ;
    .dw 0x374E ;
    .dw 0x374F ;
    .dw 0x3750 ;
    .dw 0x3751 ;
    .dw 0x3752 ;
    .dw 0x3753 ;
    .dw 0x3754 ;
    .dw 0x3755 ;
    .dw 0x3756 ;
    .dw 0x3757 ;
    .dw 0x3758 ;
    .dw 0x3759 ;
    .dw 0x375A ;
    .dw 0x375B ;
    .dw 0x375C ;
    .dw 0x375D ;
    .dw 0x375E ;
    .dw 0x375F ;
    .dw 0x3760 ;
    .dw 0x3761 ;
    .dw 0x3762 ;
    .dw 0x3763 ;
    .dw 0x3764 ;
    .dw 0x3765 ;
    .dw 0x3766 ;
    .dw 0x3767 ;
    .dw 0x3768 ;
    .dw 0x3769 ;
    .dw 0x376A ;
    .dw 0x376B ;
    .dw 0x376C ;
    .dw 0x376D ;
    .dw 0x376E ;
    .dw 0x376F ;
    .dw 0x3770 ;
    .dw 0x3771 ;
    .dw 0x3772 ;
    .dw 0x3773 ;
    .dw 0x3774 ;
    .dw 0x3775 ;
    .dw 0x3776 ;
    .dw 0x3777 ;
    .dw 0x3778 ;
    .dw 0x3779 ;
    .dw 0x377A ;
    .dw 0x377B ;
    .dw 0x377C ;
    .dw 0x377D ;
    .dw 0x377E ;
    .dw 0x377F ;
    .dw 0x3940 ;
    .dw 0x3941 ;
    .dw 0x3942 ;
    .dw 0x3943 ;
    .dw 0x3944 ;
    .dw 0x3945 ;
    .dw 0x3946 ;
    .dw 0x3947 ;
    .dw 0x3948 ;
    .dw 0x3949 ;
    .dw 0x394A ;
    .dw 0x394B ;
    .dw 0x394C ;
    .dw 0x394D ;
    .dw 0x394E ;
    .dw 0x394F ;
    .dw 0x3950 ;
    .dw 0x3951 ;
    .dw 0x3952 ;
    .dw 0x3953 ;
    .dw 0x3954 ;
    .dw 0x3955 ;
    .dw 0x3956 ;
    .dw 0x3957 ;
    .dw 0x3958 ;
    .dw 0x3959 ;
    .dw 0x395A ;
    .dw 0x395B ;
    .dw 0x395C ;
    .dw 0x395D ;
    .dw 0x395E ;
    .dw 0x395F ;
    .dw 0x3960 ;
    .dw 0x3961 ;
    .dw 0x3962 ;
    .dw 0x3963 ;
    .dw 0x3964 ;
    .dw 0x3965 ;
    .dw 0x3966 ;
    .dw 0x3967 ;
    .dw 0x3968 ;
    .dw 0x3969 ;
    .dw 0x396A ;
    .dw 0x396B ;
    .dw 0x396C ;
    .dw 0x396D ;
    .dw 0x396E ;
    .dw 0x396F ;
    .dw 0x3970 ;
    .dw 0x3971 ;
    .dw 0x3972 ;
    .dw 0x3973 ;
    .dw 0x3974 ;
    .dw 0x3975 ;
    .dw 0x3976 ;
    .dw 0x3977 ;
    .dw 0x3978 ;
    .dw 0x3979 ;
    .dw 0x397A ;
    .dw 0x397B ;
    .dw 0x397C ;
    .dw 0x397D ;
    .dw 0x397E ;
    .dw 0x397F ;
    .dw 0x3D40 ;
    .dw 0x3D41 ;
    .dw 0x3D42 ;
    .dw 0x3D43 ;
    .dw 0x3D44 ;
    .dw 0x3D45 ;
    .dw 0x3D46 ;
    .dw 0x3D47 ;
    .dw 0x3D48 ;
    .dw 0x3D49 ;
    .dw 0x3D4A ;
    .dw 0x3D4B ;
    .dw 0x3D4C ;
    .dw 0x3D4D ;
    .dw 0x3D4E ;
    .dw 0x3D4F ;
    .dw 0x3D50 ;
    .dw 0x3D51 ;
    .dw 0x3D52 ;
    .dw 0x3D53 ;
    .dw 0x3D54 ;
    .dw 0x3D55 ;
    .dw 0x3D56 ;
    .dw 0x3D57 ;
    .dw 0x3D58 ;
    .dw 0x3D59 ;
    .dw 0x3D5A ;
    .dw 0x3D5B ;
    .dw 0x3D5C ;
    .dw 0x3D5D ;
    .dw 0x3D5E ;
    .dw 0x3D5F ;
    .dw 0x3D60 ;
    .dw 0x3D61 ;
    .dw 0x3D62 ;
    .dw 0x3D63 ;
    .dw 0x3D64 ;
    .dw 0x3D65 ;
    .dw 0x3D66 ;
    .dw 0x3D67 ;
    .dw 0x3D68 ;
    .dw 0x3D69 ;
    .dw 0x3D6A ;
    .dw 0x3D6B ;
    .dw 0x3D6C ;
    .dw 0x3D6D ;
    .dw 0x3D6E ;
    .dw 0x3D6F ;
    .dw 0x3D70 ;
    .dw 0x3D71 ;
    .dw 0x3D72 ;
    .dw 0x3D73 ;
    .dw 0x3D74 ;
    .dw 0x3D75 ;
    .dw 0x3D76 ;
    .dw 0x3D77 ;
    .dw 0x3D78 ;
    .dw 0x3D79 ;
    .dw 0x3D7A ;
    .dw 0x3D7B ;
    .dw 0x3D7C ;
    .dw 0x3D7D ;
    .dw 0x3D7E ;
    .dw 0x3D7F ;
    .dw 0x3F40 ;
    .dw 0x3F41 ;
    .dw 0x3F42 ;
    .dw 0x3F43 ;
    .dw 0x3F44 ;
    .dw 0x3F45 ;
    .dw 0x3F46 ;
    .dw 0x3F47 ;
    .dw 0x3F48 ;
    .dw 0x3F49 ;
    .dw 0x3F4A ;
    .dw 0x3F4B ;
    .dw 0x3F4C ;
    .dw 0x3F4D ;
    .dw 0x3F4E ;
    .dw 0x3F4F ;
    .dw 0x3F50 ;
    .dw 0x3F51 ;
    .dw 0x3F52 ;
    .dw 0x3F53 ;
    .dw 0x3F54 ;
    .dw 0x3F55 ;
    .dw 0x3F56 ;
    .dw 0x3F57 ;
    .dw 0x3F58 ;
    .dw 0x3F59 ;
    .dw 0x3F5A ;
    .dw 0x3F5B ;
    .dw 0x3F5C ;
    .dw 0x3F5D ;
    .dw 0x3F5E ;
    .dw 0x3F5F ;
    .dw 0x3F60 ;
    .dw 0x3F61 ;
    .dw 0x3F62 ;
    .dw 0x3F63 ;
    .dw 0x3F64 ;
    .dw 0x3F65 ;
    .dw 0x3F66 ;
    .dw 0x3F67 ;
    .dw 0x3F68 ;
    .dw 0x3F69 ;
    .dw 0x3F6A ;
    .dw 0x3F6B ;
    .dw 0x3F6C ;
    .dw 0x3F6D ;
    .dw 0x3F6E ;
    .dw 0x3F6F ;
    .dw 0x3F70 ;
    .dw 0x3F71 ;
    .dw 0x3F72 ;
    .dw 0x3F73 ;
    .dw 0x3F74 ;
    .dw 0x3F75 ;
    .dw 0x3F76 ;
    .dw 0x3F77 ;
    .dw 0x3F78 ;
    .dw 0x3F79 ;
    .dw 0x3F7A ;
    .dw 0x3F7B ;
    .dw 0x3F7C ;
    .dw 0x3F7D ;
    .dw 0x3F7E ;
    .dw 0x3F7F ;
    .dw 0x3104 ;
    .dw 0x3105 ;
    .dw 0x310C ;
    .dw 0x310D ;
    .dw 0x3114 ;
    .dw 0x3115 ;
    .dw 0x311C ;
    .dw 0x311D ;
    .dw 0x3124 ;
    .dw 0x3125 ;
    .dw 0x312C ;
    .dw 0x312D ;
    .dw 0x3134 ;
    .dw 0x3135 ;
    .dw 0x313C ;
    .dw 0x313D ;
    .dw 0x3304 ;
    .dw 0x3305 ;
    .dw 0x330C ;
    .dw 0x330D ;
    .dw 0x3314 ;
    .dw 0x3315 ;
    .dw 0x331C ;
    .dw 0x331D ;
    .dw 0x3324 ;
    .dw 0x3325 ;
    .dw 0x332C ;
    .dw 0x332D ;
    .dw 0x3334 ;
    .dw 0x3335 ;
    .dw 0x333C ;
    .dw 0x333D ;
    .dw 0x3504 ;
    .dw 0x3505 ;
    .dw 0x350C ;
    .dw 0x350D ;
    .dw 0x3514 ;
    .dw 0x3515 ;
    .dw 0x351C ;
    .dw 0x351D ;
    .dw 0x3524 ;
    .dw 0x3525 ;
    .dw 0x352C ;
    .dw 0x352D ;
    .dw 0x3534 ;
    .dw 0x3535 ;
    .dw 0x353C ;
    .dw 0x353D ;
    .dw 0x3704 ;
    .dw 0x3705 ;
    .dw 0x370C ;
    .dw 0x370D ;
    .dw 0x3714 ;
    .dw 0x3715 ;
    .dw 0x371C ;
    .dw 0x371D ;
    .dw 0x3724 ;
    .dw 0x3725 ;
    .dw 0x372C ;
    .dw 0x372D ;
    .dw 0x3734 ;
    .dw 0x3735 ;
    .dw 0x373C ;
    .dw 0x373D ;
    .dw 0x3904 ;
    .dw 0x3905 ;
    .dw 0x390C ;
    .dw 0x390D ;
    .dw 0x3914 ;
    .dw 0x3915 ;
    .dw 0x391C ;
    .dw 0x391D ;
    .dw 0x3924 ;
    .dw 0x3925 ;
    .dw 0x392C ;
    .dw 0x392D ;
    .dw 0x3934 ;
    .dw 0x3935 ;
    .dw 0x393C ;
    .dw 0x393D ;
    .dw 0x3D04 ;
    .dw 0x3D05 ;
    .dw 0x3D0C ;
    .dw 0x3D0D ;
    .dw 0x3D14 ;
    .dw 0x3D15 ;
    .dw 0x3D1C ;
    .dw 0x3D1D ;
    .dw 0x3D24 ;
    .dw 0x3D25 ;
    .dw 0x3D2C ;
    .dw 0x3D2D ;
    .dw 0x3D34 ;
    .dw 0x3D35 ;
    .dw 0x3D3C ;
    .dw 0x3D3D ;
    .dw 0x3F04 ;
    .dw 0x3F05 ;
    .dw 0x3F0C ;
    .dw 0x3F0D ;
    .dw 0x3F14 ;
    .dw 0x3F15 ;
    .dw 0x3F1C ;
    .dw 0x3F1D ;
    .dw 0x3F24 ;
    .dw 0x3F25 ;
    .dw 0x3F2C ;
    .dw 0x3F2D ;
    .dw 0x3F34 ;
    .dw 0x3F35 ;
    .dw 0x3F3C ;
    .dw 0x3F3D ;
    .dw 0x3820 ;
    .dw 0x3821 ;
    .dw 0x3822 ;
    .dw 0x3823 ;
    .dw 0x3824 ;
    .dw 0x3825 ;
    .dw 0x3826 ;
    .dw 0x3827 ;
    .dw 0x3828 ;
    .dw 0x3829 ;
    .dw 0x382A ;
    .dw 0x382B ;
    .dw 0x382C ;
    .dw 0x382D ;
    .dw 0x382E ;
    .dw 0x382F ;
    .dw 0x3860 ;
    .dw 0x3861 ;
    .dw 0x3862 ;
    .dw 0x3863 ;
    .dw 0x3864 ;
    .dw 0x3865 ;
    .dw 0x3866 ;
    .dw 0x3867 ;
    .dw 0x3868 ;
    .dw 0x3869 ;
    .dw 0x386A ;
    .dw 0x386B ;
    .dw 0x386C ;
    .dw 0x386D ;
    .dw 0x386E ;
    .dw 0x386F ;
    .dw 0x38A0 ;
    .dw 0x38A1 ;
    .dw 0x38A2 ;
    .dw 0x38A3 ;
    .dw 0x38A4 ;
    .dw 0x38A5 ;
    .dw 0x38A6 ;
    .dw 0x38A7 ;
    .dw 0x38A8 ;
    .dw 0x38A9 ;
    .dw 0x38AA ;
    .dw 0x38AB ;
    .dw 0x38AC ;
    .dw 0x38AD ;
    .dw 0x38AE ;
    .dw 0x38AF ;
    .dw 0x38E0 ;
    .dw 0x38E1 ;
    .dw 0x38E2 ;
    .dw 0x38E3 ;
    .dw 0x38E4 ;
    .dw 0x38E5 ;
    .dw 0x38E6 ;
    .dw 0x38E7 ;
    .dw 0x38E8 ;
    .dw 0x38E9 ;
    .dw 0x38EA ;
    .dw 0x38EB ;
    .dw 0x38EC ;
    .dw 0x38ED ;
    .dw 0x38EE ;
    .dw 0x38EF ;
    .dw 0x3920 ;
    .dw 0x3921 ;
    .dw 0x3922 ;
    .dw 0x3923 ;
    .dw 0x3924 ;
    .dw 0x3925 ;
    .dw 0x3926 ;
    .dw 0x3927 ;
    .dw 0x3928 ;
    .dw 0x3929 ;
    .dw 0x392A ;
    .dw 0x392B ;
    .dw 0x392C ;
    .dw 0x392D ;
    .dw 0x392E ;
    .dw 0x392F ;
    .dw 0x39A0 ;
    .dw 0x39A1 ;
    .dw 0x39A2 ;
    .dw 0x39A3 ;
    .dw 0x39A4 ;
    .dw 0x39A5 ;
    .dw 0x39A6 ;
    .dw 0x39A7 ;
    .dw 0x39A8 ;
    .dw 0x39A9 ;
    .dw 0x39AA ;
    .dw 0x39AB ;
    .dw 0x39AC ;
    .dw 0x39AD ;
    .dw 0x39AE ;
    .dw 0x39AF ;
    .dw 0x39E0 ;
    .dw 0x39E1 ;
    .dw 0x39E2 ;
    .dw 0x39E3 ;
    .dw 0x39E4 ;
    .dw 0x39E5 ;
    .dw 0x39E6 ;
    .dw 0x39E7 ;
    .dw 0x39E8 ;
    .dw 0x39E9 ;
    .dw 0x39EA ;
    .dw 0x39EB ;
    .dw 0x39EC ;
    .dw 0x39ED ;
    .dw 0x39EE ;
    .dw 0x39EF ;
#if 0
	// EMUDAT = Dreg; is valid
    .dw 0x3E38 ;
    .dw 0x3E39 ;
    .dw 0x3E3A ;
    .dw 0x3E3B ;
    .dw 0x3E3C ;
    .dw 0x3E3D ;
    .dw 0x3E3E ;
    .dw 0x3E3F ;
	// EMUDAT = Preg; is valid
    .dw 0x3E78 ;
    .dw 0x3E79 ;
    .dw 0x3E7A ;
    .dw 0x3E7B ;
    .dw 0x3E7C ;
    .dw 0x3E7D ;
    .dw 0x3E7E ;
    .dw 0x3E7F ;
	// EMUDAT = Ireg; is valid
    .dw 0x3EB8 ;
    .dw 0x3EB9 ;
    .dw 0x3EBA ;
    .dw 0x3EBB ;
	// EMUDAT = Mreg; is valid
    .dw 0x3EBC ;
    .dw 0x3EBD ;
    .dw 0x3EBE ;
    .dw 0x3EBF ;
	// EMUDAT = Breg; is valid
    .dw 0x3EF8 ;
    .dw 0x3EF9 ;
    .dw 0x3EFA ;
    .dw 0x3EFB ;
	// EMUDAT = Lreg; is valid
    .dw 0x3EFC ;
    .dw 0x3EFD ;
    .dw 0x3EFE ;
    .dw 0x3EFF ;
	// EMUDAT = Areg; is valid
    .dw 0x3F38 ;
    .dw 0x3F39 ;
    .dw 0x3F3A ;
    .dw 0x3F3B ;
#endif
    .dw 0x3F3C ;
    .dw 0x3F3D ;
#if 0
	// EMUDAT = ASTAT; is valid
    .dw 0x3F3E ;
	// EMUDAT = RETS; is valid
    .dw 0x3F3F ;
	// EMUDAT = loopregs; is valid
    .dw 0x3FB8 ;
    .dw 0x3FB9 ;
    .dw 0x3FBA ;
    .dw 0x3FBB ;
    .dw 0x3FBC ;
    .dw 0x3FBD ;
	// EMUDAT = cycles; is valid
    .dw 0x3FBE ;
    .dw 0x3FBF ;
	// EMUDAT = USP; is valid
    .dw 0x3FF8 ;
	// EMUDAT = SEQSTAT; is valid
    .dw 0x3FF9 ;
	// EMUDAT = SYSCFG; is valid
    .dw 0x3FFA ;
	// EMDUAT = RET[IXNE]; is valid
    .dw 0x3FFB ;
    .dw 0x3FFC ;
    .dw 0x3FFD ;
    .dw 0x3FFE ;
	// EMUDAT = EMUDAT; is valid
    .dw 0x3FFF ;
	// Dreg = EMUDAT; is valid
    .dw 0x31C7 ;
    .dw 0x31CF ;
    .dw 0x31D7 ;
    .dw 0x31DF ;
#if 0
	// R4 = EMUDAT; breaks the test
    .dw 0x31E7 ;
	// R5 = EMUDAT; breaks the test
    .dw 0x31EF ;
#endif
    .dw 0x31F7 ;
    .dw 0x31FF ;
	// Preg = EMUDAT; is valid
    .dw 0x33C7 ;
    .dw 0x33CF ;
    .dw 0x33D7 ;
    .dw 0x33DF ;
    .dw 0x33E7 ;
    .dw 0x33EF ;
    .dw 0x33F7 ;
    .dw 0x33FF ;
	// Ireg = EMUDAT; is valid
    .dw 0x35C7 ;
    .dw 0x35CF ;
    .dw 0x35D7 ;
    .dw 0x35DF ;
	// Mreg = EMUDAT; is valid
    .dw 0x35E7 ;
    .dw 0x35EF ;
    .dw 0x35F7 ;
    .dw 0x35FF ;
	// EMUDAT = Breg; is valid
    .dw 0x37C7 ;
    .dw 0x37CF ;
    .dw 0x37D7 ;
    .dw 0x37DF ;
	// EMUDAT = Lreg; is valid
    .dw 0x37E7 ;
    .dw 0x37EF ;
    .dw 0x37F7 ;
    .dw 0x37FF ;
#endif
    .dw 0x39C7 ;
    .dw 0x39CF ;
    .dw 0x39D7 ;
    .dw 0x39DF ;
    .dw 0x39E7 ;
    .dw 0x39EF ;
#if 0
	// ASTAT = EMUDAT; is valid
    .dw 0x39F7 ;
	// RETS = EMUDAT; is valid
    .dw 0x39FF ;
	// loopregs = EMUDAT; is valid
    .dw 0x3DC7 ;
    .dw 0x3DCF ;
    .dw 0x3DD7 ;
    .dw 0x3DDF ;
    .dw 0x3DE7 ;
    .dw 0x3DEF ;
	// cycles = EMUDAT; is valid
    .dw 0x3DF7 ;
    .dw 0x3DFF ;
	// USP = EMUDAT; is valid
    .dw 0x3FC7 ;
	// SEQSTAT = EMUDAT; is valid
    .dw 0x3FCF ;
	// SYSCFG = EMUDAT; is valid
    .dw 0x3FD7 ;
	// RET[IXNE] = EMUDAT; is valid
    .dw 0x3FDF ;
    .dw 0x3FE7 ;
    .dw 0x3FEF ;
    .dw 0x3FF7 ;
	// EMUDAT = EMUDAT; is valid
    .dw 0x3FFF ;
#endif
    .dw 0x3D80 ;
    .dw 0x3D81 ;
    .dw 0x3D82 ;
    .dw 0x3D83 ;
    .dw 0x3D84 ;
    .dw 0x3D85 ;
    .dw 0x3D86 ;
    .dw 0x3D87 ;
    .dw 0x3D88 ;
    .dw 0x3D89 ;
    .dw 0x3D8A ;
    .dw 0x3D8B ;
    .dw 0x3D8C ;
    .dw 0x3D8D ;
    .dw 0x3D8E ;
    .dw 0x3D8F ;
    .dw 0x3D90 ;
    .dw 0x3D91 ;
    .dw 0x3D92 ;
    .dw 0x3D93 ;
    .dw 0x3D94 ;
    .dw 0x3D95 ;
    .dw 0x3D96 ;
    .dw 0x3D97 ;
    .dw 0x3D98 ;
    .dw 0x3D99 ;
    .dw 0x3D9A ;
    .dw 0x3D9B ;
    .dw 0x3D9C ;
    .dw 0x3D9D ;
    .dw 0x3D9E ;
    .dw 0x3D9F ;
    .dw 0x3DA0 ;
    .dw 0x3DA1 ;
    .dw 0x3DA2 ;
    .dw 0x3DA3 ;
    .dw 0x3DA4 ;
    .dw 0x3DA5 ;
    .dw 0x3DA6 ;
    .dw 0x3DA7 ;
    .dw 0x3DA8 ;
    .dw 0x3DA9 ;
    .dw 0x3DAA ;
    .dw 0x3DAB ;
    .dw 0x3DAC ;
    .dw 0x3DAD ;
    .dw 0x3DAE ;
    .dw 0x3DAF ;
    .dw 0x3DB0 ;
    .dw 0x3DB1 ;
    .dw 0x3DB2 ;
    .dw 0x3DB3 ;
    .dw 0x3DB4 ;
    .dw 0x3DB5 ;
    .dw 0x3DB6 ;
    .dw 0x3DB7 ;
    .dw 0x3DB8 ;
    .dw 0x3DB9 ;
    .dw 0x3DBA ;
    .dw 0x3DBB ;
    .dw 0x3DBC ;
    .dw 0x3DBD ;
    .dw 0x3DBE ;
    .dw 0x3DBF ;
    .dw 0x3DC1 ;
    .dw 0x3DC2 ;
    .dw 0x3DC3 ;
    .dw 0x3DC4 ;
    .dw 0x3DC5 ;
    .dw 0x3DC6 ;
#if 0
	// loopregs = EMUDAT; is valid
    .dw 0x3DC7 ;
#endif
    .dw 0x3DC9 ;
    .dw 0x3DCA ;
    .dw 0x3DCB ;
    .dw 0x3DCC ;
    .dw 0x3DCD ;
    .dw 0x3DCE ;
#if 0
	// loopregs = EMUDAT; is valid
    .dw 0x3DCF ;
#endif
    .dw 0x3DD1 ;
    .dw 0x3DD2 ;
    .dw 0x3DD3 ;
    .dw 0x3DD4 ;
    .dw 0x3DD5 ;
    .dw 0x3DD6 ;
#if 0
	// loopregs = EMUDAT; is valid
    .dw 0x3DD7 ;
#endif
    .dw 0x3DD9 ;
    .dw 0x3DDA ;
    .dw 0x3DDB ;
    .dw 0x3DDC ;
    .dw 0x3DDD ;
    .dw 0x3DDE ;
#if 0
	// loopregs = EMUDAT; is valid
    .dw 0x3DDF ;
#endif
    .dw 0x3DE1 ;
    .dw 0x3DE2 ;
    .dw 0x3DE3 ;
    .dw 0x3DE4 ;
    .dw 0x3DE5 ;
    .dw 0x3DE6 ;
#if 0
	// loopregs = EMUDAT; is valid
    .dw 0x3DE7 ;
#endif
    .dw 0x3DE9 ;
    .dw 0x3DEA ;
    .dw 0x3DEB ;
    .dw 0x3DEC ;
    .dw 0x3DED ;
    .dw 0x3DEE ;
#if 0
	// loopregs = EMUDAT; is valid
    .dw 0x3DEF ;
#endif
    .dw 0x3DF1 ;
    .dw 0x3DF2 ;
    .dw 0x3DF3 ;
    .dw 0x3DF4 ;
    .dw 0x3DF5 ;
    .dw 0x3DF6 ;
#if 0
	// cycles = EMUDAT; is valid
    .dw 0x3DF7 ;
#endif
    .dw 0x3DF9 ;
    .dw 0x3DFA ;
    .dw 0x3DFB ;
    .dw 0x3DFC ;
    .dw 0x3DFD ;
    .dw 0x3DFE ;
#if 0
	// cycles = EMUDAT; is valid
    .dw 0x3DFF ;
#endif
    .dw 0x3F88 ;
    .dw 0x3F89 ;
    .dw 0x3F8A ;
    .dw 0x3F8B ;
    .dw 0x3F8C ;
    .dw 0x3F8D ;
    .dw 0x3F8E ;
    .dw 0x3F8F ;
    .dw 0x3F90 ;
    .dw 0x3F91 ;
    .dw 0x3F92 ;
    .dw 0x3F93 ;
    .dw 0x3F94 ;
    .dw 0x3F95 ;
    .dw 0x3F96 ;
    .dw 0x3F97 ;
    .dw 0x3F98 ;
    .dw 0x3F99 ;
    .dw 0x3F9A ;
    .dw 0x3F9B ;
    .dw 0x3F9C ;
    .dw 0x3F9D ;
    .dw 0x3F9E ;
    .dw 0x3F9F ;
    .dw 0x3FA0 ;
    .dw 0x3FA1 ;
    .dw 0x3FA2 ;
    .dw 0x3FA3 ;
    .dw 0x3FA4 ;
    .dw 0x3FA5 ;
    .dw 0x3FA6 ;
    .dw 0x3FA7 ;
    .dw 0x3FA8 ;
    .dw 0x3FA9 ;
    .dw 0x3FAA ;
    .dw 0x3FAB ;
    .dw 0x3FAC ;
    .dw 0x3FAD ;
    .dw 0x3FAE ;
    .dw 0x3FAF ;
    .dw 0x3FB0 ;
    .dw 0x3FB1 ;
    .dw 0x3FB2 ;
    .dw 0x3FB3 ;
    .dw 0x3FB4 ;
    .dw 0x3FB5 ;
    .dw 0x3FB6 ;
    .dw 0x3FB7 ;
#if 0
	// EMUDAT = loopregs; is valid
    .dw 0x3FB8 ;
    .dw 0x3FB9 ;
    .dw 0x3FBA ;
    .dw 0x3FBB ;
    .dw 0x3FBC ;
    .dw 0x3FBD ;
	// EMUDAT = cycles; is valid
    .dw 0x3FBE ;
    .dw 0x3FBF ;
#endif
    .dw 0x3FC9 ;
    .dw 0x3FCA ;
    .dw 0x3FCB ;
    .dw 0x3FCC ;
    .dw 0x3FCD ;
    .dw 0x3FCE ;
#if 0
	// SEQSTAT = EMUDAT; is valid
    .dw 0x3FCF ;
#endif
    .dw 0x3FD1 ;
    .dw 0x3FD2 ;
    .dw 0x3FD3 ;
    .dw 0x3FD4 ;
    .dw 0x3FD5 ;
    .dw 0x3FD6 ;
#if 0
	// SYSCFG = EMUDAT; is valid
    .dw 0x3FD7 ;
#endif
    .dw 0x3FD9 ;
    .dw 0x3FDA ;
    .dw 0x3FDB ;
    .dw 0x3FDC ;
    .dw 0x3FDD ;
    .dw 0x3FDE ;
#if 0
	// RET[IXNE] = EMUDAT; is valid
    .dw 0x3FDF ;
#endif
    .dw 0x3FE1 ;
    .dw 0x3FE2 ;
    .dw 0x3FE3 ;
    .dw 0x3FE4 ;
    .dw 0x3FE5 ;
    .dw 0x3FE6 ;
#if 0
	// RET[IXNE] = EMUDAT; is valid
    .dw 0x3FE7 ;
#endif
    .dw 0x3FE9 ;
    .dw 0x3FEA ;
    .dw 0x3FEB ;
    .dw 0x3FEC ;
    .dw 0x3FED ;
    .dw 0x3FEE ;
#if 0
	// RET[IXNE] = EMUDAT; is valid
    .dw 0x3FEF ;
#endif
    .dw 0x3FF1 ;
    .dw 0x3FF2 ;
    .dw 0x3FF3 ;
    .dw 0x3FF4 ;
    .dw 0x3FF5 ;
    .dw 0x3FF6 ;
#if 0
	// RET[IXNE] = EMUDAT; is valid
    .dw 0x3FF7 ;
	// EMUDAT = SEQSTAT; is valid
    .dw 0x3FF9 ;
	// EMUDAT = SYSCFG; is valid
    .dw 0x3FFA ;
	// EMDUAT = RET[IXNE]; is valid
    .dw 0x3FFB ;
    .dw 0x3FFC ;
    .dw 0x3FFD ;
    .dw 0x3FFE ;
	// EMUDAT = EMUDAT; is valid
    .dw 0x3FFF ;
#endif
    .dw 0x39B0 ;
    .dw 0x39B1 ;
    .dw 0x39B2 ;
    .dw 0x39B3 ;
    .dw 0x39B4 ;
    .dw 0x39B5 ;
    .dw 0x39B6 ;
    .dw 0x39B7 ;
    .dw 0x39B8 ;
    .dw 0x39B9 ;
    .dw 0x39BA ;
    .dw 0x39BB ;
    .dw 0x39BC ;
    .dw 0x39BD ;
    .dw 0x39BE ;
    .dw 0x39BF ;
    .dw 0x39F1 ;
    .dw 0x39F2 ;
    .dw 0x39F3 ;
    .dw 0x39F4 ;
    .dw 0x39F5 ;
    .dw 0x39F6 ;
#if 0
	// ASTAT = EMUDAT; is valid
    .dw 0x39F7 ;
#endif
    .dw 0x39F9 ;
    .dw 0x39FA ;
    .dw 0x39FB ;
    .dw 0x39FC ;
    .dw 0x39FD ;
    .dw 0x39FE ;
#if 0
	// RETS = EMUDAT; is valid
    .dw 0x39FF ;
#endif
    .dw 0x3D06 ;
    .dw 0x3D07 ;
    .dw 0x3D0E ;
    .dw 0x3D0F ;
    .dw 0x3D16 ;
    .dw 0x3D17 ;
    .dw 0x3D1E ;
    .dw 0x3D1F ;
    .dw 0x3D26 ;
    .dw 0x3D27 ;
    .dw 0x3D2E ;
    .dw 0x3D2F ;
    .dw 0x3D36 ;
    .dw 0x3D37 ;
    .dw 0x3D3E ;
    .dw 0x3D3F ;
    .dw 0x3F0E ;
    .dw 0x3F0F ;
    .dw 0x3F16 ;
    .dw 0x3F17 ;
    .dw 0x3F1E ;
    .dw 0x3F1F ;
    .dw 0x3F26 ;
    .dw 0x3F27 ;
    .dw 0x3F2E ;
    .dw 0x3F2F ;
    .dw 0x3F36 ;
    .dw 0x3F37 ;
#if 0
	// EMUDAT = ASTAT; is valid
    .dw 0x3F3E ;
	// EMUDAT = RETS; is valid
    .dw 0x3F3F ;
#endif
    .dw 0x3936 ;
    .dw 0x3937 ;
    .dw 0x393E ;
    .dw 0x393F ;
    .dw 0x3C80 ;
    .dw 0x3C81 ;
    .dw 0x3C82 ;
    .dw 0x3C83 ;
    .dw 0x3C84 ;
    .dw 0x3C85 ;
    .dw 0x3C86 ;
    .dw 0x3C87 ;
    .dw 0x3C88 ;
    .dw 0x3C89 ;
    .dw 0x3C8A ;
    .dw 0x3C8B ;
    .dw 0x3C8C ;
    .dw 0x3C8D ;
    .dw 0x3C8E ;
    .dw 0x3C8F ;
    .dw 0x3C90 ;
    .dw 0x3C91 ;
    .dw 0x3C92 ;
    .dw 0x3C93 ;
    .dw 0x3C94 ;
    .dw 0x3C95 ;
    .dw 0x3C96 ;
    .dw 0x3C97 ;
    .dw 0x3C98 ;
    .dw 0x3C99 ;
    .dw 0x3C9A ;
    .dw 0x3C9B ;
    .dw 0x3C9C ;
    .dw 0x3C9D ;
    .dw 0x3C9E ;
    .dw 0x3C9F ;
    .dw 0x3CA0 ;
    .dw 0x3CA1 ;
    .dw 0x3CA2 ;
    .dw 0x3CA3 ;
    .dw 0x3CA4 ;
    .dw 0x3CA5 ;
    .dw 0x3CA6 ;
    .dw 0x3CA7 ;
    .dw 0x3CA8 ;
    .dw 0x3CA9 ;
    .dw 0x3CAA ;
    .dw 0x3CAB ;
    .dw 0x3CAC ;
    .dw 0x3CAD ;
    .dw 0x3CAE ;
    .dw 0x3CAF ;
    .dw 0x3CB0 ;
    .dw 0x3CB1 ;
    .dw 0x3CB2 ;
    .dw 0x3CB3 ;
    .dw 0x3CB4 ;
    .dw 0x3CB5 ;
    .dw 0x3CB6 ;
    .dw 0x3CB7 ;
    .dw 0x3CB8 ;
    .dw 0x3CB9 ;
    .dw 0x3CBA ;
    .dw 0x3CBB ;
    .dw 0x3CBC ;
    .dw 0x3CBD ;
    .dw 0x3CBE ;
    .dw 0x3CBF ;
    .dw 0x3CC0 ;
    .dw 0x3CC1 ;
    .dw 0x3CC2 ;
    .dw 0x3CC3 ;
    .dw 0x3CC4 ;
    .dw 0x3CC5 ;
    .dw 0x3CC6 ;
    .dw 0x3CC7 ;
    .dw 0x3CC8 ;
    .dw 0x3CC9 ;
    .dw 0x3CCA ;
    .dw 0x3CCB ;
    .dw 0x3CCC ;
    .dw 0x3CCD ;
    .dw 0x3CCE ;
    .dw 0x3CCF ;
    .dw 0x3CD0 ;
    .dw 0x3CD1 ;
    .dw 0x3CD2 ;
    .dw 0x3CD3 ;
    .dw 0x3CD4 ;
    .dw 0x3CD5 ;
    .dw 0x3CD6 ;
    .dw 0x3CD7 ;
    .dw 0x3CD8 ;
    .dw 0x3CD9 ;
    .dw 0x3CDA ;
    .dw 0x3CDB ;
    .dw 0x3CDC ;
    .dw 0x3CDD ;
    .dw 0x3CDE ;
    .dw 0x3CDF ;
    .dw 0x3CE0 ;
    .dw 0x3CE1 ;
    .dw 0x3CE2 ;
    .dw 0x3CE3 ;
    .dw 0x3CE4 ;
    .dw 0x3CE5 ;
    .dw 0x3CE6 ;
    .dw 0x3CE7 ;
    .dw 0x3CE8 ;
    .dw 0x3CE9 ;
    .dw 0x3CEA ;
    .dw 0x3CEB ;
    .dw 0x3CEC ;
    .dw 0x3CED ;
    .dw 0x3CEE ;
    .dw 0x3CEF ;
    .dw 0x3CF0 ;
    .dw 0x3CF1 ;
    .dw 0x3CF2 ;
    .dw 0x3CF3 ;
    .dw 0x3CF4 ;
    .dw 0x3CF5 ;
    .dw 0x3CF6 ;
    .dw 0x3CF7 ;
    .dw 0x3CF8 ;
    .dw 0x3CF9 ;
    .dw 0x3CFA ;
    .dw 0x3CFB ;
    .dw 0x3CFC ;
    .dw 0x3CFD ;
    .dw 0x3CFE ;
    .dw 0x3CFF ;
    .dw 0x3E88 ;
    .dw 0x3E89 ;
    .dw 0x3E8A ;
    .dw 0x3E8B ;
    .dw 0x3E8C ;
    .dw 0x3E8D ;
    .dw 0x3E8E ;
    .dw 0x3E8F ;
    .dw 0x3E90 ;
    .dw 0x3E91 ;
    .dw 0x3E92 ;
    .dw 0x3E93 ;
    .dw 0x3E94 ;
    .dw 0x3E95 ;
    .dw 0x3E96 ;
    .dw 0x3E97 ;
    .dw 0x3E98 ;
    .dw 0x3E99 ;
    .dw 0x3E9A ;
    .dw 0x3E9B ;
    .dw 0x3E9C ;
    .dw 0x3E9D ;
    .dw 0x3E9E ;
    .dw 0x3E9F ;
    .dw 0x3EA0 ;
    .dw 0x3EA1 ;
    .dw 0x3EA2 ;
    .dw 0x3EA3 ;
    .dw 0x3EA4 ;
    .dw 0x3EA5 ;
    .dw 0x3EA6 ;
    .dw 0x3EA7 ;
    .dw 0x3EA8 ;
    .dw 0x3EA9 ;
    .dw 0x3EAA ;
    .dw 0x3EAB ;
    .dw 0x3EAC ;
    .dw 0x3EAD ;
    .dw 0x3EAE ;
    .dw 0x3EAF ;
    .dw 0x3EB0 ;
    .dw 0x3EB1 ;
    .dw 0x3EB2 ;
    .dw 0x3EB3 ;
    .dw 0x3EB4 ;
    .dw 0x3EB5 ;
    .dw 0x3EB6 ;
    .dw 0x3EB7 ;
#if 0
	// EMUDAT = Ireg; is valid
    .dw 0x3EB8 ;
    .dw 0x3EB9 ;
    .dw 0x3EBA ;
    .dw 0x3EBB ;
	// EMUDAT = Mreg; is valid
    .dw 0x3EBC ;
    .dw 0x3EBD ;
    .dw 0x3EBE ;
    .dw 0x3EBF ;
#endif
    .dw 0x3EC8 ;
    .dw 0x3EC9 ;
    .dw 0x3ECA ;
    .dw 0x3ECB ;
    .dw 0x3ECC ;
    .dw 0x3ECD ;
    .dw 0x3ECE ;
    .dw 0x3ECF ;
    .dw 0x3ED0 ;
    .dw 0x3ED1 ;
    .dw 0x3ED2 ;
    .dw 0x3ED3 ;
    .dw 0x3ED4 ;
    .dw 0x3ED5 ;
    .dw 0x3ED6 ;
    .dw 0x3ED7 ;
    .dw 0x3ED8 ;
    .dw 0x3ED9 ;
    .dw 0x3EDA ;
    .dw 0x3EDB ;
    .dw 0x3EDC ;
    .dw 0x3EDD ;
    .dw 0x3EDE ;
    .dw 0x3EDF ;
    .dw 0x3EE0 ;
    .dw 0x3EE1 ;
    .dw 0x3EE2 ;
    .dw 0x3EE3 ;
    .dw 0x3EE4 ;
    .dw 0x3EE5 ;
    .dw 0x3EE6 ;
    .dw 0x3EE7 ;
    .dw 0x3EE8 ;
    .dw 0x3EE9 ;
    .dw 0x3EEA ;
    .dw 0x3EEB ;
    .dw 0x3EEC ;
    .dw 0x3EED ;
    .dw 0x3EEE ;
    .dw 0x3EEF ;
    .dw 0x3EF0 ;
    .dw 0x3EF1 ;
    .dw 0x3EF2 ;
    .dw 0x3EF3 ;
    .dw 0x3EF4 ;
    .dw 0x3EF5 ;
    .dw 0x3EF6 ;
    .dw 0x3EF7 ;
#if 0
	// EMUDAT = Breg; is valid
    .dw 0x3EF8 ;
    .dw 0x3EF9 ;
    .dw 0x3EFA ;
    .dw 0x3EFB ;
	// EMUDAT = Lreg; is valid
    .dw 0x3EFC ;
    .dw 0x3EFD ;
    .dw 0x3EFE ;
    .dw 0x3EFF ;
#endif
    .dw 0x38B0 ;
    .dw 0x38B1 ;
    .dw 0x38B2 ;
    .dw 0x38B3 ;
    .dw 0x38B4 ;
    .dw 0x38B5 ;
    .dw 0x38B6 ;
    .dw 0x38B7 ;
    .dw 0x38B8 ;
    .dw 0x38B9 ;
    .dw 0x38BA ;
    .dw 0x38BB ;
    .dw 0x38BC ;
    .dw 0x38BD ;
    .dw 0x38BE ;
    .dw 0x38BF ;
    .dw 0x38F0 ;
    .dw 0x38F1 ;
    .dw 0x38F2 ;
    .dw 0x38F3 ;
    .dw 0x38F4 ;
    .dw 0x38F5 ;
    .dw 0x38F6 ;
    .dw 0x38F7 ;
    .dw 0x38F8 ;
    .dw 0x38F9 ;
    .dw 0x38FA ;
    .dw 0x38FB ;
    .dw 0x38FC ;
    .dw 0x38FD ;
    .dw 0x38FE ;
    .dw 0x38FF ;
#if 0
	// Preg = sysreg; is valid
    .dw 0x3380 ;
    .dw 0x3381 ;
    .dw 0x3382 ;
    .dw 0x3383 ;
    .dw 0x3384 ;
    .dw 0x3385 ;
    .dw 0x3386 ;
    .dw 0x3387 ;
    .dw 0x3388 ;
    .dw 0x3389 ;
    .dw 0x338A ;
    .dw 0x338B ;
    .dw 0x338C ;
    .dw 0x338D ;
    .dw 0x338E ;
    .dw 0x338F ;
    .dw 0x3390 ;
    .dw 0x3391 ;
    .dw 0x3392 ;
    .dw 0x3393 ;
    .dw 0x3394 ;
    .dw 0x3395 ;
    .dw 0x3396 ;
    .dw 0x3397 ;
    .dw 0x3398 ;
    .dw 0x3399 ;
    .dw 0x339A ;
    .dw 0x339B ;
    .dw 0x339C ;
    .dw 0x339D ;
    .dw 0x339E ;
    .dw 0x339F ;
    .dw 0x33A0 ;
    .dw 0x33A1 ;
    .dw 0x33A2 ;
    .dw 0x33A3 ;
    .dw 0x33A4 ;
    .dw 0x33A5 ;
    .dw 0x33A6 ;
    .dw 0x33A7 ;
    .dw 0x33A8 ;
    .dw 0x33A9 ;
    .dw 0x33AA ;
    .dw 0x33AB ;
    .dw 0x33AC ;
    .dw 0x33AD ;
    .dw 0x33AE ;
    .dw 0x33AF ;
    .dw 0x33B0 ;
    .dw 0x33B1 ;
    .dw 0x33B2 ;
    .dw 0x33B3 ;
    .dw 0x33B4 ;
    .dw 0x33B5 ;
    .dw 0x33B6 ;
    .dw 0x33B7 ;
    .dw 0x33B8 ;
    .dw 0x33B9 ;
    .dw 0x33BA ;
    .dw 0x33BB ;
    .dw 0x33BC ;
    .dw 0x33BD ;
    .dw 0x33BE ;
    .dw 0x33BF ;
    .dw 0x33C1 ;
    .dw 0x33C2 ;
    .dw 0x33C3 ;
    .dw 0x33C4 ;
    .dw 0x33C5 ;
    .dw 0x33C6 ;
    .dw 0x33C7 ;
    .dw 0x33C9 ;
    .dw 0x33CA ;
    .dw 0x33CB ;
    .dw 0x33CC ;
    .dw 0x33CD ;
    .dw 0x33CE ;
    .dw 0x33CF ;
    .dw 0x33D1 ;
    .dw 0x33D2 ;
    .dw 0x33D3 ;
    .dw 0x33D4 ;
    .dw 0x33D5 ;
    .dw 0x33D6 ;
    .dw 0x33D7 ;
    .dw 0x33D9 ;
    .dw 0x33DA ;
    .dw 0x33DB ;
    .dw 0x33DC ;
    .dw 0x33DD ;
    .dw 0x33DE ;
    .dw 0x33DF ;
    .dw 0x33E1 ;
    .dw 0x33E2 ;
    .dw 0x33E3 ;
    .dw 0x33E4 ;
    .dw 0x33E5 ;
    .dw 0x33E6 ;
    .dw 0x33E7 ;
    .dw 0x33E9 ;
    .dw 0x33EA ;
    .dw 0x33EB ;
    .dw 0x33EC ;
    .dw 0x33ED ;
    .dw 0x33EE ;
    .dw 0x33EF ;
    .dw 0x33F1 ;
    .dw 0x33F2 ;
    .dw 0x33F3 ;
    .dw 0x33F4 ;
    .dw 0x33F5 ;
    .dw 0x33F6 ;
    .dw 0x33F7 ;
    .dw 0x33F9 ;
    .dw 0x33FA ;
    .dw 0x33FB ;
    .dw 0x33FC ;
    .dw 0x33FD ;
    .dw 0x33FE ;
    .dw 0x33FF ;
    .dw 0x3306 ;
    .dw 0x3307 ;
    .dw 0x330E ;
    .dw 0x330F ;
    .dw 0x3316 ;
    .dw 0x3317 ;
    .dw 0x331E ;
    .dw 0x331F ;
    .dw 0x3326 ;
    .dw 0x3327 ;
    .dw 0x332E ;
    .dw 0x332F ;
    .dw 0x3336 ;
    .dw 0x3337 ;
    .dw 0x333E ;
    .dw 0x333F ;
#endif
    .dw 0x3580 ;
    .dw 0x3581 ;
    .dw 0x3582 ;
    .dw 0x3583 ;
    .dw 0x3584 ;
    .dw 0x3585 ;
    .dw 0x3586 ;
    .dw 0x3587 ;
    .dw 0x3588 ;
    .dw 0x3589 ;
    .dw 0x358A ;
    .dw 0x358B ;
    .dw 0x358C ;
    .dw 0x358D ;
    .dw 0x358E ;
    .dw 0x358F ;
    .dw 0x3590 ;
    .dw 0x3591 ;
    .dw 0x3592 ;
    .dw 0x3593 ;
    .dw 0x3594 ;
    .dw 0x3595 ;
    .dw 0x3596 ;
    .dw 0x3597 ;
    .dw 0x3598 ;
    .dw 0x3599 ;
    .dw 0x359A ;
    .dw 0x359B ;
    .dw 0x359C ;
    .dw 0x359D ;
    .dw 0x359E ;
    .dw 0x359F ;
    .dw 0x35A0 ;
    .dw 0x35A1 ;
    .dw 0x35A2 ;
    .dw 0x35A3 ;
    .dw 0x35A4 ;
    .dw 0x35A5 ;
    .dw 0x35A6 ;
    .dw 0x35A7 ;
    .dw 0x35A8 ;
    .dw 0x35A9 ;
    .dw 0x35AA ;
    .dw 0x35AB ;
    .dw 0x35AC ;
    .dw 0x35AD ;
    .dw 0x35AE ;
    .dw 0x35AF ;
    .dw 0x35B0 ;
    .dw 0x35B1 ;
    .dw 0x35B2 ;
    .dw 0x35B3 ;
    .dw 0x35B4 ;
    .dw 0x35B5 ;
    .dw 0x35B6 ;
    .dw 0x35B7 ;
    .dw 0x35B8 ;
    .dw 0x35B9 ;
    .dw 0x35BA ;
    .dw 0x35BB ;
    .dw 0x35BC ;
    .dw 0x35BD ;
    .dw 0x35BE ;
    .dw 0x35BF ;
    .dw 0x35C1 ;
    .dw 0x35C2 ;
    .dw 0x35C3 ;
    .dw 0x35C4 ;
    .dw 0x35C5 ;
    .dw 0x35C6 ;
#if 0
	// Ireg = EMUDAT; is valid
    .dw 0x35C7 ;
#endif
    .dw 0x35C9 ;
    .dw 0x35CA ;
    .dw 0x35CB ;
    .dw 0x35CC ;
    .dw 0x35CD ;
    .dw 0x35CE ;
#if 0
	// Ireg = EMUDAT; is valid
    .dw 0x35CF ;
#endif
    .dw 0x35D1 ;
    .dw 0x35D2 ;
    .dw 0x35D3 ;
    .dw 0x35D4 ;
    .dw 0x35D5 ;
    .dw 0x35D6 ;
#if 0
	// Ireg = EMUDAT; is valid
    .dw 0x35D7 ;
#endif
    .dw 0x35D9 ;
    .dw 0x35DA ;
    .dw 0x35DB ;
    .dw 0x35DC ;
    .dw 0x35DD ;
    .dw 0x35DE ;
#if 0
	// Ireg = EMUDAT; is valid
    .dw 0x35DF ;
#endif
    .dw 0x35E1 ;
    .dw 0x35E2 ;
    .dw 0x35E3 ;
    .dw 0x35E4 ;
    .dw 0x35E5 ;
    .dw 0x35E6 ;
#if 0
	// Mreg = EMUDAT; is valid
    .dw 0x35E7 ;
#endif
    .dw 0x35E9 ;
    .dw 0x35EA ;
    .dw 0x35EB ;
    .dw 0x35EC ;
    .dw 0x35ED ;
    .dw 0x35EE ;
#if 0
	// Mreg = EMUDAT; is valid
    .dw 0x35EF ;
#endif
    .dw 0x35F1 ;
    .dw 0x35F2 ;
    .dw 0x35F3 ;
    .dw 0x35F4 ;
    .dw 0x35F5 ;
    .dw 0x35F6 ;
#if 0
	// Mreg = EMUDAT; is valid
    .dw 0x35F7 ;
#endif
    .dw 0x35F9 ;
    .dw 0x35FA ;
    .dw 0x35FB ;
    .dw 0x35FC ;
    .dw 0x35FD ;
    .dw 0x35FE ;
#if 0
	// Mreg = EMUDAT; is valid
    .dw 0x35FF ;
#endif
    .dw 0x3780 ;
    .dw 0x3781 ;
    .dw 0x3782 ;
    .dw 0x3783 ;
    .dw 0x3784 ;
    .dw 0x3785 ;
    .dw 0x3786 ;
    .dw 0x3787 ;
    .dw 0x3788 ;
    .dw 0x3789 ;
    .dw 0x378A ;
    .dw 0x378B ;
    .dw 0x378C ;
    .dw 0x378D ;
    .dw 0x378E ;
    .dw 0x378F ;
    .dw 0x3790 ;
    .dw 0x3791 ;
    .dw 0x3792 ;
    .dw 0x3793 ;
    .dw 0x3794 ;
    .dw 0x3795 ;
    .dw 0x3796 ;
    .dw 0x3797 ;
    .dw 0x3798 ;
    .dw 0x3799 ;
    .dw 0x379A ;
    .dw 0x379B ;
    .dw 0x379C ;
    .dw 0x379D ;
    .dw 0x379E ;
    .dw 0x379F ;
    .dw 0x37A0 ;
    .dw 0x37A1 ;
    .dw 0x37A2 ;
    .dw 0x37A3 ;
    .dw 0x37A4 ;
    .dw 0x37A5 ;
    .dw 0x37A6 ;
    .dw 0x37A7 ;
    .dw 0x37A8 ;
    .dw 0x37A9 ;
    .dw 0x37AA ;
    .dw 0x37AB ;
    .dw 0x37AC ;
    .dw 0x37AD ;
    .dw 0x37AE ;
    .dw 0x37AF ;
    .dw 0x37B0 ;
    .dw 0x37B1 ;
    .dw 0x37B2 ;
    .dw 0x37B3 ;
    .dw 0x37B4 ;
    .dw 0x37B5 ;
    .dw 0x37B6 ;
    .dw 0x37B7 ;
    .dw 0x37B8 ;
    .dw 0x37B9 ;
    .dw 0x37BA ;
    .dw 0x37BB ;
    .dw 0x37BC ;
    .dw 0x37BD ;
    .dw 0x37BE ;
    .dw 0x37BF ;
    .dw 0x37C1 ;
    .dw 0x37C2 ;
    .dw 0x37C3 ;
    .dw 0x37C4 ;
    .dw 0x37C5 ;
    .dw 0x37C6 ;
#if 0
	// EMUDAT = Breg; is valid
    .dw 0x37C7 ;
#endif
    .dw 0x37C9 ;
    .dw 0x37CA ;
    .dw 0x37CB ;
    .dw 0x37CC ;
    .dw 0x37CD ;
    .dw 0x37CE ;
#if 0
	// EMUDAT = Breg; is valid
    .dw 0x37CF ;
#endif
    .dw 0x37D1 ;
    .dw 0x37D2 ;
    .dw 0x37D3 ;
    .dw 0x37D4 ;
    .dw 0x37D5 ;
    .dw 0x37D6 ;
#if 0
	// EMUDAT = Breg; is valid
    .dw 0x37D7 ;
#endif
    .dw 0x37D9 ;
    .dw 0x37DA ;
    .dw 0x37DB ;
    .dw 0x37DC ;
    .dw 0x37DD ;
    .dw 0x37DE ;
#if 0
	// EMUDAT = Breg; is valid
    .dw 0x37DF ;
#endif
    .dw 0x37E1 ;
    .dw 0x37E2 ;
    .dw 0x37E3 ;
    .dw 0x37E4 ;
    .dw 0x37E5 ;
    .dw 0x37E6 ;
#if 0
	// EMUDAT = Lreg; is valid
    .dw 0x37E7 ;
#endif
    .dw 0x37E9 ;
    .dw 0x37EA ;
    .dw 0x37EB ;
    .dw 0x37EC ;
    .dw 0x37ED ;
    .dw 0x37EE ;
#if 0
	// EMUDAT = Lreg; is valid
    .dw 0x37EF ;
#endif
    .dw 0x37F1 ;
    .dw 0x37F2 ;
    .dw 0x37F3 ;
    .dw 0x37F4 ;
    .dw 0x37F5 ;
    .dw 0x37F6 ;
#if 0
	// EMUDAT = Lreg; is valid
    .dw 0x37F7 ;
#endif
    .dw 0x37F9 ;
    .dw 0x37FA ;
    .dw 0x37FB ;
    .dw 0x37FC ;
    .dw 0x37FD ;
    .dw 0x37FE ;
#if 0
	// EMUDAT = Lreg; is valid
    .dw 0x37FF ;
#endif
    .dw 0x3506 ;
    .dw 0x3507 ;
    .dw 0x350E ;
    .dw 0x350F ;
    .dw 0x3516 ;
    .dw 0x3517 ;
    .dw 0x351E ;
    .dw 0x351F ;
    .dw 0x3526 ;
    .dw 0x3527 ;
    .dw 0x352E ;
    .dw 0x352F ;
    .dw 0x3536 ;
    .dw 0x3537 ;
    .dw 0x353E ;
    .dw 0x353F ;
    .dw 0x3706 ;
    .dw 0x3707 ;
    .dw 0x370E ;
    .dw 0x370F ;
    .dw 0x3716 ;
    .dw 0x3717 ;
    .dw 0x371E ;
    .dw 0x371F ;
    .dw 0x3726 ;
    .dw 0x3727 ;
    .dw 0x372E ;
    .dw 0x372F ;
    .dw 0x3736 ;
    .dw 0x3737 ;
    .dw 0x373E ;
    .dw 0x373F ;
    .dw 0x4180 ;
    .dw 0x4181 ;
    .dw 0x4182 ;
    .dw 0x4183 ;
    .dw 0x4184 ;
    .dw 0x4185 ;
    .dw 0x4186 ;
    .dw 0x4187 ;
    .dw 0x4188 ;
    .dw 0x4189 ;
    .dw 0x418A ;
    .dw 0x418B ;
    .dw 0x418C ;
    .dw 0x418D ;
    .dw 0x418E ;
    .dw 0x418F ;
    .dw 0x4190 ;
    .dw 0x4191 ;
    .dw 0x4192 ;
    .dw 0x4193 ;
    .dw 0x4194 ;
    .dw 0x4195 ;
    .dw 0x4196 ;
    .dw 0x4197 ;
    .dw 0x4198 ;
    .dw 0x4199 ;
    .dw 0x419A ;
    .dw 0x419B ;
    .dw 0x419C ;
    .dw 0x419D ;
    .dw 0x419E ;
    .dw 0x419F ;
    .dw 0x41A0 ;
    .dw 0x41A1 ;
    .dw 0x41A2 ;
    .dw 0x41A3 ;
    .dw 0x41A4 ;
    .dw 0x41A5 ;
    .dw 0x41A6 ;
    .dw 0x41A7 ;
    .dw 0x41A8 ;
    .dw 0x41A9 ;
    .dw 0x41AA ;
    .dw 0x41AB ;
    .dw 0x41AC ;
    .dw 0x41AD ;
    .dw 0x41AE ;
    .dw 0x41AF ;
    .dw 0x41B0 ;
    .dw 0x41B1 ;
    .dw 0x41B2 ;
    .dw 0x41B3 ;
    .dw 0x41B4 ;
    .dw 0x41B5 ;
    .dw 0x41B6 ;
    .dw 0x41B7 ;
    .dw 0x41B8 ;
    .dw 0x41B9 ;
    .dw 0x41BA ;
    .dw 0x41BB ;
    .dw 0x41BC ;
    .dw 0x41BD ;
    .dw 0x41BE ;
    .dw 0x41BF ;
    .dw 0x41C0 ;
    .dw 0x41C1 ;
    .dw 0x41C2 ;
    .dw 0x41C3 ;
    .dw 0x41C4 ;
    .dw 0x41C5 ;
    .dw 0x41C6 ;
    .dw 0x41C7 ;
    .dw 0x41C8 ;
    .dw 0x41C9 ;
    .dw 0x41CA ;
    .dw 0x41CB ;
    .dw 0x41CC ;
    .dw 0x41CD ;
    .dw 0x41CE ;
    .dw 0x41CF ;
    .dw 0x41D0 ;
    .dw 0x41D1 ;
    .dw 0x41D2 ;
    .dw 0x41D3 ;
    .dw 0x41D4 ;
    .dw 0x41D5 ;
    .dw 0x41D6 ;
    .dw 0x41D7 ;
    .dw 0x41D8 ;
    .dw 0x41D9 ;
    .dw 0x41DA ;
    .dw 0x41DB ;
    .dw 0x41DC ;
    .dw 0x41DD ;
    .dw 0x41DE ;
    .dw 0x41DF ;
    .dw 0x41E0 ;
    .dw 0x41E1 ;
    .dw 0x41E2 ;
    .dw 0x41E3 ;
    .dw 0x41E4 ;
    .dw 0x41E5 ;
    .dw 0x41E6 ;
    .dw 0x41E7 ;
    .dw 0x41E8 ;
    .dw 0x41E9 ;
    .dw 0x41EA ;
    .dw 0x41EB ;
    .dw 0x41EC ;
    .dw 0x41ED ;
    .dw 0x41EE ;
    .dw 0x41EF ;
    .dw 0x41F0 ;
    .dw 0x41F1 ;
    .dw 0x41F2 ;
    .dw 0x41F3 ;
    .dw 0x41F4 ;
    .dw 0x41F5 ;
    .dw 0x41F6 ;
    .dw 0x41F7 ;
    .dw 0x41F8 ;
    .dw 0x41F9 ;
    .dw 0x41FA ;
    .dw 0x41FB ;
    .dw 0x41FC ;
    .dw 0x41FD ;
    .dw 0x41FE ;
    .dw 0x41FF ;
.ifndef BFIN_HW
	// XXX: These cause double fault on hardware when run in IVG15 !?
    .dw 0x9040 ;
    .dw 0x9049 ;
    .dw 0x9052 ;
    .dw 0x905B ;
    .dw 0x9064 ;
    .dw 0x906D ;
    .dw 0x9076 ;
    .dw 0x907F ;
    .dw 0x90C0 ;
    .dw 0x90C9 ;
    .dw 0x90D2 ;
    .dw 0x90DB ;
    .dw 0x90E4 ;
    .dw 0x90ED ;
    .dw 0x90F6 ;
    .dw 0x90FF ;
.endif
    .dw 0x9180 ;
// Starting 32bit s section  COUNT = 3481

CHECK_INIT_DEF(p0);  //CHECK_INIT(p0, 0xFF7FFFFC);
                    // Xhandler counts all EXCAUSE = 0x21;
.ifndef BFIN_HW
CHECKREG(r5, 2651 - 507); // count of all 16 bit UI's.
.else
CHECKREG(r5, 2651 - 524); // count of all 16 bit UI's.
.endif

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

    // handler MADE LEAN and destructive so test runs more quckly
    //   se_undefinedinstruction1.dsp tests using a "nice" handler

//    [--sp] = ASTAT; // save what we damage
//    [--sp] = (r7 - r6);
    R7 = SEQSTAT;
    R7 <<= 26;
    R7 >>= 26;      // only want EXCAUSE
    R6 = 0x21;      // EXCAUSE 0x21 means I-Fetch Undefined Instruction
CC = r7 == r6;
IF CC JUMP UNDEFINEDINSTRUCTION;   // If EXCAUSE != 0x21 then leave

    R6 = 0x22;      // Also accept illegal insn combo
CC = r7 == r6;
IF CC JUMP UNDEFINEDINSTRUCTION;   // If EXCAUSE != 0x21 then leave

dbg_fail;

UNDEFINEDINSTRUCTION:
    R7 = RETX;      // Fix up return address

    r4 += 2;
    CC = r4 == r7;
    if !CC jump fail;

    R7 += 2;        // skip offending 16 bit instruction

RETX = r7;      // and put back in RETX

    R5 += 1;        // Increment global counter

OUT:
//    (r7 - r6) = [sp++];
//    ASTAT = [sp++];

RTX;
fail:
dbg_fail;

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
