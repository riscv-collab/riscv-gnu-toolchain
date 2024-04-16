# Copyright 2020-2024 Free Software Foundation, Inc.

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

# Test that "break /absolute/file:line" works ok with imported CUs.

load_lib dwarf.exp

# This test can only be run on targets which support DWARF-2 and use gas.
if {![dwarf2_support]} {
    return 0
}

# The .c files use __attribute__.
if ![is_c_compiler_gcc] {
    return 0
}

standard_testfile imported-unit-bp-alt.c .S imported-unit-bp-main.c

set build_options {nodebug optimize=-O1}

set asm_file [standard_output_file $srcfile2]
Dwarf::assemble $asm_file {
    global srcdir subdir srcfile srcfile
    global build_options
    global lang
    declare_labels lines_label callee_subprog_label cu_label

    get_func_info func "$build_options additional_flags=-DWITHMAIN"

    cu {} {
	compile_unit {
	    {language @$lang}
	    {name "<artificial>"}
	} {
	    imported_unit {
		{import %$cu_label}
	    }
	}
    }

    cu {} {
	cu_label: compile_unit {
	    {producer "gcc"}
	    {language @$lang}
	    {name ${srcfile}}
	    {comp_dir "/tmp"}
	    {low_pc 0 addr}
	    {stmt_list ${lines_label} DW_FORM_sec_offset}
	} {
	    callee_subprog_label: subprogram {
		{external 1 flag}
		{name callee}
		{inline 3 data1}
	    }
	    subprogram {
		{external 1 flag}
		{name func}
		{low_pc $func_start addr}
		{high_pc "$func_start + $func_len" addr}
	    } {
	    }
	}
    }

    lines {version 2 default_is_stmt 1} lines_label {
	include_dir "/tmp"
	file_name "$srcfile" 1

	program {
	    DW_LNE_set_address line_label_1
	    DW_LNS_advance_line 15
	    DW_LNS_copy

	    DW_LNE_set_address line_label_2
	    DW_LNS_advance_line 1
	    DW_LNS_copy

	    DW_LNE_set_address line_label_3
	    DW_LNS_advance_line 4
	    DW_LNS_copy

	    DW_LNE_set_address line_label_4
	    DW_LNS_advance_line 1
	    DW_LNS_copy

	    DW_LNS_advance_line -4
	    DW_LNS_negate_stmt
	    DW_LNS_copy

	    DW_LNE_set_address line_label_5
	    DW_LNS_advance_line 1
	    DW_LNS_copy

	    DW_LNE_set_address line_label_6
	    DW_LNS_advance_line 1
	    DW_LNS_negate_stmt
	    DW_LNS_copy

	    DW_LNE_set_address line_label_7
	    DW_LNE_end_sequence
	}
    }
}

if { [prepare_for_testing "failed to prepare" ${testfile} \
	  [list $srcfile $asm_file $srcfile3] $build_options] } {
    return -1
}

gdb_reinitialize_dir /tmp

# Compilation on remote host downloads the source files to remote host, but
# doesn't clean them up, allowing gdb to find $srcfile, in contrast to
# non-remote host.
remote_file host delete $srcfile

# Using an absolute path is important to see the bug.
gdb_test "break /tmp/${srcfile}:19" "Breakpoint .* file $srcfile, line .*"
