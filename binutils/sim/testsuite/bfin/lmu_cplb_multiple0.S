//Original:/proj/frio/dv/testcases/lmu/lmu_cplb_multiple0/lmu_cplb_multiple0.dsp
// Description: Multiple CPLB Hit exceptions
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
	WR_MMR(DCPLB_ADDR0,  0x10000000, p0, r0);
	WR_MMR(DCPLB_ADDR1,  0x10000000, p0, r0);
	WR_MMR(DCPLB_ADDR2,  0x10000000, p0, r0);
	WR_MMR(DCPLB_ADDR3,  0x10000000, p0, r0);
	WR_MMR(DCPLB_ADDR4,  0x10000000, p0, r0);
	WR_MMR(DCPLB_ADDR5,  0x10000000, p0, r0);
	WR_MMR(DCPLB_ADDR6,  0x10000000, p0, r0);
	WR_MMR(DCPLB_ADDR7,  0x10000000, p0, r0);
	WR_MMR(DCPLB_ADDR8,  0x10000000, p0, r0);
	WR_MMR(DCPLB_ADDR9,  0x10000000, p0, r0);
	WR_MMR(DCPLB_ADDR10, 0x10000000, p0, r0);
	WR_MMR(DCPLB_ADDR11, 0x10000000, p0, r0);
	WR_MMR(DCPLB_ADDR12, 0x10000000, p0, r0);
	WR_MMR(DCPLB_ADDR13, 0x10000000, p0, r0);
	WR_MMR(DCPLB_ADDR14, 0x10000000, p0, r0);

	//  MMR space
	WR_MMR(DCPLB_ADDR15, 0xFFC00000, p0, r0);
	WR_MMR(DCPLB_DATA15, PAGE_SIZE_4M|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR, p0, r0);

	// setup interrupt controller with exception handler address
	WR_MMR_LABEL(EVT3,  handler, p0, r1);
	WR_MMR_LABEL(EVT15, int_15, p0, r1);
	WR_MMR(EVT_IMASK, 0xFFFFFFFF, p0, r0);
	WR_MMR(EVT_OVERRIDE, 0x00000000, p0, r0);
	CSYNC;

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

handler:
	// generic protection exception handler
	// Inputs:
	//	p2:	addr of CPLB entry to be modified	( current test)
	//
	// Outputs:
	//	r4:	SEQSTAT
	//	r5:	DCPLB_FAULT_ADDR
	//	r6:	DCPLB_STATUS
	//	r7:	RETX	(instruction addr where exception occurred)


	R4 = SEQSTAT;	// Get exception cause
	R4 <<= 24;	// Clear HWERRCAUSE + SFTRESET
	R4 >>= 24;

	// read data addr which caused exception
	RD_MMR(DCPLB_FAULT_ADDR, p0, r5);

	RD_MMR(DCPLB_STATUS, p0, r6);

	R7 = RETX;	// get address of excepting instruction

	// disable the offending CPLB entries
	R2 = 0;
	[ P2 ] = R2;

	CSYNC;

	// return from exception and re-execute offending instruction
	RTX;

	// Nops to work around ICache bug
	NOP;NOP;NOP;NOP;NOP;
	NOP;NOP;NOP;NOP;NOP;


int_15:
	// Interrupt 15 handler - test will run in supervisor mode

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA0, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA1, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x41C6 (Z);
	LD32(p2, DCPLB_DATA1);

X0_1:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA0, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB0|FAULT_CPLB1));
	CHECKREG_SYM(r7, X0_1, r0);		// RETX should be value of X0_1	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA0, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA2, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x167E (Z);
	LD32(p2, DCPLB_DATA2);

X0_2:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA0, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB0|FAULT_CPLB2));
	CHECKREG_SYM(r7, X0_2, r0);		// RETX should be value of X0_2	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA0, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA3, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x2781 (Z);
	LD32(p2, DCPLB_DATA3);

