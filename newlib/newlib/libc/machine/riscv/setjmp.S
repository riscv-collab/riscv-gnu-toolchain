#ifdef __riscv64
# define REG_S    sd
# define REG_L     ld
# define SZREG 8
#else
# define REG_S    sw
# define REG_L     lw
# define SZREG 4
#endif

#ifdef __riscv_flen
/* For ABI uniformity, reserve 8 bytes for floats, even if double-precision
   floating-point is not supported in hardware.  */
# define SZFREG 8
# if __riscv_flen == 32
#  define FREG_L flw
#  define FREG_S fsw
# elif __riscv_flen == 64
#  define FREG_L fld
#  define FREG_S fsd
# else
#  error unsupported FLEN
# endif
#endif

/* int setjmp (jmp_buf);  */
  .globl  setjmp
setjmp:
	REG_S ra,  0*SZREG(a0)
	REG_S s0,  1*SZREG(a0)
	REG_S s1,  2*SZREG(a0)
	REG_S s2,  3*SZREG(a0)
	REG_S s3,  4*SZREG(a0)
	REG_S s4,  5*SZREG(a0)
	REG_S s5,  6*SZREG(a0)
	REG_S s6,  7*SZREG(a0)
	REG_S s7,  8*SZREG(a0)
	REG_S s8,  9*SZREG(a0)
	REG_S s9, 10*SZREG(a0)
	REG_S s10,11*SZREG(a0)
	REG_S s11,12*SZREG(a0)
	REG_S sp, 13*SZREG(a0)

#ifdef __riscv_flen
	frsr a3

	FREG_S fs0, 16*SZREG+ 0*SZFREG(a0)
	FREG_S fs1, 16*SZREG+ 1*SZFREG(a0)
	FREG_S fs2, 16*SZREG+ 2*SZFREG(a0)
	FREG_S fs3, 16*SZREG+ 3*SZFREG(a0)
	FREG_S fs4, 16*SZREG+ 4*SZFREG(a0)
	FREG_S fs5, 16*SZREG+ 5*SZFREG(a0)
	FREG_S fs6, 16*SZREG+ 6*SZFREG(a0)
	FREG_S fs7, 16*SZREG+ 7*SZFREG(a0)
	FREG_S fs8, 16*SZREG+ 8*SZFREG(a0)
	FREG_S fs9, 16*SZREG+ 9*SZFREG(a0)
	FREG_S fs10,16*SZREG+10*SZFREG(a0)
	FREG_S fs11,16*SZREG+11*SZFREG(a0)

	REG_S a3, 15*SZREG(a0)
#endif

	li    a0, 0
	ret

/* volatile void longjmp (jmp_buf, int);  */
  .globl  longjmp
longjmp:
	REG_L ra,  0*SZREG(a0)
	REG_L s0,  1*SZREG(a0)
	REG_L s1,  2*SZREG(a0)
	REG_L s2,  3*SZREG(a0)
	REG_L s3,  4*SZREG(a0)
	REG_L s4,  5*SZREG(a0)
	REG_L s5,  6*SZREG(a0)
	REG_L s6,  7*SZREG(a0)
	REG_L s7,  8*SZREG(a0)
	REG_L s8,  9*SZREG(a0)
	REG_L s9, 10*SZREG(a0)
	REG_L s10,11*SZREG(a0)
	REG_L s11,12*SZREG(a0)
	REG_L sp, 13*SZREG(a0)

#ifdef __riscv_flen
	REG_L a3, 15*SZREG(a0)

	FREG_L fs0, 16*SZREG+ 0*SZFREG(a0)
	FREG_L fs1, 16*SZREG+ 1*SZFREG(a0)
	FREG_L fs2, 16*SZREG+ 2*SZFREG(a0)
	FREG_L fs3, 16*SZREG+ 3*SZFREG(a0)
	FREG_L fs4, 16*SZREG+ 4*SZFREG(a0)
	FREG_L fs5, 16*SZREG+ 5*SZFREG(a0)
	FREG_L fs6, 16*SZREG+ 6*SZFREG(a0)
	FREG_L fs7, 16*SZREG+ 7*SZFREG(a0)
	FREG_L fs8, 16*SZREG+ 8*SZFREG(a0)
	FREG_L fs9, 16*SZREG+ 9*SZFREG(a0)
	FREG_L fs10,16*SZREG+10*SZFREG(a0)
	FREG_L fs11,16*SZREG+11*SZFREG(a0)

	fssr a3
#endif

	seqz a0, a1
	add  a0, a0, a1   # a0 = (a1 == 0) ? 1 : a1
	ret
