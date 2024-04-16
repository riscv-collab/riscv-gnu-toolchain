// assert that we can issue a software exception
// and that the expt number is passed correctly through
// SEQSTAT.
# mach: bfin
# sim: --environment operating

	.include "testutils.inc"

	start
.ifndef BFIN_HOST
	imm32 p0, 0xFFE02000;	/* EVT0 */
	P1 = re (Z);		// load a pointer to ihandler interrupt 1
	P1.H = re;
	[ P0 + (4*3) ] = P1;

	R0 = -1;	/* unmask all interrupts */
	imm32 p1, 0xFFE02104;
	[P1] = R0;

	R0 = start_uspace (Z);
	R0.H = start_uspace;
	RETI = R0;
	RTI;
start_uspace:
	EXCPT 10;

	DBGA ( R1.L , 0x1238 );

	dbg_pass;

	// ihandler
re:
	R0 = SEQSTAT;
	R0 <<= (32-6);
	R0 >>= (32-6);
	R2 = 0x20;
	CC = R0 < R2;
	IF !CC JUMP _error;
	DBGA ( R0.L , 0xa );
	R1 = 0x1234 (X);
	R1 += 1;
	R1 += 1;
	R1 += 1;
	R1 += 1;
	RTX;

_error:
	DBGA ( R0.L , EXCPT_PROTVIOL );
	dbg_fail;

.endif
