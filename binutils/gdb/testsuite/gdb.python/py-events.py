# Copyright (C) 2010-2024 Free Software Foundation, Inc.

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

# This file is part of the GDB testsuite.  It tests python pretty
# printers.
import gdb


def signal_stop_handler(event):
    if isinstance(event, gdb.StopEvent):
        print("event type: stop")
    if isinstance(event, gdb.SignalEvent):
        print("stop reason: signal")
        print("stop signal: %s" % (event.stop_signal))
        if event.inferior_thread is not None:
            print("thread num: %s" % (event.inferior_thread.num))


def breakpoint_stop_handler(event):
    if isinstance(event, gdb.StopEvent):
        print("event type: stop")
    if isinstance(event, gdb.BreakpointEvent):
        print("stop reason: breakpoint")
        print("first breakpoint number: %s" % (event.breakpoint.number))
        for bp in event.breakpoints:
            print("breakpoint number: %s" % (bp.number))
        if event.inferior_thread is not None:
            print("thread num: %s" % (event.inferior_thread.num))
        else:
            print("all threads stopped")


def exit_handler(event):
    assert isinstance(event, gdb.ExitedEvent)
    print("event type: exit")
    if hasattr(event, "exit_code"):
        print("exit code: %d" % (event.exit_code))
    else:
        print("exit code: not-present")
    print("exit inf: %d" % (event.inferior.num))
    print("exit pid: %d" % (event.inferior.pid))
    print("dir ok: %s" % str("exit_code" in dir(event)))


def continue_handler(event):
    assert isinstance(event, gdb.ContinueEvent)
    print("event type: continue")
    if event.inferior_thread is not None:
        print("thread num: %s" % (event.inferior_thread.num))


def new_objfile_handler(event):
    assert isinstance(event, gdb.NewObjFileEvent)
    print("event type: new_objfile")
    print("new objfile name: %s" % (event.new_objfile.filename))


def clear_objfiles_handler(event):
    assert isinstance(event, gdb.ClearObjFilesEvent)
    print("event type: clear_objfiles")
    print("progspace: %s" % (event.progspace.filename))


def inferior_call_handler(event):
    if isinstance(event, gdb.InferiorCallPreEvent):
        print("event type: pre-call")
    elif isinstance(event, gdb.InferiorCallPostEvent):
        print("event type: post-call")
    else:
        assert False
    print("ptid: %s" % (event.ptid,))
    print("address: 0x%x" % (event.address))


def register_changed_handler(event):
    assert isinstance(event, gdb.RegisterChangedEvent)
    print("event type: register-changed")
    assert isinstance(event.frame, gdb.Frame)
    print("frame: %s" % (event.frame))
    print("num: %s" % (event.regnum))


def memory_changed_handler(event):
    assert isinstance(event, gdb.MemoryChangedEvent)
    print("event type: memory-changed")
    print("address: %s" % (event.address))
    print("length: %s" % (event.length))


class test_events(gdb.Command):
    """Test events."""

    def __init__(self):
        gdb.Command.__init__(self, "test-events", gdb.COMMAND_STACK)

    def invoke(self, arg, from_tty):
        gdb.events.stop.connect(signal_stop_handler)
        gdb.events.stop.connect(breakpoint_stop_handler)
        gdb.events.exited.connect(exit_handler)
        gdb.events.cont.connect(continue_handler)
        gdb.events.inferior_call.connect(inferior_call_handler)
        gdb.events.memory_changed.connect(memory_changed_handler)
        gdb.events.register_changed.connect(register_changed_handler)
        print("Event testers registered.")


test_events()


class test_newobj_events(gdb.Command):
    """NewObj events."""

    def __init__(self):
        gdb.Command.__init__(self, "test-objfile-events", gdb.COMMAND_STACK)

    def invoke(self, arg, from_tty):
        gdb.events.new_objfile.connect(new_objfile_handler)
        gdb.events.clear_objfiles.connect(clear_objfiles_handler)
        print("Object file events registered.")


test_newobj_events()


def gdb_exiting_handler(event, throw_error):
    assert isinstance(event, gdb.GdbExitingEvent)
    if throw_error:
        raise gdb.GdbError("error from gdb_exiting_handler")
    else:
        print("event type: gdb-exiting")
        print("exit code: %d" % (event.exit_code))


class test_exiting_event(gdb.Command):
    """GDB Exiting event."""

    def __init__(self):
        gdb.Command.__init__(self, "test-exiting-event", gdb.COMMAND_STACK)

    def invoke(self, arg, from_tty):
        if arg == "normal":
            gdb.events.gdb_exiting.connect(lambda e: gdb_exiting_handler(e, False))
        elif arg == "error":
            gdb.events.gdb_exiting.connect(lambda e: gdb_exiting_handler(e, True))
        else:
            raise gdb.GdbError("invalid or missing argument")
        print("GDB exiting event registered.")


test_exiting_event()
