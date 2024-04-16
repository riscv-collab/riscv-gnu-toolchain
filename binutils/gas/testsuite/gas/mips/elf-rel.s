	.text
	.align 15
l0:
l2	= l0+49150

	.set	noat
	.set	noreorder
	lui	$at,%hi(l1)
	lui	$at,%hi(l1+4)
	lui	$at,%hi(l1+0x10000)
	lui	$at,%hi(l1+0x10004)
	lui	$at,%hi(l0-4)
	lui	$at,%hi(l1+0x8000)
l1:		
	addiu	$at,$at,%lo(l1)
	addiu	$at,$at,%lo(l1+0x10004)
	addiu	$at,$at,%lo(l1+0x10000)
	addiu	$at,$at,%lo(l1+4)
	addiu	$at,$at,%lo(l1+0x8000)
	addiu	$at,$at,%lo(l0-4)

	lui	$at,%hi(l2)
	lui	$at,%hi(l2+4)
	lui	$at,%hi(l2+0x10000)
	lui	$at,%hi(l2+0x10004)
	lui	$at,%hi(l2-4)
	lui	$at,%hi(l2+0x8000)
	addiu	$at,$at,%lo(l2)
	addiu	$at,$at,%lo(l2+4)
	addiu	$at,$at,%lo(l2+0x10000)
	addiu	$at,$at,%lo(l2+0x10004)
	addiu	$at,$at,%lo(l2+0x8000)
	addiu	$at,$at,%lo(l2-4)

	lui	$at,%hi((l2))
	lui	$at,%hi(((l2+4)))
	lui	$at,%hi((((l2+0x10000))))
	lui	$at,%hi(((((l2+0x10004)))))
	lui	$at,%hi((((((l2-4))))))
	lui	$at,%hi(((((((l2+0x8000)))))))
	addiu	$at,$at,%lo((l2))
	addiu	$at,$at,%lo(((l2+4)))
	addiu	$at,$at,%lo((((l2+0x10000))))
	addiu	$at,$at,%lo(((((l2+0x10004)))))
	addiu	$at,$at,%lo((((((l2+0x8000))))))
	addiu	$at,$at,%lo(((((((l2-4)))))))
