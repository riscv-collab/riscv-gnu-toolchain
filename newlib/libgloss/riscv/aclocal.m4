#=========================================================================
# aclocal.m4 for maven libgloss
#=========================================================================
# We cannot use the normal AC_PROG_CC since that macro will try and do a
# link with the found compiler. Since we don't have all the startup
# files setup yet (that's what we are compiling in libgloss!) we want to
# find a compiler without actually doing a link. So the LIB_AC_PROG_CC
# check is copied from xcc/src/libgloss/acinclude.m4
 
#-------------------------------------------------------------------------
# LIB_AC_PROG_CC_GNU
#-------------------------------------------------------------------------

AC_DEFUN([LIB_AC_PROG_CC_GNU],
[
  AC_CACHE_CHECK(whether we are using GNU C, ac_cv_prog_gcc,
  [dnl The semicolon is to pacify NeXT's syntax-checking cpp.

cat > conftest.c <<EOF
#ifdef __GNUC__
  yes;
#endif
EOF

if AC_TRY_COMMAND(${CC-cc} -E conftest.c) | egrep yes >/dev/null 2>&1; then
  ac_cv_prog_gcc=yes
else
  ac_cv_prog_gcc=no
fi

  ])
])

#-------------------------------------------------------------------------
# LIB_AC_PROG_CC
#-------------------------------------------------------------------------

AC_DEFUN([LIB_AC_PROG_CC],
[
  AC_BEFORE([$0],[AC_PROG_CPP])dnl
  AC_CHECK_PROG(CC, gcc, gcc)

if test -z "$CC"; then
  AC_CHECK_PROG(CC, cc, cc, , , /usr/ucb/cc)
  test -z "$CC" && AC_MSG_ERROR([no acceptable cc found in \$PATH])
fi

LIB_AC_PROG_CC_GNU

if test $ac_cv_prog_gcc = yes; then
  GCC=yes
  dnl Check whether -g works, even if CFLAGS is set, in case the package
  dnl plays around with CFLAGS (such as to build both debugging and
  dnl normal versions of a library), tasteless as that idea is.
  ac_test_CFLAGS="${CFLAGS+set}"
  ac_save_CFLAGS="$CFLAGS"
  CFLAGS=
  _AC_PROG_CC_G
  if test "$ac_test_CFLAGS" = set; then
    CFLAGS="$ac_save_CFLAGS"
  elif test $ac_cv_prog_cc_g = yes; then
    CFLAGS="-g -O2"
  else
    CFLAGS="-O2"
  fi
else
  GCC=
  test "${CFLAGS+set}" = set || CFLAGS="-g"
fi

])

