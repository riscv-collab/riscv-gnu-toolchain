# Blackfin testcase for the CEC (handling exceptions from usermode)
# mach: bfin
# sim: --environment operating

	.include "testutils.inc"

	start
.ifndef BFIN_HOST
	// load address of exception handler
	imm32 p0, 0xFFE02000;	/* EVT0 */
	R0 = exception_handler (Z);
	R0.H = exception_handler;
	[ P0 + (4*3) ] = R0;
	//  Jump to User mode and enable exceptions
	R0 = UserCode (Z);
	R0.H = UserCode;
	RETI = R0;
	RTI;

UserCode:
	R4 = 0xec39 (Z);
	R0 = 0xcafe (Z);
	L3 = 0xf41f (Z);
	L3.H = 0x1ce9;
	I3 = 0xfe10 (Z);
	I3.H = 0x20a9;
	B3 = 0x4552 (Z);
	B3.H = 0x15f0;

	// should except - r4 dep
	// R4 = R4 >> 25 || W [ I3 ++ ] = R0.H || R4 = [ I3 ];
.Lskip_start:
	.rep 8
	.byte 0xff
	.endr
	dbg_fail;
.Lskip_end:
	NOP;
	NOP;
	NOP;
	NOP;
	NOP;
	dbg_pass;

exception_handler:
	// just skip over excepting instructions
	R0 = RETX;
	R1.L = .Lskip_start;
	R1.H = .Lskip_start;
	R2.L = .Lskip_end;
	R2.H = .Lskip_end;
	R2 = R2 - R1;
	R0 = R0 + R2;
	RETX = R0;
	RTX;
.endif