X0_3:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA0, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB0|FAULT_CPLB3));
	CHECKREG_SYM(r7, X0_3, r0);		// RETX should be value of X0_3	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA0, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA4, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x446B (Z);
	LD32(p2, DCPLB_DATA4);

X0_4:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA0, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB0|FAULT_CPLB4));
	CHECKREG_SYM(r7, X0_4, r0);		// RETX should be value of X0_4	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA0, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA5, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x794B (Z);
	LD32(p2, DCPLB_DATA5);

X0_5:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA0, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB0|FAULT_CPLB5));
	CHECKREG_SYM(r7, X0_5, r0);		// RETX should be value of X0_5	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA0, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA6, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x15FB (Z);
	LD32(p2, DCPLB_DATA6);

X0_6:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA0, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB0|FAULT_CPLB6));
	CHECKREG_SYM(r7, X0_6, r0);		// RETX should be value of X0_6	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA0, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA7, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x59E2 (Z);
	LD32(p2, DCPLB_DATA7);

X0_7:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA0, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB0|FAULT_CPLB7));
	CHECKREG_SYM(r7, X0_7, r0);		// RETX should be value of X0_7	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA0, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA8, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x1CFB (Z);
	LD32(p2, DCPLB_DATA8);

X0_8:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA0, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB0|FAULT_CPLB8));
	CHECKREG_SYM(r7, X0_8, r0);		// RETX should be value of X0_8	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA0, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA9, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x3F54 (Z);
	LD32(p2, DCPLB_DATA9);

X0_9:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA0, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB0|FAULT_CPLB9));
	CHECKREG_SYM(r7, X0_9, r0);		// RETX should be value of X0_9	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA0, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA10, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x0FF6 (Z);
	LD32(p2, DCPLB_DATA10);

X0_10:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA0, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB0|FAULT_CPLB10));
	CHECKREG_SYM(r7, X0_10, r0);		// RETX should be value of X0_10	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA0, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA11, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x0ABD (Z);
	LD32(p2, DCPLB_DATA11);

X0_11:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA0, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB0|FAULT_CPLB11));
	CHECKREG_SYM(r7, X0_11, r0);		// RETX should be value of X0_11	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA0, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA12, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x31DF (Z);
	LD32(p2, DCPLB_DATA12);

X0_12:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA0, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB0|FAULT_CPLB12));
	CHECKREG_SYM(r7, X0_12, r0);		// RETX should be value of X0_12	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA0, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA13, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x237C (Z);
	LD32(p2, DCPLB_DATA13);

X0_13:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA0, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB0|FAULT_CPLB13));
	CHECKREG_SYM(r7, X0_13, r0);		// RETX should be value of X0_13	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA0, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA14, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x2F1C (Z);
	LD32(p2, DCPLB_DATA14);

X0_14:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA0, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB0|FAULT_CPLB14));
	CHECKREG_SYM(r7, X0_14, r0);		// RETX should be value of X0_14	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA1, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA2, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x7DE1 (Z);
	LD32(p2, DCPLB_DATA2);

X1_2:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA1, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB1|FAULT_CPLB2));
	CHECKREG_SYM(r7, X1_2, r0);		// RETX should be value of X1_2	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA1, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA3, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x4487 (Z);
	LD32(p2, DCPLB_DATA3);

X1_3:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA1, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB1|FAULT_CPLB3));
	CHECKREG_SYM(r7, X1_3, r0);		// RETX should be value of X1_3	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA1, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA4, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x6201 (Z);
	LD32(p2, DCPLB_DATA4);

X1_4:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA1, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB1|FAULT_CPLB4));
	CHECKREG_SYM(r7, X1_4, r0);		// RETX should be value of X1_4	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA1, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA5, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x52BF (Z);
	LD32(p2, DCPLB_DATA5);

