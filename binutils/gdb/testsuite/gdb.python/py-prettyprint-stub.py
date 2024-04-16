# Copyright (C) 2023-2024 Free Software Foundation, Inc.

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
# It tests Python-based pretty-printing of stubs.


class SPrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        char_ptr = gdb.lookup_type("char").pointer()
        int_ptr = gdb.lookup_type("int").pointer()
        # m_i should be after the vtable, which has the size of a pointer
        i = (
            (self.val.address.cast(char_ptr) + int_ptr.sizeof)
            .cast(int_ptr)
            .dereference()
        )
        return "{pp m_i = %d}" % int(i)


pp = gdb.printing.RegexpCollectionPrettyPrinter("S")
pp.add_printer("S", "^S$", SPrinter)
gdb.printing.register_pretty_printer(gdb.current_objfile(), pp, True)
