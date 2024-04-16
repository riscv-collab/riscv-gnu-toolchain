# Copyright (C) 2014-2024 Free Software Foundation, Inc.

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

# This file is part of the GDB testsuite.  It tests Python-based
# frame-filters.
import gdb
import itertools
from gdb.FrameDecorator import FrameDecorator
import copy


class Reverse_Function(FrameDecorator):
    def __init__(self, fobj):
        super(Reverse_Function, self).__init__(fobj)
        self.fobj = fobj

    def function(self):
        fname = str(self.fobj.function())
        if not fname:
            return None
        if fname == "end_func":
            extra = self.fobj.inferior_frame().read_var("str").string()
        else:
            extra = ""
        fname = fname[::-1] + extra
        return fname


class FrameFilter:
    def __init__(self):
        self.name = "Reverse"
        self.priority = 100
        self.enabled = True
        gdb.frame_filters[self.name] = self

    def filter(self, frame_iter):
        # Python 3.x moved the itertools.imap functionality to map(),
        # so check if it is available.
        if hasattr(itertools, "imap"):
            frame_iter = itertools.imap(Reverse_Function, frame_iter)
        else:
            frame_iter = map(Reverse_Function, frame_iter)

        return frame_iter


FrameFilter()
