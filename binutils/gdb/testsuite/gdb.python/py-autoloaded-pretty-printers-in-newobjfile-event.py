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

# This file is part of the GDB testsuite. It tests that python pretty
# printers defined in a python script that is autoloaded have been
# registered when a custom event handler for the new_objfile event
# is called.

import gdb
import os


def new_objfile_handler(event):
    assert isinstance(event, gdb.NewObjFileEvent)
    objfile = event.new_objfile

    # Only observe the custom test library.
    libname = "libpy-autoloaded-pretty-printers-in-newobjfile-event"
    if libname in os.path.basename(objfile.filename):
        # If everything went well and the pretty-printer auto-load happened
        # before notifying the Python listeners, we expect to see one pretty
        # printer, and it must be ours.
        all_good = (
            len(objfile.pretty_printers) == 1
            and objfile.pretty_printers[0].name == "my_library"
        )

        if all_good:
            gdb.parse_and_eval("all_good = 1")
        else:
            print("Oops, not all good:")
            print("pretty printer count: {}".format(len(objfile.pretty_printers)))

            for pp in objfile.pretty_printers:
                print("  - {}".format(pp.name))


gdb.events.new_objfile.connect(new_objfile_handler)
