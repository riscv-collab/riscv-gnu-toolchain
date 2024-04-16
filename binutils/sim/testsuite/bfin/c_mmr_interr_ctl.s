# Blackfin testcase for the CEC
# mach: bfin
# sim: --environment operating

	.include "testutils.inc"

	start

	INIT_R_REGS 0;
	INIT_P_REGS 0;
	INIT_I_REGS 0;
	INIT_M_REGS 0;
	INIT_L_REGS 0;
	INIT_B_REGS 0;

	CLI R1;               // inhibit events during MMR writes

	loadsym sp, USTACK;   // setup the user stack pointer
	usp = sp;                  // and frame pointer

	loadsym sp, KSTACK;   // setup the stack pointer
	fp = sp;                  // and frame pointer

	imm32 p0, 0xFFE02000;
	loadsym r0, EHANDLE;  // Emulation Handler (Int0)
	[p0++] = r0;

	loadsym r0, RHANDLE;  // Reset Handler (Int1)
	[p0++] = r0;

	loadsym r0, NHANDLE;  // NMI Handler (Int2)
	[p0++] = r0;

	loadsym r0, XHANDLE;  // Exception Handler (Int3)
	[p0++] = r0;

	[p0++] = r0;          // EVT4 not used global Interr Enable (INT4)

	loadsym r0, HWHANDLE; // HW Error Handler (Int5)
	[p0++] = r0;

	loadsym r0, THANDLE;  // Timer Handler (Int6)
	[p0++] = r0;

	loadsym r0, I7HANDLE; // IVG7 Handler
	[p0++] = r0;

	loadsym r0, I8HANDLE; // IVG8 Handler
	[p0++] = r0;

	loadsym r0, I9HANDLE; // IVG9 Handler
	[p0++] = r0;

	loadsym r0, I10HANDLE;// IVG10 Handler
	[p0++] = r0;

	loadsym r0, I11HANDLE;// IVG11 Handler
	[p0++] = r0;

	loadsym r0, I12HANDLE;// IVG12 Handler
	[p0++] = r0;

	loadsym r0, I13HANDLE;// IVG13 Handler
	[p0++] = r0;

	loadsym r0, I14HANDLE;// IVG14 Handler
	[p0++] = r0;

	loadsym r0, I15HANDLE;// IVG15 Handler
	[p0++] = r0;

	imm32 p0, 0xFFE02100  // EVT_OVERRIDE
	r0 = 0;
	[p0++] = r0;

	r1 = -1;     // Change this to mask interrupts (*)
	csync;       // wait for MMR writes to finish
	sti r1;      // sync and reenable events (implicit write to IMASK)

	imm32 p0, 0xFFE02104;
	r0   = [p0];
	// ckeck that sti allows the lower 5 bits of imask to be written
	CHECKREG r0, 0xffff;

DUMMY:

	r0 = 0 (z);

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

	loadsym r0, STARTUSER;// One gets to user mode by doing a
	                    // ReturnFromInterrupt (RTI)
	RETI = r0;      // We need to load the return address

// Comment the following line for a USER Mode test

	JUMP    STARTSUP;   // jump to code start for SUPERVISOR mode

	RTI;

STARTSUP:
	loadsym p1, BEGIN;

	imm32 p0, (0xFFE02000 + 4 * 15);

	CLI R1;    // inhibit events during write to MMR
	[p0] = p1;   // IVG15 (General) handler (Int 15) load with start
	csync;       // wait for it
	sti r1;      // reenable events with proper imask

	RAISE 15;       // after we RTI, INT 15 should be taken

	RTI;

//
// The Main Program
//
STARTUSER:
	LINK 0;     // change for how much stack frame space you need.

	JUMP BEGIN;

// *********************************************************************

BEGIN:

	            // COMMENT the following line for USER MODE tests
	[--sp] = RETI;  // enable interrupts in supervisor mode

	            // **** YOUR CODE GOES HERE ****
