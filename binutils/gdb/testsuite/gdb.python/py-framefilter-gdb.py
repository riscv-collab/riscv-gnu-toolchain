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


class FrameObjFile:
    def __init__(self):
        self.name = "Filter1"
        self.priority = 1
        self.enabled = False
        gdb.current_progspace().frame_filters["Progspace" + self.name] = self
        gdb.current_objfile().frame_filters["ObjectFile" + self.name] = self

    def filter(self, frame_iter):
        return frame_iter


class FrameObjFile2:
    def __init__(self):
        self.name = "Filter2"
        self.priority = 100
        self.enabled = True
        gdb.current_progspace().frame_filters["Progspace" + self.name] = self
        gdb.current_objfile().frame_filters["ObjectFile" + self.name] = self

    def filter(self, frame_iter):
        return frame_iter


FrameObjFile()
FrameObjFile2()
