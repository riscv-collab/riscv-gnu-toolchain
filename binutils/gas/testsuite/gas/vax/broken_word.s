	.text

	.globl	printf

	.globl asmfn
	.type	asmfn, @function
asmfn:
	.word 0
	subl2 $4,%sp
	movl 4(%ap), %r0

	casel	%r0, $1, $(3 - 1)
	.type	.casetable, @object
.casetable:
	.word	case1 - .casetable
	.word	case2 - .casetable
	.word	case3 - .casetable
# define a label here for disassembly of magically added branch and jump
.casetableend = .casetable + 6
	.type	.casetableend, @notype

casedefault:
	pushal	msg_default
	calls	$1, printf
asmret:
	ret

# Case1 is close by, within the range of a word offset
case1:
	pushal	msg_case1
	calls	$1, printf
	jmp	asmret
	.skip	32600

# Case2 is still within the range of a signed word offset
case2:
	pushal	msg_case2
	calls	$1, printf
	jmp	asmret
	.skip	1024

# Case3 is now no longer within the range of a signed word offset
case3:
	pushal	msg_case3
	calls	$1, printf
	jmp	asmret


	.section	.rodata
msg_case1:
	.string	"Case 1\n"
msg_case2:
	.string	"Case 2\n"
msg_case3:
	.string	"Case 3\n"
msg_default:
	.string	"Default case\n"
