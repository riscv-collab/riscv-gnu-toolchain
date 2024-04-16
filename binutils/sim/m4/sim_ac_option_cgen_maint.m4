dnl Copyright (C) 1997-2024 Free Software Foundation, Inc.
dnl
dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 3 of the License, or
dnl (at your option) any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program.  If not, see <http://www.gnu.org/licenses/>.
dnl
dnl --enable-cgen-maint support
AC_DEFUN([SIM_AC_OPTION_CGEN_MAINT],
[
cgen_maint=no
dnl Default is to use one in build tree.
cgen=guile
cgendir='$(srcdir)/../../cgen'
if test -r ${srcdir}/../cgen/iformat.scm; then
    cgendir='$(srcdir)/../cgen'
fi
dnl Having --enable-maintainer-mode take arguments is another way to go.
dnl ??? One can argue --with is more appropriate if one wants to specify
dnl a directory name, but what we're doing here is an enable/disable kind
dnl of thing and specifying both --enable and --with is klunky.
dnl If you reeely want this to be --with, go ahead and change it.
AC_ARG_ENABLE(cgen-maint,
[AS_HELP_STRING([--enable-cgen-maint[=DIR]], [build cgen generated files])],
[case "${enableval}" in
  yes)	cgen_maint=yes ;;
  no)	cgen_maint=no ;;
  *)
	# Argument is a directory where cgen can be found.  In some
	# future world cgen could be installable, but right now this
	# is not the case.  Instead we assume the directory is a path
	# to the cgen source tree.
	cgen_maint=yes
        if test -r ${enableval}/iformat.scm; then
          # This looks like a cgen source tree.
	  cgendir=${enableval}
        else
	  AC_MSG_ERROR(${enableval} doesn't look like a cgen source tree)
        fi
	;;
esac])dnl
dnl AM_CONDITIONAL(CGEN_MAINT, test x${cgen_maint} != xno)
if test x${cgen_maint} != xno ; then
  CGEN_MAINT=''
else
  CGEN_MAINT='#'
fi
AC_SUBST(CGEN_MAINT)
AC_SUBST(cgendir)
AC_SUBST(cgen)
])
