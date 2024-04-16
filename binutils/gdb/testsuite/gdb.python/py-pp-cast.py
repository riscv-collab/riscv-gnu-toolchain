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


class PpIntPrinter(object):
    def __init__(self, val):
        self.val = val

    def to_string(self):
        val = self.val.cast(self.val.type)
        return "%dp" % int(val)


pp = gdb.printing.RegexpCollectionPrettyPrinter("pp-cast")
pp.add_printer("pp_int", "^pp_int$", PpIntPrinter)
gdb.printing.register_pretty_printer(gdb.current_objfile(), pp)
