# output(): Hello, world.\n
# mach(): all

# This is a test for movw & movt instructions.
# It emits hello world if movw & movt works appropriately.

	.macro invalid
# This is "undefined" but it's not properly decoded yet.
	.word 0x07ffffff
# This is stc which isn't recognized yet.
	stc 0,cr0,[r0]
	.endm

	.global _start
_start:
# Run some simple insns to confirm the engine is at least working.
	nop

# Skip over output text.

	bl skip_output

hello_text:
	.asciz "Hello, world.\n"

	.p2align 2
skip_output:
	movw r4, #:lower16:hello_text
	movt r4, #:upper16:hello_text

output_next:
# Output a character
	mov r0,#3
	mov r1,r4
	swi #0x123456

# Load next character, see if done.
	add r4,r4,#1
	sub r3,r3,r3
	ldrb r5,[r4,r3]
	teq r5,#0
	bne output_next

done:
	mov r0,#0x18
	ldr r1,exit_code
	swi #0x123456

# If that fails, try to die with an invalid insn.
	invalid

exit_code:
	.word 0x20026
