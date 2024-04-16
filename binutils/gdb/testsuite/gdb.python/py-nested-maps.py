# Copyright (C) 2019-2024 Free Software Foundation, Inc.

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

# This file is part of the GDB testsuite.  It tests GDB's printing of
# nested map like structures.

import re
import gdb


def _iterator1(pointer, len):
    while len > 0:
        map = pointer.dereference()
        yield ("", map["name"])
        yield ("", map.dereference())
        pointer += 1
        len -= 1


def _iterator2(pointer1, pointer2, len):
    while len > 0:
        yield ("", pointer1.dereference())
        yield ("", pointer2.dereference())
        pointer1 += 1
        pointer2 += 1
        len -= 1


class pp_map(object):
    def __init__(self, val):
        self.val = val

    def to_string(self):
        if self.val["show_header"] == 0:
            return None
        else:
            return "pp_map"

    def children(self):
        return _iterator2(self.val["keys"], self.val["values"], self.val["length"])

    def display_hint(self):
        return "map"


class pp_map_map(object):
    def __init__(self, val):
        self.val = val

    def to_string(self):
        if self.val["show_header"] == 0:
            return None
        else:
            return "pp_map_map"

    def children(self):
        return _iterator1(self.val["values"], self.val["length"])

    def display_hint(self):
        return "map"


def lookup_function(val):
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

    if typename is None:
        return None

    # Iterate over local dictionary of types to determine
    # if a printer is registered for that type.  Return an
    # instantiation of the printer if found.
    for function in pretty_printers_dict:
        if function.match(typename):
            return pretty_printers_dict[function](val)

    # Cannot find a pretty printer.  Return None.
    return None


# Lookup a printer for VAL in the typedefs dict.
def lookup_typedefs_function(val):
    "Look-up and return a pretty-printer that can print val (typedefs)."

    # Get the type.
    type = val.type

    if type is None or type.name is None or type.code != gdb.TYPE_CODE_TYPEDEF:
        return None

    # Iterate over local dictionary of typedef types to determine if a
    # printer is registered for that type.  Return an instantiation of
    # the printer if found.
    for function in typedefs_pretty_printers_dict:
        if function.match(type.name):
            return typedefs_pretty_printers_dict[function](val)

    # Cannot find a pretty printer.
    return None


def register_pretty_printers():
    pretty_printers_dict[re.compile("^struct map_t$")] = pp_map
    pretty_printers_dict[re.compile("^map_t$")] = pp_map
    pretty_printers_dict[re.compile("^struct map_map_t$")] = pp_map_map
    pretty_printers_dict[re.compile("^map_map_t$")] = pp_map_map


# Dict for struct types with typedefs fully stripped.
pretty_printers_dict = {}
# Dict for typedef types.
typedefs_pretty_printers_dict = {}

register_pretty_printers()
gdb.pretty_printers.append(lookup_function)
gdb.pretty_printers.append(lookup_typedefs_function)
