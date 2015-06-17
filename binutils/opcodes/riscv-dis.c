/* RISC-V disassembler
   Copyright 2011-2014 Free Software Foundation, Inc.

   Contributed by Andrew Waterman (waterman@cs.berkeley.edu) at UC Berkeley.
   Based on MIPS target.

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
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#include "sysdep.h"
#include "dis-asm.h"
#include "libiberty.h"
#include "opcode/riscv.h"
#include "opintl.h"
#include "elf-bfd.h"
#include "elf/riscv.h"

#include <stdint.h>
#include <ctype.h>

struct riscv_private_data
{
  bfd_vma gp;
  bfd_vma print_addr;
  bfd_vma hi_addr[OP_MASK_RD + 1];
};

static const char * const *riscv_gpr_names;
static const char * const *riscv_fpr_names;

/* Other options */
static int no_aliases;	/* If set disassemble as most general inst.  */

static void
set_default_riscv_dis_options (void)
{
  riscv_gpr_names = riscv_gpr_names_abi;
  riscv_fpr_names = riscv_fpr_names_abi;
  no_aliases = 0;
}

static void
parse_riscv_dis_option (const char *option)
{
  if (CONST_STRNEQ (option, "no-aliases"))
    no_aliases = 1;
  else if (CONST_STRNEQ (option, "numeric"))
    {
      riscv_gpr_names = riscv_gpr_names_numeric;
      riscv_fpr_names = riscv_fpr_names_numeric;
    }
  else
    {
      /* Invalid option.  */
      fprintf (stderr, _("Unrecognized disassembler option: %s\n"), option);
    }
}

static void
parse_riscv_dis_options (const char *opts_in)
{
  char *opts = xstrdup (opts_in), *opt = opts, *opt_end = opts;

  set_default_riscv_dis_options ();

  for ( ; opt_end != NULL; opt = opt_end + 1)
    {
      if ((opt_end = strchr (opt, ',')) != NULL)
	*opt_end = 0;
      parse_riscv_dis_option (opt);
    }

  free (opts);
}

/* Print one argument from an array. */

static void
arg_print (struct disassemble_info *info, unsigned long val,
	   const char* const* array, size_t size)
{
  const char *s = val >= size || array[val] == NULL ? "unknown" : array[val];
  (*info->fprintf_func) (info->stream, "%s", s);
}

static void
maybe_print_address (struct riscv_private_data *pd, int base_reg, int offset)
{
  if (pd->hi_addr[base_reg] != (bfd_vma)-1)
    {
      pd->print_addr = pd->hi_addr[base_reg] + offset;
      pd->hi_addr[base_reg] = -1;
    }
  else if (base_reg == X_GP && pd->gp != (bfd_vma)-1)
    pd->print_addr = pd->gp + offset;
  else if (base_reg == X_TP)
    pd->print_addr = offset;
}

/* Print insn arguments for 32/64-bit code.  */

static void
print_insn_args (const char *d, insn_t l, bfd_vma pc, disassemble_info *info)
{
  struct riscv_private_data *pd = info->private_data;
  int rs1 = (l >> OP_SH_RS1) & OP_MASK_RS1;
  int rd = (l >> OP_SH_RD) & OP_MASK_RD;
  fprintf_ftype print = info->fprintf_func;

  if (*d != '\0')
    print (info->stream, "\t");

  for (; *d != '\0'; d++)
    {
      switch (*d)
	{
        /* Xcustom */
        case '^':
          switch (*++d)
            {
            case 'd':
              (*info->fprintf_func) (info->stream, "%d", rd);
              break;
            case 's':
              (*info->fprintf_func) (info->stream, "%d", rs1);
              break;
            case 't':
              (*info->fprintf_func)
                ( info->stream, "%d", (int)((l >> OP_SH_RS2) & OP_MASK_RS2));
              break;
            case 'j':
              (*info->fprintf_func)
                ( info->stream, "%d", (int)((l >> OP_SH_CUSTOM_IMM) & OP_MASK_CUSTOM_IMM));
              break;
            }
          break;

        /* Xhwacha */
        case '#':
          switch ( *++d ) {
            case 'g':
              (*info->fprintf_func)
                ( info->stream, "%d",
                  (int)((l >> OP_SH_IMMNGPR) & OP_MASK_IMMNGPR));
              break;
            case 'f':
              (*info->fprintf_func)
                ( info->stream, "%d",
                  (int)((l >> OP_SH_IMMNFPR) & OP_MASK_IMMNFPR));
              break;
            case 'p':
              (*info->fprintf_func)
                ( info->stream, "%d",
                 (int)((l >> OP_SH_CUSTOM_IMM) & OP_MASK_CUSTOM_IMM));
              break;
            case 'n':
              (*info->fprintf_func)
                ( info->stream, "%d",
                  (int)(((l >> OP_SH_IMMSEGNELM) & OP_MASK_IMMSEGNELM) + 1));
              break;
            case 'd':
              (*info->fprintf_func)
                ( info->stream, "%s",
                  riscv_vec_gpr_names[(l >> OP_SH_VRD) & OP_MASK_VRD]);
              break;
            case 's':
              (*info->fprintf_func)
                ( info->stream, "%s",
                  riscv_vec_gpr_names[(l >> OP_SH_VRS) & OP_MASK_VRS]);
              break;
            case 't':
              (*info->fprintf_func)
                ( info->stream, "%s",
                  riscv_vec_gpr_names[(l >> OP_SH_VRT) & OP_MASK_VRT]);
              break;
            case 'r':
              (*info->fprintf_func)
                ( info->stream, "%s",
                  riscv_vec_gpr_names[(l >> OP_SH_VRR) & OP_MASK_VRR]);
              break;
            case 'D':
              (*info->fprintf_func)
                ( info->stream, "%s",
                  riscv_vec_fpr_names[(l >> OP_SH_VFD) & OP_MASK_VFD]);
              break;
            case 'S':
              (*info->fprintf_func)
                ( info->stream, "%s",
                  riscv_vec_fpr_names[(l >> OP_SH_VFS) & OP_MASK_VFS]);
              break;
            case 'T':
              (*info->fprintf_func)
                ( info->stream, "%s",
                  riscv_vec_fpr_names[(l >> OP_SH_VFT) & OP_MASK_VFT]);
              break;
            case 'R':
              (*info->fprintf_func)
                ( info->stream, "%s",
                  riscv_vec_fpr_names[(l >> OP_SH_VFR) & OP_MASK_VFR]);
              break;
          }
          break;

	case 'C': /* RVC */
	  switch (*++d)
	    {
	    case 'd': /* RD x8-x15 */
	      print (info->stream, "%s",
		     riscv_gpr_names[((l >> OP_SH_CRDS) & OP_MASK_CRDS) + 8]);
	      break;
	    case 's': /* RS1 x8-x15 */
	    case 'w': /* RS1 x8-x15 */
	      print (info->stream, "%s",
		     riscv_gpr_names[((l >> OP_SH_CRS1S) & OP_MASK_CRS1S) + 8]);
	      break;
	    case 't': /* RS2 x8-x15 */
	    case 'x': /* RS2 x8-x15 */
	      print (info->stream, "%s",
		     riscv_gpr_names[((l >> OP_SH_CRS2S) & OP_MASK_CRS2S) + 8]);
	      break;
	    case 'U': /* RS1, constrained to equal RD */
	    case 'D': /* RS1 or RD, nonzero */
	      print (info->stream, "%s", riscv_gpr_names[rd]);
	      break;
	    case 'c': /* RS1, constrained to equal sp */
	      print (info->stream, "%s", riscv_gpr_names[X_SP]);
	      continue;
	    case 'T': /* RS2, nonzero */
	    case 'V': /* RS2 */
	      print (info->stream, "%s",
		     riscv_gpr_names[(l >> OP_SH_CRS2) & OP_MASK_CRS2]);
	      continue;
	    case 'i':
	      print (info->stream, "%d", (int)EXTRACT_RVC_SIMM3 (l));
	      break;
	    case 'j':
	      print (info->stream, "%d", (int)EXTRACT_RVC_IMM (l));
	      break;
	    case 'k':
	      print (info->stream, "%d", (int)EXTRACT_RVC_LW_IMM (l));
	      break;
	    case 'l':
	      print (info->stream, "%d", (int)EXTRACT_RVC_LD_IMM (l));
	      break;
	    case 'm':
	      print (info->stream, "%d", (int)EXTRACT_RVC_LWSP_IMM (l));
	      break;
	    case 'n':
	      print (info->stream, "%d", (int)EXTRACT_RVC_LDSP_IMM (l));
	      break;
	    case 'K':
	      print (info->stream, "%d", (int)EXTRACT_RVC_ADDI4SPN_IMM (l));
	      break;
	    case 'L':
	      print (info->stream, "%d", (int)EXTRACT_RVC_ADDI16SP_IMM (l));
	      break;
	    case 'M':
	      print (info->stream, "%d", (int)EXTRACT_RVC_SWSP_IMM (l));
	      break;
	    case 'N':
	      print (info->stream, "%d", (int)EXTRACT_RVC_SDSP_IMM (l));
	      break;
	    case 'p':
	      info->target = EXTRACT_RVC_B_IMM (l) + pc;
	      (*info->print_address_func) (info->target, info);
	      break;
	    case 'a':
	      info->target = EXTRACT_RVC_J_IMM (l) + pc;
	      (*info->print_address_func) (info->target, info);
	      break;
	    case 'u':
	      print (info->stream, "0x%x",
		     (int)(EXTRACT_RVC_IMM (l) & (RISCV_BIGIMM_REACH-1)));
	      break;
	    case '>':
	      print (info->stream, "0x%x", (int)EXTRACT_RVC_IMM (l) & 0x3f);
	      break;
	    case '<':
	      print (info->stream, "0x%x", (int)EXTRACT_RVC_IMM (l) & 0x1f);
	      break;
	    }
	  break;

	case ',':
	case '(':
	case ')':
	case '[':
	case ']':
	  (*info->fprintf_func) (info->stream, "%c", *d);
	  break;

	case '0':
	  /* Only print constant 0 if it is the last argument */
	  if (!d[1])
	    print (info->stream, "0");
	  break;

	case 'b':
	case 's':
	  (*info->fprintf_func) (info->stream, "%s", riscv_gpr_names[rs1]);
	  break;

	case 't':
	  (*info->fprintf_func) (info->stream, "%s",
				 riscv_gpr_names[(l >> OP_SH_RS2) & OP_MASK_RS2]);
	  break;

	case 'u':
	  (*info->fprintf_func) (info->stream, "0x%x", (unsigned)EXTRACT_UTYPE_IMM (l) >> RISCV_IMM_BITS);
	  break;

	case 'm':
	  arg_print(info, (l >> OP_SH_RM) & OP_MASK_RM,
		    riscv_rm, ARRAY_SIZE(riscv_rm));
	  break;

	case 'P':
	  arg_print(info, (l >> OP_SH_PRED) & OP_MASK_PRED,
	            riscv_pred_succ, ARRAY_SIZE(riscv_pred_succ));
	  break;

	case 'Q':
	  arg_print(info, (l >> OP_SH_SUCC) & OP_MASK_SUCC,
	            riscv_pred_succ, ARRAY_SIZE(riscv_pred_succ));
	  break;

	case 'o':
	  maybe_print_address (pd, rs1, EXTRACT_ITYPE_IMM (l));
	case 'j':
	  if ((l & MASK_ADDI) == MATCH_ADDI || (l & MASK_JALR) == MATCH_JALR)
	    maybe_print_address (pd, rs1, EXTRACT_ITYPE_IMM (l));
	  (*info->fprintf_func) (info->stream, "%d", (int)EXTRACT_ITYPE_IMM (l));
	  break;

	case 'q':
	  maybe_print_address (pd, rs1, EXTRACT_STYPE_IMM (l));
	  (*info->fprintf_func) (info->stream, "%d", (int)EXTRACT_STYPE_IMM (l));
	  break;

	case 'a':
	  info->target = EXTRACT_UJTYPE_IMM (l) + pc;
	  (*info->print_address_func) (info->target, info);
	  break;

	case 'p':
	  info->target = EXTRACT_SBTYPE_IMM (l) + pc;
	  (*info->print_address_func) (info->target, info);
	  break;

	case 'd':
	  if ((l & MASK_AUIPC) == MATCH_AUIPC)
	    pd->hi_addr[rd] = pc + EXTRACT_UTYPE_IMM (l);
	  else if ((l & MASK_LUI) == MATCH_LUI)
	    pd->hi_addr[rd] = EXTRACT_UTYPE_IMM (l);
	  (*info->fprintf_func) (info->stream, "%s", riscv_gpr_names[rd]);
	  break;

	case 'z':
	  (*info->fprintf_func) (info->stream, "%s", riscv_gpr_names[0]);
	  break;

	case '>':
	  (*info->fprintf_func) (info->stream, "0x%x",
				 (unsigned)((l >> OP_SH_SHAMT) & OP_MASK_SHAMT));
	  break;

	case '<':
	  (*info->fprintf_func) (info->stream, "0x%x",
				 (unsigned)((l >> OP_SH_SHAMTW) & OP_MASK_SHAMTW));
	  break;

	case 'S':
	case 'U':
	  (*info->fprintf_func) (info->stream, "%s", riscv_fpr_names[rs1]);
	  break;

	case 'T':
	  (*info->fprintf_func) (info->stream, "%s",
				 riscv_fpr_names[(l >> OP_SH_RS2) & OP_MASK_RS2]);
	  break;

	case 'D':
	  (*info->fprintf_func) (info->stream, "%s", riscv_fpr_names[rd]);
	  break;

	case 'R':
	  (*info->fprintf_func) (info->stream, "%s",
				 riscv_fpr_names[(l >> OP_SH_RS3) & OP_MASK_RS3]);
	  break;

	case 'E':
	  {
	    const char* csr_name = NULL;
	    unsigned int csr = (l >> OP_SH_CSR) & OP_MASK_CSR;
	    switch (csr)
	      {
		#define DECLARE_CSR(name, num) case num: csr_name = #name; break;
		#include "opcode/riscv-opc.h"
		#undef DECLARE_CSR
	      }
	    if (csr_name)
	      (*info->fprintf_func) (info->stream, "%s", csr_name);
	    else
	      (*info->fprintf_func) (info->stream, "0x%x", csr);
	    break;
	  }

	case 'Z':
	  (*info->fprintf_func) (info->stream, "%d", rs1);
	  break;

	default:
	  /* xgettext:c-format */
	  (*info->fprintf_func) (info->stream,
				 _("# internal error, undefined modifier (%c)"),
				 *d);
	  return;
	}
    }
}

