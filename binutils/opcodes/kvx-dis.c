/* kvx-dis.c -- Kalray MPPA generic disassembler.
   Copyright (C) 2009-2024 Free Software Foundation, Inc.
   Contributed by Kalray SA.

   This file is part of the GNU opcodes library.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   It is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING3. If not,
   see <http://www.gnu.org/licenses/>.  */

#define STATIC_TABLE
#define DEFINE_TABLE

#include "sysdep.h"
#include "disassemble.h"
#include "libiberty.h"
#include "opintl.h"
#include <assert.h>
#include "elf-bfd.h"
#include "kvx-dis.h"

#include "elf/kvx.h"
#include "opcode/kvx.h"

/* Steering values for the kvx VLIW architecture.  */

typedef enum
{
  Steering_BCU,
  Steering_LSU,
  Steering_MAU,
  Steering_ALU,
  Steering__
} enum_Steering;
typedef uint8_t Steering;

/* BundleIssue enumeration.  */

typedef enum
{
  BundleIssue_BCU,
  BundleIssue_TCA,
  BundleIssue_ALU0,
  BundleIssue_ALU1,
  BundleIssue_MAU,
  BundleIssue_LSU,
  BundleIssue__,
} enum_BundleIssue;
typedef uint8_t BundleIssue;

/* An IMMX syllable is associated with the BundleIssue Extension_BundleIssue[extension].  */
static const BundleIssue Extension_BundleIssue[] = {
  BundleIssue_ALU0,
  BundleIssue_ALU1,
  BundleIssue_MAU,
  BundleIssue_LSU
};

static inline int
kvx_steering (uint32_t x)
{
  return (((x) & 0x60000000) >> 29);
}

static inline int
kvx_extension (uint32_t x)
{
  return (((x) & 0x18000000) >> 27);
}

static inline int
kvx_has_parallel_bit (uint32_t x)
{
  return (((x) & 0x80000000) == 0x80000000);
}

static inline int
kvx_is_tca_opcode (uint32_t x)
{
  unsigned major = ((x) >> 24) & 0x1F;
  return (major > 1) && (major < 8);
}

static inline int
kvx_is_nop_opcode (uint32_t x)
{
  return ((x) << 1) == 0xFFFFFFFE;
}

/* A raw instruction.  */

struct insn_s
{
  uint32_t syllables[KVXMAXSYLLABLES];
  int len;
};
typedef struct insn_s insn_t;


static uint32_t bundle_words[KVXMAXBUNDLEWORDS];

static insn_t bundle_insn[KVXMAXBUNDLEISSUE];

/* A re-interpreted instruction.  */

struct instr_s
{
  int valid;
  int opcode;
  int immx[2];
  int immx_valid[2];
  int immx_count;
  int nb_syllables;
};

/* Option for "pretty printing", ie, not the usual little endian objdump output.  */
static int opt_pretty = 0;
/* Option for not emiting a new line between all bundles.  */
static int opt_compact_assembly = 0;

void
parse_kvx_dis_option (const char *option)
{
  /* Try to match options that are simple flags.  */
  if (startswith (option, "pretty"))
    {
      opt_pretty = 1;
      return;
    }

  if (startswith (option, "compact-assembly"))
    {
      opt_compact_assembly = 1;
      return;
    }

  if (startswith (option, "no-compact-assembly"))
    {
      opt_compact_assembly = 0;
      return;
    }

  /* Invalid option.  */
  opcodes_error_handler (_("unrecognised disassembler option: %s"), option);
}

static void
parse_kvx_dis_options (const char *options)
{
  const char *option_end;

  if (options == NULL)
    return;

  while (*options != '\0')
    {
      /* Skip empty options.  */
      if (*options == ',')
	{
	  options++;
	  continue;
	}

      /* We know that *options is neither NUL or a comma.  */
      option_end = options + 1;
      while (*option_end != ',' && *option_end != '\0')
	option_end++;

      parse_kvx_dis_option (options);

      /* Go on to the next one.  If option_end points to a comma, it
         will be skipped above.  */
      options = option_end;
    }
}

struct kvx_dis_env
{
  int kvx_arch_size;
  struct kvxopc *opc_table;
  struct kvx_Register *kvx_registers;
  const char ***kvx_modifiers;
  int *kvx_dec_registers;
  int *kvx_regfiles;
  unsigned int kvx_max_dec_registers;
  int initialized_p;
};

static struct kvx_dis_env env = {
  .kvx_arch_size = 0,
  .opc_table = NULL,
  .kvx_registers = NULL,
  .kvx_modifiers = NULL,
  .kvx_dec_registers = NULL,
  .kvx_regfiles = NULL,
  .initialized_p = 0,
  .kvx_max_dec_registers = 0
};

static void
kvx_dis_init (struct disassemble_info *info)
{
  env.kvx_arch_size = 32;
  switch (info->mach)
    {
    case bfd_mach_kv3_1_64:
      env.kvx_arch_size = 64;
      /* fallthrough */
    case bfd_mach_kv3_1_usr:
    case bfd_mach_kv3_1:
    default:
      env.opc_table = kvx_kv3_v1_optab;
      env.kvx_regfiles = kvx_kv3_v1_regfiles;
      env.kvx_registers = kvx_kv3_v1_registers;
      env.kvx_modifiers = kvx_kv3_v1_modifiers;
      env.kvx_dec_registers = kvx_kv3_v1_dec_registers;
      break;
    case bfd_mach_kv3_2_64:
      env.kvx_arch_size = 64;
      /* fallthrough */
    case bfd_mach_kv3_2_usr:
    case bfd_mach_kv3_2:
      env.opc_table = kvx_kv3_v2_optab;
      env.kvx_regfiles = kvx_kv3_v2_regfiles;
      env.kvx_registers = kvx_kv3_v2_registers;
      env.kvx_modifiers = kvx_kv3_v2_modifiers;
      env.kvx_dec_registers = kvx_kv3_v2_dec_registers;
      break;
    case bfd_mach_kv4_1_64:
      env.kvx_arch_size = 64;
      /* fallthrough */
    case bfd_mach_kv4_1_usr:
    case bfd_mach_kv4_1:
      env.opc_table = kvx_kv4_v1_optab;
      env.kvx_regfiles = kvx_kv4_v1_regfiles;
      env.kvx_registers = kvx_kv4_v1_registers;
      env.kvx_modifiers = kvx_kv4_v1_modifiers;
      env.kvx_dec_registers = kvx_kv4_v1_dec_registers;
      break;
    }

  env.kvx_max_dec_registers = env.kvx_regfiles[KVX_REGFILE_DEC_REGISTERS];

  if (info->disassembler_options)
    parse_kvx_dis_options (info->disassembler_options);

  env.initialized_p = 1;
}

