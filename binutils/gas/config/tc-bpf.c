/* tc-bpf.c -- Assembler for the Linux eBPF.
   Copyright (C) 2019-2024 Free Software Foundation, Inc.
   Contributed by Oracle, Inc.

   This file is part of GAS, the GNU Assembler.

   GAS is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GAS is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GAS; see the file COPYING.  If not, write to
   the Free Software Foundation, 51 Franklin Street - Fifth Floor,
   Boston, MA 02110-1301, USA.  */

#include "as.h"
#include "subsegs.h"
#include "symcat.h"
#include "opcode/bpf.h"
#include "elf/common.h"
#include "elf/bpf.h"
#include "dwarf2dbg.h"
#include "libiberty.h"
#include <ctype.h>

/* Data structure representing a parsed BPF instruction.  */

struct bpf_insn
{
  enum bpf_insn_id id;
  int size; /* Instruction size in bytes.  */
  bpf_insn_word opcode;
  uint8_t dst;
  uint8_t src;
  expressionS offset16;
  expressionS imm32;
  expressionS imm64;
  expressionS disp16;
  expressionS disp32;

  unsigned int has_dst : 1;
  unsigned int has_src : 1;
  unsigned int has_offset16 : 1;
  unsigned int has_disp16 : 1;
  unsigned int has_disp32 : 1;
  unsigned int has_imm32 : 1;
  unsigned int has_imm64 : 1;

  unsigned int is_relaxable : 1;
  expressionS *relaxed_exp;
};

const char comment_chars[]        = "#";
const char line_comment_chars[]   = "#";
const char line_separator_chars[] = ";`";
const char EXP_CHARS[]            = "eE";
const char FLT_CHARS[]            = "fFdD";

/* Like s_lcomm_internal in gas/read.c but the alignment string
   is allowed to be optional.  */

static symbolS *
pe_lcomm_internal (int needs_align, symbolS *symbolP, addressT size)
{
  addressT align = 0;

  SKIP_WHITESPACE ();

  if (needs_align
      && *input_line_pointer == ',')
    {
      align = parse_align (needs_align - 1);

      if (align == (addressT) -1)
	return NULL;
    }
  else
    {
      if (size >= 8)
	align = 3;
      else if (size >= 4)
	align = 2;
      else if (size >= 2)
	align = 1;
      else
	align = 0;
    }

  bss_alloc (symbolP, size, align);
  return symbolP;
}

static void
pe_lcomm (int needs_align)
{
  s_comm_internal (needs_align * 2, pe_lcomm_internal);
}

/* The target specific pseudo-ops which we support.  */
const pseudo_typeS md_pseudo_table[] =
{
    { "half",      cons,              2 },
    { "word",      cons,              4 },
    { "dword",     cons,              8 },
    { "lcomm",	   pe_lcomm,	      1 },
    { NULL,        NULL,              0 }
};



/* Command-line options processing.  */

enum options
{
  OPTION_LITTLE_ENDIAN = OPTION_MD_BASE,
  OPTION_BIG_ENDIAN,
  OPTION_XBPF,
  OPTION_DIALECT,
  OPTION_ISA_SPEC,
  OPTION_NO_RELAX,
};

struct option md_longopts[] =
{
  { "EL", no_argument, NULL, OPTION_LITTLE_ENDIAN },
  { "EB", no_argument, NULL, OPTION_BIG_ENDIAN },
  { "mxbpf", no_argument, NULL, OPTION_XBPF },
  { "mdialect", required_argument, NULL, OPTION_DIALECT},
  { "misa-spec", required_argument, NULL, OPTION_ISA_SPEC},
  { "mno-relax", no_argument, NULL, OPTION_NO_RELAX},
  { NULL,          no_argument, NULL, 0 },
};

size_t md_longopts_size = sizeof (md_longopts);

const char * md_shortopts = "";

/* BPF supports little-endian and big-endian variants.  The following
   global records what endianness to use.  It can be configured using
   command-line options.  It defaults to the host endianness
   initialized in md_begin.  */

static int set_target_endian = 0;
extern int target_big_endian;

/* Whether to relax branch instructions.  Default is yes.  Can be
   changed using the -mno-relax command line option.  */

static int do_relax = 1;

/* The ISA specification can be one of BPF_V1, BPF_V2, BPF_V3, BPF_V4
   or BPF_XPBF.  The ISA spec to use can be configured using
   command-line options.  It defaults to the latest BPF spec.  */

static int isa_spec = BPF_V4;

/* The assembler supports two different dialects: "normal" syntax and
   "pseudoc" syntax.  The dialect to use can be configured using
   command-line options.  */

enum target_asm_dialect
{
  DIALECT_NORMAL,
  DIALECT_PSEUDOC
};

static int asm_dialect = DIALECT_NORMAL;

int
md_parse_option (int c, const char * arg)
{
  switch (c)
    {
    case OPTION_BIG_ENDIAN:
      set_target_endian = 1;
      target_big_endian = 1;
      break;
    case OPTION_LITTLE_ENDIAN:
      set_target_endian = 0;
      target_big_endian = 0;
      break;
    case OPTION_DIALECT:
      if (strcmp (arg, "normal") == 0)
        asm_dialect = DIALECT_NORMAL;
      else if (strcmp (arg, "pseudoc") == 0)
        asm_dialect = DIALECT_PSEUDOC;
      else
        as_fatal (_("-mdialect=%s is not valid.  Expected normal or pseudoc"),
                  arg);
      break;
    case OPTION_ISA_SPEC:
      if (strcmp (arg, "v1") == 0)
        isa_spec = BPF_V1;
      else if (strcmp (arg, "v2") == 0)
        isa_spec = BPF_V2;
      else if (strcmp (arg, "v3") == 0)
        isa_spec = BPF_V3;
      else if (strcmp (arg, "v4") == 0)
        isa_spec = BPF_V4;
      else if (strcmp (arg, "xbpf") == 0)
        isa_spec = BPF_XBPF;
      else
        as_fatal (_("-misa-spec=%s is not valid.  Expected v1, v2, v3, v4 o xbpf"),
                  arg);
      break;
    case OPTION_XBPF:
      /* This is an alias for -misa-spec=xbpf.  */
      isa_spec = BPF_XBPF;
      break;
    case OPTION_NO_RELAX:
      do_relax = 0;
      break;
    default:
      return 0;
    }

  return 1;
}

void
md_show_usage (FILE * stream)
{
  fprintf (stream, _("\nBPF options:\n"));
  fprintf (stream, _("\
BPF options:\n\
  -EL                         generate code for a little endian machine\n\
  -EB                         generate code for a big endian machine\n\
  -mdialect=DIALECT           set the assembly dialect (normal, pseudoc)\n\
  -misa-spec                  set the BPF ISA spec (v1, v2, v3, v4, xbpf)\n\
  -mxbpf                      alias for -misa-spec=xbpf\n"));
}


/* This function is called once, at assembler startup time.  This
   should set up all the tables, etc that the MD part of the assembler
   needs.  */

