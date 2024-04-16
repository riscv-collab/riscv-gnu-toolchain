	.data
data:

	.macro const, sfx
	.byte 0\sfx
	.byte 00\sfx
	.byte 0x0\sfx
	.byte 1\sfx
	.endm

	const u
	const l
	const ul
	const lu
	const ll
	const llu
	const ull
	const lll
	const ulu
	const lul