X1_5:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA1, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB1|FAULT_CPLB5));
	CHECKREG_SYM(r7, X1_5, r0);		// RETX should be value of X1_5	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA1, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA6, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x6231 (Z);
	LD32(p2, DCPLB_DATA6);

X1_6:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA1, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB1|FAULT_CPLB6));
	CHECKREG_SYM(r7, X1_6, r0);		// RETX should be value of X1_6	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA1, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA7, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x63DE (Z);
	LD32(p2, DCPLB_DATA7);

X1_7:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA1, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB1|FAULT_CPLB7));
	CHECKREG_SYM(r7, X1_7, r0);		// RETX should be value of X1_7	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA1, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA8, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x6956 (Z);
	LD32(p2, DCPLB_DATA8);

X1_8:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA1, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB1|FAULT_CPLB8));
	CHECKREG_SYM(r7, X1_8, r0);		// RETX should be value of X1_8	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA1, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA9, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x1372 (Z);
	LD32(p2, DCPLB_DATA9);

X1_9:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA1, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB1|FAULT_CPLB9));
	CHECKREG_SYM(r7, X1_9, r0);		// RETX should be value of X1_9	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA1, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA10, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x500F (Z);
	LD32(p2, DCPLB_DATA10);

X1_10:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA1, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB1|FAULT_CPLB10));
	CHECKREG_SYM(r7, X1_10, r0);		// RETX should be value of X1_10	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA1, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA11, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x2847 (Z);
	LD32(p2, DCPLB_DATA11);

X1_11:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA1, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB1|FAULT_CPLB11));
	CHECKREG_SYM(r7, X1_11, r0);		// RETX should be value of X1_11	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA1, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA12, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x2C67 (Z);
	LD32(p2, DCPLB_DATA12);

X1_12:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA1, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB1|FAULT_CPLB12));
	CHECKREG_SYM(r7, X1_12, r0);		// RETX should be value of X1_12	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA1, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA13, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x7566 (Z);
	LD32(p2, DCPLB_DATA13);

X1_13:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA1, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB1|FAULT_CPLB13));
	CHECKREG_SYM(r7, X1_13, r0);		// RETX should be value of X1_13	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA1, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA14, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x4287 (Z);
	LD32(p2, DCPLB_DATA14);

X1_14:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA1, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB1|FAULT_CPLB14));
	CHECKREG_SYM(r7, X1_14, r0);		// RETX should be value of X1_14	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA2, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA3, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x3359 (Z);
	LD32(p2, DCPLB_DATA3);

X2_3:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA2, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB2|FAULT_CPLB3));
	CHECKREG_SYM(r7, X2_3, r0);		// RETX should be value of X2_3	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA2, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA4, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x4DAA (Z);
	LD32(p2, DCPLB_DATA4);

X2_4:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA2, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB2|FAULT_CPLB4));
	CHECKREG_SYM(r7, X2_4, r0);		// RETX should be value of X2_4	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA2, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA5, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x6488 (Z);
	LD32(p2, DCPLB_DATA5);

X2_5:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA2, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB2|FAULT_CPLB5));
	CHECKREG_SYM(r7, X2_5, r0);		// RETX should be value of X2_5	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA2, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA6, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x773C (Z);
	LD32(p2, DCPLB_DATA6);

X2_6:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA2, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB2|FAULT_CPLB6));
	CHECKREG_SYM(r7, X2_6, r0);		// RETX should be value of X2_6	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA2, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA7, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x6F59 (Z);
	LD32(p2, DCPLB_DATA7);

X2_7:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA2, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB2|FAULT_CPLB7));
	CHECKREG_SYM(r7, X2_7, r0);		// RETX should be value of X2_7	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA2, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA8, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x6EEA (Z);
	LD32(p2, DCPLB_DATA8);

X2_8:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA2, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB2|FAULT_CPLB8));
	CHECKREG_SYM(r7, X2_8, r0);		// RETX should be value of X2_8	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA2, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA9, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x5656 (Z);
	LD32(p2, DCPLB_DATA9);

