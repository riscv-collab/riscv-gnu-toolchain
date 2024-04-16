# Copyright (C) 2015-2024 Free Software Foundation, Inc.
#
# This file is free software; the Free Software Foundation gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.

# GDB_AC_TRANSFORM([PROGRAM], [VAR])
#
# Transform a tool name to get the installed name of PROGRAM and store
# it in the output variable VAR.
#
# This macro uses the SED command stored in $program_transform_name,
# but it undoes the Makefile-like escaping of $s performed by
# AC_ARG_PROGRAM.

AC_DEFUN([GDB_AC_TRANSFORM], [
  gdb_ac_transform=`echo "$program_transform_name" | sed -e 's/[\\$][\\$]/\\$/g'`
  $2=`echo $1 | sed -e "$gdb_ac_transform"`
  if test "x$$2" = x; then
     $2=$1
  fi
  AC_SUBST($2)
])
