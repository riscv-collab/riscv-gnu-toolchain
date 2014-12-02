/* Facilitate pthread initialization using init_array.  */

#ifdef PREINIT_FUNCTION

#if PREINIT_FUNCTION_WEAK
# error PREINIT_FUNCTION_WEAK is unsupported
#endif

	.section .init_array, "aw"
#ifdef __riscv64
	.dword PREINIT_FUNCTION
#else
	.word PREINIT_FUNCTION
#endif

#endif
