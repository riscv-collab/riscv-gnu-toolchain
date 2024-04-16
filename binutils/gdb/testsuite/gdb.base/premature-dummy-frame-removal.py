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

# This dummy unwinder will break GDB's backtrce at the function called
# 'break_bt_here'.

import gdb
from gdb.unwinder import Unwinder


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
        Unwinder.__init__(self, "break unwinding")

    def __call__(self, pending_frame):
        pc_desc = pending_frame.architecture().registers().find("pc")
        pc = pending_frame.read_register(pc_desc)

        sp_desc = pending_frame.architecture().registers().find("sp")
        sp = pending_frame.read_register(sp_desc)

        block = gdb.block_for_pc(int(pc))
        if block is None:
            return None
        func = block.function
        if func is None:
            return None
        if str(func) != "break_bt_here":
            return None
        fid = FrameId(pc, sp)
        return pending_frame.create_unwind_info(fid)


gdb.unwinder.register_unwinder(None, TestUnwinder(), True)
