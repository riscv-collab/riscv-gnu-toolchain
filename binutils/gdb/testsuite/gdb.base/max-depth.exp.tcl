# Copyright 2019-2024 Free Software Foundation, Inc.

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

# Tests GDB's handling of 'set print max-depth'.

proc compile_and_run_tests { lang } {
    global testfile
    global srcfile
    global binfile
    global hex

    standard_testfile max-depth.c

    # Create the additional flags.
    set flags "debug"
    lappend flags $lang
    if { "$lang" == "c++" } {
	lappend flags "additional_flags=-std=c++11"
    }

    if { [prepare_for_testing "failed to prepare" "${binfile}" "${srcfile}" "${flags}"] } {
	return 0
    }

    # Advance to main.
    if { ![runto_main] } then {
	return 0
    }

    # The max-depth setting has no effect as the anonymous scopes are
    # ignored and the members are aggregated into the parent scope.
    gdb_print_expr_at_depths "s1" {"{...}" \
				       "{x = 0, y = 0}"\
				       "{x = 0, y = 0}"}

    gdb_print_expr_at_depths "s2" {"{...}" \
				       "{x = 0, y = 0, {z = 0, a = 0}}" \
				       "{x = 0, y = 0, {z = 0, a = 0}}"}

    gdb_print_expr_at_depths "s3" {"{...}" \
				       "{x = 0, y = 0, {z = 0, a = 0, {b = 0, c = 0}}}" \
				       "{x = 0, y = 0, {z = 0, a = 0, {b = 0, c = 0}}}" \
				       "{x = 0, y = 0, {z = 0, a = 0, {b = 0, c = 0}}}"}

    # Increasing max-depth unfurls more of the object.
    gdb_print_expr_at_depths "s4" {"{...}" \
				       "{x = 0, y = 0, l1 = {...}}" \
				       "{x = 0, y = 0, l1 = {x = 0, y = 0, l2 = {...}}}" \
				       "{x = 0, y = 0, l1 = {x = 0, y = 0, l2 = {x = 0, y = 0}}}"}

    # Check handling of unions, in this case 'raw' is printed instead of
    # just {...} as this is not useful.
    gdb_print_expr_at_depths "s5" {"{...}" \
				       "{{raw = {...}, {x = 0, y = 0, z = 0}}}" \
				       "{{raw = \\{0, 0, 0\\}, {x = 0, y = 0, z = 0}}}"}

    # Check handling of typedefs.
    gdb_print_expr_at_depths "s6" {"{...}" \
				       "{{raw = {...}, {x = 0, y = 0, z = 0}}}" \
				       "{{raw = \\{0, 0, 0\\}, {x = 0, y = 0, z = 0}}}"}

    # Multiple anonymous structures in parallel.
    gdb_print_expr_at_depths "s7" {"{...}" \
				       "{{x = 0, y = 0}, {z = 0, a = 0}, {b = 0, c = 0}}" \
				       "{{x = 0, y = 0}, {z = 0, a = 0}, {b = 0, c = 0}}"}

    # Flip flop between named and anonymous.  Expected to unfurl to the
    # first non-anonymous type.
    gdb_print_expr_at_depths "s8" {"{...}" \
				       "{x = 0, y = 0, d1 = {...}}" \
				       "{x = 0, y = 0, d1 = {z = 0, a = 0, {b = 0, c = 0}}}"}

    # Imbalanced tree, this will unfurl one size more than the other as
    # one side has more anonymous levels.
    gdb_print_expr_at_depths "s9" {"{...}" \
				       "{x = 0, y = 0, {k = 0, j = 0, d1 = {...}}, d2 = {...}}" \
				       "{x = 0, y = 0, {k = 0, j = 0, d1 = {z = 0, a = 0, {b = 0, c = 0}}}, d2 = {z = 0, a = 0, {b = 0, c = 0}}}"}

    # Arrays are treated as an extra level, while scalars are not.
    gdb_print_expr_at_depths "s10" {"{...}" \
					"{x = {...}, y = 0, {k = {...}, j = 0, d1 = {...}}, d2 = {...}}" \
					"{x = \\{0, 0, 0, 0, 0, 0, 0, 0, 0, 0\\}, y = 0, {k = \\{0, 0, 0, 0, 0, 0, 0, 0, 0, 0\\}, j = 0, d1 = {z = 0, a = 0, {b = {...}, c = 0}}}, d2 = {z = 0, a = 0, {b = {...}, c = 0}}}" \
					"{x = \\{0, 0, 0, 0, 0, 0, 0, 0, 0, 0\\}, y = 0, {k = \\{0, 0, 0, 0, 0, 0, 0, 0, 0, 0\\}, j = 0, d1 = {z = 0, a = 0, {b = \\{0, 0, 0, 0, 0, 0, 0, 0, 0, 0\\}, c = 0}}}, d2 = {z = 0, a = 0, {b = \\{0, 0, 0, 0, 0, 0, 0, 0, 0, 0\\}, c = 0}}}"}

    # Strings are treated as scalars.
    gdb_print_expr_at_depths "s11" {"{...}" \
					"{x = 0, s = \"\\\\000\\\\000\\\\000\\\\000\\\\000\\\\000\\\\000\\\\000\\\\000\", {z = 0, a = 0}}"}


    if { $lang == "c++" } {
	gdb_print_expr_at_depths "c1" {"{...}" \
					   "{c1 = 1}" }
	gdb_print_expr_at_depths "c2" { "{...}" "{c2 = 2}" }
	gdb_print_expr_at_depths "c3" { "{...}" \
					    "{<C2> = {...}, c3 = 3}" \
					    "{<C2> = {c2 = 2}, c3 = 3}" }
	gdb_print_expr_at_depths "c4" { "{...}" "{c4 = 4}" }
	gdb_print_expr_at_depths "c5" { "{...}" \
					    "{<C4> = {...}, c5 = 5}" \
					    "{<C4> = {c4 = 4}, c5 = 5}" }
	gdb_print_expr_at_depths "c6" { "{...}" \
					    "{<C5> = {...}, c6 = 6}" \
					    "{<C5> = {<C4> = {...}, c5 = 5}, c6 = 6}" \
					    "{<C5> = {<C4> = {c4 = 4}, c5 = 5}, c6 = 6}" }
	gdb_print_expr_at_depths "c7" { "{...}" \
					    "{<C1> = {...}, <C3> = {...}, <C6> = {...}, c7 = 7}" \
					    "{<C1> = {c1 = 1}, <C3> = {<C2> = {...}, c3 = 3}, <C6> = {<C5> = {...}, c6 = 6}, c7 = 7}" \
					    "{<C1> = {c1 = 1}, <C3> = {<C2> = {c2 = 2}, c3 = 3}, <C6> = {<C5> = {<C4> = {...}, c5 = 5}, c6 = 6}, c7 = 7}" \
					    "{<C1> = {c1 = 1}, <C3> = {<C2> = {c2 = 2}, c3 = 3}, <C6> = {<C5> = {<C4> = {c4 = 4}, c5 = 5}, c6 = 6}, c7 = 7}" }

	gdb_print_expr_at_depths "v1" [list "{...}" "{v1 = 1}" ]
	gdb_print_expr_at_depths "v2" [list "{...}" \
					    "{<V1> = {...}, _vptr.V2 = $hex <VTT for V2>, v2 = 2}" \
					    "{<V1> = {v1 = 1}, _vptr.V2 = $hex <VTT for V2>, v2 = 2}" ]
	gdb_print_expr_at_depths "v3" [list "{...}" \
					    "{<V1> = {...}, _vptr.V3 = $hex <VTT for V3>, v3 = 3}" \
					    "{<V1> = {v1 = 1}, _vptr.V3 = $hex <VTT for V3>, v3 = 3}" ]
	gdb_print_expr_at_depths "v4" [list "{...}" \
					    "{<V2> = {...}, _vptr.V4 = $hex <vtable for V4\[^>\]+>, v4 = 4}" \
					    "{<V2> = {<V1> = {...}, _vptr.V2 = $hex <VTT for V4>, v2 = 2}, _vptr.V4 = $hex <vtable for V4\[^>\]+>, v4 = 4}" \
					    "{<V2> = {<V1> = {v1 = 1}, _vptr.V2 = $hex <VTT for V4>, v2 = 2}, _vptr.V4 = $hex <vtable for V4\[^>\]+>, v4 = 4}" ]
	gdb_print_expr_at_depths "v5" [list "{...}" \
					    "{<V2> = {...}, _vptr.V5 = $hex <vtable for V5\[^>\]+>, v5 = 1}" \
					    "{<V2> = {<V1> = {...}, _vptr.V2 = $hex <VTT for V5>, v2 = 2}, _vptr.V5 = $hex <vtable for V5\[^>\]+>, v5 = 1}" \
					    "{<V2> = {<V1> = {v1 = 1}, _vptr.V2 = $hex <VTT for V5>, v2 = 2}, _vptr.V5 = $hex <vtable for V5\[^>\]+>, v5 = 1}" ]
	gdb_print_expr_at_depths "v6" [list "{...}" \
					    "{<V2> = {...}, <V3> = {...}, _vptr.V6 = $hex <vtable for V6\[^>\]+>, v6 = 1}" \
					    "{<V2> = {<V1> = {...}, _vptr.V2 = $hex <vtable for V6\[^>\]+>, v2 = 2}, <V3> = {_vptr.V3 = $hex <VTT for V6>, v3 = 3}, _vptr.V6 = $hex <vtable for V6\[^>\]+>, v6 = 1}" \
					    "{<V2> = {<V1> = {v1 = 1}, _vptr.V2 = $hex <vtable for V6\[^>\]+>, v2 = 2}, <V3> = {_vptr.V3 = $hex <VTT for V6>, v3 = 3}, _vptr.V6 = $hex <vtable for V6\[^>\]+>, v6 = 1}" ]
	gdb_print_expr_at_depths "v7" [list "{...}" \
					    "{<V4> = {...}, <V5> = {...}, <V6> = {...}, _vptr.V7 = $hex <vtable for V7\[^>\]+>, v7 = 1}" \
					    "{<V4> = {<V2> = {...}, _vptr.V4 = $hex <vtable for V7\[^>\]+>, v4 = 4}, <V5> = {_vptr.V5 = $hex <vtable for V7\[^>\]+>, v5 = 1}, <V6> = {<V3> = {...}, _vptr.V6 = $hex <vtable for V7\[^>\]+>, v6 = 1}, _vptr.V7 = $hex <vtable for V7\[^>\]+>, v7 = 1}" \
					    "{<V4> = {<V2> = {<V1> = {...}, _vptr.V2 = $hex <vtable for V7\[^>\]+>, v2 = 2}, _vptr.V4 = $hex <vtable for V7\[^>\]+>, v4 = 4}, <V5> = {_vptr.V5 = $hex <vtable for V7\[^>\]+>, v5 = 1}, <V6> = {<V3> = {_vptr.V3 = $hex <VTT for V7>, v3 = 3}, _vptr.V6 = $hex <vtable for V7\[^>\]+>, v6 = 1}, _vptr.V7 = $hex <vtable for V7\[^>\]+>, v7 = 1}" \
					    "{<V4> = {<V2> = {<V1> = {v1 = 1}, _vptr.V2 = $hex <vtable for V7\[^>\]+>, v2 = 2}, _vptr.V4 = $hex <vtable for V7\[^>\]+>, v4 = 4}, <V5> = {_vptr.V5 = $hex <vtable for V7\[^>\]+>, v5 = 1}, <V6> = {<V3> = {_vptr.V3 = $hex <VTT for V7>, v3 = 3}, _vptr.V6 = $hex <vtable for V7\[^>\]+>, v6 = 1}, _vptr.V7 = $hex <vtable for V7\[^>\]+>, v7 = 1}" ]
    }
}

compile_and_run_tests $lang