void
md_begin (void)
{
  /* If not specified in the command line, use the host
     endianness.  */
  if (!set_target_endian)
    {
#ifdef WORDS_BIGENDIAN
      target_big_endian = 1;
#else
      target_big_endian = 0;
#endif
    }

  /* Ensure that lines can begin with '*' in BPF store pseudoc instruction.  */
  lex_type['*'] |= LEX_BEGIN_NAME;

  /* Set the machine type. */
  bfd_default_set_arch_mach (stdoutput, bfd_arch_bpf, bfd_mach_bpf);
}

/* Round up a section size to the appropriate boundary.  */

valueT
md_section_align (segT segment, valueT size)
{
  int align = bfd_section_alignment (segment);

  return ((size + (1 << align) - 1) & -(1 << align));
}

/* Return non-zero if the indicated VALUE has overflowed the maximum
   range expressible by an signed number with the indicated number of
   BITS.  */

static bool
signed_overflow (offsetT value, unsigned bits)
{
  offsetT lim;
  if (bits >= sizeof (offsetT) * 8)
    return false;
  lim = (offsetT) 1 << (bits - 1);
  return (value < -lim || value >= lim);
}

/* Return non-zero if the two's complement encoding of VALUE would
   overflow an immediate field of width BITS bits.  */

static bool
immediate_overflow (int64_t value, unsigned bits)
{
  if (value < 0)
    return signed_overflow (value, bits);
  else
    {
      valueT lim;

      if (bits >= sizeof (valueT) * 8)
        return false;

      lim = (valueT) 1 << bits;
      return ((valueT) value >= lim);
    }
}


/* Functions concerning relocs.  */

/* The location from which a PC relative jump should be calculated,
   given a PC relative reloc.  */

long
md_pcrel_from_section (fixS *fixP, segT sec)
{
  if (fixP->fx_addsy != (symbolS *) NULL
      && (! S_IS_DEFINED (fixP->fx_addsy)
          || (S_GET_SEGMENT (fixP->fx_addsy) != sec)
          || S_IS_EXTERNAL (fixP->fx_addsy)
          || S_IS_WEAK (fixP->fx_addsy)))
    {
        /* The symbol is undefined (or is defined but not in this section).
         Let the linker figure it out.  */
      return 0;
    }

  return fixP->fx_where + fixP->fx_frag->fr_address;
}

/* Write a value out to the object file, using the appropriate endianness.  */

void
md_number_to_chars (char * buf, valueT val, int n)
{
  if (target_big_endian)
    number_to_chars_bigendian (buf, val, n);
  else
    number_to_chars_littleendian (buf, val, n);
}

arelent *
tc_gen_reloc (asection *sec ATTRIBUTE_UNUSED, fixS *fixP)
{
  bfd_reloc_code_real_type r_type = fixP->fx_r_type;
  arelent *reloc;

  reloc = XNEW (arelent);

  if (fixP->fx_pcrel)
   {
      r_type = (r_type == BFD_RELOC_8 ? BFD_RELOC_8_PCREL
                : r_type == BFD_RELOC_16 ? BFD_RELOC_16_PCREL
                : r_type == BFD_RELOC_24 ? BFD_RELOC_24_PCREL
                : r_type == BFD_RELOC_32 ? BFD_RELOC_32_PCREL
                : r_type == BFD_RELOC_64 ? BFD_RELOC_64_PCREL
                : r_type);
   }

  reloc->howto = bfd_reloc_type_lookup (stdoutput, r_type);

  if (reloc->howto == (reloc_howto_type *) NULL)
    {
      as_bad_where (fixP->fx_file, fixP->fx_line,
		    _("relocation is not supported"));
      return NULL;
    }

  //XXX  gas_assert (!fixP->fx_pcrel == !reloc->howto->pc_relative);

  reloc->sym_ptr_ptr = XNEW (asymbol *);
  *reloc->sym_ptr_ptr = symbol_get_bfdsym (fixP->fx_addsy);

  /* Use fx_offset for these cases.  */
  if (fixP->fx_r_type == BFD_RELOC_VTABLE_ENTRY
      || fixP->fx_r_type == BFD_RELOC_VTABLE_INHERIT)
    reloc->addend = fixP->fx_offset;
  else
    reloc->addend = fixP->fx_addnumber;

  reloc->address = fixP->fx_frag->fr_address + fixP->fx_where;
  return reloc;
}


/* Relaxations supported by this assembler.  */

#define RELAX_BRANCH_ENCODE(uncond, constant, length)    \
  ((relax_substateT)                                     \
   (0xc0000000                                           \
    | ((uncond) ? 1 : 0)                                 \
    | ((constant) ? 2 : 0)                               \
    | ((length) << 2)))

#define RELAX_BRANCH_P(i) (((i) & 0xf0000000) == 0xc0000000)
#define RELAX_BRANCH_LENGTH(i) (((i) >> 2) & 0xff)
#define RELAX_BRANCH_CONST(i) (((i) & 2) != 0)
#define RELAX_BRANCH_UNCOND(i) (((i) & 1) != 0)


/* Compute the length of a branch sequence, and adjust the stored
   length accordingly.  If FRAG is NULL, the worst-case length is
   returned.  */

static unsigned
relaxed_branch_length (fragS *fragp, asection *sec, int update)
{
  int length, uncond;

  if (!fragp)
    return 8 * 3;

  uncond = RELAX_BRANCH_UNCOND (fragp->fr_subtype);
  length = RELAX_BRANCH_LENGTH (fragp->fr_subtype);

  if (uncond)
    /* Length is the same for both JA and JAL.  */
    length = 8;
  else
    {
      if (RELAX_BRANCH_CONST (fragp->fr_subtype))
        {
          int64_t val = fragp->fr_offset;

          if (val < -32768 || val > 32767)
            length =  8 * 3;
          else
            length = 8;
        }
      else if (fragp->fr_symbol != NULL
          && S_IS_DEFINED (fragp->fr_symbol)
          && !S_IS_WEAK (fragp->fr_symbol)
          && sec == S_GET_SEGMENT (fragp->fr_symbol))
        {
          offsetT val = S_GET_VALUE (fragp->fr_symbol) + fragp->fr_offset;

          /* Convert to 64-bit words, minus one.  */
          val = (val - 8) / 8;

          /* See if it fits in the signed 16-bits field.  */
          if (val < -32768 || val > 32767)
            length = 8 * 3;
          else
            length = 8;
        }
      else
        /* Use short version, and let the linker relax instead, if
           appropriate and if supported.  */
        length = 8;
    }

  if (update)
    fragp->fr_subtype = RELAX_BRANCH_ENCODE (uncond,
                                             RELAX_BRANCH_CONST (fragp->fr_subtype),
                                             length);

  return length;
}

/* Estimate the size of a variant frag before relaxing.  */

int
md_estimate_size_before_relax (fragS *fragp, asection *sec)
{
  return (fragp->fr_var = relaxed_branch_length (fragp, sec, true));
}

/* Read a BPF instruction word from BUF.  */

static uint64_t
read_insn_word (bfd_byte *buf)
{
  return bfd_getb64 (buf);
}

/* Write the given signed 16-bit value in the given BUFFER using the
   target endianness.  */

static void
encode_int16 (int16_t value, char *buffer)
{
  uint16_t val = value;

  if (target_big_endian)
    {
      buffer[0] = (val >> 8) & 0xff;
      buffer[1] = val & 0xff;
    }
  else
    {
      buffer[1] = (val >> 8) & 0xff;
      buffer[0] = val & 0xff;
    }
}

/* Write the given signed 32-bit value in the given BUFFER using the
   target endianness.  */

static void
encode_int32 (int32_t value, char *buffer)
{
  uint32_t val = value;

  if (target_big_endian)
    {
      buffer[0] = (val >> 24) & 0xff;
      buffer[1] = (val >> 16) & 0xff;
      buffer[2] = (val >> 8) & 0xff;
      buffer[3] = val & 0xff;
    }
  else
    {
      buffer[3] = (val >> 24) & 0xff;
      buffer[2] = (val >> 16) & 0xff;
      buffer[1] = (val >> 8) & 0xff;
      buffer[0] = value & 0xff;
    }
}

/* Write a BPF instruction to BUF.  */

static void
write_insn_bytes (bfd_byte *buf, char *bytes)
{
  int i;

  for (i = 0; i < 8; ++i)
    md_number_to_chars ((char *) buf + i, (valueT) bytes[i], 1);
}

/* *FRAGP has been relaxed to its final size, and now needs to have
   the bytes inside it modified to conform to the new size.

   Called after relaxation is finished.
   fragP->fr_type == rs_machine_dependent.
   fragP->fr_subtype is the subtype of what the address relaxed to.  */

void
md_convert_frag (bfd *abfd ATTRIBUTE_UNUSED,
		 segT sec ATTRIBUTE_UNUSED,
		 fragS *fragp ATTRIBUTE_UNUSED)
{
  bfd_byte *buf = (bfd_byte *) fragp->fr_literal + fragp->fr_fix;
  expressionS exp;
  fixS *fixp;
  bpf_insn_word word;
  int disp_is_known = 0;
  int64_t disp_to_target = 0;

  uint64_t code;

  gas_assert (RELAX_BRANCH_P (fragp->fr_subtype));

  /* Expression to be used in any resulting relocation in the relaxed
     instructions.  */
  exp.X_op = O_symbol;
  exp.X_add_symbol = fragp->fr_symbol;
  exp.X_add_number = fragp->fr_offset;

  gas_assert (fragp->fr_var == RELAX_BRANCH_LENGTH (fragp->fr_subtype));

  /* Read an instruction word from the instruction to be relaxed, and
     get the code.  */
  word = read_insn_word (buf);
  code = (word >> 60) & 0xf;

  /* Determine whether the 16-bit displacement to the target is known
     at this point.  */
  if (RELAX_BRANCH_CONST (fragp->fr_subtype))
    {
      disp_to_target = fragp->fr_offset;
      disp_is_known = 1;
    }
  else if (fragp->fr_symbol != NULL
           && S_IS_DEFINED (fragp->fr_symbol)
           && !S_IS_WEAK (fragp->fr_symbol)
           && sec == S_GET_SEGMENT (fragp->fr_symbol))
    {
      offsetT val = S_GET_VALUE (fragp->fr_symbol) + fragp->fr_offset;
      /* Convert to 64-bit blocks minus one.  */
      disp_to_target = (val - 8) / 8;
      disp_is_known = 1;
    }

  /* The displacement should fit in a signed 32-bit number.  */
  if (disp_is_known && signed_overflow (disp_to_target, 32))
    as_bad_where (fragp->fr_file, fragp->fr_line,
                  _("signed instruction operand out of range, shall fit in 32 bits"));

  /* Now relax particular jump instructions.  */
  if (code == BPF_CODE_JA)
    {
      /* Unconditional jump.
         JA d16 -> JAL d32  */

      gas_assert (RELAX_BRANCH_UNCOND (fragp->fr_subtype));

      if (disp_is_known)
        {
          if (disp_to_target >= -32768 && disp_to_target <= 32767)
            {
              /* 16-bit disp is known and in range.  Install a fixup
                 for the disp16 if the branch value is not constant.
                 This will be resolved by the assembler and units
                 converted.  */

              if (!RELAX_BRANCH_CONST (fragp->fr_subtype))
                {
                  /* Install fixup for the JA.  */
                  reloc_howto_type *reloc_howto
                    = bfd_reloc_type_lookup (stdoutput, BFD_RELOC_BPF_DISP16);
                  if (!reloc_howto)
                    abort();

                  fixp = fix_new_exp (fragp, buf - (bfd_byte *) fragp->fr_literal,
                                      bfd_get_reloc_size (reloc_howto),
                                      &exp,
                                      reloc_howto->pc_relative,
                                      BFD_RELOC_BPF_DISP16);
                  fixp->fx_file = fragp->fr_file;
                  fixp->fx_line = fragp->fr_line;
                }
            }
          else
            {
              /* 16-bit disp is known and not in range.  Turn the JA
                 into a JAL with a 32-bit displacement.  */
              char bytes[8];

              bytes[0] = ((BPF_CLASS_JMP32|BPF_CODE_JA|BPF_SRC_K) >> 56) & 0xff;
              bytes[1] = (word >> 48) & 0xff;
              bytes[2] = 0; /* disp16 high */
              bytes[3] = 0; /* disp16 lo */
              encode_int32 ((int32_t) disp_to_target, bytes + 4);

              write_insn_bytes (buf, bytes);
            }
        }
      else
        {
          /* The displacement to the target is not known.  Do not
             relax.  The linker will maybe do it if it chooses to.  */

          reloc_howto_type *reloc_howto = NULL;

          gas_assert (!RELAX_BRANCH_CONST (fragp->fr_subtype));

          /* Install fixup for the JA.  */
          reloc_howto = bfd_reloc_type_lookup (stdoutput, BFD_RELOC_BPF_DISP16);
          if (!reloc_howto)
            abort ();

          fixp = fix_new_exp (fragp, buf - (bfd_byte *) fragp->fr_literal,
                              bfd_get_reloc_size (reloc_howto),
                              &exp,
                              reloc_howto->pc_relative,
                              BFD_RELOC_BPF_DISP16);
          fixp->fx_file = fragp->fr_file;
          fixp->fx_line = fragp->fr_line;
        }

      buf += 8;
    }
  else
    {
      /* Conditional jump.
         JXX d16 -> JXX +1; JA +1; JAL d32 */

      gas_assert (!RELAX_BRANCH_UNCOND (fragp->fr_subtype));

      if (disp_is_known)
        {
          if (disp_to_target >= -32768 && disp_to_target <= 32767)
            {
              /* 16-bit disp is known and in range.  Install a fixup
                 for the disp16 if the branch value is not constant.
                 This will be resolved by the assembler and units
                 converted.  */

              if (!RELAX_BRANCH_CONST (fragp->fr_subtype))
                {
                  /* Install fixup for the branch.  */
                  reloc_howto_type *reloc_howto
                    = bfd_reloc_type_lookup (stdoutput, BFD_RELOC_BPF_DISP16);
                  if (!reloc_howto)
                    abort();

                  fixp = fix_new_exp (fragp, buf - (bfd_byte *) fragp->fr_literal,
                                      bfd_get_reloc_size (reloc_howto),
                                      &exp,
                                      reloc_howto->pc_relative,
                                      BFD_RELOC_BPF_DISP16);
                  fixp->fx_file = fragp->fr_file;
                  fixp->fx_line = fragp->fr_line;
                }

              buf += 8;
            }
          else
            {
              /* 16-bit disp is known and not in range.  Turn the JXX
                 into a sequence JXX +1; JA +1; JAL d32.  */

              char bytes[8];

              /* First, set the 16-bit offset in the current
                 instruction to 1.  */

              if (target_big_endian)
                bfd_putb16 (1, buf + 2);
              else
                bfd_putl16 (1, buf + 2);
              buf += 8;

              /* Then, write the JA + 1  */

              bytes[0] = 0x05; /* JA */
              bytes[1] = 0x0;
              encode_int16 (1, bytes + 2);
              bytes[4] = 0x0;
              bytes[5] = 0x0;
              bytes[6] = 0x0;
              bytes[7] = 0x0;
              write_insn_bytes (buf, bytes);
              buf += 8;

              /* Finally, write the JAL to the target. */

              bytes[0] = ((BPF_CLASS_JMP32|BPF_CODE_JA|BPF_SRC_K) >> 56) & 0xff;
              bytes[1] = 0;
              bytes[2] = 0;
              bytes[3] = 0;
              encode_int32 ((int32_t) disp_to_target, bytes + 4);
              write_insn_bytes (buf, bytes);
              buf += 8;
            }
        }
      else
        {
          /* The displacement to the target is not known.  Do not
             relax.  The linker will maybe do it if it chooses to.  */

          reloc_howto_type *reloc_howto = NULL;

          gas_assert (!RELAX_BRANCH_CONST (fragp->fr_subtype));

          /* Install fixup for the conditional jump.  */
          reloc_howto = bfd_reloc_type_lookup (stdoutput, BFD_RELOC_BPF_DISP16);
          if (!reloc_howto)
            abort ();

          fixp = fix_new_exp (fragp, buf - (bfd_byte *) fragp->fr_literal,
                              bfd_get_reloc_size (reloc_howto),
                              &exp,
                              reloc_howto->pc_relative,
                              BFD_RELOC_BPF_DISP16);
          fixp->fx_file = fragp->fr_file;
          fixp->fx_line = fragp->fr_line;
          buf += 8;
        }
    }

  gas_assert (buf == (bfd_byte *)fragp->fr_literal
              + fragp->fr_fix + fragp->fr_var);

  fragp->fr_fix += fragp->fr_var;
}


