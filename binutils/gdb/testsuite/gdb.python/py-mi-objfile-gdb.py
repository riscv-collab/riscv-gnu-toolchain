# Copyright (C) 2015-2024 Free Software Foundation, Inc.

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

# This file is part of the GDB testsuite.

import gdb

# PR 18833
# We want to have two levels of redirection while MI is current_uiout.
# This will create one for to_string=True and then another for the
# parameter change notification.
gdb.execute("set width 101", to_string=True)
# And finally a command that will use the console stream without redirection
gdb.execute("list -q main")
