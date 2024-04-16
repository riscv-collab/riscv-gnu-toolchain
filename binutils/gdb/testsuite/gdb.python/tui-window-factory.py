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


class TestWindow:
    def __init__(self, tui_win, msg):
        self.msg = msg
        self.tui_win = tui_win
        print("Entering TestWindow.__init__: %s" % self.msg)

    def render(self):
        self.tui_win.erase()
        self.tui_win.write("TestWindow (%s)" % self.msg)

    def __del__(self):
        print("Entering TestWindow.__del__: %s" % self.msg)


class TestWindowFactory:
    def __init__(self, msg):
        self.msg = msg
        print("Entering TestWindowFactory.__init__: %s" % self.msg)

    def __call__(self, tui_win):
        print("Entering TestWindowFactory.__call__: %s" % self.msg)
        return TestWindow(tui_win, self.msg)

    def __del__(self):
        print("Entering TestWindowFactory.__del__: %s" % self.msg)


def register_window_factory(msg):
    gdb.register_window_type("test_window", TestWindowFactory(msg))


print("Python script imported")
