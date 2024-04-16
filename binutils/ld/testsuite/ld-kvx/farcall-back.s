	.global _start
	.global _back
	.global bar1
	.global bar2
	.global bar3

# We will place the section .text at 0x1000.

	.text

	.type _start, @function
_start:
	call	bar1
	;; 
	goto	bar1
	;; 
	call	bar2
	;; 
	goto	bar2
	;; 
	call	bar3
	;; 
	goto	bar3
	;; 
	ret
	;; 
	.space	0x1000
	.type _back, @function
_back:	ret
	;;

# We will place the section .foo at 0x80001000.

	.section .foo, "xa"
	.type bar1, @function
bar1:
	ret
	;; 
	goto	_start
	;; 
	.space 0x1000
	.type bar2, @function
bar2:
	ret
	;; 
	goto	_start
	;; 
	.space 0x1000
	.type bar3, @function
bar3:
	ret
	;; 
	goto	_back
	;; 
