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
#include <assert.h>

/* FIXME: These should be shared with gdb somehow.  */

static const char * const riscv_gpr_names_numeric[32] =
{
  "x0",   "x1",   "x2",   "x3",   "x4",   "x5",   "x6",   "x7",
  "x8",   "x9",   "x10",  "x11",  "x12",  "x13",  "x14",  "x15",
  "x16",  "x17",  "x18",  "x19",  "x20",  "x21",  "x22",  "x23",
  "x24",  "x25",  "x26",  "x27",  "x28",  "x29",  "x30",  "x31"
};

static const char * const riscv_gpr_names_abi[32] = {
  "zero", "ra", "s0", "s1",  "s2",  "s3",  "s4",  "s5",
  "s6",   "s7", "s8", "s9", "s10", "s11",  "sp",  "tp",
  "v0",   "v1", "a0", "a1",  "a2",  "a3",  "a4",  "a5",
  "a6",   "a7", "t0", "t1",  "t2",  "t3",  "t4",  "gp"
};


static const char * const riscv_fpr_names_numeric[32] =
{
  "f0",   "f1",   "f2",   "f3",   "f4",   "f5",   "f6",   "f7",
  "f8",   "f9",   "f10",  "f11",  "f12",  "f13",  "f14",  "f15",
  "f16",  "f17",  "f18",  "f19",  "f20",  "f21",  "f22",  "f23",
  "f24",  "f25",  "f26",  "f27",  "f28",  "f29",  "f30",  "f31"
};

static const char * const riscv_fpr_names_abi[32] = {
  "fs0", "fs1",  "fs2",  "fs3",  "fs4",  "fs5",  "fs6",  "fs7",
  "fs8", "fs9", "fs10", "fs11", "fs12", "fs13", "fs14", "fs15",
  "fv0", "fv1", "fa0",   "fa1",  "fa2",  "fa3",  "fa4",  "fa5",
  "fa6", "fa7", "ft0",   "ft1",  "ft2",  "ft3",  "ft4",  "ft5"
};

static const char * const riscv_vgr_reg_names_riscv[32] =
{
  "vx0",  "vx1",  "vx2",  "vx3",  "vx4",  "vx5",  "vx6",  "vx7",
  "vx8",  "vx9",  "vx10", "vx11", "vx12", "vx13", "vx14", "vx15",
  "vx16", "vx17", "vx18", "vx19", "vx20", "vx21", "vx22", "vx23",
  "vx24", "vx25", "vx26", "vx27", "vx28", "vx29", "vx30", "vx31"
};

static const char * const riscv_vfp_reg_names_riscv[32] =
{
  "vf0",  "vf1",  "vf2",  "vf3",  "vf4",  "vf5",  "vf6",  "vf7",
  "vf8",  "vf9",  "vf10", "vf11", "vf12", "vf13", "vf14", "vf15",
  "vf16", "vf17", "vf18", "vf19", "vf20", "vf21", "vf22", "vf23",
  "vf24", "vf25", "vf26", "vf27", "vf28", "vf29", "vf30", "vf31"
};

struct riscv_abi_choice
{
  const char * name;
  const char * const *gpr_names;
  const char * const *fpr_names;
};

struct riscv_abi_choice riscv_abi_choices[] =
{
  { "numeric", riscv_gpr_names_numeric, riscv_fpr_names_numeric },
  { "32", riscv_gpr_names_abi, riscv_fpr_names_abi },
  { "64", riscv_gpr_names_abi, riscv_fpr_names_abi },
};

struct riscv_arch_choice
{
  const char *name;
  int bfd_mach_valid;
  unsigned long bfd_mach;
};

const struct riscv_arch_choice riscv_arch_choices[] =
{
  { "numeric",	0, 0 },
  { "rv32",	1, bfd_mach_riscv32 },
  { "rv64",	1, bfd_mach_riscv64 },
};

struct riscv_private_data
{
  bfd_vma gp;
  bfd_vma print_addr;
  bfd_vma hi_addr[OP_MASK_RD + 1];
};

/* ISA and processor type to disassemble for, and register names to use.
   set_default_riscv_dis_options and parse_riscv_dis_options fill in these
   values.  */
static const char * const *riscv_gpr_names;
static const char * const *riscv_fpr_names;

/* Other options */
static int no_aliases;	/* If set disassemble as most general inst.  */

