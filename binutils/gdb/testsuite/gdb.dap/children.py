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


import gdb


class Printer(gdb.ValuePrinter):
    """Pretty print test class."""

    def __init__(self, val):
        self._val = val

    def to_string(self):
        return "contents"

    def children(self):
        yield "first", 23
        yield "second", "DEI"


def lookup_function(val):
    typ = val.type
    if typ.code == gdb.TYPE_CODE_PTR:
        return Printer(val)
    return None


gdb.pretty_printers.append(lookup_function)