X2_9:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA2, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB2|FAULT_CPLB9));
	CHECKREG_SYM(r7, X2_9, r0);		// RETX should be value of X2_9	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA2, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA10, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x6113 (Z);
	LD32(p2, DCPLB_DATA10);

X2_10:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA2, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB2|FAULT_CPLB10));
	CHECKREG_SYM(r7, X2_10, r0);		// RETX should be value of X2_10	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA2, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA11, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x4A7B (Z);
	LD32(p2, DCPLB_DATA11);

X2_11:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA2, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB2|FAULT_CPLB11));
	CHECKREG_SYM(r7, X2_11, r0);		// RETX should be value of X2_11	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA2, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA12, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x31D2 (Z);
	LD32(p2, DCPLB_DATA12);

X2_12:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA2, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB2|FAULT_CPLB12));
	CHECKREG_SYM(r7, X2_12, r0);		// RETX should be value of X2_12	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA2, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA13, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x2D85 (Z);
	LD32(p2, DCPLB_DATA13);

X2_13:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA2, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB2|FAULT_CPLB13));
	CHECKREG_SYM(r7, X2_13, r0);		// RETX should be value of X2_13	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA2, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA14, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x19A1 (Z);
	LD32(p2, DCPLB_DATA14);

X2_14:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA2, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB2|FAULT_CPLB14));
	CHECKREG_SYM(r7, X2_14, r0);		// RETX should be value of X2_14	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA3, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA4, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x69D8 (Z);
	LD32(p2, DCPLB_DATA4);

X3_4:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA3, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB3|FAULT_CPLB4));
	CHECKREG_SYM(r7, X3_4, r0);		// RETX should be value of X3_4	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA3, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA5, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x353C (Z);
	LD32(p2, DCPLB_DATA5);

X3_5:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA3, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB3|FAULT_CPLB5));
	CHECKREG_SYM(r7, X3_5, r0);		// RETX should be value of X3_5	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA3, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA6, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x3B54 (Z);
	LD32(p2, DCPLB_DATA6);

X3_6:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA3, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB3|FAULT_CPLB6));
	CHECKREG_SYM(r7, X3_6, r0);		// RETX should be value of X3_6	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA3, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA7, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x7D55 (Z);
	LD32(p2, DCPLB_DATA7);

X3_7:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA3, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB3|FAULT_CPLB7));
	CHECKREG_SYM(r7, X3_7, r0);		// RETX should be value of X3_7	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA3, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA8, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x102F (Z);
	LD32(p2, DCPLB_DATA8);

X3_8:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA3, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB3|FAULT_CPLB8));
	CHECKREG_SYM(r7, X3_8, r0);		// RETX should be value of X3_8	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA3, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA9, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x1B37 (Z);
	LD32(p2, DCPLB_DATA9);

X3_9:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA3, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB3|FAULT_CPLB9));
	CHECKREG_SYM(r7, X3_9, r0);		// RETX should be value of X3_9	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA3, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA10, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x7AAE (Z);
	LD32(p2, DCPLB_DATA10);

X3_10:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA3, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB3|FAULT_CPLB10));
	CHECKREG_SYM(r7, X3_10, r0);		// RETX should be value of X3_10	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA3, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA11, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x5E65 (Z);
	LD32(p2, DCPLB_DATA11);

X3_11:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA3, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB3|FAULT_CPLB11));
	CHECKREG_SYM(r7, X3_11, r0);		// RETX should be value of X3_11	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA3, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA12, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x345B (Z);
	LD32(p2, DCPLB_DATA12);

X3_12:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA3, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB3|FAULT_CPLB12));
	CHECKREG_SYM(r7, X3_12, r0);		// RETX should be value of X3_12	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA3, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA13, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x63DA (Z);
	LD32(p2, DCPLB_DATA13);

