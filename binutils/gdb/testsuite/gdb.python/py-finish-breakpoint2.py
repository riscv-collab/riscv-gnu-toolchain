# Copyright (C) 2011-2024 Free Software Foundation, Inc.

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

# This file is part of the GDB testsuite.  It tests python Finish
# Breakpoints.


class ExceptionFinishBreakpoint(gdb.FinishBreakpoint):
    def __init__(self, frame):
        gdb.FinishBreakpoint.__init__(self, frame, internal=1)
        self.silent = True
        print("init ExceptionFinishBreakpoint")

    def stop(self):
        print("stopped at ExceptionFinishBreakpoint")
        return True

    def out_of_scope(self):
        print("exception did not finish ...")


print("Python script imported")