static bool
kvx_reassemble_bundle (int wordcount, int *_insncount)
{

  /* Debugging flag.  */
  int debug = 0;

  /* Available resources.  */
  int bcu_taken = 0;
  int tca_taken = 0;
  int alu0_taken = 0;
  int alu1_taken = 0;
  int mau_taken = 0;
  int lsu_taken = 0;

  if (debug)
    fprintf (stderr, "kvx_reassemble_bundle: wordcount = %d\n", wordcount);

  if (wordcount > KVXMAXBUNDLEWORDS)
    {
      if (debug)
	fprintf (stderr, "bundle exceeds maximum size\n");
      return false;
    }

  struct instr_s instr[KVXMAXBUNDLEISSUE];
  memset (instr, 0, sizeof (instr));
  assert (KVXMAXBUNDLEISSUE >= BundleIssue__);

  int i;
  unsigned int j;

  for (i = 0; i < wordcount; i++)
    {
      uint32_t syllable = bundle_words[i];
      switch (kvx_steering (syllable))
	{
	case Steering_BCU:
	  /* BCU or TCA instruction.  */
	  if (i == 0)
	    {
	      if (kvx_is_tca_opcode (syllable))
		{
		  if (tca_taken)
		    {
		      if (debug)
			fprintf (stderr, "Too many TCA instructions");
		      return false;
		    }
		  if (debug)
		    fprintf (stderr,
			     "Syllable 0: Set valid on TCA for instr %d with 0x%x\n",
			     BundleIssue_TCA, syllable);
		  instr[BundleIssue_TCA].valid = 1;
		  instr[BundleIssue_TCA].opcode = syllable;
		  instr[BundleIssue_TCA].nb_syllables = 1;
		  tca_taken = 1;
		}
	      else
		{
		  if (debug)
		    fprintf (stderr,
			     "Syllable 0: Set valid on BCU for instr %d with 0x%x\n",
			     BundleIssue_BCU, syllable);

		  instr[BundleIssue_BCU].valid = 1;
		  instr[BundleIssue_BCU].opcode = syllable;
		  instr[BundleIssue_BCU].nb_syllables = 1;
		  bcu_taken = 1;
		}
	    }
	  else
	    {
	      if (i == 1 && bcu_taken && kvx_is_tca_opcode (syllable))
		{
		  if (tca_taken)
		    {
		      if (debug)
			fprintf (stderr, "Too many TCA instructions");
		      return false;
		    }
		  if (debug)
		    fprintf (stderr,
			     "Syllable 0: Set valid on TCA for instr %d with 0x%x\n",
			     BundleIssue_TCA, syllable);
		  instr[BundleIssue_TCA].valid = 1;
		  instr[BundleIssue_TCA].opcode = syllable;
		  instr[BundleIssue_TCA].nb_syllables = 1;
		  tca_taken = 1;
		}
	      else
		{
		  /* Not first syllable in bundle, IMMX.  */
		  struct instr_s *instr_p =
		    &(instr[Extension_BundleIssue[kvx_extension (syllable)]]);
		  int immx_count = instr_p->immx_count;
		  if (immx_count > 1)
		    {
		      if (debug)
			fprintf (stderr, "Too many IMMX syllables");
		      return false;
		    }
		  instr_p->immx[immx_count] = syllable;
		  instr_p->immx_valid[immx_count] = 1;
		  instr_p->nb_syllables++;
		  if (debug)
		    fprintf (stderr,
			     "Set IMMX[%d] on instr %d for extension %d @ %d\n",
			     immx_count,
			     Extension_BundleIssue[kvx_extension (syllable)],
			     kvx_extension (syllable), i);
		  instr_p->immx_count = immx_count + 1;
		}
	    }
	  break;

	case Steering_ALU:
	  if (alu0_taken == 0)
	    {
	      if (debug)
		fprintf (stderr, "Set valid on ALU0 for instr %d with 0x%x\n",
			 BundleIssue_ALU0, syllable);
	      instr[BundleIssue_ALU0].valid = 1;
	      instr[BundleIssue_ALU0].opcode = syllable;
	      instr[BundleIssue_ALU0].nb_syllables = 1;
	      alu0_taken = 1;
	    }
	  else if (alu1_taken == 0)
	    {
	      if (debug)
		fprintf (stderr, "Set valid on ALU1 for instr %d with 0x%x\n",
			 BundleIssue_ALU1, syllable);
	      instr[BundleIssue_ALU1].valid = 1;
	      instr[BundleIssue_ALU1].opcode = syllable;
	      instr[BundleIssue_ALU1].nb_syllables = 1;
	      alu1_taken = 1;
	    }
	  else if (mau_taken == 0)
	    {
	      if (debug)
		fprintf (stderr,
			 "Set valid on MAU (ALU) for instr %d with 0x%x\n",
			 BundleIssue_MAU, syllable);
	      instr[BundleIssue_MAU].valid = 1;
	      instr[BundleIssue_MAU].opcode = syllable;
	      instr[BundleIssue_MAU].nb_syllables = 1;
	      mau_taken = 1;
	    }
	  else if (lsu_taken == 0)
	    {
	      if (debug)
		fprintf (stderr,
			 "Set valid on LSU (ALU) for instr %d with 0x%x\n",
			 BundleIssue_LSU, syllable);
	      instr[BundleIssue_LSU].valid = 1;
	      instr[BundleIssue_LSU].opcode = syllable;
	      instr[BundleIssue_LSU].nb_syllables = 1;
	      lsu_taken = 1;
	    }
	  else if (kvx_is_nop_opcode (syllable))
	    {
	      if (debug)
		fprintf (stderr, "Ignoring NOP (ALU) syllable\n");
	    }
	  else
	    {
	      if (debug)
		fprintf (stderr, "Too many ALU instructions");
	      return false;
	    }
	  break;

	case Steering_MAU:
	  if (mau_taken == 1)
	    {
	      if (debug)
		fprintf (stderr, "Too many MAU instructions");
	      return false;
	    }
	  else
	    {
	      if (debug)
		fprintf (stderr, "Set valid on MAU for instr %d with 0x%x\n",
			 BundleIssue_MAU, syllable);
	      instr[BundleIssue_MAU].valid = 1;
	      instr[BundleIssue_MAU].opcode = syllable;
	      instr[BundleIssue_MAU].nb_syllables = 1;
	      mau_taken = 1;
	    }
	  break;

	case Steering_LSU:
	  if (lsu_taken == 1)
	    {
	      if (debug)
		fprintf (stderr, "Too many LSU instructions");
	      return false;
	    }
	  else
	    {
	      if (debug)
		fprintf (stderr, "Set valid on LSU for instr %d with 0x%x\n",
			 BundleIssue_LSU, syllable);
	      instr[BundleIssue_LSU].valid = 1;
	      instr[BundleIssue_LSU].opcode = syllable;
	      instr[BundleIssue_LSU].nb_syllables = 1;
	      lsu_taken = 1;
	    }
	}
      if (debug)
	fprintf (stderr, "Continue %d < %d?\n", i, wordcount);
    }

  /* Fill bundle_insn and count read syllables.  */
  int instr_idx = 0;
  for (i = 0; i < KVXMAXBUNDLEISSUE; i++)
    {
      if (instr[i].valid == 1)
	{
	  int syllable_idx = 0;

	  /* First copy opcode.  */
	  bundle_insn[instr_idx].syllables[syllable_idx++] = instr[i].opcode;
	  bundle_insn[instr_idx].len = 1;

	  for (j = 0; j < 2; j++)
	    {
	      if (instr[i].immx_valid[j])
		{
		  if (debug)
		    fprintf (stderr, "Instr %d valid immx[%d] is valid\n", i,
			     j);
		  bundle_insn[instr_idx].syllables[syllable_idx++] =
		    instr[i].immx[j];
		  bundle_insn[instr_idx].len++;
		}
	    }

	  if (debug)
	    fprintf (stderr,
		     "Instr %d valid, copying in bundle_insn (%d syllables <-> %d)\n",
		     i, bundle_insn[instr_idx].len, instr[i].nb_syllables);
	  instr_idx++;
	}
    }

  if (debug)
    fprintf (stderr, "End => %d instructions\n", instr_idx);

  *_insncount = instr_idx;
  return true;
}

struct decoded_insn
{
  /* The entry in the opc_table. */
  struct kvxopc *opc;
  /* The number of operands.  */
  int nb_ops;
  /* The content of an operands.  */
  struct
  {
    enum
    {
      CAT_REGISTER,
      CAT_MODIFIER,
      CAT_IMMEDIATE,
    } type;
    /* The value of the operands.  */
    uint64_t val;
    /* If it is an immediate, its sign.  */
    int sign;
    /* If it is an immediate, is it pc relative.  */
    int pcrel;
    /* The width of the operand.  */
    int width;
    /* If it is a modifier, the modifier category.
       An index in the modifier table.  */
    int mod_idx;
  } operands[KVXMAXOPERANDS];
};

