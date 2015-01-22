/* tc-riscv.c -- RISC-V assembler
   Copyright 2011-2014 Free Software Foundation, Inc.

   Contributed by Andrew Waterman (waterman@cs.berkeley.edu) at UC Berkeley.
   Based on MIPS target.

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

#include "elf/riscv.h"
#include "opcode/riscv.h"

#include <execinfo.h>
#include <stdint.h>

/* Information about an instruction, including its format, operands
   and fixups.  */
struct riscv_cl_insn
{
  /* The opcode's entry in riscv_opcodes.  */
  const struct riscv_opcode *insn_mo;

  /* The encoded instruction bits.  */
  insn_t insn_opcode;

  /* The frag that contains the instruction.  */
  struct frag *frag;

  /* The offset into FRAG of the first instruction byte.  */
  long where;

  /* The relocs associated with the instruction, if any.  */
  fixS *fixp;
};

bfd_boolean rv64 = TRUE; /* RV64 (true) or RV32 (false) */
#define LOAD_ADDRESS_INSN (rv64 ? "ld" : "lw")
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

struct riscv_set_options
{
  /* Generate position-independent code.  */
  int pic;
  /* Generate RVC code.  */
  int rvc;
};

static struct riscv_set_options riscv_opts =
{
  0,	/* pic */
  0,	/* rvc */
};

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

#define RELAX_BRANCH_ENCODE(uncond, toofar)		\
  ((relax_substateT) 					\
   (0xc0000000						\
    | ((toofar) ? 1 : 0)				\
    | ((uncond) ? 2 : 0)))
#define RELAX_BRANCH_P(i) (((i) & 0xf0000000) == 0xc0000000)
#define RELAX_BRANCH_TOOFAR(i) (((i) & 1) != 0)
#define RELAX_BRANCH_UNCOND(i) (((i) & 2) != 0)

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
  (STRUCT) = (((STRUCT) & ~((insn_t)(MASK) << (SHIFT))) \
	      | ((insn_t)((VALUE) & (MASK)) << (SHIFT)))

/* Extract bits MASK << SHIFT from STRUCT and shift them right
   SHIFT places.  */
#define EXTRACT_BITS(STRUCT, MASK, SHIFT) \
  (((STRUCT) >> (SHIFT)) & (MASK))

/* Change INSN's opcode so that the operand given by FIELD has value VALUE.
   INSN is a riscv_cl_insn structure and VALUE is evaluated exactly once. */
