/* Copyright 2010-2024 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* Compiled from:

    namespace N
    {
      class C
      {
      public:
        typedef void (*t) (C);
        C (t) {}
      };
      typedef C::t u;
      u f;
      C c (f);
    };

    int
    main ()
    {
      return 0;
    }
*/

	.text
text_start:
_ZN1N1cE:
	/* Valid function must have non-empty PC range.  */
	.4byte	0
text_end:

	.section	.debug_info
d:
	.4byte	.Ldebug_info_end - 1f /* Length of CU info */
1:
	.2byte	0x2		/* DWARF version number */
	.4byte	.Ldebug_abbrev0	/* Abbrev offset */
	.byte	0x4		/* Pointer size */
dieb:	.uleb128 0x1		/* DW_TAG_compile_unit */
	.4byte	.LASF4		/* DW_AT_producer */
	.byte	0x4		/* DW_AT_language */
	.4byte	.LASF5		/* DW_AT_name */
	.4byte	.LASF6		/* DW_AT_comp_dir */
	.4byte	text_start	/* DW_AT_low_pc */
	.4byte	text_end	/* DW_AT_high_pc */
	.4byte	text_start	/* DW_AT_entry_pc */
die29:	.uleb128 0x2		/* DW_TAG_namespace */
	.string	"N"		/* DW_AT_name */
die32:	.uleb128 0x3		/* DW_TAG_class_type */
	.string	"C"		/* DW_AT_name */
	.byte	0x1		/* DW_AT_declaration */
die36:	.uleb128 0x4		/* DW_TAG_typedef */
	.string	"u"		/* DW_AT_name */
	.4byte	die7e-d		/* DW_AT_type */
die3f:	.uleb128 0x5		/* DW_TAG_variable */
	.string	"f"		/* DW_AT_name */
	.4byte	.LASF0		/* DW_AT_MIPS_linkage_name */
	.4byte	die36-d		/* DW_AT_type */
	.byte	0x1		/* DW_AT_external */
	.byte	0x1		/* DW_AT_declaration */
die4e:	.uleb128 0x5		/* DW_TAG_variable */
	.string	"c"		/* DW_AT_name */
	.4byte	.LASF1		/* DW_AT_MIPS_linkage_name */
	.4byte	die5e-d		/* DW_AT_type */
	.byte	0x1		/* DW_AT_external */
	.byte	0x1		/* DW_AT_declaration */
	.byte	0x0
die5e:	.uleb128 0x6		/* DW_TAG_class_type */
	.4byte	die32-d		/* DW_AT_specification */
	.byte	0x1		/* DW_AT_byte_size */
die6a:	.uleb128 0x7		/* DW_TAG_subprogram */
	.byte	0x1		/* DW_AT_external */
	.string	"C"		/* DW_AT_name */
	.byte	0x1		/* DW_AT_declaration */
die71:	.uleb128 0x8		/* DW_TAG_formal_parameter */
	.4byte	die8f-d		/* DW_AT_type */
	.byte	0x1		/* DW_AT_artificial */
die77:	.uleb128 0x9		/* DW_TAG_formal_parameter */
	.4byte	die7e-d		/* DW_AT_type */
	.byte	0x0
	.byte	0x0
die7e:	.uleb128 0xa		/* DW_TAG_pointer_type */
	.byte	PTRBITS / 8	/* DW_AT_byte_size */
	.4byte	die84-d		/* DW_AT_type */
die84:	.uleb128 0xb		/* DW_TAG_subroutine_type */
die89:	.uleb128 0x9		/* DW_TAG_formal_parameter */
	.4byte	die5e-d		/* DW_AT_type */
	.byte	0x0
die8f:	.uleb128 0xa		/* DW_TAG_pointer_type */
	.byte	PTRBITS / 8	/* DW_AT_byte_size */
	.4byte	die5e-d		/* DW_AT_type */
die95:	.uleb128 0xc		/* DW_TAG_subprogram */
	.4byte	die6a-d		/* DW_AT_specification */
	.byte	0x2		/* DW_AT_inline */
die9f:	.uleb128 0xd		/* DW_TAG_formal_parameter */
	.4byte	.LASF7		/* DW_AT_name */
	.4byte	dieaf-d		/* DW_AT_type */
	.byte	0x1		/* DW_AT_artificial */
