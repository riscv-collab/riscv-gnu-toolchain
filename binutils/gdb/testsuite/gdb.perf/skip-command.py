# Copyright (C) 2016-2024 Free Software Foundation, Inc.

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


class SkipCommand(perftest.TestCaseWithBasicMeasurements):
    def __init__(self, name, step):
        super(SkipCommand, self).__init__(name)
        self.step = step

    def warm_up(self):
        for _ in range(0, 10):
            gdb.execute("step", False, True)

    def _run(self, r):
        for _ in range(0, r):
            gdb.execute("step", False, True)

    def execute_test(self):
        for i in range(1, 5):
            func = lambda: self._run(i * self.step)
            self.measure.measure(func, i * self.step)
