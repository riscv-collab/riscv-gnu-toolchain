AC_INIT(riscv-toolchain, 1.0)

AC_PROG_CC
AC_PROG_FGREP
AC_PROG_GREP

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
AS_IF([test x"$CURL" != xno], [FETCHER="$CURL -L -o - --ftp-pasv --retry 10"],
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

AC_ARG_ENABLE(atomic,
	[AS_HELP_STRING([--disable-atomic],
		[disable use of atomic memory instructions in glibc @<:@--enable-atomic@:>@])],
	[],
	[enable_atomic=yes]
	)

AC_ARG_WITH(arch,
	[AS_HELP_STRING([--with-arch=rv64imafdc],
		[Sets the base RISC-V ISA, defaults to rv64imafdc])],
	[],
	[with_arch=rv64imafdc]
	)

AC_ARG_WITH(abi,
	[AS_HELP_STRING([--with-abi=lp64d],
		[Sets the base RISC-V ABI, defaults to lp64d])],
	[],
	[with_abi=default]
	)

AS_IF([test "x$with_abi" == xdefault],
	[AS_CASE([$with_arch],
		[*rv64g* | *rv64*d*], [with_abi=lp64d],
		[*rv64*f*], [with_abi=lp64f],
		[*rv64*], [with_abi=lp64],
		[*rv32g* | *rv32*d*], [with_abi=ilp32d],
		[*rv32*f*], [with_abi=ilp32f],
		[*rv32*], [with_abi=ilp32],
		[AC_MSG_ERROR([Unknown arch])]
	)])

AC_SUBST(WITH_ARCH, --with-arch=$with_arch)
AC_SUBST(WITH_ABI, --with-abi=$with_abi)

AC_ARG_ENABLE(multilib,
	[AS_HELP_STRING([--enable-multilib],
		[build both RV32 and RV64 runtime libraries @<:@--disable-multilib@:>@])],
	[],
	[enable_multilib=no]
	)

AS_IF([test "x$enable_multilib" != xno],
        [AC_SUBST(multilib_flags,--enable-multilib)],
	[AC_SUBST(multilib_flags,--disable-multilib)])

AS_IF([test "x$enable_multilib" != xno],
        [AC_SUBST(multilib_names,"rv32imac-ilp32 rv32imafdc-ilp32d rv64imac-lp64 rv64imafdc-lp64d")],
	[])

AC_ARG_ENABLE(gcc-checking,
        [AS_HELP_STRING([--enable-gcc-checking],
		[Enable gcc internal checking, it will make gcc very slow, only enable it when developing gcc @<:@--disable-gcc-checking@:>@])],
        [],
        [enable_gcc_checking=yes]
        )
AS_IF([test "x$enable_gcc_checking" != xno],
	[AC_SUBST(gcc_checking, --enable-checking=yes)],
	[AC_SUBST(gcc_checking, --enable-checking=release)])

AC_ARG_WITH(cmodel,
	[AS_HELP_STRING([--with-cmodel],
		[Select the code model to use when building libc and libgcc @<:@--with-cmodel=medlow@:>@])],
	[],
	[enable_gcc_checking=yes]
	)
AS_IF([test "x$with_cmodel" != x],
	[AC_SUBST(cmodel, -mcmodel=$with_cmodel)],
	[AC_SUBST(cmodel, -mcmodel=medlow)])

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([scripts/wrapper/awk/awk], [chmod +x scripts/wrapper/awk])
AC_CONFIG_FILES([scripts/wrapper/sed/sed], [chmod +x scripts/wrapper/sed])

AC_ARG_WITH(host,
	[AS_HELP_STRING([--with-host=x86_64-w64-mingw32],
			[Sets the host for the tools, you probably want nothing])],
	[],
	[with_host=default]
	)

AS_IF([test "x$with_host" != xdefault],
	[AC_SUBST(configure_host,--host=$with_host)],
	[AC_SUBST(configure_host,"")])

AC_ARG_WITH(system-zlib,
	[AS_HELP_STRING([--without-system-zlib],
		[use the builtin copy of zlib from GCC])],
	[],
	[with_system_zlib=yes]
	)

AS_IF([test "x$with_system_zlib" != xno],
	[AC_SUBST(with_system_zlib,--with-system-zlib)],
	[AC_SUBST(with_system_zlib,--without-system-zlib)])

AC_ARG_WITH(guile,
	[AC_HELP_STRING([--with-guile],
			[Set which guile to use, if any])],
	[],
	[with_guile=default]
	)

AS_IF([test "x$with_guile" != xdefault],
	[AC_SUBST(with_guile,--with-guile=$with_guile)],
	[AC_SUBST(with_guile,"")])

AC_OUTPUT
