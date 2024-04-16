# Copyright (C) 2019-2024 Free Software Foundation, Inc.
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


class BadKey:
    def __repr__(self):
        return "Bad Key"


class ReallyBadKey:
    def __repr__(self):
        return BadKey()


class pycmd1(gdb.MICommand):
    def invoke(self, argv):
        if argv[0] == "int":
            return {"result": 42}
        elif argv[0] == "str":
            return {"result": "Hello world!"}
        elif argv[0] == "ary":
            return {"result": ["Hello", 42]}
        elif argv[0] == "dct":
            return {"result": {"hello": "world", "times": 42}}
        elif argv[0] == "bk1":
            return {"result": {BadKey(): "world"}}
        elif argv[0] == "bk2":
            return {"result": {1: "world"}}
        elif argv[0] == "bk3":
            return {"result": {ReallyBadKey(): "world"}}
        elif argv[0] == "tpl":
            return {"result": (42, "Hello")}
        elif argv[0] == "itr":
            return {"result": iter([1, 2, 3])}
        elif argv[0] == "nn1":
            return None
        elif argv[0] == "nn2":
            return {"result": [None]}
        elif argv[0] == "red":
            pycmd2("-pycmd")
            return None
        elif argv[0] == "nd1":
            return [1, 2, 3]
        elif argv[0] == "nd2":
            return 123
        elif argv[0] == "nd3":
            return "abc"
        elif argv[0] == "ik1":
            return {"xxx yyy": 123}
        elif argv[0] == "ik2":
            return {"result": {"xxx yyy": 123}}
        elif argv[0] == "ik3":
            return {"xxx+yyy": 123}
        elif argv[0] == "ik4":
            return {"xxx.yyy": 123}
        elif argv[0] == "ik5":
            return {"123xxxyyy": 123}
        elif argv[0] == "empty_key":
            return {"": 123}
        elif argv[0] == "dash-key":
            return {"the-key": 123}
        elif argv[0] == "exp":
            raise gdb.GdbError()
        else:
            raise gdb.GdbError("Invalid parameter: %s" % argv[0])


class pycmd2(gdb.MICommand):
    def invoke(self, argv):
        if argv[0] == "str":
            return {"result": "Ciao!"}
        elif argv[0] == "red":
            pycmd1("-pycmd")
            raise gdb.GdbError("Command redefined but we failing anyway")
        elif argv[0] == "new":
            pycmd1("-pycmd-new")
            return None
        else:
            raise gdb.GdbError("Invalid parameter: %s" % argv[0])


# This class creates a command that returns a string, which is passed
# when the command is created.
class pycmd3(gdb.MICommand):
    def __init__(self, name, msg, top_level):
        super(pycmd3, self).__init__(name)
        self._msg = msg
        self._top_level = top_level

    def invoke(self, args):
        return {self._top_level: {"msg": self._msg}}


# A command that is missing it's invoke method.
class no_invoke(gdb.MICommand):
    def __init__(self, name):
        super(no_invoke, self).__init__(name)


def free_invoke(obj, args):
    return {"result": args}


# Run some test involving catching exceptions.  It's easier to write
# these as a Python function which is then called from the exp script.
def run_exception_tests():
    print("PASS")


# Run some execute_mi tests.  This is easier to do from Python.
def run_execute_mi_tests():
    # Install the command.
    cmd = pycmd1("-pycmd")
    # Pass in a representative subset of the pycmd1 keys, and then
    # check that the result via MI is the same as the result via a
    # direct Python call.  Note that some results won't compare as
    # equal -- for example, a Python MI command can return a tuple,
    # but that will be translated to a Python list.
    for name in ("int", "str", "dct"):
        expect = cmd.invoke([name])
        got = gdb.execute_mi("-pycmd", name)
        if expect != got:
            print("FAIL: saw " + repr(got) + ", but expected " + repr(expect))
            return
    ok = False
    try:
        gdb.execute_mi("-pycmd", "exp")
    # Due to the "denaturation" problem, we have to expect a gdb.error
    # here and not a gdb.GdbError.
    except gdb.error:
        ok = True
    if not ok:
        print("FAIL: did not throw exception")
    print("PASS")
