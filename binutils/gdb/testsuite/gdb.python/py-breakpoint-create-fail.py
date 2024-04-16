# Copyright (C) 2016-2024 Free Software Foundation, Inc.
#
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


class MyBP(gdb.Breakpoint):
    def stop(self):
        print("MyBP.stop was invoked!")
        # Don't make this breakpoint stop
        return False


try:
    bp = MyBP("does_not_exist", gdb.BP_WATCHPOINT)
except RuntimeError:
    pass
else:
    assert False