X3_13:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA3, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB3|FAULT_CPLB13));
	CHECKREG_SYM(r7, X3_13, r0);		// RETX should be value of X3_13	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA3, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA14, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x6102 (Z);
	LD32(p2, DCPLB_DATA14);

X3_14:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA3, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB3|FAULT_CPLB14));
	CHECKREG_SYM(r7, X3_14, r0);		// RETX should be value of X3_14	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA4, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA5, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x7A79 (Z);
	LD32(p2, DCPLB_DATA5);

X4_5:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA4, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB4|FAULT_CPLB5));
	CHECKREG_SYM(r7, X4_5, r0);		// RETX should be value of X4_5	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA4, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA6, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x0398 (Z);
	LD32(p2, DCPLB_DATA6);

X4_6:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA4, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB4|FAULT_CPLB6));
	CHECKREG_SYM(r7, X4_6, r0);		// RETX should be value of X4_6	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA4, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA7, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x28CC (Z);
	LD32(p2, DCPLB_DATA7);

X4_7:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA4, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB4|FAULT_CPLB7));
	CHECKREG_SYM(r7, X4_7, r0);		// RETX should be value of X4_7	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA4, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA8, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x60E3 (Z);
	LD32(p2, DCPLB_DATA8);

X4_8:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA4, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB4|FAULT_CPLB8));
	CHECKREG_SYM(r7, X4_8, r0);		// RETX should be value of X4_8	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA4, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA9, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x1F1A (Z);
	LD32(p2, DCPLB_DATA9);

X4_9:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA4, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB4|FAULT_CPLB9));
	CHECKREG_SYM(r7, X4_9, r0);		// RETX should be value of X4_9	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA4, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA10, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x4B76 (Z);
	LD32(p2, DCPLB_DATA10);

X4_10:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA4, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB4|FAULT_CPLB10));
	CHECKREG_SYM(r7, X4_10, r0);		// RETX should be value of X4_10	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA4, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA11, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x058E (Z);
	LD32(p2, DCPLB_DATA11);

X4_11:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA4, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB4|FAULT_CPLB11));
	CHECKREG_SYM(r7, X4_11, r0);		// RETX should be value of X4_11	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA4, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA12, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x7A5F (Z);
	LD32(p2, DCPLB_DATA12);

X4_12:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA4, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB4|FAULT_CPLB12));
	CHECKREG_SYM(r7, X4_12, r0);		// RETX should be value of X4_12	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA4, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA13, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x28D9 (Z);
	LD32(p2, DCPLB_DATA13);

X4_13:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA4, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB4|FAULT_CPLB13));
	CHECKREG_SYM(r7, X4_13, r0);		// RETX should be value of X4_13	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA4, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA14, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x0799 (Z);
	LD32(p2, DCPLB_DATA14);

X4_14:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA4, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB4|FAULT_CPLB14));
	CHECKREG_SYM(r7, X4_14, r0);		// RETX should be value of X4_14	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA5, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA6, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x388F (Z);
	LD32(p2, DCPLB_DATA6);

X5_6:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA5, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB5|FAULT_CPLB6));
	CHECKREG_SYM(r7, X5_6, r0);		// RETX should be value of X5_6	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA5, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA7, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x751F (Z);
	LD32(p2, DCPLB_DATA7);

X5_7:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA5, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB5|FAULT_CPLB7));
	CHECKREG_SYM(r7, X5_7, r0);		// RETX should be value of X5_7	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA5, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA8, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x493F (Z);
	LD32(p2, DCPLB_DATA8);

X5_8:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA5, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB5|FAULT_CPLB8));
	CHECKREG_SYM(r7, X5_8, r0);		// RETX should be value of X5_8	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA5, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA9, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x0F36 (Z);
	LD32(p2, DCPLB_DATA9);

X5_9:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA5, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB5|FAULT_CPLB9));
	CHECKREG_SYM(r7, X5_9, r0);		// RETX should be value of X5_9	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA5, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA10, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x48EE (Z);
	LD32(p2, DCPLB_DATA10);

