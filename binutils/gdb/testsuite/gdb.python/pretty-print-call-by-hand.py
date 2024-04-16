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


class MytypePrinter:
    """pretty print my type"""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        calls = gdb.parse_and_eval("f()")
        return "mytype is %s" % self.val["x"]


def ec_lookup_function(val):
    typ = val.type
    if typ.code == gdb.TYPE_CODE_REF:
        typ = typ.target()
    if str(typ) == "struct mytype":
        return MytypePrinter(val)
    return None


def disable_lookup_function():
    ec_lookup_function.enabled = False


def enable_lookup_function():
    ec_lookup_function.enabled = True


gdb.pretty_printers.append(ec_lookup_function)
