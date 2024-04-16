# Test for saturation behavior with fract multiplication
# mach: bfin

.include "testutils.inc"

	start

	dmm32 A0.w, 0x45c1969f;
	dmm32 A0.x, 0x00000000;
	R4 = A0 (IU);
	checkreg R4, 0x45c1969f;

	pass
