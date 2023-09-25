## Process this file with autoconf 2.69. Other versions may not work.
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

AC_ARG_ENABLE(debug_info,
        [AS_HELP_STRING([--enable-debug-info],
		[build glibc/musl/newlibc/libgcc with debug information])],
        [enable_debug_info=yes],
        [enable_debug_info=no]
        )

AS_IF([test "x$enable_debug_info" != xyes],
        [disable_debug_info=yes],
        [disable_debug_info=no]
        )

AS_IF([test "x$enable_debug_info" != xyes],
	[AC_SUBST(debug_info, "")],
	[AC_SUBST(debug_info, "-g")])

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

AC_ARG_WITH(isa-spec,
	[AS_HELP_STRING([--with-isa-spec=20191213],
		[Set the default ISA spec version, default to 20191213, available options: 2.2, 20190608, 20191213])],
	[],
	[with_isa_spec=20191213]
	)

AC_ARG_WITH(sim,
	[AS_HELP_STRING([--with-sim=qemu],
		[Sets the base RISC-V Simulator, defaults to qemu])],
	[],
	[with_sim=qemu]
	)

AS_IF([test "x$with_abi" = xdefault],
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
AC_SUBST(WITH_ISA_SPEC, --with-isa-spec=$with_isa_spec)
AC_SUBST(WITH_SIM, $with_sim)

AC_ARG_ENABLE(multilib,
	[AS_HELP_STRING([--enable-multilib],
		[build both RV32 and RV64 runtime libraries (only RV64 for musl libc) @<:@--disable-multilib@:>@])],
	[],
	[enable_multilib=no]
	)

AC_ARG_WITH(multilib-generator,
	[AS_HELP_STRING([--with-multilib-generator],
		[Multi-libs configuration string, only supported for bare-metal/elf toolchain, this option implied --enable-multilib])],
	[],
	[with_multilib_generator=no]
	)

AC_ARG_WITH(extra-multilib-test,
	[AS_HELP_STRING([--with-extra-multilib-test],
		[Adding extra multi-libs configuration for testing, this can be use even --disable-multilib, but be aware user need to make sure those extra multilib set can work/compatible with existing multi-libs, format: <arch>-<abi>;<arch>-<abi>  e.g: --with-extra-multilib-test="rv64gcv-lp64;rv64gcv_zba-lp64"])],
	[],
	[with_extra_multilib_test=no]
	)

AS_IF([test "x$with_multilib_generator" != xno],
        [AC_SUBST(multilib_gen,"$with_multilib_generator")],
        [AC_SUBST(multilib_gen,"")])

AS_IF([test "x$with_extra_multilib_test" != xno],
        [AC_SUBST(extra_multilib_test,"$with_extra_multilib_test")],
        [AC_SUBST(extra_multilib_test,"")])

AS_IF([test "x$enable_multilib" != xno || test "x$with_multilib_generator" != xno],
        [AC_SUBST(multilib_flags,--enable-multilib)],
	[AC_SUBST(multilib_flags,--disable-multilib)])

AS_IF([test "x$enable_multilib" != xno],
        [AC_SUBST(glibc_multilib_names,"rv32imac-ilp32 rv32imafdc-ilp32d rv64imac-lp64 rv64imafdc-lp64d")],
        [AC_SUBST(glibc_multilib_names,"$with_arch-$with_abi")])

AS_IF([test "x$enable_multilib" != xno],
        [AC_SUBST(newlib_multilib_names,"rv32i-ilp32 rv32iac-ilp32 rv32im-ilp32 rv32imac-ilp32 rv32imafc-ilp32f rv64imac-lp64 rv64imafdc-lp64d")],
        [AC_SUBST(newlib_multilib_names,"$with_arch-$with_abi")])

AS_IF([test "x$enable_multilib" != xno],
        [AC_SUBST(musl_multilib_names,"rv64imac-lp64 rv64imafdc-lp64d")],
        [AC_SUBST(musl_multilib_names,"$with_arch-$with_abi")])

AC_ARG_ENABLE(gcc-checking,
        [AS_HELP_STRING([--enable-gcc-checking],
		[Enable gcc internal checking, it will make gcc very slow, only enable it when developing gcc @<:@--disable-gcc-checking@:>@])],
        [],
        []
        )
AS_IF([test "x$enable_gcc_checking" != x],
	[AC_SUBST(gcc_checking, --enable-checking=$enable_gcc_checking)],
	[AC_SUBST(gcc_checking, "")])

AC_ARG_WITH(cmodel,
	[AS_HELP_STRING([--with-cmodel],
		[Select the code model to use when building libc and libgcc @<:@--with-cmodel=medlow@:>@])],
	[],
	[]
	)
AS_IF([test "x$with_cmodel" != x],
	[AC_SUBST(cmodel, -mcmodel=$with_cmodel)],
	[AC_SUBST(cmodel, -mcmodel=medlow)])

AC_ARG_WITH(target_cflags,
	[AS_HELP_STRING([--with-target-cflags],
		[Add extra target flags for C for library code])],
	[],
	[]
	)
AC_SUBST(target_cflags, $with_target_cflags)
AC_ARG_WITH(target_cxxflags,
	[AS_HELP_STRING([--with-target-cxxflags],
		[Add extra target flags for C++ for library code])],
	[],
	[]
	)
AC_SUBST(target_cxxflags, $with_target_cxxflags)

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

AC_ARG_ENABLE(llvm,
	[AS_HELP_STRING([--enable-llvm],
		[Build LLVM (clang)])],
	[enable_llvm=yes],
	[]
	)

AS_IF([test "x$enable_llvm" != xyes],
	[AC_SUBST(enable_llvm, --disable-llvm)],
	[AC_SUBST(enable_llvm, --enable-llvm)])

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
		[AC_SUBST(opt_name,"\$(srcdir)/$2")])
	  m4_popdef([opt_name])
	}])

