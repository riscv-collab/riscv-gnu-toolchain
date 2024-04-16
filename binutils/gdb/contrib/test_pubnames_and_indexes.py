#! /usr/bin/env python3

# Copyright (C) 2011-2024 Free Software Foundation, Inc.
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

# This program requires readelf, gdb and objcopy.  The default values are gdb
# from the build tree and objcopy and readelf from $PATH.  They may be
# overridden by setting environment variables GDB, READELF and OBJCOPY
# respectively.  We assume the current directory is either $obj/gdb or
# $obj/gdb/testsuite.
#
# Example usage:
#
# bash$ cd $objdir/gdb/testsuite
# bash$ python test_pubnames_and_indexes.py <binary_name>

"""test_pubnames_and_indexes.py

Test that the gdb_index produced by gold is identical to the gdb_index
produced by gdb itself.

Further check that the pubnames and pubtypes produced by gcc are identical
to those that gdb produces.

Finally, check that all strings are canonicalized identically.
"""

__author__ = "saugustine@google.com (Sterling Augustine)"

import os
import subprocess
import sys

OBJCOPY = None
READELF = None
GDB = None


def get_pub_info(filename, readelf_option):
    """Parse and return all the pubnames or pubtypes produced by readelf with the
    given option.
    """
    readelf = subprocess.Popen(
        [READELF, "--debug-dump=" + readelf_option, filename], stdout=subprocess.PIPE
    )
    pubnames = []

    in_list = False
    for line in readelf.stdout:
        fields = line.split(None, 1)
        if len(fields) == 2 and fields[0] == "Offset" and fields[1].strip() == "Name":
            in_list = True
        # Either a blank-line or a new Length field terminates the current section.
        elif len(fields) == 0 or fields[0] == "Length:":
            in_list = False
        elif in_list:
            pubnames.append(fields[1].strip())

    readelf.wait()
    return pubnames


def get_gdb_index(filename):
    """Use readelf to dump the gdb index and collect the types and names"""
    readelf = subprocess.Popen(
        [READELF, "--debug-dump=gdb_index", filename], stdout=subprocess.PIPE
    )
    index_symbols = []
    symbol_table_started = False
    for line in readelf.stdout:
        if line == "Symbol table:\n":
            symbol_table_started = True
        elif symbol_table_started:
            # Readelf prints gdb-index lines formatted like so:
            # [  4] two::c2<double>::c2: 0
            # So take the string between the first close bracket and the last colon.
            index_symbols.append(line[line.find("]") + 2 : line.rfind(":")])

    readelf.wait()
    return index_symbols


def CheckSets(list0, list1, name0, name1):
    """Report any setwise differences between the two lists"""

    if len(list0) == 0 or len(list1) == 0:
        return False

    difference0 = set(list0) - set(list1)
    if len(difference0) != 0:
        print("Elements in " + name0 + " but not " + name1 + ": (", end=" ")
        print(len(difference0), end=" ")
        print(")")
        for element in difference0:
            print("  " + element)

    difference1 = set(list1) - set(list0)
    if len(difference1) != 0:
        print("Elements in " + name1 + " but not " + name0 + ": (", end=" ")
        print(len(difference1), end=" ")
        print(")")
        for element in difference1:
            print("  " + element)

    if len(difference0) != 0 or len(difference1) != 0:
        return True

    print(name0 + " and " + name1 + " are identical.")
    return False


def find_executables():
    """Find the copies of readelf, objcopy and gdb to use."""
    # Executable finding logic follows cc-with-index.sh
    global READELF
    READELF = os.getenv("READELF")
    if READELF is None:
        READELF = "readelf"
    global OBJCOPY
    OBJCOPY = os.getenv("OBJCOPY")
    if OBJCOPY is None:
        OBJCOPY = "objcopy"

    global GDB
    GDB = os.getenv("GDB")
    if GDB is None:
        if os.path.isfile("./gdb") and os.access("./gdb", os.X_OK):
            GDB = "./gdb"
        elif os.path.isfile("../gdb") and os.access("../gdb", os.X_OK):
            GDB = "../gdb"
        elif os.path.isfile("../../gdb") and os.access("../../gdb", os.X_OK):
            GDB = "../../gdb"
        else:
            # Punt and use the gdb in the path.
            GDB = "gdb"


def main(argv):
    """The main subprogram."""
    if len(argv) != 2:
        print("Usage: test_pubnames_and_indexes.py <filename>")
        sys.exit(2)

    find_executables()

    # Get the index produced by Gold--It should have been built into the binary.
    gold_index = get_gdb_index(argv[1])

    # Collect the pubnames and types list
    pubs_list = get_pub_info(argv[1], "pubnames")
    pubs_list = pubs_list + get_pub_info(argv[1], "pubtypes")

    # Generate a .gdb_index with gdb
    gdb_index_file = argv[1] + ".gdb-generated-index"
    subprocess.check_call(
        [OBJCOPY, "--remove-section", ".gdb_index", argv[1], gdb_index_file]
    )
    subprocess.check_call(
        [
            GDB,
            "-batch",
            "-nx",
            gdb_index_file,
            "-ex",
            "save gdb-index " + os.path.dirname(argv[1]),
            "-ex",
            "quit",
        ]
    )
    subprocess.check_call(
        [
            OBJCOPY,
            "--add-section",
            ".gdb_index=" + gdb_index_file + ".gdb-index",
            gdb_index_file,
        ]
    )
    gdb_index = get_gdb_index(gdb_index_file)
    os.remove(gdb_index_file)
    os.remove(gdb_index_file + ".gdb-index")

    failed = False
    gdb_index.sort()
    gold_index.sort()
    pubs_list.sort()

    # Find the differences between the various indices.
    if len(gold_index) == 0:
        print("Gold index is empty")
        failed |= True

    if len(gdb_index) == 0:
        print("Gdb index is empty")
        failed |= True

    if len(pubs_list) == 0:
        print("Pubs list is empty")
        failed |= True

    failed |= CheckSets(gdb_index, gold_index, "gdb index", "gold index")
    failed |= CheckSets(pubs_list, gold_index, "pubs list", "gold index")
    failed |= CheckSets(pubs_list, gdb_index, "pubs list", "gdb index")

    if failed:
        print("Test failed")
        sys.exit(1)


if __name__ == "__main__":
    main(sys.argv)
