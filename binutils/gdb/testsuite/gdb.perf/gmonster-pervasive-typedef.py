# Copyright (C) 2015-2024 Free Software Foundation, Inc.

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

# Measure the speed of "ptype func" where a parameter of the function is a
# typedef used pervasively.  This exercises the perf regression introduced by
# the original patch to pr 16253.

from perftest import perftest
from perftest import measure
from perftest import utils


class PervasiveTypedef(perftest.TestCaseWithBasicMeasurements):
    def __init__(self, name, run_names, binfile):
        # We want to measure time in this test.
        super(PervasiveTypedef, self).__init__(name)
        self.run_names = run_names
        self.binfile = binfile

    def warm_up(self):
        pass

    def func(self):
        utils.select_file(self.this_run_binfile)
        utils.safe_execute("ptype call_use_my_int_1")

    def execute_test(self):
        for run in self.run_names:
            self.this_run_binfile = "%s-%s" % (self.binfile, utils.convert_spaces(run))
            iteration = 5
            while iteration > 0:
                self.measure.measure(self.func, run)
                iteration -= 1
