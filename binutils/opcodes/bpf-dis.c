/* bpf-dis.c - BPF disassembler.
   Copyright (C) 2023-2024 Free Software Foundation, Inc.

   Contributed by Oracle Inc.

   This file is part of the GNU binutils.

   This is free software; you can redistribute them and/or modify them
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING3. If not,
   see <http://www.gnu.org/licenses/>.  */

#include "sysdep.h"
#include "disassemble.h"
#include "libiberty.h"
#include "opintl.h"
#include "opcode/bpf.h"
#include "elf-bfd.h"
#include "elf/bpf.h"

#include <string.h>
#include <inttypes.h>

/* This disassembler supports two different syntaxes for BPF assembly.
   One is called "normal" and has the typical form for assembly
   languages, with mnemonics and the like.  The other is called
   "pseudoc" and looks like C.  */

enum bpf_dialect
{
  BPF_DIALECT_NORMAL,
  BPF_DIALECT_PSEUDOC
};

/* Global configuration for the disassembler.  */

static enum bpf_dialect asm_dialect = BPF_DIALECT_NORMAL;
static int asm_bpf_version = -1;
static int asm_obase = 10;

/* Print BPF specific command-line options.  */

void
print_bpf_disassembler_options (FILE *stream)
{
  fprintf (stream, _("\n\
The following BPF specific disassembler options are supported for use\n\
with the -M switch (multiple options should be separated by commas):\n"));
  fprintf (stream, "\n");
  fprintf (stream, _("\
      pseudoc                  Use pseudo-c syntax.\n\
      v1,v2,v3,v4,xbpf         Version of the BPF ISA to use.\n\
      hex,oct,dec              Output numerical base for immediates.\n"));
}

/* Parse BPF specific command-line options.  */

static void
parse_bpf_dis_option (const char *option)
{
  if (strcmp (option, "pseudoc") == 0)
    asm_dialect = BPF_DIALECT_PSEUDOC;
  else if (strcmp (option, "v1") == 0)
    asm_bpf_version = BPF_V1;
  else if (strcmp (option, "v2") == 0)
    asm_bpf_version = BPF_V2;
  else if (strcmp (option, "v3") == 0)
    asm_bpf_version = BPF_V3;
  else if (strcmp (option, "v4") == 0)
    asm_bpf_version = BPF_V4;
  else if (strcmp (option, "xbpf") == 0)
    asm_bpf_version = BPF_XBPF;
  else if (strcmp (option, "hex") == 0)
    asm_obase = 16;
  else if (strcmp (option, "oct") == 0)
    asm_obase = 8;
  else if (strcmp (option, "dec") == 0)
    asm_obase = 10;
  else
    /* xgettext:c-format */
    opcodes_error_handler (_("unrecognized disassembler option: %s"), option);
}

static void
parse_bpf_dis_options (const char *opts_in)
{
  char *opts = xstrdup (opts_in), *opt = opts, *opt_end = opts;

  for ( ; opt_end != NULL; opt = opt_end + 1)
    {
      if ((opt_end = strchr (opt, ',')) != NULL)
	*opt_end = 0;
      parse_bpf_dis_option (opt);
    }

  free (opts);
}

/* Auxiliary function used in print_insn_bpf below.  */

static void
print_register (disassemble_info *info,
                const char *tag, uint8_t regno)
{
  const char *fmt
    = (asm_dialect == BPF_DIALECT_NORMAL
       ? "%%r%d"
       : ((*(tag + 2) == 'w')
          ? "w%d"
          : "r%d"));

  (*info->fprintf_styled_func) (info->stream, dis_style_register, fmt, regno);
}

/* Main entry point.
   Print one instruction from PC on INFO->STREAM.
   Return the size of the instruction (in bytes).  */