static int
decode_insn (bfd_vma memaddr, insn_t * insn, struct decoded_insn *res)
{

  int found = 0;
  int idx = 0;
  for (struct kvxopc * op = env.opc_table;
       op->as_op && (((char) op->as_op[0]) != 0); op++)
    {
      /* Find the format of this insn.  */
      int opcode_match = 1;

      if (op->wordcount != insn->len)
	continue;

      for (int i = 0; i < op->wordcount; i++)
	if ((op->codewords[i].mask & insn->syllables[i]) !=
	    op->codewords[i].opcode)
	  opcode_match = 0;

      int encoding_space_flags = env.kvx_arch_size == 32
	? kvxOPCODE_FLAG_MODE32 : kvxOPCODE_FLAG_MODE64;

      for (int i = 0; i < op->wordcount; i++)
	if (!(op->codewords[i].flags & encoding_space_flags))
	  opcode_match = 0;

      if (opcode_match)
	{
	  res->opc = op;

	  for (int i = 0; op->format[i]; i++)
	    {
	      struct kvx_bitfield *bf = op->format[i]->bfield;
	      int bf_nb = op->format[i]->bitfields;
	      int width = op->format[i]->width;
	      int type = op->format[i]->type;
	      const char *type_name = op->format[i]->tname;
	      int flags = op->format[i]->flags;
	      int shift = op->format[i]->shift;
	      int bias = op->format[i]->bias;
	      uint64_t value = 0;

	      for (int bf_idx = 0; bf_idx < bf_nb; bf_idx++)
		{
		  int insn_idx = (int) bf[bf_idx].to_offset / 32;
		  int to_offset = bf[bf_idx].to_offset % 32;
		  uint64_t encoded_value =
		    insn->syllables[insn_idx] >> to_offset;
		  encoded_value &= (1LL << bf[bf_idx].size) - 1;
		  value |= encoded_value << bf[bf_idx].from_offset;
		}
	      if (flags & kvxSIGNED)
		{
		  uint64_t signbit = 1LL << (width - 1);
		  value = (value ^ signbit) - signbit;
		}
	      value = (value << shift) + bias;

#define KVX_PRINT_REG(regfile,value) \
    if(env.kvx_regfiles[regfile]+value < env.kvx_max_dec_registers) { \
        res->operands[idx].val = env.kvx_dec_registers[env.kvx_regfiles[regfile]+value]; \
        res->operands[idx].type = CAT_REGISTER; \
	idx++; \
    } else { \
        res->operands[idx].val = ~0; \
        res->operands[idx].type = CAT_REGISTER; \
	idx++; \
    }

	      if (env.opc_table == kvx_kv3_v1_optab)
		{
		  switch (type)
		    {
		    case RegClass_kv3_v1_singleReg:
		      KVX_PRINT_REG (KVX_REGFILE_DEC_GPR, value)
		      break;
		    case RegClass_kv3_v1_pairedReg:
		      KVX_PRINT_REG (KVX_REGFILE_DEC_PGR, value)
		      break;
		    case RegClass_kv3_v1_quadReg:
		      KVX_PRINT_REG (KVX_REGFILE_DEC_QGR, value)
		      break;
		    case RegClass_kv3_v1_systemReg:
		    case RegClass_kv3_v1_aloneReg:
		    case RegClass_kv3_v1_onlyraReg:
		    case RegClass_kv3_v1_onlygetReg:
		    case RegClass_kv3_v1_onlysetReg:
		    case RegClass_kv3_v1_onlyfxReg:
		      KVX_PRINT_REG (KVX_REGFILE_DEC_SFR, value)
		      break;
		    case RegClass_kv3_v1_coproReg0M4:
		    case RegClass_kv3_v1_coproReg1M4:
		    case RegClass_kv3_v1_coproReg2M4:
		    case RegClass_kv3_v1_coproReg3M4:
		      KVX_PRINT_REG (KVX_REGFILE_DEC_XCR, value)
		      break;
		    case RegClass_kv3_v1_blockRegE:
		    case RegClass_kv3_v1_blockRegO:
		    case RegClass_kv3_v1_blockReg0M4:
		    case RegClass_kv3_v1_blockReg1M4:
		    case RegClass_kv3_v1_blockReg2M4:
		    case RegClass_kv3_v1_blockReg3M4:
		      KVX_PRINT_REG (KVX_REGFILE_DEC_XBR, value)
		      break;
		    case RegClass_kv3_v1_vectorReg:
		    case RegClass_kv3_v1_vectorRegE:
		    case RegClass_kv3_v1_vectorRegO:
		      KVX_PRINT_REG (KVX_REGFILE_DEC_XVR, value)
		      break;
		    case RegClass_kv3_v1_tileReg:
		      KVX_PRINT_REG (KVX_REGFILE_DEC_XTR, value)
		      break;
		    case RegClass_kv3_v1_matrixReg:
		      KVX_PRINT_REG (KVX_REGFILE_DEC_XMR, value)
		      break;
		    case Immediate_kv3_v1_sysnumber:
		    case Immediate_kv3_v1_signed10:
		    case Immediate_kv3_v1_signed16:
		    case Immediate_kv3_v1_signed27:
		    case Immediate_kv3_v1_wrapped32:
		    case Immediate_kv3_v1_signed37:
		    case Immediate_kv3_v1_signed43:
		    case Immediate_kv3_v1_signed54:
		    case Immediate_kv3_v1_wrapped64:
		    case Immediate_kv3_v1_unsigned6:
		      res->operands[idx].val = value;
		      res->operands[idx].sign = flags & kvxSIGNED;
		      res->operands[idx].width = width;
		      res->operands[idx].type = CAT_IMMEDIATE;
		      res->operands[idx].pcrel = 0;
		      idx++;
		      break;
		    case Immediate_kv3_v1_pcrel17:
		    case Immediate_kv3_v1_pcrel27:
		      res->operands[idx].val = value + memaddr;
		      res->operands[idx].sign = flags & kvxSIGNED;
		      res->operands[idx].width = width;
		      res->operands[idx].type = CAT_IMMEDIATE;
		      res->operands[idx].pcrel = 1;
		      idx++;
		      break;
		    case Modifier_kv3_v1_column:
		    case Modifier_kv3_v1_comparison:
		    case Modifier_kv3_v1_doscale:
		    case Modifier_kv3_v1_exunum:
		    case Modifier_kv3_v1_floatcomp:
		    case Modifier_kv3_v1_qindex:
		    case Modifier_kv3_v1_rectify:
		    case Modifier_kv3_v1_rounding:
		    case Modifier_kv3_v1_roundint:
		    case Modifier_kv3_v1_saturate:
		    case Modifier_kv3_v1_scalarcond:
		    case Modifier_kv3_v1_silent:
		    case Modifier_kv3_v1_simplecond:
		    case Modifier_kv3_v1_speculate:
		    case Modifier_kv3_v1_splat32:
		    case Modifier_kv3_v1_variant:
		      {
			int sz = 0;
			int mod_idx = type - Modifier_kv3_v1_column;
			for (sz = 0; env.kvx_modifiers[mod_idx][sz]; ++sz);
			const char *mod = value < (unsigned) sz
			  ? env.kvx_modifiers[mod_idx][value] : NULL;
			if (!mod) goto retry;
			res->operands[idx].val = value;
			res->operands[idx].type = CAT_MODIFIER;
			res->operands[idx].mod_idx = mod_idx;
			idx++;
		      }
		      break;
		    default:
		      fprintf (stderr, "error: unexpected operand type (%s)\n",
			       type_name);
		      exit (-1);
		    };
		}
	      else if (env.opc_table == kvx_kv3_v2_optab)
		{
		  switch (type)
		    {
		    case RegClass_kv3_v2_singleReg:
		      KVX_PRINT_REG (KVX_REGFILE_DEC_GPR, value)
		      break;
		    case RegClass_kv3_v2_pairedReg:
		      KVX_PRINT_REG (KVX_REGFILE_DEC_PGR, value)
		      break;
		    case RegClass_kv3_v2_quadReg:
		      KVX_PRINT_REG (KVX_REGFILE_DEC_QGR, value)
		      break;
		    case RegClass_kv3_v2_systemReg:
		    case RegClass_kv3_v2_aloneReg:
		    case RegClass_kv3_v2_onlyraReg:
		    case RegClass_kv3_v2_onlygetReg:
		    case RegClass_kv3_v2_onlysetReg:
		    case RegClass_kv3_v2_onlyfxReg:
		      KVX_PRINT_REG (KVX_REGFILE_DEC_SFR, value)
		      break;
		    case RegClass_kv3_v2_coproReg:
		    case RegClass_kv3_v2_coproReg0M4:
		    case RegClass_kv3_v2_coproReg1M4:
		    case RegClass_kv3_v2_coproReg2M4:
		    case RegClass_kv3_v2_coproReg3M4:
		      KVX_PRINT_REG (KVX_REGFILE_DEC_XCR, value)
		      break;
		    case RegClass_kv3_v2_blockReg:
		    case RegClass_kv3_v2_blockRegE:
		    case RegClass_kv3_v2_blockRegO:
		      KVX_PRINT_REG (KVX_REGFILE_DEC_XBR, value)
		      break;
		    case RegClass_kv3_v2_vectorReg:
		      KVX_PRINT_REG (KVX_REGFILE_DEC_XVR, value)
		      break;
		    case RegClass_kv3_v2_tileReg:
		      KVX_PRINT_REG (KVX_REGFILE_DEC_XTR, value)
		      break;
		    case RegClass_kv3_v2_matrixReg:
		      KVX_PRINT_REG (KVX_REGFILE_DEC_XMR, value)
		      break;
		    case RegClass_kv3_v2_buffer2Reg:
		      KVX_PRINT_REG (KVX_REGFILE_DEC_X2R, value)
		      break;
		    case RegClass_kv3_v2_buffer4Reg:
		      KVX_PRINT_REG (KVX_REGFILE_DEC_X4R, value)
		      break;
		    case RegClass_kv3_v2_buffer8Reg:
		      KVX_PRINT_REG (KVX_REGFILE_DEC_X8R, value)
		      break;
		    case RegClass_kv3_v2_buffer16Reg:
		      KVX_PRINT_REG (KVX_REGFILE_DEC_X16R, value)
		      break;
		    case RegClass_kv3_v2_buffer32Reg:
		      KVX_PRINT_REG (KVX_REGFILE_DEC_X32R, value)
		      break;
		    case RegClass_kv3_v2_buffer64Reg:
		      KVX_PRINT_REG (KVX_REGFILE_DEC_X64R, value)
		      break;
		    case Immediate_kv3_v2_brknumber:
		    case Immediate_kv3_v2_sysnumber:
		    case Immediate_kv3_v2_signed10:
		    case Immediate_kv3_v2_signed16:
		    case Immediate_kv3_v2_signed27:
		    case Immediate_kv3_v2_wrapped32:
		    case Immediate_kv3_v2_signed37:
		    case Immediate_kv3_v2_signed43:
		    case Immediate_kv3_v2_signed54:
		    case Immediate_kv3_v2_wrapped64:
		    case Immediate_kv3_v2_unsigned6:
		      res->operands[idx].val = value;
		      res->operands[idx].sign = flags & kvxSIGNED;
		      res->operands[idx].width = width;
		      res->operands[idx].type = CAT_IMMEDIATE;
		      res->operands[idx].pcrel = 0;
		      idx++;
		      break;
		    case Immediate_kv3_v2_pcrel27:
		    case Immediate_kv3_v2_pcrel17:
		      res->operands[idx].val = value + memaddr;
		      res->operands[idx].sign = flags & kvxSIGNED;
		      res->operands[idx].width = width;
		      res->operands[idx].type = CAT_IMMEDIATE;
		      res->operands[idx].pcrel = 1;
		      idx++;
		      break;
		    case Modifier_kv3_v2_accesses:
		    case Modifier_kv3_v2_boolcas:
		    case Modifier_kv3_v2_cachelev:
		    case Modifier_kv3_v2_channel:
		    case Modifier_kv3_v2_coherency:
		    case Modifier_kv3_v2_comparison:
		    case Modifier_kv3_v2_conjugate:
		    case Modifier_kv3_v2_doscale:
		    case Modifier_kv3_v2_exunum:
		    case Modifier_kv3_v2_floatcomp:
		    case Modifier_kv3_v2_hindex:
		    case Modifier_kv3_v2_lsomask:
		    case Modifier_kv3_v2_lsumask:
		    case Modifier_kv3_v2_lsupack:
		    case Modifier_kv3_v2_qindex:
		    case Modifier_kv3_v2_rounding:
		    case Modifier_kv3_v2_scalarcond:
		    case Modifier_kv3_v2_shuffleV:
		    case Modifier_kv3_v2_shuffleX:
		    case Modifier_kv3_v2_silent:
		    case Modifier_kv3_v2_simplecond:
		    case Modifier_kv3_v2_speculate:
		    case Modifier_kv3_v2_splat32:
		    case Modifier_kv3_v2_transpose:
		    case Modifier_kv3_v2_variant:
		      {
			int sz = 0;
			int mod_idx = type - Modifier_kv3_v2_accesses;
			for (sz = 0; env.kvx_modifiers[mod_idx][sz];
			     ++sz);
			const char *mod = value < (unsigned) sz
			  ? env.kvx_modifiers[mod_idx][value] : NULL;
			if (!mod) goto retry;
			res->operands[idx].val = value;
			res->operands[idx].type = CAT_MODIFIER;
			res->operands[idx].mod_idx = mod_idx;
			idx++;
		      };
		      break;
		    default:
		      fprintf (stderr,
			       "error: unexpected operand type (%s)\n",
			       type_name);
		      exit (-1);
		    };
		}
	      else if (env.opc_table == kvx_kv4_v1_optab)
		{
		  switch (type)
		    {

		    case RegClass_kv4_v1_singleReg:
		      KVX_PRINT_REG (KVX_REGFILE_DEC_GPR, value)
		      break;
		    case RegClass_kv4_v1_pairedReg:
		      KVX_PRINT_REG (KVX_REGFILE_DEC_PGR, value)
		      break;
		    case RegClass_kv4_v1_quadReg:
		      KVX_PRINT_REG (KVX_REGFILE_DEC_QGR, value)
		      break;
		    case RegClass_kv4_v1_systemReg:
		    case RegClass_kv4_v1_aloneReg:
		    case RegClass_kv4_v1_onlyraReg:
		    case RegClass_kv4_v1_onlygetReg:
		    case RegClass_kv4_v1_onlysetReg:
		    case RegClass_kv4_v1_onlyfxReg:
		      KVX_PRINT_REG (KVX_REGFILE_DEC_SFR, value)
		      break;
		    case RegClass_kv4_v1_coproReg:
		    case RegClass_kv4_v1_coproReg0M4:
		    case RegClass_kv4_v1_coproReg1M4:
		    case RegClass_kv4_v1_coproReg2M4:
		    case RegClass_kv4_v1_coproReg3M4:
		      KVX_PRINT_REG (KVX_REGFILE_DEC_XCR, value)
		      break;
		    case RegClass_kv4_v1_blockReg:
		    case RegClass_kv4_v1_blockRegE:
		    case RegClass_kv4_v1_blockRegO:
		      KVX_PRINT_REG (KVX_REGFILE_DEC_XBR, value)
		      break;
		    case RegClass_kv4_v1_vectorReg:
		      KVX_PRINT_REG (KVX_REGFILE_DEC_XVR, value)
		      break;
		    case RegClass_kv4_v1_tileReg:
		      KVX_PRINT_REG (KVX_REGFILE_DEC_XTR, value)
		      break;
		    case RegClass_kv4_v1_matrixReg:
		      KVX_PRINT_REG (KVX_REGFILE_DEC_XMR, value)
		      break;
		    case RegClass_kv4_v1_buffer2Reg:
		      KVX_PRINT_REG (KVX_REGFILE_DEC_X2R, value)
		      break;
		    case RegClass_kv4_v1_buffer4Reg:
		      KVX_PRINT_REG (KVX_REGFILE_DEC_X4R, value)
		      break;
		    case RegClass_kv4_v1_buffer8Reg:
		      KVX_PRINT_REG (KVX_REGFILE_DEC_X8R, value)
		      break;
		    case RegClass_kv4_v1_buffer16Reg:
		      KVX_PRINT_REG (KVX_REGFILE_DEC_X16R, value)
		      break;
		    case RegClass_kv4_v1_buffer32Reg:
		      KVX_PRINT_REG (KVX_REGFILE_DEC_X32R, value)
		      break;
		    case RegClass_kv4_v1_buffer64Reg:
		      KVX_PRINT_REG (KVX_REGFILE_DEC_X64R, value)
		      break;
		    case Immediate_kv4_v1_brknumber:
		    case Immediate_kv4_v1_sysnumber:
		    case Immediate_kv4_v1_signed10:
		    case Immediate_kv4_v1_signed16:
		    case Immediate_kv4_v1_signed27:
		    case Immediate_kv4_v1_wrapped32:
		    case Immediate_kv4_v1_signed37:
		    case Immediate_kv4_v1_signed43:
		    case Immediate_kv4_v1_signed54:
		    case Immediate_kv4_v1_wrapped64:
		    case Immediate_kv4_v1_unsigned6:
		      res->operands[idx].val = value;
		      res->operands[idx].sign = flags & kvxSIGNED;
		      res->operands[idx].width = width;
		      res->operands[idx].type = CAT_IMMEDIATE;
		      res->operands[idx].pcrel = 0;
		      idx++;
		      break;
		    case Immediate_kv4_v1_pcrel27:
		    case Immediate_kv4_v1_pcrel17:
		      res->operands[idx].val = value + memaddr;
		      res->operands[idx].sign = flags & kvxSIGNED;
		      res->operands[idx].width = width;
		      res->operands[idx].type = CAT_IMMEDIATE;
		      res->operands[idx].pcrel = 1;
		      idx++;
		      break;
		    case Modifier_kv4_v1_accesses:
		    case Modifier_kv4_v1_boolcas:
		    case Modifier_kv4_v1_cachelev:
		    case Modifier_kv4_v1_channel:
		    case Modifier_kv4_v1_coherency:
		    case Modifier_kv4_v1_comparison:
		    case Modifier_kv4_v1_conjugate:
		    case Modifier_kv4_v1_doscale:
		    case Modifier_kv4_v1_exunum:
		    case Modifier_kv4_v1_floatcomp:
		    case Modifier_kv4_v1_hindex:
		    case Modifier_kv4_v1_lsomask:
		    case Modifier_kv4_v1_lsumask:
		    case Modifier_kv4_v1_lsupack:
		    case Modifier_kv4_v1_qindex:
		    case Modifier_kv4_v1_rounding:
		    case Modifier_kv4_v1_scalarcond:
		    case Modifier_kv4_v1_shuffleV:
		    case Modifier_kv4_v1_shuffleX:
		    case Modifier_kv4_v1_silent:
		    case Modifier_kv4_v1_simplecond:
		    case Modifier_kv4_v1_speculate:
		    case Modifier_kv4_v1_splat32:
		    case Modifier_kv4_v1_transpose:
		    case Modifier_kv4_v1_variant:
		      {
			int sz = 0;
			int mod_idx = type - Modifier_kv4_v1_accesses;
			for (sz = 0; env.kvx_modifiers[mod_idx][sz]; ++sz);
			const char *mod = value < (unsigned) sz
			  ? env.kvx_modifiers[mod_idx][value] : NULL;
			if (!mod) goto retry;
			res->operands[idx].val = value;
			res->operands[idx].type = CAT_MODIFIER;
			res->operands[idx].mod_idx = mod_idx;
			idx++;
		      }
		      break;
		    default:
		      fprintf (stderr, "error: unexpected operand type (%s)\n",
			       type_name);
		      exit (-1);
		    };
		}

#undef KVX_PRINT_REG
	    }

	  found = 1;
	  break;
	retry:;
	  idx = 0;
	  continue;
	}
 }
  res->nb_ops = idx;
  return found;
}

int
print_insn_kvx (bfd_vma memaddr, struct disassemble_info *info)
{
  static int insnindex = 0;
  static int insncount = 0;
  insn_t *insn;
  int readsofar = 0;
  int found = 0;
  int invalid_bundle = 0;

  if (!env.initialized_p)
    kvx_dis_init (info);

  /* Clear instruction information field.  */
  info->insn_info_valid = 0;
  info->branch_delay_insns = 0;
  info->data_size = 0;
  info->insn_type = dis_noninsn;
  info->target = 0;
  info->target2 = 0;

  /* Set line length.  */
  info->bytes_per_line = 16;


  /* If this is the beginning of the bundle, read BUNDLESIZE words and apply
     decentrifugate function.  */
  if (insnindex == 0)
    {
      int wordcount;
      for (wordcount = 0; wordcount < KVXMAXBUNDLEWORDS; wordcount++)
	{
	  int status;
	  status =
	    (*info->read_memory_func) (memaddr + 4 * wordcount,
				       (bfd_byte *) (bundle_words +
						     wordcount), 4, info);
	  if (status != 0)
	    {
	      (*info->memory_error_func) (status, memaddr + 4 * wordcount,
					  info);
	      return -1;
	    }
	  if (!kvx_has_parallel_bit (bundle_words[wordcount]))
	    break;
	}
      wordcount++;
      invalid_bundle = !kvx_reassemble_bundle (wordcount, &insncount);
    }

  assert (insnindex < KVXMAXBUNDLEISSUE);
  insn = &(bundle_insn[insnindex]);
  readsofar = insn->len * 4;
  insnindex++;

  if (opt_pretty)
    {
      (*info->fprintf_func) (info->stream, "[ ");
      for (int i = 0; i < insn->len; i++)
	(*info->fprintf_func) (info->stream, "%08x ", insn->syllables[i]);
      (*info->fprintf_func) (info->stream, "] ");
    }

  /* Check for extension to right iff this is not the end of bundle.  */

  struct decoded_insn dec;
  memset (&dec, 0, sizeof dec);
  if (!invalid_bundle && (found = decode_insn (memaddr, insn, &dec)))
    {
      int ch;
      (*info->fprintf_func) (info->stream, "%s", dec.opc->as_op);
      const char *fmtp = dec.opc->fmtstring;
      for (int i = 0; i < dec.nb_ops; ++i)
	{
	  /* Print characters in the format string up to the following % or nul.  */
	  while ((ch = *fmtp) && ch != '%')
	    {
	      (*info->fprintf_func) (info->stream, "%c", ch);
	      fmtp++;
	    }

	  /* Skip past %s.  */
	  if (ch == '%')
	    {
	      ch = *fmtp++;
	      fmtp++;
	    }

	  switch (dec.operands[i].type)
	    {
	    case CAT_REGISTER:
	      (*info->fprintf_func) (info->stream, "%s",
				     env.kvx_registers[dec.operands[i].val].name);
	      break;
	    case CAT_MODIFIER:
	      {
		const char *mod = env.kvx_modifiers[dec.operands[i].mod_idx][dec.operands[i].val];
		(*info->fprintf_func) (info->stream, "%s", !mod || !strcmp (mod, ".") ? "" : mod);
	      }
	      break;
	    case CAT_IMMEDIATE:
	      {
		if (dec.operands[i].pcrel)
		  {
		    /* Fill in instruction information.  */
		    info->insn_info_valid = 1;
		    info->insn_type =
		      dec.operands[i].width ==
		      17 ? dis_condbranch : dis_branch;
		    info->target = dec.operands[i].val;

		    info->print_address_func (dec.operands[i].val, info);
		  }
		else if (dec.operands[i].sign)
		  {
		    if (dec.operands[i].width <= 32)
		      {
			(*info->fprintf_func) (info->stream, "%" PRId32 " (0x%" PRIx32 ")",
					       (int32_t) dec.operands[i].val,
					       (int32_t) dec.operands[i].val);
		      }
		    else
		      {
			(*info->fprintf_func) (info->stream, "%" PRId64 " (0x%" PRIx64 ")",
					       dec.operands[i].val,
					       dec.operands[i].val);
		      }
		  }
		else
		  {
		    if (dec.operands[i].width <= 32)
		      {
			(*info->fprintf_func) (info->stream, "%" PRIu32 " (0x%" PRIx32 ")",
					       (uint32_t) dec.operands[i].
					       val,
					       (uint32_t) dec.operands[i].
					       val);
		      }
		    else
		      {
			(*info->fprintf_func) (info->stream, "%" PRIu64 " (0x%" PRIx64 ")",
					       (uint64_t) dec.
					       operands[i].val,
					       (uint64_t) dec.
					       operands[i].val);
		      }
		  }
	      }
	      break;
	    default:
	      break;

	    }
	}

      while ((ch = *fmtp))
	{
	  (*info->fprintf_styled_func) (info->stream, dis_style_text, "%c",
					ch);
	  fmtp++;
	}
    }
  else
    {
      (*info->fprintf_func) (info->stream, "*** invalid opcode ***\n");
      insnindex = 0;
      readsofar = 4;
    }

  if (found && (insnindex == insncount))
    {
      (*info->fprintf_func) (info->stream, ";;");
      if (!opt_compact_assembly)
	(*info->fprintf_func) (info->stream, "\n");
      insnindex = 0;
    }

  return readsofar;
}

