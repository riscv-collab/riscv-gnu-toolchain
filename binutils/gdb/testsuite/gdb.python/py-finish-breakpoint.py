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


class MyFinishBreakpoint(gdb.FinishBreakpoint):
    def __init__(self, val, frame):
        gdb.FinishBreakpoint.__init__(self, frame)
        print("MyFinishBreakpoint init")
        self.val = val

    def stop(self):
        print("MyFinishBreakpoint stop with %d" % int(self.val.dereference()))
        print("return_value is: %d" % int(self.return_value))
        gdb.execute("where 1")
        return True

    def out_of_scope(self):
        print("MyFinishBreakpoint out of scope")


class TestBreakpoint(gdb.Breakpoint):
    def __init__(self):
        gdb.Breakpoint.__init__(self, spec="test_1", internal=1)
        self.silent = True
        self.count = 0
        print("TestBreakpoint init")

    def stop(self):
        self.count += 1
        try:
            TestFinishBreakpoint(gdb.newest_frame(), self.count)
        except ValueError as e:
            print(e)
        return False


class TestFinishBreakpoint(gdb.FinishBreakpoint):
    def __init__(self, frame, count):
        self.count = count
        gdb.FinishBreakpoint.__init__(self, frame, internal=1)

    def stop(self):
        print("-->", self.number)
        if self.count == 3:
            print("test stop: %d" % self.count)
            return True
        else:
            print("test don't stop: %d" % self.count)
            return False

    def out_of_scope(self):
        print("test didn't finish: %d" % self.count)


class TestExplicitBreakpoint(gdb.Breakpoint):
    def stop(self):
        try:
            SimpleFinishBreakpoint(gdb.newest_frame())
        except ValueError as e:
            print(e)
        return False


class SimpleFinishBreakpoint(gdb.FinishBreakpoint):
    def __init__(self, frame):
        gdb.FinishBreakpoint.__init__(self, frame)

        print("SimpleFinishBreakpoint init")

    def stop(self):
        print("SimpleFinishBreakpoint stop")
        return True

    def out_of_scope(self):
        print("SimpleFinishBreakpoint out of scope")


print("Python script imported")
