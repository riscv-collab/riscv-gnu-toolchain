	.global _start
	.global bar

# We will place the section .text at 0x1000.

	.text

_start:
	goto bar
	;;
	ret
	;;

# We will place the section .foo at 0x20001000.

	.section .foo, "xa"
	.type bar, @function
bar:
	ret
	;;
