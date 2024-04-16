# Copyright (C) 2015-2024 Free Software Foundation, Inc.

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


def safe_execute(command):
    """Execute command, ignoring any gdb errors."""
    result = None
    try:
        result = gdb.execute(command, to_string=True)
    except gdb.error:
        pass
    return result


def convert_spaces(file_name):
    """Return file_name with all spaces replaced with "-"."""
    return file_name.replace(" ", "-")


def select_file(file_name):
    """Select a file for debugging.

    N.B. This turns confirmation off.
    """
    safe_execute("set confirm off")
    safe_execute("kill")
    print("Selecting file %s" % (file_name))
    if file_name is None:
        gdb.execute("file")
    else:
        gdb.execute("file %s" % (file_name))


def runto(location):
    """Run the program to location.

    N.B. This turns confirmation off.
    """
    safe_execute("set confirm off")
    gdb.execute("tbreak %s" % (location))
    gdb.execute("run")


def runto_main():
    """Run the program to "main".

    N.B. This turns confirmation off.
    """
    runto("main")


def run_n_times(count, func):
    """Execute func count times."""
    while count > 0:
        func()
        count -= 1
