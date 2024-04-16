# Copyright (C) 2023-2024 Free Software Foundation, Inc.

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

from gdb.FrameDecorator import DAPFrameDecorator


class ElidingFrameDecorator(DAPFrameDecorator):
    def __init__(self, frame, elided_frames):
        super(ElidingFrameDecorator, self).__init__(frame)
        self.elided_frames = elided_frames

    def elided(self):
        return iter(self.elided_frames)


class ElidingIterator:
    def __init__(self, ii):
        self.input_iterator = ii

    def __iter__(self):
        return self

    def __next__(self):
        frame = next(self.input_iterator)
        if str(frame.function()) != "function":
            return frame

        # Elide the next three frames.
        elided = []
        elided.append(next(self.input_iterator))
        elided.append(next(self.input_iterator))
        elided.append(next(self.input_iterator))

        return ElidingFrameDecorator(frame, elided)


class FrameElider:
    def __init__(self):
        self.name = "Elider"
        self.priority = 900
        self.enabled = True
        gdb.frame_filters[self.name] = self

    def filter(self, frame_iter):
        return ElidingIterator(frame_iter)


FrameElider()
