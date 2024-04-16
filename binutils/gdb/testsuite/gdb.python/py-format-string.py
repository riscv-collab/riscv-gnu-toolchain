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

# This file is part of the GDB testsuite.  It tests python pretty
# printers.

import gdb

saved_options = {}


class PointPrinter(object):
    def __init__(self, val):
        self.val = val

    def to_string(self):
        global saved_options
        saved_options = gdb.print_options()
        if saved_options["summary"]:
            return "No Data"
        return "Pretty Point (%s, %s)" % (self.val["x"], self.val["y"])


def test_lookup_function(val):
    "Look-up and return a pretty-printer that can print val."

    # Get the type.
    type = val.type

    # If it points to a reference, get the reference.
    if type.code == gdb.TYPE_CODE_REF:
        type = type.target()

    # Get the unqualified type, stripped of typedefs.
    type = type.unqualified().strip_typedefs()

    # Get the type name.
    typename = type.tag

    if typename == "point":
        return PointPrinter(val)

    return None


gdb.pretty_printers.append(test_lookup_function)
