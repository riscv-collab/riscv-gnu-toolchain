# mach: bfin

.include "testutils.inc"
	start


// 0.5
	imm32 r0, 0x40004000;
	imm32 r1, 0x40004000;
	R2 = R0 +|+ R1, R3 = R0 -|- R1 (S , ASR);
	checkreg r2, 0x40004000;
	checkreg r3, 0;

	imm32 r1, 0x10001000;

	R2 = R0 +|+ R1, R3 = R0 -|- R1 (S , ASR);
	checkreg r2, 0x28002800;
	checkreg r3, 0x18001800;

	R0 = R2 +|+ R3, R1 = R2 -|- R3 (S , ASR);
	checkreg r0, 0x20002000;
	checkreg r1, 0x08000800;

	R0 = 1;
	R0 <<= 15;
	R1 = R0 << 16;
	R0 = R0 | R1;
	R1 = R0;
	checkreg r0, 0x80008000;
	checkreg r1, 0x80008000;

	R2 = R0 +|+ R1, R3 = R0 -|- R1 (S , ASR);
	checkreg r2, 0x80008000;
	checkreg r3, 0x0;

	R4 = 0;
	R2 = R2 +|+ R4, R3 = R2 -|- R4 (S , ASR);
	checkreg r2, 0xc000c000;
	checkreg r3, 0xc000c000;

	R2 = R2 +|+ R3, R3 = R2 -|- R3 (S , ASR);
	checkreg r2, 0xc000c000;
	checkreg r3, 0x0;

	R4 = R2 +|+ R2, R5 = R2 -|- R2 (ASL);
	checkreg r4, 0x0
	checkreg r5, 0x0

	R2 = R2 +|+ R2, R3 = R2 -|- R2 (S , ASL);
	checkreg r2, 0x80008000;
	checkreg r3, 0x0;


imm32 r0, 0x50004000;
imm32 r1, 0x40005000;
R2 = R0 +|+ R1, R3 = R0 -|- R1 (S, ASL);
checkreg r2, 0x7fff7fff;
checkreg r3, 0x2000e000;
R4 = R0 +|+ R1, R5 = R0 -|- R1 (ASL);
checkreg r4, 0x20002000
checkreg r5, 0x2000e000

imm32 r0, 0x30001000;
imm32 r1, 0x10003000;
R2 = R0 +|+ R1, R3 = R0 -|- R1 (S, ASL);
checkreg r2, 0x7fff7fff;
checkreg r3, 0x4000c000;
R4 = R0 +|+ R1, R5 = R0 -|- R1 (ASL);
checkreg r4, 0x80008000
checkreg r5, 0x4000c000

imm32 r0, 0x20001fff;
imm32 r1, 0x1fff2000;
R2 = R0 +|+ R1, R3 = R0 -|- R1 (S, ASL);
checkreg r2, 0x7ffe7ffe;
checkreg r3, 0x0002fffe;


	pass