// EVTx
	// wrt-rd  EVT0: 0 bits, rw=0   = 0xFFE02000
	imm32 p0, 0xFFE02000;
	imm32 r0, 0x00000000
	[p0] = r0;

	// wrt-rd EVT1: 32 bits, rw=0   = 0xFFE02004
	imm32 p0, 0xFFE02004;
	imm32 r0, 0x00000000
	[p0] = r0;

	// wrt-rd     EVT2              = 0xFFE02008
	imm32 p0, 0xFFE02008
	imm32 r0, 0xE1DE5D1C
	[p0] = r0;

	// wrt-rd     EVT3              = 0xFFE0200C
	imm32 p0, 0xFFE0200C
	imm32 r0, 0x9CC20332
	[p0] = r0;

	// wrt-rd     EVT4              = 0xFFE02010
	imm32 p0, 0xFFE02010
	imm32 r0, 0x00000000
	[p0] = r0;

	// wrt-rd     EVT5              = 0xFFE02014
	imm32 p0, 0xFFE02014
	imm32 r0, 0x55552345
	[p0] = r0;

	// wrt-rd     EVT6              = 0xFFE02018
	imm32 p0, 0xFFE02018
	imm32 r0, 0x66663456
	[p0] = r0;

	// wrt-rd     EVT7              = 0xFFE0201C
	imm32 p0, 0xFFE0201C
	imm32 r0, 0x77774567
	[p0] = r0;

	// wrt-rd     EVT8              = 0xFFE02020
	imm32 p0, 0xFFE02020
	imm32 r0, 0x88885678
	[p0] = r0;

	// wrt-rd     EVT9              = 0xFFE02024
	imm32 p0, 0xFFE02024
	imm32 r0, 0x99996789
	[p0] = r0;

	// wrt-rd     EVT10             = 0xFFE02028
	imm32 p0, 0xFFE02028
	imm32 r0, 0xaaaa1234
	[p0] = r0;

	// wrt-rd     EVT11             = 0xFFE0202C
	imm32 p0, 0xFFE0202C
	imm32 r0, 0xBBBBABC6
	[p0] = r0;

	// wrt-rd     EVT12             = 0xFFE02030
	imm32 p0, 0xFFE02030
	imm32 r0, 0xCCCCABC6
	[p0] = r0;

	// wrt-rd     EVT13             = 0xFFE02034
	imm32 p0, 0xFFE02034
	imm32 r0, 0xDDDDABC6
	[p0] = r0;

	// wrt-rd     EVT14             = 0xFFE02038
	imm32 p0, 0xFFE02038
	imm32 r0, 0xEEEEABC6
	[p0] = r0;

	// wrt-rd     EVT15             = 0xFFE0203C
	imm32 p0, 0xFFE0203C
	imm32 r0, 0xFFFFABC6
	[p0] = r0;

	// wrt-rd  EVT_OVERRIDE:9 bits  = 0xFFE02100
	imm32 p0, 0xFFE02100
	imm32 r0, 0x000001ff
	[p0] = r0;

	// wrt-rd  IMASK: 16 bits       = 0xFFE02104
	imm32 p0, 0xFFE02104
	imm32 r0, 0x00000fff
	[p0] = r0;

	// wrt-rd  IPEND: 16 bits, rw=0 = 0xFFE02108
	imm32 p0, 0xFFE02108
	imm32 r0, 0x00000000
	//[p0] = r0;
	raise 12;
	raise 13;

	// wrt-rd  ILAT: 16 bits, rw=0  = 0xFFE0210C
	imm32 p0, 0xFFE0210C
	imm32 r0, 0x00000000
	//[p0] = r0;
	csync;

	// *** read ops
	imm32 p0, 0xFFE02000
	r0   = [p0];
	CHECKREG r0, 0;

	imm32 p0, 0xFFE02004
	r1   = [p0];
	CHECKREG r1, 0;

	imm32 p0, 0xFFE02008
	r2   = [p0];
	CHECKREG r2, 0xE1DE5D1C;

	imm32 p0, 0xFFE0200C
	r3   = [p0];
	CHECKREG r3, 0x9CC20332;

	imm32 p0, 0xFFE02014
	r4   = [p0];
	imm32 p0, 0xFFE02018
	r5   = [p0];
	imm32 p0, 0xFFE0201C
	r6   = [p0];
	imm32 p0, 0xFFE02020	/* EVT8 */
	r7   = [p0];