static const struct riscv_abi_choice *
choose_abi_by_name (const char *name, unsigned int namelen)
{
  const struct riscv_abi_choice *c;
  unsigned int i;

  for (i = 0, c = NULL; i < ARRAY_SIZE (riscv_abi_choices) && c == NULL; i++)
    if (strncmp (riscv_abi_choices[i].name, name, namelen) == 0
	&& strlen (riscv_abi_choices[i].name) == namelen)
      c = &riscv_abi_choices[i];

  return c;
}

static void
set_default_riscv_dis_options (struct disassemble_info *info ATTRIBUTE_UNUSED)
{
  riscv_gpr_names = riscv_gpr_names_abi;
  riscv_fpr_names = riscv_fpr_names_abi;
  no_aliases = 0;
}

static void
parse_riscv_dis_option (const char *option, unsigned int len)
{
  unsigned int i, optionlen, vallen;
  const char *val;
  const struct riscv_abi_choice *chosen_abi;

  /* Try to match options that are simple flags */
  if (CONST_STRNEQ (option, "no-aliases"))
    {
      no_aliases = 1;
      return;
    }
  
  /* Look for the = that delimits the end of the option name.  */
  for (i = 0; i < len; i++)
    if (option[i] == '=')
      break;

  if (i == 0)		/* Invalid option: no name before '='.  */
    return;
  if (i == len)		/* Invalid option: no '='.  */
    return;
  if (i == (len - 1))	/* Invalid option: no value after '='.  */
    return;

  optionlen = i;
  val = option + (optionlen + 1);
  vallen = len - (optionlen + 1);

  if (strncmp ("gpr-names", option, optionlen) == 0
      && strlen ("gpr-names") == optionlen)
    {
      chosen_abi = choose_abi_by_name (val, vallen);
      if (chosen_abi != NULL)
	riscv_gpr_names = chosen_abi->gpr_names;
      return;
    }

  if (strncmp ("fpr-names", option, optionlen) == 0
      && strlen ("fpr-names") == optionlen)
    {
      chosen_abi = choose_abi_by_name (val, vallen);
      if (chosen_abi != NULL)
	riscv_fpr_names = chosen_abi->fpr_names;
      return;
    }

  /* Invalid option.  */
}

static void
parse_riscv_dis_options (const char *options)
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

      parse_riscv_dis_option (options, option_end - options);

      /* Go on to the next one.  If option_end points to a comma, it
	 will be skipped above.  */
      options = option_end;
    }
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
  else if (base_reg == GP_REG && pd->gp != (bfd_vma)-1)
    pd->print_addr = pd->gp + offset;
  else if (base_reg == TP_REG)
    pd->print_addr = offset;
}

/* Print insn arguments for 32/64-bit code.  */

static void
print_insn_args (const char *d, insn_t l, bfd_vma pc, disassemble_info *info)
{
  struct riscv_private_data *pd = info->private_data;
  int rs1 = (l >> OP_SH_RS1) & OP_MASK_RS1;
  int rd = (l >> OP_SH_RD) & OP_MASK_RD;

  if (*d != '\0')
    (*info->fprintf_func) (info->stream, "\t");

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
                  riscv_vgr_reg_names_riscv[(l >> OP_SH_VRD) & OP_MASK_VRD]);
              break;
            case 's':
              (*info->fprintf_func)
                ( info->stream, "%s",
                  riscv_vgr_reg_names_riscv[(l >> OP_SH_VRS) & OP_MASK_VRS]);
              break;
            case 't':
              (*info->fprintf_func)
                ( info->stream, "%s",
                  riscv_vgr_reg_names_riscv[(l >> OP_SH_VRT) & OP_MASK_VRT]);
              break;
            case 'r':
              (*info->fprintf_func)
                ( info->stream, "%s",
                  riscv_vgr_reg_names_riscv[(l >> OP_SH_VRR) & OP_MASK_VRR]);
              break;
            case 'D':
              (*info->fprintf_func)
                ( info->stream, "%s",
                  riscv_vfp_reg_names_riscv[(l >> OP_SH_VFD) & OP_MASK_VFD]);
              break;
            case 'S':
              (*info->fprintf_func)
                ( info->stream, "%s",
                  riscv_vfp_reg_names_riscv[(l >> OP_SH_VFS) & OP_MASK_VFS]);
              break;
            case 'T':
              (*info->fprintf_func)
                ( info->stream, "%s",
                  riscv_vfp_reg_names_riscv[(l >> OP_SH_VFT) & OP_MASK_VFT]);
              break;
            case 'R':
              (*info->fprintf_func)
                ( info->stream, "%s",
                  riscv_vfp_reg_names_riscv[(l >> OP_SH_VFR) & OP_MASK_VFR]);
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
	  (*info->fprintf_func) (info->stream, "0x%x", (unsigned)EXTRACT_UTYPE_IMM (l) << RISCV_IMM_BITS >> RISCV_IMM_BITS);
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
	    pd->hi_addr[rd] = pc + (EXTRACT_UTYPE_IMM (l) << RISCV_IMM_BITS);
	  else if ((l & MASK_LUI) == MATCH_LUI)
	    pd->hi_addr[rd] = EXTRACT_UTYPE_IMM (l) << RISCV_IMM_BITS;
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
	    const char* csr_name = "unknown";
	    switch ((l >> OP_SH_CSR) & OP_MASK_CSR)
	      {
		#define DECLARE_CSR(name, num) case num: csr_name = #name; break;
		#include "opcode/riscv-opc.h"
		#undef DECLARE_CSR
	      }
	    (*info->fprintf_func) (info->stream, "%s", csr_name);
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

