//Original:/proj/frio/dv/testcases/lmu/lmu_excpt_prot1/lmu_excpt_prot1.dsp
// Description: LMU protection exceptions
# mach: bfin
# sim: --environment operating

#include "test.h"
.include "testutils.inc"
start

include(selfcheck.inc)
include(std.inc)
include(mmrs.inc)

//-------------------------------------

// Test LMU/CPLB exceptions

// Basic outline:
//   Set exception handler
//   program CPLB Entries
//   Enable CPLB in DMEM_CNTL
//   perform access
//   verify exception occurred

CHECK_INIT(p5, 0xEFFFFFFC);

	A0 = 0;

//-------------------------
// Zero the CPLB Address and Data regs.

	LD32(p0, DCPLB_ADDR0);
	R0 = 0;
	[ P0 ++ ] = R0;	// 0
	[ P0 ++ ] = R0;	// 1
	[ P0 ++ ] = R0;	// 2
	[ P0 ++ ] = R0;	// 3
	[ P0 ++ ] = R0;	// 4
	[ P0 ++ ] = R0;	// 5
	[ P0 ++ ] = R0;	// 6
	[ P0 ++ ] = R0;	// 7
	[ P0 ++ ] = R0;	// 8
	[ P0 ++ ] = R0;	// 9
	[ P0 ++ ] = R0;	// 10
	[ P0 ++ ] = R0;	// 11
	[ P0 ++ ] = R0;	// 12
	[ P0 ++ ] = R0;	// 13
	[ P0 ++ ] = R0;	// 14
	[ P0 ++ ] = R0;	// 15

	LD32(p0, DCPLB_DATA0);
	[ P0 ++ ] = R0;	// 0
	[ P0 ++ ] = R0;	// 1
	[ P0 ++ ] = R0;	// 2
	[ P0 ++ ] = R0;	// 3
	[ P0 ++ ] = R0;	// 4
	[ P0 ++ ] = R0;	// 5
	[ P0 ++ ] = R0;	// 6
	[ P0 ++ ] = R0;	// 7
	[ P0 ++ ] = R0;	// 8
	[ P0 ++ ] = R0;	// 9
	[ P0 ++ ] = R0;	// 10
	[ P0 ++ ] = R0;	// 11
	[ P0 ++ ] = R0;	// 12
	[ P0 ++ ] = R0;	// 13
	[ P0 ++ ] = R0;	// 14
	[ P0 ++ ] = R0;	// 15

	// Now set the CPLB entries we will need

























	// Data area for the desired error
	WR_MMR(DCPLB_ADDR0, 0x800, p0, r0);
	WR_MMR(DCPLB_ADDR1, 0x1000, p0, r0);
	WR_MMR(DCPLB_DATA0, PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_L1_CACHABLE, p0, r0);
	WR_MMR(DCPLB_ADDR2, 0x2000, p0, r0);
	WR_MMR(DCPLB_ADDR3, 0x3000, p0, r0);
	WR_MMR(DCPLB_ADDR4, 0x4000, p0, r0);
	WR_MMR(DCPLB_ADDR5, 0x5000, p0, r0);
	WR_MMR(DCPLB_ADDR6, 0x6000, p0, r0);
	WR_MMR(DCPLB_ADDR7, 0x7000, p0, r0);

	// CHECKREG segment
	WR_MMR(DCPLB_ADDR14, 0xEFFFFC00, p0, r0);
	WR_MMR(DCPLB_DATA14, PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_WT|CPLB_L1_CACHABLE|CPLB_SUPV_WR|CPLB_USER_RW, p0, r0);

	//  MMR space
	WR_MMR(DCPLB_ADDR15, 0xFFC00000, p0, r0);
	WR_MMR(DCPLB_DATA15, PAGE_SIZE_4M|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR, p0, r0);

	// setup interrupt controller with exception handler address
	WR_MMR_LABEL(EVT3,  handler, p0, r1);
	WR_MMR_LABEL(EVT15, int_15, p0, r1);
	WR_MMR(EVT_IMASK, 0xFFFFFFFF, p0, r0);
	WR_MMR(EVT_OVERRIDE, 0x00000000, p0, r0);

	// enable CPLB
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	NOP;NOP;NOP;NOP;NOP;	// in lieu of CSYNC

	// Address for slot 0 accesses
	// LD32(p4, 0xEFFFFFF8);

	// go to user mode. and enable exceptions
	LD32_LABEL(r0, User);
	RETI = R0;

	// But first raise interrupt 15 so we can do one test
	// in supervisor mode.
	RAISE 15;
	NOP;

	RTI;

	// Nops to work around ICache bug
	NOP;NOP;NOP;NOP;NOP;
	NOP;NOP;NOP;NOP;NOP;