CHECKREG r0, 0x00000000;
//CHECKREG(r1, 0x00000000);   /// mismatch = 00
CHECKREG r2, 0xE1DE5D1C;
CHECKREG r3, 0x9CC20332;
CHECKREG r4, 0x55552345;
CHECKREG r5, 0x66663456;
CHECKREG r6, 0x77774567;
CHECKREG r7, 0x88885678;

	imm32 p0, 0xFFE02024	/* EVT9 */
	r0   = [p0];
	imm32 p0, 0xFFE02028	/* EVT10 */
	r1   = [p0];
	imm32 p0, 0xFFE0202C	/* EVT11 */
	r2   = [p0];
	imm32 p0, 0xFFE02030	/* EVT12 */
	r3   = [p0];
	imm32 p0, 0xFFE02034	/* EVT13 */
	r4   = [p0];
	imm32 p0, 0xFFE02038	/* EVT14 */
	r5   = [p0];
	imm32 p0, 0xFFE0203C	/* EVT15 */
	r6   = [p0];
CHECKREG r0, 0x99996789;
CHECKREG r1, 0xaaaa1234;
CHECKREG r2, 0xBBBBABC6;
CHECKREG r3, 0xCCCCABC6;
CHECKREG r4, 0xDDDDABC6;
CHECKREG r5, 0xEEEEABC6;
CHECKREG r6, 0xFFFFABC6;

	imm32 p0, 0xFFE02100	/* EVT_OVERRIDE */
	r0   = [p0];
	imm32 p0, 0xFFE02104	/* IMASK */
	r1   = [p0];
	imm32 p0, 0xFFE02108	/* IPEND */
	r2   = [p0];
	imm32 p0, 0xFFE0210C	/* ILAT */
	r3   = [p0];
CHECKREG r0, 0x000001ff;
CHECKREG r1, 0x00000fff;	/* XXX: original had 0xfe0 ??  */
CHECKREG r2, 0x00008000;
CHECKREG r3, 0x00003000;

	dbg_pass;

// *********************************************************************

//
// Handlers for Events
//

EHANDLE:            // Emulation Handler 0
	RTE;

RHANDLE:            // Reset Handler 1
	RTI;

NHANDLE:            // NMI Handler 2
	r0 = 2;
	RTN;

XHANDLE:            // Exception Handler 3

	RTX;

HWHANDLE:           // HW Error Handler 5
	r2 = 5;
	RTI;

THANDLE:            // Timer Handler 6
	r3 = 6;
	RTI;

I7HANDLE:           // IVG 7 Handler
	r4 = 7;
	RTI;

I8HANDLE:           // IVG 8 Handler
	r5 = 8;
	RTI;

I9HANDLE:           // IVG 9 Handler
	r6 = 9;
	RTI;

I10HANDLE:          // IVG 10 Handler
	r7 = 10;
	RTI;

I11HANDLE:          // IVG 11 Handler
	r0 = 11;
	RTI;

I12HANDLE:          // IVG 12 Handler
	r1 = 12;
	RTI;

I13HANDLE:          // IVG 13 Handler
	r2 = 13;
	RTI;

I14HANDLE:          // IVG 14 Handler
	r3 = 14;
	RTI;

I15HANDLE:          // IVG 15 Handler
	r4 = 15;
	RTI;

	nop;nop;nop;nop;nop;nop;nop; // needed for icache bug

//
// Data Segment
//

.data
// Stack Segments (Both Kernel and User)

.rep 0x10
.byte 0
.endr
KSTACK:

.rep 0x10
.byte 0
.endr
USTACK:
