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

need_gcc_external_libraries="no"
AC_CHECK_LIB(gmp, __gmpz_init, ,
        [need_gcc_external_libraries="yes"])
AC_CHECK_LIB(mpfr, mpfr_init, ,
        [need_gcc_external_libraries="yes"])
AC_CHECK_LIB(mpc, mpc_init2, ,
        [need_gcc_external_libraries="yes"])
AS_IF([test x"$need_gcc_external_libraries" != xno],
      [AC_SUBST(NEED_GCC_EXTERNAL_LIBRARIES,true)],
      [AC_SUBST(NEED_GCC_EXTERNAL_LIBRARIES,false)])

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

AC_ARG_WITH(arch,
	[AS_HELP_STRING([--with-arch=IMAFD],
		[Sets the base RISC-V ISA, defaults to IMAFD])],
	AC_SUBST(WITH_ARCH, --with-arch=$withval),
	AC_SUBST(WITH_ARCH,)
	)

AC_ARG_ENABLE(atomic,
	[AS_HELP_STRING([--disable-atomic],
		[disable use of atomic memory instructions in glibc @<:@--enable-atomic@:>@])],
	[],
	[enable_atomic=yes]
	)

AS_IF([test "x$enable_atomic" != xno],
        [],
	[AC_SUBST(atomic_cflags,-mno-atomic)])

AC_ARG_ENABLE(float,
	[AS_HELP_STRING([--disable-float],
		[use software floating point in glibc @<:@--enable-float@:>@])],
	[],
	[enable_float=yes]
	)

AS_IF([test "x$enable_float" != xno],
	    [],
	    [AC_SUBST(float_cflags,-msoft-float)dnl
AC_SUBST(float_asflags,-msoft-float)dnl
AC_SUBST(glibc_configure_flags,--without-fp)])

AC_ARG_ENABLE(multilib,
	[AS_HELP_STRING([--enable-multilib],
		[build multilib glibc (32 and 64 bit) @<:@--disable-multilib@:>@])],
	[],
	[enable_multilib=no]
	)

AS_IF([test "x$enable_multilib" != xno],
        [AC_SUBST(multilib_flags,)],
	[AC_SUBST(multilib_flags,--disable-multilib)])

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([scripts/wrapper/awk/awk], [chmod +x scripts/wrapper/awk])
AC_CONFIG_FILES([scripts/wrapper/sed/sed], [chmod +x scripts/wrapper/sed])

AC_OUTPUT
