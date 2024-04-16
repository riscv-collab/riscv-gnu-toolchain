	.global _start
	.global bar_gsym

# We will place the section .text at 0x1000.

	.text

_start:
# for long jump (goto) to global symbol, we shouldn't insert veneer
# as the veneer will clobber r16/r17 which is caller saved, gcc only
# reserve them for function call relocation (call).
	goto bar_gsym
	;;
	# ((1 << 26) - 1) << 2
	.skip 268435452, 0
bar_gsym:
	nop
	;;
	ret
	;;