/* Apply a fixS (fixup of an instruction or data that we didn't have
   enough info to complete immediately) to the data in a frag.  */

void
md_apply_fix (fixS *fixP, valueT *valP, segT seg ATTRIBUTE_UNUSED)
{
  char *where = fixP->fx_frag->fr_literal + fixP->fx_where;

  switch (fixP->fx_r_type)
    {
    case BFD_RELOC_BPF_DISP16:
      /* Convert from bytes to number of 64-bit words to the target,
         minus one.  */
      *valP = (((long) (*valP)) - 8) / 8;
      break;
    case BFD_RELOC_BPF_DISPCALL32:
    case BFD_RELOC_BPF_DISP32:
      /* Convert from bytes to number of 64-bit words to the target,
         minus one.  */
      *valP = (((long) (*valP)) - 8) / 8;

      if (fixP->fx_r_type == BFD_RELOC_BPF_DISPCALL32)
        {
          /* eBPF supports two kind of CALL instructions: the so
             called pseudo calls ("bpf to bpf") and external calls
             ("bpf to kernel").

             Both kind of calls use the same instruction (CALL).
             However, external calls are constructed by passing a
             constant argument to the instruction, whereas pseudo
             calls result from expressions involving symbols.  In
             practice, instructions requiring a fixup are interpreted
             as pseudo-calls.  If we are executing this code, this is
             a pseudo call.

             The kernel expects for pseudo-calls to be annotated by
             having BPF_PSEUDO_CALL in the SRC field of the
             instruction.  But beware the infamous nibble-swapping of
             eBPF and take endianness into account here.

             Note that the CALL instruction has only one operand, so
             this code is executed only once per instruction.  */
          md_number_to_chars (where + 1, target_big_endian ? 0x01 : 0x10, 1);
        }
      break;
    case BFD_RELOC_16_PCREL:
      /* Convert from bytes to number of 64-bit words to the target,
         minus one.  */
      *valP = (((long) (*valP)) - 8) / 8;
      break;
    default:
      break;
    }

  if (fixP->fx_addsy == (symbolS *) NULL)
    fixP->fx_done = 1;

  if (fixP->fx_done)
    {
      /* We're finished with this fixup.  Install it because
	 bfd_install_relocation won't be called to do it.  */
      switch (fixP->fx_r_type)
	{
	case BFD_RELOC_8:
	  md_number_to_chars (where, *valP, 1);
	  break;
	case BFD_RELOC_16:
	  md_number_to_chars (where, *valP, 2);
	  break;
	case BFD_RELOC_32:
	  md_number_to_chars (where, *valP, 4);
	  break;
	case BFD_RELOC_64:
	  md_number_to_chars (where, *valP, 8);
	  break;
        case BFD_RELOC_BPF_DISP16:
          md_number_to_chars (where + 2, (uint16_t) *valP, 2);
          break;
        case BFD_RELOC_BPF_DISP32:
        case BFD_RELOC_BPF_DISPCALL32:
          md_number_to_chars (where + 4, (uint32_t) *valP, 4);
          break;
        case BFD_RELOC_16_PCREL:
          md_number_to_chars (where + 2, (uint32_t) *valP, 2);
          break;
	default:
	  as_bad_where (fixP->fx_file, fixP->fx_line,
			_("internal error: can't install fix for reloc type %d (`%s')"),
			fixP->fx_r_type, bfd_get_reloc_code_name (fixP->fx_r_type));
	  break;
	}
    }

  /* Tuck `value' away for use by tc_gen_reloc.
     See the comment describing fx_addnumber in write.h.
     This field is misnamed (or misused :-).  */
  fixP->fx_addnumber = *valP;
}


/* Instruction writing routines.  */

/* Encode a BPF instruction in the given buffer BYTES.  Non-constant
   immediates are encoded as zeroes.  */

