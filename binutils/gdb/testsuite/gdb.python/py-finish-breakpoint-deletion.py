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


class MyFinishBreakpoint(gdb.FinishBreakpoint):
    def stop(self):
        print("stopped at MyFinishBreakpoint")
        return self.return_value == 4


class MyBreakpoint(gdb.Breakpoint):
    def stop(self):
        print("stopped at MyBreakpoint")
        MyFinishBreakpoint()
        return False


MyBreakpoint("subroutine", internal=True)

print("Python script imported")
