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
from gdb.missing_debug import MissingDebugHandler
from enum import Enum
import os

# A global log that is filled in by instances of the LOG_HANDLER class
# when they are called.
handler_call_log = []


class Mode(Enum):
    RETURN_NONE = 0
    RETURN_TRUE = 1
    RETURN_FALSE = 2
    RETURN_STRING = 3


class handler(MissingDebugHandler):
    def __init__(self):
        super().__init__("handler")
        self._call_count = 0
        self._mode = Mode.RETURN_NONE

    def __call__(self, objfile):
        global handler_call_log
        handler_call_log.append(self.name)
        self._call_count += 1
        if self._mode == Mode.RETURN_NONE:
            return None

        if self._mode == Mode.RETURN_TRUE:
            os.rename(self._src, self._dest)
            return True

        if self._mode == Mode.RETURN_FALSE:
            return False

        if self._mode == Mode.RETURN_STRING:
            return self._dest

        assert False

    @property
    def call_count(self):
        """Return a count, the number of calls to __call__ since the last
        call to set_mode.
        """
        return self._call_count

    def set_mode(self, mode, *args):
        self._call_count = 0
        self._mode = mode

        if mode == Mode.RETURN_NONE:
            assert len(args) == 0
            return

        if mode == Mode.RETURN_TRUE:
            assert len(args) == 2
            self._src = args[0]
            self._dest = args[1]
            return

        if mode == Mode.RETURN_FALSE:
            assert len(args) == 0
            return

        if mode == Mode.RETURN_STRING:
            assert len(args) == 1
            self._dest = args[0]
            return

        assert False


class exception_handler(MissingDebugHandler):
    def __init__(self):
        super().__init__("exception_handler")
        self.exception_type = None

    def __call__(self, objfile):
        global handler_call_log
        handler_call_log.append(self.name)
        assert self.exception_type is not None
        raise self.exception_type("message")


class log_handler(MissingDebugHandler):
    def __call__(self, objfile):
        global handler_call_log
        handler_call_log.append(self.name)
        return None


# A basic helper function, this keeps lines shorter in the TCL script.
def register(name, locus=None):
    gdb.missing_debug.register_handler(locus, log_handler(name))


# Create instances of the handlers, but don't install any.  We install
# these as needed from the TCL script.
rhandler = exception_handler()
handler_obj = handler()

print("Success")
