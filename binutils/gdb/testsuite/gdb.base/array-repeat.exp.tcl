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

# Test the detection and printing of repeated elements in C/C++ arrays.

standard_testfile ${srcdir}/gdb.base/array-repeat.c

if {[prepare_for_testing ${testfile}.exp ${testfile} ${srcfile} \
	[list debug ${lang}]]} {
    return -1
}

if {![runto_main]} {
    perror "Could not run to main."
    continue
}

gdb_breakpoint [gdb_get_line_number "Break here"]
gdb_continue_to_breakpoint "Break here"

# Build up the expected output for each array.
set a9p9o "{9, 9, 9, 9, 9, 9}"
set a1p   "{1, 1, 1, 1, 1}"
set a1p9  "{1, 1, 1, 1, 1, 9}"
set a2po  "{2, 2, 2, 2, 2}"
set a2p   "{${a2po}, ${a2po}, ${a2po}, ${a2po}, ${a2po}}"
set a2p9o "{2, 2, 2, 2, 2, 9}"
set a2p9  "{${a2p9o}, ${a2p9o}, ${a2p9o}, ${a2p9o}, ${a2p9o}, ${a9p9o}}"
set a3po  "{3, 3, 3, 3, 3}"
set a3p   "{${a3po}, ${a3po}, ${a3po}, ${a3po}, ${a3po}}"
set a3p   "{${a3p}, ${a3p}, ${a3p}, ${a3p}, ${a3p}}"
set a3p9o "{3, 3, 3, 3, 3, 9}"
set a3p9  "{${a3p9o}, ${a3p9o}, ${a3p9o}, ${a3p9o}, ${a3p9o}, ${a9p9o}}"
set a9p9  "{${a9p9o}, ${a9p9o}, ${a9p9o}, ${a9p9o}, ${a9p9o}, ${a9p9o}}"
set a3p9  "{${a3p9}, ${a3p9}, ${a3p9}, ${a3p9}, ${a3p9}, ${a9p9}}"

# Convert the output into a regexp.
set r1p   [string_to_regexp $a1p]
set r1p9  [string_to_regexp $a1p9]
set r2po  [string_to_regexp $a2po]
set r2p9o [string_to_regexp $a2p9o]
set r2p   [string_to_regexp $a2p]
set r2p9  [string_to_regexp $a2p9]
set r3po  [string_to_regexp $a3po]
set r3p9o [string_to_regexp $a3p9o]
set r3p   [string_to_regexp $a3p]
set r3p9  [string_to_regexp $a3p9]

set rep5  "<repeats 5 times>"
set rep6  "<repeats 6 times>"

with_test_prefix "repeats=unlimited, elements=unlimited" {
    # Check the arrays print as expected.
    gdb_test_no_output "set print repeats unlimited"
    gdb_test_no_output "set print elements unlimited"

    gdb_test "print array_1d"  "${r1p}"
    gdb_test "print array_1d9" "${r1p9}"
    gdb_test "print array_2d"  "${r2p}"
    gdb_test "print array_2d9" "${r2p9}"
    gdb_test "print array_3d"  "${r3p}"
    gdb_test "print array_3d9" "${r3p9}"
}

with_test_prefix "repeats=4, elements=unlimited" {
    # Now set the repeat limit.
    gdb_test_no_output "set print repeats 4"
    gdb_test_no_output "set print elements unlimited"

    gdb_test "print array_1d" \
	[string_to_regexp "{1 ${rep5}}"]
    gdb_test "print array_1d9" \
	[string_to_regexp "{1 ${rep5}, 9}"]
    gdb_test "print array_2d" \
	[string_to_regexp "{{2 ${rep5}} ${rep5}}"]
    gdb_test "print array_2d9" \
	[string_to_regexp "{{2 ${rep5}, 9} ${rep5}, {9 ${rep6}}}"]
    gdb_test "print array_3d" \
	[string_to_regexp "{{{3 ${rep5}} ${rep5}} ${rep5}}"]
    gdb_test "print array_3d9" \
	[string_to_regexp "{{{3 ${rep5}, 9} ${rep5}, {9 ${rep6}}} ${rep5},\
			    {{9 ${rep6}} ${rep6}}}"]
}

with_test_prefix "repeats=unlimited, elements=3" {
    # Now set the element limit.
    gdb_test_no_output "set print repeats unlimited"
    gdb_test_no_output "set print elements 3"

    gdb_test "print array_1d" \
	[string_to_regexp "{1, 1, 1...}"]
    gdb_test "print array_1d9" \
	[string_to_regexp "{1, 1, 1...}"]
    gdb_test "print array_2d" \
	[string_to_regexp "{{2, 2, 2...}, {2, 2, 2...}, {2, 2, 2...}...}"]
    gdb_test "print array_2d9" \
	[string_to_regexp "{{2, 2, 2...}, {2, 2, 2...}, {2, 2, 2...}...}"]
    gdb_test "print array_3d" \
	[string_to_regexp "{{{3, 3, 3...}, {3, 3, 3...}, {3, 3, 3...}...},\
			    {{3, 3, 3...}, {3, 3, 3...}, {3, 3, 3...}...},\
			    {{3, 3, 3...}, {3, 3, 3...}, {3, 3, 3...}...}...}"]
    gdb_test "print array_3d9" \
	[string_to_regexp "{{{3, 3, 3...}, {3, 3, 3...}, {3, 3, 3...}...},\
			    {{3, 3, 3...}, {3, 3, 3...}, {3, 3, 3...}...},\
			    {{3, 3, 3...}, {3, 3, 3...}, {3, 3, 3...}...}...}"]
}

with_test_prefix "repeats=4, elements=12" {
    # Now set both limits.
    gdb_test_no_output "set print repeats 4"
    gdb_test_no_output "set print elements 12"

    gdb_test "print array_1d" \
	[string_to_regexp "{1 ${rep5}}"]
    gdb_test "print array_1d9" \
	[string_to_regexp "{1 ${rep5}, 9}"]
    gdb_test "print array_2d" \
	[string_to_regexp "{{2 ${rep5}} ${rep5}}"]
    gdb_test "print array_2d9" \
	[string_to_regexp "{{2 ${rep5}, 9} ${rep5}, {9 ${rep6}}}"]
    gdb_test "print array_3d" \
	[string_to_regexp "{{{3 ${rep5}} ${rep5}} ${rep5}}"]
    gdb_test "print array_3d9" \
	[string_to_regexp "{{{3 ${rep5}, 9} ${rep5}, {9 ${rep6}}} ${rep5},\
			    {{9 ${rep6}} ${rep6}}}"]
}
