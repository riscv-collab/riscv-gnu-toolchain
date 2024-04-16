# Copyright (C) 2019-2024 Free Software Foundation, Inc.

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

# The toplevel makefile overrides $(CC) (in EXTRA_HOST_FLAGS) by setting
# it to whatever CC was in the toplevel configure.  This breaks in Gnulib with
# older GCCs, because Gnulib's configure uses this variable to store the
# standard-setting switch (e.g. -std=gnu99/-std=gnu11).  To fix this, we just
# set MAKEOVERRIDES to empty here so that import/ uses the right $(CC).
#
# This problem is visible with compilers that don't use C99 by default, such
# as GCC 4.8.5 (Centos 7).
MAKEOVERRIDES =

SUBDIRS = import
