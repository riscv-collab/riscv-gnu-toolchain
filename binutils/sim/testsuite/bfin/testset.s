# Blackfin testcase for playing with TESTSET
# mach: bfin

	.include "testutils.inc"

	start

	.macro _ts val:req
	/* Load value to the external data storage */
	imm32 R0, \val
	[P4] = R0;
	FLUSHINV[P4];
	SSYNC;
	mnop;

	imm32 R1, 0xdeadbeef
	imm32 R2, 0xdeadbeef

	TESTSET (P4);
	SSYNC;
	mnop;
	mnop;

	/* TESTSET will set CC based on low byte == 0 */
	.if \val & 0xff
	if CC jump 1f;
	.else
	if ! CC jump 1f;
	.endif

	/* Regardless of CC, the byte MSB is set to 1 */
	imm32 R1, \val | 0x80

	/* Make sure the result is what we want */
	R2 = [P4];
	FLUSHINV[P4];
	SSYNC;
	mnop;
	CC = R2 == R1;
	if ! CC jump 1f;
	jump 2f;
1:	fail
2:
	.endm
	.macro ts val:req
	_ts \val
	_ts ~(\val)
	.endm

	loadsym P4, _data

	ts 0x00000000
	ts 0x00000011
	ts 0x11111111
	ts 0x11111101
	ts 0x11111110
	ts 0x111111bb
	ts 0xaaaaaa00
	ts 0xabcd2222
	ts 0x000000bb
	ts 0x55555555
	ts 0x5555550a
	ts 0x00100010
	ts 0x00100100
	ts 0x33333000
	ts 0x000000aa

	pass

.data
_data:
.long 0
.size _data, .-_data
