# ffs.m4 serial 5
dnl Copyright (C) 2011-2022 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_FFS],
[
  AC_REQUIRE([gl_STRINGS_H_DEFAULTS])

  dnl We can't use AC_CHECK_FUNC here, because ffs() is defined as a
  dnl static inline function when compiling for Android 4.2 or older.
  dnl But require that ffs() is declared; otherwise we may be using
  dnl the GCC built-in function, which leads to warnings
  dnl "warning: implicit declaration of function 'ffs'".
  AC_CACHE_CHECK([for ffs], [gl_cv_func_ffs],
    [AC_LINK_IFELSE(
       [AC_LANG_PROGRAM(
          [[#include <strings.h>
            int x;
          ]],
          [[int (*func) (int) = ffs;
            return func (x);
          ]])
       ],
       [gl_cv_func_ffs=yes],
       [gl_cv_func_ffs=no])
    ])
  if test $gl_cv_func_ffs = no; then
    HAVE_FFS=0
  fi
])
