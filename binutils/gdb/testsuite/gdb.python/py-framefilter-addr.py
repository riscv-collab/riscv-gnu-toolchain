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
import itertools
from gdb.FrameDecorator import FrameDecorator
import copy


# A FrameDecorator that just returns gdb.Frame.pc () from 'function'.
# We want to ensure that GDB correctly handles this case.
class Function_Returns_Address(FrameDecorator):
    def __init__(self, fobj):
        super(Function_Returns_Address, self).__init__(fobj)
        self._fobj = fobj

    def function(self):
        frame = self.inferior_frame()
        return frame.pc()


class Frame_Filter:
    def __init__(self):
        self.name = "function_returns_address"
        self.priority = 100
        self.enabled = True
        gdb.frame_filters[self.name] = self

    def filter(self, frame_iter):
        # Python 3.x moved the itertools.imap functionality to map(),
        # so check if it is available.
        if hasattr(itertools, "imap"):
            frame_iter = itertools.imap(Function_Returns_Address, frame_iter)
        else:
            frame_iter = map(Function_Returns_Address, frame_iter)

        return frame_iter


Frame_Filter()
