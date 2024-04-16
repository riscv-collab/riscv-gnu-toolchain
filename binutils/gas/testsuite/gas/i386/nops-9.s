	.text
default:
	movsbl %al,%esi
	.p2align 4

	.arch .nopopcnt
nopopcnt:
	movsbl %al,%esi
	.p2align 4

	.arch .popcnt
popcnt:
	popcnt %eax,%esi
	.p2align 4

	.arch .nop
nop:
	movsbl %al,%esi
	.p2align 4