diea9:	.uleb128 0x9		/* DW_TAG_formal_parameter */
	.4byte	die7e-d		/* DW_AT_type */
	.byte	0x0
dieaf:	.uleb128 0xe		/* DW_TAG_const_type */
	.4byte	die8f-d		/* DW_AT_type */
dieb4:	.uleb128 0xf		/* DW_TAG_subprogram */
	.4byte	die95-d		/* DW_AT_abstract_origin */
	.4byte	_ZN1N1cE	/* DW_AT_low_pc */
	.4byte	_ZN1N1cE + (text_end - text_start)
				/* DW_AT_high_pc */
diec9:	.uleb128 0x10		/* DW_TAG_subprogram */
	.4byte	die9f-d		/* DW_AT_abstract_origin */
	.byte	2f-1f		/* DW_AT_location */
1:
	.byte	0x50		/* DW_OP_reg0 */
2:
died1:	.uleb128 0x10		/* DW_TAG_formal_parameter */
	.4byte	diea9-d		/* DW_AT_abstract_origin */
	.byte	2f-1f		/* DW_AT_location */
1:
	.byte	0x51		/* DW_OP_reg1 */
2:
	.byte	0x0
dieda:	.uleb128 0x11		/* DW_TAG_subprogram */
	.byte	0x1		/* DW_AT_external */
	.4byte	.LASF8		/* DW_AT_name */
	.4byte	dief2-d		/* DW_AT_type */
	.4byte	_ZN1N1cE	/* DW_AT_low_pc */
	.4byte	_ZN1N1cE + (text_end - text_start)
				/* DW_AT_high_pc */
dief2:	.uleb128 0x12		/* DW_TAG_base_type */
	.byte	0x4		/* DW_AT_byte_size */
	.byte	0x5		/* DW_AT_encoding */
	.string	"int"		/* DW_AT_name */
die149:	.uleb128 0x16		/* DW_TAG_variable */
	.4byte	die4e-d		/* DW_AT_specification */
	.byte	0x5		/* DW_AT_location */
	.byte	0x3
	.4byte	_ZN1N1cE
	.byte	0x0
.Ldebug_info_end:
	.section	.debug_abbrev
