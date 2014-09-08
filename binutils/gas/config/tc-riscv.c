/* tc-mips.c -- assemble code for a MIPS chip.
   Copyright 1993, 1994, 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002,
   2003, 2004, 2005, 2006, 2007, 2008, 2009  Free Software Foundation, Inc.
   Contributed by the OSF and Ralph Campbell.
   Written by Keith Knowles and Ralph Campbell, working independently.
   Modified for ECOFF and R4000 support by Ian Lance Taylor of Cygnus
   Support.

   This file is part of GAS.

   GAS is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GAS is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GAS; see the file COPYING.  If not, write to the Free
   Software Foundation, 51 Franklin Street - Fifth Floor, Boston, MA
   02110-1301, USA.  */

#include "as.h"
#include "config.h"
#include "subsegs.h"
#include "safe-ctype.h"

#include "itbl-ops.h"
#include "dwarf2dbg.h"
#include "dw2gencfi.h"

#include <execinfo.h>
#include <stdint.h>

#ifdef DEBUG
#define DBG(x) printf x
#else
#define DBG(x)
#endif

#ifdef OBJ_MAYBE_ELF
/* Clean up namespace so we can include obj-elf.h too.  */
static int mips_output_flavor (void);
static int mips_output_flavor (void) { return OUTPUT_FLAVOR; }
#undef OBJ_PROCESS_STAB
#undef OUTPUT_FLAVOR
#undef S_GET_ALIGN
#undef S_GET_SIZE
#undef S_SET_ALIGN
#undef S_SET_SIZE
#undef obj_frob_file
#undef obj_frob_file_after_relocs
#undef obj_frob_symbol
#undef obj_pop_insert
#undef obj_sec_sym_ok_for_reloc
#undef OBJ_COPY_SYMBOL_ATTRIBUTES

#include "obj-elf.h"
/* Fix any of them that we actually care about.  */
#undef OUTPUT_FLAVOR
#define OUTPUT_FLAVOR mips_output_flavor()
#endif

#if defined (OBJ_ELF)
#include "elf/riscv.h"
#endif

#include "opcode/riscv.h"

#define ZERO 0
#define SP 14

/* Information about an instruction, including its format, operands
   and fixups.  */
struct mips_cl_insn
{
  /* The opcode's entry in riscv_opcodes or mips16_opcodes.  */
  const struct riscv_opcode *insn_mo;

  /* The 16-bit or 32-bit bitstring of the instruction itself.  This is
     a copy of INSN_MO->match with the operands filled in.  */
  insn_t insn_opcode;

  /* The frag that contains the instruction.  */
  struct frag *frag;

  /* The offset into FRAG of the first instruction byte.  */
  long where;

  /* The relocs associated with the instruction, if any.  */
  fixS *fixp;
};

static bfd_boolean rv64 = TRUE; /* RV64 (true) or RV32 (false) */
#define HAVE_32BIT_SYMBOLS 1 /* LUI/ADDI for symbols, even in RV64 */
#define HAVE_32BIT_ADDRESSES (!rv64)
#define LOAD_ADDRESS_INSN (HAVE_32BIT_ADDRESSES ? "lw" : "ld")
#define ADD32_INSN (rv64 ? "addiw" : "addi")

struct riscv_subset
{
  const char* name;
  int version_major;
  int version_minor;

  struct riscv_subset* next;
};

static struct riscv_subset* riscv_subsets;

static int
riscv_subset_supports(const char* feature)
{
  struct riscv_subset* s;
  bfd_boolean rv64_insn;

  if ((rv64_insn = !strncmp(feature, "64", 2)) || !strncmp(feature, "32", 2))
    {
      if (rv64 != rv64_insn)
        return 0;
      feature += 2;
    }

  for (s = riscv_subsets; s != NULL; s = s->next)
    if (strcmp(s->name, feature) == 0)
      /* FIXME: once we support version numbers:
         return major == s->version_major && minor <= s->version_minor; */
      return 1;

  return 0;
}

static void
riscv_add_subset(const char* subset)
{
  struct riscv_subset* s = xmalloc(sizeof(struct riscv_subset));
  s->name = xstrdup(subset);
  s->version_major = 1;
  s->version_minor = 0;
  s->next = riscv_subsets;
  riscv_subsets = s;
}

static void
riscv_set_arch(const char* arg)
{
  /* Formally, ISA subset names begin with RV, RV32, or RV64, but we allow the
     prefix to be omitted.  We also allow all-lowercase names if version
     numbers and eXtensions are omitted (i.e. only some combination of imafd
     is supported in this case).
     
     FIXME: Version numbers are not supported yet. */
  const char* subsets = "IMAFD";
  const char* p;
  
  for (p = arg; *p; p++)
    if (!ISLOWER(*p) || strchr(subsets, TOUPPER(*p)) == NULL)
      break;

  if (!*p)
    {
      /* Legal all-lowercase name. */
      for (p = arg; *p; p++)
        {
          char subset[2] = {TOUPPER(*p), 0};
          riscv_add_subset(subset);
        }
      return;
    }

  if (strncmp(arg, "RV32", 4) == 0)
    {
      rv64 = FALSE;
      arg += 4;
    }
  else if (strncmp(arg, "RV64", 4) == 0)
    {
      rv64 = TRUE;
      arg += 4;
    }
  else if (strncmp(arg, "RV", 2) == 0)
    arg += 2;

  if (*arg && *arg != 'I')
    as_fatal("`I' must be the first ISA subset name specified (got %c)", *arg);

  for (p = arg; *p; p++)
    {
      if (*p == 'X')
        {
          const char* q = p+1;
          while (ISLOWER(*q))
            q++;

          char subset[q-p+1];
          memcpy(subset, p, q-p);
          subset[q-p] = 0;

          riscv_add_subset(subset);
          p = q-1;
        }
      else if (strchr(subsets, *p) != NULL)
        {
          char subset[2] = {*p, 0};
          riscv_add_subset(subset);
        }
      else
        as_fatal("unsupported ISA subset %c", *p);
    }
}

/* This is the set of options which may be modified by the .set
   pseudo-op.  We use a struct so that .set push and .set pop are more
   reliable.  */

struct mips_set_options
{
  /* Enable RVC instruction compression */
  int rvc;
};

static struct mips_set_options mips_opts =
{
  /* rvc */ 0
};

/* Whether or not we're generating position-independent code.  */
static bfd_boolean is_pic = FALSE;

/* handle of the OPCODE hash table */
static struct hash_control *op_hash = NULL;

/* This array holds the chars that always start a comment.  If the
    pre-processor is disabled, these aren't very useful */
const char comment_chars[] = "#";

/* This array holds the chars that only start a comment at the beginning of
   a line.  If the line seems to have the form '# 123 filename'
   .line and .file directives will appear in the pre-processed output */
/* Note that input_file.c hand checks for '#' at the beginning of the
   first line of the input file.  This is because the compiler outputs
   #NO_APP at the beginning of its output.  */
/* Also note that C style comments are always supported.  */
const char line_comment_chars[] = "#";

/* This array holds machine specific line separator characters.  */
const char line_separator_chars[] = ";";

/* Chars that can be used to separate mant from exp in floating point nums */
const char EXP_CHARS[] = "eE";

/* Chars that mean this number is a floating point constant */
/* As in 0f12.456 */
/* or    0d1.2345e12 */
const char FLT_CHARS[] = "rRsSfFdDxXpP";

/* Also be aware that MAXIMUM_NUMBER_OF_CHARS_FOR_FLOAT may have to be
   changed in read.c .  Ideally it shouldn't have to know about it at all,
   but nothing is ideal around here.
 */

static char *insn_error;

static int auto_align = 1;

/* To output NOP instructions correctly, we need to keep information
   about the previous two instructions.  */

/* Debugging level.  -g sets this to 2.  -gN sets this to N.  -g0 is
   equivalent to seeing no -g option at all.  */
static int mips_debug = 0;

/* For ECOFF and ELF, relocations against symbols are done in two
   parts, with a HI relocation and a LO relocation.  Each relocation
   has only 16 bits of space to store an addend.  This means that in
   order for the linker to handle carries correctly, it must be able
   to locate both the HI and the LO relocation.  This means that the
   relocations must appear in order in the relocation table.

   In order to implement this, we keep track of each unmatched HI
   relocation.  We then sort them so that they immediately precede the
   corresponding LO relocation.  */

struct mips_hi_fixup
{
  /* Next HI fixup.  */
  struct mips_hi_fixup *next;
  /* This fixup.  */
  fixS *fixp;
  /* The section this fixup is in.  */
  segT seg;
};


#define RELAX_BRANCH_ENCODE(uncond, rvc, toofar)	\
  ((relax_substateT) 					\
   (0xc0000000						\
    | ((rvc) ? 1 : 0)					\
    | ((toofar) ? 2 : 0)				\
    | ((uncond) ? 8 : 0)))
#define RELAX_BRANCH_P(i) (((i) & 0xf0000000) == 0xc0000000)
#define RELAX_BRANCH_UNCOND(i) (((i) & 8) != 0)
#define RELAX_BRANCH_TOOFAR(i) (((i) & 2) != 0)
#define RELAX_BRANCH_RVC(i) (((i) & 1) != 0)

/* Is the given value a sign-extended 32-bit value?  */
#define IS_SEXT_32BIT_NUM(x)						\
  (((x) &~ (offsetT) 0x7fffffff) == 0					\
   || (((x) &~ (offsetT) 0x7fffffff) == ~ (offsetT) 0x7fffffff))

#define IS_SEXT_NBIT_NUM(x,n) \
  ({ int64_t __tmp = (x); \
     __tmp = (__tmp << (64-(n))) >> (64-(n)); \
     __tmp == (x); })

/* Is the given value a zero-extended 32-bit value?  Or a negated one?  */
#define IS_ZEXT_32BIT_NUM(x)						\
  (((x) &~ (offsetT) 0xffffffff) == 0					\
   || (((x) &~ (offsetT) 0xffffffff) == ~ (offsetT) 0xffffffff))

/* Replace bits MASK << SHIFT of STRUCT with the equivalent bits in
   VALUE << SHIFT.  VALUE is evaluated exactly once.  */
#define INSERT_BITS(STRUCT, VALUE, MASK, SHIFT) \
  (STRUCT) = (((STRUCT) & ~((MASK) << (SHIFT))) \
	      | (((VALUE) & (MASK)) << (SHIFT)))

/* Extract bits MASK << SHIFT from STRUCT and shift them right
   SHIFT places.  */
#define EXTRACT_BITS(STRUCT, MASK, SHIFT) \
  (((STRUCT) >> (SHIFT)) & (MASK))

/* Change INSN's opcode so that the operand given by FIELD has value VALUE.
   INSN is a mips_cl_insn structure and VALUE is evaluated exactly once. */
