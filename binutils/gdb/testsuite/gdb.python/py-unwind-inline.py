# Copyright (C) 2020-2024 Free Software Foundation, Inc.

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

# A dummy stack unwinder used for testing the Python unwinders when we
# have inline frames.  This unwinder will never claim any frames,
# instead, all it does it try to read all registers possible target
# registers as part of the frame sniffing process..

import gdb
from gdb.unwinder import Unwinder

apb_global = None


class dummy_unwinder(Unwinder):
    """A dummy unwinder that looks at a bunch of registers as part of
    the unwinding process."""

    class frame_id(object):
        """Basic frame id."""

        def __init__(self, sp, pc):
            """Create the frame id."""
            self.sp = sp
            self.pc = pc

    def __init__(self):
        """Create the unwinder."""
        Unwinder.__init__(self, "dummy stack unwinder")
        self.void_ptr_t = gdb.lookup_type("void").pointer()
        self.regs = None

    def get_regs(self, pending_frame):
        """Return a list of register names that should be read.  Only
        gathers the list once, then caches the result."""
        if self.regs is not None:
            return self.regs

        # Collect the names of all registers to read.
        self.regs = list(pending_frame.architecture().register_names())

        return self.regs

    def __call__(self, pending_frame):
        """Actually performs the unwind, or at least sniffs this frame
        to see if the unwinder should claim it, which is never does."""
        try:
            for r in self.get_regs(pending_frame):
                v = pending_frame.read_register(r).cast(self.void_ptr_t)
        except:
            print("Dummy unwinder, exception")
            raise

        return None


# Register the ComRV stack unwinder.
gdb.unwinder.register_unwinder(None, dummy_unwinder(), True)

print("Python script imported")