static void
encode_insn (struct bpf_insn *insn, char *bytes,
             int relaxed ATTRIBUTE_UNUSED)
{
  uint8_t src, dst;

  /* Zero all the bytes.  */
  memset (bytes, 0, 16);

  /* First encode the opcodes.  Note that we have to handle the
     endianness groups of the BPF instructions: 8 | 4 | 4 | 16 |
     32. */
  if (target_big_endian)
    {
      /* code */
      bytes[0] = (insn->opcode >> 56) & 0xff;
      /* regs */
      bytes[1] = (insn->opcode >> 48) & 0xff;
      /* offset16 */
      bytes[2] = (insn->opcode >> 40) & 0xff;
      bytes[3] = (insn->opcode >> 32) & 0xff;
      /* imm32 */
      bytes[4] = (insn->opcode >> 24) & 0xff;
      bytes[5] = (insn->opcode >> 16) & 0xff;
      bytes[6] = (insn->opcode >> 8) & 0xff;
      bytes[7] = insn->opcode & 0xff;
    }
  else
    {
      /* code */
      bytes[0] = (insn->opcode >> 56) & 0xff;
      /* regs */
      bytes[1] = (((((insn->opcode >> 48) & 0xff) & 0xf) << 4)
                  | (((insn->opcode >> 48) & 0xff) & 0xf));
      /* offset16 */
      bytes[3] = (insn->opcode >> 40) & 0xff;
      bytes[2] = (insn->opcode >> 32) & 0xff;
      /* imm32 */
      bytes[7] = (insn->opcode >> 24) & 0xff;
      bytes[6] = (insn->opcode >> 16) & 0xff;
      bytes[5] = (insn->opcode >> 8) & 0xff;
      bytes[4] = insn->opcode & 0xff;
    }

  /* Now the registers.  */
  src = insn->has_src ? insn->src : 0;
  dst = insn->has_dst ? insn->dst : 0;

  if (target_big_endian)
    bytes[1] = ((dst & 0xf) << 4) | (src & 0xf);
  else
    bytes[1] = ((src & 0xf) << 4) | (dst & 0xf);

  /* Now the immediates that are known to be constant.  */

  if (insn->has_imm32 && insn->imm32.X_op == O_constant)
    {
      int64_t imm = insn->imm32.X_add_number;

      if (immediate_overflow (imm, 32))
        as_bad (_("immediate out of range, shall fit in 32 bits"));
      else
        encode_int32 (insn->imm32.X_add_number, bytes + 4);        
    }

  if (insn->has_disp32 && insn->disp32.X_op == O_constant)
    {
      int64_t disp = insn->disp32.X_add_number;

      if (immediate_overflow (disp, 32))
        as_bad (_("pc-relative offset out of range, shall fit in 32 bits"));
      else
        encode_int32 (insn->disp32.X_add_number, bytes + 4);
    }

  if (insn->has_offset16 && insn->offset16.X_op == O_constant)
    {
      int64_t offset = insn->offset16.X_add_number;

      if (immediate_overflow (offset, 16))
        as_bad (_("pc-relative offset out of range, shall fit in 16 bits"));
      else
        encode_int16 (insn->offset16.X_add_number, bytes + 2);
    }

  if (insn->has_disp16 && insn->disp16.X_op == O_constant)
    {
      int64_t disp = insn->disp16.X_add_number;

      if (immediate_overflow (disp, 16))
        as_bad (_("pc-relative offset out of range, shall fit in 16 bits"));
      else
        encode_int16 (insn->disp16.X_add_number, bytes + 2);
    }

  if (insn->has_imm64 && insn->imm64.X_op == O_constant)
    {
      uint64_t imm64 = insn->imm64.X_add_number;

      if (target_big_endian)
        {
          bytes[12] = (imm64 >> 56) & 0xff;
          bytes[13] = (imm64 >> 48) & 0xff;
          bytes[14] = (imm64 >> 40) & 0xff;
          bytes[15] = (imm64 >> 32) & 0xff;
          bytes[4] = (imm64 >> 24) & 0xff;
          bytes[5] = (imm64 >> 16) & 0xff;
          bytes[6] = (imm64 >> 8) & 0xff;
          bytes[7] = imm64 & 0xff;
        }
      else
        {
          bytes[15] = (imm64 >> 56) & 0xff;
          bytes[14] = (imm64 >> 48) & 0xff;
          bytes[13] = (imm64 >> 40) & 0xff;
          bytes[12] = (imm64 >> 32) & 0xff;
          bytes[7] = (imm64 >> 24) & 0xff;
          bytes[6] = (imm64 >> 16) & 0xff;
          bytes[5] = (imm64 >> 8) & 0xff;
          bytes[4] = imm64 & 0xff;
        }
    }
}

/* Install the fixups in INSN in their proper location in the
   specified FRAG at the location pointed by WHERE.  */

static void
install_insn_fixups (struct bpf_insn *insn, fragS *frag, long where)
{
  if (insn->has_imm64)
    {
      switch (insn->imm64.X_op)
        {
        case O_symbol:
        case O_subtract:
        case O_add:
          {
            reloc_howto_type *reloc_howto;
            int size;

            reloc_howto = bfd_reloc_type_lookup (stdoutput, BFD_RELOC_BPF_64);
            if (!reloc_howto)
              abort ();

            size = bfd_get_reloc_size (reloc_howto);

            fix_new_exp (frag, where,
                         size, &insn->imm64, reloc_howto->pc_relative,
                         BFD_RELOC_BPF_64);
            break;
          }
        case O_constant:
          /* Already handled in encode_insn.  */
          break;
        default:
          abort ();
        }
    }

  if (insn->has_imm32)
    {
      switch (insn->imm32.X_op)
        {
        case O_symbol:
        case O_subtract:
        case O_add:
        case O_uminus:
          {
            reloc_howto_type *reloc_howto;
            int size;

            reloc_howto = bfd_reloc_type_lookup (stdoutput, BFD_RELOC_32);
            if (!reloc_howto)
              abort ();

            size = bfd_get_reloc_size (reloc_howto);

            fix_new_exp (frag, where + 4,
                         size, &insn->imm32, reloc_howto->pc_relative,
                         BFD_RELOC_32);
            break;
          }
        case O_constant:
          /* Already handled in encode_insn.  */
          break;
        default:
          abort ();
        }
    }

  if (insn->has_disp32)
    {
      switch (insn->disp32.X_op)
        {
        case O_symbol:
        case O_subtract:
        case O_add:
          {
            reloc_howto_type *reloc_howto;
            int size;
            unsigned int bfd_reloc
              = (insn->id == BPF_INSN_CALL
                 ? BFD_RELOC_BPF_DISPCALL32
                 : BFD_RELOC_BPF_DISP32);

            reloc_howto = bfd_reloc_type_lookup (stdoutput, bfd_reloc);
            if (!reloc_howto)
              abort ();

            size = bfd_get_reloc_size (reloc_howto);

            fix_new_exp (frag, where,
                         size, &insn->disp32, reloc_howto->pc_relative,
                         bfd_reloc);
            break;
          }
        case O_constant:
          /* Already handled in encode_insn.  */
          break;
        default:
          abort ();
        }
    }

  if (insn->has_offset16)
    {
      switch (insn->offset16.X_op)
        {
        case O_symbol:
        case O_subtract:
        case O_add:
          {
            reloc_howto_type *reloc_howto;
            int size;

            /* XXX we really need a new pc-rel offset in bytes
               relocation for this.  */
            reloc_howto = bfd_reloc_type_lookup (stdoutput, BFD_RELOC_BPF_DISP16);
            if (!reloc_howto)
              abort ();

            size = bfd_get_reloc_size (reloc_howto);

            fix_new_exp (frag, where,
                         size, &insn->offset16, reloc_howto->pc_relative,
                         BFD_RELOC_BPF_DISP16);
            break;
          }
        case O_constant:
          /* Already handled in encode_insn.  */
          break;
        default:
          abort ();
        }
    }

  if (insn->has_disp16)
    {
      switch (insn->disp16.X_op)
        {
        case O_symbol:
        case O_subtract:
        case O_add:
          {
            reloc_howto_type *reloc_howto;
            int size;

            reloc_howto = bfd_reloc_type_lookup (stdoutput, BFD_RELOC_BPF_DISP16);
            if (!reloc_howto)
              abort ();

            size = bfd_get_reloc_size (reloc_howto);

            fix_new_exp (frag, where,
                         size, &insn->disp16, reloc_howto->pc_relative,
                         BFD_RELOC_BPF_DISP16);
            break;
          }
        case O_constant:
          /* Already handled in encode_insn.  */
          break;
        default:
          abort ();
        }
    }

}

