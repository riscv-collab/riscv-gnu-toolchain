//Original:/proj/frio/dv/testcases/lmu/lmu_excpt_default/lmu_excpt_default.dsp
// Description: Default protection checks (CPLB disabled)
//		- MMR access in User mode
//		- DAG1 Access MMRs (supv/user mode, read/write)
//		- DAG1 Access Scratch SRAM (user or supervisor mode, read/write)
# mach: bfin
# sim: --environment operating

#include "test.h"
.include "testutils.inc"
start

include(selfcheck.inc)
include(std.inc)
include(mmrs.inc)

#define EXCPT_PROTVIOL 0x23
#define OMODE_SUPV 0 // not used in the hardware



	CHECK_INIT(p5, 0xE0000000);

	// setup interrupt controller with exception handler address
	WR_MMR_LABEL(EVT3, handler, p0, r1);
	WR_MMR_LABEL(EVT15, Supv, p0, r1);
	WR_MMR(EVT_IMASK, 0xFFFFFFFF, p0, r0);
	WR_MMR(EVT_OVERRIDE, 0x00000000, p0, r0);
	CSYNC;

	A0 = 0;

	// go to user mode. and enable exceptions
	LD32_LABEL(r0, User);
	RETI = R0;

	// But first raise interrupt 15 so we can run in supervisor mode.
	RAISE 15;

	RTI;

Supv:

	//-------------------------------------------------------
	// DAG1 MMR Write access



	LD32(i1, (DCPLB_ADDR0));
	LD32_LABEL(p2, Y01);			// Exception handler will return to this address
	LD32(r0, 0xdeadbeef);


	R4 = 0;R5 = 0;R6 = 0;R7 = 0;			// Exception handler will set these, reset them first
X01:	A0 = 0 || NOP || [ I1 ] = R1;			// Exception should occur here
Y01:

	// Now check that handler read correct values
	CHECKREG(r4, (OMODE_SUPV|EXCPT_PROTVIOL));		// SEQSTAT
	CHECKREG(r5, DCPLB_ADDR0);				// FAULT ADDRESS
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG1|FAULT_SUPV));	// DCPLB_STATUS
	CHECKREG_SYM(r7, X01, r0);					// RETX  X01:	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	// DAG1 MMR Read access



	LD32(i1, (DCPLB_ADDR1));
	LD32_LABEL(p2, Y02);			// Exception handler will return to this address


	R4 = 0;R5 = 0;R6 = 0;R7 = 0;			// Exception handler will set these, reset them first
X02:	A0 = 0 || NOP || R1 = [ I1 ];			// Exception should occur here
Y02:

	// Now check that handler read correct values
	CHECKREG(r4, (OMODE_SUPV|EXCPT_PROTVIOL));		// SEQSTAT
	CHECKREG(r5, DCPLB_ADDR1);				// FAULT ADDRESS
	CHECKREG(r6, (FAULT_READ|FAULT_DAG1|FAULT_SUPV));	// DCPLB_STATUS
	CHECKREG_SYM(r7, X02, r0);					// RETX  X02:	(HARDCODED ADDR!!)

#if 0
	//-------------------------------------------------------
	// DAG1 Scratch SRAM Write access



	LD32(i1, (( 0xFF800000 + 0x300000)));
	LD32_LABEL(p2, Y03);			// Exception handler will return to this address
	LD32(r1, 0xdeadbeef);


	R4 = 0;R5 = 0;R6 = 0;R7 = 0;			// Exception handler will set these, reset them first
X03:	A0 = 0 || NOP || [ I1 ] = R1;			// Exception should occur here
Y03:

	// Now check that handler read correct values
	CHECKREG(r4, (OMODE_SUPV|EXCPT_PROTVIOL));		// SEQSTAT
	CHECKREG(r5, ( 0xFF800000 + 0x300000));				// FAULT ADDRESS
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG1|FAULT_SUPV));	// DCPLB_STATUS
	CHECKREG_SYM(r7, X03, r0);					// RETX  X03:	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	// DAG1 Scratch SRAM Read access



	LD32(i1, ((( 0xFF800000 + 0x300000) + 4)));
	LD32_LABEL(p2, Y04);			// Exception handler will return to this address


	R4 = 0;R5 = 0;R6 = 0;R7 = 0;			// Exception handler will set these, reset them first
