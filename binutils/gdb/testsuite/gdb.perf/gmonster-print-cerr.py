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

# Test printing of std::cerr.
# libstdc++ is typically near the end of the list of shared libraries,
# and thus searched last (or near last).
# Plus llvm had a bug where its pubnames output that gold uses to generate
# the index caused a massive perf regression (basically it emitted an entry
# for every CU that used it, whereas we only want the CU with the
# definition).
#
# Note: One difference between this test and gmonster-ptype-string.py
# is that here we do not pre-expand the symtab: we don't want include
# GDB's slowness in searching expanded symtabs first to color these results.

from perftest import perftest
from perftest import measure
from perftest import utils


class PrintCerr(perftest.TestCaseWithBasicMeasurements):
    def __init__(self, name, run_names, binfile):
        super(PrintCerr, self).__init__(name)
        self.run_names = run_names
        self.binfile = binfile

    def warm_up(self):
        pass

    def execute_test(self):
        for run in self.run_names:
            this_run_binfile = "%s-%s" % (self.binfile, utils.convert_spaces(run))
            utils.select_file(this_run_binfile)
            utils.runto_main()
            iteration = 5
            while iteration > 0:
                utils.safe_execute("mt flush symbol-cache")
                func = lambda: utils.safe_execute("print gm_std::cerr")
                self.measure.measure(func, run)
                iteration -= 1