int
print_insn_bpf (bfd_vma pc, disassemble_info *info)
{
  int insn_size = 8, status;
  bfd_byte insn_bytes[16];
  bpf_insn_word word = 0;
  const struct bpf_opcode *insn = NULL;
  enum bpf_endian endian = (info->endian == BFD_ENDIAN_LITTLE
                            ? BPF_ENDIAN_LITTLE : BPF_ENDIAN_BIG);

  /* Handle bpf-specific command-line options.  */
  if (info->disassembler_options != NULL)
    {
      parse_bpf_dis_options (info->disassembler_options);
      /* Avoid repeteadly parsing the options.  */
      info->disassembler_options = NULL;
    }

  /* Determine what version of the BPF ISA to use when disassembling.
     If the user didn't explicitly specify an ISA version, then derive
     it from the CPU Version flag in the ELF header.  A CPU version of
     0 in the header means "latest version".  */
  if (asm_bpf_version == -1 && info->section && info->section->owner)
    {
      struct bfd *abfd = info->section->owner;
      Elf_Internal_Ehdr *header = elf_elfheader (abfd);
      int cpu_version = header->e_flags & EF_BPF_CPUVER;

      switch (cpu_version)
        {
        case 0: asm_bpf_version = BPF_V4; break;
        case 1: asm_bpf_version = BPF_V1; break;
        case 2: asm_bpf_version = BPF_V2; break;
        case 3: asm_bpf_version = BPF_V3; break;
        case 4: asm_bpf_version = BPF_V4; break;
        case 0xf: asm_bpf_version = BPF_XBPF; break;
        default:
          /* xgettext:c-format */
          opcodes_error_handler (_("unknown BPF CPU version %u\n"),
                                 cpu_version);
          break;
        }
    }

  /* Print eight bytes per line.  */
  info->bytes_per_chunk = 1;
  info->bytes_per_line = 8;

  /* Read an instruction word.  */
  status = (*info->read_memory_func) (pc, insn_bytes, 8, info);
  if (status != 0)
    {
      (*info->memory_error_func) (status, pc, info);
      return -1;
    }
  word = (bpf_insn_word) bfd_getb64 (insn_bytes);

  /* Try to match an instruction with it.  */
  insn = bpf_match_insn (word, endian, asm_bpf_version);

  /* Print it out.  */
  if (insn)
    {
      const char *insn_tmpl
        = asm_dialect == BPF_DIALECT_NORMAL ? insn->normal : insn->pseudoc;
      const char *p = insn_tmpl;

      /* Print the template contents completed with the instruction
         operands.  */
      for (p = insn_tmpl; *p != '\0';)
        {
          switch (*p)
            {
            case ' ':
              /* Single space prints to nothing.  */
              p += 1;
              break;
            case '%':
              if (*(p + 1) == '%')
                {
                  (*info->fprintf_styled_func) (info->stream, dis_style_text, "%%");
                  p += 2;
                }
              else if (*(p + 1) == 'w' || *(p + 1) == 'W')
                {
                  /* %W prints to a single space.  */
                  (*info->fprintf_styled_func) (info->stream, dis_style_text, " ");
                  p += 2;
                }
              else if (strncmp (p, "%dr", 3) == 0)
                {
                  print_register (info, p, bpf_extract_dst (word, endian));
                  p += 3;
                }
              else if (strncmp (p, "%sr", 3) == 0)
                {
                  print_register (info, p, bpf_extract_src (word, endian));
                  p += 3;
                }
              else if (strncmp (p, "%dw", 3) == 0)
                {
                  print_register (info, p, bpf_extract_dst (word, endian));
                  p += 3;
                }
              else if (strncmp (p, "%sw", 3) == 0)
                {
                  print_register (info, p, bpf_extract_src (word, endian));
                  p += 3;
                }
              else if (strncmp (p, "%i32", 4) == 0
                       || strncmp (p, "%d32", 4) == 0
                       || strncmp (p, "%I32", 4) == 0)
                {
                  int32_t imm32 = bpf_extract_imm32 (word, endian);

                  if (p[1] == 'I')
                    (*info->fprintf_styled_func) (info->stream, dis_style_immediate,
                                                  "%s",
						  asm_obase != 10 || imm32 >= 0 ? "+" : "");
                  (*info->fprintf_styled_func) (info->stream, dis_style_immediate,
                                                asm_obase == 10 ? "%" PRIi32
                                                : asm_obase == 8 ? "%" PRIo32
                                                : "0x%" PRIx32,
                                                imm32);
                  p += 4;
                }
              else if (strncmp (p, "%o16", 4) == 0
                       || strncmp (p, "%d16", 4) == 0)
                {
                  int16_t offset16 = bpf_extract_offset16 (word, endian);

                  if (p[1] == 'o')
                    (*info->fprintf_styled_func) (info->stream, dis_style_immediate,
                                                  "%s",
						  asm_obase != 10 || offset16 >= 0 ? "+" : "");
                  if (asm_obase == 16 || asm_obase == 8)
                    (*info->fprintf_styled_func) (info->stream, dis_style_immediate,
                                                  asm_obase == 8 ? "0%" PRIo16 : "0x%" PRIx16,
                                                  (uint16_t) offset16);
                  else
                    (*info->fprintf_styled_func) (info->stream, dis_style_immediate,
                                                  "%" PRIi16, offset16);
                  p += 4;
                }
              else if (strncmp (p, "%i64", 4) == 0)
                {
                  bpf_insn_word word2 = 0;

                  status = (*info->read_memory_func) (pc + 8, insn_bytes + 8,
                                                          8, info);
                  if (status != 0)
                    {
                      (*info->memory_error_func) (status, pc + 8, info);
                      return -1;
                    }
                  word2 = (bpf_insn_word) bfd_getb64 (insn_bytes + 8);

                  (*info->fprintf_styled_func) (info->stream, dis_style_immediate,
                                                asm_obase == 10 ? "%" PRIi64
                                                : asm_obase == 8 ? "0%" PRIo64
                                                : "0x%" PRIx64,
                                                bpf_extract_imm64 (word, word2, endian));
                  insn_size = 16;
                  p += 4;
                }
              else
                {
                  /* xgettext:c-format */
                  opcodes_error_handler (_("# internal error, unknown tag in opcode template (%s)"),
                                         insn_tmpl);
                  return -1;
                }
              break;
            default:
              /* Any other character is printed literally.  */
              (*info->fprintf_styled_func) (info->stream, dis_style_text, "%c", *p);
              p += 1;
            }
        }
    }
  else
    (*info->fprintf_styled_func) (info->stream, dis_style_text, "<unknown>");

  return insn_size;
}
