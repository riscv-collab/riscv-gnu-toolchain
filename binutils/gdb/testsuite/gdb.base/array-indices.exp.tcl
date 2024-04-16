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

# Test the printing of element indices in C/C++ arrays.

standard_testfile ${srcdir}/gdb.base/array-repeat.c

if {[prepare_for_testing ${testfile}.exp ${testfile} ${srcfile} \
	[list debug ${lang}]]} {
    return -1
}

gdb_test_no_output "set print array-indexes on"

if {![runto_main]} {
    perror "Could not run to main."
    continue
}

gdb_breakpoint [gdb_get_line_number "Break here"]
gdb_continue_to_breakpoint "Break here"

# Build up the expected output for each array.
set n0    {[0]}
set n1    {[1]}
set n2    {[2]}
set n3    {[3]}
set n4    {[4]}
set n5    {[5]}
set a9p9o "{$n0 = 9, $n1 = 9, $n2 = 9, $n3 = 9, $n4 = 9, $n5 = 9}"
set a1p   "{$n0 = 1, $n1 = 1, $n2 = 1, $n3 = 1, $n4 = 1}"
set a1p9  "{$n0 = 1, $n1 = 1, $n2 = 1, $n3 = 1, $n4 = 1, $n5 = 9}"
set a2po  "{$n0 = 2, $n1 = 2, $n2 = 2, $n3 = 2, $n4 = 2}"
set a2p   "{$n0 = ${a2po}, $n1 = ${a2po}, $n2 = ${a2po}, $n3 = ${a2po},\
	    $n4 = ${a2po}}"
set a2p9o "{$n0 = 2, $n1 = 2, $n2 = 2, $n3 = 2, $n4 = 2, $n5 = 9}"
set a2p9  "{$n0 = ${a2p9o}, $n1 = ${a2p9o}, $n2 = ${a2p9o}, $n3 = ${a2p9o},\
	    $n4 = ${a2p9o}, $n5 = ${a9p9o}}"
set a3po  "{$n0 = 3, $n1 = 3, $n2 = 3, $n3 = 3, $n4 = 3}"
set a3p   "{$n0 = ${a3po}, $n1 = ${a3po}, $n2 = ${a3po}, $n3 = ${a3po},\
	    $n4 = ${a3po}}"
set a3p   "{$n0 = ${a3p}, $n1 = ${a3p}, $n2 = ${a3p}, $n3 = ${a3p},\
	    $n4 = ${a3p}}"
set a3p9o "{$n0 = 3, $n1 = 3, $n2 = 3, $n3 = 3, $n4 = 3, $n5 = 9}"
set a3p9  "{$n0 = ${a3p9o}, $n1 = ${a3p9o}, $n2 = ${a3p9o}, $n3 = ${a3p9o},\
	    $n4 = ${a3p9o}, $n5 = ${a9p9o}}"
set a9p9  "{$n0 = ${a9p9o}, $n1 = ${a9p9o}, $n2 = ${a9p9o}, $n3 = ${a9p9o},\
	    $n4 = ${a9p9o}, $n5 = ${a9p9o}}"
set a3p9  "{$n0 = ${a3p9}, $n1 = ${a3p9}, $n2 = ${a3p9}, $n3 = ${a3p9},\
	    $n4 = ${a3p9}, $n5 = ${a9p9}}"

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
	[string_to_regexp "{$n0 = 1 ${rep5}}"]
    gdb_test "print array_1d9" \
	[string_to_regexp "{$n0 = 1 ${rep5}, $n5 = 9}"]
    gdb_test "print array_2d" \
	[string_to_regexp "{$n0 = {$n0 = 2 ${rep5}} ${rep5}}"]
    gdb_test "print array_2d9" \
	[string_to_regexp "{$n0 = {$n0 = 2 ${rep5}, $n5 = 9} ${rep5},\
			    $n5 = {$n0 = 9 ${rep6}}}"]
    gdb_test "print array_3d" \
	[string_to_regexp "{$n0 = {$n0 = {$n0 = 3 ${rep5}} ${rep5}} ${rep5}}"]
    gdb_test "print array_3d9" \
	[string_to_regexp "{$n0 = {$n0 = {$n0 = 3 ${rep5}, $n5 = 9} ${rep5},\
				   $n5 = {$n0 = 9 ${rep6}}} ${rep5},\
			    $n5 = {$n0 = {$n0 = 9 ${rep6}} ${rep6}}}"]
}

