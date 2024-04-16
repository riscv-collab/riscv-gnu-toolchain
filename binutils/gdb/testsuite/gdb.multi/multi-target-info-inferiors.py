# Copyright (C) 2021-2024 Free Software Foundation, Inc.

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


# Take a gdb.TargetConnection and return the connection number.
def conn_num(c):
    return c.num


# Takes a gdb.TargetConnection and return a string that is either the
# type, or the type and details (if the details are not None).
def make_target_connection_string(c):
    if c.details is None:
        return c.type
    else:
        return "%s %s" % (c.type, c.details)


# A Python implementation of 'info connections'.  Produce output that
# is identical to the output of 'info connections' so we can check
# that aspects of gdb.TargetConnection work correctly.
def info_connections():
    all_connections = sorted(gdb.connections(), key=conn_num)
    current_conn = gdb.selected_inferior().connection
    what_width = 0
    for c in all_connections:
        s = make_target_connection_string(c)
        if len(s) > what_width:
            what_width = len(s)

    fmt = "  Num  %%-%ds  Description" % what_width
    print(fmt % "What")
    fmt = "%%s%%-3d  %%-%ds  %%s" % what_width
    for c in all_connections:
        if c == current_conn:
            prefix = "* "
        else:
            prefix = "  "

        print(fmt % (prefix, c.num, make_target_connection_string(c), c.description))


def inf_num(i):
    return i.num


# Print information about each inferior, and the connection it is
# using.
def info_inferiors():
    all_inferiors = sorted(gdb.inferiors(), key=inf_num)
    for i in gdb.inferiors():
        print(
            "Inferior %d, Connection #%d: %s"
            % (i.num, i.connection_num, make_target_connection_string(i.connection))
        )