/* Add a new insn to the list of instructions.  */

static void
add_fixed_insn (struct bpf_insn *insn)
{
  char *this_frag = frag_more (insn->size);
  char bytes[16];
  int i;

  /* First encode the known parts of the instruction, including
     opcodes and constant immediates, and write them to the frag.  */
  encode_insn (insn, bytes, 0 /* relax */);
  for (i = 0; i < insn->size; ++i)
    md_number_to_chars (this_frag + i, (valueT) bytes[i], 1);

  /* Now install the instruction fixups.  */
  install_insn_fixups (insn, frag_now,
                       this_frag - frag_now->fr_literal);
}

/* Add a new relaxable to the list of instructions.  */

static void
add_relaxed_insn (struct bpf_insn *insn, expressionS *exp)
{
  char bytes[16];
  int i;
  char *this_frag;
  unsigned worst_case = relaxed_branch_length (NULL, NULL, 0);
  unsigned best_case = insn->size;

  /* We only support relaxing branches, for the moment.  */
  relax_substateT subtype
    = RELAX_BRANCH_ENCODE (insn->id == BPF_INSN_JAR,
                           exp->X_op == O_constant,
                           worst_case);

  frag_grow (worst_case);
  this_frag = frag_more (0);

  /* First encode the known parts of the instruction, including
     opcodes and constant immediates, and write them to the frag.  */
  encode_insn (insn, bytes, 1 /* relax */);
  for (i = 0; i < insn->size; ++i)
    md_number_to_chars (this_frag + i, (valueT) bytes[i], 1);

  /* Note that instruction fixups will be applied once the frag is
     relaxed, in md_convert_frag.  */
  frag_var (rs_machine_dependent,
            worst_case, best_case,
            subtype, exp->X_add_symbol, exp->X_add_number /* offset */,
            NULL);
}


/* Parse an operand expression.  Returns the first character that is
   not part of the expression, or NULL in case of parse error.

   See md_operand below to see how exp_parse_failed is used.  */

static int exp_parse_failed = 0;
static bool parsing_insn_operands = false;

static char *
parse_expression (char *s, expressionS *exp)
{
  char *saved_input_line_pointer = input_line_pointer;
  char *saved_s = s;

  /* Wake up bpf_parse_name before the call to expression ().  */
  parsing_insn_operands = true;

  exp_parse_failed = 0;
  input_line_pointer = s;
  expression (exp);
  s = input_line_pointer;
  input_line_pointer = saved_input_line_pointer;

  switch (exp->X_op == O_absent || exp_parse_failed)
    return NULL;

  /* The expression parser may consume trailing whitespaces.  We have
     to undo that since the instruction templates may be expecting
     these whitespaces.  */
  {
    char *p;
    for (p = s - 1; p >= saved_s && *p == ' '; --p)
      --s;
  }

  return s;
}

/* Parse a BPF register name and return the corresponding register
   number.  Return NULL in case of parse error, or a pointer to the
   first character in S that is not part of the register name.  */

static char *
parse_bpf_register (char *s, char rw, uint8_t *regno)
{
  if (asm_dialect == DIALECT_NORMAL)
    {
      rw = 'r';
      if (*s != '%')
	return NULL;
      s += 1;

      if (*s == 'f' && *(s + 1) == 'p')
	{
	  *regno = 10;
	  s += 2;
	  return s;
	}
    }

  if (*s != rw)
    return NULL;
  s += 1;

  if (*s == '1')
    {
      if (*(s + 1) == '0')
        {
          *regno = 10;
          s += 2;
        }
      else
        {
          *regno = 1;
          s += 1;
        }
    }
  else if (*s >= '0' && *s <= '9')
    {
      *regno = *s - '0';
      s += 1;
    }

  /* If we are still parsing a name, it is not a register.  */
  if (is_part_of_name (*s))
    return NULL;

  return s;
}

/* Symbols created by this parse, but not yet committed to the real
   symbol table.  */
static symbolS *deferred_sym_rootP;
static symbolS *deferred_sym_lastP;

/* Symbols discarded by a previous parse.  Symbols cannot easily be freed
   after creation, so try to recycle.  */
static symbolS *orphan_sym_rootP;
static symbolS *orphan_sym_lastP;

/* Implement md_parse_name hook.  Handles any symbol found in an expression.
   This allows us to tentatively create symbols, before we know for sure
   whether the parser is using the correct template for an instruction.
   If we end up keeping the instruction, the deferred symbols are committed
   to the real symbol table. This approach is modeled after the riscv port.  */

