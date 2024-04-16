	.text
	.hidden	a_func
	.hidden	b_func
	.hidden	c_func

	.global	a_func
	.type	a_func, %function
a_func:
	b	b_func
	b	extern_func

.zero	0x07000000

	.section	.note.gnu.property,"a"
	.align	3
	.word	4
	.word	16
	.word	5
	.string	"GNU"
	.word	3221225472
	.word	4
	.word	1
	.align	3
