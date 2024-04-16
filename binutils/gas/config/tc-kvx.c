/* tc-kvx.c -- Assemble for the KVX ISA

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

#include "as.h"
#include "obstack.h"
#include "subsegs.h"
#include "tc-kvx.h"
#include "libiberty.h"

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#ifdef OBJ_ELF
#include "elf/kvx.h"
#include "dwarf2dbg.h"
#include "dw2gencfi.h"
#endif

#define D(args...) do { if(debug) fprintf(args); } while(0)

static void supported_cores (char buf[], size_t buflen);

#define NELEMS(a) ((int) (sizeof (a)/sizeof ((a)[0])))

#define STREQ(x,y) !strcmp(((x) ? (x) : ""), ((y) ? (y) : ""))
#define STRNEQ(x,y,n) !strncmp(((x) ? (x) : ""), ((y) ? (y) : ""),(n))

/* The PARALLEL_BIT is set to 0 when an instruction is the last of a bundle. */
#define PARALLEL_BIT (1u << 31)

/*TB begin*/
int size_type_function = 1;
/*TB end */

struct kvx_as_env env = {
  .params = {
    .abi = ELF_KVX_ABI_UNDEF,
    .osabi = ELFOSABI_NONE,
    .core = -1,
    .core_set = 0,
    .abi_set = 0,
    .osabi_set = 0,
    .pic_flags = 0,
    .arch_size = 64
  },
  .opts = {
    .march = NULL,
    .check_resource_usage = 1,
    .generate_illegal_code = 0,
    .dump_table = 0,
    .dump_insn = 0,
    .diagnostics = 1,
    .more = 1,
    .allow_all_sfr = 0
  }
};

/* This string should contain position in string where error occured. */

/* Default kvx_registers array. */
const struct kvx_Register *kvx_registers = NULL;
/* Default kvx_modifiers array. */
const char ***kvx_modifiers = NULL;
/* Default kvx_regfiles array. */
const int *kvx_regfiles = NULL;
/* Default values used if no assume directive is given */
const struct kvx_core_info *kvx_core_info = NULL;

/***********************************************/
/*    Generic Globals for GAS                  */
/***********************************************/

const char comment_chars[]        = "#";
const char line_comment_chars[]   = "#";
const char line_separator_chars[] = ";";
const char EXP_CHARS[]            = "eE";
const char FLT_CHARS[]            = "dD";
const int md_short_jump_size      = 0;
const int md_long_jump_size       = 0;

/***********************************************/
/*           Local Types                       */
/***********************************************/

/* a fix up record                       */

struct kvx_fixup
{
  /* The expression used.  */
  expressionS exp;
  /* The place in the frag where this goes.  */
  int where;
  /* The relocation.  */
  bfd_reloc_code_real_type reloc;
};

/* a single assembled instruction record */
/* may include immediate extension words  */
struct kvxinsn
{
  /* written out?  */
  int written;
  /* Opcode table entry for this insn */
  const struct kvxopc *opdef;
  /* length of instruction in words (1 or 2) */
  int len;
  /* insn is extended */
  int immx0;
  /* insn has two immx */
  int immx1;
  /* order to stabilize sort */
  int order;
  /* instruction words */
  uint32_t words[KVXMAXBUNDLEWORDS];	
  /* the number of fixups [0,2] */
  int nfixups;
  /* the actual fixups */
  struct kvx_fixup fixup[2];
};

typedef void (*print_insn_t) (struct kvxopc * op);
static print_insn_t print_insn = NULL;

/* Set to TRUE when we assemble instructions.  */
static bool assembling_insn = false;

#define NOIMMX -1

/* Was KVXMAXBUNDLEISSUE, changed because of NOPs */
static struct kvxinsn insbuf[KVXMAXBUNDLEWORDS];
static int insncnt = 0;
static struct kvxinsn immxbuf[KVXMAXBUNDLEWORDS];
static int immxcnt = 0;

static void
incr_immxcnt (void)
{
  immxcnt++;
  if (immxcnt >= KVXMAXBUNDLEWORDS)
    as_bad ("Max immx number exceeded: %d", immxcnt);
}

static void set_byte_counter (asection * sec, int value);
static void
set_byte_counter (asection * sec, int value)
{
  sec->target_index = value;
}

static int get_byte_counter (asection * sec);
int
get_byte_counter (asection * sec)
{
  return sec->target_index;
}

const char *
kvx_target_format (void)
{
  return env.params.arch_size == 64 ? "elf64-kvx" : "elf32-kvx";
}

/****************************************************/
/*  ASSEMBLER Pseudo-ops.  Some of this just        */
/*  extends the default definitions                 */
/*  others are KVX specific                          */
/****************************************************/

static void kvx_check_resources (int);
static void kvx_proc (int start);
static void kvx_endp (int start);
static void kvx_type (int start);

const pseudo_typeS md_pseudo_table[] = {
  /* override default 2-bytes */
  { "word",             cons,                  4 },

  /* KVX specific */
  { "dword",            cons,                  8 },

  /* Override align directives to have a boundary as argument (and not the
     power of two as in p2align) */
  { "align",            s_align_bytes,         0 },

  { "checkresources",   kvx_check_resources,   1 },
  { "nocheckresources", kvx_check_resources,   0 },

  { "proc",             kvx_proc,              1 },
  { "endp",             kvx_endp,              0 },

  { "type",             kvx_type,              0 },

#ifdef OBJ_ELF
  { "file",             dwarf2_directive_file, 0 },
  { "loc",              dwarf2_directive_loc,  0 },
#endif
  { NULL,               0,                     0 }
};


static int inside_bundle = 0;

/* Stores the labels inside bundles (typically debug labels) that need
   to be postponed to the next bundle. */
struct label_fix
{
  struct label_fix *next;
  symbolS *sym;
} *label_fixes = 0;

/*****************************************************/
/*   OPTIONS PROCESSING                              */
/*****************************************************/

const char *md_shortopts = "hV";	/* catted to std short options */

/* added to std long options */

#define OPTION_HEXFILE               (OPTION_MD_BASE + 0)
#define OPTION_MARCH                 (OPTION_MD_BASE + 4)
#define OPTION_CHECK_RESOURCES       (OPTION_MD_BASE + 5)
#define OPTION_NO_CHECK_RESOURCES    (OPTION_MD_BASE + 6)
#define OPTION_GENERATE_ILLEGAL_CODE (OPTION_MD_BASE + 7)
#define OPTION_DUMP_TABLE            (OPTION_MD_BASE + 8)
#define OPTION_PIC                   (OPTION_MD_BASE + 9)
#define OPTION_BIGPIC                (OPTION_MD_BASE + 10)
#define OPTION_NOPIC                 (OPTION_MD_BASE + 12)
#define OPTION_32                    (OPTION_MD_BASE + 13)
#define OPTION_DUMPINSN              (OPTION_MD_BASE + 15)
#define OPTION_ALL_SFR               (OPTION_MD_BASE + 16)
#define OPTION_DIAGNOSTICS           (OPTION_MD_BASE + 17)
#define OPTION_NO_DIAGNOSTICS        (OPTION_MD_BASE + 18)
#define OPTION_MORE                  (OPTION_MD_BASE + 19)
#define OPTION_NO_MORE               (OPTION_MD_BASE + 20)

struct option md_longopts[] = {
  { "march",                 required_argument, NULL, OPTION_MARCH                 },
  { "check-resources",       no_argument,       NULL, OPTION_CHECK_RESOURCES       },
  { "no-check-resources",    no_argument,       NULL, OPTION_NO_CHECK_RESOURCES    },
  { "generate-illegal-code", no_argument,       NULL, OPTION_GENERATE_ILLEGAL_CODE },
  { "dump-table",            no_argument,       NULL, OPTION_DUMP_TABLE            },
  { "mpic",                  no_argument,       NULL, OPTION_PIC                   },
  { "mPIC",                  no_argument,       NULL, OPTION_BIGPIC                },
  { "mnopic",                no_argument,       NULL, OPTION_NOPIC                 },
  { "m32",                   no_argument,       NULL, OPTION_32                    },
  { "dump-insn",             no_argument,       NULL, OPTION_DUMPINSN              },
  { "all-sfr",               no_argument,       NULL, OPTION_ALL_SFR               },
  { "diagnostics",           no_argument,       NULL, OPTION_DIAGNOSTICS           },
  { "no-diagnostics",        no_argument,       NULL, OPTION_NO_DIAGNOSTICS        },
  { "more",                  no_argument,       NULL, OPTION_MORE                  },
  { "no-more",               no_argument,       NULL, OPTION_NO_MORE               },
  { NULL,                    no_argument,       NULL, 0                            }
};

size_t md_longopts_size = sizeof (md_longopts);

int
md_parse_option (int c, const char *arg ATTRIBUTE_UNUSED)
{
  int find_core = 0;

  switch (c)
    {
    case 'h':
      md_show_usage (stdout);
      exit (EXIT_SUCCESS);
      break;

      /* -V: SVR4 argument to print version ID.  */
    case 'V':
      print_version_id ();
      exit (EXIT_SUCCESS);
      break;
    case OPTION_MARCH:
      env.opts.march = strdup (arg);
      for (int i = 0; i < KVXNUMCORES && !find_core; ++i)
	    if (!strcasecmp (env.opts.march, kvx_core_info_table[i]->name))
	      {
		kvx_core_info = kvx_core_info_table[i];
		kvx_registers = kvx_registers_table[i];
		kvx_modifiers = kvx_modifiers_table[i];
		kvx_regfiles = kvx_regfiles_table[i];

		find_core = 1;
		break;
	      }
      if (!find_core)
	{
	  char buf[100];
	  supported_cores (buf, sizeof (buf));
	  as_fatal ("Specified arch not supported [%s]", buf);
	}
      break;
    case OPTION_CHECK_RESOURCES:
      env.opts.check_resource_usage = 1;
      break;
    case OPTION_NO_CHECK_RESOURCES:
      env.opts.check_resource_usage = 0;
      break;
    case OPTION_GENERATE_ILLEGAL_CODE:
      env.opts.generate_illegal_code = 1;
      break;
    case OPTION_DUMP_TABLE:
      env.opts.dump_table = 1;
      break;
    case OPTION_DUMPINSN:
      env.opts.dump_insn = 1;
      break;
    case OPTION_ALL_SFR:
      env.opts.allow_all_sfr = 1;
      break;
    case OPTION_DIAGNOSTICS:
      env.opts.diagnostics = 1;
      break;
    case OPTION_NO_DIAGNOSTICS:
      env.opts.diagnostics = 0;
      break;
    case OPTION_MORE:
      env.opts.more = 1;
      break;
    case OPTION_NO_MORE:
      env.opts.more = 0;
      break;
    case OPTION_PIC:
      /* fallthrough, for now the same on KVX */
    case OPTION_BIGPIC:
      env.params.pic_flags |= ELF_KVX_ABI_PIC_BIT;
      break;
    case OPTION_NOPIC:
      env.params.pic_flags &= ~(ELF_KVX_ABI_PIC_BIT);
      break;
    case OPTION_32:
      env.params.arch_size = 32;
      break;

    default:
      return 0;
    }
  return 1;
}

