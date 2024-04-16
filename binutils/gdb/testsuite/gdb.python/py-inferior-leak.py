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

import tracemalloc
import gdb
import re

# A global variable in which we store a reference to the gdb.Inferior
# object sent to us in the new_inferior event.
inf = None


# Register the new_inferior event handler.
def new_inferior_handler(event):
    global inf
    inf = event.inferior


gdb.events.new_inferior.connect(new_inferior_handler)

# A global filters list, we only care about memory allocations
# originating from this script.
filters = [tracemalloc.Filter(True, "*py-inferior-leak.py")]


# Add a new inferior, and return the number of the new inferior.
def add_inferior():
    output = gdb.execute("add-inferior", False, True)
    m = re.search(r"Added inferior (\d+)", output)
    if m:
        num = int(m.group(1))
    else:
        raise RuntimeError("no match")
    return num


# Run the test.  When CLEAR is True we clear the global INF variable
# before comparing the before and after memory allocation traces.
# When CLEAR is False we leave INF set to reference the gdb.Inferior
# object, thus preventing the gdb.Inferior from being deallocated.
def test(clear):
    global filters, inf

    # Start tracing, and take a snapshot of the current allocations.
    tracemalloc.start()
    snapshot1 = tracemalloc.take_snapshot()

    # Create an inferior, this triggers the new_inferior event, which
    # in turn holds a reference to the new gdb.Inferior object in the
    # global INF variable.
    num = add_inferior()
    gdb.execute("remove-inferiors %s" % num)

    # Possibly clear the global INF variable.
    if clear:
        inf = None

    # Now grab a second snapshot of memory allocations, and stop
    # tracing memory allocations.
    snapshot2 = tracemalloc.take_snapshot()
    tracemalloc.stop()

    # Filter the snapshots; we only care about allocations originating
    # from this file.
    snapshot1 = snapshot1.filter_traces(filters)
    snapshot2 = snapshot2.filter_traces(filters)

    # Compare the snapshots, this leaves only things that were
    # allocated, but not deallocated since the first snapshot.
    stats = snapshot2.compare_to(snapshot1, "traceback")

    # Total up all the deallocated things.
    total = 0
    for stat in stats:
        total += stat.size_diff
    return total


# The first time we run this some global state will be allocated which
# shows up as memory that is allocated, but not released.  So, run the
# test once and discard the result.
test(True)

# Now run the test twice, the first time we clear our global reference
# to the gdb.Inferior object, which should allow Python to deallocate
# the object.  The second time we hold onto the global reference,
# preventing Python from performing the deallocation.
bytes_with_clear = test(True)
bytes_without_clear = test(False)

# The bug that used to exist in GDB was that even when we released the
# global reference the gdb.Inferior object would not be deallocated.
if bytes_with_clear > 0:
    raise gdb.GdbError("memory leak when gdb.Inferior should be released")
if bytes_without_clear == 0:
    raise gdb.GdbError("gdb.Inferior object is no longer allocated")

# Print a PASS message that the test script can see.
print("PASS")