/* This function searches in the current bundle for the instructions required
   by unwinding. For prologue:
     (1) addd $r12 = $r12, <res_stack>
     (2) get <gpr_ra_reg> = $ra
     (3) sd <ofs>[$r12] = <gpr_ra_reg> or sq/so containing <gpr_ra_reg>
     (4) sd <ofs>[$r12] = $r14 or sq/so containing r14
     (5) addd $r14 = $r12, <fp_ofs> or copyd $r14 = $r12
	 The only difference seen between the code generated by gcc and clang
	 is the setting/resetting r14. gcc could also generate copyd $r14=$r12
	 instead of add addd $r14 = $r12, <ofs> when <ofs> is 0.
	 Vice-versa, <ofs> is not guaranteed to be 0 for clang, so, clang
	 could also generate addd instead of copyd
     (6) call, icall, goto, igoto, cb., ret
  For epilogue:
     (1) addd $r12 = $r12, <res_stack>
     (2) addd $r12 = $r14, <offset> or copyd $r12 = $r14
	 Same comment as prologue (5).
     (3) ret, goto
     (4) call, icall, igoto, cb.  */

int
decode_prologue_epilogue_bundle (bfd_vma memaddr,
				 struct disassemble_info *info,
				 struct kvx_prologue_epilogue_bundle *peb)
{
  int i, nb_insn, nb_syl;