AX_ARG_WITH_SRC(gcc, gcc)
AX_ARG_WITH_SRC(binutils, binutils)
AX_ARG_WITH_SRC(newlib, newlib)
AX_ARG_WITH_SRC(glibc, glibc)
AX_ARG_WITH_SRC(musl, musl)
AX_ARG_WITH_SRC(gdb, gdb)
AX_ARG_WITH_SRC(qemu, qemu)
AX_ARG_WITH_SRC(spike, spike)
AX_ARG_WITH_SRC(pk, pk)
AX_ARG_WITH_SRC(llvm, llvm)
AX_ARG_WITH_SRC(dejagnu, dejagnu)

AC_ARG_WITH(linux-headers-src,
	[AC_HELP_STRING([--with-linux-headers-src],
			[Set linux-headers source path, use builtin source by default])],
	[],
	[with_linux_headers_src=default]
	)

AS_IF([test "x$with_linux_headers_src" != xdefault],
	[AC_SUBST(with_linux_headers_src,$with_linux_headers_src)],
	[AC_SUBST(with_linux_headers_src,"\$(srcdir)/linux-headers/include")])

AC_ARG_ENABLE(libsanitizer,
	[AS_HELP_STRING([--enable-libsanitizer],
		[Build libsanitizer, which only supports rv64])],
	[],
	[enable_libsanitizer=no]
	)

AS_IF([test "x$enable_libsanitizer" != xno],
	[AC_SUBST(enable_libsanitizer, --enable-libsanitizer)],
	[AC_SUBST(enable_libsanitizer, --disable-libsanitizer)])

AC_ARG_ENABLE(qemu_system,
	[AS_HELP_STRING([--enable-qemu-system],
		[Build qemu with system-mode emulation])],
	[],
	[enable_qemu_system=no]
	)

AS_IF([test "x$enable_qemu_system" != xno],
	[AC_SUBST(qemu_targets, [riscv64-linux-user,riscv32-linux-user,riscv64-softmmu,riscv32-softmmu])],
	[AC_SUBST(qemu_targets, [riscv64-linux-user,riscv32-linux-user])])

AC_OUTPUT