int_15:
	// Interrupt 15 handler - needed to try supervisor access with exceptions enabled
	//-------------------------------------------------------
	// Protection violation - Illegal Supervisor Write Access
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	LD32(i1, 0x800);
	LD32(r1, 0xDEADBEEF);

	LD32(p2, DCPLB_DATA0);
	LD32(r2, PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_L1_CACHABLE|CPLB_SUPV_WR);

	LD32(p3, DCPLB_DATA1);
	LD32(r3, PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_L1_CACHABLE|CPLB_SUPV_WR);


X0:	//[p1] = r1;		// Exception should occur here
	A0 = 0 || NOP || [ I1 ] = R1;	// test access with DAG1


	// Now check that handler read correct values
	CHECKREG(r4,0x23);			// supv and EXCPT_PROT
	CHECKREG(r5, 0x800);
	CHECKREG(r6, (FAULT_SUPV|FAULT_WRITE|FAULT_DAG1 | FAULT_CPLB0));
	CHECKREG_SYM(r7, X0, r0);			// RETX should be value of X0	(HARDCODED ADDR!!)

	// go to user mode. and enable exceptions
	LD32_LABEL(r0, User);
	RTI;
	NOP;NOP;NOP;NOP;NOP;
	NOP;NOP;NOP;NOP;NOP;


User:
	NOP;NOP;NOP;NOP;NOP;

	//-------------------------------------------------------
	// Protection violation - Illegal User Write Access
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	LD32(i1, 0x1000);
	LD32(r1, 0xDEADBEEF);


	// values to fix up current test
	LD32(p2, DCPLB_DATA1);
	LD32(r2, PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_L1_CACHABLE|CPLB_USER_RW|CPLB_SUPV_WR);

	// values for next test
	LD32(p3, DCPLB_DATA2);
	LD32(r3, PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_L1_CACHABLE);

X1:	//[p1] = r1;		// Exception should occur here
	A0 = 0 || NOP || [ I1 ] = R1;	// test access with DAG1

	// Now check that handler read correct values

	CHECKREG(r4,0x23);			// supv and EXCPT_PROT
	CHECKREG(r5, 0x1000);
	CHECKREG(r6, (FAULT_USER|FAULT_WRITE|FAULT_DAG1 | FAULT_CPLB1));
	CHECKREG_SYM(r7, X1, r0);			// RETX should be value of X1	(HARDCODED ADDR!!)


	//-------------------------------------------------------
	// Protection violation - Illegal User Read Access
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	LD32(i1, 0x2000);
	LD32(r1, 0xDEADBEEF);

	LD32(p2, DCPLB_DATA2);
	LD32(r2, PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_L1_CACHABLE|CPLB_USER_RO|CPLB_SUPV_WR);

	LD32(p3, DCPLB_DATA3);
	LD32(r3, PAGE_SIZE_1K|CPLB_VALID|CPLB_L1_CACHABLE|CPLB_USER_RW|CPLB_SUPV_WR);

X2:	//[p1] = r1;		// Exception should occur here
	A0 = 0 || NOP || R0 = [ I1 ];	// test access with DAG1


	// Now check that handler read correct values
	CHECKREG(r4,0x23);			// supv and EXCPT_PROT
	CHECKREG(r5, 0x2000);
	CHECKREG(r6, (FAULT_USER|FAULT_READ|FAULT_DAG1 | FAULT_CPLB2));
	CHECKREG_SYM(r7, X2, r0);			// RETX should be value of X2	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	// Protection violation - Illegal Dirty Page Access
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	LD32(i1, 0x3000);
	LD32(r1, 0xDEADBEEF);

	LD32(p2, DCPLB_DATA3);
	LD32(r2, PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_L1_CACHABLE|CPLB_USER_RW|CPLB_SUPV_WR);

	LD32(p3, DCPLB_DATA4);
	LD32(r3, PAGE_SIZE_1K|CPLB_VALID|CPLB_DA0ACC|CPLB_DIRTY|CPLB_L1_CACHABLE|CPLB_SUPV_WR);


