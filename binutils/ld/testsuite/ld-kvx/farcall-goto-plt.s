	.global _start
	.global foo
	.type foo, @function
	.text
_start:
	# ((1 << 26) - 1) << 2
	# PCREL27 relocation out of range to plt stub,
	# we need long branch veneer.
	.skip 268435452, 0
	goto foo
	;;
	ret
	;;
