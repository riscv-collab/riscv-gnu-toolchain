AC_INIT(riscv-toolchain, 1.0)

AC_PROG_CC
AC_PROG_FGREP

AC_PATH_PROG([BASH], [bash],
	[AC_MSG_ERROR([bash not found])])

AC_PATH_PROGS_FEATURE_CHECK([GAWK], [gawk awk],
	[AS_IF([$ac_path_GAWK --version 2>&1 | $FGREP GNU >/dev/null],
		[ac_cv_path_GAWK=$ac_path_GAWK ac_path_GAWK_found=:])],
	[AC_MSG_ERROR([GNU Awk not found])])
AC_SUBST([GAWK], [$ac_cv_path_GAWK])

AC_PATH_PROGS_FEATURE_CHECK([GSED], [gsed sed],
	[AS_IF([$ac_path_GSED --version 2>&1 | $FGREP GNU >/dev/null],
		[ac_cv_path_GSED=$ac_path_GSED ac_path_GSED_found=:])],
	[AC_MSG_ERROR([GNU sed not found])])
AC_SUBST([GSED], [$ac_cv_path_GSED])

AC_CHECK_LIB(gmp, __gmpz_init, ,
	[AC_MSG_ERROR([GNU MP not found; see https://gmplib.org/])])

AC_CHECK_LIB(mpfr, mpfr_init, ,
	[AC_MSG_ERROR([MPFR not found; see http://www.mpfr.org/])])

AC_CHECK_LIB(mpc, mpc_init2, ,
	[AC_MSG_ERROR([MPC not found; see http://www.multiprecision.org/])])

AC_PATH_PROG([CURL], [curl], [no])
AC_PATH_PROG([WGET], [wget], [no])
AC_PATH_PROG([FTP], [ftp], [no])
AS_IF([test x"$CURL" != xno], [FETCHER="$CURL -o - --ftp-pasv"],
	[test x"$WGET" != xno], [FETCHER="$WGET -O -"],
	[test x"$FTP" != xno], [FETCHER="$FTP -o - -p"],
	[AC_MSG_ERROR([no file transfer utility found])])
AC_SUBST(FETCHER)

AC_ARG_ENABLE(linux,
        [AS_HELP_STRING([--enable-linux],
		[set linux as the default make target @<:@--disable-linux@:>@])],
        [],
        [enable_linux=no]
        )

AS_IF([test "x$enable_linux" != xno],
	[AC_SUBST(default_target, linux)],
	[AC_SUBST(default_target, newlib)])

AC_ARG_WITH(xlen,
	[AS_HELP_STRING([--with-xlen=XLEN],
		[Set XLEN, the X-register bit width (default is 64)])],
	AC_SUBST(XLEN, $withval),
	AC_SUBST(XLEN, 64)
	)

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([scripts/wrapper/awk], [chmod +x scripts/wrapper/awk])
AC_CONFIG_FILES([scripts/wrapper/sed], [chmod +x scripts/wrapper/sed])

AC_OUTPUT
