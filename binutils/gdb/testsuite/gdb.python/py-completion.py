# Copyright (C) 2014-2024 Free Software Foundation, Inc.

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

# This testcase tests PR python/16699

import gdb


class CompleteFileInit(gdb.Command):
    def __init__(self):
        gdb.Command.__init__(
            self, "completefileinit", gdb.COMMAND_USER, gdb.COMPLETE_FILENAME
        )

    def invoke(self, argument, from_tty):
        raise gdb.GdbError("not implemented")


class CompleteFileNone(gdb.Command):
    def __init__(self):
        gdb.Command.__init__(self, "completefilenone", gdb.COMMAND_USER)

    def invoke(self, argument, from_tty):
        raise gdb.GdbError("not implemented")

    def complete(self, text, word):
        return None


class CompleteFileMethod(gdb.Command):
    def __init__(self):
        gdb.Command.__init__(self, "completefilemethod", gdb.COMMAND_USER)

    def invoke(self, argument, from_tty):
        raise gdb.GdbError("not implemented")

    def complete(self, text, word):
        return gdb.COMPLETE_FILENAME


class CompleteFileCommandCond(gdb.Command):
    def __init__(self):
        gdb.Command.__init__(self, "completefilecommandcond", gdb.COMMAND_USER)

    def invoke(self, argument, from_tty):
        raise gdb.GdbError("not implemented")

    def complete(self, text, word):
        # This is a test made to know if the command
        # completion still works fine.  When the user asks to
        # complete something like "completefilecommandcond
        # /path/to/py-completion-t", it should not complete to
        # "/path/to/py-completion-test/", but instead just
        # wait for input.
        if "py-completion-t" in text:
            return gdb.COMPLETE_COMMAND
        else:
            return gdb.COMPLETE_FILENAME


class CompleteLimit1(gdb.Command):
    def __init__(self):
        gdb.Command.__init__(self, "completelimit1", gdb.COMMAND_USER)

    def invoke(self, argument, from_tty):
        raise gdb.GdbError("not implemented")

    def complete(self, text, word):
        return ["cl11", "cl12", "cl13"]


class CompleteLimit2(gdb.Command):
    def __init__(self):
        gdb.Command.__init__(self, "completelimit2", gdb.COMMAND_USER)

    def invoke(self, argument, from_tty):
        raise gdb.GdbError("not implemented")

    def complete(self, text, word):
        return [
            "cl21",
            "cl23",
            "cl25",
            "cl27",
            "cl29",
            "cl22",
            "cl24",
            "cl26",
            "cl28",
            "cl210",
        ]


class CompleteLimit3(gdb.Command):
    def __init__(self):
        gdb.Command.__init__(self, "completelimit3", gdb.COMMAND_USER)

    def invoke(self, argument, from_tty):
        raise gdb.GdbError("not implemented")

    def complete(self, text, word):
        return [
            "cl31",
            "cl33",
            "cl35",
            "cl37",
            "cl39",
            "cl32",
            "cl34",
            "cl36",
            "cl38",
            "cl310",
        ]


class CompleteLimit4(gdb.Command):
    def __init__(self):
        gdb.Command.__init__(self, "completelimit4", gdb.COMMAND_USER)

    def invoke(self, argument, from_tty):
        raise gdb.GdbError("not implemented")

    def complete(self, text, word):
        return [
            "cl41",
            "cl43",
            "cl45",
            "cl47",
            "cl49",
            "cl42",
            "cl44",
            "cl46",
            "cl48",
            "cl410",
        ]


class CompleteLimit5(gdb.Command):
    def __init__(self):
        gdb.Command.__init__(self, "completelimit5", gdb.COMMAND_USER)

    def invoke(self, argument, from_tty):
        raise gdb.GdbError("not implemented")

    def complete(self, text, word):
        return [
            "cl51",
            "cl53",
            "cl55",
            "cl57",
            "cl59",
            "cl52",
            "cl54",
            "cl56",
            "cl58",
            "cl510",
        ]


class CompleteLimit6(gdb.Command):
    def __init__(self):
        gdb.Command.__init__(self, "completelimit6", gdb.COMMAND_USER)

    def invoke(self, argument, from_tty):
        raise gdb.GdbError("not implemented")

    def complete(self, text, word):
        return [
            "cl61",
            "cl63",
            "cl65",
            "cl67",
            "cl69",
            "cl62",
            "cl64",
            "cl66",
            "cl68",
            "cl610",
        ]


class CompleteLimit7(gdb.Command):
    def __init__(self):
        gdb.Command.__init__(self, "completelimit7", gdb.COMMAND_USER)

    def invoke(self, argument, from_tty):
        raise gdb.GdbError("not implemented")

    def complete(self, text, word):
        return [
            "cl71",
            "cl73",
            "cl75",
            "cl77",
            "cl79",
            "cl72",
            "cl74",
            "cl76",
            "cl78",
            "cl710",
        ]


class CompleteBrkCharException(gdb.Command):
    def __init__(self):
        gdb.Command.__init__(self, "complete_brkchar_exception", gdb.COMMAND_USER)

    def invoke(self, argument, from_tty):
        raise gdb.GdbError("not implemented")

    def complete(self, text, word):
        if word is None:
            raise gdb.GdbError("brkchars exception")
        else:
            raise gdb.GdbError("completion exception")


class CompleteRaiseException(gdb.Command):
    def __init__(self):
        gdb.Command.__init__(self, "complete_raise_exception", gdb.COMMAND_USER)

    def invoke(self, argument, from_tty):
        raise gdb.GdbError("not implemented")

    def complete(self, text, word):
        if word is None:
            return []
        else:
            raise gdb.GdbError("completion exception")


CompleteFileInit()
CompleteFileNone()
CompleteFileMethod()
CompleteFileCommandCond()
CompleteLimit1()
CompleteLimit2()
CompleteLimit3()
CompleteLimit4()
CompleteLimit5()
CompleteLimit6()
CompleteLimit7()
CompleteBrkCharException()
CompleteRaiseException()