#if 0
static unsigned long
riscv_rvc_uncompress(unsigned long rvc_insn)
{
  #define IS_INSN(x, op) (((x) & MASK_##op) == MATCH_##op)
  #define EXTRACT_OPERAND(x, op) (((x) >> OP_SH_##op) & OP_MASK_##op)

  int crd = EXTRACT_OPERAND(rvc_insn, CRD);
  int crs1 = EXTRACT_OPERAND(rvc_insn, CRS1);
  int crs2 = EXTRACT_OPERAND(rvc_insn, CRS2);
  int crds = EXTRACT_OPERAND(rvc_insn, CRDS);
  int crs1s = EXTRACT_OPERAND(rvc_insn, CRS1S);
  int crs2s = EXTRACT_OPERAND(rvc_insn, CRS2S);
  int crs2bs = EXTRACT_OPERAND(rvc_insn, CRS2BS);

  int cimm6 = EXTRACT_OPERAND(rvc_insn, CIMM6);
  int imm6 = ((int32_t)cimm6 << 26 >> 26) & (RISCV_IMM_REACH-1);
  int imm6x4 = (((int32_t)cimm6 << 26 >> 26)*4) & (RISCV_IMM_REACH-1);
  int imm6x4lo = imm6x4 & ((1<<RISCV_IMMLO_BITS)-1);
  int imm6x4hi = (imm6x4 >> RISCV_IMMLO_BITS) & ((1<<RISCV_IMMHI_BITS)-1);
  int imm6x8 = (((int32_t)cimm6 << 26 >> 26)*8) & (RISCV_IMM_REACH-1);
  int imm6x8lo = imm6x8 & ((1<<RISCV_IMMLO_BITS)-1);
  int imm6x8hi = (imm6x8 >> RISCV_IMMLO_BITS) & ((1<<RISCV_IMMHI_BITS)-1);

  int cimm5 = EXTRACT_OPERAND(rvc_insn, CIMM5);
  int imm5 = ((int32_t)cimm5 << 27 >> 27) & (RISCV_IMM_REACH-1);
  int imm5lo = imm5 & ((1<<RISCV_IMMLO_BITS)-1);
  int imm5hi = (imm5 >> RISCV_IMMLO_BITS) & ((1<<RISCV_IMMHI_BITS)-1);
  int imm5x4 = (((int32_t)cimm5 << 27 >> 27)*4) & (RISCV_IMM_REACH-1);
  int imm5x4lo = imm5x4 & ((1<<RISCV_IMMLO_BITS)-1);
  int imm5x4hi = (imm5x4 >> RISCV_IMMLO_BITS) & ((1<<RISCV_IMMHI_BITS)-1);
  int imm5x8 = (((int32_t)cimm5 << 27 >> 27)*8) & (RISCV_IMM_REACH-1);
  int imm5x8lo = imm5x8 & ((1<<RISCV_IMMLO_BITS)-1);
  int imm5x8hi = (imm5x8 >> RISCV_IMMLO_BITS) & ((1<<RISCV_IMMHI_BITS)-1);

  int cimm10 = EXTRACT_OPERAND(rvc_insn, CIMM10);
  int jt10 = ((int32_t)cimm10 << 22 >> 22) & ((1<<RISCV_JUMP_BITS)-1);

  if(IS_INSN(rvc_insn, C_ADDI))
  {
    if(crd == 0)
    {
      if(imm6 & 0x20)
        return MATCH_JALR | (LINK_REG << OP_SH_RD) | (crs1 << OP_SH_RS1);
      else
        return MATCH_JALR | (crs1 << OP_SH_RS1);
    }
    return MATCH_ADDI | (crd << OP_SH_RD) | (crd << OP_SH_RS1) |
           (imm6 << OP_SH_IMMEDIATE);
  }
  if(IS_INSN(rvc_insn, C_ADDIW))
    return MATCH_ADDIW | (crd << OP_SH_RD) | (crd << OP_SH_RS1) | (imm6 << OP_SH_IMMEDIATE);
  if(IS_INSN(rvc_insn, C_LI))
    return MATCH_ADDI | (crd << OP_SH_RD) | (imm6 << OP_SH_IMMEDIATE);
  if(IS_INSN(rvc_insn, C_MOVE))
    return MATCH_ADDI | (crd << OP_SH_RD) | (crs1 << OP_SH_RS1);
  if(IS_INSN(rvc_insn, C_SLLI))
    return MATCH_SLLI | (cimm5 << OP_SH_SHAMT) | (rvc_rd_regmap[crds] << OP_SH_RD) | (rvc_rd_regmap[crds] << OP_SH_RS1);
  if(IS_INSN(rvc_insn, C_SLLI32))
    return MATCH_SLLI | ((cimm5+32) << OP_SH_SHAMT) | (rvc_rd_regmap[crds] << OP_SH_RD) | (rvc_rd_regmap[crds] << OP_SH_RS1);
  if(IS_INSN(rvc_insn, C_SRLI))
    return MATCH_SRLI | (cimm5 << OP_SH_SHAMT) | (rvc_rd_regmap[crds] << OP_SH_RD) | (rvc_rd_regmap[crds] << OP_SH_RS1);
  if(IS_INSN(rvc_insn, C_SRLI32))
    return MATCH_SRLI | ((cimm5+32) << OP_SH_SHAMT) | (rvc_rd_regmap[crds] << OP_SH_RD) | (rvc_rd_regmap[crds] << OP_SH_RS1);
  if(IS_INSN(rvc_insn, C_SRAI))
    return MATCH_SRAI | (cimm5 << OP_SH_SHAMT) | (rvc_rd_regmap[crds] << OP_SH_RD) | (rvc_rd_regmap[crds] << OP_SH_RS1);
  if(IS_INSN(rvc_insn, C_SRAI32))
    return MATCH_SRAI | ((cimm5+32) << OP_SH_SHAMT) | (rvc_rd_regmap[crds] << OP_SH_RD) | (rvc_rd_regmap[crds] << OP_SH_RS1);
  if(IS_INSN(rvc_insn, C_SLLIW))
    return MATCH_SLLIW | (cimm5 << OP_SH_SHAMT) | (rvc_rd_regmap[crds] << OP_SH_RD) | (rvc_rd_regmap[crds] << OP_SH_RS1);
  if(IS_INSN(rvc_insn, C_ADD))
    return MATCH_ADD | (crd << OP_SH_RD) | (crs1 << OP_SH_RS1) | (crd << OP_SH_RS2);
  if(IS_INSN(rvc_insn, C_SUB))
    return MATCH_SUB | (crd << OP_SH_RD) | (crs1 << OP_SH_RS1) | (crd << OP_SH_RS2);
  if(IS_INSN(rvc_insn, C_ADD3))
    return MATCH_ADD | (rvc_rd_regmap[crds] << OP_SH_RD) | (rvc_rs1_regmap[crs1s] << OP_SH_RS1) | (rvc_rs2b_regmap[crs2bs] << OP_SH_RS2);
  if(IS_INSN(rvc_insn, C_SUB3))
    return MATCH_SUB | (rvc_rd_regmap[crds] << OP_SH_RD) | (rvc_rs1_regmap[crs1s] << OP_SH_RS1) | (rvc_rs2b_regmap[crs2bs] << OP_SH_RS2);
  if(IS_INSN(rvc_insn, C_AND3))
    return MATCH_AND | (rvc_rd_regmap[crds] << OP_SH_RD) | (rvc_rs1_regmap[crs1s] << OP_SH_RS1) | (rvc_rs2b_regmap[crs2bs] << OP_SH_RS2);
  if(IS_INSN(rvc_insn, C_OR3))
    return MATCH_OR | (rvc_rd_regmap[crds] << OP_SH_RD) | (rvc_rs1_regmap[crs1s] << OP_SH_RS1) | (rvc_rs2b_regmap[crs2bs] << OP_SH_RS2);
  if(IS_INSN(rvc_insn, C_J))
    return MATCH_JAL | (jt10 << OP_SH_TARGET);
  if(IS_INSN(rvc_insn, C_BEQ))
    return MATCH_BEQ | (rvc_rs1_regmap[crs1s] << OP_SH_RS1) | (rvc_rs2_regmap[crs2s] << OP_SH_RS2) | (imm5lo << OP_SH_IMMLO) | (imm5hi << OP_SH_IMMHI);
  if(IS_INSN(rvc_insn, C_BNE))
    return MATCH_BNE | (rvc_rs1_regmap[crs1s] << OP_SH_RS1) | (rvc_rs2_regmap[crs2s] << OP_SH_RS2) | (imm5lo << OP_SH_IMMLO) | (imm5hi << OP_SH_IMMHI);
  if(IS_INSN(rvc_insn, C_LDSP))
    return MATCH_LD | (30 << OP_SH_RS1) | (crd << OP_SH_RD) | (imm6x8 << OP_SH_IMMEDIATE);
  if(IS_INSN(rvc_insn, C_LWSP))
    return MATCH_LW | (30 << OP_SH_RS1) | (crd << OP_SH_RD) | (imm6x4 << OP_SH_IMMEDIATE);
  if(IS_INSN(rvc_insn, C_SDSP))
    return MATCH_SD | (30 << OP_SH_RS1) | (crs2 << OP_SH_RS2) | (imm6x8lo << OP_SH_IMMLO) | (imm6x8hi << OP_SH_IMMHI);
  if(IS_INSN(rvc_insn, C_SWSP))
    return MATCH_SW | (30 << OP_SH_RS1) | (crs2 << OP_SH_RS2) | (imm6x4lo << OP_SH_IMMLO) | (imm6x4hi << OP_SH_IMMHI);
  if(IS_INSN(rvc_insn, C_LD))
    return MATCH_LD | (rvc_rs1_regmap[crs1s] << OP_SH_RS1) | (rvc_rd_regmap[crds] << OP_SH_RD) | (imm5x8 << OP_SH_IMMEDIATE);
  if(IS_INSN(rvc_insn, C_LW))
    return MATCH_LW | (rvc_rs1_regmap[crs1s] << OP_SH_RS1) | (rvc_rd_regmap[crds] << OP_SH_RD) | (imm5x4 << OP_SH_IMMEDIATE);
  if(IS_INSN(rvc_insn, C_SD))
    return MATCH_SD | (rvc_rs1_regmap[crs1s] << OP_SH_RS1) | (rvc_rs2_regmap[crs2s] << OP_SH_RS2) | (imm5x8lo << OP_SH_IMMLO) | (imm5x8hi << OP_SH_IMMHI);
  if(IS_INSN(rvc_insn, C_SW))
    return MATCH_SW | (rvc_rs1_regmap[crs1s] << OP_SH_RS1) | (rvc_rs2_regmap[crs2s] << OP_SH_RS2) | (imm5x4lo << OP_SH_IMMLO) | (imm5x4hi << OP_SH_IMMHI);
  if(IS_INSN(rvc_insn, C_LD0))
    return MATCH_LD | (crs1 << OP_SH_RS1) | (crd << OP_SH_RD);
  if(IS_INSN(rvc_insn, C_LW0))
    return MATCH_LW | (crs1 << OP_SH_RS1) | (crd << OP_SH_RD);
  if(IS_INSN(rvc_insn, C_FLD))
    return MATCH_FLD | (rvc_rs1_regmap[crs1s] << OP_SH_RS1) | (rvc_rd_regmap[crds] << OP_SH_RD) | (imm5x8 << OP_SH_IMMEDIATE);
  if(IS_INSN(rvc_insn, C_FLW))
    return MATCH_FLW | (rvc_rs1_regmap[crs1s] << OP_SH_RS1) | (rvc_rd_regmap[crds] << OP_SH_RD) | (imm5x4 << OP_SH_IMMEDIATE);
  if(IS_INSN(rvc_insn, C_FSD))
    return MATCH_FSD | (rvc_rs1_regmap[crs1s] << OP_SH_RS1) | (rvc_rs2_regmap[crs2s] << OP_SH_RS2) | (imm5x8lo << OP_SH_IMMLO) | (imm5x8hi << OP_SH_IMMHI);
  if(IS_INSN(rvc_insn, C_FSW))
    return MATCH_FSW | (rvc_rs1_regmap[crs1s] << OP_SH_RS1) | (rvc_rs2_regmap[crs2s] << OP_SH_RS2) | (imm5x4lo << OP_SH_IMMLO) | (imm5x4hi << OP_SH_IMMHI);

  return rvc_insn;
}
#endif

