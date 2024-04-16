//Original:/proj/frio/dv/testcases/core/c_interr_timer_tcount/c_interr_timer_tcount.dsp
// Spec Reference: interrupt on HW TIMER tcount
# mach: bfin
# sim: --environment operating

#include "test.h"
.include "testutils.inc"
start

//
// Include Files
//

include(std.inc)
include(selfcheck.inc)

// Defines

#ifndef TCNTL
#define TCNTL            0xFFE03000
#endif
#ifndef TPERIOD
#define TPERIOD          0xFFE03004
#endif
#ifndef TSCALE
#define TSCALE           0xFFE03008
#endif
#ifndef TCOUNT
#define TCOUNT           0xFFE0300c
#endif
#ifndef EVT
#define EVT              0xFFE02000
#endif
#ifndef EVT15
#define EVT15            0xFFE0203c
#endif
#ifndef EVT_OVERRIDE
#define EVT_OVERRIDE     0xFFE02100
#endif
#ifndef ITABLE
#define ITABLE           0x000FF000
#endif
#ifndef PROGRAM_STACK
#define PROGRAM_STACK    0x000FF100
#endif
#ifndef STACKSIZE
#define STACKSIZE        0x00000300
#endif

// Boot code

 BOOT :
INIT_R_REGS(0);                             // Initialize Dregs
INIT_P_REGS(0);                             // Initialize Pregs

     // CHECK_INIT(p5,   0x00BFFFFC);
     // CHECK_INIT(p5,   0xE0000000);
include(symtable.inc)
CHECK_INIT_DEF(p5);


LD32(sp, 0x000FF200);
LD32(p0, EVT);              // Setup Event Vectors and Handlers

LD32_LABEL(r0, EHANDLE);    // Emulation Handler (Int0)
        [ P0 ++ ] = R0;

LD32_LABEL(r0, RHANDLE);    // Reset Handler (Int1)
        [ P0 ++ ] = R0;

LD32_LABEL(r0, NHANDLE);    // NMI Handler (Int2)
        [ P0 ++ ] = R0;

LD32_LABEL(r0, XHANDLE);    // Exception Handler (Int3)
        [ P0 ++ ] = R0;

        [ P0 ++ ] = R0;                // IVT4 not used

LD32_LABEL(r0, HWHANDLE);   // HW Error Handler (Int5)
        [ P0 ++ ] = R0;

LD32_LABEL(r0, THANDLE);    // Timer Handler (Int6)
        [ P0 ++ ] = R0;

LD32_LABEL(r0, I7HANDLE);   // IVG7 Handler
        [ P0 ++ ] = R0;

LD32_LABEL(r0, I8HANDLE);   // IVG8 Handler
        [ P0 ++ ] = R0;

LD32_LABEL(r0, I9HANDLE);   // IVG9 Handler
        [ P0 ++ ] = R0;

LD32_LABEL(r0, I10HANDLE);  // IVG10 Handler
        [ P0 ++ ] = R0;

LD32_LABEL(r0, I11HANDLE);  // IVG11 Handler
        [ P0 ++ ] = R0;

LD32_LABEL(r0, I12HANDLE);  // IVG12 Handler
        [ P0 ++ ] = R0;

LD32_LABEL(r0, I13HANDLE);  // IVG13 Handler
        [ P0 ++ ] = R0;

LD32_LABEL(r0, I14HANDLE);  // IVG14 Handler
        [ P0 ++ ] = R0;

LD32_LABEL(r0, I15HANDLE);  // IVG15 Handler
        [ P0 ++ ] = R0;

LD32(p0, EVT_OVERRIDE);
        R0 = 0;
        [ P0 ++ ] = R0;
        R0 = -1;     // Change this to mask interrupts (*)
        [ P0 ] = R0;   // IMASK

LD32_LABEL(p1, START);

LD32(p0, EVT15);
        [ P0 ] = P1;   // IVG15 (General) handler (Int 15) load with start
CSYNC;

RAISE 15;    // after we RTI, INT 15 should be taken

LD32_LABEL(r7, START);
RETI = r7;
NOP;        // Workaround for Bug 217
RTI;
NOP;
NOP;

//.code 0x200
 START :
        R7 = 0x0;
        R6 = 0x1;
        [ -- SP ] = RETI;        // Enable Nested Interrupts

WR_MMR(TCNTL,   0x00000001, p0, r0);   // Turn ON TMPWR (active state)
WR_MMR(TPERIOD, 0x00000010, p0, r0);
WR_MMR(TCOUNT,  0x00000002, p0, r0);
WR_MMR(TSCALE,  0x00000001, p0, r0);
WR_MMR(TCNTL,   0x00000003, p0, r0);    // enable Timer (TMPWR, TMREN)
CSYNC;


RD_MMR(TCNTL, p0, r5);
CHECKREG(r5,    0x0000000B);

WR_MMR(TCNTL,   0x00000000, p0, r0);        // Turn OFF Timer
CSYNC;
CHECKREG(r7,    0x00000001);
        R7 = 0;
NOP;
WR_MMR(TCNTL,   0x00000001, p0, r0);        // Turn ON Timer Power
WR_MMR(TPERIOD, 0x00000010, p0, r0);
WR_MMR(TCOUNT,  0x00000002, p0, r0);
WR_MMR(TSCALE,  0x00000003, p0, r0);
WR_MMR(TCNTL,   0x00000003, p0, r0);        // Turn ON Timer
CSYNC;
NOP;
NOP;
        // Read the contents of the Timer


RD_MMR(TCNTL , p0, r3);
CHECKREG(r3,    0x0000000B);

CHECKREG(r7,    0x00000001);


WR_MMR(TCNTL,   0x00000000, p0, r0);        // Turn OFF Timer
CSYNC;
NOP; NOP; NOP;





dbg_pass;        // Call Endtest Macro



//*********************************************************************
//
// Handlers for Events
//

EHANDLE:            // Emulation Handler 0
RTE;

RHANDLE:            // Reset Handler 1
RTI;

NHANDLE:            // NMI Handler 2
RTN;

XHANDLE:            // Exception Handler 3
RTX;

HWHANDLE:           // HW Error Handler 5
RTI;

THANDLE:            // Timer Handler 6
        R7 = R7 + R6;
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
        R5 = RETI;
        P0 = R5;
JUMP ( P0 );
RTI;

.section MEM_DATA_ADDR_1,"aw"

.space (STACKSIZE);
STACK:
NOP;NOP;NOP;NOP;NOP;NOP;NOP; // needed for icache bug
