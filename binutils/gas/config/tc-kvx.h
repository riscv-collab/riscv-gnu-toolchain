/* tc-kvx.h -- Header file for tc-kvx.c

   Copyright (C) 2009-2024 Free Software Foundation, Inc.
   Contributed by Kalray SA.

   This file is part of GAS.

   GAS is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the license, or
   (at your option) any later version.

   GAS is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING3. If not,
   see <http://www.gnu.org/licenses/>.  */

#ifndef TC_KVX
#define TC_KVX

#include "as.h"
#include "include/hashtab.h"
#include "opcode/kvx.h"

#ifdef OBJ_ELF
#include "elf/kvx.h"
#endif

#include <stdlib.h>

#define TARGET_ARCH bfd_arch_kvx

#define KVX_RA_REGNO (67)
#define KVX_SP_REGNO (12)

#define O_pseudo_fixup O_md1

#define TOKEN_NAME(tok) \
  ((tok) <= 0 ? "unknown token" : env.tokens_names[(tok) - 1])

struct token_s {
  char *insn;
  int begin, end;
  int category;
  int64_t class_id;
  uint64_t val;
};

struct token_list
{
  char *tok;
  uint64_t val;
  int64_t class_id;
  int category;
  int loc;
  struct token_list *next;
  int len;
};


struct token_class {
  const char ** class_values;
  int64_t class_id;
  int sz;
};

enum token_category {
  CAT_INSTRUCTION,
  CAT_MODIFIER,
  CAT_IMMEDIATE,
  CAT_SEPARATOR,
  CAT_REGISTER,
  CAT_INVALID
};

struct token_classes {
  struct token_class *reg_classes;
  struct token_class *mod_classes;
  struct token_class *imm_classes;
  struct token_class *insn_classes;
  struct token_class *sep_classes;
};

struct steering_rule {
  int steering;
  int jump_target;
  int stack_it;
};

struct rule {
  struct steering_rule *rules;
};

/* Default kvx_registers array. */
extern const struct kvx_Register *kvx_registers;
/* Default kvx_modifiers array. */
extern const char ***kvx_modifiers;
/* Default kvx_regfiles array. */
extern const int *kvx_regfiles;
/* Default values used if no assume directive is given */
extern const struct kvx_core_info *kvx_core_info;

struct kvx_as_options {
  /* Arch string passed as argument with -march option.  */
  char *march;
  /* Resource usage checking.  */
  int check_resource_usage;
  /* Generate illegal code: only use for debugging !*/
  int generate_illegal_code;
  /* Dump asm tables: for debugging */
  int dump_table;
  /* Dump instructions: for documentation */
  int dump_insn;
  /* Enable multiline diagnostics */
  int diagnostics;
  /* Enable more helpful error messages */
  int more;
  /* Used for HW validation: allows all SFR in GET/SET/WFX */
  int allow_all_sfr;
};

extern struct kvx_as_options kvx_options;

struct kvx_as_params {
  /* The target's ABI */
  int abi;
  /* The target's OS/ABI */
  int osabi;
  /* The target core (0: KV3-1, 1: KV3-2, 2: KV4-1) */
  int core;
  /* Guard to check if KVX_CORE has been set */
  int core_set;
  /* Guard to check if KVX_ABI has been set */
  int abi_set;
  /* Guard to check if KVX_OSABI has been set */
  int osabi_set;
  /* Flags controlling Position-Independent Code.  */
  flagword pic_flags;
  /* Either 32 or 64.  */
  int arch_size;
};

extern struct kvx_as_params kvx_params;

struct kvx_as_env {
  const char ** tokens_names;
  int fst_reg, sys_reg, fst_mod;
  int (*promote_immediate) (int);
  struct rule *rules;
  struct token_classes *token_classes;
  struct node_s *insns;
  /* Records enabled options.  */
  struct kvx_as_options opts;
  /* Record the parameters of the target architecture.  */
  struct kvx_as_params params;
  /* The hash table of instruction opcodes.  */
  htab_t opcode_hash;
  /* The hash table of register symbols.  */
  htab_t reg_hash;
  /* The hash table of relocations for immediates.  */
  htab_t reloc_hash;
};

extern struct kvx_as_env env;

struct token_list* parse (struct token_s tok);
void print_token_list (struct token_list *lst);
void free_token_list (struct token_list* tok_list);
void setup (int version);
void cleanup (void);


/* Hooks configuration.  */

extern const char * kvx_target_format (void);
#undef TARGET_FORMAT
#define TARGET_FORMAT kvx_target_format ()

/* default little endian */
#define TARGET_BYTES_BIG_ENDIAN 0
#define md_number_to_chars number_to_chars_littleendian

/* for now we have no BFD target */

/* lexing macros */
/* Allow `$' in names.  */
#define LEX_DOLLAR (LEX_BEGIN_NAME | LEX_NAME)
/* Disable legacy `broken words' processing.  */
#define WORKING_DOT_WORD

extern void kvx_end (void);
#undef md_finish
#define md_finish kvx_end

#define TC_FIX_TYPE struct _symbol_struct *
#define TC_SYMFILED_TYPE struct list_info_struct *
#define TC_INIT_FIX_DATA(FIXP) ((FIXP)->tc_fix_data = NULL)
#define REPEAT_CONS_EXPRESSIONS

#define tc_frob_label(sym) kvx_frob_label(sym)
#define tc_check_label(sym) kvx_check_label(sym)
extern void kvx_frob_label (struct symbol *);
extern void kvx_check_label (struct symbol *);


