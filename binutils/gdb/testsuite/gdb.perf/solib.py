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

# This test case is to test the speed of GDB when it is handling the
# shared libraries of inferior are loaded and unloaded.

from perftest import perftest
from perftest import measure


class SolibLoadUnload1(perftest.TestCaseWithBasicMeasurements):
    def __init__(self, solib_count, measure_load):
        if measure_load:
            name = "solib_load"
        else:
            name = "solib_unload"
        # We want to measure time in this test.
        super(SolibLoadUnload1, self).__init__(name)
        self.solib_count = solib_count
        self.measure_load = measure_load

    def warm_up(self):
        do_test_load = "call do_test_load (%d)" % self.solib_count
        do_test_unload = "call do_test_unload (%d)" % self.solib_count
        gdb.execute(do_test_load)
        gdb.execute(do_test_unload)

    def execute_test(self):
        num = self.solib_count
        iteration = 5

        while num > 0 and iteration > 0:
            # Do inferior calls to do_test_load and do_test_unload in pairs,
            # but measure differently.
            if self.measure_load:
                do_test_load = "call do_test_load (%d)" % num
                func = lambda: gdb.execute(do_test_load)

                self.measure.measure(func, num)

                do_test_unload = "call do_test_unload (%d)" % num
                gdb.execute(do_test_unload)

            else:
                do_test_load = "call do_test_load (%d)" % num
                gdb.execute(do_test_load)

                do_test_unload = "call do_test_unload (%d)" % num
                func = lambda: gdb.execute(do_test_unload)

                self.measure.measure(func, num)

            num = num / 2
            iteration -= 1


class SolibLoadUnload(object):
    def __init__(self, solib_count):
        self.solib_count = solib_count

    def run(self):
        SolibLoadUnload1(self.solib_count, True).run()
        SolibLoadUnload1(self.solib_count, False).run()
