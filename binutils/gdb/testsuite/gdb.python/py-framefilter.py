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


class Dummy(FrameDecorator):
    def __init__(self, fobj):
        super(Dummy, self).__init__(fobj)
        self.fobj = fobj

    def function(self):
        return "Dummy function"

    def address(self):
        return 0x123

    def filename(self):
        return "Dummy filename"

    def frame_args(self):
        return [("Foo", gdb.Value(12)), ("Bar", "Stuff"), ("FooBar", 42)]

    def frame_locals(self):
        return []

    def line(self):
        return 0

    def elided(self):
        return None


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


class ElidingFrameDecorator(FrameDecorator):
    def __init__(self, frame, elided_frames):
        super(ElidingFrameDecorator, self).__init__(frame)
        self.elided_frames = elided_frames

    def elided(self):
        return iter(self.elided_frames)

    def address(self):
        # Regression test for an overflow in the python layer.
        bitsize = 8 * gdb.lookup_type("void").pointer().sizeof
        mask = (1 << bitsize) - 1
        return 0xFFFFFFFFFFFFFFFF & mask


class ElidingIterator:
    def __init__(self, ii):
        self.input_iterator = ii

    def __iter__(self):
        return self

    def next(self):
        frame = next(self.input_iterator)
        if str(frame.function()) != "func1":
            return frame

        # Suppose we want to return the 'func1' frame but elide the
        # next frame.  E.g., if call in our interpreter language takes
        # two C frames to implement, and the first one we see is the
        # "sentinel".
        elided = next(self.input_iterator)
        return ElidingFrameDecorator(frame, [elided])

    # Python 3.x requires __next__(self) while Python 2.x requires
    # next(self).  Define next(self), and for Python 3.x create this
    # wrapper.
    def __next__(self):
        return self.next()


class FrameElider:
    def __init__(self):
        self.name = "Elider"
        self.priority = 900
        self.enabled = True
        gdb.frame_filters[self.name] = self

    def filter(self, frame_iter):
        return ElidingIterator(frame_iter)


# This is here so the test can change the kind of error that is
# thrown.
name_error = RuntimeError


# A simple decorator that gives an error when computing the function.
class ErrorInName(FrameDecorator):
    def __init__(self, frame):
        FrameDecorator.__init__(self, frame)

    def function(self):
        raise name_error("whoops")


# A filter that supplies buggy frames.  Disabled by default.
class ErrorFilter:
    def __init__(self):
        self.name = "Error"
        self.priority = 1
        self.enabled = False
        gdb.frame_filters[self.name] = self

    def filter(self, frame_iter):
        # Python 3.x moved the itertools.imap functionality to map(),
        # so check if it is available.
        if hasattr(itertools, "imap"):
            return itertools.imap(ErrorInName, frame_iter)
        else:
            return map(ErrorInName, frame_iter)


FrameFilter()
FrameElider()
ErrorFilter()
