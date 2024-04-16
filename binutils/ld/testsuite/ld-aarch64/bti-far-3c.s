	.text
	.hidden	a_func
	.hidden	b_func
	.hidden	c_func

.zero	0x01000000

	.global	c_func
	.type	c_func, %function
c_func:
	b	a_func
	b	b_func
	b	extern_func

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
