#!/bin/sh

# GDB script to create list of files to check using gdb_ari.sh.
#
# Copyright (C) 2003-2024 Free Software Foundation, Inc.
#
# This file is part of GDB.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# Make certain that the script is not running in an internationalized
# environment.

LANG=C ; export LANG
LC_ALL=C ; export LC_ALL


# A find that prunes files that GDB users shouldn't be interested in.
# Use sort to order files alphabetically.

find "$@" \
    -name testsuite -prune -o \
    -name gdbserver -prune -o \
    -name gdbtk -prune -o \
    -name gnulib -prune -o \
    -name '*-stub.c' -prune -o \
    -name '*-exp.c' -prune -o \
    -name ada-lex.c -prune -o \
    -name cp-name-parser.c -prune -o \
    -type f -name '*.[lyhc]' -print | sort
