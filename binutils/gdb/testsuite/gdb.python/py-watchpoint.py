# Copyright (C) 2022-2024 Free Software Foundation, Inc.
#
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


class MyBreakpoint(gdb.Breakpoint):
    def __init__(self):
        super().__init__("i", gdb.BP_WATCHPOINT)
        self.n = 0

    def stop(self):
        self.n += 1
        print("Watchpoint Hit:", self.n, flush=True)
        return False


bpt = MyBreakpoint()

print("Python script imported")