#define INSERT_OPERAND(FIELD, INSN, VALUE) \
  INSERT_BITS ((INSN).insn_opcode, VALUE, OP_MASK_##FIELD, OP_SH_##FIELD)

/* Extract the operand given by FIELD from riscv_cl_insn INSN.  */
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

static char *expr_end;

/* Expressions which appear in instructions.  These are set by
   riscv_ip.  */

static expressionS imm_expr;
static expressionS offset_expr;

/* Relocs associated with imm_expr and offset_expr.  */

static bfd_reloc_code_real_type imm_reloc = BFD_RELOC_UNUSED;
static bfd_reloc_code_real_type offset_reloc = BFD_RELOC_UNUSED;

/* The default target format to use.  */

const char *
riscv_target_format (void)
{
  return rv64 ? "elf64-littleriscv" : "elf32-littleriscv";
}

/* Return the length of instruction INSN.  */

static inline unsigned int
insn_length (const struct riscv_cl_insn *insn)
{
  return riscv_insn_length (insn->insn_opcode);
}

/* Initialise INSN from opcode entry MO.  Leave its position unspecified.  */

static void
create_insn (struct riscv_cl_insn *insn, const struct riscv_opcode *mo)
{
  insn->insn_mo = mo;
  insn->insn_opcode = mo->match;
  insn->frag = NULL;
  insn->where = 0;
  insn->fixp = NULL;
}

/* Install INSN at the location specified by its "frag" and "where" fields.  */

static void
install_insn (const struct riscv_cl_insn *insn)
{
  char *f = insn->frag->fr_literal + insn->where;
  md_number_to_chars (f, insn->insn_opcode, insn_length(insn));
}

/* Move INSN to offset WHERE in FRAG.  Adjust the fixups accordingly
   and install the opcode in the new location.  */

static void
move_insn (struct riscv_cl_insn *insn, fragS *frag, long where)
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
add_fixed_insn (struct riscv_cl_insn *insn)
{
  char *f = frag_more (insn_length (insn));
  move_insn (insn, frag_now, f - frag_now->fr_literal);
}

static void
add_relaxed_insn (struct riscv_cl_insn *insn, int max_chars, int var,
      relax_substateT subtype, symbolS *symbol, offsetT offset)
{
  frag_grow (max_chars);
  move_insn (insn, frag_now, frag_more (0) - frag_now->fr_literal);
  frag_var (rs_machine_dependent, max_chars, var,
      subtype, symbol, offset, NULL);
}

/* Compute the length of a branch sequence, and adjust the
   RELAX_BRANCH_TOOFAR bit accordingly.  If FRAGP is NULL, the
   worst-case length is computed. */
static int
relaxed_branch_length (fragS *fragp, asection *sec, int update)
{
  bfd_boolean toofar = TRUE;

  if (fragp)
    {
      bfd_boolean uncond = RELAX_BRANCH_UNCOND (fragp->fr_subtype);

      if (S_IS_DEFINED (fragp->fr_symbol)
	  && sec == S_GET_SEGMENT (fragp->fr_symbol))
	{
	  offsetT val = S_GET_VALUE (fragp->fr_symbol) + fragp->fr_offset;
	  bfd_vma range;
	  val -= fragp->fr_address + fragp->fr_fix;

	  if (uncond)
	    range = RISCV_JUMP_REACH;
	  else
	    range = RISCV_BRANCH_REACH;
	  toofar = (bfd_vma)(val + range/2) >= range;
	}

      if (update && toofar != RELAX_BRANCH_TOOFAR (fragp->fr_subtype))
	fragp->fr_subtype = RELAX_BRANCH_ENCODE (uncond, toofar);
    }

  return toofar ? 8 : 4;
}

struct regname {
  const char *name;
  unsigned int num;
};

enum reg_class {
  RCLASS_GPR,
  RCLASS_FPR,
  RCLASS_CSR,
  RCLASS_VEC_GPR,
  RCLASS_VEC_FPR,
  RCLASS_MAX
};

static struct hash_control *reg_names_hash = NULL;

#define ENCODE_REG_HASH(cls, n) (void*)(uintptr_t)((n)*RCLASS_MAX + (cls) + 1)
#define DECODE_REG_CLASS(hash) (((uintptr_t)(hash) - 1) % RCLASS_MAX)
#define DECODE_REG_NUM(hash) (((uintptr_t)(hash) - 1) / RCLASS_MAX)

static void
hash_reg_name (enum reg_class class, const char *name, unsigned n)
{
  void *hash = ENCODE_REG_HASH (class, n);
  const char *retval = hash_insert (reg_names_hash, name, hash);
  if (retval != NULL)
    as_fatal (_("internal error: can't hash `%s': %s"), name, retval);
}

static void
hash_reg_names (enum reg_class class, const char * const names[], unsigned n)
{
  unsigned i;
  for (i = 0; i < n; i++)
    hash_reg_name (class, names[i], i);
}

static unsigned int
reg_lookup_internal (const char *s, enum reg_class class)
{
  struct regname *r = (struct regname *) hash_find (reg_names_hash, s);
  if (r == NULL || DECODE_REG_CLASS (r) != class)
    return -1;
  return DECODE_REG_NUM (r);
}

static int
reg_lookup (char **s, enum reg_class class, unsigned int *regnop)
{
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

  /* Look for the register.  Advance to next token if one was recognized.  */
  if ((reg = reg_lookup_internal (*s, class)) >= 0)
    *s = e;

  *e = save_c;
  if (regnop)
    *regnop = reg;
  return reg >= 0;
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

/* For consistency checking, verify that all bits are specified either
   by the match/mask part of the instruction definition, or by the
   operand list.  */
static int
validate_riscv_insn (const struct riscv_opcode *opc)
{
  const char *p = opc->args;
  char c;
  insn_t required_bits, used_bits = opc->mask;

  if ((used_bits & opc->match) != opc->match)
    {
      as_bad (_("internal: bad RISC-V opcode (mask error): %s %s"),
	      opc->name, opc->args);
      return 0;
    }
  required_bits = ((insn_t)1 << (8 * riscv_insn_length (opc->match))) - 1;
  /* Work around for undefined behavior of uint64_t << 64 */
  if(riscv_insn_length (opc->match) == 8)
    required_bits = 0xffffffffffffffff;

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
          as_bad (_("internal: bad RISC-V opcode (unknown extension operand type `#%c'): %s %s"),
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
	as_bad (_("internal: bad RISC-V opcode (unknown operand type `%c'): %s %s"),
		c, opc->name, opc->args);
	return 0;
      }
#undef USE_BITS
  if (used_bits != required_bits)
    {
      as_bad (_("internal: bad RISC-V opcode (bits 0x%lx undefined): %s %s"),
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
	      if (!validate_riscv_insn (&riscv_opcodes[i]))
		as_fatal (_("Broken assembler.  No assembly attempted."));
	    }
	  ++i;
	}
      while ((i < NUMOPCODES) && !strcmp (riscv_opcodes[i].name, name));
    }

  reg_names_hash = hash_new ();
  hash_reg_names (RCLASS_GPR, riscv_gpr_names_numeric, NGPR);
  hash_reg_names (RCLASS_GPR, riscv_gpr_names_abi, NGPR);
  hash_reg_names (RCLASS_FPR, riscv_fpr_names_numeric, NFPR);
  hash_reg_names (RCLASS_FPR, riscv_fpr_names_abi, NFPR);
  hash_reg_names (RCLASS_VEC_GPR, riscv_vec_gpr_names, NVGPR);
  hash_reg_names (RCLASS_VEC_FPR, riscv_vec_fpr_names, NVFPR);

#define DECLARE_CSR(name, num) hash_reg_name (RCLASS_CSR, #name, num);
#include "opcode/riscv-opc.h"
#undef DECLARE_CSR

  /* set the default alignment for the text section (2**2) */
  record_alignment (text_section, 2);
}

/* Output an instruction.  IP is the instruction information.
   ADDRESS_EXPR is an operand of the instruction to be used with
   RELOC_TYPE.  */

static void
append_insn (struct riscv_cl_insn *ip, expressionS *address_expr,
	     bfd_reloc_code_real_type reloc_type)
{
#ifdef OBJ_ELF
  dwarf2_emit_insn (0);
#endif

  gas_assert(reloc_type <= BFD_RELOC_UNUSED);

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
		RISCV_CONST_HIGH_PART (address_expr->X_add_number));
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
			    RELAX_BRANCH_ENCODE (0, 0),
			    address_expr->X_add_symbol,
			    address_expr->X_add_number);
	  return;
	}
      else if (reloc_type < BFD_RELOC_UNUSED)
	{
	  reloc_howto_type *howto;

	  howto = bfd_reloc_type_lookup (stdoutput, reloc_type);
	  if (howto == NULL)
	    as_bad (_("Unsupported RISC-V relocation number %d"), reloc_type);
	 
	  ip->fixp = fix_new_exp (ip->frag, ip->where,
				  bfd_get_reloc_size (howto),
				  address_expr,
				  reloc_type == BFD_RELOC_12_PCREL ||
				  reloc_type == BFD_RELOC_RISCV_CALL ||
				  reloc_type == BFD_RELOC_RISCV_JMP,
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

  add_fixed_insn (ip);

  install_insn (ip);
}

/* Build an instruction created by a macro expansion.  This is passed
   a pointer to the count of instructions created so far, an
   expression, the name of the instruction to build, an operand format
   string, and corresponding arguments.  */

static void
macro_build (expressionS *ep, const char *name, const char *fmt, ...)
{
  const struct riscv_opcode *mo;
  struct riscv_cl_insn insn;
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
  if ((ex->X_op == O_constant || ex->X_op == O_symbol)
      && IS_ZEXT_32BIT_NUM (ex->X_add_number))
    ex->X_add_number = (((ex->X_add_number & 0xffffffff) ^ 0x80000000)
			- 0x80000000);
}

static symbolS *
make_internal_label (void)
{
  return (symbolS *) local_symbol_make (FAKE_LABEL_NAME, now_seg,
					(valueT) frag_now_fix(), frag_now);
}

/* Load an entry from the GOT. */
static void
pcrel_access (int destreg, int tempreg, expressionS *ep,
	      const char* lo_insn, const char* lo_pattern,
	      bfd_reloc_code_real_type hi_reloc,
	      bfd_reloc_code_real_type lo_reloc)
{
  expressionS ep2;
  ep2.X_op = O_symbol;
  ep2.X_add_symbol = make_internal_label ();
  ep2.X_add_number = 0;

  macro_build (ep, "auipc", "d,u", tempreg, hi_reloc);
  macro_build (&ep2, lo_insn, lo_pattern, destreg, tempreg, lo_reloc);
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
check_absolute_expr (struct riscv_cl_insn *ip, expressionS *ex)
{
  if (ex->X_op == O_big)
    as_bad (_("unsupported large constant"));
  else if (ex->X_op != O_constant)
    as_bad (_("Instruction %s requires absolute expression"),
	    ip->insn_mo->name);
  normalize_constant_expr (ex);
}

/* Load an integer constant into a register.  */

static void
load_const (int reg, expressionS *ep)
{
  int shift = RISCV_IMM_BITS;
  expressionS upper = *ep, lower = *ep;
  lower.X_add_number = (int32_t) ep->X_add_number << (32-shift) >> (32-shift);
  upper.X_add_number -= lower.X_add_number;

  gas_assert (ep->X_op == O_constant);

  if (rv64 && !IS_SEXT_32BIT_NUM(ep->X_add_number))
    {
      /* Reduce to a signed 32-bit constant using SLLI and ADDI, which
	 is not optimal but also not so bad.  */
      while (((upper.X_add_number >> shift) & 1) == 0)
	shift++;

      upper.X_add_number = (int64_t) upper.X_add_number >> shift;
      load_const(reg, &upper);

      macro_build (NULL, "slli", "d,s,>", reg, reg, shift);
      if (lower.X_add_number != 0)
	macro_build (&lower, "addi", "d,s,j", reg, reg, BFD_RELOC_RISCV_LO12_I);
    }
  else
    {
      int hi_reg = 0;

      if (upper.X_add_number != 0)
	{
	  macro_build (ep, "lui", "d,u", reg, BFD_RELOC_RISCV_HI20);
	  hi_reg = reg;
	}

      if (lower.X_add_number != 0 || hi_reg == 0)
        macro_build (ep, ADD32_INSN, "d,s,j", reg, hi_reg,
		     BFD_RELOC_RISCV_LO12_I);
    }
}

/* Expand RISC-V assembly macros into one or more instructions. */
static void
macro (struct riscv_cl_insn *ip)
{
  int rd = (ip->insn_opcode >> OP_SH_RD) & OP_MASK_RD;
  int rs1 = (ip->insn_opcode >> OP_SH_RS1) & OP_MASK_RS1;
  int rs2 = (ip->insn_opcode >> OP_SH_RS2) & OP_MASK_RS2;
  int mask = ip->insn_mo->mask;

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
      else if (riscv_opts.pic && mask == M_LA) /* Global PIC symbol */
	pcrel_load (rd, rd, &offset_expr, LOAD_ADDRESS_INSN,
		    BFD_RELOC_RISCV_GOT_HI20, BFD_RELOC_RISCV_PCREL_LO12_I);
      else /* Local PIC symbol, or any non-PIC symbol */
	pcrel_load (rd, rd, &offset_expr, "addi",
		    BFD_RELOC_RISCV_PCREL_HI20, BFD_RELOC_RISCV_PCREL_LO12_I);
      break;

    case M_LA_TLS_GD: 
      pcrel_load (rd, rd, &offset_expr, "addi",
		  BFD_RELOC_RISCV_TLS_GD_HI20, BFD_RELOC_RISCV_PCREL_LO12_I);
      break;

    case M_LA_TLS_IE: 
      pcrel_load (rd, rd, &offset_expr, LOAD_ADDRESS_INSN,
		  BFD_RELOC_RISCV_TLS_GOT_HI20, BFD_RELOC_RISCV_PCREL_LO12_I);
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
      pcrel_access (0, rs1, &offset_expr, "vf", "s,s,q",
		    BFD_RELOC_RISCV_PCREL_HI20, BFD_RELOC_RISCV_PCREL_LO12_S);
      break;

    case M_CALL:
      riscv_call (rd, rs1, &offset_expr, offset_reloc);
      break;

    default:
      as_bad (_("Macro %s not implemented"), ip->insn_mo->name);
      break;
    }
}

static const struct percent_op_match percent_op_utype[] =
{
  {"%tprel_hi", BFD_RELOC_RISCV_TPREL_HI20},
  {"%pcrel_hi", BFD_RELOC_RISCV_PCREL_HI20},
  {"%tls_ie_pcrel_hi", BFD_RELOC_RISCV_TLS_GOT_HI20},
  {"%tls_gd_pcrel_hi", BFD_RELOC_RISCV_TLS_GD_HI20},
  {"%hi", BFD_RELOC_RISCV_HI20},
  {0, 0}
};

static const struct percent_op_match percent_op_itype[] =
{
  {"%lo", BFD_RELOC_RISCV_LO12_I},
  {"%tprel_lo", BFD_RELOC_RISCV_TPREL_LO12_I},
  {"%pcrel_lo", BFD_RELOC_RISCV_PCREL_LO12_I},
  {0, 0}
};

static const struct percent_op_match percent_op_stype[] =
{
  {"%lo", BFD_RELOC_RISCV_LO12_S},
  {"%tprel_lo", BFD_RELOC_RISCV_TPREL_LO12_S},
  {"%pcrel_lo", BFD_RELOC_RISCV_PCREL_LO12_S},
  {0, 0}
};

static const struct percent_op_match percent_op_rtype[] =
{
  {"%tprel_add", BFD_RELOC_RISCV_TPREL_ADD},
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
riscv_ip (char *str, struct riscv_cl_insn *ip)
{
  char *s;
  const char *args;
  char c = 0;
  struct riscv_opcode *insn;
  char *argsStart;
  unsigned int regno;
  char save_c = 0;
  int argnum;
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
                  ok = reg_lookup( &s, RCLASS_VEC_GPR, &regno );
                  if ( !ok )
                    as_bad( _( "Invalid vector register" ) );
                  INSERT_OPERAND( VRD, *ip, regno );
                  continue;
                case 's':
                  ok = reg_lookup( &s, RCLASS_VEC_GPR, &regno );
                  if ( !ok )
                    as_bad( _( "Invalid vector register" ) );
                  INSERT_OPERAND( VRS, *ip, regno );
                  continue;
                case 't':
                  ok = reg_lookup( &s, RCLASS_VEC_GPR, &regno );
                  if ( !ok )
                    as_bad( _( "Invalid vector register" ) );
                  INSERT_OPERAND( VRT, *ip, regno );
                  continue;
                case 'r':
                  ok = reg_lookup( &s, RCLASS_VEC_GPR, &regno );
                  if ( !ok )
                    as_bad( _( "Invalid vector register" ) );
                  INSERT_OPERAND( VRR, *ip, regno );
                  continue;
                case 'D':
                  ok = reg_lookup( &s, RCLASS_VEC_FPR, &regno );
                  if ( !ok )
                    as_bad( _( "Invalid vector register" ) );
                  INSERT_OPERAND( VFD, *ip, regno );
                  continue;
                case 'S':
                  ok = reg_lookup( &s, RCLASS_VEC_FPR, &regno );
                  if ( !ok )
                    as_bad( _( "Invalid vector register" ) );
                  INSERT_OPERAND( VFS, *ip, regno );
                  continue;
                case 'T':
                  ok = reg_lookup( &s, RCLASS_VEC_FPR, &regno );
                  if ( !ok )
                    as_bad( _( "Invalid vector register" ) );
                  INSERT_OPERAND( VFT, *ip, regno );
                  continue;
                case 'R':
                  ok = reg_lookup( &s, RCLASS_VEC_FPR, &regno );
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

	    case '<':		/* shift amount, 0 - 31 */
	      my_getExpression (&imm_expr, s);
	      check_absolute_expr (ip, &imm_expr);
	      if ((unsigned long) imm_expr.X_add_number > 31)
		as_warn (_("Improper shift amount (%lu)"),
			 (unsigned long) imm_expr.X_add_number);
	      INSERT_OPERAND (SHAMTW, *ip, imm_expr.X_add_number);
	      imm_expr.X_op = O_absent;
	      s = expr_end;
	      continue;

	    case '>':		/* shift amount, 0 - (XLEN-1) */
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
	      ok = reg_lookup (&s, RCLASS_CSR, &regno);
	      if (ok)
	        INSERT_OPERAND (CSR, *ip, regno);
	      else
		{
	          my_getExpression (&imm_expr, s);
	          check_absolute_expr (ip, &imm_expr);
		  if ((unsigned long) imm_expr.X_add_number > 0xfff)
	            as_warn(_("Improper CSR address (%lu)"),
	                    (unsigned long) imm_expr.X_add_number);
	          INSERT_OPERAND (CSR, *ip, imm_expr.X_add_number);
		  imm_expr.X_op = O_absent;
		  s = expr_end;
	        }
	      continue;

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
	      ok = reg_lookup (&s, RCLASS_GPR, &regno);
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
	      if (reg_lookup (&s, RCLASS_FPR, &regno))
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
	      normalize_constant_expr (&offset_expr);
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
	      offset_reloc = BFD_RELOC_RISCV_JMP;
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

void
md_assemble (char *str)
{
  struct riscv_cl_insn insn;

  imm_expr.X_op = O_absent;
  offset_expr.X_op = O_absent;
  imm_reloc = BFD_RELOC_UNUSED;
  offset_reloc = BFD_RELOC_UNUSED;

  riscv_ip (str, &insn);

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
    case OPTION_MRVC:
      riscv_opts.rvc = 1;
      break;

    case OPTION_MNO_RVC:
      riscv_opts.rvc = 0;
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
      riscv_opts.pic = FALSE;
      break;

    case OPTION_PIC:
      riscv_opts.pic = TRUE;
      break;

    default:
      return 0;
    }

  return 1;
}

void
riscv_after_parse_args (void)
{
  if (riscv_subsets == NULL)
    riscv_set_arch("RVIMAFDXcustom");
}

void
riscv_init_after_args (void)
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

  /* Remember value for tc_gen_reloc.  */
  fixP->fx_addnumber = *valP;

  switch (fixP->fx_r_type)
    {
    case BFD_RELOC_RISCV_TLS_GOT_HI20:
    case BFD_RELOC_RISCV_TLS_GD_HI20:
    case BFD_RELOC_RISCV_TLS_DTPREL32:
    case BFD_RELOC_RISCV_TLS_DTPREL64:
    case BFD_RELOC_RISCV_TPREL_HI20:
    case BFD_RELOC_RISCV_TPREL_LO12_I:
    case BFD_RELOC_RISCV_TPREL_LO12_S:
    case BFD_RELOC_RISCV_TPREL_ADD:
      S_SET_THREAD_LOCAL (fixP->fx_addsy);
      /* fall through */

    case BFD_RELOC_RISCV_GOT_HI20:
    case BFD_RELOC_RISCV_PCREL_HI20:
    case BFD_RELOC_RISCV_HI20:
    case BFD_RELOC_RISCV_LO12_I:
    case BFD_RELOC_RISCV_LO12_S:
    case BFD_RELOC_RISCV_ADD8:
    case BFD_RELOC_RISCV_ADD16:
    case BFD_RELOC_RISCV_ADD32:
    case BFD_RELOC_RISCV_ADD64:
    case BFD_RELOC_RISCV_SUB8:
    case BFD_RELOC_RISCV_SUB16:
    case BFD_RELOC_RISCV_SUB32:
    case BFD_RELOC_RISCV_SUB64:
      gas_assert (fixP->fx_addsy != NULL);
      /* Nothing needed to do.  The value comes from the reloc entry.  */
      break;

    case BFD_RELOC_64:
    case BFD_RELOC_32:
    case BFD_RELOC_16:
    case BFD_RELOC_8:
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
	  else if (fixP->fx_r_type == BFD_RELOC_32)
	    fixP->fx_r_type = BFD_RELOC_RISCV_ADD32,
	    fixP->fx_next->fx_r_type = BFD_RELOC_RISCV_SUB32;
	  else if (fixP->fx_r_type == BFD_RELOC_16)
	    fixP->fx_r_type = BFD_RELOC_RISCV_ADD16,
	    fixP->fx_next->fx_r_type = BFD_RELOC_RISCV_SUB16;
	  else
	    fixP->fx_r_type = BFD_RELOC_RISCV_ADD8,
	    fixP->fx_next->fx_r_type = BFD_RELOC_RISCV_SUB8;
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
      break;

    case BFD_RELOC_RISCV_JMP:
      if (fixP->fx_addsy)
	{
	  /* Fill in a tentative value to improve objdump readability.  */
	  bfd_vma delta = ENCODE_UJTYPE_IMM (S_GET_VALUE (fixP->fx_addsy) + *valP);
	  bfd_putl32 (bfd_getl32 (buf) | delta, buf);
	}
      break;

    case BFD_RELOC_12_PCREL:
      if (fixP->fx_addsy)
	{
	  /* Fill in a tentative value to improve objdump readability.  */
	  bfd_vma delta = ENCODE_SBTYPE_IMM (S_GET_VALUE (fixP->fx_addsy) + *valP);
	  bfd_putl32 (bfd_getl32 (buf) | delta, buf);
	}
      break;

    case BFD_RELOC_RISCV_PCREL_LO12_S:
    case BFD_RELOC_RISCV_PCREL_LO12_I:
    case BFD_RELOC_RISCV_CALL:
    case BFD_RELOC_RISCV_CALL_PLT:
    case BFD_RELOC_RISCV_ALIGN:
      break;

    default:
      /* We ignore generic BFD relocations we don't know about.  */
      if (bfd_reloc_type_lookup (stdoutput, fixP->fx_r_type) != NULL)
	internalError ();
    }
}

/* This structure is used to hold a stack of .set values.  */

struct riscv_option_stack
{
  struct riscv_option_stack *next;
  struct riscv_set_options options;
};

static struct riscv_option_stack *riscv_opts_stack;

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
    riscv_opts.rvc = 1;
  else if (strcmp (name, "norvc") == 0)
    riscv_opts.rvc = 0;
  else if (strcmp (name, "push") == 0)
    {
      struct riscv_option_stack *s;

      s = (struct riscv_option_stack *) xmalloc (sizeof *s);
      s->next = riscv_opts_stack;
      s->options = riscv_opts;
      riscv_opts_stack = s;
    }
  else if (strcmp (name, "pop") == 0)
    {
      struct riscv_option_stack *s;

      s = riscv_opts_stack;
      if (s == NULL)
	as_bad (_(".option pop with no .option push"));
      else
	{
	  riscv_opts = s->options;
	  riscv_opts_stack = s->next;
	  free (s);
	}
    }
  else
    {
      as_warn (_("Unrecognized .option directive: %s\n"), name);
    }
  *input_line_pointer = ch;
  demand_empty_rest_of_line ();
}

