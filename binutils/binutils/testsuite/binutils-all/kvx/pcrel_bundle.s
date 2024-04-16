foo:
	pcrel $r0 = @pcrel(.table)
	;;
	call bar
	pcrel $r0 = @pcrel(.table)
	;;
	call bar
	ld $r0 = 0[$r0]
	pcrel $r0 = @pcrel(.table)
	;;
	nop
	;;
	nop
	;;
.table:
	nop
	;;
	nop
	;; 
bar:
	nop
	;;
	

	