X3:	//[p1] = r1;		// Exception should occur here
	A0 = 0 || NOP || [ I1 ] = R1;	// test access with DAG1


	// Now check that handler read correct values
	CHECKREG(r4,0x23);			// supv and EXCPT_PROT
	CHECKREG(r5, 0x3000);
	CHECKREG(r6, (FAULT_USER|FAULT_WRITE|FAULT_DAG1 | FAULT_CPLB3));
	CHECKREG_SYM(r7, X3, r0);			// RETX should be value of X3	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	// Protection violation - Illegal DAG1 Access
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	LD32(i1, 0x4000);
	LD32(r1, 0xDEADBEEF);

	LD32(p2, DCPLB_DATA4);
	LD32(r2, PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_L1_CACHABLE|CPLB_USER_RW|CPLB_SUPV_WR);

	LD32(p3, DCPLB_DATA5);
	LD32(r3, PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_L1_CACHABLE|CPLB_USER_RW|CPLB_SUPV_WR);


X4:	//[p1] = r1;		// Exception should occur here
	A0 = 0 || NOP || [ I1 ] = R1;	// test access with DAG1


	// Now check that handler read correct values
	CHECKREG(r4,0x23);			// supv and EXCPT_PROT
	CHECKREG(r5, 0x4000);
	CHECKREG(r6, (FAULT_USER|FAULT_WRITE|FAULT_DAG1 | FAULT_CPLB4));
	CHECKREG_SYM(r7, X4, r0);			// RETX should be value of X4	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	// L1Miss not implemented yet - skip for now....

//	//-------------------------------------------------------
//	// Protection violation - L1 Miss
//	r0=0;r1=0;r2=0;r3=0;r4=0;r5=0;r6=0;r7=0;
//
//	LD32(p1, 0x6000);
//	LD32(r1, 0xDEADBEEF);
//
//	LD32(p2, DCPLB_DATA6);
//	LD32(r2, PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_L1_CACHABLE|CPLB_USER_RW|CPLB_SUPV_WR);
//
//	LD32(p3, DCPLB_DATA7);
//	LD32(r3, PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_USER_RW|CPLB_SUPV_WR);
//
//
//X6:	//[p1] = r1;		// Exception should occur here
//	r0 = [p1];
//
//
//	// Now check that handler read correct values
//	CHECKREG(r4,0x23);			// supv and EXCPT_PROT
//	CHECKREG(r5, 0x6000);
//	// CHECKREG(r6, FAULT_USER|FAULT_WRITE|FAULT_DAG1 | FAULT_CPLB6);
//	CHECKREG_SYM(r7, X6, r0);			// RETX should be value of X6	(HARDCODED ADDR!!)


	//-------------------------------------------------------
	dbg_pass;


handler:
	// generic protection exception handler
	// Inputs:
	//	p2:	addr of CPLB entry to be modified	( current test)
	//	r2:	new data for CPLB entry
	//
	//	p3:	addr of CPLB entry to be modified	( next test)
	//	r3:	new data for CPLB entry
	//
	// Outputs:
	//	r4:	SEQSTAT
	//	r5:	DCPLB_FAULT_ADDR
	//	r6:	DCPLB_STATUS
	//	r7:	RETX	(instruction addr where exception occurred)


	R4 = SEQSTAT;	// Get exception cause

	// read data addr which caused exception
	RD_MMR(DCPLB_FAULT_ADDR, p0, r5);
	RD_MMR(DCPLB_STATUS, p0, r6);

	// Reset status regs
	WR_MMR(DCPLB_FAULT_ADDR, 0, p0, r0);
	WR_MMR(DCPLB_STATUS,     0, p0, r0);

	R7 = RETX;	// get address of excepting instruction


	// modify CPLB to allow access.  Main pgm passes in addr and data
	[ P2 ] = R2;

	// Set up for next test
	[ P3 ] = R3;

	NOP;NOP;NOP;NOP;NOP;NOP;NOP;	// in lieu of CSYNC;

	// return from exception and re-execute offending instruction
	RTX;

	// Nops to work around ICache bug
	NOP;NOP;NOP;NOP;NOP;
	NOP;NOP;NOP;NOP;NOP;


.section MEM_0x800,"aw"
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000

.section MEM_0x1000,"aw"
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000

.section MEM_0x2000,"aw"
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000

.section MEM_0x3000,"aw"
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000

.section MEM_0x4000,"aw"
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000

.section MEM_0x5000,"aw"
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000

// Need a cache miss to test CPLB_L1REF
//.data 0x6000
//	.dd 0x00000000
//	.dd 0x00000000
//	.dd 0x00000000
//	.dd 0x00000000

.section MEM_0x7000,"aw"
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
