#=========================================================================
# configure.ac for riscv libgloss and crt0
#=========================================================================

#-------------------------------------------------------------------------
# Project metadata
#-------------------------------------------------------------------------

m4_define( proj_name,         [RISC-V libgloss])
m4_define( proj_version,      [?])
m4_define( proj_maintainer,   [Andrew Waterman])
m4_define( proj_abbreviation, [riscv-libgloss])

#-------------------------------------------------------------------------
# Setup
#-------------------------------------------------------------------------

AC_INIT([proj_name],[proj_version],[proj_maintainer],[proj_abbreviation])
AC_CONFIG_SRCDIR([crt0.S])
AC_CONFIG_AUX_DIR([${srcdir}/../..])

#-------------------------------------------------------------------------
# Checks for programs
#-------------------------------------------------------------------------

LIB_AC_PROG_CC
AC_CHECK_TOOL([AR],[ar])
AC_CHECK_TOOL([RANLIB],[ranlib])
AC_PROG_INSTALL

#-------------------------------------------------------------------------
# Output
#-------------------------------------------------------------------------

if test "$srcdir" = "." ; then
  if test "${with_target_subdir}" != "." ; then
    libgloss_topdir="${srcdir}/${with_multisrctop}../../.."
  else
    libgloss_topdir="${srcdir}/${with_multisrctop}../.."
  fi
else
  libgloss_topdir="${srcdir}/../.."
fi
AC_CONFIG_AUX_DIR($libgloss_topdir)
AC_CONFIG_FILES(Makefile,
. ${libgloss_topdir}/config-ml.in,
srcdir=${srcdir}
target=${target}
with_multisubdir=${with_multisubdir}
ac_configure_args="${ac_configure_args} --enable-multilib"
CONFIG_SHELL=${CONFIG_SHELL-/bin/sh}
libgloss_topdir=${libgloss_topdir}
)
AC_OUTPUT