X5_10:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA5, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB5|FAULT_CPLB10));
	CHECKREG_SYM(r7, X5_10, r0);		// RETX should be value of X5_10	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA5, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA11, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x2043 (Z);
	LD32(p2, DCPLB_DATA11);

X5_11:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA5, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB5|FAULT_CPLB11));
	CHECKREG_SYM(r7, X5_11, r0);		// RETX should be value of X5_11	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA5, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA12, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x3F78 (Z);
	LD32(p2, DCPLB_DATA12);

X5_12:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA5, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB5|FAULT_CPLB12));
	CHECKREG_SYM(r7, X5_12, r0);		// RETX should be value of X5_12	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA5, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA13, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x1E4D (Z);
	LD32(p2, DCPLB_DATA13);

X5_13:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA5, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB5|FAULT_CPLB13));
	CHECKREG_SYM(r7, X5_13, r0);		// RETX should be value of X5_13	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA5, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA14, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x3D0D (Z);
	LD32(p2, DCPLB_DATA14);

X5_14:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA5, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB5|FAULT_CPLB14));
	CHECKREG_SYM(r7, X5_14, r0);		// RETX should be value of X5_14	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA6, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA7, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x33FA (Z);
	LD32(p2, DCPLB_DATA7);

X6_7:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA6, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB6|FAULT_CPLB7));
	CHECKREG_SYM(r7, X6_7, r0);		// RETX should be value of X6_7	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA6, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA8, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x6FBE (Z);
	LD32(p2, DCPLB_DATA8);

X6_8:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA6, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB6|FAULT_CPLB8));
	CHECKREG_SYM(r7, X6_8, r0);		// RETX should be value of X6_8	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA6, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA9, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x36A6 (Z);
	LD32(p2, DCPLB_DATA9);

X6_9:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA6, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB6|FAULT_CPLB9));
	CHECKREG_SYM(r7, X6_9, r0);		// RETX should be value of X6_9	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA6, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA10, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x2DDA (Z);
	LD32(p2, DCPLB_DATA10);

X6_10:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA6, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB6|FAULT_CPLB10));
	CHECKREG_SYM(r7, X6_10, r0);		// RETX should be value of X6_10	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA6, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA11, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x30E4 (Z);
	LD32(p2, DCPLB_DATA11);

X6_11:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA6, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB6|FAULT_CPLB11));
	CHECKREG_SYM(r7, X6_11, r0);		// RETX should be value of X6_11	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA6, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA12, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x0586 (Z);
	LD32(p2, DCPLB_DATA12);

X6_12:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA6, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB6|FAULT_CPLB12));
	CHECKREG_SYM(r7, X6_12, r0);		// RETX should be value of X6_12	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA6, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA13, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x148E (Z);
	LD32(p2, DCPLB_DATA13);

X6_13:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA6, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB6|FAULT_CPLB13));
	CHECKREG_SYM(r7, X6_13, r0);		// RETX should be value of X6_13	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA6, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA14, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x42DC (Z);
	LD32(p2, DCPLB_DATA14);

X6_14:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA6, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB6|FAULT_CPLB14));
	CHECKREG_SYM(r7, X6_14, r0);		// RETX should be value of X6_14	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA7, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA8, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x5929 (Z);
	LD32(p2, DCPLB_DATA8);

X7_8:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA7, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB7|FAULT_CPLB8));
	CHECKREG_SYM(r7, X7_8, r0);		// RETX should be value of X7_8	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA7, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA9, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x0C6D (Z);
	LD32(p2, DCPLB_DATA9);

X7_9:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA7, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB7|FAULT_CPLB9));
	CHECKREG_SYM(r7, X7_9, r0);		// RETX should be value of X7_9	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA7, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA10, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x334E (Z);
	LD32(p2, DCPLB_DATA10);

