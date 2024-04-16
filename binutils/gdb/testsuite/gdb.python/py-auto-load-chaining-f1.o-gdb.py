# Copyright (C) 2021-2024 Free Software Foundation, Inc.
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

# This script is auto-loaded when the py-auto-load-chaining-f1.o
# object is loaded.

import re

print("Entering f1.o auto-load script")

print("Current objfile is: %s" % gdb.current_objfile().filename)

print("Chain loading f2.o...")

filename = gdb.current_objfile().filename
filename = re.sub(r"-f1.o$", "-f2.o", filename)
r2 = gdb.lookup_global_symbol("region_2").value()
gdb.execute("add-symbol-file %s 0x%x" % (filename, r2))

print("After loading f2.o...")
print("Current objfile is: %s" % gdb.current_objfile().filename)

print("Leaving f1.o auto-load script")
