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
import gdb.printing


class TestPrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        return "map"

    def children(self):
        yield "1", "flicker"


def str_lookup_function(val):
    lookup_tag = val.type.tag
    if lookup_tag == "test":
        return TestPrinter(val)
    if val.type.code == gdb.TYPE_CODE_PTR and val.type.target().tag == "test":
        return TestPrinter(val.dereference())


gdb.printing.register_pretty_printer(None, str_lookup_function)
