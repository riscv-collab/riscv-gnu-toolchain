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

# Set up the DWARF for the test.

set main_str_label [Dwarf::_compute_label info_string3]
set int_str_label [Dwarf::_compute_label info_string4]
set main_die_label [Dwarf::_compute_label main_die_label]
set int_die_label [Dwarf::_compute_label int_die_label]

set debug_str \
    [list \
         "$main_str_label:" \
         "  .asciz \"main\"" \
         "$int_str_label:" \
         "  .asciz \"int\""]

set debug_names \
    [list \
         "  .4byte  .Ldebug_names_end - .Ldebug_names_start" \
         ".Ldebug_names_start:" \
         "  .short 5                     /* Header: version */" \
         "  .short 0                     /* Header: padding */" \
         "  .long 1                      /* Header: compilation unit count */" \
         "  .long 0                      /* Header: local type unit count */" \
         "  .long 0                      /* Header: foreign type unit count */" \
         "  .long 2                      /* Header: bucket count */" \
         "  .long 2                      /* Header: name count */" \
         "  .long .Lnames_abbrev_end0-.Lnames_abbrev_start0 " \
         "                               /* Header: abbreviation table size */" \
         "  .long 8                      /* Header: augmentation string size */" \
         "  .ascii \"LLVM0700\"   /* Header: augmentation string */" \
         "  .long .Lcu1_begin            /* Compilation unit 0 */" \
         "  .long 1                      /* Bucket 0 */" \
         "  .long 0                      /* Bucket 1 */" \
         "  .long 193495088              /* Hash in Bucket 0 */" \
         "  .long 2090499946             /* Hash in Bucket 0 */" \
         "  .long $int_str_label         /* String in Bucket 0: int */" \
         "  .long $main_str_label        /* String in Bucket 0: main */" \
         "  .long .Lnames1-.Lnames_entries0/* Offset in Bucket 0 */" \
         "  .long .Lnames0-.Lnames_entries0/* Offset in Bucket 0 */" \
         ".Lnames_abbrev_start0:" \
         "  .byte 46                     /* Abbrev code */" \
         "  .byte 46                     /* DW_TAG_subprogram */" \
         "  .byte 3                      /* DW_IDX_die_offset */" \
         "  .byte 19                     /* DW_FORM_ref4 */" \
         "  .byte 0                      /* End of abbrev */" \
         "  .byte 0                      /* End of abbrev */" \
         "  .byte 36                     /* Abbrev code */" \
         "  .byte 36                     /* DW_TAG_base_type */" \
         "  .byte 3                      /* DW_IDX_die_offset */" \
         "  .byte 19                     /* DW_FORM_ref4 */" \
         "  .byte 0                      /* End of abbrev */" \
         "  .byte 0                      /* End of abbrev */" \
         "  .byte 0                      /* End of abbrev list */" \
         ".Lnames_abbrev_end0:" \
         ".Lnames_entries0:" \
         ".Lnames1:" \
         "  .byte 36                     /* Abbreviation code */" \
         "  .long $int_die_label - .Lcu1_begin/* DW_IDX_die_offset */" \
         "  .long 0                      /* End of list: int */" \
         ".Lnames0:" \
         "  .byte 46                     /* Abbreviation code */" \
         "  .long $main_die_label - .Lcu1_begin/* DW_IDX_die_offset */" \
         "  .long 0                      /* End of list: main */" \
         "  .p2align 2" \
         ".Ldebug_names_end:"]

Dwarf::assemble $asm_file {
    global srcdir subdir srcfile
    global main_start main_length

    cu {} {
	DW_TAG_compile_unit {
                {DW_AT_language @DW_LANG_C}
                {DW_AT_name     clang-debug-names.c}
                {DW_AT_comp_dir /tmp}

        } {
	    global int_die_label
	    global main_die_label

	    define_label $int_die_label
	    base_type {
		{name "int"}
		{encoding @DW_ATE_signed}
		{byte_size 4 DW_FORM_sdata}
	    }

	    define_label $main_die_label
	    subprogram {
		{name main}
		{type :$int_die_label}
		{low_pc $main_start addr}
		{high_pc "$main_start + $main_length" addr}
	    }
	}
    }

    _defer_output .debug_str {
	global debug_str
	_emit [join $debug_str "\n"]
    }

    _defer_output .debug_names {
	global debug_names
	_emit [join $debug_names "\n"]
    }
}
