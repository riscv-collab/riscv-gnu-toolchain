# Test maximum encoding range of call

	.global _start
	.global bar

# We will place the section .text at 0x0000.

	.text

_start:
	goto bar
	;;
	ret
	;;

# We will place the section .foo at 0x10000000

	.section .foo, "xa"
	.type bar, @function
bar:
	ret
	;;