/* GAS listings (enabled by `-a') */

#define LISTING_HEADER "KVX GAS LISTING"
#define LISTING_LHS_CONT_LINES 100


#define md_start_line_hook kvx_md_start_line_hook
extern void kvx_md_start_line_hook (void);
#define md_emit_single_noop kvx_emit_single_noop()
extern void kvx_emit_single_noop (void);

/* Values passed to md_apply_fix don't include the symbol value.  */
#define MD_APPLY_SYM_VALUE(FIX) 0

/* Allow O_subtract in expressionS.  */
#define DIFF_EXPR_OK 1

/* Controls the emission of relocations even when the symbol may be resolved
   directly by the assembler.  */
extern int kvx_force_reloc (struct fix *);
#undef TC_FORCE_RELOCATION
#define TC_FORCE_RELOCATION(fixP) kvx_force_reloc(fixP)

/* Force a relocation for global symbols.  */
#define EXTERN_FORCE_RELOC 1

/* Controls the resolution of fixup expressions involving the difference of two
   symbols.  */
extern int kvx_force_reloc_sub_same (struct fix *, segT);
#define TC_FORCE_RELOCATION_SUB_SAME(FIX, SEC)                                 \
  (! SEG_NORMAL (S_GET_SEGMENT((FIX)->fx_addsy))                               \
   || kvx_force_reloc_sub_same(FIX, SEC))

/* This expression evaluates to true if the relocation is for a local object
   for which we still want to do the relocation at runtime.  False if we are
   willing to perform this relocation while building the .o file.

   We can't resolve references to the GOT or the PLT when creating the object
   file, since these tables are only created by the linker.  Also, if the symbol
   is global, weak, common or not defined, the assembler can't compute the
   appropriate reloc, since its location can only be determined at link time.
   */

#define TC_FORCE_RELOCATION_LOCAL(FIX) \
  (!(FIX)->fx_pcrel || TC_FORCE_RELOCATION (FIX))

/* This expression evaluates to false if the relocation is for a local object
   for which we still want to do the relocation at runtime.  True if we are
   willing to perform this relocation while building the .o file. This is only
   used for pcrel relocations. Use this to ensure that a branch to a preemptible
   symbol is not resolved by the assembler. */

#define TC_RELOC_RTSYM_LOC_FIXUP(FIX)                                          \
  ((FIX)->fx_r_type != BFD_RELOC_KVX_23_PCREL                                  \
   || (FIX)->fx_addsy == NULL                                                  \
   || (! S_IS_EXTERNAL ((FIX)->fx_addsy)                                       \
       && ! S_IS_WEAK ((FIX)->fx_addsy)                                        \
       && S_IS_DEFINED ((FIX)->fx_addsy)                                       \
       && ! S_IS_COMMON ((FIX)->fx_addsy)))

/* Local symbols will be adjusted against the section symbol.  */
#define tc_fix_adjustable(fixP) 1

/* This arranges for gas/write.c to not apply a relocation if
   tc_fix_adjustable says it is not adjustable.  The "! symbol_used_in_reloc_p"
   test is there specifically to cover the case of non-global symbols in
   linkonce sections.  It's the generally correct thing to do though;  If a
   reloc is going to be emitted against a symbol then we don't want to adjust
   the fixup by applying the reloc during assembly.  The reloc will be applied
   by the linker during final link.  */
#define TC_FIX_ADJUSTABLE(fixP) \
  (! symbol_used_in_reloc_p ((fixP)->fx_addsy) && tc_fix_adjustable (fixP))

/* Force this to avoid -g to fail because of dwarf2 expression .L0 - .L0 */
extern int kvx_validate_sub_fix (struct fix *fix);
#define TC_VALIDATE_FIX_SUB(FIX, SEG)                                          \
  (((FIX)->fx_r_type == BFD_RELOC_32 || (FIX)->fx_r_type == BFD_RELOC_16)      \
   && kvx_validate_sub_fix((FIX)))

/* Generate a fix for a data allocation pseudo-op*/
#define TC_CONS_FIX_NEW(FRAG,OFF,LEN,EXP,RELOC) kvx_cons_fix_new(FRAG,OFF,LEN,EXP,RELOC)
extern void kvx_cons_fix_new (fragS *f, int where, int nbytes,
			      expressionS *exp, bfd_reloc_code_real_type);

/* No post-alignment of sections */
#define SUB_SEGMENT_ALIGN(SEG, FRCHAIN) 0

/* Enable special handling for the alignment directive.  */
extern void kvx_handle_align (fragS *);
#undef HANDLE_ALIGN
#define HANDLE_ALIGN kvx_handle_align

#ifdef OBJ_ELF

/* Enable CFI support.  */
#define TARGET_USE_CFIPOP 1

extern void kvx_cfi_frame_initial_instructions (void);
#undef tc_cfi_frame_initial_instructions
#define tc_cfi_frame_initial_instructions kvx_cfi_frame_initial_instructions

extern int kvx_regname_to_dw2regnum (const char *regname);
#undef tc_regname_to_dw2regnum
#define tc_regname_to_dw2regnum kvx_regname_to_dw2regnum

/* All KVX instructions are multiples of 32 bits.  */
#define DWARF2_LINE_MIN_INSN_LENGTH 1
#define DWARF2_DEFAULT_RETURN_COLUMN (KVX_RA_REGNO)
#define DWARF2_CIE_DATA_ALIGNMENT -4
#endif
#endif
