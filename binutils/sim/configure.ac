dnl Process this file with autoconf to produce a configure script.
dnl NB: The version here is not used.  If gdb ever changes from generating its
dnl version at build time to autoconf time (like bfd et al do), we can switch.
AC_INIT([sim], [0],
  [https://sourceware.org/bugzilla/enter_bug.cgi?product=gdb&component=sim],
  [], [https://sourceware.org/gdb/wiki/Sim/])

dnl Probably should unify PKGVERSION with PACKAGE_* settings from AC_INIT.
ACX_PKGVERSION([SIM])
AC_DEFINE_UNQUOTED([PKGVERSION], ["$PKGVERSION"], [Additional package description])
dnl PACKAGE_BUGREPORT is provided by AC_INIT.
ACX_BUGURL([$PACKAGE_BUGREPORT])
AC_DEFINE_UNQUOTED([REPORT_BUGS_TO], ["$REPORT_BUGS_TO"], [Bug reporting address])

AC_CONFIG_HEADERS([config.h])

dnl Set the 'development' global before loading the SIM macros.
. $srcdir/../bfd/development.sh

SIM_AC_TOOLCHAIN
SIM_AC_PLATFORM

m4_include([../gdbsupport/libiberty.m4])
libiberty_INIT

AM_MAINTAINER_MODE
AM_INIT_AUTOMAKE
AM_SILENT_RULES([yes])

# If a cpu ever has more than one simulator to choose from, use
# --enable-sim=... to choose.
AC_ARG_ENABLE(sim,
[AS_HELP_STRING([--enable-sim], [Enable the GNU simulator])],
[case "${enableval}" in
yes | no) ;;
*)	AC_MSG_ERROR(bad value ${enableval} given for --enable-sim option) ;;
esac])

AC_ARG_ENABLE([example-sims],
	      [AC_HELP_STRING([--enable-example-sims],
			      [enable example GNU simulators])])

AC_ARG_ENABLE(targets,
[  --enable-targets        alternative target configurations],
[case "${enableval}" in
  yes | "") AC_MSG_ERROR(enable-targets option must specify target names or 'all')
	    ;;
  no)       enable_targets= ;;
  *)        enable_targets=$enableval ;;
esac])

dnl Used to keep track of which target (if any) is the default one.  This is
dnl used when installing files to see if they need to be suffixed.
SIM_PRIMARY_TARGET=
AC_SUBST(SIM_PRIMARY_TARGET)

dnl List of enabled arch backends.
SIM_ENABLED_ARCHES=
AC_SUBST(SIM_ENABLED_ARCHES)

ENABLE_SIM=no
dnl Build a particular arch subdir.
dnl arg[1] is the arch subdir name.
m4_define([SIM_BUILD_TARGET], [dnl
  ENABLE_SIM=yes
  AS_VAR_APPEND([SIM_ENABLED_ARCHES], [" $1"])
  AC_CONFIG_FILES($1/.gdbinit:common/gdbinit.in)
])
dnl Enable a particular arch subdir.
dnl arg[1] is the matching target triple.
dnl arg[2] is the arch subdir name.
m4_define([SIM_TARGET], [dnl
  sim_enable_arch_$2=false
  case "${targ}" in
    all|$1)
      if test "${targ}" = "${target}"; then
        SIM_PRIMARY_TARGET=$2
      fi
      SIM_BUILD_TARGET($2)
      sim_enable_arch_$2=true
      ;;
  esac
  SIM_AC_TOOLCHAIN_FOR_TARGET($2)
  AM_CONDITIONAL([SIM_ENABLE_ARCH_$2], [${sim_enable_arch_$2}])
])

dnl WHEN ADDING ENTRIES TO THIS MATRIX:
dnl Make sure that the left side always has two dashes.  Otherwise you can get
dnl spurious matches.  Even for unambiguous cases, do this as a convention, else
dnl the table becomes a real mess to understand and maintain.
dnl
dnl NB: Target matching is aligned with gdb/configure.tgt.  Changes must be kept
dnl in sync with that file.
if test "${enable_sim}" != no; then
  for targ in `echo $target $enable_targets | sed 's/,/ /g'`
  do
    m4_map([SIM_TARGET], [
      [[aarch64*-*-*], [aarch64]],
      [[arm*-*-*], [arm]],
      [[avr*-*-*], [avr]],
      [[bfin-*-*], [bfin]],
      [[bpf-*-*], [bpf]],
      [[cr16*-*-*], [cr16]],
      [[cris-*-* | crisv32-*-*], [cris]],
      [[d10v-*-*], [d10v]],
      [[frv-*-*], [frv]],
      [[ft32-*-*], [ft32]],
      [[h8300*-*-*], [h8300]],
      [[iq2000-*-*], [iq2000]],
      [[lm32-*-*], [lm32]],
      [[m32c-*-*], [m32c]],
      [[m32r-*-*], [m32r]],
      [[m68hc11-*-*|m6811-*-*], [m68hc11]],
      [[mcore-*-*], [mcore]],
      [[microblaze*-*-*], [microblaze]],
      [[mips*-*-*], [mips]],
      [[mn10300*-*-*], [mn10300]],
      [[moxie-*-*], [moxie]],
      [[msp430*-*-*], [msp430]],
      [[or1k*-*-*], [or1k]],
      [[powerpc*-*-*], [ppc]],
      [[pru*-*-*], [pru]],
      [[riscv*-*-*], [riscv]],
      [[rl78-*-*], [rl78]],
      [[rx-*-*], [rx]],
      [[sh*-*-*], [sh]],
      [[sparc-*-*], [erc32]],
      [[v850*-*-*], [v850]],
    ])
  done

  if test "x${enable_example_sims}" = xyes; then
    SIM_AC_TOOLCHAIN_FOR_TARGET(example-synacor)
    SIM_BUILD_TARGET([example-synacor])
  fi
  AM_CONDITIONAL([SIM_ENABLE_ARCH_examples], [test "${enable_example_sims}" = "yes"])
fi
AM_CONDITIONAL([ENABLE_SIM], [test "$ENABLE_SIM" = "yes"])

dnl Standard (and optional) simulator options.
dnl Eventually all simulators will support these.
SIM_AC_OPTION_ALIGNMENT
SIM_AC_OPTION_ASSERT
SIM_AC_OPTION_CGEN_MAINT
SIM_AC_OPTION_DEBUG
SIM_AC_OPTION_ENDIAN
SIM_AC_OPTION_ENVIRONMENT
SIM_AC_OPTION_HARDWARE
SIM_AC_OPTION_INLINE
SIM_AC_OPTION_PROFILE
SIM_AC_OPTION_RESERVED_BITS
SIM_AC_OPTION_SCACHE
SIM_AC_OPTION_SMP
SIM_AC_OPTION_STDIO
SIM_AC_OPTION_TRACE
SIM_AC_OPTION_WARNINGS

dnl Some arches have unique configure flags.
m4_include([frv/acinclude.m4])
m4_include([mips/acinclude.m4])
m4_include([ppc/acinclude.m4])
m4_include([riscv/acinclude.m4])
m4_include([rx/acinclude.m4])

AC_CONFIG_FILES([Makefile .gdbinit:gdbinit.in])
AC_OUTPUT
