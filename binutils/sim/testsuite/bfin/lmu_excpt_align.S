//Original:/proj/frio/dv/testcases/lmu/lmu_excpt_align/lmu_excpt_align.dsp
// Description: LMU data alignment exceptions
# mach: bfin
# sim: --environment operating

#include "test.h"
.include "testutils.inc"
start

include(selfcheck.inc)
include(std.inc)
include(mmrs.inc)

CHECK_INIT(p5, 0xE0000000);

		// test address for DAG0
		// test address for DAG1

	// setup interrupt controller with exception handler address
	WR_MMR_LABEL(EVT3, handler, p0, r1);

	// Write fault addr MMR to known state
	WR_MMR(DCPLB_FAULT_ADDR, 0, p0, r6);

	//nop;nop;nop;nop;nop;	// in lieu of CSYNC
	CSYNC;

	A0 = 0;

	// go to user mode. and enable exceptions
	LD32_LABEL(r0, User);
	RETI = R0;
	RTI;

	// Nops to work around ICache bug
	NOP;NOP;NOP;NOP;NOP;
	NOP;NOP;NOP;NOP;NOP;


User:
	NOP;NOP;NOP;NOP;NOP;

	//-------------------------------------------------------
	// First do stores
	//-------------------------------------------------------
	// 16-bit alignment, DAG0



	LD32(i1, ((0x1000 + 1)));
	LD32(p2, ((0x1000 + 1) & 0xFFFFFFFE));	// Aligned version
	LD32(r1, 0xDEADBEEF);
	R5 = 0;R6 = 0;R7 = 0;				// Exception handler will set these, reset them first

X01:	W [ I1 ] = R1.L;				// Exception should occur here

	CHECKREG(r5,0x24);			// supv and EXCPT_ALIGN
	CHECKREG(r6, (0x1000 + 1));		// FAULT_ADDR should contain fail address
	CHECKREG_SYM(r7, X01, r0);			// RETX should be value of X01	(HARDCODED ADDR!!)


	//-------------------------------------------------------
	// 32-bit alignment, DAG0



	LD32(i1, ((0x1000 + 1)));
	LD32(p2, ((0x1000 + 1) & 0xFFFFFFFC));	// Aligned version
	LD32(r1, 0xDEADBEEF);
	R5 = 0;R6 = 0;R7 = 0;				// Exception handler will set these, reset them first

X02:	[ I1 ] = R1;				// Exception should occur here

	CHECKREG(r5,0x24);			// supv and EXCPT_ALIGN
	CHECKREG(r6, (0x1000 + 1));		// FAULT_ADDR should contain fail address
	CHECKREG_SYM(r7, X02, r0);			// RETX should be value of X02	(HARDCODED ADDR!!)


	//-------------------------------------------------------
	// 32-bit alignment, DAG0



	LD32(i1, ((0x1000 + 2)));
	LD32(p2, ((0x1000 + 2) & 0xFFFFFFFC));	// Aligned version
	LD32(r1, 0xDEADBEEF);
	R5 = 0;R6 = 0;R7 = 0;				// Exception handler will set these, reset them first

X03:	[ I1 ] = R1;				// Exception should occur here

	CHECKREG(r5,0x24);			// supv and EXCPT_ALIGN
	CHECKREG(r6, (0x1000 + 2));		// FAULT_ADDR should contain fail address
	CHECKREG_SYM(r7, X03, r0);			// RETX should be value of X03	(HARDCODED ADDR!!)


	//-------------------------------------------------------
	// 32-bit alignment, DAG0



	LD32(i1, ((0x1000 + 3)));
	LD32(p2, ((0x1000 + 3) & 0xFFFFFFFC));	// Aligned version
	LD32(r1, 0xDEADBEEF);
	R5 = 0;R6 = 0;R7 = 0;				// Exception handler will set these, reset them first

X04:	[ I1 ] = R1;				// Exception should occur here

	CHECKREG(r5,0x24);			// supv and EXCPT_ALIGN
	CHECKREG(r6, (0x1000 + 3)); 		// FAULT_ADDR should contain fail address
	CHECKREG_SYM(r7, X04, r0);			// RETX should be value of X04	(HARDCODED ADDR!!)


	//-------------------------------------------------------
	// 16-bit alignment, DAG1



	LD32(i1, ((0x1000 + 1)));
	LD32(p2, ((0x1000 + 1) & 0xFFFFFFFE));	// Aligned version
	LD32(r1, 0xDEADBEEF);
	R5 = 0;R6 = 0;R7 = 0;				// Exception handler will set these, reset them first