bool
bpf_parse_name (const char *name, expressionS *exp, enum expr_mode mode)
{
  symbolS *sym;

  /* If we aren't currently parsing an instruction, don't do anything.
     This prevents tampering with operands to directives.  */
  if (!parsing_insn_operands)
    return false;

  gas_assert (mode == expr_normal);

  /* Pseudo-C syntax uses unprefixed register names like r2 or w3.
     Since many instructions take either a register or an
     immediate/expression, we should not allow references to symbols
     with these names in operands.  */
  if (asm_dialect == DIALECT_PSEUDOC)
    {
      uint8_t regno;

      if (parse_bpf_register ((char *) name, 'r', &regno)
          || parse_bpf_register ((char *) name, 'w', &regno))
        {
          as_bad (_("unexpected register name `%s' in expression"),
                  name);
          return false;
        }
    }

  if (symbol_find (name) != NULL)
    return false;

  for (sym = deferred_sym_rootP; sym; sym = symbol_next (sym))
    if (strcmp (name, S_GET_NAME (sym)) == 0)
      break;

  /* Tentatively create a symbol.  */
  if (!sym)
    {
      /* See if we can reuse a symbol discarded by a previous parse.
	 This may be quite common, for example when trying multiple templates
	 for an instruction with the first reference to a valid symbol.  */
      for (sym = orphan_sym_rootP; sym; sym = symbol_next (sym))
	if (strcmp (name, S_GET_NAME (sym)) == 0)
	  {
	    symbol_remove (sym, &orphan_sym_rootP, &orphan_sym_lastP);
	    break;
	  }

      if (!sym)
	  sym = symbol_create (name, undefined_section, &zero_address_frag, 0);

      /* Add symbol to the deferred list.  If we commit to the isntruction,
	 then the symbol will be inserted into to the real symbol table at
	 that point (in md_assemble).  */
      symbol_append (sym, deferred_sym_lastP, &deferred_sym_rootP,
		     &deferred_sym_lastP);
    }

  exp->X_op = O_symbol;
  exp->X_add_symbol = sym;
  exp->X_add_number = 0;

  return true;
}

/* Collect a parse error message.  */

static int partial_match_length = 0;
static char *errmsg = NULL;

static void
parse_error (int length, const char *fmt, ...)
{
  if (length > partial_match_length)
    {
      va_list args;

      free (errmsg);
      va_start (args, fmt);
      errmsg = xvasprintf (fmt, args);
      va_end (args);
      partial_match_length = length;
    }

  /* Discard deferred symbols from the failed parse.  They may potentially
     be reused in the future from the orphan list.  */
  while (deferred_sym_rootP)
    {
      symbolS *sym = deferred_sym_rootP;
      symbol_remove (sym, &deferred_sym_rootP, &deferred_sym_lastP);
      symbol_append (sym, orphan_sym_lastP, &orphan_sym_rootP,
		     &orphan_sym_lastP);
    }
}

/* Assemble a machine instruction in STR and emit the frags/bytes it
   assembles to.  */

void
md_assemble (char *str ATTRIBUTE_UNUSED)
{
  /* There are two different syntaxes that can be used to write BPF
     instructions.  One is very conventional and like any other
     assembly language where each instruction is conformed by an
     instruction mnemonic followed by its operands.  This is what we
     call the "normal" syntax.  The other syntax tries to look like C
     statements. We have to support both syntaxes in this assembler.

     One of the many nuisances introduced by this eccentricity is that
     in the pseudo-c syntax it is not possible to hash the opcodes
     table by instruction mnemonic, because there is none.  So we have
     no other choice than to try to parse all instruction opcodes
     until one matches.  This is slow.

     Another problem is that emitting detailed diagnostics becomes
     tricky, since the lack of mnemonic means it is not clear what
     instruction was intended by the user, and we cannot emit
     diagnostics for every attempted template.  So if an instruction
     is not parsed, we report the diagnostic corresponding to the
     partially parsed instruction that was matched further.  */

  unsigned int idx = 0;
  struct bpf_insn insn;
  const struct bpf_opcode *opcode;

  /* Initialize the global diagnostic variables.  See the parse_error
     function above.  */
  partial_match_length = 0;
  errmsg = NULL;

#define PARSE_ERROR(...) parse_error (s - str, __VA_ARGS__)

  while ((opcode = bpf_get_opcode (idx++)) != NULL)
    {
      const char *p;
      char *s;
      const char *template
        = (asm_dialect == DIALECT_PSEUDOC ? opcode->pseudoc : opcode->normal);

      /* Do not try to match opcodes with a higher version than the
         selected ISA spec.  */
      if (opcode->version > isa_spec)
        continue;

      memset (&insn, 0, sizeof (struct bpf_insn));
      insn.size = 8;
      for (s = str, p = template; *p != '\0';)
        {
          if (*p == ' ')
            {
              /* Expect zero or more spaces.  */
              while (*s != '\0' && (*s == ' ' || *s == '\t'))
                s += 1;
              p += 1;
            }
          else if (*p == '%')
            {
              if (*(p + 1) == '%')
                {
                  if (*s != '%')
                    {
                      PARSE_ERROR ("expected '%%'");
                      break;
                    }
                  p += 2;
                  s += 1;
                }
              else if (*(p + 1) == 'w')
                {
                  /* Expect zero or more spaces.  */
                  while (*s != '\0' && (*s == ' ' || *s == '\t'))
                    s += 1;
                  p += 2;
                }
              else if (*(p + 1) == 'W')
                {
                  /* Expect one or more spaces.  */
                  if (*s != ' ' && *s != '\t')
                    {
                      PARSE_ERROR ("expected white space, got '%s'",
                                   s);
                      break;
                    }
                  while (*s != '\0' && (*s == ' ' || *s == '\t'))
                    s += 1;
                  p += 2;
                }
              else if (strncmp (p, "%dr", 3) == 0)
                {
                  uint8_t regno;
                  char *news = parse_bpf_register (s, 'r', &regno);

                  if (news == NULL || (insn.has_dst && regno != insn.dst))
                    {
                      if (news != NULL)
                        PARSE_ERROR ("expected register r%d, got r%d",
                                     insn.dst, regno);
                      else
                        PARSE_ERROR ("expected register name, got '%s'", s);
                      break;
                    }
                  s = news;
                  insn.dst = regno;
                  insn.has_dst = 1;
                  p += 3;
                }
              else if (strncmp (p, "%sr", 3) == 0)
                {
                  uint8_t regno;
                  char *news = parse_bpf_register (s, 'r', &regno);

                  if (news == NULL || (insn.has_src && regno != insn.src))
                    {
                      if (news != NULL)
                        PARSE_ERROR ("expected register r%d, got r%d",
                                     insn.dst, regno);
                      else
                        PARSE_ERROR ("expected register name, got '%s'", s);
                      break;
                    }
                  s = news;
                  insn.src = regno;
                  insn.has_src = 1;
                  p += 3;
                }
              else if (strncmp (p, "%dw", 3) == 0)
                {
                  uint8_t regno;
                  char *news = parse_bpf_register (s, 'w', &regno);

                  if (news == NULL || (insn.has_dst && regno != insn.dst))
                    {
                      if (news != NULL)
                        PARSE_ERROR ("expected register r%d, got r%d",
                                     insn.dst, regno);
                      else
                        PARSE_ERROR ("expected register name, got '%s'", s);
                      break;
                    }
                  s = news;
                  insn.dst = regno;
                  insn.has_dst = 1;
                  p += 3;
                }
              else if (strncmp (p, "%sw", 3) == 0)
                {
                  uint8_t regno;
                  char *news = parse_bpf_register (s, 'w', &regno);

                  if (news == NULL || (insn.has_src && regno != insn.src))
                    {
                      if (news != NULL)
                        PARSE_ERROR ("expected register r%d, got r%d",
                                     insn.dst, regno);
                      else
                        PARSE_ERROR ("expected register name, got '%s'", s);
                      break;
                    }
                  s = news;
                  insn.src = regno;
                  insn.has_src = 1;
                  p += 3;
                }
              else if (strncmp (p, "%i32", 4) == 0
                       || strncmp (p, "%I32", 4) == 0)
                {
                  if (p[1] == 'I')
                    {
                      while (*s == ' ' || *s == '\t')
                        s += 1;
                      if (*s != '+' && *s != '-')
                        {
                          PARSE_ERROR ("expected `+' or `-', got `%c'", *s);
                          break;
                        }
                    }

                  s = parse_expression (s, &insn.imm32);
                  if (s == NULL)
                    {
                      PARSE_ERROR ("expected signed 32-bit immediate");
                      break;
                    }
                  insn.has_imm32 = 1;
                  p += 4;
                }
              else if (strncmp (p, "%o16", 4) == 0)
                {
                  while (*s == ' ' || *s == '\t')
                    s += 1;
                  if (*s != '+' && *s != '-')
                    {
                      PARSE_ERROR ("expected `+' or `-', got `%c'", *s);
                      break;
                    }

                  s = parse_expression (s, &insn.offset16);
                  if (s == NULL)
                    {
                      PARSE_ERROR ("expected signed 16-bit offset");
                      break;
                    }
                  insn.has_offset16 = 1;
                  p += 4;
                }
              else if (strncmp (p, "%d16", 4) == 0)
                {
                  s = parse_expression (s, &insn.disp16);
                  if (s == NULL)
                    {
                      PARSE_ERROR ("expected signed 16-bit displacement");
                      break;
                    }
                  insn.has_disp16 = 1;
                  insn.is_relaxable = (insn.disp16.X_op != O_constant);
                  p += 4;
                }
              else if (strncmp (p, "%d32", 4) == 0)
                {
                  s = parse_expression (s, &insn.disp32);
                  if (s == NULL)
                    {
                      PARSE_ERROR ("expected signed 32-bit displacement");
                      break;
                    }
                  insn.has_disp32 = 1;
                  p += 4;
                }
              else if (strncmp (p, "%i64", 4) == 0)
                {
                  s = parse_expression (s, &insn.imm64);
                  if (s == NULL)
                    {
                      PARSE_ERROR ("expected signed 64-bit immediate");
                      break;
                    }
                  insn.has_imm64 = 1;
                  insn.size = 16;
                  p += 4;
                }
              else
                as_fatal (_("invalid %%-tag in BPF opcode '%s'\n"), template);
            }
          else
            {
              /* Match a literal character.  */
              if (*s != *p)
                {
                  if (*s == '\0')
                    PARSE_ERROR ("expected '%c'", *p);
                  else if (*s == '%')
                    {
                      /* This is to workaround a bug in as_bad. */
                      char tmp[3];

                      tmp[0] = '%';
                      tmp[1] = '%';
                      tmp[2] = '\0';

                      PARSE_ERROR ("expected '%c', got '%s'", *p, tmp);
                    }
                  else
                    PARSE_ERROR ("expected '%c', got '%c'", *p, *s);
                  break;
                }
              p += 1;
              s += 1;
            }
        }

      if (*p == '\0')
        {
          /* Allow white spaces at the end of the line.  */
          while (*s != '\0' && (*s == ' ' || *s == '\t'))
            s += 1;
          if (*s == '\0')
            /* We parsed an instruction successfully.  */
            break;
          PARSE_ERROR ("extra junk at end of line");
        }
    }

  /* Mark that we are no longer parsing an instruction, bpf_parse_name does
     not interfere with symbols in e.g. assembler directives.  */
  parsing_insn_operands = false;

  if (opcode == NULL)
    {
      as_bad (_("unrecognized instruction `%s'"), str);
      if (errmsg != NULL)
        {
          as_bad ("%s", errmsg);
          free (errmsg);
        }

      return;
    }
  insn.id = opcode->id;
  insn.opcode = opcode->opcode;

#undef PARSE_ERROR

  /* Commit any symbols created while parsing the instruction.  */
  while (deferred_sym_rootP)
    {
      symbolS *sym = deferred_sym_rootP;
      symbol_remove (sym, &deferred_sym_rootP, &deferred_sym_lastP);
      symbol_append (sym, symbol_lastP, &symbol_rootP, &symbol_lastP);
      symbol_table_insert (sym);
    }

  /* Generate the frags and fixups for the parsed instruction.  */
  if (do_relax && isa_spec >= BPF_V4 && insn.is_relaxable)
    {
      expressionS *relaxable_exp = NULL;

      if (insn.has_disp16)
        relaxable_exp = &insn.disp16;
      else
        abort ();

      add_relaxed_insn (&insn, relaxable_exp);
    }
  else
    add_fixed_insn (&insn);

  /* Emit DWARF2 debugging information.  */
  dwarf2_emit_insn (insn.size);
}

