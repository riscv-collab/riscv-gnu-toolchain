        .section	.text.start
        
        # Set stack pointer. The main thread uses a statically-allocated stack
        # in the .data section.

        la	sp, _main_guard

        # Initalize the trap vector (mtvec CSR). The trap handler is defined
        # in trap.s.

        la      t0, _trap_entry
        csrw    mtvec, t0

        # Jump to main. Arrange it so that if main() returns (it shouldn't),
        # it jumps to halt_failure().

        la      ra, halt_failure
        j       main

        .section	.data.main_stack
        .align		16
        
        .equ		MAIN_STACK_SIZE, 1024
        .equ		MAIN_GUARD_SIZE, 256

        .global		_main_stack
        .type		_main_stack, @object
        .size		_main_stack, MAIN_STACK_SIZE

        .global		_main_guard
        .type		_main_guard, @object
        .size		_main_guard, MAIN_GUARD_SIZE

_main_stack:
        .fill	MAIN_STACK_SIZE, 1, 0xA5

_main_guard:
        .fill	MAIN_GUARD_SIZE, 1, 0x5A
        .end