X7_10:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA7, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB7|FAULT_CPLB10));
	CHECKREG_SYM(r7, X7_10, r0);		// RETX should be value of X7_10	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA7, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA11, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x62FF (Z);
	LD32(p2, DCPLB_DATA11);

X7_11:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA7, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB7|FAULT_CPLB11));
	CHECKREG_SYM(r7, X7_11, r0);		// RETX should be value of X7_11	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA7, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA12, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x1F56 (Z);
	LD32(p2, DCPLB_DATA12);

X7_12:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA7, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB7|FAULT_CPLB12));
	CHECKREG_SYM(r7, X7_12, r0);		// RETX should be value of X7_12	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA7, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA13, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x2BE1 (Z);
	LD32(p2, DCPLB_DATA13);

X7_13:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA7, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB7|FAULT_CPLB13));
	CHECKREG_SYM(r7, X7_13, r0);		// RETX should be value of X7_13	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA7, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA14, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x1D70 (Z);
	LD32(p2, DCPLB_DATA14);

X7_14:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA7, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB7|FAULT_CPLB14));
	CHECKREG_SYM(r7, X7_14, r0);		// RETX should be value of X7_14	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA8, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA9, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x2620 (Z);
	LD32(p2, DCPLB_DATA9);

X8_9:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA8, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB8|FAULT_CPLB9));
	CHECKREG_SYM(r7, X8_9, r0);		// RETX should be value of X8_9	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA8, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA10, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x26FB (Z);
	LD32(p2, DCPLB_DATA10);

X8_10:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA8, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB8|FAULT_CPLB10));
	CHECKREG_SYM(r7, X8_10, r0);		// RETX should be value of X8_10	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA8, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA11, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x718F (Z);
	LD32(p2, DCPLB_DATA11);

X8_11:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA8, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB8|FAULT_CPLB11));
	CHECKREG_SYM(r7, X8_11, r0);		// RETX should be value of X8_11	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA8, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA12, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x04B1 (Z);
	LD32(p2, DCPLB_DATA12);

X8_12:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA8, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB8|FAULT_CPLB12));
	CHECKREG_SYM(r7, X8_12, r0);		// RETX should be value of X8_12	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA8, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA13, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x5358 (Z);
	LD32(p2, DCPLB_DATA13);

X8_13:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA8, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB8|FAULT_CPLB13));
	CHECKREG_SYM(r7, X8_13, r0);		// RETX should be value of X8_13	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA8, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA14, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x3305 (Z);
	LD32(p2, DCPLB_DATA14);

X8_14:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA8, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB8|FAULT_CPLB14));
	CHECKREG_SYM(r7, X8_14, r0);		// RETX should be value of X8_14	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA9, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA10, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x5690 (Z);
	LD32(p2, DCPLB_DATA10);

X9_10:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA9, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB9|FAULT_CPLB10));
	CHECKREG_SYM(r7, X9_10, r0);		// RETX should be value of X9_10	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA9, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA11, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x5DC5 (Z);
	LD32(p2, DCPLB_DATA11);

X9_11:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA9, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB9|FAULT_CPLB11));
	CHECKREG_SYM(r7, X9_11, r0);		// RETX should be value of X9_11	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA9, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA12, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x7809 (Z);
	LD32(p2, DCPLB_DATA12);

X9_12:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA9, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB9|FAULT_CPLB12));
	CHECKREG_SYM(r7, X9_12, r0);		// RETX should be value of X9_12	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA9, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA13, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x1DDC (Z);
	LD32(p2, DCPLB_DATA13);

X9_13:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA9, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB9|FAULT_CPLB13));
	CHECKREG_SYM(r7, X9_13, r0);		// RETX should be value of X9_13	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA9, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA14, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x6B53 (Z);
	LD32(p2, DCPLB_DATA14);