#define INSERT_OPERAND(FIELD, INSN, VALUE) \
  INSERT_BITS ((INSN).insn_opcode, VALUE, OP_MASK_##FIELD, OP_SH_##FIELD)

/* Extract the operand given by FIELD from mips_cl_insn INSN.  */
#define EXTRACT_OPERAND(FIELD, INSN) \
  EXTRACT_BITS ((INSN).insn_opcode, OP_MASK_##FIELD, OP_SH_##FIELD)

/* Determine if an instruction matches an opcode. */
#define OPCODE_MATCHES(OPCODE, OP) \
  (((OPCODE) & MASK_##OP) == MATCH_##OP)

#define INSN_MATCHES(INSN, OP) \
  (((INSN).insn_opcode & MASK_##OP) == MATCH_##OP)

/* Prototypes for static functions.  */

#define internalError()							\
    as_fatal (_("internal Error, line %d, %s"), __LINE__, __FILE__)

enum mips_regclass { MIPS_GR_REG, MIPS_FP_REG };

static void append_insn
  (struct mips_cl_insn *ip, expressionS *p, bfd_reloc_code_real_type r);
static void macro (struct mips_cl_insn * ip);
static void mips_ip (char *str, struct mips_cl_insn * ip);
static void my_getExpression (expressionS *, char *);
static void s_align (int);
static void s_change_sec (int);
static void s_change_section (int);
static void s_cons (int);
static void s_float_cons (int);
static void s_riscv_option (int);
static void s_dtprelword (int);
static void s_dtpreldword (int);
static int validate_mips_insn (const struct riscv_opcode *);
static int relaxed_branch_length (fragS *fragp, asection *sec, int update);

/* Pseudo-op table.

   The following pseudo-ops from the Kane and Heinrich MIPS book
   should be defined here, but are currently unsupported: .alias,
   .galive, .gjaldef, .gjrlive, .livereg, .noalias.

   The following pseudo-ops from the Kane and Heinrich MIPS book are
   specific to the type of debugging information being generated, and
   should be defined by the object format: .aent, .begin, .bend,
   .bgnb, .end, .endb, .ent, .fmask, .frame, .loc, .mask, .verstamp,
   .vreg.

   The following pseudo-ops from the Kane and Heinrich MIPS book are
   not MIPS CPU specific, but are also not specific to the object file
   format.  This file is probably the best place to define them, but
   they are not currently supported: .asm0, .endr, .lab, .struct.  */

static const pseudo_typeS mips_pseudo_table[] =
{
  /* MIPS specific pseudo-ops.  */
  {"option", s_riscv_option, 0},
  {"rdata", s_change_sec, 'r'},
  {"dtprelword", s_dtprelword, 0},
  {"dtpreldword", s_dtpreldword, 0},

  /* Relatively generic pseudo-ops that happen to be used on MIPS
     chips.  */
  {"asciiz", stringer, 8 + 1},
  {"bss", s_change_sec, 'b'},
  {"err", s_err, 0},
  {"half", s_cons, 1},
  {"dword", s_cons, 3},
  {"origin", s_org, 0},
  {"repeat", s_rept, 0},

  /* leb128 doesn't work with relaxation; disallow it */
  {"uleb128", s_err, 0},
  {"sleb128", s_err, 0},

  /* These pseudo-ops are defined in read.c, but must be overridden
     here for one reason or another.  */
  {"align", s_align, 0},
  {"byte", s_cons, 0},
  {"data", s_change_sec, 'd'},
  {"double", s_float_cons, 'd'},
  {"float", s_float_cons, 'f'},
  {"globl", s_globl, 0},
  {"global", s_globl, 0},
  {"hword", s_cons, 1},
  {"int", s_cons, 2},
  {"long", s_cons, 2},
  {"octa", s_cons, 4},
  {"quad", s_cons, 3},
  {"section", s_change_section, 0},
  {"short", s_cons, 1},
  {"single", s_float_cons, 'f'},
  {"text", s_change_sec, 't'},
  {"word", s_cons, 2},

  {"bgnb", s_ignore, 0},
  {"endb", s_ignore, 0},
  {"file", (void (*) (int)) dwarf2_directive_file, 0 },
  {"loc",  dwarf2_directive_loc,  0 },
  {"verstamp", s_ignore, 0},

  { NULL, NULL, 0 },
};

extern void pop_insert (const pseudo_typeS *);

void
mips_pop_insert (void)
{
  pop_insert (mips_pseudo_table);
}

/* Symbols labelling the current insn.  */

struct insn_label_list
{
  struct insn_label_list *next;
  symbolS *label;
};

static struct insn_label_list *free_insn_labels;
#define label_list tc_segment_info_data.labels

void
mips_clear_insn_labels (void)
{
  register struct insn_label_list **pl;
  segment_info_type *si;

  if (now_seg)
    {
      for (pl = &free_insn_labels; *pl != NULL; pl = &(*pl)->next)
	;
      
      si = seg_info (now_seg);
      *pl = si->label_list;
      si->label_list = NULL;
    }
}


static char *expr_end;

/* Expressions which appear in instructions.  These are set by
   mips_ip.  */

static expressionS imm_expr;
static expressionS offset_expr;

/* Relocs associated with imm_expr and offset_expr.  */

static bfd_reloc_code_real_type imm_reloc = BFD_RELOC_UNUSED;
static bfd_reloc_code_real_type offset_reloc = BFD_RELOC_UNUSED;

/* The default target format to use.  */

const char *
mips_target_format (void)
{
  return rv64 ? "elf64-littleriscv" : "elf32-littleriscv";
}

/* Return the length of instruction INSN.  */

static inline unsigned int
insn_length (const struct mips_cl_insn *insn)
{
  return riscv_insn_length (insn->insn_opcode);
}

#if 0
static int
imm_bits_needed(int32_t imm)
{
  int imm_bits = 32;
  while(imm_bits > 1 && (imm << (32-(imm_bits-1)) >> (32-(imm_bits-1))) == imm)
    imm_bits--;
  return imm_bits;
}

/* return the rvc small register id, if it exists; else, return -1. */
#define ARRAY_FIND(array, x) ({ \
  size_t _pos = ARRAY_SIZE(array), _i; \
  for(_i = 0; _i < ARRAY_SIZE(array); _i++) \
    if((x) == (array)[_i]) \
      { _pos = _i; break; } \
  _pos; })
#define IN_ARRAY(array, x) (ARRAY_FIND(array, x) != ARRAY_SIZE(array))

#define is_rvc_reg(type, x) IN_ARRAY(rvc_##type##_regmap, x)
#define rvc_reg(type, x) ARRAY_FIND(rvc_##type##_regmap, x)

/* If insn can be compressed, compress it and return 1; else return 0. */
static int
riscv_rvc_compress(struct mips_cl_insn* insn)
{
  int rd = EXTRACT_OPERAND(RD, *insn);
  int rs1 = EXTRACT_OPERAND(RS1, *insn);
  int rs2 ATTRIBUTE_UNUSED = EXTRACT_OPERAND(RS2, *insn);
  int32_t imm = EXTRACT_OPERAND(IMMEDIATE, *insn);
  imm = imm << (32-RISCV_IMM_BITS) >> (32-RISCV_IMM_BITS);
  int32_t shamt = imm & 0x3f;
  int32_t bimm = EXTRACT_OPERAND(IMMLO, *insn) |
                 (EXTRACT_OPERAND(IMMHI, *insn) << RISCV_IMMLO_BITS);
  bimm = bimm << (32-RISCV_IMM_BITS) >> (32-RISCV_IMM_BITS);
  int32_t jt = EXTRACT_OPERAND(TARGET, *insn);
  jt = jt << (32-RISCV_JUMP_BITS) >> (32-RISCV_JUMP_BITS);

  gas_assert(insn_length(insn) == 4);

  int imm_bits = imm_bits_needed(imm);
  int bimm_bits = imm_bits_needed(bimm);
  int jt_bits = imm_bits_needed(jt);

  if(INSN_MATCHES(*insn, ADDI) && rd != 0 && rd == rs1 && imm_bits <= 6)
  {
    insn->insn_opcode = MATCH_C_ADDI;
    INSERT_OPERAND(CRD, *insn, rd);
    INSERT_OPERAND(CIMM6, *insn, imm);
  }
  else if(INSN_MATCHES(*insn, ADDIW) && rd != 0 && rd == rs1 && imm_bits <= 6)
  {
    insn->insn_opcode = MATCH_C_ADDIW;
    INSERT_OPERAND(CRD, *insn, rd);
    INSERT_OPERAND(CIMM6, *insn, imm);
  }
  else if(INSN_MATCHES(*insn, JALR) && rd == 0 && imm == 0)
  {
    // jalr rd=0, imm=0 is encoded as c.addi rd=0, imm={1'b0,rs1}
    insn->insn_opcode = MATCH_C_ADDI;
    INSERT_OPERAND(CIMM6, *insn, rs1);
  }
  else if(INSN_MATCHES(*insn, JALR) && rd == 1 && imm == 0)
  {
    // jalr rd=1, rs1, imm=0 is encoded as c.addi rd=0, imm={1'b1,rs1}
    insn->insn_opcode = MATCH_C_ADDI;
    INSERT_OPERAND(CIMM6, *insn, 0x20 | rs1);
  }
  else if((INSN_MATCHES(*insn, ADDI) || INSN_MATCHES(*insn, ORI) ||
          INSN_MATCHES(*insn, XORI)) && rs1 == 0 && imm_bits <= 6)
  {
    insn->insn_opcode = MATCH_C_LI;
    INSERT_OPERAND(CRD, *insn, rd);
    INSERT_OPERAND(CIMM6, *insn, imm);
  }
  else if((INSN_MATCHES(*insn, ADDI) || INSN_MATCHES(*insn, ORI) ||
          INSN_MATCHES(*insn, XORI)) && rs1 == 0 && imm_bits <= 6)
  {
    insn->insn_opcode = MATCH_C_LI;
    INSERT_OPERAND(CRD, *insn, rd);
    INSERT_OPERAND(CIMM6, *insn, imm);
  }
  else if((INSN_MATCHES(*insn, ADDI) || INSN_MATCHES(*insn, ORI) ||
           INSN_MATCHES(*insn, XORI)) && imm == 0)
  {
    insn->insn_opcode = MATCH_C_MOVE;
    INSERT_OPERAND(CRD, *insn, rd);
    INSERT_OPERAND(CRS1, *insn, rs1);
  }
  else if((INSN_MATCHES(*insn, ADD) || INSN_MATCHES(*insn, OR) ||
           INSN_MATCHES(*insn, XOR)) && 
          (rs1 == 0 || rs2 == 0))
  {
    insn->insn_opcode = MATCH_C_MOVE;
    INSERT_OPERAND(CRD, *insn, rd);
    INSERT_OPERAND(CRS1, *insn, rs1 == 0 ? rs2 : rs1);
  }
  else if(INSN_MATCHES(*insn, ADD) && (rd == rs1 || rd == rs2))
  {
    insn->insn_opcode = MATCH_C_ADD;
    INSERT_OPERAND(CRD, *insn, rd);
    INSERT_OPERAND(CRS1, *insn, rd == rs1 ? rs2 : rs1);
  }
  else if(INSN_MATCHES(*insn, SUB) && rd == rs2)
  {
    insn->insn_opcode = MATCH_C_SUB;
    INSERT_OPERAND(CRD, *insn, rd);
    INSERT_OPERAND(CRS1, *insn, rs1);
  }
  else if(INSN_MATCHES(*insn, ADD) && is_rvc_reg(rd, rd) && is_rvc_reg(rs1, rs1) && is_rvc_reg(rs2b, rs2))
  {
    insn->insn_opcode = MATCH_C_ADD3;
    INSERT_OPERAND(CRDS, *insn, rvc_reg(rd, rd));
    INSERT_OPERAND(CRS1S, *insn, rvc_reg(rs1, rs1));
    INSERT_OPERAND(CRS2BS, *insn, rvc_reg(rs2b, rs2));
  }
  else if(INSN_MATCHES(*insn, SUB) && is_rvc_reg(rd, rd) && is_rvc_reg(rs1, rs1) && is_rvc_reg(rs2b, rs2))
  {
    insn->insn_opcode = MATCH_C_SUB3;
    INSERT_OPERAND(CRDS, *insn, rvc_reg(rd, rd));
    INSERT_OPERAND(CRS1S, *insn, rvc_reg(rs1, rs1));
    INSERT_OPERAND(CRS2BS, *insn, rvc_reg(rs2b, rs2));
  }
  else if(INSN_MATCHES(*insn, OR) && is_rvc_reg(rd, rd) && is_rvc_reg(rs1, rs1) && is_rvc_reg(rs2b, rs2))
  {
    insn->insn_opcode = MATCH_C_OR3;
    INSERT_OPERAND(CRDS, *insn, rvc_reg(rd, rd));
    INSERT_OPERAND(CRS1S, *insn, rvc_reg(rs1, rs1));
    INSERT_OPERAND(CRS2BS, *insn, rvc_reg(rs2b, rs2));
  }
  else if(INSN_MATCHES(*insn, AND) && is_rvc_reg(rd, rd) && is_rvc_reg(rs1, rs1) && is_rvc_reg(rs2b, rs2))
  {
    insn->insn_opcode = MATCH_C_AND3;
    INSERT_OPERAND(CRDS, *insn, rvc_reg(rd, rd));
    INSERT_OPERAND(CRS1S, *insn, rvc_reg(rs1, rs1));
    INSERT_OPERAND(CRS2BS, *insn, rvc_reg(rs2b, rs2));
  }
  else if(INSN_MATCHES(*insn, SLLI) && rd == rs1 && is_rvc_reg(rd, rd))
  {
    insn->insn_opcode = shamt >= 32 ? MATCH_C_SLLI32 : MATCH_C_SLLI;
    INSERT_OPERAND(CRDS, *insn, rvc_reg(rd, rd));
    INSERT_OPERAND(CIMM5, *insn, shamt);
  }
  else if(INSN_MATCHES(*insn, SRLI) && rd == rs1 && is_rvc_reg(rd, rd))
  {
    insn->insn_opcode = shamt >= 32 ? MATCH_C_SRLI32 : MATCH_C_SRLI;
    INSERT_OPERAND(CRDS, *insn, rvc_reg(rd, rd));
    INSERT_OPERAND(CIMM5, *insn, shamt);
  }
  else if(INSN_MATCHES(*insn, SRAI) && rd == rs1 && is_rvc_reg(rd, rd))
  {
    insn->insn_opcode = shamt >= 32 ? MATCH_C_SRAI32 : MATCH_C_SRAI;
    INSERT_OPERAND(CRDS, *insn, rvc_reg(rd, rd));
    INSERT_OPERAND(CIMM5, *insn, shamt);
  }
  else if(INSN_MATCHES(*insn, SLLIW) && rd == rs1 && is_rvc_reg(rd, rd))
  {
    insn->insn_opcode = MATCH_C_SLLIW;
    INSERT_OPERAND(CRDS, *insn, rvc_reg(rd, rd));
    INSERT_OPERAND(CIMM5, *insn, shamt);
  }
  else if(INSN_MATCHES(*insn, JAL) && rd == 0 && jt_bits <= 10)
  {
    insn->insn_opcode = MATCH_C_J;
    INSERT_OPERAND(CIMM10, *insn, jt);
  }
  else if(INSN_MATCHES(*insn, BEQ) && rs1 == rs2 && bimm_bits <= 10)
  {
    insn->insn_opcode = MATCH_C_J;
    INSERT_OPERAND(CIMM10, *insn, bimm);
  }
  else if(INSN_MATCHES(*insn, BEQ) && is_rvc_reg(rs1, rs1) && is_rvc_reg(rs2, rs2) && bimm_bits <= 5)
  {
    insn->insn_opcode = MATCH_C_BEQ;
    INSERT_OPERAND(CRS1S, *insn, rvc_reg(rs1, rs1));
    INSERT_OPERAND(CRS2S, *insn, rvc_reg(rs2, rs2));
    INSERT_OPERAND(CIMM5, *insn, bimm);
  }
  else if(INSN_MATCHES(*insn, BNE) && is_rvc_reg(rs1, rs1) && is_rvc_reg(rs2, rs2) && bimm_bits <= 5)
  {
    insn->insn_opcode = MATCH_C_BNE;
    INSERT_OPERAND(CRS1S, *insn, rvc_reg(rs1, rs1));
    INSERT_OPERAND(CRS2S, *insn, rvc_reg(rs2, rs2));
    INSERT_OPERAND(CIMM5, *insn, bimm);
  }
  else if(INSN_MATCHES(*insn, LD) && rs1 == 30 && imm%8 == 0 && imm_bits <= 9)
  {
    insn->insn_opcode = MATCH_C_LDSP;
    INSERT_OPERAND(CRD, *insn, rd);
    INSERT_OPERAND(CIMM6, *insn, imm/8);
  }
  else if(INSN_MATCHES(*insn, LW) && rs1 == 30 && imm%4 == 0 && imm_bits <= 8)
  {
    insn->insn_opcode = MATCH_C_LWSP;
    INSERT_OPERAND(CRD, *insn, rd);
    INSERT_OPERAND(CIMM6, *insn, imm/4);
  }
  else if(INSN_MATCHES(*insn, SD) && rs1 == 30 && bimm%8 == 0 && bimm_bits <= 9)
  {
    insn->insn_opcode = MATCH_C_SDSP;
    INSERT_OPERAND(CRS2, *insn, rs2);
    INSERT_OPERAND(CIMM6, *insn, bimm/8);
  }
  else if(INSN_MATCHES(*insn, SW) && rs1 == 30 && bimm%4 == 0 && bimm_bits <= 8)
  {
    insn->insn_opcode = MATCH_C_SWSP;
    INSERT_OPERAND(CRS2, *insn, rs2);
    INSERT_OPERAND(CIMM6, *insn, bimm/4);
  }
  else if(INSN_MATCHES(*insn, LD) && is_rvc_reg(rs1, rs1) && is_rvc_reg(rd, rd) && imm%8 == 0 && imm_bits <= 8)
  {
    insn->insn_opcode = MATCH_C_LD;
    INSERT_OPERAND(CRS1S, *insn, rvc_reg(rs1, rs1));
    INSERT_OPERAND(CRDS, *insn, rvc_reg(rd, rd));
    INSERT_OPERAND(CIMM5, *insn, imm/8);
  }
  else if(INSN_MATCHES(*insn, LW) && is_rvc_reg(rs1, rs1) && is_rvc_reg(rd, rd) && imm%4 == 0 && imm_bits <= 7)
  {
    insn->insn_opcode = MATCH_C_LW;
    INSERT_OPERAND(CRS1S, *insn, rvc_reg(rs1, rs1));
    INSERT_OPERAND(CRDS, *insn, rvc_reg(rd, rd));
    INSERT_OPERAND(CIMM5, *insn, imm/4);
  }
  else if(INSN_MATCHES(*insn, SD) && is_rvc_reg(rs1, rs1) && is_rvc_reg(rs2, rs2) && bimm%8 == 0 && bimm_bits <= 8)
  {
    insn->insn_opcode = MATCH_C_SD;
    INSERT_OPERAND(CRS1S, *insn, rvc_reg(rs1, rs1));
    INSERT_OPERAND(CRS2S, *insn, rvc_reg(rs2, rs2));
    INSERT_OPERAND(CIMM5, *insn, bimm/8);
  }
  else if(INSN_MATCHES(*insn, SW) && is_rvc_reg(rs1, rs1) && is_rvc_reg(rs2, rs2) && bimm%4 == 0 && bimm_bits <= 7)
  {
    insn->insn_opcode = MATCH_C_SW;
    INSERT_OPERAND(CRS1S, *insn, rvc_reg(rs1, rs1));
    INSERT_OPERAND(CRS2S, *insn, rvc_reg(rs2, rs2));
    INSERT_OPERAND(CIMM5, *insn, bimm/4);
  }
  else if(INSN_MATCHES(*insn, LD) && imm == 0)
  {
    insn->insn_opcode = MATCH_C_LD0;
    INSERT_OPERAND(CRS1, *insn, rs1);
    INSERT_OPERAND(CRD, *insn, rd);
  }
  else if(INSN_MATCHES(*insn, LW) && imm == 0)
  {
    insn->insn_opcode = MATCH_C_LW0;
    INSERT_OPERAND(CRS1, *insn, rs1);
    INSERT_OPERAND(CRD, *insn, rd);
  }
  else if(INSN_MATCHES(*insn, FLD) && is_rvc_reg(rs1, rs1) && is_rvc_reg(rd, rd) && imm%8 == 0 && imm_bits <= 8)
  {
    insn->insn_opcode = MATCH_C_FLD;
    INSERT_OPERAND(CRS1S, *insn, rvc_reg(rs1, rs1));
    INSERT_OPERAND(CRDS, *insn, rvc_reg(rd, rd));
    INSERT_OPERAND(CIMM5, *insn, imm/8);
  }
  else if(INSN_MATCHES(*insn, FLW) && is_rvc_reg(rs1, rs1) && is_rvc_reg(rd, rd) && imm%4 == 0 && imm_bits <= 7)
  {
    insn->insn_opcode = MATCH_C_FLW;
    INSERT_OPERAND(CRS1S, *insn, rvc_reg(rs1, rs1));
    INSERT_OPERAND(CRDS, *insn, rvc_reg(rd, rd));
    INSERT_OPERAND(CIMM5, *insn, imm/4);
  }
  else if(INSN_MATCHES(*insn, FSD) && is_rvc_reg(rs1, rs1) && is_rvc_reg(rs2, rs2) && bimm%8 == 0 && bimm_bits <= 8)
  {
    insn->insn_opcode = MATCH_C_FSD;
    INSERT_OPERAND(CRS1S, *insn, rvc_reg(rs1, rs1));
    INSERT_OPERAND(CRS2S, *insn, rvc_reg(rs2, rs2));
    INSERT_OPERAND(CIMM5, *insn, bimm/8);
  }
  else if(INSN_MATCHES(*insn, FSW) && is_rvc_reg(rs1, rs1) && is_rvc_reg(rs2, rs2) && bimm%4 == 0 && bimm_bits <= 7)
  {
    insn->insn_opcode = MATCH_C_FSW;
    INSERT_OPERAND(CRS1S, *insn, rvc_reg(rs1, rs1));
    INSERT_OPERAND(CRS2S, *insn, rvc_reg(rs2, rs2));
    INSERT_OPERAND(CIMM5, *insn, bimm/4);
  }
  else
    return 0;

  gas_assert(insn_length(insn) == 2);

  return 1;
}
#endif

/* Initialise INSN from opcode entry MO.  Leave its position unspecified.  */

static void
create_insn (struct mips_cl_insn *insn, const struct riscv_opcode *mo)
{
  insn->insn_mo = mo;
  insn->insn_opcode = mo->match;
  insn->frag = NULL;
  insn->where = 0;
  insn->fixp = NULL;
}

/* Install INSN at the location specified by its "frag" and "where" fields.  */

static void
install_insn (const struct mips_cl_insn *insn)
{
  char *f = insn->frag->fr_literal + insn->where;
  md_number_to_chars (f, insn->insn_opcode, insn_length(insn));
}

/* Move INSN to offset WHERE in FRAG.  Adjust the fixups accordingly
   and install the opcode in the new location.  */

static void
move_insn (struct mips_cl_insn *insn, fragS *frag, long where)
{
  insn->frag = frag;
  insn->where = where;
  if (insn->fixp != NULL)
    {
      insn->fixp->fx_frag = frag;
      insn->fixp->fx_where = where;
    }
  install_insn (insn);
}

/* Add INSN to the end of the output.  */

static void
add_fixed_insn (struct mips_cl_insn *insn)
{
  char *f = frag_more (insn_length (insn));
  move_insn (insn, frag_now, f - frag_now->fr_literal);
}

static void
add_relaxed_insn (struct mips_cl_insn *insn, int max_chars, int var,
      relax_substateT subtype, symbolS *symbol, offsetT offset)
{
  frag_grow (max_chars);
  move_insn (insn, frag_now, frag_more (0) - frag_now->fr_literal);
  frag_var (rs_machine_dependent, max_chars, var,
      subtype, symbol, offset, NULL);
}

struct regname {
  const char *name;
  unsigned int num;
};

#define RNUM_MASK	    0x000fff
#define RTYPE_NUM	    0x001000
#define RTYPE_FPU	    0x002000
#define RTYPE_VEC	    0x004000
#define RTYPE_GP	    0x008000
#define RTYPE_CP0	    0x010000
#define RTYPE_VGR_REG	0x020000
#define RTYPE_VFP_REG	0x040000

#define X_REGISTER_NUMBERS \
    {"x0",	RTYPE_NUM | 0},  \
    {"x1",	RTYPE_NUM | 1},  \
    {"x2",	RTYPE_NUM | 2},  \
    {"x3",	RTYPE_NUM | 3},  \
    {"x4",	RTYPE_NUM | 4},  \
    {"x5",	RTYPE_NUM | 5},  \
    {"x6",	RTYPE_NUM | 6},  \
    {"x7",	RTYPE_NUM | 7},  \
    {"x8",	RTYPE_NUM | 8},  \
    {"x9",	RTYPE_NUM | 9},  \
    {"x10",	RTYPE_NUM | 10}, \
    {"x11",	RTYPE_NUM | 11}, \
    {"x12",	RTYPE_NUM | 12}, \
    {"x13",	RTYPE_NUM | 13}, \
    {"x14",	RTYPE_NUM | 14}, \
    {"x15",	RTYPE_NUM | 15}, \
    {"x16",	RTYPE_NUM | 16}, \
    {"x17",	RTYPE_NUM | 17}, \
    {"x18",	RTYPE_NUM | 18}, \
    {"x19",	RTYPE_NUM | 19}, \
    {"x20",	RTYPE_NUM | 20}, \
    {"x21",	RTYPE_NUM | 21}, \
    {"x22",	RTYPE_NUM | 22}, \
    {"x23",	RTYPE_NUM | 23}, \
    {"x24",	RTYPE_NUM | 24}, \
    {"x25",	RTYPE_NUM | 25}, \
    {"x26",	RTYPE_NUM | 26}, \
    {"x27",	RTYPE_NUM | 27}, \
    {"x28",	RTYPE_NUM | 28}, \
    {"x29",	RTYPE_NUM | 29}, \
    {"x30",	RTYPE_NUM | 30}, \
    {"x31",	RTYPE_NUM | 31} 

#define F_REGISTER_NUMBERS       \
    {"f0",	RTYPE_FPU | 0},  \
    {"f1",	RTYPE_FPU | 1},  \
    {"f2",	RTYPE_FPU | 2},  \
    {"f3",	RTYPE_FPU | 3},  \
    {"f4",	RTYPE_FPU | 4},  \
    {"f5",	RTYPE_FPU | 5},  \
    {"f6",	RTYPE_FPU | 6},  \
    {"f7",	RTYPE_FPU | 7},  \
    {"f8",	RTYPE_FPU | 8},  \
    {"f9",	RTYPE_FPU | 9},  \
    {"f10",	RTYPE_FPU | 10}, \
    {"f11",	RTYPE_FPU | 11}, \
    {"f12",	RTYPE_FPU | 12}, \
    {"f13",	RTYPE_FPU | 13}, \
    {"f14",	RTYPE_FPU | 14}, \
    {"f15",	RTYPE_FPU | 15}, \
    {"f16",	RTYPE_FPU | 16}, \
    {"f17",	RTYPE_FPU | 17}, \
    {"f18",	RTYPE_FPU | 18}, \
    {"f19",	RTYPE_FPU | 19}, \
    {"f20",	RTYPE_FPU | 20}, \
    {"f21",	RTYPE_FPU | 21}, \
    {"f22",	RTYPE_FPU | 22}, \
    {"f23",	RTYPE_FPU | 23}, \
    {"f24",	RTYPE_FPU | 24}, \
    {"f25",	RTYPE_FPU | 25}, \
    {"f26",	RTYPE_FPU | 26}, \
    {"f27",	RTYPE_FPU | 27}, \
    {"f28",	RTYPE_FPU | 28}, \
    {"f29",	RTYPE_FPU | 29}, \
    {"f30",	RTYPE_FPU | 30}, \
    {"f31",	RTYPE_FPU | 31}

/* Remaining symbolic register names */
#define X_REGISTER_NAMES \
  { "zero",	 0 | RTYPE_GP }, \
  { "ra",	 1 | RTYPE_GP }, \
  { "s0",	 2 | RTYPE_GP }, \
  { "s1",	 3 | RTYPE_GP }, \
  { "s2",	 4 | RTYPE_GP }, \
  { "s3",	 5 | RTYPE_GP }, \
  { "s4",	 6 | RTYPE_GP }, \
  { "s5",	 7 | RTYPE_GP }, \
  { "s6",	 8 | RTYPE_GP }, \
  { "s7",	 9 | RTYPE_GP }, \
  { "s8",	10 | RTYPE_GP }, \
  { "s9",	11 | RTYPE_GP }, \
  { "s10",	12 | RTYPE_GP }, \
  { "s11",	13 | RTYPE_GP }, \
  { "sp",	14 | RTYPE_GP }, \
  { "tp",	15 | RTYPE_GP }, \
  { "v0",	16 | RTYPE_GP }, \
  { "v1",	17 | RTYPE_GP }, \
  { "a0",	18 | RTYPE_GP }, \
  { "a1",	19 | RTYPE_GP }, \
  { "a2",	20 | RTYPE_GP }, \
  { "a3",	21 | RTYPE_GP }, \
  { "a4",	22 | RTYPE_GP }, \
  { "a5",	23 | RTYPE_GP }, \
  { "a6",	24 | RTYPE_GP }, \
  { "a7",	25 | RTYPE_GP }, \
  { "t0",	26 | RTYPE_GP }, \
  { "t1",	27 | RTYPE_GP }, \
  { "t2",	28 | RTYPE_GP }, \
  { "t3",	29 | RTYPE_GP }, \
  { "t4",	30 | RTYPE_GP }, \
  { "gp",	31 | RTYPE_GP }

#define F_REGISTER_NAMES  \
  { "fs0",	 0 | RTYPE_FPU }, \
  { "fs1",	 1 | RTYPE_FPU }, \
  { "fs2",	 2 | RTYPE_FPU }, \
  { "fs3",	 3 | RTYPE_FPU }, \
  { "fs4",	 4 | RTYPE_FPU }, \
  { "fs5",	 5 | RTYPE_FPU }, \
  { "fs6",	 6 | RTYPE_FPU }, \
  { "fs7",	 7 | RTYPE_FPU }, \
  { "fs8",	 8 | RTYPE_FPU }, \
  { "fs9",	 9 | RTYPE_FPU }, \
  { "fs10",	10 | RTYPE_FPU }, \
  { "fs11",	11 | RTYPE_FPU }, \
  { "fs12",	12 | RTYPE_FPU }, \
  { "fs13",	13 | RTYPE_FPU }, \
  { "fs14",	14 | RTYPE_FPU }, \
  { "fs15",	15 | RTYPE_FPU }, \
  { "fv0",	16 | RTYPE_FPU }, \
  { "fv1",	17 | RTYPE_FPU }, \
  { "fa0",	18 | RTYPE_FPU }, \
  { "fa1",	19 | RTYPE_FPU }, \
  { "fa2",	20 | RTYPE_FPU }, \
  { "fa3",	21 | RTYPE_FPU }, \
  { "fa4",	22 | RTYPE_FPU }, \
  { "fa5",	23 | RTYPE_FPU }, \
  { "fa6",	24 | RTYPE_FPU }, \
  { "fa7",	25 | RTYPE_FPU }, \
  { "ft0",	26 | RTYPE_FPU }, \
  { "ft1",	27 | RTYPE_FPU }, \
  { "ft2",	28 | RTYPE_FPU }, \
  { "ft3",	29 | RTYPE_FPU }, \
  { "ft4",	30 | RTYPE_FPU }, \
  { "ft5",	31 | RTYPE_FPU }

#define RISCV_VEC_GR_REGISTER_NAMES \
    {"vx0",	RTYPE_VGR_REG | 0}, \
    {"vx1",	RTYPE_VGR_REG | 1}, \
    {"vx2",	RTYPE_VGR_REG | 2}, \
    {"vx3",	RTYPE_VGR_REG | 3}, \
    {"vx4",	RTYPE_VGR_REG | 4}, \
    {"vx5",	RTYPE_VGR_REG | 5}, \
    {"vx6",	RTYPE_VGR_REG | 6}, \
    {"vx7",	RTYPE_VGR_REG | 7}, \
    {"vx8",	RTYPE_VGR_REG | 8}, \
    {"vx9",	RTYPE_VGR_REG | 9}, \
    {"vx10",	RTYPE_VGR_REG | 10}, \
    {"vx11",	RTYPE_VGR_REG | 11}, \
    {"vx12",	RTYPE_VGR_REG | 12}, \
    {"vx13",	RTYPE_VGR_REG | 13}, \
    {"vx14",	RTYPE_VGR_REG | 14}, \
    {"vx15",	RTYPE_VGR_REG | 15}, \
    {"vx16",	RTYPE_VGR_REG | 16}, \
    {"vx17",	RTYPE_VGR_REG | 17}, \
    {"vx18",	RTYPE_VGR_REG | 18}, \
    {"vx19",	RTYPE_VGR_REG | 19}, \
    {"vx20",	RTYPE_VGR_REG | 20}, \
    {"vx21",	RTYPE_VGR_REG | 21}, \
    {"vx22",	RTYPE_VGR_REG | 22}, \
    {"vx23",	RTYPE_VGR_REG | 23}, \
    {"vx24",	RTYPE_VGR_REG | 24}, \
    {"vx25",	RTYPE_VGR_REG | 25}, \
    {"vx26",	RTYPE_VGR_REG | 26}, \
    {"vx27",	RTYPE_VGR_REG | 27}, \
    {"vx28",	RTYPE_VGR_REG | 28}, \
    {"vx29",	RTYPE_VGR_REG | 29}, \
    {"vx30",	RTYPE_VGR_REG | 30}, \
    {"vx31",	RTYPE_VGR_REG | 31}

#define RISCV_VEC_GR_SYMBOLIC_REGISTER_NAMES \
    {"vzero",	RTYPE_VGR_REG | 0}, \
    {"vra",	RTYPE_VGR_REG | 1}, \
    {"vs0",	RTYPE_VGR_REG | 2}, \
    {"vs1",	RTYPE_VGR_REG | 3}, \
    {"vs2",	RTYPE_VGR_REG | 4}, \
    {"vs3",	RTYPE_VGR_REG | 5}, \
    {"vs4",	RTYPE_VGR_REG | 6}, \
    {"vs5",	RTYPE_VGR_REG | 7}, \
    {"vs6",	RTYPE_VGR_REG | 8}, \
    {"vs7",	RTYPE_VGR_REG | 9}, \
    {"vs8",	RTYPE_VGR_REG | 10}, \
    {"vs9",	RTYPE_VGR_REG | 11}, \
    {"vs10",	RTYPE_VGR_REG | 12}, \
    {"vs11",	RTYPE_VGR_REG | 13}, \
    {"vsp",	RTYPE_VGR_REG | 14}, \
    {"vtp",	RTYPE_VGR_REG | 15}, \
    {"vv0",	RTYPE_VGR_REG | 16}, \
    {"vv1",	RTYPE_VGR_REG | 17}, \
    {"va0",	RTYPE_VGR_REG | 18}, \
    {"va1",	RTYPE_VGR_REG | 19}, \
    {"va2",	RTYPE_VGR_REG | 20}, \
    {"va3",	RTYPE_VGR_REG | 21}, \
    {"va4",	RTYPE_VGR_REG | 22}, \
    {"va5",	RTYPE_VGR_REG | 23}, \
    {"va6",	RTYPE_VGR_REG | 24}, \
    {"va7",	RTYPE_VGR_REG | 25}, \
    {"vt0",	RTYPE_VGR_REG | 26}, \
    {"vt1",	RTYPE_VGR_REG | 27}, \
    {"vt2",	RTYPE_VGR_REG | 28}, \
    {"vt3",	RTYPE_VGR_REG | 29}, \
    {"vt4",	RTYPE_VGR_REG | 30}, \
    {"vgp",	RTYPE_VGR_REG | 31}

#define RISCV_VEC_FP_REGISTER_NAMES \
    {"vf0",	RTYPE_VFP_REG | 0}, \
    {"vf1",	RTYPE_VFP_REG | 1}, \
    {"vf2",	RTYPE_VFP_REG | 2}, \
    {"vf3",	RTYPE_VFP_REG | 3}, \
    {"vf4",	RTYPE_VFP_REG | 4}, \
    {"vf5",	RTYPE_VFP_REG | 5}, \
    {"vf6",	RTYPE_VFP_REG | 6}, \
    {"vf7",	RTYPE_VFP_REG | 7}, \
    {"vf8",	RTYPE_VFP_REG | 8}, \
    {"vf9",	RTYPE_VFP_REG | 9}, \
    {"vf10",	RTYPE_VFP_REG | 10}, \
    {"vf11",	RTYPE_VFP_REG | 11}, \
    {"vf12",	RTYPE_VFP_REG | 12}, \
    {"vf13",	RTYPE_VFP_REG | 13}, \
    {"vf14",	RTYPE_VFP_REG | 14}, \
    {"vf15",	RTYPE_VFP_REG | 15}, \
    {"vf16",	RTYPE_VFP_REG | 16}, \
    {"vf17",	RTYPE_VFP_REG | 17}, \
    {"vf18",	RTYPE_VFP_REG | 18}, \
    {"vf19",	RTYPE_VFP_REG | 19}, \
    {"vf20",	RTYPE_VFP_REG | 20}, \
    {"vf21",	RTYPE_VFP_REG | 21}, \
    {"vf22",	RTYPE_VFP_REG | 22}, \
    {"vf23",	RTYPE_VFP_REG | 23}, \
    {"vf24",	RTYPE_VFP_REG | 24}, \
    {"vf25",	RTYPE_VFP_REG | 25}, \
    {"vf26",	RTYPE_VFP_REG | 26}, \
    {"vf27",	RTYPE_VFP_REG | 27}, \
    {"vf28",	RTYPE_VFP_REG | 28}, \
    {"vf29",	RTYPE_VFP_REG | 29}, \
    {"vf30",	RTYPE_VFP_REG | 30}, \
    {"vf31",	RTYPE_VFP_REG | 31}

static const struct regname reg_names[] = {
  X_REGISTER_NUMBERS,
  X_REGISTER_NAMES,

  F_REGISTER_NUMBERS,
  F_REGISTER_NAMES,

#define DECLARE_CSR(name, num) {#name, RTYPE_CP0 | num},
#include "opcode/riscv-opc.h"
#undef DECLARE_CSR

  RISCV_VEC_GR_REGISTER_NAMES,
  RISCV_VEC_FP_REGISTER_NAMES,
  RISCV_VEC_GR_SYMBOLIC_REGISTER_NAMES,

  {0, 0}
};

static struct hash_control *reg_names_hash = NULL;

static int
reg_lookup (char **s, unsigned int types, unsigned int *regnop)
{
  struct regname *r;
  char *e;
  char save_c;
  int reg = -1;

  /* Find end of name.  */
  e = *s;
  if (is_name_beginner (*e))
    ++e;
  while (is_part_of_name (*e))
    ++e;

  /* Terminate name.  */
  save_c = *e;
  *e = '\0';

  /* Look for the register.  */
  r = (struct regname *) hash_find (reg_names_hash, *s);
  if (r != NULL && (r->num & types))
    reg = r->num & RNUM_MASK;

  /* Advance to next token if a register was recognised.  */
  if (reg >= 0)
    *s = e;

  *e = save_c;
  if (regnop)
    *regnop = reg;
  return reg >= 0;
}

static unsigned int
reg_lookup_assert (const char *s, unsigned int types)
{
  struct regname *r = (struct regname *) hash_find (reg_names_hash, s);
  gas_assert (r != NULL && (r->num & types));
  return r->num & RNUM_MASK;
}

static int
arg_lookup(char **s, const char* const* array, size_t size, unsigned *regnop)
{
  const char *p = strchr(*s, ',');
  size_t i, len = p ? (size_t)(p - *s) : strlen(*s);
  
  for (i = 0; i < size; i++)
    if (array[i] != NULL && strncmp(array[i], *s, len) == 0)
      {
        *regnop = i;
        *s += len;
        return 1;
      }

  return 0;
}

/* This function is called once, at assembler startup time.  It should set up
   all the tables, etc. that the MD part of the assembler will need.  */

void
md_begin (void)
{
  const char *retval = NULL;
  int i = 0;

  if (! bfd_set_arch_mach (stdoutput, bfd_arch_riscv, 0))
    as_warn (_("Could not set architecture and machine"));

  op_hash = hash_new ();

  for (i = 0; i < NUMOPCODES;)
    {
      const char *name = riscv_opcodes[i].name;

      if (riscv_subset_supports(riscv_opcodes[i].subset))
        retval = hash_insert (op_hash, name, (void *) &riscv_opcodes[i]);

      if (retval != NULL)
	{
	  fprintf (stderr, _("internal error: can't hash `%s': %s\n"),
		   riscv_opcodes[i].name, retval);
	  /* Probably a memory allocation problem?  Give up now.  */
	  as_fatal (_("Broken assembler.  No assembly attempted."));
	}
      do
	{
	  if (riscv_opcodes[i].pinfo != INSN_MACRO)
	    {
	      if (!validate_mips_insn (&riscv_opcodes[i]))
		as_fatal (_("Broken assembler.  No assembly attempted."));
	    }
	  ++i;
	}
      while ((i < NUMOPCODES) && !strcmp (riscv_opcodes[i].name, name));
    }

  reg_names_hash = hash_new ();
  for (i = 0; reg_names[i].name; i++)
    {
      retval = hash_insert (reg_names_hash, reg_names[i].name,
			    (void*) &reg_names[i]);
      if (retval != NULL)
	{
	  fprintf (stderr, _("internal error: can't hash `%s': %s\n"),
		   reg_names[i].name, retval);
	  /* Probably a memory allocation problem?  Give up now.  */
	  as_fatal (_("Broken assembler.  No assembly attempted."));
	}
    }

  mips_clear_insn_labels ();

  /* set the default alignment for the text section (2**2) */
  record_alignment (text_section, 2);
}

void
md_assemble (char *str)
{
  struct mips_cl_insn insn;

  imm_expr.X_op = O_absent;
  offset_expr.X_op = O_absent;
  imm_reloc = BFD_RELOC_UNUSED;
  offset_reloc = BFD_RELOC_UNUSED;

  mips_ip (str, &insn);
  DBG ((_("returned from mips_ip(%s) insn_opcode = 0x%x\n"),
    str, insn.insn_opcode));
  

  if (insn_error)
    {
      as_bad ("%s `%s'", insn_error, str);
      return;
    }

  if (insn.insn_mo->pinfo == INSN_MACRO)
    macro (&insn);
  else
    {
      if (imm_expr.X_op != O_absent)
	append_insn (&insn, &imm_expr, imm_reloc);
      else if (offset_expr.X_op != O_absent)
	append_insn (&insn, &offset_expr, offset_reloc);
      else
	append_insn (&insn, NULL, BFD_RELOC_UNUSED);
    }
}

/* Output an instruction.  IP is the instruction information.
   ADDRESS_EXPR is an operand of the instruction to be used with
   RELOC_TYPE.  */

static void
append_insn (struct mips_cl_insn *ip, expressionS *address_expr,
	     bfd_reloc_code_real_type reloc_type)
{
#ifdef OBJ_ELF
  dwarf2_emit_insn (0);
#endif

  gas_assert(reloc_type <= BFD_RELOC_UNUSED);

#if 0
  /* don't compress instructions with relocs */
  int compressible = (reloc_type == BFD_RELOC_UNUSED ||
    address_expr == NULL || address_expr->X_op == O_constant) && mips_opts.rvc;

  /* speculate that branches/jumps can be compressed.  if not, we'll relax. */
  if (address_expr != NULL && mips_opts.rvc)
  {
    int compressible_branch = reloc_type == BFD_RELOC_12_PCREL &&
      (INSN_MATCHES(*ip, BEQ) || INSN_MATCHES(*ip, BNE));
    int compressible_jump = reloc_type == BFD_RELOC_MIPS_JMP &&
      INSN_MATCHES(*ip, JAL);
    if(compressible_branch || compressible_jump)
    {
      if(riscv_rvc_compress(ip))
      {
        add_relaxed_insn(ip, 4 /* worst case length */, 0,
                         RELAX_BRANCH_ENCODE(compressible_jump, 0),
                         address_expr->X_add_symbol,
                         address_expr->X_add_number);
        reloc_type = BFD_RELOC_UNUSED;
        return;
      }
    }
  }
#endif

  if (address_expr != NULL)
    {
      if (address_expr->X_op == O_constant)
	{
	  switch (reloc_type)
	    {
	    case BFD_RELOC_32:
	      ip->insn_opcode |= address_expr->X_add_number;
	      break;

	    case BFD_RELOC_RISCV_HI20:
	      ip->insn_opcode |= ENCODE_UTYPE_IMM (
		RISCV_LUI_HIGH_PART (address_expr->X_add_number));
	      break;

	    case BFD_RELOC_RISCV_LO12_S:
	      ip->insn_opcode |= ENCODE_STYPE_IMM (address_expr->X_add_number);
	      break;

	    case BFD_RELOC_UNUSED:
	    case BFD_RELOC_RISCV_LO12_I:
	      ip->insn_opcode |= ENCODE_ITYPE_IMM (address_expr->X_add_number);
	      break;

	    default:
	      internalError ();
	    }
	    reloc_type = BFD_RELOC_UNUSED;
	}
      else if (reloc_type == BFD_RELOC_12_PCREL)
	{
	  add_relaxed_insn (ip, relaxed_branch_length (NULL, NULL, 0), 4,
			    RELAX_BRANCH_ENCODE (0, 0, 0),
			    address_expr->X_add_symbol,
			    address_expr->X_add_number);
	  return;
	}
      else if (reloc_type < BFD_RELOC_UNUSED)
	{
	  reloc_howto_type *howto;

	  howto = bfd_reloc_type_lookup (stdoutput, reloc_type);
	  if (howto == NULL)
	    as_bad (_("Unsupported MIPS relocation number %d"), reloc_type);
	 
	  ip->fixp = fix_new_exp (ip->frag, ip->where,
				  bfd_get_reloc_size (howto),
				  address_expr,
				  reloc_type == BFD_RELOC_12_PCREL ||
				  reloc_type == BFD_RELOC_RISCV_CALL ||
				  reloc_type == BFD_RELOC_MIPS_JMP,
				  reloc_type);

	  /* These relocations can have an addend that won't fit in
	     4 octets for 64bit assembly.  */
	  if (rv64
	      && ! howto->partial_inplace
	      && (reloc_type == BFD_RELOC_32
		  || reloc_type == BFD_RELOC_64
		  || reloc_type == BFD_RELOC_CTOR
		  || reloc_type == BFD_RELOC_RISCV_HI20
		  || reloc_type == BFD_RELOC_RISCV_LO12_I
		  || reloc_type == BFD_RELOC_RISCV_LO12_S))
	    ip->fixp->fx_no_overflow = 1;
	}
    }

#if 0
  if (compressible)
    riscv_rvc_compress(ip);
#endif
  add_fixed_insn (ip);

  install_insn (ip);

  /* We just output an insn, so the next one doesn't have a label.  */
  mips_clear_insn_labels ();
}

/* Build an instruction created by a macro expansion.  This is passed
   a pointer to the count of instructions created so far, an
   expression, the name of the instruction to build, an operand format
   string, and corresponding arguments.  */

static void
macro_build (expressionS *ep, const char *name, const char *fmt, ...)
{
  const struct riscv_opcode *mo;
  struct mips_cl_insn insn;
  bfd_reloc_code_real_type r;
  va_list args;

  va_start (args, fmt);

  r = BFD_RELOC_UNUSED;
  mo = (struct riscv_opcode *) hash_find (op_hash, name);
  gas_assert (mo);
  gas_assert (strcmp (name, mo->name) == 0);

  create_insn (&insn, mo);
  for (;;)
    {
      switch (*fmt++)
	{
	case 'd':
	  INSERT_OPERAND (RD, insn, va_arg (args, int));
	  continue;

	case 's':
	  INSERT_OPERAND (RS1, insn, va_arg (args, int));
	  continue;

	case 't':
	  INSERT_OPERAND (RS2, insn, va_arg (args, int));
	  continue;

	case '>':
	  INSERT_OPERAND (SHAMT, insn, va_arg (args, int));
	  continue;

	case 'j':
	case 'u':
	case 'q':
	  gas_assert (ep != NULL);
	  r = va_arg (args, int);
	  continue;

	case '\0':
	  break;
	case ',':
	  continue;
	default:
	  internalError ();
	}
      break;
    }
  va_end (args);
  gas_assert (r == BFD_RELOC_UNUSED ? ep == NULL : ep != NULL);

  append_insn (&insn, ep, r);
}

/*
 * Sign-extend 32-bit mode constants that have bit 31 set and all
 * higher bits unset.
 */
static void
normalize_constant_expr (expressionS *ex)
{
  if (rv64)
    return;
  if (ex->X_op == O_constant
      && IS_ZEXT_32BIT_NUM (ex->X_add_number))
    ex->X_add_number = (((ex->X_add_number & 0xffffffff) ^ 0x80000000)
			- 0x80000000);
}

/*
 * Sign-extend 32-bit mode address offsets that have bit 31 set and
 * all higher bits unset.
 */
static void
normalize_address_expr (expressionS *ex)
{
  if (((ex->X_op == O_constant && HAVE_32BIT_ADDRESSES)
	|| (ex->X_op == O_symbol && HAVE_32BIT_SYMBOLS))
      && IS_ZEXT_32BIT_NUM (ex->X_add_number))
    ex->X_add_number = (((ex->X_add_number & 0xffffffff) ^ 0x80000000)
			- 0x80000000);
}

/* Load an entry from the GOT. */
static void
pcrel_access (int destreg, int tempreg, expressionS *ep,
	      const char* lo_insn, const char* lo_pattern,
              bfd_reloc_code_real_type hi_reloc,
	      bfd_reloc_code_real_type lo_reloc)
{
  macro_build (ep, "auipc", "d,u", tempreg, hi_reloc);
  macro_build (ep, lo_insn, lo_pattern, destreg, tempreg, lo_reloc);
}

static void
pcrel_load (int destreg, int tempreg, expressionS *ep, const char* lo_insn,
            bfd_reloc_code_real_type hi_reloc,
	    bfd_reloc_code_real_type lo_reloc)
{
  pcrel_access (destreg, tempreg, ep, lo_insn, "d,s,j", hi_reloc, lo_reloc);
}

static void
pcrel_store (int srcreg, int tempreg, expressionS *ep, const char* lo_insn,
             bfd_reloc_code_real_type hi_reloc,
	     bfd_reloc_code_real_type lo_reloc)
{
  pcrel_access (srcreg, tempreg, ep, lo_insn, "t,s,q", hi_reloc, lo_reloc);
}

static void
pcrel_vf (int tempreg, expressionS *ep, 
             bfd_reloc_code_real_type hi_reloc,
	     bfd_reloc_code_real_type lo_reloc)
{
  macro_build (ep, "auipc", "d,u", tempreg, hi_reloc);
  macro_build (ep, "vf", "s,q", tempreg, lo_reloc);
}

/* PC-relative function call using AUIPC/JALR, relaxed to JAL. */
static void
riscv_call (int destreg, int tempreg, expressionS *ep,
	    bfd_reloc_code_real_type reloc)
{
  macro_build (ep, "auipc", "d,u", tempreg, reloc);
  macro_build (NULL, "jalr", "d,s", destreg, tempreg);
}

/* Warn if an expression is not a constant.  */

static void
check_absolute_expr (struct mips_cl_insn *ip, expressionS *ex)
{
  if (ex->X_op == O_big)
    as_bad (_("unsupported large constant"));
  else if (ex->X_op != O_constant)
    as_bad (_("Instruction %s requires absolute expression"),
	    ip->insn_mo->name);
  normalize_constant_expr (ex);
}

/* load_const generates an unoptimized instruction sequence to load
 * an absolute expression into a register. */
static void
load_const (int reg, expressionS *ep)
{
  gas_assert (ep->X_op == O_constant);
  gas_assert (reg != ZERO);

  // this is an awful way to generate arbitrary 64-bit constants.
  // fortunately, this is just used for hand-coded assembly programs.
  if (rv64 && !IS_SEXT_32BIT_NUM(ep->X_add_number))
  {
    expressionS upper = *ep, lower = *ep;
    upper.X_add_number = (int64_t)ep->X_add_number >> (RISCV_IMM_BITS-1);
    load_const(reg, &upper);

    macro_build (NULL, "slli", "d,s,>", reg, reg, RISCV_IMM_BITS-1);

    lower.X_add_number = ep->X_add_number & (RISCV_IMM_REACH/2-1);
    if (lower.X_add_number != 0)
      macro_build (&lower, "addi", "d,s,j", reg, reg, BFD_RELOC_RISCV_LO12_I);
  }
  else // load a sign-extended 32-bit constant
  {
    int hi_reg = ZERO;

    int32_t hi = ep->X_add_number & (RISCV_IMM_REACH-1);
    hi = hi << (32-RISCV_IMM_BITS) >> (32-RISCV_IMM_BITS);
    hi = (int32_t)ep->X_add_number - hi;
    if(hi)
    {
      macro_build (ep, "lui", "d,u", reg, BFD_RELOC_RISCV_HI20);
      hi_reg = reg;
    }

    if((ep->X_add_number & (RISCV_IMM_REACH-1)) || hi_reg == ZERO)
      macro_build (ep, ADD32_INSN, "d,s,j", reg, hi_reg, BFD_RELOC_RISCV_LO12_I);
  }
}

/*
 *			Build macros
 *   This routine implements the seemingly endless macro or synthesized
 * instructions and addressing modes in the mips assembly language. Many
 * of these macros are simple and are similar to each other. These could
 * probably be handled by some kind of table or grammar approach instead of
 * this verbose method. Others are not simple macros but are more like
 * optimizing code generation.
 *   One interesting optimization is when several store macros appear
 * consecutively that would load AT with the upper half of the same address.
 * The ensuing load upper instructions are ommited. This implies some kind
 * of global optimization. We currently only optimize within a single macro.
 *   For many of the load and store macros if the address is specified as a
 * constant expression in the first 64k of memory (ie ld $2,0x4000c) we
 * first load register 'at' with zero and use it as the base register. The
 * mips assembler simply uses register $zero. Just one tiny optimization
 * we're missing.
 */
static void
macro (struct mips_cl_insn *ip)
{
  unsigned int rd, rs1, rs2;
  int mask;

  rd = (ip->insn_opcode >> OP_SH_RD) & OP_MASK_RD;
  rs1 = (ip->insn_opcode >> OP_SH_RS1) & OP_MASK_RS1;
  rs2 = (ip->insn_opcode >> OP_SH_RS2) & OP_MASK_RS2;
  mask = ip->insn_mo->mask;

  switch (mask)
    {
    case M_LI:
      load_const (rd, &imm_expr);
      break;

    case M_LA:
    case M_LLA:
      /* Load the address of a symbol into a register. */
      if (!IS_SEXT_32BIT_NUM (offset_expr.X_add_number))
	as_bad(_("offset too large"));

      if (offset_expr.X_op == O_constant)
	load_const (rd, &offset_expr);
      else if (is_pic && mask == M_LA) /* Global PIC symbol */
	pcrel_load (rd, rd, &offset_expr, LOAD_ADDRESS_INSN,
		    BFD_RELOC_RISCV_GOT_HI20, BFD_RELOC_RISCV_GOT_LO12);
      else /* Local PIC symbol, or any non-PIC symbol */
	pcrel_load (rd, rd, &offset_expr, "addi",
		    BFD_RELOC_RISCV_PCREL_HI20, BFD_RELOC_RISCV_PCREL_LO12_I);
      break;

    case M_LA_TLS_GD: 
      pcrel_load (rd, rd, &offset_expr, "addi",
		  BFD_RELOC_RISCV_TLS_GD_HI20, BFD_RELOC_RISCV_TLS_GD_LO12);
      break;

    case M_LA_TLS_IE: 
      pcrel_load (rd, rd, &offset_expr, LOAD_ADDRESS_INSN,
		  BFD_RELOC_RISCV_TLS_GOT_HI20, BFD_RELOC_RISCV_TLS_GOT_LO12);
      break;

    case M_LB:
      pcrel_load (rd, rd, &offset_expr, "lb",
		  BFD_RELOC_RISCV_PCREL_HI20, BFD_RELOC_RISCV_PCREL_LO12_I);
      break;

    case M_LBU:
      pcrel_load (rd, rd, &offset_expr, "lbu",
		  BFD_RELOC_RISCV_PCREL_HI20, BFD_RELOC_RISCV_PCREL_LO12_I);
      break;

    case M_LH:
      pcrel_load (rd, rd, &offset_expr, "lh",
		  BFD_RELOC_RISCV_PCREL_HI20, BFD_RELOC_RISCV_PCREL_LO12_I);
      break;

    case M_LHU:
      pcrel_load (rd, rd, &offset_expr, "lhu",
		  BFD_RELOC_RISCV_PCREL_HI20, BFD_RELOC_RISCV_PCREL_LO12_I);
      break;

    case M_LW:
      pcrel_load (rd, rd, &offset_expr, "lw",
		  BFD_RELOC_RISCV_PCREL_HI20, BFD_RELOC_RISCV_PCREL_LO12_I);
      break;

    case M_LWU:
      pcrel_load (rd, rd, &offset_expr, "lwu",
		  BFD_RELOC_RISCV_PCREL_HI20, BFD_RELOC_RISCV_PCREL_LO12_I);
      break;

    case M_LD:
      pcrel_load (rd, rd, &offset_expr, "ld",
		  BFD_RELOC_RISCV_PCREL_HI20, BFD_RELOC_RISCV_PCREL_LO12_I);
      break;

    case M_FLW:
      pcrel_load (rd, rs1, &offset_expr, "flw",
		  BFD_RELOC_RISCV_PCREL_HI20, BFD_RELOC_RISCV_PCREL_LO12_I);
      break;

    case M_FLD:
      pcrel_load (rd, rs1, &offset_expr, "fld",
		  BFD_RELOC_RISCV_PCREL_HI20, BFD_RELOC_RISCV_PCREL_LO12_I);
      break;

    case M_SB:
      pcrel_store (rs2, rs1, &offset_expr, "sb",
		   BFD_RELOC_RISCV_PCREL_HI20, BFD_RELOC_RISCV_PCREL_LO12_S);
      break;

    case M_SH:
      pcrel_store (rs2, rs1, &offset_expr, "sh",
		   BFD_RELOC_RISCV_PCREL_HI20, BFD_RELOC_RISCV_PCREL_LO12_S);
      break;

    case M_SW:
      pcrel_store (rs2, rs1, &offset_expr, "sw",
		   BFD_RELOC_RISCV_PCREL_HI20, BFD_RELOC_RISCV_PCREL_LO12_S);
      break;

    case M_SD:
      pcrel_store (rs2, rs1, &offset_expr, "sd",
		   BFD_RELOC_RISCV_PCREL_HI20, BFD_RELOC_RISCV_PCREL_LO12_S);
      break;

    case M_FSW:
      pcrel_store (rs2, rs1, &offset_expr, "fsw",
		   BFD_RELOC_RISCV_PCREL_HI20, BFD_RELOC_RISCV_PCREL_LO12_S);
      break;

    case M_FSD:
      pcrel_store (rs2, rs1, &offset_expr, "fsd",
		   BFD_RELOC_RISCV_PCREL_HI20, BFD_RELOC_RISCV_PCREL_LO12_S);
      break;

    case M_VF:
      pcrel_vf (rs1, &offset_expr,
                   BFD_RELOC_RISCV_PCREL_HI20, BFD_RELOC_RISCV_PCREL_LO12_S);
      break;

    case M_JUMP:
      rd = 0;
      goto do_call;
    case M_CALL:
      rd = LINK_REG;
do_call:
      rs1 = reg_lookup_assert ("t0", RTYPE_GP);
      riscv_call (rd, rs1, &offset_expr, offset_reloc);
      break;

    default:
      as_bad (_("Macro %s not implemented"), ip->insn_mo->name);
      break;
    }
}

/* For consistency checking, verify that all bits are specified either
   by the match/mask part of the instruction definition, or by the
   operand list.  */
static int
validate_mips_insn (const struct riscv_opcode *opc)
{
  const char *p = opc->args;
  char c;
  insn_t required_bits, used_bits = opc->mask;

  if ((used_bits & opc->match) != opc->match)
    {
      as_bad (_("internal: bad mips opcode (mask error): %s %s"),
	      opc->name, opc->args);
      return 0;
    }
  required_bits = ((insn_t)1 << (8 * riscv_insn_length (opc->match))) - 1;

#define USE_BITS(mask,shift)	(used_bits |= ((insn_t)(mask) << (shift)))
  while (*p)
    switch (c = *p++)
      {
      /* Xcustom */
      case '^':
      switch (c = *p++)
        {
        case 'd': USE_BITS (OP_MASK_RD, OP_SH_RD); break;
        case 's': USE_BITS (OP_MASK_RS1, OP_SH_RS1); break;
        case 't': USE_BITS (OP_MASK_RS2, OP_SH_RS2); break;
        case 'j': USE_BITS (OP_MASK_CUSTOM_IMM, OP_SH_CUSTOM_IMM); break;
        }
      break;
      /* Xhwacha */
      case '#':
      switch (c = *p++)
        {
        case 'g': USE_BITS (OP_MASK_IMMNGPR, OP_SH_IMMNGPR); break;
        case 'f': USE_BITS (OP_MASK_IMMNFPR, OP_SH_IMMNFPR); break;
        case 'n': USE_BITS (OP_MASK_IMMSEGNELM, OP_SH_IMMSEGNELM); break;
        case 'd': USE_BITS (OP_MASK_VRD, OP_SH_VRD); break;
        case 's': USE_BITS (OP_MASK_VRS, OP_SH_VRS); break;
        case 't': USE_BITS (OP_MASK_VRT, OP_SH_VRT); break;
        case 'r': USE_BITS (OP_MASK_VRR, OP_SH_VRR); break;
        case 'D': USE_BITS (OP_MASK_VFD, OP_SH_VFD); break;
        case 'S': USE_BITS (OP_MASK_VFS, OP_SH_VFS); break;
        case 'T': USE_BITS (OP_MASK_VFT, OP_SH_VFT); break;
        case 'R': USE_BITS (OP_MASK_VFR, OP_SH_VFR); break;

        default:
          as_bad (_("internal: bad mips opcode (unknown extension operand type `#%c'): %s %s"),
                  c, opc->name, opc->args);
          return 0;
        }
      break;
      case ',': break;
      case '(': break;
      case ')': break;
      case '<': USE_BITS (OP_MASK_SHAMTW,	OP_SH_SHAMTW);	break;
      case '>':	USE_BITS (OP_MASK_SHAMT,	OP_SH_SHAMT);	break;
      case 'A': break;
      case 'D':	USE_BITS (OP_MASK_RD,		OP_SH_RD);	break;
      case 'Z':	USE_BITS (OP_MASK_RS1,		OP_SH_RS1);	break;
      case 'E':	USE_BITS (OP_MASK_CSR,		OP_SH_CSR);	break;
      case 'I': break;
      case 'R':	USE_BITS (OP_MASK_RS3,		OP_SH_RS3);	break;
      case 'S':	USE_BITS (OP_MASK_RS1,		OP_SH_RS1);	break;
      case 'U':	USE_BITS (OP_MASK_RS1,		OP_SH_RS1);	/* fallthru */
      case 'T':	USE_BITS (OP_MASK_RS2,		OP_SH_RS2);	break;
      case 'd':	USE_BITS (OP_MASK_RD,		OP_SH_RD);	break;
      case 'm':	USE_BITS (OP_MASK_RM,		OP_SH_RM);	break;
      case 's':	USE_BITS (OP_MASK_RS1,		OP_SH_RS1);	break;
      case 't':	USE_BITS (OP_MASK_RS2,		OP_SH_RS2);	break;
      case 'P':	USE_BITS (OP_MASK_PRED,		OP_SH_PRED); break;
      case 'Q':	USE_BITS (OP_MASK_SUCC,		OP_SH_SUCC); break;
      case 'o':
      case 'j': used_bits |= ENCODE_ITYPE_IMM(-1U); break;
      case 'a':	used_bits |= ENCODE_UJTYPE_IMM(-1U); break;
      case 'p':	used_bits |= ENCODE_SBTYPE_IMM(-1U); break;
      case 'q':	used_bits |= ENCODE_STYPE_IMM(-1U); break;
      case 'u':	used_bits |= ENCODE_UTYPE_IMM(-1U); break;
      case '[': break;
      case ']': break;
      case '0': break;
      default:
	as_bad (_("internal: bad mips opcode (unknown operand type `%c'): %s %s"),
		c, opc->name, opc->args);
	return 0;
      }
#undef USE_BITS
  if (used_bits != required_bits)
    {
      as_bad (_("internal: bad mips opcode (bits 0x%lx undefined): %s %s"),
	      ~(long)(used_bits & required_bits), opc->name, opc->args);
      return 0;
    }
  return 1;
}

struct percent_op_match
{
  const char *str;
  bfd_reloc_code_real_type reloc;
};

static const struct percent_op_match percent_op_utype[] =
{
  {"%tprel_hi", BFD_RELOC_RISCV_TPREL_HI20},
  {"%tls_ie_hi", BFD_RELOC_RISCV_TLS_IE_HI20},
  {"%hi", BFD_RELOC_RISCV_HI20},
  {0, 0}
};

static const struct percent_op_match percent_op_itype[] =
{
  {"%lo", BFD_RELOC_RISCV_LO12_I},
  {"%tprel_lo", BFD_RELOC_RISCV_TPREL_LO12_I},
  {"%tls_ie_lo", BFD_RELOC_RISCV_TLS_IE_LO12},
  {"%tls_ie_off", BFD_RELOC_RISCV_TLS_IE_LO12_I},
  {0, 0}
};

static const struct percent_op_match percent_op_stype[] =
{
  {"%lo", BFD_RELOC_RISCV_LO12_S},
  {"%tprel_lo", BFD_RELOC_RISCV_TPREL_LO12_S},
  {"%tls_ie_off", BFD_RELOC_RISCV_TLS_IE_LO12_S},
  {0, 0}
};

static const struct percent_op_match percent_op_rtype[] =
{
  {"%tprel_add", BFD_RELOC_RISCV_TPREL_ADD},
  {"%tls_ie_add", BFD_RELOC_RISCV_TLS_IE_ADD},
  {0, 0}
};

/* Return true if *STR points to a relocation operator.  When returning true,
   move *STR over the operator and store its relocation code in *RELOC.
   Leave both *STR and *RELOC alone when returning false.  */

static bfd_boolean
parse_relocation (char **str, bfd_reloc_code_real_type *reloc,
		  const struct percent_op_match *percent_op)
{
  for ( ; percent_op->str; percent_op++)
    if (strncasecmp (*str, percent_op->str, strlen (percent_op->str)) == 0)
      {
	int len = strlen (percent_op->str);

	if (!ISSPACE ((*str)[len]) && (*str)[len] != '(')
	  continue;

	*str += strlen (percent_op->str);
	*reloc = percent_op->reloc;

	/* Check whether the output BFD supports this relocation.
	   If not, issue an error and fall back on something safe.  */
	if (!bfd_reloc_type_lookup (stdoutput, percent_op->reloc))
	  {
	    as_bad ("relocation %s isn't supported by the current ABI",
		    percent_op->str);
	    *reloc = BFD_RELOC_UNUSED;
	  }
	return TRUE;
      }
  return FALSE;
}


/* Parse string STR as a 16-bit relocatable operand.  Store the
   expression in *EP and the relocation, if any, in RELOC.
   Return the number of relocation operators used (0 or 1).

   On exit, EXPR_END points to the first character after the expression.  */

static size_t
my_getSmallExpression (expressionS *ep, bfd_reloc_code_real_type *reloc,
		       char *str, const struct percent_op_match *percent_op)
{
  size_t reloc_index;
  int crux_depth, str_depth;
  char *crux;

  /* Search for the start of the main expression.
     End the loop with CRUX pointing to the start
     of the main expression and with CRUX_DEPTH containing the number
     of open brackets at that point.  */
  reloc_index = -1;
  str_depth = 0;
  do
    {
      reloc_index++;
      crux = str;
      crux_depth = str_depth;

      /* Skip over whitespace and brackets, keeping count of the number
	 of brackets.  */
      while (*str == ' ' || *str == '\t' || *str == '(')
	if (*str++ == '(')
	  str_depth++;
    }
  while (*str == '%'
	 && reloc_index < 1
	 && parse_relocation (&str, reloc, percent_op));

  my_getExpression (ep, crux);
  str = expr_end;

  /* Match every open bracket.  */
  while (crux_depth > 0 && (*str == ')' || *str == ' ' || *str == '\t'))
    if (*str++ == ')')
      crux_depth--;

  if (crux_depth > 0)
    as_bad ("unclosed '('");

  expr_end = str;

  return reloc_index;
}

/* This routine assembles an instruction into its binary format.  As a
   side effect, it sets one of the global variables imm_reloc or
   offset_reloc to the type of relocation to do if one of the operands
   is an address expression.  */

static void
mips_ip (char *str, struct mips_cl_insn *ip)
{
  char *s;
  const char *args;
  char c = 0;
  struct riscv_opcode *insn;
  char *argsStart;
  unsigned int regno;
  char save_c = 0;
  int argnum;
  unsigned int rtype;
  const struct percent_op_match *p;

  insn_error = NULL;

  /* If the instruction contains a '.', we first try to match an instruction
     including the '.'.  Then we try again without the '.'.  */
  insn = NULL;
  for (s = str; *s != '\0' && !ISSPACE (*s); ++s)
    continue;

  /* If we stopped on whitespace, then replace the whitespace with null for
     the call to hash_find.  Save the character we replaced just in case we
     have to re-parse the instruction.  */
  if (ISSPACE (*s))
    {
      save_c = *s;
      *s++ = '\0';
    }

  insn = (struct riscv_opcode *) hash_find (op_hash, str);

  /* If we didn't find the instruction in the opcode table, try again, but
     this time with just the instruction up to, but not including the
     first '.'.  */
  if (insn == NULL)
    {
      /* Restore the character we overwrite above (if any).  */
      if (save_c)
	*(--s) = save_c;

      /* Scan up to the first '.' or whitespace.  */
      for (s = str;
	   *s != '\0' && *s != '.' && !ISSPACE (*s);
	   ++s)
	continue;

      /* If we did not find a '.', then we can quit now.  */
      if (*s != '.')
	{
	  insn_error = "unrecognized opcode";
	  return;
	}

      /* Lookup the instruction in the hash table.  */
      *s++ = '\0';
      if ((insn = (struct riscv_opcode *) hash_find (op_hash, str)) == NULL)
	{
	  insn_error = "unrecognized opcode";
	  return;
	}
    }

  argsStart = s;
  for (;;)
    {
      bfd_boolean ok = TRUE;
      gas_assert (strcmp (insn->name, str) == 0);

      create_insn (ip, insn);
      insn_error = NULL;
      argnum = 1;
      for (args = insn->args;; ++args)
	{
	  s += strspn (s, " \t");
	  switch (*args)
	    {
	    case '\0':		/* end of args */
	      if (*s == '\0')
		return;
	      break;
            /* Xcustom */
            case '^':
            {
              unsigned long max = OP_MASK_RD;
              my_getExpression (&imm_expr, s);
              check_absolute_expr (ip, &imm_expr);
              switch (*++args)
                {
                case 'j':
                  max = OP_MASK_CUSTOM_IMM;
                  INSERT_OPERAND (CUSTOM_IMM, *ip, imm_expr.X_add_number);
                  break;
                case 'd':
                  INSERT_OPERAND (RD, *ip, imm_expr.X_add_number);
                  break;
                case 's':
                  INSERT_OPERAND (RS1, *ip, imm_expr.X_add_number);
                  break;
                case 't':
                  INSERT_OPERAND (RS2, *ip, imm_expr.X_add_number);
                  break;
                }
              imm_expr.X_op = O_absent;
              s = expr_end;
              if ((unsigned long) imm_expr.X_add_number > max)
                  as_warn ("Bad custom immediate (%lu), must be at most %lu",
                           (unsigned long)imm_expr.X_add_number, max);
              continue;
            }

            /* Xhwacha */
            case '#':
              switch ( *++args )
                {
                case 'g':
                  my_getExpression( &imm_expr, s );
                  /* check_absolute_expr( ip, &imm_expr ); */
                  if ((unsigned long) imm_expr.X_add_number > 32 )
                    as_warn( _( "Improper ngpr amount (%lu)" ),
                             (unsigned long) imm_expr.X_add_number );
                  INSERT_OPERAND( IMMNGPR, *ip, imm_expr.X_add_number );
                  imm_expr.X_op = O_absent;
                  s = expr_end;
                  continue;
                case 'f':
                  my_getExpression( &imm_expr, s );
                  /* check_absolute_expr( ip, &imm_expr ); */
                  if ((unsigned long) imm_expr.X_add_number > 32 )
                    as_warn( _( "Improper nfpr amount (%lu)" ),
                             (unsigned long) imm_expr.X_add_number );
                  INSERT_OPERAND( IMMNFPR, *ip, imm_expr.X_add_number );
                  imm_expr.X_op = O_absent;
                  s = expr_end;
                  continue;
                case 'n':
                  my_getExpression( &imm_expr, s );
                  /* check_absolute_expr( ip, &imm_expr ); */
                  if ((unsigned long) imm_expr.X_add_number > 8 )
                    as_warn( _( "Improper nelm amount (%lu)" ),
                             (unsigned long) imm_expr.X_add_number );
                  INSERT_OPERAND( IMMSEGNELM, *ip, imm_expr.X_add_number - 1 );
                  imm_expr.X_op = O_absent;
                  s = expr_end;
                  continue;
                case 'd':
                  ok = reg_lookup( &s, RTYPE_NUM|RTYPE_VGR_REG, &regno );
                  if ( !ok )
                    as_bad( _( "Invalid vector register" ) );
                  INSERT_OPERAND( VRD, *ip, regno );
                  continue;
                case 's':
                  ok = reg_lookup( &s, RTYPE_NUM|RTYPE_VGR_REG, &regno );
                  if ( !ok )
                    as_bad( _( "Invalid vector register" ) );
                  INSERT_OPERAND( VRS, *ip, regno );
                  continue;
                case 't':
                  ok = reg_lookup( &s, RTYPE_NUM|RTYPE_VGR_REG, &regno );
                  if ( !ok )
                    as_bad( _( "Invalid vector register" ) );
                  INSERT_OPERAND( VRT, *ip, regno );
                  continue;
                case 'r':
                  ok = reg_lookup( &s, RTYPE_NUM|RTYPE_VGR_REG, &regno );
                  if ( !ok )
                    as_bad( _( "Invalid vector register" ) );
                  INSERT_OPERAND( VRR, *ip, regno );
                  continue;
                case 'D':
                  ok = reg_lookup( &s, RTYPE_NUM|RTYPE_VFP_REG, &regno );
                  if ( !ok )
                    as_bad( _( "Invalid vector register" ) );
                  INSERT_OPERAND( VFD, *ip, regno );
                  continue;
                case 'S':
                  ok = reg_lookup( &s, RTYPE_NUM|RTYPE_VFP_REG, &regno );
                  if ( !ok )
                    as_bad( _( "Invalid vector register" ) );
                  INSERT_OPERAND( VFS, *ip, regno );
                  continue;
                case 'T':
                  ok = reg_lookup( &s, RTYPE_NUM|RTYPE_VFP_REG, &regno );
                  if ( !ok )
                    as_bad( _( "Invalid vector register" ) );
                  INSERT_OPERAND( VFT, *ip, regno );
                  continue;
                case 'R':
                  ok = reg_lookup( &s, RTYPE_NUM|RTYPE_VFP_REG, &regno );
                  if ( !ok )
                    as_bad( _( "Invalid vector register" ) );
                  INSERT_OPERAND( VFR, *ip, regno );
                  continue;
                }
              break;

	    case ',':
	      ++argnum;
	      if (*s++ == *args)
		continue;
	      s--;
	      break;

	    case '(':
	    case ')':
	    case '[':
	    case ']':
	      if (*s++ == *args)
		continue;
	      break;

	    case '<':		/* must be at least one digit */
	      /*
	       * According to the manual, if the shift amount is greater
	       * than 31 or less than 0, then the shift amount should be
	       * mod 32.  In reality the mips assembler issues an error.
	       * We issue a warning and mask out all but the low 5 bits.
	       */
	      my_getExpression (&imm_expr, s);
	      check_absolute_expr (ip, &imm_expr);
	      if ((unsigned long) imm_expr.X_add_number > 31)
		as_warn (_("Improper shift amount (%lu)"),
			 (unsigned long) imm_expr.X_add_number);
	      INSERT_OPERAND (SHAMTW, *ip, imm_expr.X_add_number);
	      imm_expr.X_op = O_absent;
	      s = expr_end;
	      continue;

	    case '>':		/* shift amount, 0-63 */
	      my_getExpression (&imm_expr, s);
	      check_absolute_expr (ip, &imm_expr);
	      if ((unsigned long) imm_expr.X_add_number > (rv64 ? 63 : 31))
		as_warn (_("Improper shift amount (%lu)"),
			 (unsigned long) imm_expr.X_add_number);
	      INSERT_OPERAND (SHAMT, *ip, imm_expr.X_add_number);
	      imm_expr.X_op = O_absent;
	      s = expr_end;
	      continue;

	    case 'Z':		/* CSRRxI immediate */
	      my_getExpression (&imm_expr, s);
	      check_absolute_expr (ip, &imm_expr);
	      if ((unsigned long) imm_expr.X_add_number > 31)
		as_warn (_("Improper CSRxI immediate (%lu)"),
			 (unsigned long) imm_expr.X_add_number);
	      INSERT_OPERAND (RS1, *ip, imm_expr.X_add_number);
	      imm_expr.X_op = O_absent;
	      s = expr_end;
	      continue;

	    case 'E':		/* Control register.  */
	      ok = reg_lookup (&s, RTYPE_NUM | RTYPE_CP0, &regno);
	      INSERT_OPERAND (CSR, *ip, regno);
	      if (ok) 
		continue;
	      else
		break;

            case 'm':		/* rounding mode */
              if (arg_lookup (&s, riscv_rm, ARRAY_SIZE(riscv_rm), &regno))
                {
                  INSERT_OPERAND (RM, *ip, regno);
                  continue;
                }
              break;

	    case 'P':
	    case 'Q':		/* fence predecessor/successor */
              if (arg_lookup (&s, riscv_pred_succ, ARRAY_SIZE(riscv_pred_succ), &regno))
                {
	          if (*args == 'P')
	            INSERT_OPERAND(PRED, *ip, regno);
	          else
	            INSERT_OPERAND(SUCC, *ip, regno);
	          continue;
                }
              break;

	    case 'd':		/* destination register */
	    case 's':		/* source register */
	    case 't':		/* target register */
	      ok = reg_lookup (&s, RTYPE_NUM | RTYPE_GP, &regno);
	      if (ok)
		{
		  c = *args;
		  if (*s == ' ')
		    ++s;

	/* Now that we have assembled one operand, we use the args string
	 * to figure out where it goes in the instruction.  */
		  switch (c)
		    {
		    case 's':
		      INSERT_OPERAND (RS1, *ip, regno);
		      break;
		    case 'd':
		      INSERT_OPERAND (RD, *ip, regno);
		      break;
		    case 't':
		      INSERT_OPERAND (RS2, *ip, regno);
		      break;
		    }
		  continue;
		}
	      break;

	    case 'D':		/* floating point rd */
	    case 'S':		/* floating point rs1 */
	    case 'T':		/* floating point rs2 */
	    case 'U':		/* floating point rs1 and rs2 */
	    case 'R':		/* floating point rs3 */
	      rtype = RTYPE_FPU;
	      if (reg_lookup (&s, rtype, &regno))
		{
		  c = *args;
		  if (*s == ' ')
		    ++s;
		  switch (c)
		    {
		    case 'D':
		      INSERT_OPERAND (RD, *ip, regno);
		      break;
		    case 'S':
		      INSERT_OPERAND (RS1, *ip, regno);
		      break;
		    case 'U':
		      INSERT_OPERAND (RS1, *ip, regno);
		      /* fallthru */
		    case 'T':
		      INSERT_OPERAND (RS2, *ip, regno);
		      break;
		    case 'R':
		      INSERT_OPERAND (RS3, *ip, regno);
		      break;
		    }
		  continue;
		}

	      break;

	    case 'I':
	      my_getExpression (&imm_expr, s);
	      if (imm_expr.X_op != O_big
		  && imm_expr.X_op != O_constant)
		insn_error = _("absolute expression required");
	      normalize_constant_expr (&imm_expr);
	      s = expr_end;
	      continue;

	    case 'A':
	      my_getExpression (&offset_expr, s);
	      normalize_address_expr (&offset_expr);
	      imm_reloc = BFD_RELOC_32;
	      s = expr_end;
	      continue;

	    case 'j': /* sign-extended immediate */
	      imm_reloc = BFD_RELOC_RISCV_LO12_I;
	      p = percent_op_itype;
	      goto alu_op;
	    case 'q': /* store displacement */
	      p = percent_op_stype;
	      offset_reloc = BFD_RELOC_RISCV_LO12_S;
	      goto load_store;
	    case 'o': /* load displacement */
	      p = percent_op_itype;
	      offset_reloc = BFD_RELOC_RISCV_LO12_I;
	      goto load_store;
	    case '0': /* AMO "displacement," which must be zero */
	      p = percent_op_rtype;
	      offset_reloc = BFD_RELOC_UNUSED;
load_store:
	      /* Check whether there is only a single bracketed expression
	         left.  If so, it must be the base register and the
	         constant must be zero.  */
	      offset_expr.X_op = O_constant;
	      offset_expr.X_add_number = 0;
	      if (*s == '(' && strchr (s + 1, '(') == 0)
		continue;
alu_op:
	      /* If this value won't fit into a 16 bit offset, then go
	         find a macro that will generate the 32 bit offset
	         code pattern.  */
	      if (!my_getSmallExpression (&offset_expr, &offset_reloc, s, p))
		{
		  normalize_constant_expr (&offset_expr);
		  if (offset_expr.X_op != O_constant
		      || (*args == '0' && offset_expr.X_add_number != 0)
	              || offset_expr.X_add_number >= (signed)RISCV_IMM_REACH/2
	              || offset_expr.X_add_number < -(signed)RISCV_IMM_REACH/2)
		    break;
		}

	      s = expr_end;
	      continue;

	    case 'p':		/* pc relative offset */
	      offset_reloc = BFD_RELOC_12_PCREL;
	      my_getExpression (&offset_expr, s);
	      s = expr_end;
	      continue;

	    case 'u':		/* upper 20 bits */
	      p = percent_op_utype;
	      if (!my_getSmallExpression (&imm_expr, &imm_reloc, s, p)
		  && imm_expr.X_op == O_constant)
		{
		  if (imm_expr.X_add_number < 0
		      || imm_expr.X_add_number >= (signed)RISCV_BIGIMM_REACH)
		    as_bad (_("lui expression not in range 0..1048575"));
	      
		  imm_reloc = BFD_RELOC_RISCV_HI20;
		  imm_expr.X_add_number <<= RISCV_IMM_BITS;
		}
	      s = expr_end;
	      continue;

	    case 'a':		/* 26 bit address */
	      my_getExpression (&offset_expr, s);
	      s = expr_end;
	      offset_reloc = BFD_RELOC_MIPS_JMP;
	      continue;

	    case 'c':
	      my_getExpression (&offset_expr, s);
	      s = expr_end;
	      offset_reloc = BFD_RELOC_RISCV_CALL;
	      if (*s == '@')
		offset_reloc = BFD_RELOC_RISCV_CALL_PLT, s++;
	      continue;

	    default:
	      as_bad (_("bad char = '%c'\n"), *args);
	      internalError ();
	    }
	  break;
	}
      /* Args don't match.  */
      if (insn + 1 < &riscv_opcodes[NUMOPCODES] &&
	  !strcmp (insn->name, insn[1].name))
	{
	  ++insn;
	  s = argsStart;
	  insn_error = _("illegal operands");
	  continue;
	}
      if (save_c)
	*(--argsStart) = save_c;
      insn_error = _("illegal operands");
      return;
    }
}

static void
my_getExpression (expressionS *ep, char *str)
{
  char *save_in;

  save_in = input_line_pointer;
  input_line_pointer = str;
  expression (ep);
  expr_end = input_line_pointer;
  input_line_pointer = save_in;
}

char *
md_atof (int type, char *litP, int *sizeP)
{
  return ieee_md_atof (type, litP, sizeP, TARGET_BYTES_BIG_ENDIAN);
}

void
md_number_to_chars (char *buf, valueT val, int n)
{
  number_to_chars_littleendian (buf, val, n);
}

const char *md_shortopts = "O::g::G:";

enum options
  {
    OPTION_M32 = OPTION_MD_BASE,
    OPTION_M64,
    OPTION_MARCH,
    OPTION_PIC,
    OPTION_NO_PIC,
    OPTION_MRVC,
    OPTION_MNO_RVC,
    OPTION_END_OF_ENUM    
  };
  
struct option md_longopts[] =
{
  {"m32", no_argument, NULL, OPTION_M32},
  {"m64", no_argument, NULL, OPTION_M64},
  {"march", required_argument, NULL, OPTION_MARCH},
  {"fPIC", no_argument, NULL, OPTION_PIC},
  {"fpic", no_argument, NULL, OPTION_PIC},
  {"fno-pic", no_argument, NULL, OPTION_NO_PIC},
  {"mrvc", no_argument, NULL, OPTION_MRVC},
  {"mno-rvc", no_argument, NULL, OPTION_MNO_RVC},

  {NULL, no_argument, NULL, 0}
};
size_t md_longopts_size = sizeof (md_longopts);

int
md_parse_option (int c, char *arg)
{
  switch (c)
    {
    case 'g':
      if (arg == NULL)
	mips_debug = 2;
      else
	mips_debug = atoi (arg);
      break;

    case OPTION_MRVC:
      mips_opts.rvc = 1;
      break;

    case OPTION_MNO_RVC:
      mips_opts.rvc = 0;
      break;

    case OPTION_M32:
      rv64 = FALSE;
      break;

    case OPTION_M64:
      rv64 = TRUE;
      break;

    case OPTION_MARCH:
      riscv_set_arch(arg);

    case OPTION_NO_PIC:
      is_pic = FALSE;
      break;

    case OPTION_PIC:
      is_pic = TRUE;
      break;

    default:
      return 0;
    }

  return 1;
}

void
mips_after_parse_args (void)
{
  if (riscv_subsets == NULL)
    riscv_set_arch("RVIMAFDXcustom");
}

void
mips_init_after_args (void)
{
  /* initialize opcodes */
  bfd_riscv_num_opcodes = bfd_riscv_num_builtin_opcodes;
  riscv_opcodes = (struct riscv_opcode *) riscv_builtin_opcodes;
}

long
md_pcrel_from (fixS *fixP)
{
  return fixP->fx_where + fixP->fx_frag->fr_address;
}

/* Apply a fixup to the object file.  */

void
md_apply_fix (fixS *fixP, valueT *valP, segT seg ATTRIBUTE_UNUSED)
{
  bfd_byte *buf = (bfd_byte *) (fixP->fx_frag->fr_literal + fixP->fx_where);
  bfd_reloc_code_real_type pcrel_r_type;

  /* We ignore generic BFD relocations we don't know about.  */
  if (! bfd_reloc_type_lookup (stdoutput, fixP->fx_r_type))
    return;

  /* Remember value for tc_gen_reloc.  */
  fixP->fx_addnumber = *valP;

  switch (fixP->fx_r_type)
    {
    case BFD_RELOC_MIPS_TLS_DTPREL32:
    case BFD_RELOC_MIPS_TLS_DTPREL64:
    case BFD_RELOC_RISCV_TPREL_HI20:
    case BFD_RELOC_RISCV_TPREL_LO12_I:
    case BFD_RELOC_RISCV_TPREL_LO12_S:
    case BFD_RELOC_RISCV_TPREL_ADD:
    case BFD_RELOC_RISCV_TLS_IE_HI20:
    case BFD_RELOC_RISCV_TLS_IE_LO12:
    case BFD_RELOC_RISCV_TLS_IE_ADD:
    case BFD_RELOC_RISCV_TLS_IE_LO12_I:
    case BFD_RELOC_RISCV_TLS_IE_LO12_S:
      S_SET_THREAD_LOCAL (fixP->fx_addsy);
      /* fall through */

    case BFD_RELOC_RISCV_TLS_GOT_HI20:
    case BFD_RELOC_RISCV_TLS_GD_HI20:
    case BFD_RELOC_RISCV_GOT_HI20:
    case BFD_RELOC_RISCV_PCREL_HI20:
    case BFD_RELOC_RISCV_HI20:
    case BFD_RELOC_RISCV_LO12_I:
    case BFD_RELOC_RISCV_LO12_S:
    case BFD_RELOC_RISCV_ADD32:
    case BFD_RELOC_RISCV_ADD64:
    case BFD_RELOC_RISCV_SUB32:
    case BFD_RELOC_RISCV_SUB64:
      gas_assert (fixP->fx_addsy != NULL);
      /* Nothing needed to do.  The value comes from the reloc entry.  */
      return;

    case BFD_RELOC_64:
    case BFD_RELOC_32:
      if (fixP->fx_addsy && fixP->fx_subsy)
	{
	  fixP->fx_next = xmemdup (fixP, sizeof (*fixP), sizeof (*fixP));
	  fixP->fx_next->fx_addsy = fixP->fx_subsy;
	  fixP->fx_next->fx_subsy = NULL;
	  fixP->fx_next->fx_offset = 0;
	  fixP->fx_subsy = NULL;

	  if (fixP->fx_r_type == BFD_RELOC_64)
	    fixP->fx_r_type = BFD_RELOC_RISCV_ADD64,
	    fixP->fx_next->fx_r_type = BFD_RELOC_RISCV_SUB64;
	  else
	    fixP->fx_r_type = BFD_RELOC_RISCV_ADD32,
	    fixP->fx_next->fx_r_type = BFD_RELOC_RISCV_SUB32;
	}
      /* fall through */

    case BFD_RELOC_RVA:
      /* If we are deleting this reloc entry, we must fill in the
	 value now.  This can happen if we have a .word which is not
	 resolved when it appears but is later defined.  */
      if (fixP->fx_addsy == NULL)
	{
	  gas_assert (fixP->fx_size <= sizeof (valueT));
	  md_number_to_chars ((char *) buf, *valP, fixP->fx_size);
	  fixP->fx_done = 1;
	}
      return;

    case BFD_RELOC_RISCV_TLS_GOT_LO12:
    case BFD_RELOC_RISCV_TLS_GD_LO12:
      gas_assert (fixP->fx_addsy != NULL);
      S_SET_THREAD_LOCAL (fixP->fx_addsy);
      pcrel_r_type = BFD_RELOC_RISCV_TLS_PCREL_LO12;
      break;

    case BFD_RELOC_RISCV_GOT_LO12:
      gas_assert (fixP->fx_addsy != NULL);
      pcrel_r_type = BFD_RELOC_RISCV_PCREL_LO12_I;
      break;

    case BFD_RELOC_RISCV_PCREL_LO12_S:
      if (fixP->fx_addsy != NULL)
	{
	  fixP->fx_r_type = BFD_RELOC_RISCV_LO12_S;
	  pcrel_r_type = BFD_RELOC_RISCV_PCREL_LO12_S;
	  break;
	}
      return;

    case BFD_RELOC_RISCV_PCREL_LO12_I:
      if (fixP->fx_addsy != NULL)
	{
	  fixP->fx_r_type = BFD_RELOC_RISCV_LO12_I;
	  pcrel_r_type = BFD_RELOC_RISCV_PCREL_LO12_I;
	  break;
	}
      return;

    case BFD_RELOC_RISCV_TLS_PCREL_LO12:
    case BFD_RELOC_RISCV_CALL:
    case BFD_RELOC_RISCV_CALL_PLT:
    case BFD_RELOC_MIPS_JMP:
    case BFD_RELOC_12_PCREL:
      return;

    default:
      internalError ();
    }

  /* We only get here for the low part of split PC-relative relocs.
     Record the distance between the high and low parts.  For now,
     we assume the high part (AUIPC) is immediately before us, and
     so this distance is -4. */
  gas_assert (fixP->fx_subsy == NULL);
  fixP->fx_next = xmemdup (fixP, sizeof (*fixP), sizeof (*fixP));
  fixP->fx_next->fx_addsy = NULL;
  fixP->fx_next->fx_offset = -4;
  fixP->fx_next->fx_r_type = pcrel_r_type;
}

/* Align the current frag to a given power of two.  If a particular
   fill byte should be used, FILL points to an integer that contains
   that byte, otherwise FILL is null.

   The MIPS assembler also automatically adjusts any preceding
   label.  */

static void
mips_align (int to, int *fill, symbolS *label)
{
  mips_clear_insn_labels ();
  if (fill == NULL && subseg_text_p (now_seg))
    frag_align_code (to, 0);
  else
    frag_align (to, fill ? *fill : 0, 0);
  record_alignment (now_seg, to);
  if (label != NULL)
    {
      gas_assert (S_GET_SEGMENT (label) == now_seg);
      symbol_set_frag (label, frag_now);
      S_SET_VALUE (label, (valueT) frag_now_fix ());
    }
}

/* Align to a given power of two.  .align 0 turns off the automatic
   alignment used by the data creating pseudo-ops.  */

static void
s_align (int x ATTRIBUTE_UNUSED)
{
  int temp, fill_value, *fill_ptr;
  long max_alignment = 28;

  /* o Note that the assembler pulls down any immediately preceding label
       to the aligned address.
     o It's not documented but auto alignment is reinstated by
       a .align pseudo instruction.
     o Note also that after auto alignment is turned off the mips assembler
       issues an error on attempt to assemble an improperly aligned data item.
       We don't.  */

  temp = get_absolute_expression ();
  if (temp > max_alignment)
    as_bad (_("Alignment too large: %d. assumed."), temp = max_alignment);
  else if (temp < 0)
    {
      as_warn (_("Alignment negative: 0 assumed."));
      temp = 0;
    }
  if (*input_line_pointer == ',')
    {
      ++input_line_pointer;
      fill_value = get_absolute_expression ();
      fill_ptr = &fill_value;
    }
  else
    fill_ptr = 0;
  if (temp)
    {
      segment_info_type *si = seg_info (now_seg);
      struct insn_label_list *l = si->label_list;
      /* Auto alignment should be switched on by next section change.  */
      auto_align = 1;
      mips_align (temp, fill_ptr, l != NULL ? l->label : NULL);
    }
  else
    {
      auto_align = 0;
    }

  demand_empty_rest_of_line ();
}

static void
s_change_sec (int sec)
{
#ifdef OBJ_ELF
  /* The ELF backend needs to know that we are changing sections, so
     that .previous works correctly.  We could do something like check
     for an obj_section_change_hook macro, but that might be confusing
     as it would not be appropriate to use it in the section changing
     functions in read.c, since obj-elf.c intercepts those.  FIXME:
     This should be cleaner, somehow.  */
  if (IS_ELF)
    obj_elf_section_change_hook ();
#endif

  mips_clear_insn_labels ();

  switch (sec)
    {
    case 't':
      s_text (0);
      break;
    case 'd':
      s_data (0);
      break;
    case 'b':
      subseg_set (bss_section, (subsegT) get_absolute_expression ());
      demand_empty_rest_of_line ();
      break;
    case 'r':
      subseg_new (".rodata", (subsegT) get_absolute_expression ());
      demand_empty_rest_of_line ();
      break;
    }

  auto_align = 1;
}

void
s_change_section (int ignore ATTRIBUTE_UNUSED)
{
#ifdef OBJ_ELF
  char *section_name;
  char c;
  char next_c = 0;
  int section_type;
  int section_flag;
  int section_entry_size;

  if (!IS_ELF)
    return;

  section_name = input_line_pointer;
  c = get_symbol_end ();
  if (c)
    next_c = *(input_line_pointer + 1);

  /* Do we have .section Name<,"flags">?  */
  if (c != ',' || (c == ',' && next_c == '"'))
    {
      /* just after name is now '\0'.  */
      *input_line_pointer = c;
      input_line_pointer = section_name;
      obj_elf_section (ignore);
      return;
    }
  input_line_pointer++;

  /* Do we have .section Name<,type><,flag><,entry_size><,alignment>  */
  if (c == ',')
    section_type = get_absolute_expression ();
  else
    section_type = 0;
  if (*input_line_pointer++ == ',')
    section_flag = get_absolute_expression ();
  else
    section_flag = 0;
  if (*input_line_pointer++ == ',')
    section_entry_size = get_absolute_expression ();
  else
    section_entry_size = 0;

  section_name = xstrdup (section_name);

  obj_elf_change_section (section_name, section_type, section_flag,
			  section_entry_size, 0, 0, 0);

  if (now_seg->name != section_name)
    free (section_name);
#endif /* OBJ_ELF */
}

void
mips_enable_auto_align (void)
{
  auto_align = 1;
}

static void
s_cons (int log_size)
{
  segment_info_type *si = seg_info (now_seg);
  struct insn_label_list *l = si->label_list;
  symbolS *label;

  label = l != NULL ? l->label : NULL;
  mips_clear_insn_labels ();
  if (log_size > 0 && auto_align)
    mips_align (log_size, 0, label);
  mips_clear_insn_labels ();
  cons (1 << log_size);
}

static void
s_float_cons (int type)
{
  segment_info_type *si = seg_info (now_seg);
  struct insn_label_list *l = si->label_list;
  symbolS *label;

  label = l != NULL ? l->label : NULL;

  mips_clear_insn_labels ();

  if (auto_align)
    {
      if (type == 'd')
	mips_align (3, 0, label);
      else
	mips_align (2, 0, label);
    }

  mips_clear_insn_labels ();

  float_cons (type);
}

/* This structure is used to hold a stack of .set values.  */

struct mips_option_stack
{
  struct mips_option_stack *next;
  struct mips_set_options options;
};

static struct mips_option_stack *mips_opts_stack;

/* Handle the .set pseudo-op.  */

static void
s_riscv_option (int x ATTRIBUTE_UNUSED)
{
  char *name = input_line_pointer, ch;

  while (!is_end_of_line[(unsigned char) *input_line_pointer])
    ++input_line_pointer;
  ch = *input_line_pointer;
  *input_line_pointer = '\0';

  if (strcmp (name, "rvc") == 0)
    mips_opts.rvc = 1;
  else if (strcmp (name, "norvc") == 0)
    mips_opts.rvc = 0;
  else if (strcmp (name, "push") == 0)
    {
      struct mips_option_stack *s;

      s = (struct mips_option_stack *) xmalloc (sizeof *s);
      s->next = mips_opts_stack;
      s->options = mips_opts;
      mips_opts_stack = s;
    }
  else if (strcmp (name, "pop") == 0)
    {
      struct mips_option_stack *s;

      s = mips_opts_stack;
      if (s == NULL)
	as_bad (_(".set pop with no .set push"));
      else
	{
	  mips_opts = s->options;
	  mips_opts_stack = s->next;
	  free (s);
	}
    }
  else if (strchr (name, ','))
    {
      /* Generic ".set" directive; use the generic handler.  */
      *input_line_pointer = ch;
      input_line_pointer = name;
      s_set (0);
      return;
    }
  else
    {
      as_warn (_("Tried to set unrecognized symbol: %s\n"), name);
    }
  *input_line_pointer = ch;
  demand_empty_rest_of_line ();
}

/* Handle the .dtprelword and .dtpreldword pseudo-ops.  They generate
   a 32-bit or 64-bit DTP-relative relocation (BYTES says which) for
   use in DWARF debug information.  */

static void
s_dtprel_internal (size_t bytes)
{
  expressionS ex;
  char *p;

  expression (&ex);

  if (ex.X_op != O_symbol)
    {
      as_bad (_("Unsupported use of %s"), (bytes == 8
					   ? ".dtpreldword"
					   : ".dtprelword"));
      ignore_rest_of_line ();
    }

  p = frag_more (bytes);
  md_number_to_chars (p, 0, bytes);
  fix_new_exp (frag_now, p - frag_now->fr_literal, bytes, &ex, FALSE,
	       (bytes == 8
		? BFD_RELOC_MIPS_TLS_DTPREL64
		: BFD_RELOC_MIPS_TLS_DTPREL32));

  demand_empty_rest_of_line ();
}

/* Handle .dtprelword.  */

static void
s_dtprelword (int ignore ATTRIBUTE_UNUSED)
{
  s_dtprel_internal (4);
}

/* Handle .dtpreldword.  */

static void
s_dtpreldword (int ignore ATTRIBUTE_UNUSED)
{
  s_dtprel_internal (8);
}

/* Compute the length of a branch sequence, and adjust the
   RELAX_BRANCH_TOOFAR bit accordingly.  If FRAGP is NULL, the
   worst-case length is computed. */
static int
relaxed_branch_length (fragS *fragp, asection *sec, int update)
{
  bfd_boolean toofar_rvc = TRUE, toofar = TRUE;

  if (fragp)
    {
      bfd_boolean uncond = RELAX_BRANCH_UNCOND (fragp->fr_subtype);
      bfd_boolean rvc = RELAX_BRANCH_RVC (fragp->fr_subtype);

      if (S_IS_DEFINED (fragp->fr_symbol)
	  && sec == S_GET_SEGMENT (fragp->fr_symbol))
	{
	  offsetT val = S_GET_VALUE (fragp->fr_symbol) + fragp->fr_offset;
	  bfd_vma range;
	  val -= fragp->fr_address + fragp->fr_fix;

	  if (uncond && rvc)
	    range = RVC_JUMP_REACH;
	  else if (rvc)
	    range = RVC_BRANCH_REACH;
	  else if (uncond)
	    range = RISCV_JUMP_REACH;
	  else
	    range = RISCV_BRANCH_REACH;
	  toofar = (bfd_vma)(val + range/2) >= range;
	}

      if (update && toofar != RELAX_BRANCH_TOOFAR (fragp->fr_subtype))
	fragp->fr_subtype = RELAX_BRANCH_ENCODE (uncond, rvc, toofar);
    }

  return toofar ? 8 : toofar_rvc ? 4 : 2;
}

int
md_estimate_size_before_relax (fragS *fragp, asection *segtype)
{
  return (fragp->fr_var = relaxed_branch_length (fragp, segtype, FALSE));
}

/* Translate internal representation of relocation info to BFD target
   format.  */

arelent **
tc_gen_reloc (asection *section ATTRIBUTE_UNUSED, fixS *fixp)
{
  static arelent *retval[4];
  arelent *reloc;
  bfd_reloc_code_real_type code;

  memset (retval, 0, sizeof(retval));
  reloc = retval[0] = (arelent *) xcalloc (1, sizeof (arelent));
  reloc->sym_ptr_ptr = (asymbol **) xmalloc (sizeof (asymbol *));
  *reloc->sym_ptr_ptr = symbol_get_bfdsym (fixp->fx_addsy);
  reloc->address = fixp->fx_frag->fr_address + fixp->fx_where;

  if (fixp->fx_pcrel)
    /* At this point, fx_addnumber is "symbol offset - pcrel address".
       Relocations want only the symbol offset.  */
    reloc->addend = fixp->fx_addnumber + reloc->address;
  else
    reloc->addend = fixp->fx_addnumber;

  code = fixp->fx_r_type;

  reloc->howto = bfd_reloc_type_lookup (stdoutput, code);
  if (reloc->howto == NULL)
    {
      as_bad_where (fixp->fx_file, fixp->fx_line,
		    _("Can not represent %s relocation in this object file format"),
		    bfd_get_reloc_code_name (code));
      retval[0] = NULL;
    }

  return retval;
}

int
mips_relax_frag (asection *sec, fragS *fragp, long stretch ATTRIBUTE_UNUSED)
{
  if (RELAX_BRANCH_P (fragp->fr_subtype))
    {
      offsetT old_var = fragp->fr_var;
      fragp->fr_var = relaxed_branch_length (fragp, sec, TRUE);
      return fragp->fr_var - old_var;
    }

  return 0;
}

/* Convert a machine dependent frag.  */

static void
md_convert_frag_branch (bfd *abfd ATTRIBUTE_UNUSED, segT asec ATTRIBUTE_UNUSED,
                 fragS *fragp)
{
  bfd_byte *buf;
  insn_t insn;
  expressionS exp;
  fixS *fixp;

  buf = (bfd_byte *)fragp->fr_literal + fragp->fr_fix;

  exp.X_op = O_symbol;
  exp.X_add_symbol = fragp->fr_symbol;
  exp.X_add_number = fragp->fr_offset;

#if 0
  if (RELAX_BRANCH_RVC (fragp->fr_subtype))
    {
      if (RELAX_BRANCH_TOOFAR (fragp->fr_subtype))
	{
	  bfd_reloc_code_real_type reloc_type = BFD_RELOC_12_PCREL;

	  gas_assert(fragp->fr_var == 4);
	  insn = bfd_getl16 (buf);

	  int rs1 = rvc_rs1_regmap[(insn >> OP_SH_CRS1S) & OP_MASK_CRS1S];
	  int rs2 = rvc_rs2_regmap[(insn >> OP_SH_CRS2S) & OP_MASK_CRS2S];

	  if((insn & MASK_C_J) == MATCH_C_J)
	    {
	      insn = MATCH_JAL;
	      reloc_type = BFD_RELOC_MIPS_JMP;
	    }
	  else if((insn & MASK_C_BEQ) == MATCH_C_BEQ)
	    insn = MATCH_BEQ | (rs1 << OP_SH_RS1) | (rs2 << OP_SH_RS2);
	  else if((insn & MASK_C_BNE) == MATCH_C_BNE)
	    insn = MATCH_BNE | (rs1 << OP_SH_RS1) | (rs2 << OP_SH_RS2);
	  else
	    gas_assert(0);

	  fixp = fix_new_exp (fragp, buf - (bfd_byte *)fragp->fr_literal,
			      4, &exp, FALSE, reloc_type);
	  md_number_to_chars ((char *) buf, insn, 4);
	  buf += 4;
	}
      else
	{
	  fixp = fix_new_exp (fragp, buf - (bfd_byte *)fragp->fr_literal,
			      2, &exp, FALSE, BFD_RELOC_12_PCREL);
	  buf += 2;
	}
    }
  else
#endif
    {
      if (RELAX_BRANCH_TOOFAR (fragp->fr_subtype))
	{
	  gas_assert (fragp->fr_var == 8);
	  /* We could relax JAL to AUIPC/JALR, but we don't do this yet. */
	  gas_assert (!RELAX_BRANCH_UNCOND (fragp->fr_subtype));

	  /* Invert the branch condition.  Branch over the jump. */
	  insn = bfd_getl32 (buf);
	  insn ^= MATCH_BEQ ^ MATCH_BNE;
	  insn |= ENCODE_SBTYPE_IMM (8);
	  md_number_to_chars ((char *) buf, insn, 4);
	  buf += 4;

	  /* Jump to the target. */
	  fixp = fix_new_exp (fragp, buf - (bfd_byte *)fragp->fr_literal,
			  4, &exp, FALSE, BFD_RELOC_MIPS_JMP);
	  md_number_to_chars ((char *) buf, MATCH_JAL, 4);
	  buf += 4;
	}
      else
	{
	  fixp = fix_new_exp (fragp, buf - (bfd_byte *)fragp->fr_literal,
			      4, &exp, FALSE, BFD_RELOC_12_PCREL);
	  buf += 4;
      }
    }

  fixp->fx_file = fragp->fr_file;
  fixp->fx_line = fragp->fr_line;
  fixp->fx_pcrel = 1;

  gas_assert (buf == (bfd_byte *)fragp->fr_literal
          + fragp->fr_fix + fragp->fr_var);

  fragp->fr_fix += fragp->fr_var;
}

/* Relax a machine dependent frag.  This returns the amount by which
   the current size of the frag should change.  */

void
md_convert_frag(bfd *abfd, segT asec, fragS *fragp)
{
  if(RELAX_BRANCH_P(fragp->fr_subtype))
    md_convert_frag_branch(abfd, asec, fragp);
  else
    gas_assert(0);
}

/* This function is called whenever a label is defined.  It is used
   when handling branch delays; if a branch has a label, we assume we
   can not move it.  */

void
mips_define_label (symbolS *sym)
{
  segment_info_type *si = seg_info (now_seg);
  struct insn_label_list *l;

  if (free_insn_labels == NULL)
    l = (struct insn_label_list *) xmalloc (sizeof *l);
  else
    {
      l = free_insn_labels;
      free_insn_labels = l->next;
    }

  l->label = sym;
  l->next = si->label_list;
  si->label_list = l;

#ifdef OBJ_ELF
  dwarf2_emit_label (sym);
#endif
}

void
mips_handle_align (fragS *fragp)
{
  char *p;

  if (fragp->fr_type != rs_align_code)
    return;

  p = fragp->fr_literal + fragp->fr_fix;
  md_number_to_chars (p, RISCV_NOP, 4);
  fragp->fr_var = 4;
}

void
md_show_usage (FILE *stream)
{
  fprintf (stream, _("\
RISC-V options:\n\
  -m32           assemble RV32 code\n\
  -m64           assemble RV64 code (default)\n\
  -fpic          generate position-independent code\n\
  -fno-pic       don't generate position-independent code (default)\n\
"));
}

enum dwarf2_format
mips_dwarf2_format (asection *sec ATTRIBUTE_UNUSED)
{
  if (HAVE_32BIT_SYMBOLS)
    return dwarf2_format_32bit;
  else
    return dwarf2_format_64bit;
}

int
mips_dwarf2_addr_size (void)
{
  return rv64 ? 8 : 4;
}

/* Standard calling conventions leave the CFA at SP on entry.  */
void
mips_cfi_frame_initial_instructions (void)
{
  cfi_add_CFA_def_cfa_register (SP);
}

int
tc_mips_regname_to_dw2regnum (char *regname)
{
  unsigned int regnum = -1;
  unsigned int reg;

  if (reg_lookup (&regname, RTYPE_GP | RTYPE_NUM, &reg))
    regnum = reg;

  return regnum;
}

void
riscv_elf_final_processing (void)
{
  struct riscv_subset* s;

  unsigned int Xlen = 0;
  for (s = riscv_subsets; s != NULL; s = s->next)
    if (s->name[0] == 'X')
      Xlen += strlen(s->name);

  char extension[Xlen]; 
  extension[0] = 0;
  for (s = riscv_subsets; s != NULL; s = s->next)
    if (s->name[0] == 'X')
      strcat(extension, s->name);

  EF_SET_RISCV_EXT(elf_elfheader (stdoutput)->e_flags,
    riscv_elf_name_to_flag (extension));
}