X04:	A0 = 0 || NOP || R1 = [ I1 ];			// Exception should occur here
Y04:

	// Now check that handler read correct values
	CHECKREG(r4, (OMODE_SUPV|EXCPT_PROTVIOL));		// SEQSTAT
	CHECKREG(r5, (( 0xFF800000 + 0x300000) + 4));				// FAULT ADDRESS
	CHECKREG(r6, (FAULT_READ|FAULT_DAG1|FAULT_SUPV));	// DCPLB_STATUS
	CHECKREG_SYM(r7, X04, r0);					// RETX  X04:	(HARDCODED ADDR!!)
#endif

	//-------------------------------------------------------

	// Now, go to User mode
	LD32_LABEL(r0, User);
	RETI = R0;
	RTI;


User:

	//-------------------------------------------------------
	// DAG0 MMR Write access (multi-issue)



	LD32(i1, (DCPLB_ADDR0));
	LD32_LABEL(p2, Y11);			// Exception handler will return to this address
	LD32(r0, 0xdeadbeef);


	R4 = 0;R5 = 0;R6 = 0;R7 = 0;			// Exception handler will set these, reset them first
X11:	A0 = 0 || [ I1 ] = R1 || NOP;			// Exception should occur here
Y11:

	// Now check that handler read correct values
	CHECKREG(r4, (OMODE_SUPV|EXCPT_PROTVIOL));		// SEQSTAT
	CHECKREG(r5, DCPLB_ADDR0);				// FAULT ADDRESS
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_USER));	// DCPLB_STATUS
	CHECKREG_SYM(r7, X11, r0);					// RETX  X11:	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	// DAG0 MMR Read access (multi-issue)



	LD32(i1, (DCPLB_ADDR1));
	LD32_LABEL(p2, Y12);			// Exception handler will return to this address


	R4 = 0;R5 = 0;R6 = 0;R7 = 0;			// Exception handler will set these, reset them first
X12:	A0 = 0 || R1 = [ I1 ] || NOP;			// Exception should occur here
Y12:

	// Now check that handler read correct values
	CHECKREG(r4, (OMODE_SUPV|EXCPT_PROTVIOL));		// SEQSTAT
	CHECKREG(r5, DCPLB_ADDR1);				// FAULT ADDRESS
	CHECKREG(r6, (FAULT_READ|FAULT_DAG0|FAULT_USER));	// DCPLB_STATUS
	CHECKREG_SYM(r7, X12, r0);					// RETX  X12:	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	// DAG1 MMR Write access



	LD32(i1, (DCPLB_ADDR0));
	LD32_LABEL(p2, Y13);			// Exception handler will return to this address
	LD32(r0, 0xdeadbeef);


	R4 = 0;R5 = 0;R6 = 0;R7 = 0;			// Exception handler will set these, reset them first
X13:	A0 = 0 || NOP || [ I1 ] = R1;			// Exception should occur here
Y13:

	// Now check that handler read correct values
	CHECKREG(r4, (OMODE_SUPV|EXCPT_PROTVIOL));		// SEQSTAT
	CHECKREG(r5, DCPLB_ADDR0);				// FAULT ADDRESS
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG1|FAULT_USER));	// DCPLB_STATUS
	CHECKREG_SYM(r7, X13, r0);					// RETX  X13:	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	// DAG1 MMR Read access



	LD32(i1, (DCPLB_ADDR1));
	LD32_LABEL(p2, Y14);			// Exception handler will return to this address


	R4 = 0;R5 = 0;R6 = 0;R7 = 0;			// Exception handler will set these, reset them first
X14:	A0 = 0 || NOP || R1 = [ I1 ];			// Exception should occur here
Y14:

	// Now check that handler read correct values
	CHECKREG(r4, (OMODE_SUPV|EXCPT_PROTVIOL));		// SEQSTAT
	CHECKREG(r5, DCPLB_ADDR1);				// FAULT ADDRESS
	CHECKREG(r6, (FAULT_READ|FAULT_DAG1|FAULT_USER));	// DCPLB_STATUS
	CHECKREG_SYM(r7, X14, r0);					// RETX  X14:	(HARDCODED ADDR!!)