/* Print the RISC-V instruction at address MEMADDR in debugged memory,
   on using INFO.  Returns length of the instruction, in bytes.
   BIGENDIAN must be 1 if this is big-endian code, 0 if
   this is little-endian code.  */

static int
riscv_disassemble_insn (bfd_vma memaddr, insn_t word, disassemble_info *info)
{
  const struct riscv_opcode *op;
  static bfd_boolean init = 0;
  static const struct riscv_opcode *riscv_hash[OP_MASK_OP + 1];
  struct riscv_private_data *pd;
  int insnlen;

#define OP_HASH_IDX(i) ((i) & (riscv_insn_length (i) == 2 ? 0x3 : 0x7f))

  /* Build a hash table to shorten the search time.  */
  if (! init)
    {
      for (op = riscv_opcodes; op < &riscv_opcodes[NUMOPCODES]; op++)
        {
	  if (!riscv_hash[OP_HASH_IDX (op->match)])
	    riscv_hash[OP_HASH_IDX (op->match)] = op;
        }

      init = 1;
    }

  if (info->private_data == NULL)
    {
      int i;

      pd = info->private_data = xcalloc (1, sizeof (struct riscv_private_data));
      pd->gp = -1;
      pd->print_addr = -1;
      for (i = 0; i < (int) ARRAY_SIZE(pd->hi_addr); i++)
	pd->hi_addr[i] = -1;

      for (i = 0; i < info->symtab_size; i++)
	if (strcmp (bfd_asymbol_name (info->symtab[i]), "_gp") == 0)
	  pd->gp = bfd_asymbol_value (info->symtab[i]);
    }
  else
    pd = info->private_data;

  insnlen = riscv_insn_length (word);

  info->bytes_per_chunk = insnlen % 4 == 0 ? 4 : 2;
  info->bytes_per_line = 8;
  info->display_endian = info->endian;
  info->insn_info_valid = 1;
  info->branch_delay_insns = 0;
  info->data_size = 0;
  info->insn_type = dis_nonbranch;
  info->target = 0;
  info->target2 = 0;

  op = riscv_hash[OP_HASH_IDX (word)];
  if (op != NULL)
    {
      const char *extension = NULL;
      int xlen = 0;

      /* The incoming section might not always be complete.  */
      if (info->section != NULL)
	{
	  Elf_Internal_Ehdr *ehdr = elf_elfheader (info->section->owner);
	  unsigned int e_flags = ehdr->e_flags;
	  extension = riscv_elf_flag_to_name (EF_GET_RISCV_EXT (e_flags));

	  xlen = 32;
	  if (ehdr->e_ident[EI_CLASS] == ELFCLASS64)
	    xlen = 64;
	}

      for (; op < &riscv_opcodes[NUMOPCODES]; op++)
	{
	  if ((op->match_func) (op, word)
	      && !(no_aliases && (op->pinfo & INSN_ALIAS))
	      && !(op->subset[0] == 'X' && extension != NULL
		   && strcmp (op->subset, extension))
	      && !(isdigit(op->subset[0]) && atoi(op->subset) != xlen))
	    {
	      (*info->fprintf_func) (info->stream, "%s", op->name);
	      print_insn_args (op->args, word, memaddr, info);
	      if (pd->print_addr != (bfd_vma)-1)
		{
		  info->target = pd->print_addr;
		  (*info->fprintf_func) (info->stream, " # ");
		  (*info->print_address_func) (info->target, info);
		  pd->print_addr = -1;
		}
	      return insnlen;
	    }
	}
    }

  /* Handle undefined instructions.  */
  info->insn_type = dis_noninsn;
  (*info->fprintf_func) (info->stream, "0x%llx", (unsigned long long)word);
  return insnlen;
}

