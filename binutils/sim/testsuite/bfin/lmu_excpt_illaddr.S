//Original:/proj/frio/dv/testcases/lmu/lmu_excpt_illaddr/lmu_excpt_illaddr.dsp
// Description: LMU illegal address exceptions
// Illegal core MMR: addr[19:16] != 0
// Illegal core MMR: Illegal peripheral
// Illegal core MMR: Illegal addr in peripheral
# mach: bfin
# sim: --environment operating

#include "test.h"
.include "testutils.inc"
start

include(selfcheck.inc)
include(std.inc)
include(mmrs.inc)

#ifndef SR_BASE
#define SR_BASE 		0xFF800000 	// must match value used for sram_baddr inputs
#endif
#ifndef A_SRAM_BASE
#define A_SRAM_BASE 		SR_BASE
#endif
#ifndef B_SRAM_BASE
#define B_SRAM_BASE  		 SR_BASE + 0x100000
#endif
#ifndef I_SRAM_BASE
#define I_SRAM_BASE  		 SR_BASE + 0x200000
#endif
#ifndef SCRATCH_SRAM_BASE
#define SCRATCH_SRAM_BASE 	 SR_BASE + 0x300000
#endif

#ifndef A_SRAM_SIZE
#define A_SRAM_SIZE 		0x4000
#endif
#ifndef B_SRAM_SIZE
#define B_SRAM_SIZE 		0x4000
#endif
#ifndef I_SRAM_SIZE
#define I_SRAM_SIZE 		0x4000
#endif
#ifndef SCRATCH_SRAM_SIZE
#define SCRATCH_SRAM_SIZE 	0x1000
#endif

CHECK_INIT(p5, 0xE0000000);

	// setup interrupt controller with exception handler address
	WR_MMR_LABEL(EVT3, handler, p0, r1);
	WR_MMR_LABEL(EVT15, int15, p0, r1);
	WR_MMR(EVT_IMASK, 0xFFFFFFFF, p0, r0);
	WR_MMR(EVT_OVERRIDE, 0x00000000, p0, r0);

	// Set up CPLB

	WR_MMR(DCPLB_ADDR1, SR_BASE, p0, r0);                // SRAM segment: Non-cacheable
	WR_MMR(DCPLB_DATA1, ( CPLB_VALID | CPLB_L1SRAM | CPLB_DIRTY | CPLB_SUPV_WR | PAGE_SIZE_4M), p0, r0);

	WR_MMR(DCPLB_ADDR2, 0xE0000000, p0, r0);                // CHECKREG segment: Non-cacheable
	WR_MMR(DCPLB_DATA2, ( CPLB_VALID | CPLB_DIRTY | CPLB_SUPV_WR | PAGE_SIZE_4M), p0, r0);

	WR_MMR(DCPLB_ADDR15, 0xFFC00000, p0, r0);               // MMRs: Non-cacheable
	WR_MMR(DCPLB_DATA15, ( CPLB_VALID | CPLB_DIRTY | CPLB_SUPV_WR | PAGE_SIZE_4M), p0, r0);

	WR_MMR(DMEM_CONTROL, (DMC_AB_SRAM | ENDCPLB | ENDM), p0, r0);

	CSYNC;

	// Write fault addr MMR to known state
	WR_MMR(DCPLB_FAULT_ADDR, 0, p0, r6);
	NOP;NOP;NOP;NOP;NOP;	// in lieu of CSYNC

	// go to user mode. and enable exceptions
	LD32_LABEL(r0, User);
	RETI = R0;

	// But first raise interrupt 15 so we will run in supervisor mode.
	RAISE 15;
	NOP;
	RTI;

	// Nops to work around ICache bug
	NOP;NOP;NOP;NOP;NOP;
	NOP;NOP;NOP;NOP;NOP;

int15:
	NOP;NOP;NOP;NOP;NOP;

	//-------------------------------------------------------
	// First do stores
	//-------------------------------------------------------
	//

	// illegal core MMR: addr[19] !=0


	LD32(p1, 0xFFE80000);
	LD32(p2, SRAM_BASE_ADDRESS);		// Valid addr (handler will use move this to p1)
	LD32(r1, 0xDEADBEEF);
	R5 = 0;R6 = 0;R7 = 0;			// Exception handler will set these, reset them first

X01:	[ P1 ] = R1;			// Exception should occur here

	CHECKREG(r5,0x2e);		// supv and EXCPT_PROT
	CHECKREG(r6, 0xFFE80000); 	// FAULT_ADDR should contain test address
	CHECKREG_SYM(r7, X01, r0);		// RETX should be value of X01	(HARDCODED ADDR!!)

	//-------------------------------------------------------

	// illegal core MMR: addr[18] !=0


	LD32(p1, 0xFFE40000);
	LD32(p2, SRAM_BASE_ADDRESS);		// Valid addr (handler will use move this to p1)
	LD32(r1, 0xDEADBEEF);
	R5 = 0;R6 = 0;R7 = 0;			// Exception handler will set these, reset them first