#if 0
	//-------------------------------------------------------
	// DAG1 Scratch SRAM Write access



	LD32(i1, (( 0xFF800000 + 0x300000)));
	LD32_LABEL(p2, Y15);			// Exception handler will return to this address
	LD32(r1, 0xdeadbeef);


	R4 = 0;R5 = 0;R6 = 0;R7 = 0;			// Exception handler will set these, reset them first
X15:	A0 = 0 || NOP || [ I1 ] = R1;			// Exception should occur here
Y15:

	// Now check that handler read correct values
	CHECKREG(r4, (OMODE_SUPV|EXCPT_PROTVIOL));		// SEQSTAT
	CHECKREG(r5, ( 0xFF800000 + 0x300000));				// FAULT ADDRESS
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG1|FAULT_USER));	// DCPLB_STATUS
	CHECKREG_SYM(r7, X15, r0);					// RETX  X15:	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	// DAG1 Scratch SRAM Read access



	LD32(i1, ((( 0xFF800000 + 0x300000) + 4)));
	LD32_LABEL(p2, Y16);			// Exception handler will return to this address


	R4 = 0;R5 = 0;R6 = 0;R7 = 0;			// Exception handler will set these, reset them first
X16:	A0 = 0 || NOP || R1 = [ I1 ];			// Exception should occur here
Y16:

	// Now check that handler read correct values
	CHECKREG(r4, (OMODE_SUPV|EXCPT_PROTVIOL));		// SEQSTAT
	CHECKREG(r5, (( 0xFF800000 + 0x300000) + 4));				// FAULT ADDRESS
	CHECKREG(r6, (FAULT_READ|FAULT_DAG1|FAULT_USER));	// DCPLB_STATUS
	CHECKREG_SYM(r7, X16, r0);					// RETX  X16:	(HARDCODED ADDR!!)
#endif

	//-------------------------------------------------------
	// DAG0 MMR Write access (single-issue)



	LD32(i1, (DCPLB_ADDR0));
	LD32_LABEL(p2, Y17);			// Exception handler will return to this address
	LD32(r0, 0xdeadbeef);


	R4 = 0;R5 = 0;R6 = 0;R7 = 0;			// Exception handler will set these, reset them first
X17:	[ I1 ] = R1;				// Exception should occur here
Y17:

	// Now check that handler read correct values
	CHECKREG(r4, (OMODE_SUPV|EXCPT_PROTVIOL));		// SEQSTAT
	CHECKREG(r5, DCPLB_ADDR0);				// FAULT ADDRESS
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_USER));	// DCPLB_STATUS
	CHECKREG_SYM(r7, X17, r0);					// RETX  X17:	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	// DAG0 MMR Read access (single-issue)



	LD32(i1, (DCPLB_ADDR1));
	LD32_LABEL(p2, Y18);			// Exception handler will return to this address


	R4 = 0;R5 = 0;R6 = 0;R7 = 0;			// Exception handler will set these, reset them first
X18:	R1 = [ I1 ];				// Exception should occur here
Y18:

	// Now check that handler read correct values
	CHECKREG(r4, (OMODE_SUPV|EXCPT_PROTVIOL));		// SEQSTAT
	CHECKREG(r5, DCPLB_ADDR1);				// FAULT ADDRESS
	CHECKREG(r6, (FAULT_READ|FAULT_DAG0|FAULT_USER));	// DCPLB_STATUS
	CHECKREG_SYM(r7, X18, r0);					// RETX  X18:	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	dbg_pass;


handler:
	R4 = SEQSTAT;	// Get exception cause

	// read and check fail addr (addr_which_causes_exception)
	// should not be set for alignment exception
	RD_MMR(DCPLB_FAULT_ADDR, p0, r5);
	RD_MMR(DCPLB_STATUS, p0, r6);
	R7 = RETX;	// get address of excepting instruction

	RETX = P2;

	RTX;
