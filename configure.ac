AC_INIT(riscv-toolchain, 1.0)

AC_PROG_CC
AC_PROG_AWK

AC_CHECK_LIB(gmp, __gmpz_mul_si, , [AC_MSG_ERROR(
     [GNU MP 3.1 or greater not found.
     See http://gmplib.org])])

AC_CHECK_LIB(mpfr, mpfr_add, , [AC_MSG_ERROR(
     [MPFR 3.1 or greater not found.
     See http://www.mpfr.org])])

AC_CHECK_LIB(mpc, mpc_add, , [AC_MSG_ERROR(
     [MPC 0.8.1 or greater not found.
     See http://www.multiprecision.org])])

AC_ARG_ENABLE(linux,
        [Toolchain targets Linux],
        AC_SUBST(default_target, linux),
        AC_SUBST(default_target, newlib)
        )

AC_OUTPUT(
    Makefile
)