/* Handle the .dtprelword and .dtpreldword pseudo-ops.  They generate
   a 32-bit or 64-bit DTP-relative relocation (BYTES says which) for
   use in DWARF debug information.  */

static void
s_dtprel (int bytes)
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
		? BFD_RELOC_RISCV_TLS_DTPREL64
		: BFD_RELOC_RISCV_TLS_DTPREL32));

  demand_empty_rest_of_line ();
}

/* Handle the .bss pseudo-op.  */

static void
s_bss (int ignore ATTRIBUTE_UNUSED)
{
  subseg_set (bss_section, 0);
  demand_empty_rest_of_line ();
}

/* Align to a given power of two.  */

static void
s_align (int x ATTRIBUTE_UNUSED)
{
  int alignment, fill_value = 0, fill_value_specified = 0;

  alignment = get_absolute_expression ();
  if (alignment < 0 || alignment > 31)
    as_bad (_("unsatisfiable alignment: %d"), alignment);

  if (*input_line_pointer == ',')
    {
      ++input_line_pointer;
      fill_value = get_absolute_expression ();
      fill_value_specified = 1;
    }

  if (!fill_value_specified && subseg_text_p (now_seg) && alignment > 2)
    {
      /* Emit the worst-case NOP string.  The linker will delete any
         unnecessary NOPs.  This allows us to support code alignment
         in spite of linker relaxations.  */
      bfd_vma i, worst_case_nop_bytes = (1L << alignment) - 4;
      char *nops = frag_more (worst_case_nop_bytes);
      for (i = 0; i < worst_case_nop_bytes; i += 4)
	md_number_to_chars (nops + i, RISCV_NOP, 4);

      expressionS ex;
      ex.X_op = O_constant;
      ex.X_add_number = worst_case_nop_bytes;

      fix_new_exp (frag_now, nops - frag_now->fr_literal, 0,
		   &ex, TRUE, BFD_RELOC_RISCV_ALIGN);
    }
  else if (alignment)
    frag_align (alignment, fill_value, 0);

  record_alignment (now_seg, alignment);

  demand_empty_rest_of_line ();
}

