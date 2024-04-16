# Copyright 2022-2024 Free Software Foundation, Inc.

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

import os
import gdb

# This must come before other DAP imports.
from . import startup

# Load modules that define commands.
from . import breakpoint
from . import bt
from . import disassemble
from . import evaluate
from . import launch
from . import locations
from . import memory
from . import modules
from . import next
from . import pause
from . import scopes
from . import sources
from . import threads

from .server import Server


def run():
    """Main entry point for the DAP server.
    This is called by the GDB DAP interpreter."""
    startup.exec_and_log("set python print-stack full")
    startup.exec_and_log("set pagination off")

    # We want to control gdb stdin and stdout entirely, so we dup
    # them to new file descriptors.
    saved_out = os.dup(1)
    saved_in = os.dup(0)
    # Make sure these are not inheritable.  This is already the case
    # for Unix, but not for Windows.
    os.set_inheritable(saved_out, False)
    os.set_inheritable(saved_in, False)

    # The new gdb (and inferior) stdin will just be /dev/null.  For
    # gdb, the "dap" interpreter also rewires the UI so that gdb
    # doesn't try to read this (and thus see EOF and exit).
    new_in = os.open(os.devnull, os.O_RDONLY)
    os.dup2(new_in, 0, True)
    os.close(new_in)

    # Make the new stdout be a pipe.  This way the DAP code can easily
    # read from the inferior and send OutputEvent to the client.
    (rfd, wfd) = os.pipe()
    os.set_inheritable(rfd, False)
    os.dup2(wfd, 1, True)
    # Also send stderr this way.
    os.dup2(wfd, 2, True)
    os.close(wfd)

    # Note the inferior output is opened in text mode.
    server = Server(open(saved_in, "rb"), open(saved_out, "wb"), open(rfd, "r"))
    startup.start_dap(server.main_loop)
