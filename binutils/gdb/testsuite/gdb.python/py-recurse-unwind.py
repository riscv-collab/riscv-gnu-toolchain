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

# This unwinder never does any unwinding.  It'll (pretend to) "sniff"
# the frame and ultimately return None, indicating that actual unwinding
# should be performed by some other unwinder.
#
# But, prior to returning None, it will attempt to obtain the value
# associated with a symbol via a call to gdb.parse_and_eval().  In
# the course of doing this evaluation, GDB will potentially access
# some frames, leading to the possibility of a recursive invocation of
# this unwinder.  If that should happen, code contained herein detects
# that and prints a message which will cause some of the associated
# tests to FAIL.

import gdb
from gdb.unwinder import Unwinder


class TestUnwinder(Unwinder):
    count = 0

    @classmethod
    def reset_count(cls):
        cls.count = 0

    @classmethod
    def inc_count(cls):
        cls.count += 1

    test = "check_undefined_symbol"

    @classmethod
    def set_test(cls, test):
        cls.test = test

    def __init__(self):
        Unwinder.__init__(self, "test unwinder")
        self.recurse_level = 0

    def __call__(self, pending_frame):
        if self.recurse_level > 0:
            gdb.write("TestUnwinder: Recursion detected - returning early.\n")
            return None

        self.recurse_level += 1
        TestUnwinder.inc_count()

        if TestUnwinder.test == "check_user_reg_pc":
            pc = pending_frame.read_register("pc")
            pc_as_int = int(pc.cast(gdb.lookup_type("int")))
            # gdb.write("In unwinder: pc=%x\n" % pc_as_int)

        elif TestUnwinder.test == "check_pae_pc":
            pc = gdb.parse_and_eval("$pc")
            pc_as_int = int(pc.cast(gdb.lookup_type("int")))
            # gdb.write("In unwinder: pc=%x\n" % pc_as_int)

        elif TestUnwinder.test == "check_undefined_symbol":
            try:
                val = gdb.parse_and_eval("undefined_symbol")

            except Exception as arg:
                pass

        self.recurse_level -= 1

        return None


gdb.unwinder.register_unwinder(None, TestUnwinder(), True)
gdb.write("Python script imported\n")
