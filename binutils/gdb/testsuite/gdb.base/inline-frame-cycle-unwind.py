# Copyright (C) 2021-2024 Free Software Foundation, Inc.

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

import gdb
from gdb.unwinder import Unwinder

# Set this to the stack level the backtrace should be corrupted at.
# This will only work for frame 1, 3, or 5 in the test this unwinder
# was written for.
stop_at_level = None

# Set this to the stack frame size of frames 1, 3, and 5.  These
# frames will all have the same stack frame size as they are the same
# function called recursively.
stack_adjust = None


class FrameId(object):
    def __init__(self, sp, pc):
        self._sp = sp
        self._pc = pc

    @property
    def sp(self):
        return self._sp

    @property
    def pc(self):
        return self._pc


class TestUnwinder(Unwinder):
    def __init__(self):
        Unwinder.__init__(self, "stop at level")

    def __call__(self, pending_frame):
        global stop_at_level
        global stack_adjust

        if stop_at_level is None or pending_frame.level() != stop_at_level:
            return None

        if stack_adjust is None:
            raise gdb.GdbError("invalid stack_adjust")

        if not stop_at_level in [1, 3, 5]:
            raise gdb.GdbError("invalid stop_at_level")

        sp_desc = pending_frame.architecture().registers().find("sp")
        sp = pending_frame.read_register(sp_desc) + stack_adjust
        pc = (gdb.lookup_symbol("normal_func"))[0].value().address
        unwinder = pending_frame.create_unwind_info(FrameId(sp, pc))

        for reg in pending_frame.architecture().registers("general"):
            val = pending_frame.read_register(reg)
            unwinder.add_saved_register(reg, val)
        return unwinder


gdb.unwinder.register_unwinder(None, TestUnwinder(), True)

# When loaded, it is expected that the stack looks like:
#
#   main -> normal_func -> inline_func -> normal_func -> inline_func -> normal_func -> inline_func
#
# Compute the stack frame size of normal_func, which has inline_func
# inlined within it.
f0 = gdb.newest_frame()
f1 = f0.older()
f2 = f1.older()
f0_sp = f0.read_register("sp")
f2_sp = f2.read_register("sp")
stack_adjust = f2_sp - f0_sp
