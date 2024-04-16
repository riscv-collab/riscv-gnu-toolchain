# Blackfin testcase for circular buffers
# mach: bfin

	.include "testutils.inc"

	.macro daginit i:req, b:req, l:req, m:req
	imm32 I0, \i
	imm32 B0, \b
	imm32 L0, \l
	imm32 M0, \m
	.endm
	.macro dagcheck newi:req
	DBGA ( I0.L, \newi & 0xFFFF );
	DBGA ( I0.H, \newi >> 16 );
	.endm

	.macro dagadd i:req, b:req, l:req, m:req, newi:req
	daginit \i, \b, \l, \m
	I0 += M0;
	dagcheck \newi
	.endm

	.macro dagsub i:req, b:req, l:req, m:req, newi:req
	daginit \i, \b, \l, \m
	I0 -= M0;
	dagcheck \newi
	.endm

	.macro dag i:req, b:req, l:req, m:req, addi:req, subi:req
	daginit \i, \b, \l, \m
	I0 += M0;
	dagcheck \addi
	imm32 I0, \i
	I0 -= M0;
	dagcheck \subi
	.endm

	start

	init_l_regs 0
	init_i_regs 0
	init_b_regs 0
	init_m_regs 0

_zero_len:
	dag 0, 0, 0, 0, 0, 0
	dag 100, 0, 0, 0, 100, 100
	dag 100, 0, 0, 11, 111, 89
	dag 100, 0xaa00ff00, 0, 0, 100, 100
	dag 100, 0xaa00ff00, 0, 11, 111, 89

_zero_base:
	dag 0, 0, 100, 10, 10, 90
	dag 50, 0, 100, 10, 60, 40
	dag 99, 0, 100, 10, 9, 89
	dag 50, 0, 100, 50, 0, 0
	dag 50, 0, 100, 100, 50, 50
	dag 50, 0, 100, 200, 150, -50
	dag 50, 0, 100, 2100, 2050, -1950
	dag 1000, 0, 100, 0, 900, 1000
	dag 1000, 0, 1000, 0, 0, 1000

	dag 0xffff1000, 0, 0x1000, 0, 0xffff0000, 0xffff1000
	dag 0xaaaa1000, 0, 0xaaa1000, 0, 0xa0000000, 0xaaaa1000
	dag 0xaaaa1000, 0, 0xaaa1000, 0x1000, 0xa0001000, 0xaaaa0000
	dag 0xffff1000, 0, 0xffff0000, 0xffffff, 0x1000fff, 0xfeff1001

_positive_base:
	dag 0, 100, 100, 10, 10, 90
	dag 90, 100, 100, 10, 100, 180
	dag 90, 100, 100, 2100, 2090, -1910
	dag 100, 100, 100, 100, 100, 100
	dag 0xfffff000, 0xffffff00, 0x10, 0xffff, 0xefef, 0xfffef011

_large_base_len:
	dag 0, 0xffffff00, 0xffffff00, 0x00000100, 0x00000200, 0xfffffe00
	dag 0, 0xaaaaaaaa, 0xbbbbbbbb, 0xcccccccc, 0x88888887, 0x77777779
	dag 0, 0xaaaaaaaa, 0xbbbbbbbb, 0x4ccccccc, 0x91111111, 0x6eeeeeef
	dag 0, 0xaaaaaaaa, 0xbbbbbbbb, 0x00000000, 0x44444445, 0xbbbbbbbb
	dag 0, 0xdddddddd, 0x7bbbbbbb, 0xcccccccc, 0xcccccccc, 0xb7777779
	dag 0, 0xbbbbbbbb, 0x7bbbbbbb, 0x4ccccccc, 0x4ccccccc, 0xb3333334
	dag 0, 0xbbbbbbbb, 0x7bbbbbbb, 0x00000000, 0x84444445, 0x7bbbbbbb

	pass
