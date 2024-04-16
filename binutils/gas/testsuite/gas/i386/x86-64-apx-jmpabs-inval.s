# Check bytecode of APX_F jmpabs instructions with illegal encode.

	.text
# With 66 prefix
	.insn {rex2} data16 0xa1, $1{:u64}
# With 67 prefix
	.insn {rex2} addr32 0xa1, $1{:u64}
# With F2 prefix
	.insn {rex2} repne 0xa1, $1{:u64}
# With F3 prefix
	.insn {rex2} rep 0xa1, $1{:u64}
# With LOCK prefix
	.insn {rex2} lock 0xa1, $1{:u64}
# REX2.M0 = 0 REX2.W = 1
	.byte 0xd5,0x08,0xa1,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00
