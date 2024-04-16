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


class Disassemble(perftest.TestCaseWithBasicMeasurements):
    def __init__(self):
        super(Disassemble, self).__init__("disassemble")

    def warm_up(self):
        do_test_command = "disassemble evaluate_subexp_do_call"
        gdb.execute(do_test_command, False, True)

    def _do_test(self, c):
        for func in [
            "captured_main",
            "handle_inferior_event",
            "run_inferior_call",
            "update_global_location_list",
        ]:
            do_test_command = "disassemble %s" % func
            for _ in range(c + 1):
                gdb.execute(do_test_command, False, True)

    def execute_test(self):
        for i in range(3):
            # Flush code cache.
            gdb.execute("set code-cache off")
            gdb.execute("set code-cache on")
            self.measure.measure(lambda: self._do_test(i), i)
