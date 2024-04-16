# Copyright (C) 2020-2024 Free Software Foundation, Inc.
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

# A TUI window implemented in Python.

import gdb

the_window = None


class TestWindow:
    def __init__(self, win):
        global the_window
        the_window = self
        self.count = 0
        self.win = win
        win.title = "This Is The Title"

    def render(self):
        self.win.erase()
        w = self.win.width
        h = self.win.height
        self.win.write(
            string="Test: " + str(self.count) + " " + str(w) + "x" + str(h),
            full_window=False,
        )
        self.count = self.count + 1

    # Tries to delete the title attribute.  GDB will throw an error.
    def remove_title(self):
        del self.win.title


gdb.register_window_type("test", TestWindow)


# Call REMOVE_TITLE on the global window object.
def delete_window_title():
    the_window.remove_title()


# A TUI window "constructor" that always fails.
def failwin(win):
    raise RuntimeError("Whoops")


# Change the title of the window.
def change_window_title():
    the_window.win.title = "New Title"


gdb.register_window_type("fail", failwin)