X05:	A0 = 0 || NOP || W [ I1 ] = R1.L;			// Exception should occur here

	CHECKREG(r5,0x24);			// supv and EXCPT_ALIGN
	CHECKREG(r6, (0x1000 + 1));	 	// FAULT_ADDR should contain fail address
	CHECKREG_SYM(r7, X05, r0);			// RETX should be value of X05	(HARDCODED ADDR!!)


	//-------------------------------------------------------
	// 32-bit alignment, DAG1



	LD32(i1, ((0x1000 + 1)));
	LD32(p2, ((0x1000 + 1) & 0xFFFFFFFC));	// Aligned version
	LD32(r1, 0xDEADBEEF);
	R5 = 0;R6 = 0;R7 = 0;				// Exception handler will set these, reset them first

X06:	A0 = 0 || NOP || [ I1 ] = R1;			// Exception should occur here

	CHECKREG(r5,0x24);			// supv and EXCPT_ALIGN
	CHECKREG(r6, (0x1000 + 1));	 	// FAULT_ADDR should contain fail address
	CHECKREG_SYM(r7, X06, r0);			// RETX should be value of X06	(HARDCODED ADDR!!)


	//-------------------------------------------------------
	// 32-bit alignment, DAG1



	LD32(i1, ((0x1000 + 2)));
	LD32(p2, ((0x1000 + 2) & 0xFFFFFFFC));	// Aligned version
	LD32(r1, 0xDEADBEEF);
	R5 = 0;R6 = 0;R7 = 0;				// Exception handler will set these, reset them first

X07:	A0 = 0 || NOP || [ I1 ] = R1;			// Exception should occur here

	CHECKREG(r5,0x24);			// supv and EXCPT_ALIGN
	CHECKREG(r6, (0x1000 + 2)); 		// FAULT_ADDR should contain fail address
	CHECKREG_SYM(r7, X07, r0);			// RETX should be value of X07	(HARDCODED ADDR!!)


	//-------------------------------------------------------
	// 32-bit alignment, DAG1



	LD32(i1, ((0x1000 + 3)));
	LD32(p2, ((0x1000 + 3) & 0xFFFFFFFC));	// Aligned version
	LD32(r1, 0xDEADBEEF);
	R5 = 0;R6 = 0;R7 = 0;				// Exception handler will set these, reset them first

X08:	A0 = 0 || NOP || [ I1 ] = R1;			// Exception should occur here

	CHECKREG(r5,0x24);			// supv and EXCPT_ALIGN
	CHECKREG(r6, (0x1000 + 3));	 	// FAULT_ADDR should contain fail address
	CHECKREG_SYM(r7, X08, r0);			// RETX should be value of X08	(HARDCODED ADDR!!)


	//-------------------------------------------------------
	// Now repeat for Loads
	//-------------------------------------------------------
	// 16-bit alignment, DAG0



	LD32(i1, ((0x1000 + 1)));
	LD32(p2, ((0x1000 + 1) & 0xFFFFFFFE));	// Aligned version
	LD32(r1, 0xDEADBEEF);
	R5 = 0;R6 = 0;R7 = 0;				// Exception handler will set these, reset them first

X11:	R1.L = W [ I1 ];				// Exception should occur here

	CHECKREG(r5,0x24);			// supv and EXCPT_ALIGN
	CHECKREG(r6, (0x1000 + 1));	 	// FAULT_ADDR should contain fail address
	CHECKREG_SYM(r7, X11, r0);			// RETX should be value of X11	(HARDCODED ADDR!!)


	//-------------------------------------------------------
	// 32-bit alignment, DAG0



	LD32(i1, ((0x1000 + 1)));
	LD32(p2, ((0x1000 + 1) & 0xFFFFFFFC));	// Aligned version
	LD32(r1, 0xDEADBEEF);
	R5 = 0;R6 = 0;R7 = 0;				// Exception handler will set these, reset them first

X12:	R1 = [ I1 ];				// Exception should occur here

	CHECKREG(r5,0x24);			// supv and EXCPT_ALIGN
	CHECKREG(r6, (0x1000 + 1));	 	// FAULT_ADDR should contain fail address
	CHECKREG_SYM(r7, X12, r0);			// RETX should be value of X12	(HARDCODED ADDR!!)


	//-------------------------------------------------------
	// 32-bit alignment, DAG0



	LD32(i1, ((0x1000 + 2)));
	LD32(p2, ((0x1000 + 2) & 0xFFFFFFFC));	// Aligned version
	LD32(r1, 0xDEADBEEF);
	R5 = 0;R6 = 0;R7 = 0;				// Exception handler will set these, reset them first

