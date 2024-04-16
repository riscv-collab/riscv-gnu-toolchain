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

from perftest import perftest


class BackTrace(perftest.TestCaseWithBasicMeasurements):
    def __init__(self, depth):
        super(BackTrace, self).__init__("backtrace")
        self.depth = depth

    def warm_up(self):
        # Warm up.
        gdb.execute("bt", False, True)
        gdb.execute("bt", False, True)

    def _do_test(self):
        """Do backtrace multiple times."""
        do_test_command = "bt %d" % self.depth
        for _ in range(1, 15):
            gdb.execute(do_test_command, False, True)

    def execute_test(self):
        line_size = 2
        for _ in range(1, 12):
            # Keep the total size of dcache unchanged, and increase the
            # line-size in the loop.
            line_size_command = "set dcache line-size %d" % (line_size)
            size_command = "set dcache size %d" % (4096 * 64 / line_size)
            # Cache is cleared by changing line-size or size.
            gdb.execute(line_size_command)
            gdb.execute(size_command)

            func = lambda: self._do_test()

            self.measure.measure(func, line_size)

            line_size *= 2
