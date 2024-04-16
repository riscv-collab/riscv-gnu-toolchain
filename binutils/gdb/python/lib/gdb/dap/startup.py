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

# Do not import other gdbdap modules here -- this module must come
# first.
import functools
import gdb
import queue
import threading
import traceback
import sys

from enum import IntEnum, auto

# Adapt to different Queue types.  This is exported for use in other
# modules as well.
if sys.version_info[0] == 3 and sys.version_info[1] <= 6:
    DAPQueue = queue.Queue
else:
    DAPQueue = queue.SimpleQueue


# The GDB thread, aka the main thread.
_gdb_thread = threading.current_thread()


# The DAP thread.
_dap_thread = None


# "Known" exceptions are wrapped in a DAP exception, so that, by
# default, only rogue exceptions are logged -- this is then used by
# the test suite.
class DAPException(Exception):
    pass


# Wrapper for gdb.parse_and_eval that turns exceptions into
# DAPException.
def parse_and_eval(expression, global_context=False):
    try:
        return gdb.parse_and_eval(expression, global_context=global_context)
    except Exception as e:
        # Be sure to preserve the summary, as this can propagate to
        # the client.
        raise DAPException(str(e)) from e


def start_thread(name, target, args=()):
    """Start a new thread, invoking TARGET with *ARGS there.
    This is a helper function that ensures that any GDB signals are
    correctly blocked."""
    result = gdb.Thread(name=name, target=target, args=args, daemon=True)
    result.start()


def start_dap(target):
    """Start the DAP thread and invoke TARGET there."""
    exec_and_log("set breakpoint pending on")

    # Functions in this thread contain assertions that check for this
    # global, so we must set it before letting these functions run.
    def really_start_dap():
        global _dap_thread
        _dap_thread = threading.current_thread()
        target()

    start_thread("DAP", really_start_dap)


def in_gdb_thread(func):
    """A decorator that asserts that FUNC must be run in the GDB thread."""

    @functools.wraps(func)
    def ensure_gdb_thread(*args, **kwargs):
        assert threading.current_thread() is _gdb_thread
        return func(*args, **kwargs)

    return ensure_gdb_thread


def in_dap_thread(func):
    """A decorator that asserts that FUNC must be run in the DAP thread."""

    @functools.wraps(func)
    def ensure_dap_thread(*args, **kwargs):
        assert threading.current_thread() is _dap_thread
        return func(*args, **kwargs)

    return ensure_dap_thread


# Logging levels.
class LogLevel(IntEnum):
    DEFAULT = auto()
    FULL = auto()


class LogLevelParam(gdb.Parameter):
    """DAP logging level."""

    set_doc = "Set the DAP logging level."
    show_doc = "Show the DAP logging level."

    def __init__(self):
        super().__init__(
            "debug dap-log-level", gdb.COMMAND_MAINTENANCE, gdb.PARAM_ZUINTEGER
        )
        self.value = LogLevel.DEFAULT


_log_level = LogLevelParam()


class LoggingParam(gdb.Parameter):
    """Whether DAP logging is enabled."""

    set_doc = "Set the DAP logging status."
    show_doc = "Show the DAP logging status."

    log_file = None

    def __init__(self):
        super().__init__(
            "debug dap-log-file", gdb.COMMAND_MAINTENANCE, gdb.PARAM_OPTIONAL_FILENAME
        )
        self.value = None

    def get_set_string(self):
        # Close any existing log file, no matter what.
        if self.log_file is not None:
            self.log_file.close()
            self.log_file = None
        if self.value is not None:
            self.log_file = open(self.value, "w")
        return ""


dap_log = LoggingParam()


def log(something, level=LogLevel.DEFAULT):
    """Log SOMETHING to the log file, if logging is enabled."""
    if dap_log.log_file is not None and level <= _log_level.value:
        print(something, file=dap_log.log_file)
        dap_log.log_file.flush()


def log_stack(level=LogLevel.DEFAULT):
    """Log a stack trace to the log file, if logging is enabled."""
    if dap_log.log_file is not None and level <= _log_level.value:
        traceback.print_exc(file=dap_log.log_file)


@in_gdb_thread
def exec_and_log(cmd):
    """Execute the gdb command CMD.
    If logging is enabled, log the command and its output."""
    log("+++ " + cmd)
    try:
        output = gdb.execute(cmd, from_tty=True, to_string=True)
        if output != "":
            log(">>> " + output)
    except gdb.error:
        log_stack()


class Invoker(object):
    """A simple class that can invoke a gdb command."""

    def __init__(self, cmd):
        self.cmd = cmd

    # This is invoked in the gdb thread to run the command.
    @in_gdb_thread
    def __call__(self):
        exec_and_log(self.cmd)


def send_gdb(cmd):
    """Send CMD to the gdb thread.
    CMD can be either a function or a string.
    If it is a string, it is passed to gdb.execute."""
    if isinstance(cmd, str):
        cmd = Invoker(cmd)
    gdb.post_event(cmd)


def send_gdb_with_response(fn):
    """Send FN to the gdb thread and return its result.
    If FN is a string, it is passed to gdb.execute and None is
    returned as the result.
    If FN throws an exception, this function will throw the
    same exception in the calling thread.
    """
    if isinstance(fn, str):
        fn = Invoker(fn)
    result_q = DAPQueue()

    def message():
        try:
            val = fn()
            result_q.put(val)
        except (Exception, KeyboardInterrupt) as e:
            result_q.put(e)

    send_gdb(message)
    val = result_q.get()
    if isinstance(val, (Exception, KeyboardInterrupt)):
        raise val
    return val
