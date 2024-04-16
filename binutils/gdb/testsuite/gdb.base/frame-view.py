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


class Printer1:
    def to_string(self):
        n = gdb.parse_and_eval("called_from_pretty_printer ()")
        assert n == 23
        return "hahaha"


class Printer2:
    def to_string(self):
        n = gdb.parse_and_eval("called_from_pretty_printer ()")
        assert n == 23
        return "hohoho"


def lookup_function(val):
    if str(val.type) == "struct type_1":
        return Printer1()

    if str(val.type) == "struct type_2":
        return Printer2()

    return None


gdb.pretty_printers.append(lookup_function)
