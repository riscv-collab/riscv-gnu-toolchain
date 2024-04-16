# Copyright (C) 2022-2024 Free Software Foundation, Inc.

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


class EmptyPrinter(gdb.ValuePrinter):
    """Pretty print a structure"""

    def __init__(self, val):
        self._val = val

    def to_string(self):
        return "empty"


class FullPrinter(gdb.ValuePrinter):
    """Pretty print a structure"""

    def __init__(self, val):
        self._val = val

    def to_string(self):
        return "full"

    def children(self):
        # This is used to test the renaming code.
        yield "datum", self._val["datum"]
        yield "datum", self._val["datum"]


def lookup_function(val):
    if val.type.tag == "special_type":
        if val["disc"] == 0:
            return EmptyPrinter(val)
        else:
            return FullPrinter(val)
    return None


gdb.pretty_printers.append(lookup_function)