/* Parse an operand that is machine-specific.  */

void
md_operand (expressionS *expressionP)
{
  /* If this hook is invoked it means GAS failed to parse a generic
     expression.  We should inhibit the as_bad in expr.c, so we can
     fail while parsing instruction alternatives.  To do that, we
     change the expression to not have an O_absent.  But then we also
     need to set exp_parse_failed to parse_expression above does the
     right thing.  */
  ++input_line_pointer;
  expressionP->X_op = O_constant;
  expressionP->X_add_number = 0;
  exp_parse_failed = 1;
}

symbolS *
md_undefined_symbol (char *name ATTRIBUTE_UNUSED)
{
  return NULL;
}


/* Turn a string in input_line_pointer into a floating point constant
   of type TYPE, and store the appropriate bytes in *LITP.  The number
   of LITTLENUMS emitted is stored in *SIZEP.  An error message is
   returned, or NULL on OK.  */

const char *
md_atof (int type, char *litP, int *sizeP)
{
  return ieee_md_atof (type, litP, sizeP, false);
}


/* Determine whether the equal sign in the given string corresponds to
   a BPF instruction, i.e. when it is not to be considered a symbol
   assignment.  */

bool
bpf_tc_equal_in_insn (int c ATTRIBUTE_UNUSED, char *str ATTRIBUTE_UNUSED)
{
  uint8_t regno;

  /* Only pseudo-c instructions can have equal signs, and of these,
     all that could be confused with a symbol assignment all start
     with a register name.  */
  if (asm_dialect == DIALECT_PSEUDOC)
    {
      char *w = parse_bpf_register (str, 'w', &regno);
      char *r = parse_bpf_register (str, 'r', &regno);

      if ((w != NULL && *w == '\0')
          || (r != NULL && *r == '\0'))
        return 1;
    }

  return 0;
}

/* Some special processing for a BPF ELF file.  */

void
bpf_elf_final_processing (void)
{
  /* Annotate the BPF ISA version in the ELF flag bits.  */
  elf_elfheader (stdoutput)->e_flags |= (isa_spec & EF_BPF_CPUVER);
}
