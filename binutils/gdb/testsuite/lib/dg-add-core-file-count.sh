#!/bin/sh

# Copyright (C) 2022-2024 Free Software Foundation, Inc.

# This file is part of GDB.

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

# Count number of core files in the current directory and if non-zero,
# add a line to the gdb.sum file.  This scripts assumes it is run from
# the build/gdb/testsuite/ directory.  It is normally invoked by the
# Makefile.

# Count core files portably, using POSIX compliant shell, avoiding ls,
# find, wc, etc.  Spawning a subshell isn't strictly needed, but it's
# clearer.  The "*core*" pattern is this lax in order to find all of
# "core", "core.PID", "core.<program>.PID", "<program>.core", etc.
cores=$(set -- *core*; [ $# -eq 1 -a ! -e "$1" ] && shift; echo $#)

# If no cores found, then don't add our summary line.
if [ "$cores" -eq "0" ]; then
    exit
fi

# Add our line to the summary.
sed -i'' -e "/=== gdb Summary ===/{
n
a\\
# of unexpected core files	$cores
}" gdb.sum
