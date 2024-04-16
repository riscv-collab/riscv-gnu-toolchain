# Copyright (C) 2010-2024 Free Software Foundation, Inc.

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

# This file is part of the GDB testsuite.  It tests python pretty
# printer registration.

import re
import gdb.types
import gdb.printing


def lookup_function_lookup_test(val):
    class PrintFunctionLookup(object):
        def __init__(self, val):
            self.val = val

        def to_string(self):
            return "x=<" + str(self.val["x"]) + "> y=<" + str(self.val["y"]) + ">"

    typename = gdb.types.get_basic_type(val.type).tag
    # Note: typename could be None.
    if typename == "function_lookup_test":
        return PrintFunctionLookup(val)
    return None


class pp_s1(object):
    def __init__(self, val):
        self.val = val

    def to_string(self):
        a = self.val["a"]
        b = self.val["b"]
        return "s1 a=<" + str(self.val["a"]) + "> b=<" + str(self.val["b"]) + ">"


class pp_s2(object):
    def __init__(self, val):
        self.val = val

    def to_string(self):
        a = self.val["a"]
        b = self.val["b"]
        return "s2 a=<" + str(self.val["a"]) + "> b=<" + str(self.val["b"]) + ">"


def build_pretty_printer1():
    pp = gdb.printing.RegexpCollectionPrettyPrinter("pp-test")

    pp.add_printer("struct s", "^struct s$", pp_s1)
    pp.add_printer("s", "^s$", pp_s1)

    return pp


def build_pretty_printer2():
    # This intentionally has the same name as build_pretty_printer1.
    # It is used to test the "replace" functionality of
    # register_pretty_printer.
    pp = gdb.printing.RegexpCollectionPrettyPrinter("pp-test")

    pp.add_printer("struct s", "^struct s$", pp_s2)
    pp.add_printer("s", "^s$", pp_s2)

    return pp


# Note: Registering the printers is done in the .exp file.
