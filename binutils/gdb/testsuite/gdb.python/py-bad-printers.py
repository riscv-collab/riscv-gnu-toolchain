# Copyright (C) 2008-2024 Free Software Foundation, Inc.

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

# This file is part of the GDB testsuite.  It tests GDB's handling of
# bad python pretty printers.

# Test a printer with a bad children iterator.

import re
import gdb.printing


class BadChildrenContainerPrinter1(object):
    """Children iterator doesn't return a tuple of two elements."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        return "container %s with %d elements" % (self.val["name"], self.val["len"])

    @staticmethod
    def _bad_iterator(pointer, len):
        start = pointer
        end = pointer + len
        while pointer != end:
            yield "intentional violation of children iterator protocol"
            pointer += 1

    def children(self):
        return self._bad_iterator(self.val["elements"], self.val["len"])


class BadChildrenContainerPrinter2(object):
    """Children iterator returns a tuple of two elements with bad values."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        return "container %s with %d elements" % (self.val["name"], self.val["len"])

    @staticmethod
    def _bad_iterator(pointer, len):
        start = pointer
        end = pointer + len
        while pointer != end:
            # The first argument is supposed to be a string.
            yield (42, "intentional violation of children iterator protocol")
            pointer += 1

    def children(self):
        return self._bad_iterator(self.val["elements"], self.val["len"])


def build_pretty_printer():
    pp = gdb.printing.RegexpCollectionPrettyPrinter("bad-printers")

    pp.add_printer("container1", "^container$", BadChildrenContainerPrinter1)
    pp.add_printer("container2", "^container$", BadChildrenContainerPrinter2)

    return pp


my_pretty_printer = build_pretty_printer()
gdb.printing.register_pretty_printer(gdb, my_pretty_printer)