  peb->nb_insn = 0;

  if (info->arch != bfd_arch_kvx)
    return -1;

  if (!env.initialized_p)
    kvx_dis_init (info);

  /* Read the bundle.  */
  for (nb_syl = 0; nb_syl < KVXMAXBUNDLEWORDS; nb_syl++)
    {
      if ((*info->read_memory_func) (memaddr + 4 * nb_syl,
				     (bfd_byte *) &bundle_words[nb_syl], 4,
				     info))
	return -1;
      if (!kvx_has_parallel_bit (bundle_words[nb_syl]))
	break;
    }
  nb_syl++;
  if (!kvx_reassemble_bundle (nb_syl, &nb_insn))
    return -1;

  /* Check for extension to right if this is not the end of bundle
     find the format of this insn.  */
  for (int idx_insn = 0; idx_insn < nb_insn; idx_insn++)
    {
      insn_t *insn = &bundle_insn[idx_insn];
      int is_add = 0, is_get = 0, is_a_peb_insn = 0, is_copyd = 0;

      struct decoded_insn dec;
      memset (&dec, 0, sizeof dec);
      if (!decode_insn (memaddr, insn, &dec))
	continue;

      const char *op_name = dec.opc->as_op;
      struct kvx_prologue_epilogue_insn *crt_peb_insn;

      crt_peb_insn = &peb->insn[peb->nb_insn];
      crt_peb_insn->nb_gprs = 0;

      if (!strcmp (op_name, "addd"))
	is_add = 1;
      else if (!strcmp (op_name, "copyd"))
	is_copyd = 1;
      else if (!strcmp (op_name, "get"))
	is_get = 1;
      else if (!strcmp (op_name, "sd"))
	{
	  crt_peb_insn->insn_type = KVX_PROL_EPIL_INSN_SD;
	  is_a_peb_insn = 1;
	}
      else if (!strcmp (op_name, "sq"))
	{
	  crt_peb_insn->insn_type = KVX_PROL_EPIL_INSN_SQ;
	  is_a_peb_insn = 1;
	}
      else if (!strcmp (op_name, "so"))
	{
	  crt_peb_insn->insn_type = KVX_PROL_EPIL_INSN_SO;
	  is_a_peb_insn = 1;
	}
      else if (!strcmp (op_name, "ret"))
	{
	  crt_peb_insn->insn_type = KVX_PROL_EPIL_INSN_RET;
	  is_a_peb_insn = 1;
	}
      else if (!strcmp (op_name, "goto"))
	{
	  crt_peb_insn->insn_type = KVX_PROL_EPIL_INSN_GOTO;
	  is_a_peb_insn = 1;
	}
      else if (!strcmp (op_name, "igoto"))
	{
	  crt_peb_insn->insn_type = KVX_PROL_EPIL_INSN_IGOTO;
	  is_a_peb_insn = 1;
	}
      else if (!strcmp (op_name, "call") || !strcmp (op_name, "icall"))
	{
	  crt_peb_insn->insn_type = KVX_PROL_EPIL_INSN_CALL;
	  is_a_peb_insn = 1;
	}
      else if (!strncmp (op_name, "cb", 2))
	{
	  crt_peb_insn->insn_type = KVX_PROL_EPIL_INSN_CB;
	  is_a_peb_insn = 1;
	}
      else
	continue;

      for (i = 0; dec.opc->format[i]; i++)
	{
	  struct kvx_operand *fmt = dec.opc->format[i];
	  struct kvx_bitfield *bf = fmt->bfield;
	  int bf_nb = fmt->bitfields;
	  int width = fmt->width;
	  int type = fmt->type;
	  int flags = fmt->flags;
	  int shift = fmt->shift;
	  int bias = fmt->bias;
	  uint64_t encoded_value, value = 0;

	  for (int bf_idx = 0; bf_idx < bf_nb; bf_idx++)
	    {
	      int insn_idx = (int) bf[bf_idx].to_offset / 32;
	      int to_offset = bf[bf_idx].to_offset % 32;
	      encoded_value = insn->syllables[insn_idx] >> to_offset;
	      encoded_value &= (1LL << bf[bf_idx].size) - 1;
	      value |= encoded_value << bf[bf_idx].from_offset;
	    }
	  if (flags & kvxSIGNED)
	    {
	      uint64_t signbit = 1LL << (width - 1);
	      value = (value ^ signbit) - signbit;
	    }
	  value = (value << shift) + bias;

#define chk_type(core_, val_) \
      (env.opc_table == kvx_## core_ ##_optab && type == (val_))

	  if (chk_type (kv3_v1, RegClass_kv3_v1_singleReg)
	      || chk_type (kv3_v2, RegClass_kv3_v2_singleReg)
	      || chk_type (kv4_v1, RegClass_kv4_v1_singleReg))
	    {
	      if (env.kvx_regfiles[KVX_REGFILE_DEC_GPR] + value
		  >= env.kvx_max_dec_registers)
		return -1;
	      if (is_add && i < 2)
		{
		  if (i == 0)
		    {
		      if (value == KVX_GPR_REG_SP)
			crt_peb_insn->insn_type = KVX_PROL_EPIL_INSN_ADD_SP;
		      else if (value == KVX_GPR_REG_FP)
			crt_peb_insn->insn_type = KVX_PROL_EPIL_INSN_ADD_FP;
		      else
			is_add = 0;
		    }
		  else if (i == 1)
		    {
		      if (value == KVX_GPR_REG_SP)
			is_a_peb_insn = 1;
		      else if (value == KVX_GPR_REG_FP
			       && crt_peb_insn->insn_type
			       == KVX_PROL_EPIL_INSN_ADD_SP)
			{
			  crt_peb_insn->insn_type
			    = KVX_PROL_EPIL_INSN_RESTORE_SP_FROM_FP;
			  is_a_peb_insn = 1;
			}
		      else
			is_add = 0;
		    }
		}
	      else if (is_copyd && i < 2)
		{
		  if (i == 0)
		    {
		      if (value == KVX_GPR_REG_FP)
			{
			  crt_peb_insn->insn_type = KVX_PROL_EPIL_INSN_ADD_FP;
			  crt_peb_insn->immediate = 0;
			}
		      else if (value == KVX_GPR_REG_SP)
			{
			  crt_peb_insn->insn_type
			    = KVX_PROL_EPIL_INSN_RESTORE_SP_FROM_FP;
			  crt_peb_insn->immediate = 0;
			}
		      else
			is_copyd = 0;
		    }
		  else if (i == 1)
		    {
		      if (value == KVX_GPR_REG_SP
			  && crt_peb_insn->insn_type
			  == KVX_PROL_EPIL_INSN_ADD_FP)
			is_a_peb_insn = 1;
		      else if (value == KVX_GPR_REG_FP
			       && crt_peb_insn->insn_type
			       == KVX_PROL_EPIL_INSN_RESTORE_SP_FROM_FP)
			is_a_peb_insn = 1;
		      else
			is_copyd = 0;
		    }
		}
	      else
		crt_peb_insn->gpr_reg[crt_peb_insn->nb_gprs++] = value;
	    }
	  else if (chk_type (kv3_v1, RegClass_kv3_v1_pairedReg)
		   || chk_type (kv3_v2, RegClass_kv3_v2_pairedReg)
		   || chk_type (kv4_v1, RegClass_kv4_v1_pairedReg))
	    crt_peb_insn->gpr_reg[crt_peb_insn->nb_gprs++] = value * 2;
	  else if (chk_type (kv3_v1, RegClass_kv3_v1_quadReg)
		   || chk_type (kv3_v2, RegClass_kv3_v2_quadReg)
		   || chk_type (kv4_v1, RegClass_kv4_v1_quadReg))
	    crt_peb_insn->gpr_reg[crt_peb_insn->nb_gprs++] = value * 4;
	  else if (chk_type (kv3_v1, RegClass_kv3_v1_systemReg)
		   || chk_type (kv3_v2, RegClass_kv3_v2_systemReg)
		   || chk_type (kv4_v1, RegClass_kv4_v1_systemReg)
		   || chk_type (kv3_v1, RegClass_kv3_v1_aloneReg)
		   || chk_type (kv3_v2, RegClass_kv3_v2_aloneReg)
		   || chk_type (kv4_v1, RegClass_kv4_v1_aloneReg)
		   || chk_type (kv3_v1, RegClass_kv3_v1_onlyraReg)
		   || chk_type (kv3_v2, RegClass_kv3_v2_onlyraReg)
		   || chk_type (kv4_v1, RegClass_kv4_v1_onlygetReg)
		   || chk_type (kv3_v1, RegClass_kv3_v1_onlygetReg)
		   || chk_type (kv3_v2, RegClass_kv3_v2_onlygetReg)
		   || chk_type (kv4_v1, RegClass_kv4_v1_onlygetReg)
		   || chk_type (kv3_v1, RegClass_kv3_v1_onlysetReg)
		   || chk_type (kv3_v2, RegClass_kv3_v2_onlysetReg)
		   || chk_type (kv4_v1, RegClass_kv4_v1_onlysetReg)
		   || chk_type (kv3_v1, RegClass_kv3_v1_onlyfxReg)
		   || chk_type (kv3_v2, RegClass_kv3_v2_onlyfxReg)
		   || chk_type (kv4_v1, RegClass_kv4_v1_onlyfxReg))
	    {
	      if (env.kvx_regfiles[KVX_REGFILE_DEC_GPR] + value
		  >= env.kvx_max_dec_registers)
		return -1;
	      if (is_get && !strcmp (env.kvx_registers[env.kvx_dec_registers[env.kvx_regfiles[KVX_REGFILE_DEC_SFR] + value]].name, "$ra"))
		{
		  crt_peb_insn->insn_type = KVX_PROL_EPIL_INSN_GET_RA;
		  is_a_peb_insn = 1;
		}
	    }
	  else if (chk_type (kv3_v1, RegClass_kv3_v1_coproReg)
		   || chk_type (kv3_v2, RegClass_kv3_v2_coproReg)
		   || chk_type (kv4_v1, RegClass_kv4_v1_coproReg)
		   || chk_type (kv3_v1, RegClass_kv3_v1_blockReg)
		   || chk_type (kv3_v2, RegClass_kv3_v2_blockReg)
		   || chk_type (kv4_v1, RegClass_kv4_v1_blockReg)
		   || chk_type (kv3_v1, RegClass_kv3_v1_vectorReg)
		   || chk_type (kv3_v2, RegClass_kv3_v2_vectorReg)
		   || chk_type (kv4_v1, RegClass_kv4_v1_vectorReg)
		   || chk_type (kv3_v1, RegClass_kv3_v1_tileReg)
		   || chk_type (kv3_v2, RegClass_kv3_v2_tileReg)
		   || chk_type (kv4_v1, RegClass_kv4_v1_tileReg)
		   || chk_type (kv3_v1, RegClass_kv3_v1_matrixReg)
		   || chk_type (kv3_v2, RegClass_kv3_v2_matrixReg)
		   || chk_type (kv4_v1, RegClass_kv4_v1_matrixReg)
		   || chk_type (kv3_v1, Modifier_kv3_v1_scalarcond)
		   || chk_type (kv3_v1, Modifier_kv3_v1_column)
		   || chk_type (kv3_v1, Modifier_kv3_v1_comparison)
		   || chk_type (kv3_v1, Modifier_kv3_v1_doscale)
		   || chk_type (kv3_v1, Modifier_kv3_v1_exunum)
		   || chk_type (kv3_v1, Modifier_kv3_v1_floatcomp)
		   || chk_type (kv3_v1, Modifier_kv3_v1_qindex)
		   || chk_type (kv3_v1, Modifier_kv3_v1_rectify)
		   || chk_type (kv3_v1, Modifier_kv3_v1_rounding)
		   || chk_type (kv3_v1, Modifier_kv3_v1_roundint)
		   || chk_type (kv3_v1, Modifier_kv3_v1_saturate)
		   || chk_type (kv3_v1, Modifier_kv3_v1_scalarcond)
		   || chk_type (kv3_v1, Modifier_kv3_v1_silent)
		   || chk_type (kv3_v1, Modifier_kv3_v1_simplecond)
		   || chk_type (kv3_v1, Modifier_kv3_v1_speculate)
		   || chk_type (kv3_v1, Modifier_kv3_v1_splat32)
		   || chk_type (kv3_v1, Modifier_kv3_v1_variant)
		   || chk_type (kv3_v2, Modifier_kv3_v2_accesses)
		   || chk_type (kv3_v2, Modifier_kv3_v2_boolcas)
		   || chk_type (kv3_v2, Modifier_kv3_v2_cachelev)
		   || chk_type (kv3_v2, Modifier_kv3_v2_channel)
		   || chk_type (kv3_v2, Modifier_kv3_v2_coherency)
		   || chk_type (kv3_v2, Modifier_kv3_v2_comparison)
		   || chk_type (kv3_v2, Modifier_kv3_v2_conjugate)
		   || chk_type (kv3_v2, Modifier_kv3_v2_doscale)
		   || chk_type (kv3_v2, Modifier_kv3_v2_exunum)
		   || chk_type (kv3_v2, Modifier_kv3_v2_floatcomp)
		   || chk_type (kv3_v2, Modifier_kv3_v2_hindex)
		   || chk_type (kv3_v2, Modifier_kv3_v2_lsomask)
		   || chk_type (kv3_v2, Modifier_kv3_v2_lsumask)
		   || chk_type (kv3_v2, Modifier_kv3_v2_lsupack)
		   || chk_type (kv3_v2, Modifier_kv3_v2_qindex)
		   || chk_type (kv3_v2, Modifier_kv3_v2_rounding)
		   || chk_type (kv3_v2, Modifier_kv3_v2_scalarcond)
		   || chk_type (kv3_v2, Modifier_kv3_v2_shuffleV)
		   || chk_type (kv3_v2, Modifier_kv3_v2_shuffleX)
		   || chk_type (kv3_v2, Modifier_kv3_v2_silent)
		   || chk_type (kv3_v2, Modifier_kv3_v2_simplecond)
		   || chk_type (kv3_v2, Modifier_kv3_v2_speculate)
		   || chk_type (kv3_v2, Modifier_kv3_v2_splat32)
		   || chk_type (kv3_v2, Modifier_kv3_v2_transpose)
		   || chk_type (kv3_v2, Modifier_kv3_v2_variant)
		   || chk_type (kv4_v1, Modifier_kv4_v1_accesses)
		   || chk_type (kv4_v1, Modifier_kv4_v1_boolcas)
		   || chk_type (kv4_v1, Modifier_kv4_v1_cachelev)
		   || chk_type (kv4_v1, Modifier_kv4_v1_channel)
		   || chk_type (kv4_v1, Modifier_kv4_v1_coherency)
		   || chk_type (kv4_v1, Modifier_kv4_v1_comparison)
		   || chk_type (kv4_v1, Modifier_kv4_v1_conjugate)
		   || chk_type (kv4_v1, Modifier_kv4_v1_doscale)
		   || chk_type (kv4_v1, Modifier_kv4_v1_exunum)
		   || chk_type (kv4_v1, Modifier_kv4_v1_floatcomp)
		   || chk_type (kv4_v1, Modifier_kv4_v1_hindex)
		   || chk_type (kv4_v1, Modifier_kv4_v1_lsomask)
		   || chk_type (kv4_v1, Modifier_kv4_v1_lsumask)
		   || chk_type (kv4_v1, Modifier_kv4_v1_lsupack)
		   || chk_type (kv4_v1, Modifier_kv4_v1_qindex)
		   || chk_type (kv4_v1, Modifier_kv4_v1_rounding)
		   || chk_type (kv4_v1, Modifier_kv4_v1_scalarcond)
		   || chk_type (kv4_v1, Modifier_kv4_v1_shuffleV)
		   || chk_type (kv4_v1, Modifier_kv4_v1_shuffleX)
		   || chk_type (kv4_v1, Modifier_kv4_v1_silent)
		   || chk_type (kv4_v1, Modifier_kv4_v1_simplecond)
		   || chk_type (kv4_v1, Modifier_kv4_v1_speculate)
		   || chk_type (kv4_v1, Modifier_kv4_v1_splat32)
		   || chk_type (kv4_v1, Modifier_kv4_v1_transpose)
		   || chk_type (kv4_v1, Modifier_kv4_v1_variant))
	    {
	      /* Do nothing.  */
	    }
	  else if (chk_type (kv3_v1, Immediate_kv3_v1_sysnumber)
		   || chk_type (kv3_v2, Immediate_kv3_v2_sysnumber)
		   || chk_type (kv4_v1, Immediate_kv4_v1_sysnumber)
		   || chk_type (kv3_v2, Immediate_kv3_v2_wrapped8)
		   || chk_type (kv4_v1, Immediate_kv4_v1_wrapped8)
		   || chk_type (kv3_v1, Immediate_kv3_v1_signed10)
		   || chk_type (kv3_v2, Immediate_kv3_v2_signed10)
		   || chk_type (kv4_v1, Immediate_kv4_v1_signed10)
		   || chk_type (kv3_v1, Immediate_kv3_v1_signed16)
		   || chk_type (kv3_v2, Immediate_kv3_v2_signed16)
		   || chk_type (kv4_v1, Immediate_kv4_v1_signed16)
		   || chk_type (kv3_v1, Immediate_kv3_v1_signed27)
		   || chk_type (kv3_v2, Immediate_kv3_v2_signed27)
		   || chk_type (kv4_v1, Immediate_kv4_v1_signed27)
		   || chk_type (kv3_v1, Immediate_kv3_v1_wrapped32)
		   || chk_type (kv3_v2, Immediate_kv3_v2_wrapped32)
		   || chk_type (kv4_v1, Immediate_kv4_v1_wrapped32)
		   || chk_type (kv3_v1, Immediate_kv3_v1_signed37)
		   || chk_type (kv3_v2, Immediate_kv3_v2_signed37)
		   || chk_type (kv4_v1, Immediate_kv4_v1_signed37)
		   || chk_type (kv3_v1, Immediate_kv3_v1_signed43)
		   || chk_type (kv3_v2, Immediate_kv3_v2_signed43)
		   || chk_type (kv4_v1, Immediate_kv4_v1_signed43)
		   || chk_type (kv3_v1, Immediate_kv3_v1_signed54)
		   || chk_type (kv3_v2, Immediate_kv3_v2_signed54)
		   || chk_type (kv4_v1, Immediate_kv4_v1_signed54)
		   || chk_type (kv3_v1, Immediate_kv3_v1_wrapped64)
		   || chk_type (kv3_v2, Immediate_kv3_v2_wrapped64)
		   || chk_type (kv4_v1, Immediate_kv4_v1_wrapped64)
		   || chk_type (kv3_v1, Immediate_kv3_v1_unsigned6)
		   || chk_type (kv3_v2, Immediate_kv3_v2_unsigned6)
		   || chk_type (kv4_v1, Immediate_kv4_v1_unsigned6))
	    crt_peb_insn->immediate = value;
	  else if (chk_type (kv3_v1, Immediate_kv3_v1_pcrel17)
		   || chk_type (kv3_v2, Immediate_kv3_v2_pcrel17)
		   || chk_type (kv4_v1, Immediate_kv4_v1_pcrel17)
		   || chk_type (kv3_v1, Immediate_kv3_v1_pcrel27)
		   || chk_type (kv3_v2, Immediate_kv3_v2_pcrel27)
		   || chk_type (kv4_v1, Immediate_kv4_v1_pcrel27))
	    crt_peb_insn->immediate = value + memaddr;
	  else
	    return -1;
	}

      if (is_a_peb_insn)
	peb->nb_insn++;
      continue;
    }

  return nb_syl * 4;
#undef chk_type
}

void
print_kvx_disassembler_options (FILE * stream)
{
  fprintf (stream, _("\n\
The following KVX specific disassembler options are supported for use\n\
with the -M switch (multiple options should be separated by commas):\n"));

  fprintf (stream, _("\n\
  pretty               Print 32-bit words in natural order corresponding to \
re-ordered instruction.\n"));

  fprintf (stream, _("\n\
  compact-assembly     Do not emit a new line between bundles of instructions.\
\n"));

  fprintf (stream, _("\n\
  no-compact-assembly  Emit a new line between bundles of instructions.\n"));

  fprintf (stream, _("\n"));
}
