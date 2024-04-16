target:
	c.li x1, 31
	c.li x2, 0
	c.lui x1, 1
	c.lui x3, 31
	c.lw x8, (x9)
	c.lw x9, 32(x10)
	lw a0, (sp)
	c.lwsp x1, (x2)
	c.ld x8, (x15)
	c.ld x9, 8(x10)
	ld a0,(sp)
	c.ldsp x1, (sp)
	c.sw x8, (x9)
	c.sw x9, 32(x10)
	sw a0, (sp)
	c.swsp x1, (x2)
	c.sd x8, (x15)
	c.sd x9, 8(x10)
	sd a0, (sp)
	c.sdsp x1, (sp)
	addi x0, x0, 0
	c.nop
	c.nop 31
	c.add x1, x2
	c.addi a1, 31
	c.addi x2, 0
	c.addiw a1, 31
	c.addiw x2, 0
	c.addi4spn x8, x2, 4
	c.addi16sp x2, 32
	c.addw x8, x9
	c.sub x8, x9
	c.subw x8, x9
	c.and x8, x9
	c.andi x8, 31
	c.or x8, x9
	c.xor x8, x9
	c.mv x0, x1
	c.slli x0, 1
	c.slli64 x0
	c.beqz x8, target
	c.bnez x8, target
	c.j target
	c.jr ra
	c.jalr ra
	c.ebreak
