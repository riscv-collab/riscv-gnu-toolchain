# Copyright (C) 2011-2024 Free Software Foundation, Inc.

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

"""Configure GDB using the WRS/Linux environment."""

import os

if "ENV_PREFIX" in os.environ:
    gdb.execute("set sysroot %s" % os.environ["ENV_PREFIX"])

else:
    print("warning: ENV_PREFIX environment variable missing.")
    print("The debugger will probably be unable to find the correct system libraries")