X02:	[ P1 ] = R1;			// Exception should occur here

	CHECKREG(r5,0x2e);		// supv and EXCPT_PROT
	CHECKREG(r6, 0xFFE40000); 	// FAULT_ADDR should contain test address
	CHECKREG_SYM(r7, X02, r0);		// RETX should be value of X02	(HARDCODED ADDR!!)

	//-------------------------------------------------------

	// illegal core MMR: addr[17] !=0


	LD32(p1, 0xFFE20000);
	LD32(p2, SRAM_BASE_ADDRESS);		// Valid addr (handler will use move this to p1)
	LD32(r1, 0xDEADBEEF);
	R5 = 0;R6 = 0;R7 = 0;			// Exception handler will set these, reset them first

X03:	[ P1 ] = R1;			// Exception should occur here

	CHECKREG(r5,0x2e);		// supv and EXCPT_PROT
	CHECKREG(r6, 0xFFE20000); 	// FAULT_ADDR should contain test address
	CHECKREG_SYM(r7, X03, r0);		// RETX should be value of X03	(HARDCODED ADDR!!)

	//-------------------------------------------------------

	// illegal core MMR: addr[16] !=0


	LD32(p1, 0xFFE10000);
	LD32(p2, SRAM_BASE_ADDRESS);		// Valid addr (handler will use move this to p1)
	LD32(r1, 0xDEADBEEF);
	R5 = 0;R6 = 0;R7 = 0;			// Exception handler will set these, reset them first

X04:	[ P1 ] = R1;			// Exception should occur here

	CHECKREG(r5,0x2e);		// supv and EXCPT_PROT
	CHECKREG(r6, 0xFFE10000); 	// FAULT_ADDR should contain test address
	CHECKREG_SYM(r7, X04, r0);		// RETX should be value of X04	(HARDCODED ADDR!!)

	//-------------------------------------------------------

	// illegal core MMR: illegal periperal (addr[15:12] > 8)


	LD32(p1, 0xFFE09000);
	LD32(p2, SRAM_BASE_ADDRESS);		// Valid addr (handler will use move this to p1)
	LD32(r1, 0xDEADBEEF);
	R5 = 0;R6 = 0;R7 = 0;			// Exception handler will set these, reset them first

X10:	[ P1 ] = R1;			// Exception should occur here

	CHECKREG(r5,0x2e);		// supv and EXCPT_PROT
	CHECKREG(r6, 0xFFE09000); 	// FAULT_ADDR should contain test address
	CHECKREG_SYM(r7, X10, r0);		// RETX should be value of X10	(HARDCODED ADDR!!)

	//-------------------------------------------------------

	// illegal core MMR: illegal addr in peripheral 00


	LD32(p1, 0xFFE00408);
	LD32(p2, SRAM_BASE_ADDRESS);		// Valid addr (handler will use move this to p1)
	LD32(r1, 0xDEADBEEF);
	R5 = 0;R6 = 0;R7 = 0;			// Exception handler will set these, reset them first

X20:	[ P1 ] = R1;			// Exception should occur here

	CHECKREG(r5,0x2e);		// supv and EXCPT_PROT
	CHECKREG(r6, 0xFFE00408); 	// FAULT_ADDR should contain test address
	CHECKREG_SYM(r7, X20, r0);		// RETX should be value of X20	(HARDCODED ADDR!!)

	//-------------------------------------------------------

	// illegal core MMR: illegal addr in peripheral 01


	LD32(p1, 0xFFE01408);
	LD32(p2, SRAM_BASE_ADDRESS);		// Valid addr (handler will use move this to p1)
	LD32(r1, 0xDEADBEEF);
	R5 = 0;R6 = 0;R7 = 0;			// Exception handler will set these, reset them first

X21:	[ P1 ] = R1;			// Exception should occur here

	CHECKREG(r5,0x2e);		// supv and EXCPT_PROT
	CHECKREG(r6, 0xFFE01408); 	// FAULT_ADDR should contain test address
	CHECKREG_SYM(r7, X21, r0);		// RETX should be value of X21	(HARDCODED ADDR!!)

	//-------------------------------------------------------

	// illegal core MMR: illegal addr in peripheral 02


	LD32(p1, 0xFFE02114);
	LD32(p2, SRAM_BASE_ADDRESS);		// Valid addr (handler will use move this to p1)
	LD32(r1, 0xDEADBEEF);
	R5 = 0;R6 = 0;R7 = 0;			// Exception handler will set these, reset them first

X22:	[ P1 ] = R1;			// Exception should occur here

	CHECKREG(r5,0x2e);		// supv and EXCPT_PROT
	CHECKREG(r6, 0xFFE02114); 	// FAULT_ADDR should contain test address
	CHECKREG_SYM(r7, X22, r0);		// RETX should be value of X22	(HARDCODED ADDR!!)

	//-------------------------------------------------------

	// illegal core MMR: illegal addr in peripheral 03


	LD32(p1, 0xFFE03010);
	LD32(p2, SRAM_BASE_ADDRESS);		// Valid addr (handler will use move this to p1)
	LD32(r1, 0xDEADBEEF);
	R5 = 0;R6 = 0;R7 = 0;			// Exception handler will set these, reset them first

