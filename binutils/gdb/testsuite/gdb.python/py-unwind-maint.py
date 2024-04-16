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

# This file is part of the GDB testsuite.  It tests python unwinders.

import re
import gdb.types
from gdb.unwinder import Unwinder, register_unwinder


class TestGlobalUnwinder(Unwinder):
    def __init__(self):
        super(TestGlobalUnwinder, self).__init__("global_unwinder")

    def __call__(self, unwinder_info):
        print("%s called" % self.name)
        return None


class TestProgspaceUnwinder(Unwinder):
    def __init__(self, name):
        super(TestProgspaceUnwinder, self).__init__("%s_ps_unwinder" % name)

    def __call__(self, unwinder_info):
        print("%s called" % self.name)
        return None


class TestObjfileUnwinder(Unwinder):
    def __init__(self, name):
        super(TestObjfileUnwinder, self).__init__("%s_obj_unwinder" % name)

    def __call__(self, unwinder_info):
        print("%s called" % self.name)
        return None


gdb.unwinder.register_unwinder(None, TestGlobalUnwinder())
saw_runtime_error = False
try:
    gdb.unwinder.register_unwinder(None, TestGlobalUnwinder(), replace=False)
except RuntimeError:
    saw_runtime_error = True
if not saw_runtime_error:
    raise RuntimeError("Missing runtime error from register_unwinder.")
gdb.unwinder.register_unwinder(None, TestGlobalUnwinder(), replace=True)
gdb.unwinder.register_unwinder(
    gdb.current_progspace(), TestProgspaceUnwinder("py_unwind_maint")
)
print("Python script imported")
