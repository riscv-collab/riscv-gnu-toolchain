//Original:/proj/frio/dv/testcases/seq/se_undefinedinstruction1/se_undefinedinstruction1.dsp
// Description: 16 bit "holes" Undefined Instructions in Supervisor Mode
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

    // count of UI's will be in r5, which was initialized to 0 by header


    .dw 0x1 ;
    .dw 0x2 ;
    .dw 0x3 ;
    .dw 0x4 ;
    .dw 0x5 ;
    .dw 0x6 ;
    .dw 0x7 ;
    .dw 0x8 ;
    .dw 0x9 ;
    .dw 0xA ;
    .dw 0xB ;
    .dw 0xC ;
    .dw 0xD ;
    .dw 0xE ;
    .dw 0xF ;
    .dw 0x15 ;
    .dw 0x16 ;
    .dw 0x17 ;
    .dw 0x18 ;
    .dw 0x19 ;
    .dw 0x1A ;
    .dw 0x1B ;
    .dw 0x1C ;
    .dw 0x1D ;
    .dw 0x1E ;
    .dw 0x1F ;
    .dw 0x21 ;
    .dw 0x22 ;
    .dw 0x26 ;
    .dw 0x27 ;	// XXX: hardware doesnt trigger illegal exception ?
    .dw 0x28 ;
    .dw 0x29 ;
    .dw 0x2A ;
    .dw 0x2B ;
    .dw 0x2C ;
    .dw 0x2D ;
    .dw 0x2E ;
    .dw 0x2F ;
    .dw 0x38 ;
    .dw 0x39 ;
    .dw 0x3A ;
    .dw 0x3B ;
    .dw 0x3C ;
    .dw 0x3D ;
    .dw 0x3E ;
    .dw 0x3F ;
    .dw 0x48 ;
    .dw 0x49 ;
    .dw 0x4A ;
    .dw 0x4B ;
    .dw 0x4C ;
    .dw 0x4D ;
    .dw 0x4E ;
    .dw 0x4F ;
    .dw 0x58 ;
    .dw 0x59 ;
    .dw 0x5A ;
    .dw 0x5B ;
    .dw 0x5C ;
    .dw 0x5D ;
    .dw 0x5E ;
    .dw 0x5F ;
    .dw 0x68 ;
    .dw 0x69 ;
    .dw 0x6A ;
    .dw 0x6B ;
    .dw 0x6C ;
    .dw 0x6D ;
    .dw 0x6E ;
    .dw 0x6F ;
    .dw 0x78 ;
    .dw 0x79 ;
    .dw 0x7A ;
    .dw 0x7B ;
    .dw 0x7C ;
    .dw 0x7D ;
    .dw 0x7E ;
    .dw 0x7F ;
    .dw 0x88 ;
    .dw 0x89 ;
    .dw 0x8A ;
    .dw 0x8B ;
    .dw 0x8C ;
    .dw 0x8D ;
    .dw 0x8E ;
    .dw 0x8F ;
    .dw 0xB8 ;
    .dw 0xB9 ;
    .dw 0xBA ;
    .dw 0xBB ;
    .dw 0xBC ;
    .dw 0xBD ;
    .dw 0xBE ;
    .dw 0xBF ;
    .dw 0xC0 ;
    .dw 0xC1 ;
    .dw 0xC2 ;
    .dw 0xC3 ;
    .dw 0xC4 ;
    .dw 0xC5 ;
    .dw 0xC6 ;
    .dw 0xC7 ;
    .dw 0xC8 ;
    .dw 0xC9 ;
    .dw 0xCA ;
    .dw 0xCB ;
    .dw 0xCC ;
    .dw 0xCD ;
    .dw 0xCE ;
    .dw 0xCF ;
    .dw 0xD0 ;
    .dw 0xD1 ;
    .dw 0xD2 ;
    .dw 0xD3 ;
    .dw 0xD4 ;
    .dw 0xD5 ;
    .dw 0xD6 ;
    .dw 0xD7 ;
    .dw 0xD8 ;
    .dw 0xD9 ;
    .dw 0xDA ;
    .dw 0xDB ;
    .dw 0xDC ;
    .dw 0xDD ;
    .dw 0xDE ;
    .dw 0xDF ;
    .dw 0xE0 ;
    .dw 0xE1 ;
    .dw 0xE2 ;
    .dw 0xE3 ;
    .dw 0xE4 ;
    .dw 0xE5 ;
    .dw 0xE6 ;
    .dw 0xE7 ;
    .dw 0xE8 ;
    .dw 0xE9 ;
    .dw 0xEA ;
    .dw 0xEB ;
    .dw 0xEC ;
    .dw 0xED ;
    .dw 0xEE ;
    .dw 0xEF ;
    .dw 0xF0 ;
    .dw 0xF1 ;
    .dw 0xF2 ;
    .dw 0xF3 ;
    .dw 0xF4 ;
    .dw 0xF5 ;
    .dw 0xF6 ;
    .dw 0xF7 ;
    .dw 0xF8 ;
    .dw 0xF9 ;
    .dw 0xFA ;
    .dw 0xFB ;
    .dw 0xFC ;
    .dw 0xFD ;
    .dw 0xFE ;
    .dw 0xFF ;
    .dw 0x220 ;
    .dw 0x221 ;
    .dw 0x222 ;
    .dw 0x223 ;
    .dw 0x224 ;
    .dw 0x225 ;
    .dw 0x226 ;
    .dw 0x227 ;
    .dw 0x228 ;
    .dw 0x229 ;
    .dw 0x22A ;
    .dw 0x22B ;
    .dw 0x22C ;
    .dw 0x22D ;
    .dw 0x22E ;
    .dw 0x22F ;
    .dw 0x230 ;
    .dw 0x231 ;
    .dw 0x232 ;
    .dw 0x233 ;
    .dw 0x234 ;
    .dw 0x235 ;
    .dw 0x236 ;
    .dw 0x237 ;
    .dw 0x238 ;
    .dw 0x239 ;
    .dw 0x23A ;
    .dw 0x23B ;
    .dw 0x23C ;
    .dw 0x23D ;
    .dw 0x23E ;
    .dw 0x23F ;
    .dw 0x280 ;
    .dw 0x281 ;
    .dw 0x282 ;
    .dw 0x283 ;
    .dw 0x284 ;
    .dw 0x285 ;
    .dw 0x286 ;
    .dw 0x287 ;
    .dw 0x288 ;
    .dw 0x289 ;
    .dw 0x28A ;
    .dw 0x28B ;
    .dw 0x28C ;
    .dw 0x28D ;
    .dw 0x28E ;
    .dw 0x28F ;
    .dw 0x290 ;
    .dw 0x291 ;
    .dw 0x292 ;
    .dw 0x293 ;
    .dw 0x294 ;
    .dw 0x295 ;
    .dw 0x296 ;
    .dw 0x297 ;
    .dw 0x298 ;
    .dw 0x299 ;
    .dw 0x29A ;
    .dw 0x29B ;
    .dw 0x29C ;
    .dw 0x29D ;
    .dw 0x29E ;
    .dw 0x29F ;
    .dw 0x2A0 ;
    .dw 0x2A1 ;
    .dw 0x2A2 ;
    .dw 0x2A3 ;
    .dw 0x2A4 ;
    .dw 0x2A5 ;
    .dw 0x2A6 ;
    .dw 0x2A7 ;
    .dw 0x2A8 ;
    .dw 0x2A9 ;
    .dw 0x2AA ;
    .dw 0x2AB ;
    .dw 0x2AC ;
    .dw 0x2AD ;
    .dw 0x2AE ;
    .dw 0x2AF ;
    .dw 0x2B0 ;
    .dw 0x2B1 ;
    .dw 0x2B2 ;
    .dw 0x2B3 ;
    .dw 0x2B4 ;
    .dw 0x2B5 ;
    .dw 0x2B6 ;
    .dw 0x2B7 ;
    .dw 0x2B8 ;
    .dw 0x2B9 ;
    .dw 0x2BA ;
    .dw 0x2BB ;
    .dw 0x2BC ;
    .dw 0x2BD ;
    .dw 0x2BE ;
    .dw 0x2BF ;
    .dw 0x2C0 ;
    .dw 0x2C1 ;
    .dw 0x2C2 ;
    .dw 0x2C3 ;
    .dw 0x2C4 ;
    .dw 0x2C5 ;
    .dw 0x2C6 ;
    .dw 0x2C7 ;
    .dw 0x2C8 ;
    .dw 0x2C9 ;
    .dw 0x2CA ;
    .dw 0x2CB ;
    .dw 0x2CC ;
    .dw 0x2CD ;
    .dw 0x2CE ;
    .dw 0x2CF ;
    .dw 0x2D0 ;
    .dw 0x2D1 ;
    .dw 0x2D2 ;
    .dw 0x2D3 ;
    .dw 0x2D4 ;
    .dw 0x2D5 ;
    .dw 0x2D6 ;
    .dw 0x2D7 ;
    .dw 0x2D8 ;
    .dw 0x2D9 ;
    .dw 0x2DA ;
    .dw 0x2DB ;
    .dw 0x2DC ;
    .dw 0x2DD ;
    .dw 0x2DE ;
    .dw 0x2DF ;
    .dw 0x2E0 ;
    .dw 0x2E1 ;
    .dw 0x2E2 ;
    .dw 0x2E3 ;
    .dw 0x2E4 ;
    .dw 0x2E5 ;
    .dw 0x2E6 ;
    .dw 0x2E7 ;
    .dw 0x2E8 ;
    .dw 0x2E9 ;
    .dw 0x2EA ;
    .dw 0x2EB ;
    .dw 0x2EC ;
    .dw 0x2ED ;
    .dw 0x2EE ;
    .dw 0x2EF ;
    .dw 0x2F0 ;
    .dw 0x2F1 ;
    .dw 0x2F2 ;
    .dw 0x2F3 ;
    .dw 0x2F4 ;
    .dw 0x2F5 ;
    .dw 0x2F6 ;
    .dw 0x2F7 ;
    .dw 0x2F8 ;
    .dw 0x2F9 ;
    .dw 0x2FA ;
    .dw 0x2FB ;
    .dw 0x2FC ;
    .dw 0x2FD ;
    .dw 0x2FE ;
    .dw 0x2FF ;
    .dw 0x4600 ;
    .dw 0x4601 ;
    .dw 0x4602 ;
    .dw 0x4603 ;
    .dw 0x4604 ;
    .dw 0x4605 ;
    .dw 0x4606 ;
    .dw 0x4607 ;
    .dw 0x4608 ;
    .dw 0x4609 ;
    .dw 0x460A ;
    .dw 0x460B ;
    .dw 0x460C ;
    .dw 0x460D ;
    .dw 0x460E ;
    .dw 0x460F ;
    .dw 0x4610 ;
    .dw 0x4611 ;
    .dw 0x4612 ;
    .dw 0x4613 ;
    .dw 0x4614 ;
    .dw 0x4615 ;
    .dw 0x4616 ;
    .dw 0x4617 ;
    .dw 0x4618 ;
    .dw 0x4619 ;
    .dw 0x461A ;
    .dw 0x461B ;
    .dw 0x461C ;
    .dw 0x461D ;
    .dw 0x461E ;
    .dw 0x461F ;
    .dw 0x4620 ;
    .dw 0x4621 ;
    .dw 0x4622 ;
    .dw 0x4623 ;
    .dw 0x4624 ;
    .dw 0x4625 ;
    .dw 0x4626 ;
    .dw 0x4627 ;
    .dw 0x4628 ;
    .dw 0x4629 ;
    .dw 0x462A ;
    .dw 0x462B ;
    .dw 0x462C ;
    .dw 0x462D ;
    .dw 0x462E ;
    .dw 0x462F ;
    .dw 0x4630 ;
    .dw 0x4631 ;
    .dw 0x4632 ;
    .dw 0x4633 ;
    .dw 0x4634 ;
    .dw 0x4635 ;
    .dw 0x4636 ;
    .dw 0x4637 ;
    .dw 0x4638 ;
    .dw 0x4639 ;
    .dw 0x463A ;
    .dw 0x463B ;
    .dw 0x463C ;
    .dw 0x463D ;
    .dw 0x463E ;
    .dw 0x463F ;
    .dw 0x4640 ;
    .dw 0x4641 ;
    .dw 0x4642 ;
    .dw 0x4643 ;
    .dw 0x4644 ;
    .dw 0x4645 ;
    .dw 0x4646 ;
    .dw 0x4647 ;
    .dw 0x4648 ;
    .dw 0x4649 ;
    .dw 0x464A ;
    .dw 0x464B ;
    .dw 0x464C ;
    .dw 0x464D ;
    .dw 0x464E ;
    .dw 0x464F ;
    .dw 0x4650 ;
    .dw 0x4651 ;
    .dw 0x4652 ;
    .dw 0x4653 ;
    .dw 0x4654 ;
    .dw 0x4655 ;
    .dw 0x4656 ;
    .dw 0x4657 ;
    .dw 0x4658 ;
    .dw 0x4659 ;
    .dw 0x465A ;
    .dw 0x465B ;
    .dw 0x465C ;
    .dw 0x465D ;
    .dw 0x465E ;
    .dw 0x465F ;
    .dw 0x4660 ;
    .dw 0x4661 ;
    .dw 0x4662 ;
    .dw 0x4663 ;
    .dw 0x4664 ;
    .dw 0x4665 ;
    .dw 0x4666 ;
    .dw 0x4667 ;
    .dw 0x4668 ;
    .dw 0x4669 ;
    .dw 0x466A ;
    .dw 0x466B ;
    .dw 0x466C ;
    .dw 0x466D ;
    .dw 0x466E ;
    .dw 0x466F ;
    .dw 0x4670 ;
    .dw 0x4671 ;
    .dw 0x4672 ;
    .dw 0x4673 ;
    .dw 0x4674 ;
    .dw 0x4675 ;
    .dw 0x4676 ;
    .dw 0x4677 ;
    .dw 0x4678 ;
    .dw 0x4679 ;
    .dw 0x467A ;
    .dw 0x467B ;
    .dw 0x467C ;
    .dw 0x467D ;
    .dw 0x467E ;
    .dw 0x467F ;
    .dw 0x4680 ;
    .dw 0x4681 ;
    .dw 0x4682 ;
    .dw 0x4683 ;
    .dw 0x4684 ;
    .dw 0x4685 ;
    .dw 0x4686 ;
    .dw 0x4687 ;
    .dw 0x4688 ;
    .dw 0x4689 ;
    .dw 0x468A ;
    .dw 0x468B ;
    .dw 0x468C ;
    .dw 0x468D ;
    .dw 0x468E ;
    .dw 0x468F ;
    .dw 0x4690 ;
    .dw 0x4691 ;
    .dw 0x4692 ;
    .dw 0x4693 ;
    .dw 0x4694 ;
    .dw 0x4695 ;
    .dw 0x4696 ;
    .dw 0x4697 ;
    .dw 0x4698 ;
    .dw 0x4699 ;
    .dw 0x469A ;
    .dw 0x469B ;
    .dw 0x469C ;
    .dw 0x469D ;
    .dw 0x469E ;
    .dw 0x469F ;
    .dw 0x46A0 ;
    .dw 0x46A1 ;
    .dw 0x46A2 ;
    .dw 0x46A3 ;
    .dw 0x46A4 ;
    .dw 0x46A5 ;
    .dw 0x46A6 ;
    .dw 0x46A7 ;
    .dw 0x46A8 ;
    .dw 0x46A9 ;
    .dw 0x46AA ;
    .dw 0x46AB ;
    .dw 0x46AC ;
    .dw 0x46AD ;
    .dw 0x46AE ;
    .dw 0x46AF ;
    .dw 0x46B0 ;
    .dw 0x46B1 ;
    .dw 0x46B2 ;
    .dw 0x46B3 ;
    .dw 0x46B4 ;
    .dw 0x46B5 ;
    .dw 0x46B6 ;
    .dw 0x46B7 ;
    .dw 0x46B8 ;
    .dw 0x46B9 ;
    .dw 0x46BA ;
    .dw 0x46BB ;
    .dw 0x46BC ;
    .dw 0x46BD ;
    .dw 0x46BE ;
    .dw 0x46BF ;
    .dw 0x46C0 ;
    .dw 0x46C1 ;
    .dw 0x46C2 ;
    .dw 0x46C3 ;
    .dw 0x46C4 ;
    .dw 0x46C5 ;
    .dw 0x46C6 ;
    .dw 0x46C7 ;
    .dw 0x46C8 ;
    .dw 0x46C9 ;
    .dw 0x46CA ;
    .dw 0x46CB ;
    .dw 0x46CC ;
    .dw 0x46CD ;
    .dw 0x46CE ;
    .dw 0x46CF ;
    .dw 0x46D0 ;
    .dw 0x46D1 ;
    .dw 0x46D2 ;
    .dw 0x46D3 ;
    .dw 0x46D4 ;
    .dw 0x46D5 ;
    .dw 0x46D6 ;
    .dw 0x46D7 ;
    .dw 0x46D8 ;
    .dw 0x46D9 ;
    .dw 0x46DA ;
    .dw 0x46DB ;
    .dw 0x46DC ;
    .dw 0x46DD ;
    .dw 0x46DE ;
    .dw 0x46DF ;
    .dw 0x46E0 ;
    .dw 0x46E1 ;
    .dw 0x46E2 ;
    .dw 0x46E3 ;
    .dw 0x46E4 ;
    .dw 0x46E5 ;
    .dw 0x46E6 ;
    .dw 0x46E7 ;
    .dw 0x46E8 ;
    .dw 0x46E9 ;
    .dw 0x46EA ;
    .dw 0x46EB ;
    .dw 0x46EC ;
    .dw 0x46ED ;
    .dw 0x46EE ;
    .dw 0x46EF ;
    .dw 0x46F0 ;
    .dw 0x46F1 ;
    .dw 0x46F2 ;
    .dw 0x46F3 ;
    .dw 0x46F4 ;
    .dw 0x46F5 ;
    .dw 0x46F6 ;
    .dw 0x46F7 ;
    .dw 0x46F8 ;
    .dw 0x46F9 ;
    .dw 0x46FA ;
    .dw 0x46FB ;
    .dw 0x46FC ;
    .dw 0x46FD ;
    .dw 0x46FE ;
    .dw 0x46FF ;
    .dw 0x4700 ;
    .dw 0x4701 ;
    .dw 0x4702 ;
    .dw 0x4703 ;
    .dw 0x4704 ;
    .dw 0x4705 ;
    .dw 0x4706 ;
    .dw 0x4707 ;
    .dw 0x4708 ;
    .dw 0x4709 ;
    .dw 0x470A ;
    .dw 0x470B ;
    .dw 0x470C ;
    .dw 0x470D ;
    .dw 0x470E ;
    .dw 0x470F ;
    .dw 0x4710 ;
    .dw 0x4711 ;
    .dw 0x4712 ;
    .dw 0x4713 ;
    .dw 0x4714 ;
    .dw 0x4715 ;
    .dw 0x4716 ;
    .dw 0x4717 ;
    .dw 0x4718 ;
    .dw 0x4719 ;
    .dw 0x471A ;
    .dw 0x471B ;
    .dw 0x471C ;
    .dw 0x471D ;
    .dw 0x471E ;
    .dw 0x471F ;
    .dw 0x4720 ;
    .dw 0x4721 ;
    .dw 0x4722 ;
    .dw 0x4723 ;
    .dw 0x4724 ;
    .dw 0x4725 ;
    .dw 0x4726 ;
    .dw 0x4727 ;
    .dw 0x4728 ;
    .dw 0x4729 ;
    .dw 0x472A ;
    .dw 0x472B ;
    .dw 0x472C ;
    .dw 0x472D ;
    .dw 0x472E ;
    .dw 0x472F ;
    .dw 0x4730 ;
    .dw 0x4731 ;
    .dw 0x4732 ;
    .dw 0x4733 ;
    .dw 0x4734 ;
    .dw 0x4735 ;
    .dw 0x4736 ;
    .dw 0x4737 ;
    .dw 0x4738 ;
    .dw 0x4739 ;
    .dw 0x473A ;
    .dw 0x473B ;
    .dw 0x473C ;
    .dw 0x473D ;
    .dw 0x473E ;
    .dw 0x473F ;
    .dw 0x4740 ;
    .dw 0x4741 ;
    .dw 0x4742 ;
    .dw 0x4743 ;
    .dw 0x4744 ;
    .dw 0x4745 ;
    .dw 0x4746 ;
    .dw 0x4747 ;
    .dw 0x4748 ;
    .dw 0x4749 ;
    .dw 0x474A ;
    .dw 0x474B ;
    .dw 0x474C ;
    .dw 0x474D ;
    .dw 0x474E ;
    .dw 0x474F ;
    .dw 0x4750 ;
    .dw 0x4751 ;
    .dw 0x4752 ;
    .dw 0x4753 ;
    .dw 0x4754 ;
    .dw 0x4755 ;
    .dw 0x4756 ;
    .dw 0x4757 ;
    .dw 0x4758 ;
    .dw 0x4759 ;
    .dw 0x475A ;
    .dw 0x475B ;
    .dw 0x475C ;
    .dw 0x475D ;
    .dw 0x475E ;
    .dw 0x475F ;
    .dw 0x4760 ;
    .dw 0x4761 ;
    .dw 0x4762 ;
    .dw 0x4763 ;
    .dw 0x4764 ;
    .dw 0x4765 ;
    .dw 0x4766 ;
    .dw 0x4767 ;
    .dw 0x4768 ;
    .dw 0x4769 ;
    .dw 0x476A ;
    .dw 0x476B ;
    .dw 0x476C ;
    .dw 0x476D ;
    .dw 0x476E ;
    .dw 0x476F ;
    .dw 0x4770 ;
    .dw 0x4771 ;
    .dw 0x4772 ;
    .dw 0x4773 ;
    .dw 0x4774 ;
    .dw 0x4775 ;
    .dw 0x4776 ;
    .dw 0x4777 ;
    .dw 0x4778 ;
    .dw 0x4779 ;
    .dw 0x477A ;
    .dw 0x477B ;
    .dw 0x477C ;
    .dw 0x477D ;
    .dw 0x477E ;
    .dw 0x477F ;
    .dw 0x4780 ;
    .dw 0x4781 ;
    .dw 0x4782 ;
    .dw 0x4783 ;
    .dw 0x4784 ;
    .dw 0x4785 ;
    .dw 0x4786 ;
    .dw 0x4787 ;
    .dw 0x4788 ;
    .dw 0x4789 ;
    .dw 0x478A ;
    .dw 0x478B ;
    .dw 0x478C ;
    .dw 0x478D ;
    .dw 0x478E ;
    .dw 0x478F ;
    .dw 0x4790 ;
    .dw 0x4791 ;
    .dw 0x4792 ;
    .dw 0x4793 ;
    .dw 0x4794 ;
    .dw 0x4795 ;
    .dw 0x4796 ;
    .dw 0x4797 ;
    .dw 0x4798 ;
    .dw 0x4799 ;
    .dw 0x479A ;
    .dw 0x479B ;
    .dw 0x479C ;
    .dw 0x479D ;
    .dw 0x479E ;
    .dw 0x479F ;
    .dw 0x47A0 ;
    .dw 0x47A1 ;
    .dw 0x47A2 ;
    .dw 0x47A3 ;
    .dw 0x47A4 ;
    .dw 0x47A5 ;
    .dw 0x47A6 ;
    .dw 0x47A7 ;
    .dw 0x47A8 ;
    .dw 0x47A9 ;
    .dw 0x47AA ;
    .dw 0x47AB ;
    .dw 0x47AC ;
    .dw 0x47AD ;
    .dw 0x47AE ;
    .dw 0x47AF ;
    .dw 0x47B0 ;
    .dw 0x47B1 ;
    .dw 0x47B2 ;
    .dw 0x47B3 ;
    .dw 0x47B4 ;
    .dw 0x47B5 ;
    .dw 0x47B6 ;
    .dw 0x47B7 ;
    .dw 0x47B8 ;
    .dw 0x47B9 ;
    .dw 0x47BA ;
    .dw 0x47BB ;
    .dw 0x47BC ;
    .dw 0x47BD ;
    .dw 0x47BE ;
    .dw 0x47BF ;
    .dw 0x47C0 ;
    .dw 0x47C1 ;
    .dw 0x47C2 ;
    .dw 0x47C3 ;
    .dw 0x47C4 ;
    .dw 0x47C5 ;
    .dw 0x47C6 ;
    .dw 0x47C7 ;
    .dw 0x47C8 ;
    .dw 0x47C9 ;
    .dw 0x47CA ;
    .dw 0x47CB ;
    .dw 0x47CC ;
    .dw 0x47CD ;
    .dw 0x47CE ;
    .dw 0x47CF ;
    .dw 0x47D0 ;
    .dw 0x47D1 ;
    .dw 0x47D2 ;
    .dw 0x47D3 ;
    .dw 0x47D4 ;
    .dw 0x47D5 ;
    .dw 0x47D6 ;
    .dw 0x47D7 ;
    .dw 0x47D8 ;
    .dw 0x47D9 ;
    .dw 0x47DA ;
    .dw 0x47DB ;
    .dw 0x47DC ;
    .dw 0x47DD ;
    .dw 0x47DE ;
    .dw 0x47DF ;
    .dw 0x47E0 ;
    .dw 0x47E1 ;
    .dw 0x47E2 ;
    .dw 0x47E3 ;
    .dw 0x47E4 ;
    .dw 0x47E5 ;
    .dw 0x47E6 ;
    .dw 0x47E7 ;
    .dw 0x47E8 ;
    .dw 0x47E9 ;
    .dw 0x47EA ;
    .dw 0x47EB ;
    .dw 0x47EC ;
    .dw 0x47ED ;
    .dw 0x47EE ;
    .dw 0x47EF ;
    .dw 0x47F0 ;
    .dw 0x47F1 ;
    .dw 0x47F2 ;
    .dw 0x47F3 ;
    .dw 0x47F4 ;
    .dw 0x47F5 ;
    .dw 0x47F6 ;
    .dw 0x47F7 ;
    .dw 0x47F8 ;
    .dw 0x47F9 ;
    .dw 0x47FA ;
    .dw 0x47FB ;
    .dw 0x47FC ;
    .dw 0x47FD ;
    .dw 0x47FE ;
    .dw 0x47FF ;

CHECK_INIT_DEF(p0); //CHECK_INIT(p0, 0xFF7FFFFC);
                    // Xhandler counts all EXCAUSE = 0x21;
CHECKREG(r5, 830); // count of all 16 bit UI's.

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
    R6 = 0x21;      // EXCAUSE 0x21 means I-Fetch Undefined Instruction
CC = r7 == r6;
IF CC JUMP UNDEFINEDINSTRUCTION;   // If EXCAUSE != 0x21 then leave

JUMP.S OUT;       // if the EXCAUSE is wrong the test will infinite loop

UNDEFINEDINSTRUCTION:
    R7 = RETX;      // Fix up return address

    R7 += 2;        // skip offending 16 bit instruction

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