void
md_show_usage (FILE * stream)
{
  char buf[100];
  supported_cores (buf, sizeof (buf));

  fprintf (stream, "\n"
"KVX specific options:\n\n"
"  --check-resources\t Perform minimal resource checking\n"
"  --march [%s]\t Select architecture\n"
"  -V \t\t\t Print assembler version number\n\n"
"  The options -M, --mri and -f are not supported in this assembler.\n", buf);
}

/**************************************************/
/*              UTILITIES                         */
/**************************************************/

/*
 * Read a value from to the object file
 */

static valueT md_chars_to_number (char *buf, int n);
valueT
md_chars_to_number (char *buf, int n)
{
  valueT val = 0;

  if (n > (int) sizeof (val) || n <= 0)
    abort ();

  while (n--)
    {
      val <<= 8;
      val |= (buf[n] & 0xff);
    }

  return val;
}

/* Returns the corresponding pseudo function matching SYM and to be
   used for data section */
static struct pseudo_func *
kvx_get_pseudo_func_data_scn (symbolS * sym)
{
  for (int i = 0; i < kvx_core_info->nb_pseudo_funcs; i++)
    if (sym == kvx_core_info->pseudo_funcs[i].sym
	&& kvx_core_info->pseudo_funcs[i].pseudo_relocs.single != BFD_RELOC_UNUSED)
	return &kvx_core_info->pseudo_funcs[i];
  return NULL;
}

/* Returns the corresponding pseudo function matching SYM and operand
   format OPND */
static struct pseudo_func *
kvx_get_pseudo_func2 (symbolS *sym, struct kvx_operand * opnd)
{
  for (int i = 0; i < kvx_core_info->nb_pseudo_funcs; i++)
    if (sym == kvx_core_info->pseudo_funcs[i].sym)
      for (int relidx = 0; relidx < opnd->reloc_nb; relidx++)
	if (opnd->relocs[relidx] == kvx_core_info->pseudo_funcs[i].pseudo_relocs.kreloc
	    && (env.params.arch_size == (int) kvx_core_info->pseudo_funcs[i].pseudo_relocs.avail_modes
	      || kvx_core_info->pseudo_funcs[i].pseudo_relocs.avail_modes == PSEUDO_ALL))
	  return &kvx_core_info->pseudo_funcs[i];

  return NULL;
}

static void
supported_cores (char buf[], size_t buflen)
{
  buf[0] = '\0';
  for (int i = 0; i < KVXNUMCORES; i++)
    {
      if (buf[0] == '\0')
	strcpy (buf, kvx_core_info_table[i]->name);
      else
	if ((strlen (buf) + 1 + strlen (kvx_core_info_table[i]->name) + 1) < buflen)
	  {
	    strcat (buf, "|");
	    strcat (buf, kvx_core_info_table[i]->name);
	  }
    }
}

/***************************************************/
/*   ASSEMBLE AN INSTRUCTION                       */
/***************************************************/

/*
 * Insert ARG into the operand described by OPDEF in instruction INSN
 * Returns 1 if the immediate extension (IMMX) has been
 * handled along with relocation, 0 if not.
 */
static int
insert_operand (struct kvxinsn *insn, struct kvx_operand *opdef,
		struct token_list *tok)
{
  uint64_t op = 0;
  struct kvx_bitfield *bfields = opdef->bfield;
  int bf_nb = opdef->bitfields;
  int immx_ready = 0;

