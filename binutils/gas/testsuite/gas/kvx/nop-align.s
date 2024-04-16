.text
.align 8
.global f
.type   f, @function
f:
  nop
  nop
  ;;
  addw $r0 = $r1, $r0
  ;;
	.byte 0xFF
	.align 16

  addw $r0 = $r2, $r0
  ret
  ;;
  .size   f, .-f


.align 8
.global g
.type   g, @function
g:
  addw $r0 = $r1, $r0
  addw $r0 = $r2, $r0
  addw $r0 = $r2, $r0
  ;;

  .p2align 6

	addw $r0 = $r2, $r0
	ret
	;;
.size   g, .-g

