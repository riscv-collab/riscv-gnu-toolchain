# Copyright 2023-2024 Free Software Foundation, Inc.

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

# Test the type checker.

import typing
from gdb.dap.typecheck import type_check


# A wrapper to call a function that should cause a type-checking
# failure.  Returns if the exception was seen.  Throws an exception if
# a TypeError was not seen.
def should_fail(func, **args):
    try:
        func(**args)
    except TypeError:
        return
    raise RuntimeError("function failed to throw TypeError")


# Also specify a return type to make sure return types do not confuse
# the checker.
@type_check
def simple_types(*, b: bool, s: str, i: int = 23) -> int:
    return i


def check_simple():
    simple_types(b=True, s="DEI", i=97)
    # Check the absence of a defaulted argument.
    simple_types(b=True, s="DEI")
    simple_types(b=False, s="DEI", i=97)
    should_fail(simple_types, b=97, s="DEI", i=97)
    should_fail(simple_types, b=True, s=None, i=97)
    should_fail(simple_types, b=True, s="DEI", i={})


@type_check
def sequence_type(*, s: typing.Sequence[str]):
    pass


def check_sequence():
    sequence_type(s=("hi", "out", "there"))
    sequence_type(s=["hi", "out", "there"])
    sequence_type(s=())
    sequence_type(s=[])
    should_fail(sequence_type, s=23)
    should_fail(sequence_type, s=["hi", 97])


@type_check
def map_type(*, m: typing.Mapping[str, int]):
    pass


def check_map():
    map_type(m={})
    map_type(m={"dei": 23})
    should_fail(map_type, m=[1, 2, 3])
    should_fail(map_type, m=None)
    should_fail(map_type, m={"dei": "string"})


@type_check
def opt_type(*, u: typing.Optional[int]):
    pass


def check_opt():
    opt_type(u=23)
    opt_type(u=None)
    should_fail(opt_type, u="string")


def check_everything():
    # Older versions of Python can't really implement this.
    if hasattr(typing, "get_origin"):
        # Just let any exception propagate and end up in the log.
        check_simple()
        check_sequence()
        check_map()
        check_opt()
        print("OK")
    else:
        print("UNSUPPORTED")
