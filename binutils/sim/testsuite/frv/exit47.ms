# mach: all
# status: 47
# output:

	;; Return with exit code 47.

	.global _start
_start:
	setlos	#47,gr8
	setlos	#1,gr7
	tira	gr0,#0