int
print_insn_riscv (bfd_vma memaddr, struct disassemble_info *info)
{
  uint16_t i2;
  insn_t insn = 0;
  bfd_vma n;
  int status;

  if (info->disassembler_options != NULL)
    {
      parse_riscv_dis_options (info->disassembler_options);
      /* Avoid repeatedly parsing the options.  */
      info->disassembler_options = NULL;
    }
  else if (riscv_gpr_names == NULL)
    set_default_riscv_dis_options ();

  /* Instructions are a sequence of 2-byte packets in little-endian order.  */
  for (n = 0; n < sizeof(insn) && n < riscv_insn_length (insn); n += 2)
    {
      status = (*info->read_memory_func) (memaddr + n, (bfd_byte*)&i2, 2, info);
      if (status != 0)
	{
	  if (n > 0) /* Don't fail just because we fell off the end. */
	    break;
	  (*info->memory_error_func) (status, memaddr, info);
	  return status;
	}

      i2 = bfd_getl16 (&i2);
      insn |= (insn_t)i2 << (8*n);
    }

  return riscv_disassemble_insn (memaddr, insn, info);
}

void
print_riscv_disassembler_options (FILE *stream)
{
  fprintf (stream, _("\n\
The following RISC-V-specific disassembler options are supported for use\n\
with the -M switch (multiple options should be separated by commas):\n"));

  fprintf (stream, _("\n\
  numeric       Print numeric reigster names, rather than ABI names.\n"));

  fprintf (stream, _("\n\
  no-aliases    Disassemble only into canonical instructions, rather\n\
                than into pseudoinstructions.\n"));

  fprintf (stream, _("\n"));
}
