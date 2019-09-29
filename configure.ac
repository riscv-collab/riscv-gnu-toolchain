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

AC_ARG_WITH(tune,
	[AS_HELP_STRING([--with-tune=rocket],
		[Set the base RISC-V CPU, defaults to rocket])],
	[],
	[with_tune=rocket]
	)

AS_IF([test "x$with_abi" == xdefault],
	[AS_CASE([$with_arch],
		[*rv64g* | *rv64*d*], [with_abi=lp64d],
		[*rv64*f*], [with_abi=lp64f],
		[*rv64*], [with_abi=lp64],
		[*rv32e*], [with_abi=ilp32e],
		[*rv32g* | *rv32*d*], [with_abi=ilp32d],
		[*rv32*f*], [with_abi=ilp32f],
		[*rv32*], [with_abi=ilp32],
		[AC_MSG_ERROR([Unknown arch])]
	)])

AC_SUBST(WITH_ARCH, --with-arch=$with_arch)
AC_SUBST(WITH_ABI, --with-abi=$with_abi)
AC_SUBST(WITH_TUNE, --with-tune=$with_tune)

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
        [AC_SUBST(glibc_multilib_names,"rv32imac-ilp32 rv32imafdc-ilp32d rv64imac-lp64 rv64imafdc-lp64d")],
        [AC_SUBST(glibc_multilib_names,"$with_arch-$with_abi")])

AS_IF([test "x$enable_multilib" != xno],
        [AC_SUBST(newlib_multilib_names,"rv32i-ilp32 rv32iac-ilp32 rv32im-ilp32 rv32imac-ilp32 rv32imafc-ilp32f rv64imac-lp64 rv64imafdc-lp64d")],
        [AC_SUBST(newlib_multilib_names,"$with_arch-$with_abi")])

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
AC_CONFIG_FILES([scripts/wrapper/awk/awk], [chmod +x scripts/wrapper/awk/awk])
AC_CONFIG_FILES([scripts/wrapper/sed/sed], [chmod +x scripts/wrapper/sed/sed])

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

AC_ARG_ENABLE(gdb,
	[AS_HELP_STRING([--disable-gdb],
		[Don't build GDB, as it's not upstream])],
	[],
	[enable_gdb=yes]
	)

AS_IF([test "x$enable_gdb" != xno],
	[AC_SUBST(enable_gdb, --enable-gdb)],
	[AC_SUBST(enable_gdb, --disable-gdb)])

AC_DEFUN([AX_ARG_WITH_SRC],
	[{m4_pushdef([opt_name], with_$1_src)
	  AC_ARG_WITH($1-src,
		[AC_HELP_STRING([--with-$1-src],
				[Set $1 source path, use builtin source by default])],
		[],
		[opt_name=default]
		)
	  AS_IF([test "x$opt_name" != xdefault],
		[AC_SUBST(opt_name,$opt_name)],
		[AC_SUBST(opt_name,"\$(srcdir)/riscv-$1")])
	  m4_popdef([opt_name])
	}])

AX_ARG_WITH_SRC(gcc)
AX_ARG_WITH_SRC(binutils)
AX_ARG_WITH_SRC(newlib)
AX_ARG_WITH_SRC(glibc)
AX_ARG_WITH_SRC(musl)
AX_ARG_WITH_SRC(gdb)

AC_ARG_WITH(linux-headers-src,
	[AC_HELP_STRING([--with-linux-headers-src],
			[Set linux-headers source path, use builtin source by default])],
	[],
	[with_linux_headers_src=default]
	)

AS_IF([test "x$with_linux_headers_src" != xdefault],
	[AC_SUBST(with_linux_headers_src,$with_linux_headers_src)],
	[AC_SUBST(with_linux_headers_src,"\$(srcdir)/linux-headers/include")])

AC_OUTPUT