X9_14:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA9, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB9|FAULT_CPLB14));
	CHECKREG_SYM(r7, X9_14, r0);		// RETX should be value of X9_14	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA10, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA11, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x7BCD (Z);
	LD32(p2, DCPLB_DATA11);

X10_11:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA10, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB10|FAULT_CPLB11));
	CHECKREG_SYM(r7, X10_11, r0);		// RETX should be value of X10_11	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA10, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA12, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x63AA (Z);
	LD32(p2, DCPLB_DATA12);

X10_12:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA10, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB10|FAULT_CPLB12));
	CHECKREG_SYM(r7, X10_12, r0);		// RETX should be value of X10_12	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA10, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA13, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x373B (Z);
	LD32(p2, DCPLB_DATA13);

X10_13:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA10, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB10|FAULT_CPLB13));
	CHECKREG_SYM(r7, X10_13, r0);		// RETX should be value of X10_13	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA10, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA14, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x5648 (Z);
	LD32(p2, DCPLB_DATA14);

X10_14:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA10, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB10|FAULT_CPLB14));
	CHECKREG_SYM(r7, X10_14, r0);		// RETX should be value of X10_14	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA11, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA12, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x6799 (Z);
	LD32(p2, DCPLB_DATA12);

X11_12:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA11, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB11|FAULT_CPLB12));
	CHECKREG_SYM(r7, X11_12, r0);		// RETX should be value of X11_12	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA11, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA13, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x1452 (Z);
	LD32(p2, DCPLB_DATA13);

X11_13:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA11, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB11|FAULT_CPLB13));
	CHECKREG_SYM(r7, X11_13, r0);		// RETX should be value of X11_13	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA11, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA14, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x23D3 (Z);
	LD32(p2, DCPLB_DATA14);

X11_14:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA11, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB11|FAULT_CPLB14));
	CHECKREG_SYM(r7, X11_14, r0);		// RETX should be value of X11_14	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA12, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA13, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x1152 (Z);
	LD32(p2, DCPLB_DATA13);

X12_13:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA12, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB12|FAULT_CPLB13));
	CHECKREG_SYM(r7, X12_13, r0);		// RETX should be value of X12_13	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA12, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA14, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x6E9D (Z);
	LD32(p2, DCPLB_DATA14);

X12_14:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA12, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB12|FAULT_CPLB14));
	CHECKREG_SYM(r7, X12_14, r0);		// RETX should be value of X12_14	(HARDCODED ADDR!!)

	//-------------------------------------------------------
	R0 = 0;R1 = 0;R2 = 0;R3 = 0;R4 = 0;R5 = 0;R6 = 0;R7 = 0;

	WR_MMR(DCPLB_DATA13, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DCPLB_DATA14, (PAGE_SIZE_1K|CPLB_VALID|CPLB_DIRTY|CPLB_SUPV_WR|CPLB_USER_RW), p0, r0);
	WR_MMR(DMEM_CONTROL, ENDM | ENDCPLB | DMC_AB_CACHE, p0, r0);
	CSYNC;

	LD32(i1, 0x10000000);
	R1 = 0x6006 (Z);
	LD32(p2, DCPLB_DATA14);

X13_14:	[ I1 ] = R1;		// Exception should occur here

	WR_MMR(DMEM_CONTROL, ENDM | DMC_AB_CACHE, p0, r0);
	CSYNC;
	WR_MMR(DCPLB_DATA13, 0, p0, r0);

	// Now check that handler read correct values
	CHECKREG(r4,0x27);		// supv and EXCPT_PROT
	CHECKREG(r5, 0x10000000);
	CHECKREG(r6, (FAULT_WRITE|FAULT_DAG0|FAULT_SUPV|FAULT_CPLB13|FAULT_CPLB14));
	CHECKREG_SYM(r7, X13_14, r0);		// RETX should be value of X13_14	(HARDCODED ADDR!!)

	//-------------------------------------------------------
User:
	NOP;
	dbg_pass;
