# Copyright (C) 2013-2024 Free Software Foundation, Inc.

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

# This test case is to test the speed of GDB when it is analyzing the
# function prologue.

from perftest import perftest


class SkipPrologue(perftest.TestCaseWithBasicMeasurements):
    def __init__(self, count):
        super(SkipPrologue, self).__init__("skip-prologue")
        self.count = count

    def _test(self):
        for _ in range(1, self.count):
            # Insert breakpoints on function f1 and f2.
            bp1 = gdb.Breakpoint("f1")
            bp2 = gdb.Breakpoint("f2")
            # Remove them.
            bp1.delete()
            bp2.delete()

    def warm_up(self):
        self._test()

    def execute_test(self):
        for i in range(1, 4):
            gdb.execute("set code-cache off")
            gdb.execute("set code-cache on")
            self.measure.measure(self._test, i)
