# Copyright (C) 2021-2024 Free Software Foundation, Inc.
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

# A TUI window implemented in Python that responds to, and displays,
# stop and exit events.

import gdb

# When an event arrives we ask the window to redraw itself.  We should
# only do this if the window is valid.  When this flag is true we
# perform the is_valid check.  When this flag is false
perform_valid_check = True
update_title = False
cleanup_properly = False

# A global place into which we can write the window title.
titles_at_the_close = {}


class EventWindow:
    def __init__(self, win):
        self._win = win
        self._count = 0
        win.title = "This Is The Event Window"
        self._stop_listener = lambda e: self._event("stop", e)
        gdb.events.stop.connect(self._stop_listener)
        self._exit_listener = lambda e: self._event("exit", e)
        gdb.events.exited.connect(self._exit_listener)
        self._events = []

        # Ensure we can erase and write to the window from the
        # constructor, the window should be valid by this point.
        self._win.erase()
        self._win.write("Hello world...")

    def close(self):
        global cleanup_properly
        global titles_at_the_close

        # Ensure that window properties can be read within the close method.
        titles_at_the_close[self._win.title] = dict(
            width=self._win.width, height=self._win.height
        )

        # The following calls are pretty pointless, but this ensures
        # that we can erase and write to a window from the close
        # method, the last moment a window should be valid.
        self._win.erase()
        self._win.write("Goodbye cruel world...")

        if cleanup_properly:
            # Disconnect the listeners and delete the lambda functions.
            # This removes cyclic references to SELF, and so alows SELF to
            # be deleted.
            gdb.events.stop.disconnect(self._stop_listener)
            gdb.events.exited.disconnect(self._exit_listener)
            self._stop_listener = None
            self._exit_listener = None

    def _event(self, type, event):
        global perform_valid_check
        global update_title

        self._count += 1
        self._events.insert(0, type)
        if not perform_valid_check or self._win.is_valid():
            if update_title:
                self._win.title = "This Is The Event Window (" + str(self._count) + ")"
            else:
                self.render()

    def render(self):
        self._win.erase()
        w = self._win.width
        h = self._win.height
        for i in range(min(h, len(self._events))):
            self._win.write(self._events[i] + "\n")


gdb.register_window_type("events", EventWindow)