.Ldebug_abbrev0:
	.uleb128 0x1		/* abbrev code*/
	.uleb128 0x11		/* DW_TAG_compile_unit */
	.byte	0x1		/* DW_children_yes */
	.uleb128 0x25		/* DW_AT_producer*/
	.uleb128 0xe		/* DW_FORM_strp */
	.uleb128 0x13		/* DW_AT_language */
	.uleb128 0xb		/* DW_FORM_data1 */
	.uleb128 0x3		/* DW_AT_name */
	.uleb128 0xe		/* DW_FORM_strp */
	.uleb128 0x1b		/* DW_AT_comp_dir */
	.uleb128 0xe		/* DW_FORM_strp */
	.uleb128 0x11		/* DW_AT_low_pc */
	.uleb128 0x1		/* DW_FORM_addr */
	.uleb128 0x12		/* DW_AT_high_pc */
	.uleb128 0x1		/* DW_FORM_addr */
	.uleb128 0x52		/* DW_AT_entry_pc */
	.uleb128 0x1		/* DW_FORM_addr */
	.byte	0x0
	.byte	0x0
	.uleb128 0x2		/* abbrev code */
	.uleb128 0x39		/* DW_TAG_namespace */
	.byte	0x1		/* DW_children_yes */
	.uleb128 0x3		/* DW_AT_name */
	.uleb128 0x8		/* DW_FORM_string */
	.byte	0x0
	.byte	0x0
	.uleb128 0x3		/* abbrev code */
	.uleb128 0x2		/* DW_TAG_class_type */
	.byte	0x0		/* DW_has_children_no */
	.uleb128 0x3		/* DW_AT_name */
	.uleb128 0x8		/* DW_FORM_string */
	.uleb128 0x3c		/* DW_AT_declaration */
	.uleb128 0xc		/* DW_FORM_flag */
	.byte	0x0
	.byte	0x0
	.uleb128 0x4		/* abbrev code */
	.uleb128 0x16		/* DW_TAG_typedef */
	.byte	0x0		/* DW_has_children_no */
	.uleb128 0x3		/* DW_AT_name */
	.uleb128 0x8		/* DW_FORM_string */
	.uleb128 0x49		/* DW_AT_type */
	.uleb128 0x13		/* DW_FORM_ref4 */
	.byte	0x0
	.byte	0x0
	.uleb128 0x5		/* abbrev code */
	.uleb128 0x34		/* DW_TAG_variable */
	.byte	0x0		/* DW_has_children_no */
	.uleb128 0x3		/* DW_AT_name */
	.uleb128 0x8		/* DW_FORM_string */
	.uleb128 0x2007		/* DW_AT_MIPS_linkage_name */
	.uleb128 0xe		/* DW_FORM_strp */
	.uleb128 0x49		/* DW_AT_TYPE */
	.uleb128 0x13		/* DW_FORM_ref4 */
	.uleb128 0x3f		/* DW_AT_external */
	.uleb128 0xc		/* DW_FORM_flag */
	.uleb128 0x3c		/* DW_AT_declaration */
	.uleb128 0xc		/* DW_FORM_flag */
	.byte	0x0
	.byte	0x0
	.uleb128 0x6		/* abbrev code */
	.uleb128 0x2		/* DW_TAG_class_type */
	.byte	0x1		/* DW_has_children_yes */
	.uleb128 0x47		/* DW_AT_specification */
	.uleb128 0x13		/* DW_FORM_ref4 */
	.uleb128 0xb		/* DW_AT_byte_size */
	.uleb128 0xb		/* DW_FORM_data1 */
	.byte	0x0
	.byte	0x0
	.uleb128 0x7		/* abbrev code */
	.uleb128 0x2e		/* DW_TAG_subprogra */
	.byte	0x1		/* DW_has_children_yes */
	.uleb128 0x3f		/* DW_AT_external */
	.uleb128 0xc		/* DW_FORM_flag */
	.uleb128 0x3		/* DW_AT_name */
	.uleb128 0x8		/* DW_FORM_string */
	.uleb128 0x3c		/* DW_AT_declaration */
	.uleb128 0xc		/* DW_FORM_flag */
	.byte	0x0
	.byte	0x0
	.uleb128 0x8		/* abbrev code */
	.uleb128 0x5		/* DW_TAG_formal_parameter */
	.byte	0x0		/* DW_has_children_no */
	.uleb128 0x49		/* DW_AT_type */
	.uleb128 0x13		/* DW_FORM_ref4 */
	.uleb128 0x34		/* DW_AT_artificial */
	.uleb128 0xc		/* DW_FORM_flag */
	.byte	0x0
	.byte	0x0
	.uleb128 0x9		/* abbrev code */
	.uleb128 0x5		/* DW_TAG_formal_parameter */
	.byte	0x0		/* DW_has_children_no */
	.uleb128 0x49		/* DW_AT_type */
	.uleb128 0x13		/* DW_FORM_ref4 */
	.byte	0x0
	.byte	0x0
	.uleb128 0xa		/* abbrev code */
	.uleb128 0xf		/* DW_TAG_pointer_type */
	.byte	0x0		/* DW_has_children_no */
	.uleb128 0xb		/* DW_AT_byte_size */
	.uleb128 0xb		/* DW_FORM_data1 */
	.uleb128 0x49		/* DW_AT_type */
	.uleb128 0x13		/* DW_FORM_ref4 */
	.byte	0x0
	.byte	0x0
	.uleb128 0xb		/* abbrev code */
	.uleb128 0x15		/* DW_TAG_subroutine_type */
	.byte	0x1		/* DW_has_children_yes */
	.byte	0x0
	.byte	0x0
	.uleb128 0xc		/* abbrev code */
	.uleb128 0x2e		/* DW_TAG_subprogram */
	.byte	0x1		/* DW_has_children_yes */
	.uleb128 0x47		/* DW_AT_specification */
	.uleb128 0x13		/* DW_FORM_ref4 */
	.uleb128 0x20		/* DW_AT_inline */
	.uleb128 0xb		/* DW_FORM_data1 */
	.byte	0x0
	.byte	0x0
	.uleb128 0xd		/* abbrev code */
	.uleb128 0x5		/* DW_TAG_formal_parameter */
	.byte	0x0		/* DW_has_children_no */
	.uleb128 0x3		/* DW_AT_name */
	.uleb128 0xe		/* DW_FORM_strp */
	.uleb128 0x49		/* DW_AT_type */
	.uleb128 0x13		/* DW_FORM_ref4 */
	.uleb128 0x34		/* DW_AT_artificial */
	.uleb128 0xc		/* DW_FORM_flag */
	.byte	0x0
	.byte	0x0
	.uleb128 0xe		/* abbrev code */
	.uleb128 0x26		/* DW_TAG_const_type */
	.byte	0x0		/* DW_has_children_no */
	.uleb128 0x49		/* DW_AT_type */
	.uleb128 0x13		/* DW_FORM_ref4 */
	.byte	0x0
	.byte	0x0
	.uleb128 0xf		/* abbrev code */
	.uleb128 0x2e		/* DW_TAG_subprogram */
	.byte	0x1		/* DW_has_children_yes */
	.uleb128 0x31		/* DW_AT_abstract_origin */
	.uleb128 0x13		/* DW_FORM_ref4 */
	.uleb128 0x11		/* DW_AT_low_pc */
	.uleb128 0x1		/* DW_FORM_addr */
	.uleb128 0x12		/* DW_AT_high_pc */
	.uleb128 0x1		/* DW_FORM_addr */
	.byte	0x0
	.byte	0x0
	.uleb128 0x10		/* abbrev code */
	.uleb128 0x5		/* DW_TAG_formal_parameter */
	.byte	0x0		/* DW_has_children_no */
	.uleb128 0x31		/* DW_AT_abstract_origin */
	.uleb128 0x13		/* DW_FORM_ref4 */
	.uleb128 0x2		/* DW_AT_location */
	.uleb128 0xa		/* DW_FORM_block1 */
	.byte	0x0
	.byte	0x0
	.uleb128 0x11		/* abbrev code */
	.uleb128 0x2e		/* DW_TAG_subprogram */
	.byte	0x0		/* DW_has_children_no */
	.uleb128 0x3f		/* DW_AT_external */
	.uleb128 0xc		/* DW_FORM_flag */
	.uleb128 0x3		/* DW_AT_name */
	.uleb128 0xe		/* DW_FORM_strp */
	.uleb128 0x49		/* DW_AT_type */
	.uleb128 0x13		/* DW_FORM_ref4 */
	.uleb128 0x11		/* DW_AT_low_pc */
	.uleb128 0x1		/* DW_FORM_addr */
	.uleb128 0x12		/* DW_AT_high_pc */
	.uleb128 0x1		/* DW_FORM_addr */
	.byte	0x0
	.byte	0x0
	.uleb128 0x12		/* abbrev code */
	.uleb128 0x24		/* DW_TAG_base_type */
	.byte	0x0		/* DW_has_children_no */
	.uleb128 0xb		/* DW_AT_byte_size */
	.uleb128 0xb		/* DW_FORM_data1 */
	.uleb128 0x3e		/* DW_AT_encoding */
	.uleb128 0xb		/* DW_FORM_data1 */
	.uleb128 0x3		/* DW_AT_name */
	.uleb128 0x8		/* DW_FORM_string */
	.byte	0x0
	.byte	0x0
	.uleb128 0x16		/* abbrev code */
	.uleb128 0x34		/* DW_TAG_variable */
	.byte	0x0		/* DW_has_children_no */
	.uleb128 0x47		/* DW_AT_specification */
	.uleb128 0x13		/* DW_FORM_ref4 */
	.uleb128 0x2		/* DW_AT_location */
	.uleb128 0xa		/* DW_FORM_block1 */
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.section	.debug_str
.LASF0:
	.string	"_ZN1N1fE"
.LASF7:
	.string	"this"
.LASF6:
	.string	""
.LASF8:
	.string	"main"
.LASF1:
	.string	"_ZN1N1cE"
.LASF5:
	.string	"pr11465.cc"
.LASF4:
	.string	"GNU C++ 4.4.2"
	.ident	"GCC: (GNU) 4.4.2"
#ifdef __arm__
	.section	.note.GNU-stack,"",%progbits
#else
	.section	.note.GNU-stack,"",@progbits
#endif
