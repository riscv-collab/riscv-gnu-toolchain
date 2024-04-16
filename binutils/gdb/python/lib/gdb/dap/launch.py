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

# These are deprecated in 3.9, but required in older versions.
from typing import Mapping, Optional, Sequence

from .events import exec_and_expect_stop, expect_process, suppress_stop
from .server import request, capability
from .startup import exec_and_log, DAPException


# The program being launched, or None.  This should only be accessed
# from the gdb thread.
_program = None


# True if the program was attached, False otherwise.  This should only
# be accessed from the gdb thread.
_attach = False


# Any parameters here are necessarily extensions -- DAP requires this
# from implementations.  Any additions or changes here should be
# documented in the gdb manual.
@request("launch", response=False)
def launch(
    *,
    program: Optional[str] = None,
    cwd: Optional[str] = None,
    args: Sequence[str] = (),
    env: Optional[Mapping[str, str]] = None,
    stopAtBeginningOfMainSubprogram: bool = False,
    **extra,
):
    global _program
    _program = program
    global _attach
    _attach = False
    if cwd is not None:
        exec_and_log("cd " + cwd)
    if program is not None:
        exec_and_log("file " + program)
    inf = gdb.selected_inferior()
    if stopAtBeginningOfMainSubprogram:
        main = inf.main_name
        if main is not None:
            exec_and_log("tbreak " + main)
    inf.arguments = args
    if env is not None:
        inf.clear_env()
        for name, value in env.items():
            inf.set_env(name, value)


@request("attach")
def attach(
    *,
    program: Optional[str] = None,
    pid: Optional[int] = None,
    target: Optional[str] = None,
    **args,
):
    # Ensure configurationDone does not try to run.
    global _attach
    _attach = True
    global _program
    _program = program
    if program is not None:
        exec_and_log("file " + program)
    if pid is not None:
        cmd = "attach " + str(pid)
    elif target is not None:
        cmd = "target remote " + target
    else:
        raise DAPException("attach requires either 'pid' or 'target'")
    expect_process("attach")
    suppress_stop()
    exec_and_log(cmd)


@capability("supportsConfigurationDoneRequest")
@request("configurationDone", response=False)
def config_done(**args):
    global _attach
    if not _attach:
        expect_process("process")
        exec_and_expect_stop("run")
