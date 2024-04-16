#!/usr/bin/env python3

# Generate Unicode case-folding table for Ada.

# Copyright (C) 2022-2024 Free Software Foundation, Inc.

# This file is part of GDB.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# This generates the ada-casefold.h header.
# Usage:
#   python ada-unicode.py

import gdbcopyright

# The start of the current range of case-conversions we are
# processing.  If RANGE_START is None, then we're outside of a range.
range_start = None
# End of the current range.
range_end = None
# The delta between RANGE_START and the upper-case variant of that
# character.
upper_delta = None
# The delta between RANGE_START and the lower-case variant of that
# character.
lower_delta = None

# All the ranges found and completed so far.
# Each entry is a tuple of the form (START, END, UPPER_DELTA, LOWER_DELTA).
all_ranges = []


def finish_range():
    global range_start
    global range_end
    global upper_delta
    global lower_delta
    if range_start is not None:
        all_ranges.append((range_start, range_end, upper_delta, lower_delta))
        range_start = None
        range_end = None
        upper_delta = None
        lower_delta = None


def process_codepoint(val):
    global range_start
    global range_end
    global upper_delta
    global lower_delta
    c = chr(val)
    low = c.lower()
    up = c.upper()
    # U+00DF ("LATIN SMALL LETTER SHARP S", aka eszsett) traditionally
    # upper-cases to the two-character string "SS" (the capital form
    # is a relatively recent addition -- 2017).  Our simple scheme
    # can't handle this, so we skip it.  Also, because our approach
    # just represents runs of characters with identical folding
    # deltas, this change must terminate the current run.
    if (c == low and c == up) or len(low) != 1 or len(up) != 1:
        finish_range()
        return
    updelta = ord(up) - val
    lowdelta = ord(low) - val
    if range_start is not None and (updelta != upper_delta or lowdelta != lower_delta):
        finish_range()
    if range_start is None:
        range_start = val
        upper_delta = updelta
        lower_delta = lowdelta
    range_end = val


for c in range(0, 0x10FFFF):
    process_codepoint(c)

with open("ada-casefold.h", "w") as f:
    print(
        gdbcopyright.copyright("ada-unicode.py", "UTF-32 case-folding for GDB"),
        file=f,
    )
    print("", file=f)
    for r in all_ranges:
        print(f"   {{{r[0]}, {r[1]}, {r[2]}, {r[3]}}},", file=f)
