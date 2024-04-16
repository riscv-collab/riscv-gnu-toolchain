# Copyright (C) 2021-2024 Free Software Foundation, Inc.

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

# This file is part of the GDB testsuite. It tests that python pretty
# printers defined in a python script that is autoloaded have been
# registered when a custom event handler for the new_objfile event
# is called.

import gdb.printing


class MyClassTestLibPrinter(object):
    "Print a MyClassTestLib"

    def __init__(self, val):
        self.val = val

    def to_string(self):
        return "MyClassTestLib object, id: {}".format(self.val["id"])

    def display_hint(self):
        return "string"


def build_pretty_printer():
    pp = gdb.printing.RegexpCollectionPrettyPrinter("my_library")
    pp.add_printer("MyClassTestLib", "^MyClassTestLib$", MyClassTestLibPrinter)
    return pp


gdb.printing.register_pretty_printer(gdb.current_objfile(), build_pretty_printer())
