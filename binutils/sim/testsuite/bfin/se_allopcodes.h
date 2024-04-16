/*
 * set up pointers to valid data (32Meg), to reduce address violations
 */
.macro reset_dags
	imm32 r0, 0x2000000;
	l0 = 0; l1 = 0; l2 = 0; l3 = 0;
	p0 = r0; p1 = r0; p2 = r0; p3 = r0; p4 = r0; p5 = r0;
	usp = r0; fp = r0;
	i0 = r0; i1 = r0; i2 = r0; i3 = r0;
	b0 = r0; b1 = r0; b2 = r0; b3 = r0;
.endm

#if SE_ALL_BITS == 32
# define LOAD_PFX
#elif SE_ALL_BITS == 16
# define LOAD_PFX W
#else
# error "Please define SE_ALL_BITS"
#endif

/*
 * execute a test of an opcode space.  host test
 * has to fill out a number of callbacks.
 *
 *	se_all_insn_init
 *		the first insn to start executing
 *	se_all_insn_table
 *		the table of insn ranges and expected seqstat
 *
 *	se_all_load_insn
 *	  in: P5
 *	  out: R0, R2
 *	  scratch: R1
 *		load current user insn via register P5 into R0.
 *		register R2 is available for caching with se_all_next_insn.
 *	se_all_load_table
 *	  in: P1
 *	  out: R7, R6, R5
 *	  scratch: R1
 *		load insn range/seqstat entry from table via register P1
 *		R7: low range
 *		R6: high range
 *		R5: seqstat
 *
 *	se_all_next_insn
 *	  in: P5, R2
 *	  out: <nothing>
 *	  scratch: all but P5
 *		advance current insn to next one for testing.  register R2
 *		is retained from se_all_load_insn.  write out new insn to
 *		the location via register P5.
 *
 *	se_all_new_insn_stub
 *	se_all_new_insn_log
 *		for handling of new insns ... generally not needed once done
 */
.macro se_all_test
	start

	/* Set up exception handler */
	imm32 P4, EVT3;
	loadsym R1, _evx;
	[P4] = R1;

	/* set up the _location */
	loadsym P0, _location
	loadsym P1, _table;
	[P0] = P1;

	/* Enable single stepping */
	R0 = 1;
	SYSCFG = R0;

	/* Lower to the code we want to single step through */
	loadsym P1, _usr;
	RETI = P1;

	/* set up pointers to valid data (32Meg), to reduce address violations */
	reset_dags

	RTI;

pass_lvl:
	dbg_pass;
fail_lvl:
	dbg_fail;

_evx:
	/* Make sure exception reason is as we expect */
	R3 = SEQSTAT;
	R4 = 0x3f;
	R3 = R3 & R4;

	/* find a match */
	loadsym P5, _usr;
	loadsym P4, _location;
	P1 = [P4];
	se_all_load_insn

_match:
	P2 = P1;
	se_all_load_table

	/* is this the end of the table? */
	CC = R7 == 0;
	IF CC jump _new_instruction;

	/* is the opcode (R0) greater than the 2nd entry in the table (R6) */
	/* if so look at the next line in the table */
	CC = R6 < R0;
	if CC jump _match;

	/* is the opcode (R0) smaller than the first entry in the table (R7) */
	/* this means it's somewhere between the two lines, and should be legal */
	CC = R7 <= R0;
	if !CC jump _legal_instruction;

	/* is the current EXCAUSE (R3), the same as the table (R5) */
	/* if not, fail */
	CC = R3 == R5
	if !CC jump fail_lvl;

_match_done:
	/* back up, and store the location to search next */
	[P4] = P2;

	/* it matches, so fall through */
	jump _next_instruction;

_new_instruction:
	/* The table is generated in memory and can be extracted:
	   (gdb) dump binary memory bin &table next_location

	   16bit:
	   $ od -j6 -x --width=4 bin | \
	     awk '{ s=last; e=strtonum("0x"$2); \
	       printf "\t.dw 0x%04x,\t0x%04x,\t\t0x%02x\n", \
	          s, e-1, strtonum("0x"seq); \
	       last=e; seq=$3}'

	   32bit:
	   $ od -j12 -x --width=8 bin | \
	     awk '{ s=last; e=strtonum("0x"$3$2); \
	       printf "\t.dw 0x%04x, 0x%04x,\t0x%04x, 0x%04x,\t\t0x%02x, 0\n", \
	          and(s,0xffff), rshift(s,16), and(e-1,0xffff), rshift(e-1,16), \
	          strtonum("0x"seq); \
	       last=e; seq=$3}'

	   This should be much faster than dumping over serial/jtag.  */
	se_all_new_insn_stub

	/* output the insn (R0) and excause (R3) if diff from last */
	loadsym P0, _last_excause;
	R2 = [P0];
	CC = R2 == R3;
	IF CC jump _next_instruction;
	[P0] = R3;

	se_all_new_insn_log

_legal_instruction:
	R4 = 0x10;
	CC = R3 == R4;
	IF !CC JUMP fail_lvl;
	/* it wasn't in the list, and was a single step, so fall through */

_next_instruction:
	se_all_next_insn

.ifdef BFIN_JTAG
	/* Make sure the opcode isn't in a write buffer */
	SSYNC;
.endif

	R1 = P5;
	RETX = R1;

	/* set up pointers to valid data (32Meg), to reduce address violations */
	reset_dags
	RETS = r0;

	RTX;

.section .text.usr
	.align 4
_usr:
	se_all_insn_init
	loadsym P0, fail_lvl;
	JUMP (P0);

.data
	.align 4;
_last_excause:
	.dd 0xffff
_next_location:
	.dd _table_end
_location:
	.dd 0
_table:
	se_all_insn_table
_table_end:
.endm

.macro se_all_load_table
	R7 = LOAD_PFX[P1++];
	R6 = LOAD_PFX[P1++];
	R5 = LOAD_PFX[P1++];
.endm

#ifndef SE_ALL_NEW_INSN_STUB
.macro se_all_new_insn_stub
	jump fail_lvl;
.endm
#endif

.macro se_all_new_insn_log
.ifdef BFIN_JTAG_xxxxx
	R1 = R0;
#if SE_ALL_BITS == 32
	R0 = 0x8;
	call __emu_out;
	R0 = R1;
	call __emu_out;
	R0 = R3;
#else
	R0 = 0x4;
	call __emu_out;
	R0 = R1 << 16;
	R0 = R0 | R3;
#endif
	call __emu_out;
.else
	loadsym P0, _next_location;
	P1 = [P0];
	LOAD_PFX[P1++] = R0;
	LOAD_PFX[P1++] = R3;
	[P0] = P1;
.endif
.endm