/* Print the RISC-V instruction at address MEMADDR in debugged memory,
   on using INFO.  Returns length of the instruction, in bytes.
   BIGENDIAN must be 1 if this is big-endian code, 0 if
   this is little-endian code.  */

static int
riscv_disassemble_insn (bfd_vma memaddr, insn_t word, disassemble_info *info)
{
  const struct riscv_opcode *op;
  static bfd_boolean init = 0;
  static const char *extension = NULL;
  static const struct riscv_opcode *riscv_hash[OP_MASK_OP + 1];
  struct riscv_private_data *pd;
  int insnlen;

  /* Build a hash table to shorten the search time.  */
  if (! init)
    {
      unsigned int i;
      unsigned int e_flags = elf_elfheader (info->section->owner)->e_flags;
      extension = riscv_elf_flag_to_name(EF_GET_RISCV_EXT(e_flags));

      for (i = 0; i <= OP_MASK_OP; i++)
        for (op = riscv_opcodes; op < &riscv_opcodes[NUMOPCODES]; op++)
          if (i == ((op->match >> OP_SH_OP) & OP_MASK_OP))
            {
              riscv_hash[i] = op;
              break;
            }

      init = 1;
    }

  if (info->private_data == NULL)
    {
      int i;

      pd = info->private_data = calloc(1, sizeof (struct riscv_private_data));
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

#if 0
  if (insnlen == 2)
    word = riscv_rvc_uncompress(word);
#endif

  info->bytes_per_chunk = insnlen % 4 == 0 ? 4 : 2;
  info->bytes_per_line = 8;
  info->display_endian = info->endian;
  info->insn_info_valid = 1;
  info->branch_delay_insns = 0;
  info->data_size = 0;
  info->insn_type = dis_nonbranch;
  info->target = 0;
  info->target2 = 0;

  op = riscv_hash[(word >> OP_SH_OP) & OP_MASK_OP];
  if (op != NULL)
    {
      for (; op < &riscv_opcodes[NUMOPCODES]; op++)
	{
	  if ((op->match_func) (op, word)
	      && !(no_aliases && (op->pinfo & INSN_ALIAS))
	      && !(op->subset[0] == 'X' && strcmp(op->subset, extension)))
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

  set_default_riscv_dis_options (info);
  parse_riscv_dis_options (info->disassembler_options);

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
  unsigned int i;

  fprintf (stream, _("\n\
The following RISC-V-specific disassembler options are supported for use\n\
with the -M switch (multiple options should be separated by commas):\n"));

  fprintf (stream, _("\n\
  gpr-names=ABI            Print GPR names according to  specified ABI.\n\
                           Default: based on binary being disassembled.\n"));

  fprintf (stream, _("\n\
  fpr-names=ABI            Print FPR names according to specified ABI.\n\
                           Default: numeric.\n"));

  fprintf (stream, _("\n\
  For the options above, the following values are supported for \"ABI\":\n\
   "));
  for (i = 0; i < ARRAY_SIZE (riscv_abi_choices); i++)
    fprintf (stream, " %s", riscv_abi_choices[i].name);
  fprintf (stream, _("\n"));

  fprintf (stream, _("\n"));
}