int
md_estimate_size_before_relax (fragS *fragp, asection *segtype)
{
  return (fragp->fr_var = relaxed_branch_length (fragp, segtype, FALSE));
}

/* Translate internal representation of relocation info to BFD target
   format.  */

arelent *
tc_gen_reloc (asection *section ATTRIBUTE_UNUSED, fixS *fixp)
{
  arelent *reloc = (arelent *) xmalloc (sizeof (arelent));

  reloc->sym_ptr_ptr = (asymbol **) xmalloc (sizeof (asymbol *));
  *reloc->sym_ptr_ptr = symbol_get_bfdsym (fixp->fx_addsy);
  reloc->address = fixp->fx_frag->fr_address + fixp->fx_where;

  if (fixp->fx_pcrel)
    /* At this point, fx_addnumber is "symbol offset - pcrel address".
       Relocations want only the symbol offset.  */
    reloc->addend = fixp->fx_addnumber + reloc->address;
  else
    reloc->addend = fixp->fx_addnumber;

  reloc->howto = bfd_reloc_type_lookup (stdoutput, fixp->fx_r_type);
  if (reloc->howto == NULL)
    {
      if ((fixp->fx_r_type == BFD_RELOC_16 || fixp->fx_r_type == BFD_RELOC_8)
          && fixp->fx_addsy != NULL && fixp->fx_subsy != NULL)
	{
          /* We don't have R_RISCV_8/16, but for this special case,
	     we can use R_RISCV_ADD8/16 with R_RISCV_SUB8/16.  */
	  return reloc;
	}

      as_bad_where (fixp->fx_file, fixp->fx_line,
		    _("cannot represent %s relocation in object file"),
		    bfd_get_reloc_code_name (fixp->fx_r_type));
      return NULL;
    }

  return reloc;
}