with_test_prefix "repeats=unlimited, elements=3" {
    # Now set the element limit.
    gdb_test_no_output "set print repeats unlimited"
    gdb_test_no_output "set print elements 3"

    gdb_test "print array_1d" \
	[string_to_regexp "{$n0 = 1, $n1 = 1, $n2 = 1...}"]
    gdb_test "print array_1d9" \
	[string_to_regexp "{$n0 = 1, $n1 = 1, $n2 = 1...}"]
    gdb_test "print array_2d" \
	[string_to_regexp "{$n0 = {$n0 = 2, $n1 = 2, $n2 = 2...},\
			    $n1 = {$n0 = 2, $n1 = 2, $n2 = 2...},\
			    $n2 = {$n0 = 2, $n1 = 2, $n2 = 2...}...}"]
    gdb_test "print array_2d9" \
	[string_to_regexp "{$n0 = {$n0 = 2, $n1 = 2, $n2 = 2...},\
			    $n1 = {$n0 = 2, $n1 = 2, $n2 = 2...},\
			    $n2 = {$n0 = 2, $n1 = 2, $n2 = 2...}...}"]
    gdb_test "print array_3d" \
	[string_to_regexp "{$n0 = {$n0 = {$n0 = 3, $n1 = 3, $n2 = 3...},\
				   $n1 = {$n0 = 3, $n1 = 3, $n2 = 3...},\
				   $n2 = {$n0 = 3, $n1 = 3, $n2 = 3...}...},\
			    $n1 = {$n0 = {$n0 = 3, $n1 = 3, $n2 = 3...},\
				   $n1 = {$n0 = 3, $n1 = 3, $n2 = 3...},\
				   $n2 = {$n0 = 3, $n1 = 3, $n2 = 3...}...},\
			    $n2 = {$n0 = {$n0 = 3, $n1 = 3, $n2 = 3...},\
				   $n1 = {$n0 = 3, $n1 = 3, $n2 = 3...},\
				   $n2 = {$n0 = 3, $n1 = 3,\
					  $n2 = 3...}...}...}"]
    gdb_test "print array_3d9" \
	[string_to_regexp "{$n0 = {$n0 = {$n0 = 3, $n1 = 3, $n2 = 3...},\
				   $n1 = {$n0 = 3, $n1 = 3, $n2 = 3...},\
				   $n2 = {$n0 = 3, $n1 = 3, $n2 = 3...}...},\
			    $n1 = {$n0 = {$n0 = 3, $n1 = 3, $n2 = 3...},\
				   $n1 = {$n0 = 3, $n1 = 3, $n2 = 3...},\
				   $n2 = {$n0 = 3, $n1 = 3, $n2 = 3...}...},\
			    $n2 = {$n0 = {$n0 = 3, $n1 = 3, $n2 = 3...},\
				   $n1 = {$n0 = 3, $n1 = 3, $n2 = 3...},\
				   $n2 = {$n0 = 3, $n1 = 3,\
					  $n2 = 3...}...}...}"]
}

with_test_prefix "repeats=4, elements=12" {
    # Now set both limits.
    gdb_test_no_output "set print repeats 4"
    gdb_test_no_output "set print elements 12"

    gdb_test "print array_1d" \
	[string_to_regexp "{$n0 = 1 ${rep5}}"]
    gdb_test "print array_1d9" \
	[string_to_regexp "{$n0 = 1 ${rep5}, $n5 = 9}"]
    gdb_test "print array_2d" \
	[string_to_regexp "{$n0 = {$n0 = 2 ${rep5}} ${rep5}}"]
    gdb_test "print array_2d9" \
	[string_to_regexp "{$n0 = {$n0 = 2 ${rep5}, $n5 = 9} ${rep5},\
			    $n5 = {$n0 = 9 ${rep6}}}"]
    gdb_test "print array_3d" \
	[string_to_regexp "{$n0 = {$n0 = {$n0 = 3 ${rep5}} ${rep5}} ${rep5}}"]
    gdb_test "print array_3d9" \
	[string_to_regexp "{$n0 = {$n0 = {$n0 = 3 ${rep5}, $n5 = 9} ${rep5},\
				   $n5 = {$n0 = 9 ${rep6}}} ${rep5},\
			    $n5 = {$n0 = {$n0 = 9 ${rep6}} ${rep6}}}"]
}