X23:	[ P1 ] = R1;			// Exception should occur here

	CHECKREG(r5,0x2e);		// supv and EXCPT_PROT
	CHECKREG(r6, 0xFFE03010); 	// FAULT_ADDR should contain test address
	CHECKREG_SYM(r7, X23, r0);		// RETX should be value of X23	(HARDCODED ADDR!!)

	//-------------------------------------------------------

	// illegal core MMR: illegal addr in peripheral 04


	LD32(p1, 0xFFE04008);
	LD32(p2, SRAM_BASE_ADDRESS);		// Valid addr (handler will use move this to p1)
	LD32(r1, 0xDEADBEEF);
	R5 = 0;R6 = 0;R7 = 0;			// Exception handler will set these, reset them first

X24:	[ P1 ] = R1;			// Exception should occur here

	CHECKREG(r5,0x2e);		// supv and EXCPT_PROT
	CHECKREG(r6, 0xFFE04008); 	// FAULT_ADDR should contain test address
	CHECKREG_SYM(r7, X24, r0);		// RETX should be value of X24	(HARDCODED ADDR!!)

	//-------------------------------------------------------

	// illegal core MMR: illegal addr in peripheral 05


	LD32(p1, 0xFFE05010);
	LD32(p2, SRAM_BASE_ADDRESS);		// Valid addr (handler will use move this to p1)
	LD32(r1, 0xDEADBEEF);
	R5 = 0;R6 = 0;R7 = 0;			// Exception handler will set these, reset them first

X25:	[ P1 ] = R1;			// Exception should occur here

	CHECKREG(r5,0x2e);		// supv and EXCPT_PROT
	CHECKREG(r6, 0xFFE05010); 	// FAULT_ADDR should contain test address
	CHECKREG_SYM(r7, X25, r0);		// RETX should be value of X25	(HARDCODED ADDR!!)

	//-------------------------------------------------------

	// illegal core MMR: illegal addr in peripheral 06


	LD32(p1, 0xFFE06104);
	LD32(p2, SRAM_BASE_ADDRESS);		// Valid addr (handler will use move this to p1)
	LD32(r1, 0xDEADBEEF);
	R5 = 0;R6 = 0;R7 = 0;			// Exception handler will set these, reset them first

X26:	[ P1 ] = R1;			// Exception should occur here

	CHECKREG(r5,0x2e);		// supv and EXCPT_PROT
	CHECKREG(r6, 0xFFE06104); 	// FAULT_ADDR should contain test address
	CHECKREG_SYM(r7, X26, r0);		// RETX should be value of X26	(HARDCODED ADDR!!)

	//-------------------------------------------------------

	// illegal core MMR: illegal addr in peripheral 07


	LD32(p1, 0xFFE07204);
	LD32(p2, SRAM_BASE_ADDRESS);		// Valid addr (handler will use move this to p1)
	LD32(r1, 0xDEADBEEF);
	R5 = 0;R6 = 0;R7 = 0;			// Exception handler will set these, reset them first

X27:	[ P1 ] = R1;			// Exception should occur here

	CHECKREG(r5,0x2e);		// supv and EXCPT_PROT
	CHECKREG(r6, 0xFFE07204); 	// FAULT_ADDR should contain test address
	CHECKREG_SYM(r7, X27, r0);		// RETX should be value of X27	(HARDCODED ADDR!!)

	//-------------------------------------------------------

	// illegal core MMR: illegal addr in peripheral 08


	LD32(p1, 0xFFE08108);
	LD32(p2, SRAM_BASE_ADDRESS);		// Valid addr (handler will use move this to p1)
	LD32(r1, 0xDEADBEEF);
	R5 = 0;R6 = 0;R7 = 0;			// Exception handler will set these, reset them first

X28:	[ P1 ] = R1;			// Exception should occur here

	CHECKREG(r5,0x2e);		// supv and EXCPT_PROT
	CHECKREG(r6, 0xFFE08108); 	// FAULT_ADDR should contain test address
	CHECKREG_SYM(r7, X28, r0);		// RETX should be value of X28	(HARDCODED ADDR!!)

	//-------------------------------------------------------
User:
	dbg_pass;


	//-------------------------------------------------------
handler:
	R5 = SEQSTAT;	// Get exception cause

	// read and check fail addr (addr_which_causes_exception)
	// should not be set for alignment exception
	RD_MMR(DCPLB_FAULT_ADDR, p0, r6);

	R7 = RETX;	// get address of excepting instruction

	// align the offending address
	P1 = P2;

	RTX;
	// Nops to work around ICache bug
	NOP;NOP;NOP;NOP;NOP;
	NOP;NOP;NOP;NOP;NOP;