int
riscv_relax_frag (asection *sec, fragS *fragp, long stretch ATTRIBUTE_UNUSED)
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
md_convert_frag_branch (fragS *fragp)
{
  bfd_byte *buf;
  insn_t insn;
  expressionS exp;
  fixS *fixp;

  buf = (bfd_byte *)fragp->fr_literal + fragp->fr_fix;

  exp.X_op = O_symbol;
  exp.X_add_symbol = fragp->fr_symbol;
  exp.X_add_number = fragp->fr_offset;

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
			  4, &exp, FALSE, BFD_RELOC_RISCV_JMP);
      md_number_to_chars ((char *) buf, MATCH_JAL, 4);
      buf += 4;
    }
  else
    {
      fixp = fix_new_exp (fragp, buf - (bfd_byte *)fragp->fr_literal,
			  4, &exp, FALSE, BFD_RELOC_12_PCREL);
      buf += 4;
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
md_convert_frag (bfd *abfd ATTRIBUTE_UNUSED, segT asec ATTRIBUTE_UNUSED,
		 fragS *fragp)
{
  gas_assert (RELAX_BRANCH_P (fragp->fr_subtype));
  md_convert_frag_branch (fragp);
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

/* Standard calling conventions leave the CFA at SP on entry.  */
void
riscv_cfi_frame_initial_instructions (void)
{
  cfi_add_CFA_def_cfa_register (X_SP);
}

int
tc_riscv_regname_to_dw2regnum (char *regname)
{
  int reg;

  if ((reg = reg_lookup_internal (regname, RCLASS_GPR)) >= 0)
    return reg;

  if ((reg = reg_lookup_internal (regname, RCLASS_FPR)) >= 0)
    return reg + 32;

  as_bad (_("unknown register `%s'"), regname);
  return -1;
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

/* Pseudo-op table.  */

static const pseudo_typeS riscv_pseudo_table[] =
{
  /* RISC-V-specific pseudo-ops.  */
  {"option", s_riscv_option, 0},
  {"half", cons, 2},
  {"word", cons, 4},
  {"dword", cons, 8},
  {"dtprelword", s_dtprel, 4},
  {"dtpreldword", s_dtprel, 8},
  {"bss", s_bss, 0},
  {"align", s_align, 0},

  /* leb128 doesn't work with relaxation; disallow it */
  {"uleb128", s_err, 0},
  {"sleb128", s_err, 0},

  { NULL, NULL, 0 },
};

void
riscv_pop_insert (void)
{
  extern void pop_insert (const pseudo_typeS *);

  pop_insert (riscv_pseudo_table);
}
