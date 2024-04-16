.text
.global _start
.weak foo
.hidden foo

.type _start, @function
_start:
	pcrel $r1 = @pcrel(foo)
	ret
	;;