  if (opdef->width == 0)
    return 0;

#define add_fixup(insn_, reloc_, exp_) \
  do { \
    (insn_)->fixup[(insn_)->nfixups].reloc = (reloc_); \
    (insn_)->fixup[(insn_)->nfixups].exp = (exp_);     \
    (insn_)->fixup[(insn_)->nfixups].where = 0;        \
    (insn_)->nfixups++;                                \
  } while (0)

#define add_immx(insn_, words_, reloc_, exp_, nfixups_, len_) \
  do { \
    immxbuf[immxcnt].words[0] = (words_);                \
    immxbuf[immxcnt].fixup[0].reloc = (reloc_);          \
    immxbuf[immxcnt].fixup[0].exp = (exp_);              \
    immxbuf[immxcnt].fixup[0].where = 0;                 \
    immxbuf[immxcnt].nfixups = (nfixups_);               \
    immxbuf[immxcnt].len = (len_);                       \
    /* decrement insn->len: immx part handled separately \
       from insn and must not be emited twice.  */       \
    (insn_)->len -= 1;                                   \
    incr_immxcnt ();                                     \
  } while (0)

#define chk_imm(core_, imm_) \
  (env.params.core == ELF_KVX_CORE_## core_ && opdef->type == (imm_))

  /* try to resolve the value */

  switch (tok->category)
    {
    case CAT_REGISTER:
      op = S_GET_VALUE (str_hash_find (env.reg_hash, tok->tok));
      op -= opdef->bias;
      op >>= opdef->shift;
      break;
    case CAT_MODIFIER:
      op = tok->val;
      op -= opdef->bias;
      op >>= opdef->shift;
      break;
    case CAT_IMMEDIATE:
      {
	char *ilp_save = input_line_pointer;
	input_line_pointer = tok->tok;
	expressionS exp = { 0 };
	expression (&exp);
	input_line_pointer = ilp_save;

	/* We are dealing with a pseudo-function.  */
	if (tok->tok[0] == '@')
	  {
	    if (insn->nfixups == 0)
	      {
		expressionS reloc_arg;
		reloc_arg = exp;
		reloc_arg.X_op = O_symbol;
		struct pseudo_func *pf =
		  kvx_get_pseudo_func2 (exp.X_op_symbol, opdef);
		/* S64 uses LO10/UP27/EX27 format (3 words), with one reloc in each words (3) */
		/* S43 uses LO10/EX6/UP27 format (2 words), with 2 relocs in main syllabes and 1 in extra word */
		/* S37 uses LO10/UP27 format (2 words), with one reloc in each word (2) */

		/* Beware that immxbuf must be filled in the same order as relocs should be emitted. */

		if (pf->pseudo_relocs.reloc_type == S64_LO10_UP27_EX27
		    || pf->pseudo_relocs.reloc_type == S43_LO10_UP27_EX6
		    || pf->pseudo_relocs.reloc_type == S37_LO10_UP27)
		  {
		    add_fixup (insn, pf->pseudo_relocs.reloc_lo10, reloc_arg);

		    insn->immx0 = immxcnt;
		    add_immx (insn, 0, pf->pseudo_relocs.reloc_up27,
			      reloc_arg, 1, 1);
		    immx_ready = 1;
		  }
		else if (pf->pseudo_relocs.reloc_type == S32_LO5_UP27)
		  {
		    add_fixup (insn, pf->pseudo_relocs.reloc_lo5, reloc_arg);

		    insn->immx0 = immxcnt;
		    add_immx (insn, 0, pf->pseudo_relocs.reloc_up27,
			      reloc_arg, 1, 1);
		    immx_ready = 1;
		  }
		else if (pf->pseudo_relocs.reloc_type == S16)
		  add_fixup (insn, pf->pseudo_relocs.single, reloc_arg);
		else
		  as_fatal ("Unexpected fixup");

		if (pf->pseudo_relocs.reloc_type == S64_LO10_UP27_EX27)
		  {
		    insn->immx1 = immxcnt;
		    add_immx (insn, 0, pf->pseudo_relocs.reloc_ex, reloc_arg,
			      1, 1);
		  }
		else if (pf->pseudo_relocs.reloc_type == S43_LO10_UP27_EX6)
		  add_fixup (insn, pf->pseudo_relocs.reloc_ex, reloc_arg);
	      }
	  }
	else
	  {
	    if (exp.X_op == O_constant)
	      {
		/* This is a immediate: either a regular immediate, or an
		   immediate that was saved in a variable through `.equ'.  */
		uint64_t sval = (int64_t) tok->val;
		op = opdef->flags & kvxSIGNED ? sval : tok->val;
		op >>= opdef->shift;
	      }
	    else if (exp.X_op == O_subtract)
	      as_fatal ("O_subtract not supported.");
	    else
	      {

		/* This is a symbol which needs a relocation.  */
		if (insn->nfixups == 0)
		  {
		    if (chk_imm (KV3_1, Immediate_kv3_v1_pcrel17)
			|| chk_imm (KV3_2, Immediate_kv3_v2_pcrel17)
			|| chk_imm (KV4_1, Immediate_kv4_v1_pcrel17))
		      add_fixup (insn, BFD_RELOC_KVX_PCREL17, exp);
		    else if (chk_imm (KV3_1, Immediate_kv3_v1_pcrel27)
			     || chk_imm (KV3_2, Immediate_kv3_v2_pcrel27)
			     || chk_imm (KV4_1, Immediate_kv4_v1_pcrel27))
		      add_fixup (insn, BFD_RELOC_KVX_PCREL27, exp);
		    else if (chk_imm (KV3_1, Immediate_kv3_v1_wrapped32)
			     || chk_imm (KV3_2, Immediate_kv3_v2_wrapped32)
			     || chk_imm (KV4_1, Immediate_kv4_v1_wrapped32))
		      {
			add_fixup (insn, BFD_RELOC_KVX_S32_LO5, exp);

			insn->immx0 = immxcnt;
			add_immx (insn, 0, BFD_RELOC_KVX_S32_UP27, exp, 1, 1);

			immx_ready = 1;
		      }
		    else if (chk_imm (KV3_1, Immediate_kv3_v1_signed10)
			     || chk_imm (KV3_2, Immediate_kv3_v2_signed10)
			     || chk_imm (KV4_1, Immediate_kv4_v1_signed10))
		      add_fixup (insn, BFD_RELOC_KVX_S37_LO10, exp);
		    else if (chk_imm (KV3_1, Immediate_kv3_v1_signed37)
			     || chk_imm (KV3_2, Immediate_kv3_v2_signed37)
			     || chk_imm (KV4_1, Immediate_kv4_v1_signed37))
		      {
			add_fixup (insn, BFD_RELOC_KVX_S37_LO10, exp);

			insn->immx0 = immxcnt;
			add_immx (insn, 0, BFD_RELOC_KVX_S37_UP27, exp, 1, 1);

			immx_ready = 1;
		      }
		    else if (chk_imm (KV3_1, Immediate_kv3_v1_signed43)
			     || chk_imm (KV3_2, Immediate_kv3_v2_signed43)
			     || chk_imm (KV4_1, Immediate_kv4_v1_signed43))
		      {
			add_fixup (insn, BFD_RELOC_KVX_S43_LO10, exp);
			add_fixup (insn, BFD_RELOC_KVX_S43_EX6, exp);

			insn->immx0 = immxcnt;
			add_immx (insn, insn->words[1],
				  BFD_RELOC_KVX_S43_UP27, exp, 1, 1);

			immx_ready = 1;
		      }
		    else if (chk_imm (KV3_1, Immediate_kv3_v1_wrapped64)
			     || chk_imm (KV3_2, Immediate_kv3_v2_wrapped64)
			     || chk_imm (KV4_1, Immediate_kv4_v1_wrapped64))
		      {
			add_fixup (insn, BFD_RELOC_KVX_S64_LO10, exp);

			insn->immx0 = immxcnt;
			add_immx (insn, insn->words[1],
				  BFD_RELOC_KVX_S64_UP27, exp, 1, 1);

			insn->immx1 = immxcnt;
			add_immx (insn, insn->words[2],
				  BFD_RELOC_KVX_S64_EX27, exp, 1, 1);

			immx_ready = 1;
		      }
		    else
		      as_fatal ("don't know how to generate a fixup record");
		    return immx_ready;
		  }
		else
		  as_fatal ("No room for fixup ");
	      }
	  }
      }
      break;
    default:
      break;
    }

  for (int bf_idx = 0; bf_idx < bf_nb; bf_idx++)
    {
      uint64_t value =
	((uint64_t) op >> bfields[bf_idx].from_offset);
      int j = 0;
      int to_offset = bfields[bf_idx].to_offset;
      value &= (1LL << bfields[bf_idx].size) - 1;
      j = to_offset / 32;
      to_offset = to_offset % 32;
      insn->words[j] |= (value << to_offset) & 0xffffffff;
    }

  return immx_ready;

#undef chk_imm
#undef add_immx
#undef add_fixup
}

/*
 * Given a set of operands and a matching instruction,
 * assemble it
 *
 */
static void
assemble_insn (const struct kvxopc * opcode, struct token_list *tok, struct kvxinsn *insn)
{
  unsigned immx_ready = 0;

  memset (insn, 0, sizeof (*insn));
  insn->opdef = opcode;
  for (int i = 0; i < opcode->wordcount; i++)
    {
      insn->words[i] = opcode->codewords[i].opcode;
      insn->len += 1;
    }

  insn->immx0 = NOIMMX;
  insn->immx1 = NOIMMX;

  struct token_list *tok_ = tok;
  struct kvx_operand **format = (struct kvx_operand **) opcode->format;

  while (tok_)
    {
      int ret = insert_operand (insn, *format, tok_);
      immx_ready |= ret;
      while ((tok_ = tok_->next) && tok_->category == CAT_SEPARATOR);
      format++;
    }

  // Handle immx if insert_operand did not already take care of that
  if (!immx_ready)
    {
      for (int i = 0; i < opcode->wordcount; i++)
	{
	  if (opcode->codewords[i].flags & kvxOPCODE_FLAG_IMMX0)
	    {
	      insn->immx0 = immxcnt;
	      immxbuf[immxcnt].words[0] = insn->words[i];
	      immxbuf[immxcnt].nfixups = 0;
	      immxbuf[immxcnt].len = 1;
	      insn->len -= 1;
	      incr_immxcnt ();
	    }
	  if (opcode->codewords[i].flags & kvxOPCODE_FLAG_IMMX1)
	    {
	      insn->immx1 = immxcnt;
	      immxbuf[immxcnt].words[0] = insn->words[i];
	      immxbuf[immxcnt].nfixups = 0;
	      immxbuf[immxcnt].len = 1;
	      insn->len -= 1;
	      incr_immxcnt ();
	    }
	}
    }
}

/* Emit an instruction from the instruction array into the object
 * file. INSN points to an element of the instruction array. STOPFLAG
 * is true if this is the last instruction in the bundle.
 *
 * Only handles main syllables of bundle. Immediate extensions are
 * handled by insert_operand.
 */
static void
emit_insn (struct kvxinsn * insn, int insn_pos, int stopflag)
{
  char *f;
  unsigned int image;

  /* if we are listing, attach frag to previous line.  */
  if (listing)
    listing_prev_line ();

  /* Update text size for lane parity checking.  */
  set_byte_counter (now_seg, (get_byte_counter (now_seg) + (insn->len * 4)));

  /* allocate space in the fragment.  */
  f = frag_more (insn->len * 4);

  /* spit out bits.  */
  for (int i = 0; i < insn->len; i++)
    {
      image = insn->words[i];

      /* Handle bundle parallel bit. */ ;
      if ((i == insn->len - 1) && stopflag)
	image &= ~PARALLEL_BIT;
      else
	image |= PARALLEL_BIT;

      /* Emit the instruction image. */
      md_number_to_chars (f + (i * 4), image, 4);
    }

  /* generate fixup records */

  for (int i = 0; i < insn->nfixups; i++)
    {
      int size, pcrel;
      reloc_howto_type *reloc_howto =
	bfd_reloc_type_lookup (stdoutput, insn->fixup[i].reloc);
      assert (reloc_howto);
      size = bfd_get_reloc_size (reloc_howto);
      pcrel = reloc_howto->pc_relative;

      /* In case the PCREL relocation is not for the first insn in the
         bundle, we have to offset it.  The pc used by the hardware
         references a bundle and not separate insn.
       */
      assert (!(insn_pos == -1 && pcrel));
      if (pcrel && insn_pos > 0)
	insn->fixup[i].exp.X_add_number += insn_pos * 4;

      fixS *fixup = fix_new_exp (frag_now,
				 f - frag_now->fr_literal +
				 insn->fixup[i].where,
				 size,
				 &(insn->fixup[i].exp),
				 pcrel,
				 insn->fixup[i].reloc);
      /*
       * Set this bit so that large value can still be
       * handled. Without it, assembler will fail in fixup_segment
       * when it checks there is enough bits to store the value. As we
       * usually split our reloc across different words, it may think
       * that 4 bytes are not enough for large value. This simply
       * skips the tests
       */
      fixup->fx_no_overflow = 1;
    }
}


/* Called for any expression that can not be recognized.  When the
 * function is called, `input_line_pointer' will point to the start of
 * the expression.  */
/* FIXME: Should be done by the parser */
void
md_operand (expressionS * e)
{
  /* enum pseudo_type pseudo_type; */
  /* char *name = NULL; */
  size_t len;
  int ch, i;

  switch (*input_line_pointer)
    {
    case '@':
      /* Find what relocation pseudo-function we're dealing with. */
      /* pseudo_type = 0; */
      ch = *++input_line_pointer;
      for (i = 0; i < kvx_core_info->nb_pseudo_funcs; ++i)
	if (kvx_core_info->pseudo_funcs[i].name && kvx_core_info->pseudo_funcs[i].name[0] == ch)
	  {
	    len = strlen (kvx_core_info->pseudo_funcs[i].name);
	    if (strncmp (kvx_core_info->pseudo_funcs[i].name + 1,
			 input_line_pointer + 1, len - 1) == 0
		&& !is_part_of_name (input_line_pointer[len]))
	      {
		input_line_pointer += len;
		break;
	      }
	  }
      SKIP_WHITESPACE ();
      if (*input_line_pointer != '(')
	{
	  as_bad ("Expected '('");
	  goto err;
	}
      /* Skip '('.  */
      ++input_line_pointer;
      if (!kvx_core_info->pseudo_funcs[i].pseudo_relocs.has_no_arg)
	expression (e);
      if (*input_line_pointer++ != ')')
	{
	  as_bad ("Missing ')'");
	  goto err;
	}
      if (!kvx_core_info->pseudo_funcs[i].pseudo_relocs.has_no_arg)
	{
	  if (e->X_op != O_symbol)
	    as_fatal ("Illegal combination of relocation functions");
	}
      /* Make sure gas doesn't get rid of local symbols that are used
         in relocs.  */
      e->X_op = O_pseudo_fixup;
      e->X_op_symbol = kvx_core_info->pseudo_funcs[i].sym;
      break;

    default:
      break;
    }
  return;

err:
  ignore_rest_of_line ();
}

/*
 * Return the Bundling type for an insn.
 */
static int
find_bundling (const struct kvxinsn * insn)
{
  return insn->opdef->bundling;
}

static int
find_reservation (const struct kvxinsn * insn)
{
  return insn->opdef->reservation;
}

static struct kvxopc *
assemble_tokens (struct token_list *tok_list)
{
  assert (tok_list != NULL);
  struct token_list *toks = tok_list;

  /* make sure there is room in instruction buffer */
  /* Was KVXMAXBUNDLEISSUE, changed because of NOPs */
  if (insncnt >= KVXMAXBUNDLEWORDS)
    as_fatal ("[assemble_tokens]: too many instructions in bundle.");

  /* TODO: Merge */
  struct kvxinsn *insn;
  insn = insbuf + insncnt;

  /* The formats table registers the modifier into the opcode, therefore we need
     to fuse both before looking up the opcodes hashtable.  */
  char *opcode = NULL;

  opcode = toks->tok;
  toks = toks->next;

  while (toks && toks->category == CAT_SEPARATOR)
    toks = toks->next;

  /* Find the format requested by the instruction.  */
  struct kvxopc *format_tbl = str_hash_find (env.opcode_hash, opcode);
  struct kvxopc *format = NULL;

  struct token_list *toks_ = toks;

  while (!format && format_tbl && STREQ (opcode, format_tbl->as_op))
  {
    for (int i = 0 ; toks_ && format_tbl->format[i]
	&& toks_->class_id == format_tbl->format[i]->type ;)
      {
	toks_ = toks_->next;
	while (toks_ && toks_->category == CAT_SEPARATOR)
	  toks_ = toks_->next;
	i += 1;
      }

    if (!toks_)
      format = format_tbl;
    else
      {
	toks_ = toks;
	format_tbl++;
      }
  }

  assert (format != NULL);

  assemble_insn (format, toks, insn);
  insncnt++;

  return NULL;
}

/*
 * Write in buf at most buf_size.
 * Returns the number of writen characters.
 */
static int ATTRIBUTE_UNUSED
insn_syntax (struct kvxopc * op, char *buf, int buf_size)
{
  int chars = snprintf (buf, buf_size, "%s ", op->as_op);
  const char *fmtp = op->fmtstring;
  char ch = 0;

  for (int i = 0; op->format[i]; i++)
    {
      int type = op->format[i]->type;
      const char *type_name = TOKEN_NAME (type);
      int offset = 0;

      for (int j = 0 ; type_name[j] ; ++j)
	if (type_name[j] == '_')
	  offset = j + 1;

      /* Print characters in the format string up to the following * % or nul. */
      while ((chars < buf_size) && (ch = *fmtp) && ch != '%')
	{
	  buf[chars++] = ch;
	  fmtp++;
	}

      /* Skip past %s */
      if (ch == '%')
	{
	  ch = *fmtp++;
	  fmtp++;
	}

      chars += snprintf (&buf[chars], buf_size - chars, "%s", type_name + offset);
    }

  /* Print trailing characters in the format string, if any */
  while ((chars < buf_size) && (ch = *fmtp))
    {
      buf[chars++] = ch;
      fmtp++;
    }

  if (chars < buf_size)
    buf[chars++] = '\0';
  else
    buf[buf_size - 1] = '\0';

  return chars;
}

#define ASM_CHARS_MAX (71)

static void
kvx_print_insn (struct kvxopc * op ATTRIBUTE_UNUSED)
{
  char asm_str[ASM_CHARS_MAX];
  int chars = insn_syntax (op, asm_str, ASM_CHARS_MAX);
  const char *insn_type = "UNKNOWN";
  const char *insn_mode = "";

  for (int i = chars - 1; i < ASM_CHARS_MAX - 1; i++)
    asm_str[i] = '-';

  /* This is a hack which works because the Bundling is the same for all cores
     for now.  */
  switch ((int) op->bundling)
    {
    case Bundling_kv3_v1_ALL:
      insn_type = "ALL  ";
      break;
    case Bundling_kv3_v1_BCU:
      insn_type = "BCU  ";
      break;
    case Bundling_kv3_v1_TCA:
      insn_type = "TCA  ";
      break;
    case Bundling_kv3_v1_FULL:
    case Bundling_kv3_v1_FULL_X:
    case Bundling_kv3_v1_FULL_Y:
      insn_type = "FULL ";
      break;
    case Bundling_kv3_v1_LITE:
    case Bundling_kv3_v1_LITE_X:
    case Bundling_kv3_v1_LITE_Y:
      insn_type = "LITE ";
      break;
    case Bundling_kv3_v1_TINY:
    case Bundling_kv3_v1_TINY_X:
    case Bundling_kv3_v1_TINY_Y:
      insn_type = "TINY ";
      break;
    case Bundling_kv3_v1_MAU:
    case Bundling_kv3_v1_MAU_X:
    case Bundling_kv3_v1_MAU_Y:
      insn_type = "MAU  ";
      break;
    case Bundling_kv3_v1_LSU:
    case Bundling_kv3_v1_LSU_X:
    case Bundling_kv3_v1_LSU_Y:
      insn_type = "LSU  ";
      break;
    case Bundling_kv3_v1_NOP:
      insn_type = "NOP  ";
      break;
    default:
      as_fatal ("Unhandled Bundling class %d", op->bundling);
    }

  if (op->codewords[0].flags & kvxOPCODE_FLAG_MODE64
      && op->codewords[0].flags & kvxOPCODE_FLAG_MODE32)
    insn_mode = "32 and 64";
  else if (op->codewords[0].flags & kvxOPCODE_FLAG_MODE64)
    insn_mode = "64";
  else if (op->codewords[0].flags & kvxOPCODE_FLAG_MODE32)
    insn_mode = "32";
  else
    as_fatal ("Unknown instruction mode.");

  printf ("%s | syllables: %d | type: %s | mode: %s bits\n", asm_str,
	  op->wordcount, insn_type, insn_mode);
}

/* Comparison function compatible with qsort.  This is used to sort the issues
   into the right order.  */
static int
kvxinsn_compare (const void *a, const void *b)
{
  struct kvxinsn *kvxinsn_a = *(struct kvxinsn **) a;
  struct kvxinsn *kvxinsn_b = *(struct kvxinsn **) b;
  int bundling_a = find_bundling (kvxinsn_a);
  int bundling_b = find_bundling (kvxinsn_b);
  int order_a = kvxinsn_a->order;
  int order_b = kvxinsn_b->order;
  if (bundling_a != bundling_b)
    return (bundling_b < bundling_a) - (bundling_a < bundling_b);
  return (order_b < order_a) - (order_a < order_b);
}

static void
kvx_reorder_bundle (struct kvxinsn *bundle_insn[], int bundle_insncnt)
{
  enum
  { EXU_BCU, EXU_TCA, EXU_ALU0, EXU_ALU1, EXU_MAU, EXU_LSU, EXU__ };
  struct kvxinsn *issued[EXU__];
  int tag, exu;

  memset (issued, 0, sizeof (issued));
  for (int i = 0; i < bundle_insncnt; i++)
    {
      struct kvxinsn *kvxinsn = bundle_insn[i];
      tag = -1, exu = -1;
      /* This is a hack. It works because all the Bundling are the same for all
         cores for now.  */
      switch ((int) find_bundling (kvxinsn))
	{
	case Bundling_kv3_v1_ALL:
	  if (bundle_insncnt > 1)
	    as_fatal ("Too many ops in a single op bundle");
	  issued[0] = kvxinsn;
	  break;
	case Bundling_kv3_v1_BCU:
	  if (!issued[EXU_BCU])
	    issued[EXU_BCU] = kvxinsn;
	  else
	    as_fatal ("More than one BCU instruction in bundle");
	  break;
	case Bundling_kv3_v1_TCA:
	  if (!issued[EXU_TCA])
	    issued[EXU_TCA] = kvxinsn;
	  else
	    as_fatal ("More than one TCA instruction in bundle");
	  break;
	case Bundling_kv3_v1_FULL:
	case Bundling_kv3_v1_FULL_X:
	case Bundling_kv3_v1_FULL_Y:
	  if (!issued[EXU_ALU0])
	    {
	      issued[EXU_ALU0] = kvxinsn;
	      tag = Modifier_kv3_v1_exunum_ALU0;
	      exu = EXU_ALU0;
	    }
	  else
	    as_fatal ("More than one ALU FULL instruction in bundle");
	  break;
	case Bundling_kv3_v1_LITE:
	case Bundling_kv3_v1_LITE_X:
	case Bundling_kv3_v1_LITE_Y:
	  if (!issued[EXU_ALU0])
	    {
	      issued[EXU_ALU0] = kvxinsn;
	      tag = Modifier_kv3_v1_exunum_ALU0;
	      exu = EXU_ALU0;
	    }
	  else if (!issued[EXU_ALU1])
	    {
	      issued[EXU_ALU1] = kvxinsn;
	      tag = Modifier_kv3_v1_exunum_ALU1;
	      exu = EXU_ALU1;
	    }
	  else
	    as_fatal ("Too many ALU FULL or LITE instructions in bundle");
	  break;
	case Bundling_kv3_v1_MAU:
	case Bundling_kv3_v1_MAU_X:
	case Bundling_kv3_v1_MAU_Y:
	  if (!issued[EXU_MAU])
	    {
	      issued[EXU_MAU] = kvxinsn;
	      tag = Modifier_kv3_v1_exunum_MAU;
	      exu = EXU_MAU;
	    }
	  else
	    as_fatal ("More than one MAU instruction in bundle");
	  break;
	case Bundling_kv3_v1_LSU:
	case Bundling_kv3_v1_LSU_X:
	case Bundling_kv3_v1_LSU_Y:
	  if (!issued[EXU_LSU])
	    {
	      issued[EXU_LSU] = kvxinsn;
	      tag = Modifier_kv3_v1_exunum_LSU;
	      exu = EXU_LSU;
	    }
	  else
	    as_fatal ("More than one LSU instruction in bundle");
	  break;
	case Bundling_kv3_v1_TINY:
	case Bundling_kv3_v1_TINY_X:
	case Bundling_kv3_v1_TINY_Y:
	case Bundling_kv3_v1_NOP:
	  if (!issued[EXU_ALU0])
	    {
	      issued[EXU_ALU0] = kvxinsn;
	      tag = Modifier_kv3_v1_exunum_ALU0;
	      exu = EXU_ALU0;
	    }
	  else if (!issued[EXU_ALU1])
	    {
	      issued[EXU_ALU1] = kvxinsn;
	      tag = Modifier_kv3_v1_exunum_ALU1;
	      exu = EXU_ALU1;
	    }
	  else if (!issued[EXU_MAU])
	    {
	      issued[EXU_MAU] = kvxinsn;
	      tag = Modifier_kv3_v1_exunum_MAU;
	      exu = EXU_MAU;
	    }
	  else if (!issued[EXU_LSU])
	    {
	      issued[EXU_LSU] = kvxinsn;
	      tag = Modifier_kv3_v1_exunum_LSU;
	      exu = EXU_LSU;
	    }
	  else
	    as_fatal ("Too many ALU instructions in bundle");
	  break;
	default:
	  as_fatal ("Unhandled Bundling class %d", find_bundling (kvxinsn));
	}
      if (tag >= 0)
	{
	  if (issued[exu]->immx0 != NOIMMX)
	    immxbuf[issued[exu]->immx0].words[0] |= (tag << 27);
	  if (issued[exu]->immx1 != NOIMMX)
	    immxbuf[issued[exu]->immx1].words[0] |= (tag << 27);
	}
    }

  int i;
  for (i = 0, exu = 0; exu < EXU__; exu++)
    {
      if (issued[exu])
	bundle_insn[i++] = issued[exu];
    }
  if (i != bundle_insncnt)
    as_fatal ("Mismatch between bundle and issued instructions");
}

static void
kvx_check_resource_usage (struct kvxinsn **bundle_insn, int bundle_insncnt)
{
  const int reservation_table_len =
    (kvx_core_info->reservation_table_lines * kvx_core_info->resource_max);
  const int *resources = kvx_core_info->resources;
  int *resources_used =
    malloc (reservation_table_len * sizeof (int));
  memset (resources_used, 0, reservation_table_len * sizeof (int));

  for (int i = 0; i < bundle_insncnt; i++)
  {
    int insn_reservation = find_reservation (bundle_insn[i]);
    int reservation = insn_reservation & 0xff;
    const int *reservation_table = kvx_core_info->reservation_table_table[reservation];
    for (int j = 0; j < reservation_table_len; j++)
      resources_used[j] += reservation_table[j];
  }

  for (int i = 0; i < kvx_core_info->reservation_table_lines; i++)
    {
      for (int j = 0; j < kvx_core_info->resource_max; j++)
	if (resources_used[(i * kvx_core_info->resource_max) + j] > resources[j])
	  {
	    int v = resources_used[(i * kvx_core_info->resource_max) + j];
	    free (resources_used);
	    as_fatal ("Resource %s over-used in bundle: %d used, %d available",
		kvx_core_info->resource_names[j], v, resources[j]);
	  }
  }
  free (resources_used);
}

/*
 * Called by core to assemble a single line
 */
void
md_assemble (char *line)
{
  char *line_cursor = line;

  if (get_byte_counter (now_seg) & 3)
    as_fatal ("code segment not word aligned in md_assemble");

  while (line_cursor && line_cursor[0] && (line_cursor[0] == ' '))
    line_cursor++;

  /* ;; was converted to "be" by line hook          */
  /* here we look for the bundle end                */
  /* and actually output any instructions in bundle */
  /* also we need to implement the stop bit         */
  /* check for bundle end */
  if (strncmp (line_cursor, "be", 2) == 0)
  {
    inside_bundle = 0;
    //int sec_align = bfd_get_section_alignment(stdoutput, now_seg);
    /* Was KVXMAXBUNDLEISSUE, changed because of NOPs */
    struct kvxinsn *bundle_insn[KVXMAXBUNDLEWORDS];
    int bundle_insncnt = 0;
    int syllables = 0;
    int entry;

#ifdef OBJ_ELF
    /* Emit Dwarf debug line information */
    dwarf2_emit_insn (0);
#endif
    for (int j = 0; j < insncnt; j++)
    {
      insbuf[j].order = j;
      bundle_insn[bundle_insncnt++] = &insbuf[j];
      syllables += insbuf[j].len;
    }

    if (syllables + immxcnt > KVXMAXBUNDLEWORDS)
      as_fatal ("Bundle has too many syllables : %d instead of %d",
	  syllables + immxcnt, KVXMAXBUNDLEWORDS);

    if (env.opts.check_resource_usage)
      kvx_check_resource_usage (bundle_insn, bundle_insncnt);

    /* Reorder and check the bundle.  */
    if (!env.opts.generate_illegal_code)
    {
      /* Sort the bundle_insn in order of bundling. */
      qsort (bundle_insn, bundle_insncnt, sizeof (struct kvxinsn *), kvxinsn_compare);

      kvx_reorder_bundle (bundle_insn, bundle_insncnt);
    }

    /* The ordering of the insns has been set correctly in bundle_insn. */
    for (int i = 0; i < bundle_insncnt; i++)
    {
      emit_insn (bundle_insn[i], i, (i == bundle_insncnt + immxcnt - 1));
      bundle_insn[i]->written = 1;
    }

    // Emit immx, ordering them by EXU tags, 0 to 3
    entry = 0;
    for (int tag = 0; tag < 4; tag++)
    {
      for (int j = 0; j < immxcnt; j++)
      {
#define kv3_exunum2_fld(x) (int)(((unsigned int)(x) >> 27) & 0x3)
	if (kv3_exunum2_fld (immxbuf[j].words[0]) == tag)
	{
	  assert (immxbuf[j].written == 0);
	  int insn_pos = bundle_insncnt + entry;
	  emit_insn (&(immxbuf[j]), insn_pos, entry == immxcnt - 1);
	  immxbuf[j].written = 1;
	  entry++;
	}
#undef kv3_exunum2_fld
      }
    }
    if (entry != immxcnt)
      as_fatal ("%d IMMX produced, only %d emitted.", immxcnt, entry);

    /* The debug label that appear in the middle of bundles
       had better appear to be attached to the next
       bundle. This is because usually these labels point to
       the first instruction where some condition is met. If
       the label isn't handled this way it will be attached to
       the current bundle which is wrong as the corresponding
       instruction wasn't executed yet. */
    while (label_fixes)
    {
      struct label_fix *fix = label_fixes;

      label_fixes = fix->next;
      symbol_set_value_now (fix->sym);
      free (fix);
    }

    insncnt = 0;
    immxcnt = 0;
    memset (immxbuf, 0, sizeof (immxbuf));

    return;
  }

    char *buf = NULL;
    sscanf (line_cursor, "%m[^\n]", &buf);
    struct token_s my_tok = { .insn = buf, .begin = 0, .end = 0, .class_id = -1 , .val = 0 };
    struct token_list *tok_lst = parse (my_tok);
    free (buf);

    if (!tok_lst)
      return;

    /* Skip opcode */
    line_cursor += strlen (tok_lst->tok);

  assembling_insn = true;

  inside_bundle = 1;
  assemble_tokens (tok_lst);
  free_token_list (tok_lst);
  assembling_insn = false;
}

static void
kvx_set_cpu (void)
{
  if (!kvx_core_info)
    kvx_core_info = &kvx_kv3_v1_core_info;

  if (!kvx_registers)
    kvx_registers = kvx_kv3_v1_registers;

  if (!kvx_regfiles)
    kvx_regfiles = kvx_kv3_v1_regfiles;

  if (!kvx_modifiers)
    kvx_modifiers = kvx_kv3_v1_modifiers;

  if (env.params.core == -1)
      env.params.core = kvx_core_info->elf_core;

  int kvx_bfd_mach;
  print_insn = kvx_print_insn;

  switch (kvx_core_info->elf_core)
    {
    case ELF_KVX_CORE_KV3_1:
      kvx_bfd_mach = env.params.arch_size == 32 ? bfd_mach_kv3_1 : bfd_mach_kv3_1_64;
      setup (ELF_KVX_CORE_KV3_1);
      break;
    case ELF_KVX_CORE_KV3_2:
      kvx_bfd_mach = env.params.arch_size == 32 ? bfd_mach_kv3_2 : bfd_mach_kv3_2_64;
      setup (ELF_KVX_CORE_KV3_2);
      break;
    case ELF_KVX_CORE_KV4_1:
      kvx_bfd_mach = env.params.arch_size == 32 ? bfd_mach_kv4_1 : bfd_mach_kv4_1_64;
      setup (ELF_KVX_CORE_KV4_1);
      break;
    default:
      as_fatal ("Unknown elf core: 0x%x", kvx_core_info->elf_core);
    }

  if (!bfd_set_arch_mach (stdoutput, TARGET_ARCH, kvx_bfd_mach))
    as_warn (_("could not set architecture and machine"));
}

static int
kvxop_compar (const void *a, const void *b)
{
  const struct kvxopc *opa = (const struct kvxopc *) a;
  const struct kvxopc *opb = (const struct kvxopc *) b;
  int res = strcmp (opa->as_op, opb->as_op);

  if (res)
    return res;
  else
    {
      for (int i = 0; opa->format[i] && opb->format[i]; ++i)
	if (opa->format[i]->width != opb->format[i]->width)
	  return opa->format[i]->width - opb->format[i]->width;
      return 0;
    }
}

/***************************************************/
/*    INITIALIZE ASSEMBLER                         */
/***************************************************/

static int
print_hash (void **slot, void *arg ATTRIBUTE_UNUSED)
{
  string_tuple_t *tuple = *((string_tuple_t **) slot);
  printf ("%s\n", tuple->key);
  return 1;
}

static void
declare_register (const char *name, int number)
{
  symbolS *regS = symbol_create (name, reg_section,
				 &zero_address_frag, number);

  if (str_hash_insert (env.reg_hash, S_GET_NAME (regS), regS, 0) != NULL)
    as_fatal (_("duplicate %s"), name);
}

void
md_begin ()
{
  kvx_set_cpu ();

  /*
   * Declare register names with symbols
   */

  env.reg_hash = str_htab_create ();

  for (int i = 0; i < kvx_regfiles[KVX_REGFILE_REGISTERS]; i++)
    declare_register (kvx_registers[i].name, kvx_registers[i].id);

  /* Sort optab, so that identical mnemonics appear consecutively */
  {
    int nel;
    for (nel = 0; !STREQ ("", kvx_core_info->optab[nel].as_op); nel++)
      ;
    qsort (kvx_core_info->optab, nel, sizeof (kvx_core_info->optab[0]),
	   kvxop_compar);
  }

  /* The '?' is an operand separator */
  lex_type['?'] = 0;

  /* Create the opcode hash table      */
  /* Each name should appear only once */

  env.opcode_hash = str_htab_create ();
  env.reloc_hash = str_htab_create ();

  {
    struct kvxopc *op;
    const char *name = 0;
    for (op = kvx_core_info->optab; !(STREQ ("", op->as_op)); op++)
      {
	/* enter in hash table if this is a new name */
	if (!(STREQ (name, op->as_op)))
	  {
	    name = op->as_op;
	    if (str_hash_insert (env.opcode_hash, name, op, 0))
	      as_fatal ("internal error: can't hash opcode `%s'", name);
	  }


	for (int i = 0 ; op->format[i] ; ++i)
	  {
	    const char *reloc_name = TOKEN_NAME (op->format[i]->type);
	    void *relocs = op->format[i]->relocs;
	    if (op->format[i]->relocs[0] != 0
		&& !str_hash_find (env.reloc_hash, reloc_name))
	      if (str_hash_insert (env.reloc_hash, reloc_name, relocs, 0))
		  as_fatal ("internal error: can't hash type `%s'", reloc_name);
	  }
      }
  }

  if (env.opts.dump_table)
    {
      htab_traverse (env.opcode_hash, print_hash, NULL);
      exit (0);
    }

  if (env.opts.dump_insn)
    {
      for (struct kvxopc *op = kvx_core_info->optab; !(STREQ ("", op->as_op)); op++)
	print_insn (op);
      exit (0);
    }

  /* Here we enforce the minimum section alignment.  Remember, in
   * the linker we can make the boudaries between the linked sections
   * on larger boundaries.  The text segment is aligned to long words
   * because of the odd/even constraint on immediate extensions
   */

  bfd_set_section_alignment (text_section, 3);	/* -- 8 bytes */
  bfd_set_section_alignment (data_section, 2);	/* -- 4 bytes */
  bfd_set_section_alignment (bss_section, 2);	/* -- 4 bytes */
  subseg_set (text_section, 0);

  symbolS *gotoff_sym   = symbol_create (".<gotoff>",   undefined_section, &zero_address_frag, 0);
  symbolS *got_sym      = symbol_create (".<got>",      undefined_section, &zero_address_frag, 0);
  symbolS *plt_sym      = symbol_create (".<plt>",      undefined_section, &zero_address_frag, 0);
  symbolS *tlsgd_sym    = symbol_create (".<tlsgd>",    undefined_section, &zero_address_frag, 0);
  symbolS *tlsie_sym    = symbol_create (".<tlsie>",    undefined_section, &zero_address_frag, 0);
  symbolS *tlsle_sym    = symbol_create (".<tlsle>",    undefined_section, &zero_address_frag, 0);
  symbolS *tlsld_sym    = symbol_create (".<tlsld>",    undefined_section, &zero_address_frag, 0);
  symbolS *dtpoff_sym   = symbol_create (".<dtpoff>",   undefined_section, &zero_address_frag, 0);
  symbolS *plt64_sym    = symbol_create (".<plt64>",    undefined_section, &zero_address_frag, 0);
  symbolS *gotaddr_sym  = symbol_create (".<gotaddr>",  undefined_section, &zero_address_frag, 0);
  symbolS *pcrel16_sym  = symbol_create (".<pcrel16>",  undefined_section, &zero_address_frag, 0);
  symbolS *pcrel_sym    = symbol_create (".<pcrel>",    undefined_section, &zero_address_frag, 0);
  symbolS *signed32_sym = symbol_create (".<signed32>", undefined_section, &zero_address_frag, 0);

  for (int i = 0; i < kvx_core_info->nb_pseudo_funcs; ++i)
    {
      symbolS *sym;
      if (!strcmp (kvx_core_info->pseudo_funcs[i].name, "gotoff"))
	sym = gotoff_sym;
      else if (!strcmp (kvx_core_info->pseudo_funcs[i].name, "got"))
	sym = got_sym;
      else if (!strcmp (kvx_core_info->pseudo_funcs[i].name, "plt"))
	sym = plt_sym;
      else if (!strcmp (kvx_core_info->pseudo_funcs[i].name, "tlsgd"))
	sym = tlsgd_sym;
      else if (!strcmp (kvx_core_info->pseudo_funcs[i].name, "tlsle"))
	sym = tlsle_sym;
      else if (!strcmp (kvx_core_info->pseudo_funcs[i].name, "tlsld"))
	sym = tlsld_sym;
      else if (!strcmp (kvx_core_info->pseudo_funcs[i].name, "dtpoff"))
	sym = dtpoff_sym;
      else if (!strcmp (kvx_core_info->pseudo_funcs[i].name, "tlsie"))
	sym = tlsie_sym;
      else if (!strcmp (kvx_core_info->pseudo_funcs[i].name, "plt64"))
	sym = plt64_sym;
      else if (!strcmp (kvx_core_info->pseudo_funcs[i].name, "pcrel16"))
	sym = pcrel16_sym;
      else if (!strcmp (kvx_core_info->pseudo_funcs[i].name, "pcrel"))
	sym = pcrel_sym;
      else if (!strcmp (kvx_core_info->pseudo_funcs[i].name, "gotaddr"))
	sym = gotaddr_sym;
      else if (!strcmp (kvx_core_info->pseudo_funcs[i].name, "signed32"))
	sym = signed32_sym;
      else
	as_fatal ("internal error: Unknown pseudo func `%s'",
	    kvx_core_info->pseudo_funcs[i].name);

      kvx_core_info->pseudo_funcs[i].sym = sym;
    }
}

/***************************************************/
/*          ASSEMBLER CLEANUP STUFF                */
/***************************************************/

/* Return non-zero if the indicated VALUE has overflowed the maximum
   range expressible by a signed number with the indicated number of
   BITS.

   This is from tc-aarch64.c
*/

static bfd_boolean
signed_overflow (offsetT value, unsigned bits)
{
  offsetT lim;
  if (bits >= sizeof (offsetT) * 8)
    return FALSE;
  lim = (offsetT) 1 << (bits - 1);
  return (value < -lim || value >= lim);
}

/***************************************************/
/*          ASSEMBLER FIXUP STUFF                  */
/***************************************************/

void
md_apply_fix (fixS * fixP, valueT * valueP, segT segmentP ATTRIBUTE_UNUSED)
{
  char *const fixpos = fixP->fx_frag->fr_literal + fixP->fx_where;
  valueT value = *valueP;
  valueT image;
  arelent *rel;

  rel = (arelent *) xmalloc (sizeof (arelent));

  rel->howto = bfd_reloc_type_lookup (stdoutput, fixP->fx_r_type);
  if (rel->howto == NULL)
    {
      as_fatal
	("[md_apply_fix] unsupported relocation type (can't find howto)");
    }

  /* Note whether this will delete the relocation.  */
  if (fixP->fx_addsy == NULL && fixP->fx_pcrel == 0)
    fixP->fx_done = 1;

  if (fixP->fx_size > 0)
    image = md_chars_to_number (fixpos, fixP->fx_size);
  else
    image = 0;
  if (fixP->fx_addsy != NULL)
    {
      switch (fixP->fx_r_type)
	{
	case BFD_RELOC_KVX_S37_TLS_LE_UP27:
	case BFD_RELOC_KVX_S37_TLS_LE_LO10:

	case BFD_RELOC_KVX_S43_TLS_LE_EX6:
	case BFD_RELOC_KVX_S43_TLS_LE_UP27:
	case BFD_RELOC_KVX_S43_TLS_LE_LO10:

	case BFD_RELOC_KVX_S37_TLS_GD_LO10:
	case BFD_RELOC_KVX_S37_TLS_GD_UP27:

	case BFD_RELOC_KVX_S43_TLS_GD_LO10:
	case BFD_RELOC_KVX_S43_TLS_GD_UP27:
	case BFD_RELOC_KVX_S43_TLS_GD_EX6:

	case BFD_RELOC_KVX_S37_TLS_IE_LO10:
	case BFD_RELOC_KVX_S37_TLS_IE_UP27:

	case BFD_RELOC_KVX_S43_TLS_IE_LO10:
	case BFD_RELOC_KVX_S43_TLS_IE_UP27:
	case BFD_RELOC_KVX_S43_TLS_IE_EX6:

	case BFD_RELOC_KVX_S37_TLS_LD_LO10:
	case BFD_RELOC_KVX_S37_TLS_LD_UP27:

	case BFD_RELOC_KVX_S43_TLS_LD_LO10:
	case BFD_RELOC_KVX_S43_TLS_LD_UP27:
	case BFD_RELOC_KVX_S43_TLS_LD_EX6:

	  S_SET_THREAD_LOCAL (fixP->fx_addsy);
	  break;
	default:
	  break;
	}
    }

  /* If relocation has been marked for deletion, apply remaining changes */
  if (fixP->fx_done)
    {
      switch (fixP->fx_r_type)
	{
	case BFD_RELOC_8:
	case BFD_RELOC_16:
	case BFD_RELOC_32:
	case BFD_RELOC_64:

	case BFD_RELOC_KVX_GLOB_DAT:
	case BFD_RELOC_KVX_32_GOT:
	case BFD_RELOC_KVX_64_GOT:
	case BFD_RELOC_KVX_64_GOTOFF:
	case BFD_RELOC_KVX_32_GOTOFF:
	  image = value;
	  md_number_to_chars (fixpos, image, fixP->fx_size);
	  break;

	case BFD_RELOC_KVX_PCREL17:
	  if (signed_overflow (value, 17 + 2))
	    as_bad_where (fixP->fx_file, fixP->fx_line,
			  _("branch out of range"));
	  goto pcrel_common;

	case BFD_RELOC_KVX_PCREL27:
	  if (signed_overflow (value, 27 + 2))
	    as_bad_where (fixP->fx_file, fixP->fx_line,
			  _("branch out of range"));
	  goto pcrel_common;

	case BFD_RELOC_KVX_S16_PCREL:
	  if (signed_overflow (value, 16))
	    as_bad_where (fixP->fx_file, fixP->fx_line,
			  _("signed16 PCREL value out of range"));
	  goto pcrel_common;

	case BFD_RELOC_KVX_S43_PCREL_LO10:
	case BFD_RELOC_KVX_S43_PCREL_UP27:
	case BFD_RELOC_KVX_S43_PCREL_EX6:
	  if (signed_overflow (value, 10 + 27 + 6))
	    as_bad_where (fixP->fx_file, fixP->fx_line,
			  _("signed43 PCREL value out of range"));
	  goto pcrel_common;

	case BFD_RELOC_KVX_S37_PCREL_LO10:
	case BFD_RELOC_KVX_S37_PCREL_UP27:
	  if (signed_overflow (value, 10 + 27))
	    as_bad_where (fixP->fx_file, fixP->fx_line,
			  _("signed37 PCREL value out of range"));
	  goto pcrel_common;

	case BFD_RELOC_KVX_S64_PCREL_LO10:
	case BFD_RELOC_KVX_S64_PCREL_UP27:
	case BFD_RELOC_KVX_S64_PCREL_EX27:

	pcrel_common:
	  if (fixP->fx_pcrel || fixP->fx_addsy)
	    return;
	  value =
	    (((value >> rel->howto->rightshift) << rel->howto->bitpos) & rel->
	     howto->dst_mask);
	  image = (image & ~(rel->howto->dst_mask)) | value;
	  md_number_to_chars (fixpos, image, fixP->fx_size);
	  break;

	case BFD_RELOC_KVX_S64_GOTADDR_LO10:
	case BFD_RELOC_KVX_S64_GOTADDR_UP27:
	case BFD_RELOC_KVX_S64_GOTADDR_EX27:

	case BFD_RELOC_KVX_S43_GOTADDR_LO10:
	case BFD_RELOC_KVX_S43_GOTADDR_UP27:
	case BFD_RELOC_KVX_S43_GOTADDR_EX6:

	case BFD_RELOC_KVX_S37_GOTADDR_LO10:
	case BFD_RELOC_KVX_S37_GOTADDR_UP27:
	  value = 0;
	  /* Fallthrough */

	case BFD_RELOC_KVX_S32_UP27:

	case BFD_RELOC_KVX_S37_UP27:

	case BFD_RELOC_KVX_S43_UP27:

	case BFD_RELOC_KVX_S64_UP27:
	case BFD_RELOC_KVX_S64_EX27:
	case BFD_RELOC_KVX_S64_LO10:

	case BFD_RELOC_KVX_S43_TLS_LE_UP27:
	case BFD_RELOC_KVX_S43_TLS_LE_EX6:

	case BFD_RELOC_KVX_S37_TLS_LE_UP27:

	case BFD_RELOC_KVX_S37_GOTOFF_UP27:

	case BFD_RELOC_KVX_S43_GOTOFF_UP27:
	case BFD_RELOC_KVX_S43_GOTOFF_EX6:

	case BFD_RELOC_KVX_S43_GOT_UP27:
	case BFD_RELOC_KVX_S43_GOT_EX6:

	case BFD_RELOC_KVX_S37_GOT_UP27:

	case BFD_RELOC_KVX_S32_LO5:
	case BFD_RELOC_KVX_S37_LO10:

	case BFD_RELOC_KVX_S43_LO10:
	case BFD_RELOC_KVX_S43_EX6:

	case BFD_RELOC_KVX_S43_TLS_LE_LO10:
	case BFD_RELOC_KVX_S37_TLS_LE_LO10:

	case BFD_RELOC_KVX_S37_GOTOFF_LO10:
	case BFD_RELOC_KVX_S43_GOTOFF_LO10:

	case BFD_RELOC_KVX_S43_GOT_LO10:
	case BFD_RELOC_KVX_S37_GOT_LO10:

	default:
	  as_fatal ("[md_apply_fix]:"
		    "unsupported relocation type (type not handled : %d)",
		    fixP->fx_r_type);
	}
    }
}

/*
 * Warning: Can be called only in fixup_segment() after fx_addsy field
 * has been updated by calling symbol_get_value_expression(...->X_add_symbol)
 */
int
kvx_validate_sub_fix (fixS * fixP)
{
  segT add_symbol_segment, sub_symbol_segment;

  switch (fixP->fx_r_type)
    {
    case BFD_RELOC_8:
    case BFD_RELOC_16:
    case BFD_RELOC_32:
      if (fixP->fx_addsy != NULL)
	add_symbol_segment = S_GET_SEGMENT (fixP->fx_addsy);
      else
	return 0;
      if (fixP->fx_subsy != NULL)
	sub_symbol_segment = S_GET_SEGMENT (fixP->fx_subsy);
      else
	return 0;

      if ((strcmp (S_GET_NAME (fixP->fx_addsy),
		   S_GET_NAME (fixP->fx_subsy)) == 0) &&
	  (add_symbol_segment == sub_symbol_segment))
	return 1;
      break;
    default:
      break;
    }

  return 0;
}

/* This is called whenever some data item (not an instruction) needs a
 * fixup.  */
void
kvx_cons_fix_new (fragS * f, int where, int nbytes, expressionS * exp,
		  bfd_reloc_code_real_type code)
{
  if (exp->X_op == O_pseudo_fixup)
    {
      exp->X_op = O_symbol;
      struct pseudo_func *pf =
	kvx_get_pseudo_func_data_scn (exp->X_op_symbol);
      assert (pf != NULL);
      code = pf->pseudo_relocs.single;

      if (code == BFD_RELOC_UNUSED)
	as_fatal ("Unsupported relocation");
    }
  else
    {
      switch (nbytes)
	{
	case 1:
	  code = BFD_RELOC_8;
	  break;
	case 2:
	  code = BFD_RELOC_16;
	  break;
	case 4:
	  code = BFD_RELOC_32;
	  break;
	case 8:
	  code = BFD_RELOC_64;
	  break;
	default:
	  as_fatal ("unsupported BFD relocation size %u", nbytes);
	  break;
	}
    }
  fix_new_exp (f, where, nbytes, exp, 0, code);
}

/*
 * generate a relocation record
 */

arelent *
tc_gen_reloc (asection * sec ATTRIBUTE_UNUSED, fixS * fixp)
{
  arelent *reloc;
  bfd_reloc_code_real_type code;

  reloc = (arelent *) xmalloc (sizeof (arelent));

  reloc->sym_ptr_ptr = (asymbol **) xmalloc (sizeof (asymbol *));
  *reloc->sym_ptr_ptr = symbol_get_bfdsym (fixp->fx_addsy);
  reloc->address = fixp->fx_frag->fr_address + fixp->fx_where;

  code = fixp->fx_r_type;
  if (code == BFD_RELOC_32 && fixp->fx_pcrel)
    code = BFD_RELOC_32_PCREL;
  reloc->howto = bfd_reloc_type_lookup (stdoutput, code);

  if (reloc->howto == NULL)
    {
      as_bad_where (fixp->fx_file, fixp->fx_line,
		    "cannot represent `%s' relocation in object file",
		    bfd_get_reloc_code_name (code));
      return NULL;
    }

//  if (!fixp->fx_pcrel != !reloc->howto->pc_relative)
//    {
//      as_fatal ("internal error? cannot generate `%s' relocation",
//		bfd_get_reloc_code_name (code));
//    }
//  assert (!fixp->fx_pcrel == !reloc->howto->pc_relative);

  reloc->addend = fixp->fx_offset;

  /*
   * Ohhh, this is ugly.  The problem is that if this is a local global
   * symbol, the relocation will entirely be performed at link time, not
   * at assembly time.  bfd_perform_reloc doesn't know about this sort
   * of thing, and as a result we need to fake it out here.
   */

  /* GD I'm not sure what this is used for in the kvx case but it sure  */
  /* messes up the relocs when emit_all_relocs is used as they are not */
  /* resolved with respect to a global sysmbol (e.g. .text), and hence */
  /* they are ALWAYS resolved at link time                             */
  /* FIXME FIXME                                                       */

  /* clarkes: 030827:  This code (and the other half of the fix in write.c)
   * have caused problems with the PIC relocations.
   * The root problem is that bfd_install_relocation adds in to the reloc
   * addend the section offset of a symbol defined in the current object.
   * This causes problems on numerous other targets too, and there are
   * several different methods used to get around it:
   *   1.  In tc_gen_reloc, subtract off the value that bfd_install_relocation
   *       added.  That is what we do here, and it is also done the
   *       same way for alpha.
   *   2.  In md_apply_fix, subtract off the value that bfd_install_relocation
   *       will add.  This is done on SH (non-ELF) and sparc targets.
   *   3.  In the howto structure for the relocations, specify a
   *       special function that does not return bfd_reloc_continue.
   *       This causes bfd_install_relocaion to terminate before it
   *       adds in the symbol offset.  This is done on SH ELF targets.
   *       Note that on ST200 we specify bfd_elf_generic_reloc as
   *       the special function.  This will return bfd_reloc_continue
   *       only in some circumstances, but in particular if the reloc
   *       is marked as partial_inplace in the bfd howto structure, then
   *       bfd_elf_generic_reloc will return bfd_reloc_continue.
   *       Some ST200 relocations are marked as partial_inplace
   *       (this is an error in my opinion because ST200 always uses
   *       a separate addend), but some are not.  The PIC relocations
   *       are not marked as partial_inplace, so for them,
   *       bfd_elf_generic_reloc returns bfd_reloc_ok, and the addend
   *       is not modified by bfd_install_relocation.   The relocations
   *       R_KVX_16 and R_KVX_32 are marked partial_inplace, and so for
   *       these we need to correct the addend.
   * In the code below, the condition in the emit_all_relocs branch
   * (now moved to write.c) is the inverse of the condition that
   * bfd_elf_generic_reloc uses to short-circuit the code in
   * bfd_install_relocation that modifies the addend.  The condition
   * in the else branch match the condition used in the alpha version
   * of tc_gen_reloc (see tc-alpha.c).
   * I do not know why we need to use different conditions in these
   * two branches, it seems to me that the condition should be the same
   * whether or not emit_all_relocs is true.
   * I also do not understand why it was necessary to move the emit_all_relocs
   * condition to write.c.
   */

  if (S_IS_EXTERNAL (fixp->fx_addsy) &&
      !S_IS_COMMON (fixp->fx_addsy) && reloc->howto->partial_inplace)
    reloc->addend -= symbol_get_bfdsym (fixp->fx_addsy)->value;

  return reloc;
}

/* Round up segment to appropriate boundary */

valueT
md_section_align (asection * seg ATTRIBUTE_UNUSED, valueT size)
{
#ifndef OBJ_ELF
  /* This is not right for ELF; a.out wants it, and COFF will force
   * the alignment anyways.  */
  int align = bfd_get_section_alignment (stdoutput, seg);
  valueT mask = ((valueT) 1 << align) - 1;
  return (size + mask) & ~mask;
#else
  return size;
#endif
}

int
md_estimate_size_before_relax (register fragS * fragP ATTRIBUTE_UNUSED,
			       segT segtype ATTRIBUTE_UNUSED)
{
  as_fatal ("estimate_size_before_relax called");
}

void
md_convert_frag (bfd * abfd ATTRIBUTE_UNUSED,
		 asection * sec ATTRIBUTE_UNUSED,
		 fragS * fragp ATTRIBUTE_UNUSED)
{
  as_fatal ("kvx convert_frag");
}

symbolS *
md_undefined_symbol (char *name ATTRIBUTE_UNUSED)
{
  return 0;
}

const char *
md_atof (int type ATTRIBUTE_UNUSED,
	 char *litp ATTRIBUTE_UNUSED, int *sizep ATTRIBUTE_UNUSED)
{
  return ieee_md_atof (type, litp, sizep, TARGET_BYTES_BIG_ENDIAN);
}

/*
 * calculate the base for a pcrel fixup
 * -- for relocation, we might need to add addend ?
 */

long
md_pcrel_from (fixS * fixP)
{
  return (fixP->fx_where + fixP->fx_frag->fr_address);
}

/************************************************************/
/*   Hooks into standard processing -- we hook into label   */
/*   handling code to detect double ':' and we hook before  */
/*   a line of code is processed to do some simple sed style */
/*   edits.                                                 */
/************************************************************/

static symbolS *last_proc_sym = NULL;
static int update_last_proc_sym = 0;

void
kvx_frob_label (symbolS *sym)
{
  if (update_last_proc_sym)
    {
      last_proc_sym = sym;
      update_last_proc_sym = 0;
    }

  if (inside_bundle)
    {
      struct label_fix *fix;
      fix = malloc (sizeof (*fix));
      fix->next = label_fixes;
      fix->sym = sym;
      label_fixes = fix;
    }

  dwarf2_emit_label (sym);
}

void
kvx_check_label (symbolS *sym)
{
  /* Labels followed by a second semi-colon are considered external symbols.  */
  if (*input_line_pointer == ':')
    {
      S_SET_EXTERNAL (sym);
      input_line_pointer++;
    }
}

/* Emit single bundle nop. This is needed by .nop asm directive
 * Have to manage end of bundle done usually by start_line_hook
 * using BE pseudo op
 */
void
kvx_emit_single_noop (void)
{
  char *nop;
  char *end_of_bundle;

  if (asprintf (&nop, "nop") < 0)
    as_fatal ("%s", xstrerror (errno));

  if (asprintf (&end_of_bundle, "be") < 0)
    as_fatal ("%s", xstrerror (errno));

  char *saved_ilp = input_line_pointer;
  md_assemble (nop);
  md_assemble (end_of_bundle);
  input_line_pointer = saved_ilp;
  free (nop);
  free (end_of_bundle);
}

/*  edit out some syntactic sugar that confuses GAS       */
/*  input_line_pointer is guaranteed to point to the      */
/*  the current line but may include text from following  */
/*  lines.  Thus, '\n' must be scanned for as well as '\0' */

void
kvx_md_start_line_hook (void)
{
  char *t;

  for (t = input_line_pointer; t && t[0] == ' '; t++);

  /* Detect illegal syntax patterns:
   * - two bundle ends on the same line: ;; ;;
   * - illegal token: ;;;
   */
  if (t && (t[0] == ';') && (t[1] == ';'))
    {
      char *tmp_t;
      bool newline_seen = false;

      if (t[2] == ';')
	as_fatal ("Syntax error: Illegal ;;; token");

      tmp_t = t + 2;

      while (tmp_t && tmp_t[0])
	{
	  while (tmp_t && tmp_t[0] &&
		 ((tmp_t[0] == ' ') || (tmp_t[0] == '\n')))
	    {
	      if (tmp_t[0] == '\n')
		newline_seen = true;
	      tmp_t++;
	    }
	  if (tmp_t[0] == ';' && tmp_t[1] == ';')
	    {
	      /* if there's no newline between the two bundle stops
	       * then raise a syntax error now, otherwise a strange error
	       * message from read.c will be raised: "junk at end of line..."
	       */
	      if (tmp_t[2] == ';')
		as_fatal ("Syntax error: Illegal ;;; token");

	      if (!newline_seen)
		  as_fatal ("Syntax error: More than one bundle stop on a line");
	      newline_seen = false;	/* reset */

	      /* this is an empty bundle, transform it into an
	       * empty statement */
	      tmp_t[0] = ';';
	      tmp_t[1] = ' ';

	      tmp_t += 2;
	    }
	  else
	    break;
	}
    }

  /* check for bundle end                             */
  /* we transform these into a special opcode BE      */
  /* because gas has ';' hardwired as a statement end */
  if (t && (t[0] == ';') && (t[1] == ';'))
    {
      t[0] = 'B';
      t[1] = 'E';
      return;
    }
}

static void
kvx_check_resources (int f)
{
  env.opts.check_resource_usage = f;
}

/** called before write_object_file */
void
kvx_end (void)
{
  int newflags;

  if (!env.params.core_set)
    env.params.core = kvx_core_info->elf_core;

  /* (pp) the flags must be set at once */
  newflags = env.params.core | env.params.abi | env.params.pic_flags;

  if (env.params.arch_size == 64)
    newflags |= ELF_KVX_ABI_64B_ADDR_BIT;

  bfd_set_private_flags (stdoutput, newflags);

  cleanup ();

  if (inside_bundle && insncnt != 0)
    as_bad ("unexpected end-of-file while processing a bundle."
	    "  Please check that ;; is on its own line.");
}

static void
kvx_type (int start ATTRIBUTE_UNUSED)
{
  char *name;
  char c;
  int type;
  char *typename = NULL;
  symbolS *sym;
  elf_symbol_type *elfsym;

  c = get_symbol_name (&name);
  sym = symbol_find_or_make (name);
  elfsym = (elf_symbol_type *) symbol_get_bfdsym (sym);
  *input_line_pointer = c;

  if (!*S_GET_NAME (sym))
    as_bad (_("Missing symbol name in directive"));

  SKIP_WHITESPACE ();
  if (*input_line_pointer == ',')
    ++input_line_pointer;


  SKIP_WHITESPACE ();
  if (*input_line_pointer == '#'
      || *input_line_pointer == '@'
      || *input_line_pointer == '"' || *input_line_pointer == '%')
    ++input_line_pointer;

  /* typename = input_line_pointer; */
  /* c = get_symbol_end(); */
  c = get_symbol_name (&typename);

  type = 0;
  if (strcmp (typename, "function") == 0
      || strcmp (typename, "STT_FUNC") == 0)
    type = BSF_FUNCTION;
  else if (strcmp (typename, "object") == 0
	   || strcmp (typename, "STT_OBJECT") == 0)
    type = BSF_OBJECT;
  else if (strcmp (typename, "tls_object") == 0
	   || strcmp (typename, "STT_TLS") == 0)
    type = BSF_OBJECT | BSF_THREAD_LOCAL;
  else if (strcmp (typename, "common") == 0
	   || strcmp (typename, "STT_COMMON") == 0)
    type = BSF_ELF_COMMON;
  else if (strcmp (typename, "gnu_unique_object") == 0
	   || strcmp (typename, "STB_GNU_UNIQUE") == 0)
    {
      elf_tdata (stdoutput)->has_gnu_osabi |= elf_gnu_osabi_unique;
      type = BSF_OBJECT | BSF_GNU_UNIQUE;
    }
  else if (strcmp (typename, "notype") == 0
	   || strcmp (typename, "STT_NOTYPE") == 0)
    ;
#ifdef md_elf_symbol_type
  else if ((type = md_elf_symbol_type (typename, sym, elfsym)) != -1)
    ;
#endif
  else
    as_bad (_("unrecognized symbol type \"%s\""), typename);

  *input_line_pointer = c;

  if (*input_line_pointer == '"')
    ++input_line_pointer;

  elfsym->symbol.flags |= type;
  symbol_get_bfdsym (sym)->flags |= type;

  demand_empty_rest_of_line ();
}

#define ENDPROCEXTENSION	"$endproc"
#define MINUSEXPR		".-"

static int proc_endp_status = 0;

static void
kvx_endp (int start ATTRIBUTE_UNUSED)
{
  char c;
  char *name;

  if (inside_bundle)
    as_warn (".endp directive inside a bundle.");
  /* function name is optionnal and is ignored */
  /* there may be several names separated by commas... */
  while (1)
    {
      SKIP_WHITESPACE ();
      c = get_symbol_name (&name);
      (void) restore_line_pointer (c);
      SKIP_WHITESPACE ();
      if (*input_line_pointer != ',')
	break;
      ++input_line_pointer;
    }
  demand_empty_rest_of_line ();

  if (!proc_endp_status)
    {
      as_warn (".endp directive doesn't follow .proc -- ignoring ");
      return;
    }

  proc_endp_status = 0;

  /* TB begin : add BSF_FUNCTION attribute to last_proc_sym symbol */
  if (size_type_function)
    {
      if (!last_proc_sym)
	{
	  as_bad ("Cannot set function attributes (bad symbol)");
	  return;
	}

      /*    last_proc_sym->symbol.flags |= BSF_FUNCTION; */
      symbol_get_bfdsym (last_proc_sym)->flags |= BSF_FUNCTION;
      /* Add .size funcname,.-funcname in order to add size
       * attribute to the current function */
      {
	const int newdirective_sz =
	  strlen (S_GET_NAME (last_proc_sym)) + strlen (MINUSEXPR) + 1;
	char *newdirective = malloc (newdirective_sz);
	char *savep = input_line_pointer;
	expressionS exp;

	memset (newdirective, 0, newdirective_sz);

	/* BUILD :".-funcname" expression */
	strcat (newdirective, MINUSEXPR);
	strcat (newdirective, S_GET_NAME (last_proc_sym));
	input_line_pointer = newdirective;
	expression (&exp);

	if (exp.X_op == O_constant)
	  {
	    S_SET_SIZE (last_proc_sym, exp.X_add_number);
	    if (symbol_get_obj (last_proc_sym)->size)
	      {
		xfree (symbol_get_obj (last_proc_sym)->size);
		symbol_get_obj (last_proc_sym)->size = NULL;
	      }
	  }
	else
	  {
	    symbol_get_obj (last_proc_sym)->size =
	      (expressionS *) xmalloc (sizeof (expressionS));
	    *symbol_get_obj (last_proc_sym)->size = exp;
	  }

	/* just restore the real input pointer */
	input_line_pointer = savep;
	free (newdirective);
      }
    }
  /* TB end */

  last_proc_sym = NULL;
}

static void
kvx_proc (int start ATTRIBUTE_UNUSED)
{
  char c;
  char *name;
  /* there may be several names separated by commas... */
  while (1)
    {
      SKIP_WHITESPACE ();
      c = get_symbol_name (&name);
      (void) restore_line_pointer (c);

      SKIP_WHITESPACE ();
      if (*input_line_pointer != ',')
	break;
      ++input_line_pointer;
    }
  demand_empty_rest_of_line ();

  if (proc_endp_status)
    {
      as_warn (".proc follows .proc -- ignoring");
      return;
    }

  proc_endp_status = 1;

  /* this code emit a global symbol to mark the end of each function    */
  /* the symbol emitted has a name formed by the original function name */
  /* concatenated with $endproc so if _foo is a function name the symbol */
  /* marking the end of it is _foo$endproc                              */
  /* It is also required for generation of .size directive in kvx_endp() */

  if (size_type_function)
    update_last_proc_sym = 1;
}

int
kvx_force_reloc (fixS * fixP)
{
  symbolS *sym;
  asection *symsec;

  if (generic_force_reloc (fixP))
    return 1;

  switch (fixP->fx_r_type)
    {
    case BFD_RELOC_KVX_32_GOTOFF:
    case BFD_RELOC_KVX_S37_GOTOFF_UP27:
    case BFD_RELOC_KVX_S37_GOTOFF_LO10:

    case BFD_RELOC_KVX_64_GOTOFF:
    case BFD_RELOC_KVX_S43_GOTOFF_UP27:
    case BFD_RELOC_KVX_S43_GOTOFF_LO10:
    case BFD_RELOC_KVX_S43_GOTOFF_EX6:

    case BFD_RELOC_KVX_32_GOT:
    case BFD_RELOC_KVX_64_GOT:
    case BFD_RELOC_KVX_S37_GOT_UP27:
    case BFD_RELOC_KVX_S37_GOT_LO10:

    case BFD_RELOC_KVX_GLOB_DAT:
      return 1;
    default:
      return 0;
    }

  sym = fixP->fx_addsy;
  if (sym)
    {
      symsec = S_GET_SEGMENT (sym);
      /* if (bfd_is_abs_section (symsec)) return 0; */
      if (!SEG_NORMAL (symsec))
	return 0;
    }
  return 1;
}

int
kvx_force_reloc_sub_same (fixS * fixP, segT sec)
{
  symbolS *sym;
  asection *symsec;
  const char *sec_name = NULL;

  if (generic_force_reloc (fixP))
    return 1;

  switch (fixP->fx_r_type)
    {
    case BFD_RELOC_KVX_32_GOTOFF:
    case BFD_RELOC_KVX_S37_GOTOFF_UP27:
    case BFD_RELOC_KVX_S37_GOTOFF_LO10:

    case BFD_RELOC_KVX_64_GOTOFF:
    case BFD_RELOC_KVX_S43_GOTOFF_UP27:
    case BFD_RELOC_KVX_S43_GOTOFF_LO10:
    case BFD_RELOC_KVX_S43_GOTOFF_EX6:

    case BFD_RELOC_KVX_32_GOT:
    case BFD_RELOC_KVX_64_GOT:
    case BFD_RELOC_KVX_S37_GOT_UP27:
    case BFD_RELOC_KVX_S37_GOT_LO10:

    case BFD_RELOC_KVX_S37_LO10:
    case BFD_RELOC_KVX_S37_UP27:

    case BFD_RELOC_KVX_GLOB_DAT:
      return 1;

    default:
      return 0;
    }

  sym = fixP->fx_addsy;
  if (sym)
    {
      symsec = S_GET_SEGMENT (sym);
      /* if (bfd_is_abs_section (symsec)) return 0; */
      if (!SEG_NORMAL (symsec))
	return 0;

      /*
       * for .debug_arrange, .debug_frame, .eh_frame sections, containing
       * expressions of the form "sym2 - sym1 + addend", solve them even when
       * --emit-all-relocs is set. Otherwise, a relocation on two symbols
       * is necessary and fails at elf level. Binopt should not be impacted by
       * the resolution of this relocatable expression on symbols inside a
       * function.
       */
      sec_name = segment_name (sec);
      if ((strcmp (sec_name, ".eh_frame") == 0) ||
	  (strcmp (sec_name, ".except_table") == 0) ||
	  (strncmp (sec_name, ".debug_", sizeof (".debug_")) == 0))
	return 0;
    }
  return 1;
}

/* Implement HANDLE_ALIGN.  */

static void
kvx_make_nops (char *buf, bfd_vma bytes)
{
  bfd_vma i = 0;
  unsigned int j;

  static unsigned int nop_single = 0;

  if (!nop_single)
    {
      const struct kvxopc *opcode =
	(struct kvxopc *) str_hash_find (env.opcode_hash, "nop");

      if (opcode == NULL)
	as_fatal
	  ("internal error: could not find opcode for 'nop' during padding");

      nop_single = opcode->codewords[0].opcode;
    }

  /* KVX instructions are always 4-bytes aligned. If we are at a position */
  /* that is not 4 bytes aligned, it means this is not part of an instruction, */
  /* so it is safe to use a zero byte for padding. */

  for (j = bytes % 4; j > 0; j--)
    buf[i++] = 0;

  for (j = 0; j < (bytes - i); j += 4)
    {
      unsigned nop = nop_single;

      // nop has bundle end only if #4 nop or last padding nop.
      // Sets the parallel bit when neither conditions are matched.
      // 4*4 = biggest nop bundle we can get
      // 12 = offset when writting the last nop possible in a 4 nops bundle
      // bytes-i-4 = offset for the last 4-words in the padding
      if (j % (4 * 4) != 12 && j != (bytes - i - 4))
	nop |= PARALLEL_BIT;

      memcpy (buf + i + j, &nop, sizeof (nop));
    }
}

/* Pads code section with bundle of nops when possible, 0 if not. */
void
kvx_handle_align (fragS *fragP)
{
  switch (fragP->fr_type)
    {
    case rs_align_code:
      {
	bfd_signed_vma bytes = (fragP->fr_next->fr_address
				- fragP->fr_address - fragP->fr_fix);
	char *p = fragP->fr_literal + fragP->fr_fix;

	if (bytes <= 0)
	  break;

	/* Insert zeros or nops to get 4 byte alignment.  */
	kvx_make_nops (p, bytes);
	fragP->fr_fix += bytes;
      }
      break;

    default:
      break;
    }
}
/*
 * This is just used for debugging
 */

ATTRIBUTE_UNUSED
static void
print_operand (expressionS * e, FILE * out)
{
  if (e)
    {
      switch (e->X_op)
	{
	case O_register:
	  fprintf (out, "%s", kvx_registers[e->X_add_number].name);
	  break;

	case O_constant:
	  if (e->X_add_symbol)
	    {
	      if (e->X_add_number)
		fprintf (out, "(%s + %d)", S_GET_NAME (e->X_add_symbol),
			 (int) e->X_add_number);
	      else
		fprintf (out, "%s", S_GET_NAME (e->X_add_symbol));
	    }
	  else
	    fprintf (out, "%d", (int) e->X_add_number);
	  break;

	case O_symbol:
	  if (e->X_add_symbol)
	    {
	      if (e->X_add_number)
		fprintf (out, "(%s + %d)", S_GET_NAME (e->X_add_symbol),
			 (int) e->X_add_number);
	      else
		fprintf (out, "%s", S_GET_NAME (e->X_add_symbol));
	    }
	  else
	    fprintf (out, "%d", (int) e->X_add_number);
	  break;

	default:
	  fprintf (out, "o,ptype-%d", e->X_op);
	}
    }
}

void
kvx_cfi_frame_initial_instructions (void)
{
  cfi_add_CFA_def_cfa (KVX_SP_REGNO, 0);
}

int
kvx_regname_to_dw2regnum (const char *regname)
{
  unsigned int regnum = -1;
  const char *p;
  char *q;

  if (regname[0] == 'r')
    {
      p = regname + 1;
      regnum = strtoul (p, &q, 10);
      if (p == q || *q || regnum >= 64)
	return -1;
    }
  return regnum;
}