X13:	R1 = [ I1 ];				// Exception should occur here

	CHECKREG(r5,0x24);			// supv and EXCPT_ALIGN
	CHECKREG(r6, (0x1000 + 2)); 		// FAULT_ADDR should contain fail address
	CHECKREG_SYM(r7, X13, r0);			// RETX should be value of X13	(HARDCODED ADDR!!)


	//-------------------------------------------------------
	// 32-bit alignment, DAG0



	LD32(i1, ((0x1000 + 3)));
	LD32(p2, ((0x1000 + 3) & 0xFFFFFFFC));	// Aligned version
	LD32(r1, 0xDEADBEEF);
	R5 = 0;R6 = 0;R7 = 0;				// Exception handler will set these, reset them first

X14:	R1 = [ I1 ];				// Exception should occur here

	CHECKREG(r5,0x24);			// supv and EXCPT_ALIGN
	CHECKREG(r6, (0x1000 + 3)); 		// FAULT_ADDR should contain fail address
	CHECKREG_SYM(r7, X14, r0);			// RETX should be value of X14	(HARDCODED ADDR!!)


	//-------------------------------------------------------
	// 16-bit alignment, DAG1



	LD32(i1, ((0x1000 + 1)));
	LD32(p2, ((0x1000 + 1) & 0xFFFFFFFE));	// Aligned version
	LD32(r1, 0xDEADBEEF);
	R5 = 0;R6 = 0;R7 = 0;				// Exception handler will set these, reset them first

X15:	A0 = 0 || NOP || R1.L = W [ I1 ];			// Exception should occur here

	CHECKREG(r5,0x24);			// supv and EXCPT_ALIGN
	CHECKREG(r6, (0x1000 + 1));	 	// FAULT_ADDR should contain fail address
	CHECKREG_SYM(r7, X15, r0);			// RETX should be value of X15	(HARDCODED ADDR!!)


	//-------------------------------------------------------
	// 32-bit alignment, DAG1



	LD32(i1, ((0x1000 + 1)));
	LD32(p2, ((0x1000 + 1) & 0xFFFFFFFC));	// Aligned version
	LD32(r1, 0xDEADBEEF);
	R5 = 0;R6 = 0;R7 = 0;				// Exception handler will set these, reset them first

X16:	A0 = 0 || NOP || R1 = [ I1 ];			// Exception should occur here

	CHECKREG(r5,0x24);			// supv and EXCPT_ALIGN
	CHECKREG(r6, (0x1000 + 1));	 	// FAULT_ADDR should contain fail address
	CHECKREG_SYM(r7, X16, r0);			// RETX should be value of X16	(HARDCODED ADDR!!)


	//-------------------------------------------------------
	// 32-bit alignment, DAG1



	LD32(i1, ((0x1000 + 2)));
	LD32(p2, ((0x1000 + 2) & 0xFFFFFFFC));	// Aligned version
	LD32(r1, 0xDEADBEEF);
	R5 = 0;R6 = 0;R7 = 0;				// Exception handler will set these, reset them first

X17:	A0 = 0 || NOP || R1 = [ I1 ];			// Exception should occur here

	CHECKREG(r5,0x24);			// supv and EXCPT_ALIGN
	CHECKREG(r6, (0x1000 + 2));	 	// FAULT_ADDR should contain fail address
	CHECKREG_SYM(r7, X17, r0);			// RETX should be value of X17	(HARDCODED ADDR!!)


	//-------------------------------------------------------
	// 32-bit alignment, DAG1



	LD32(i1, ((0x1000 + 3)));
	LD32(p2, ((0x1000 + 3) & 0xFFFFFFFC));	// Aligned version
	LD32(r1, 0xDEADBEEF);
	R5 = 0;R6 = 0;R7 = 0;				// Exception handler will set these, reset them first

X18:	A0 = 0 || NOP || R1 = [ I1 ];			// Exception should occur here

	CHECKREG(r5,0x24);			// supv and EXCPT_ALIGN
	CHECKREG(r6, (0x1000 + 3));	 	// FAULT_ADDR should contain fail address
	CHECKREG_SYM(r7, X18, r0);			// RETX should be value of X18	(HARDCODED ADDR!!)


	//-------------------------------------------------------
	dbg_pass;


handler:
	R5 = SEQSTAT;	// Get exception cause

	// read and check fail addr (addr_which_causes_exception)
	// should not be set for alignment exception
	RD_MMR(DCPLB_FAULT_ADDR, p0, r6);

	R7 = RETX;	// get address of excepting instruction

	// align the offending address
	I1 = P2;

	RTX;
	// Nops to work around ICache bug
	NOP;NOP;NOP;NOP;NOP;
	NOP;NOP;NOP;NOP;NOP;


.section MEM_0x1000,"aw"
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
