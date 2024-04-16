#!/usr/bin/env python

# Copyright (C) 2018-2024 Free Software Foundation, Inc.
#
# This file is part of GDB.
#
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

# This is a simple program that can be used to print timestamps on
# standard output.  The inspiration for it was ts(1)
# (<https://joeyh.name/code/moreutils/>).  We have our own version
# because unfortunately ts(1) is or may not be available on all
# systems that GDB supports.
#
# The usage is simple:
#
#   #> some_command | print-ts.py [FORMAT]
#
# FORMAT must be a string compatible with "strftime".  If nothing is
# provided, we choose a reasonable format.

import fileinput
import datetime
import sys
import os

if len(sys.argv) > 1:
    fmt = sys.argv[1]
else:
    fmt = "[%b %d %H:%M:%S]"

mypid = os.getpid()

for line in fileinput.input("-"):
    sys.stdout.write(
        "{} [{}] {}".format(datetime.datetime.now().strftime(fmt), mypid, line)
    )
    sys.stdout.flush()
