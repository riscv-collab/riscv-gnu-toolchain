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

import gdb

threadOneExit = ""
threadTwoExit = ""
# we don't want to overwrite the 2nd thread's exit event, thus
# store it here. we don't care about it though.
mainThreadExit = ""


def thread_exited_handler(event):
    global threadOneExit, threadTwoExit, mainThreadExit
    print("{}".format(event))
    assert isinstance(event, gdb.ThreadExitedEvent)
    if threadOneExit == "":
        threadOneExit = "event type: thread-exited. global num: {}".format(
            event.inferior_thread.global_num
        )
    else:
        if threadTwoExit == "":
            threadTwoExit = "event type: thread-exited. global num: {}".format(
                event.inferior_thread.global_num
            )
        else:
            mainThreadExit = "event type: thread-exited. global num: {}".format(
                event.inferior_thread.global_num
            )


class test_events(gdb.Command):
    """Test events."""

    def __init__(self):
        gdb.Command.__init__(self, "test-events", gdb.COMMAND_STACK)

    def invoke(self, arg, from_tty):
        gdb.events.thread_exited.connect(thread_exited_handler)
        print("Event testers registered.")


test_events()
