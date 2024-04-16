# Copyright (C) 2018-2024 Free Software Foundation, Inc.

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

import sys
import gdb
import gdb.types


class cons_pp(object):
    def __init__(self, val):
        self._val = val

    def to_string(self):
        if int(self._val) == 0:
            return "nil"
        elif int(self._val["type"]) == 0:
            return "( . )"
        else:
            return "%d" % self._val["atom"]["ival"]

    def children(self):
        if int(self._val) == 0:
            return []
        elif int(self._val["type"]) == 0:
            return [("atom", self._val["atom"])]
        else:
            return [
                ("car", self._val["slots"][0]),
                ("cdr", self._val["slots"][1]),
            ]


def cons_pp_lookup(val):
    if str(val.type) == "struct cons *":
        return cons_pp(val)
    else:
        return None


del gdb.pretty_printers[1:]
gdb.pretty_printers.append(cons_pp_lookup)
