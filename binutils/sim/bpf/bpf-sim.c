/* Simulator for BPF.
   Copyright (C) 2020-2024 Free Software Foundation, Inc.

   Contributed by Oracle Inc.

   This file is part of GDB, the GNU debugger.

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

/* This must come before any other includes.  */
#include "defs.h"
#include "libiberty.h"

#include "bfd.h"
#include "opcode/bpf.h"
#include "sim/sim.h"
#include "sim-main.h"
#include "sim-core.h"
#include "sim-base.h"
#include "sim-options.h"
#include "sim-signal.h"
#include "bpf-sim.h"

#include <assert.h>
#include <stdlib.h>


/***** Emulated hardware.  *****/

/* Registers are 64-bit long.
   11 general purpose registers, indexed by register number.
   1 program counter.  */

typedef uint64_t bpf_reg;

bpf_reg bpf_pc;
bpf_reg bpf_regs[11];

#define BPF_R0 0
#define BPF_R1 1
#define BPF_R2 2
#define BPF_R3 3
#define BPF_R4 4
#define BPF_R5 5
#define BPF_R6 6
#define BPF_R7 7
#define BPF_R8 8
#define BPF_R9 9
#define BPF_R10 10
#define BPF_FP 10


/***** Emulated memory accessors.  *****/

static uint8_t
bpf_read_u8 (SIM_CPU *cpu, bfd_vma address)
{
  return sim_core_read_unaligned_1 (cpu, 0, read_map, address);
}

static void
bpf_write_u8 (SIM_CPU *cpu, bfd_vma address, uint8_t value)
{
  sim_core_write_unaligned_1 (cpu, 0, write_map, address, value);
}

static uint16_t ATTRIBUTE_UNUSED
bpf_read_u16 (SIM_CPU *cpu, bfd_vma address)
{
  uint16_t val = sim_core_read_unaligned_2 (cpu, 0, read_map, address);

  if (current_target_byte_order == BFD_ENDIAN_LITTLE)
    return endian_le2h_2 (val);
  else
    return endian_le2h_2 (val);
}

static void
bpf_write_u16 (SIM_CPU *cpu, bfd_vma address, uint16_t value)
{
  sim_core_write_unaligned_2 (cpu, 0, write_map, address, endian_h2le_2 (value));
}

static uint32_t ATTRIBUTE_UNUSED
bpf_read_u32 (SIM_CPU *cpu, bfd_vma address)
{
  uint32_t val = sim_core_read_unaligned_4 (cpu, 0, read_map, address);

  if (current_target_byte_order == BFD_ENDIAN_LITTLE)
    return endian_le2h_4 (val);
  else
    return endian_le2h_4 (val);
}

static void
bpf_write_u32 (SIM_CPU *cpu, bfd_vma address, uint32_t value)
{
  sim_core_write_unaligned_4 (cpu, 0, write_map, address, endian_h2le_4 (value));
}

static uint64_t ATTRIBUTE_UNUSED
bpf_read_u64 (SIM_CPU *cpu, bfd_vma address)
{
  uint64_t val = sim_core_read_unaligned_8 (cpu, 0, read_map, address);

  if (current_target_byte_order == BFD_ENDIAN_LITTLE)
    return endian_le2h_8 (val);
  else
    return endian_le2h_8 (val);
}

static void
bpf_write_u64 (SIM_CPU *cpu, bfd_vma address, uint64_t value)
{
  sim_core_write_unaligned_8 (cpu, 0, write_map, address, endian_h2le_8 (value));
}


/***** Emulation of the BPF kernel helpers.  *****/

/* BPF programs rely on the existence of several helper functions,
   which are provided by the kernel.  This simulator provides an
   implementation of the helpers, which can be customized by the
   user.  */

/* bpf_trace_printk is a printk-like facility for debugging.

   In the kernel, it appends a line to the Linux's tracing debugging
   interface.

   In this simulator, it uses the simulator's tracing interface
   instead.

   The format tags recognized by this helper are:
   %d, %i, %u, %x, %ld, %li, %lu, %lx, %lld, %lli, %llu, %llx,
   %p, %s

   A maximum of three tags are supported.

   This helper returns the number of bytes written, or a negative
   value in case of failure.  */

static int
bpf_trace_printk (SIM_CPU *cpu)
{
  SIM_DESC sd = CPU_STATE (cpu);

  bfd_vma fmt_address;
  uint32_t size, tags_processed;
  size_t i, bytes_written = 0;

  /* The first argument is the format string, which is passed as a
     pointer in %r1.  */
  fmt_address = bpf_regs[BPF_R1];

  /* The second argument is the length of the format string, as an
     unsigned 32-bit number in %r2.  */
  size = bpf_regs[BPF_R2];

  /* Read the format string from the memory pointed by %r2, printing
     out the stuff as we go.  There is a maximum of three format tags
     supported, which are read from %r3, %r4 and %r5 respectively.  */
  for (i = 0, tags_processed = 0; i < size;)
    {
      uint64_t value;
      uint8_t c = bpf_read_u8 (cpu, fmt_address + i);

      switch (c)
        {
        case '%':
          /* Check we are not exceeding the limit of three format
             tags.  */
          if (tags_processed > 2)
            return -1; /* XXX look for kernel error code.  */

          /* Depending on the kind of tag, extract the value from the
             proper argument.  */
          if (i++ >= size)
            return -1; /* XXX look for kernel error code.  */

          value = bpf_regs[BPF_R3 + tags_processed];

          switch ((bpf_read_u8 (cpu, fmt_address + i)))
            {
            case 'd':
              trace_printf (sd, cpu, "%d", (int) value);
              break;
            case 'i':
              trace_printf (sd, cpu, "%i", (int) value);
              break;
            case 'u':
              trace_printf (sd, cpu, "%u", (unsigned int) value);
              break;
            case 'x':
              trace_printf (sd, cpu, "%x", (unsigned int) value);
              break;
            case 'l':
              {
                if (i++ >= size)
                  return -1;
                switch (bpf_read_u8 (cpu, fmt_address + i))
                  {
                  case 'd':
                    trace_printf (sd, cpu, "%ld", (long) value);
                    break;
                  case 'i':
                    trace_printf (sd, cpu, "%li", (long) value);
                    break;
                  case 'u':
                    trace_printf (sd, cpu, "%lu", (unsigned long) value);
                    break;
                  case 'x':
                    trace_printf (sd, cpu, "%lx", (unsigned long) value);
                    break;
                  case 'l':
                    {
                      if (i++ >= size)
                        return -1;
                      switch (bpf_read_u8 (cpu, fmt_address + i))
                        {
                        case 'd':
                          trace_printf (sd, cpu, "%lld", (long long) value);
                          break;
                        case 'i':
                          trace_printf (sd, cpu, "%lli", (long long) value);
                          break;
                        case 'u':
                          trace_printf (sd, cpu, "%llu", (unsigned long long) value);
                          break;
                        case 'x':
                          trace_printf (sd, cpu, "%llx", (unsigned long long) value);
                          break;
                        default:
                          assert (0);
                          break;
                      }
                      break;
                    }
                  default:
                    assert (0);
                    break;
                }
                break;
              }
            default:
              /* XXX completeme */
              assert (0);
              break;
            }

          tags_processed++;
          i++;
          break;
        case '\0':
          i = size;
          break;
        default:
          trace_printf (sd, cpu, "%c", c);
          bytes_written++;
          i++;
          break;
        }
    }

  return bytes_written;
}


/****** Accessors to install in the CPU description.  ******/

static int
bpf_reg_get (SIM_CPU *cpu, int rn, void *buf, int length)
{
  bpf_reg val;
  unsigned char *memory = buf;

  if (length != 8 || rn >= 11)
    return 0;

  val = bpf_regs[rn];

  if (current_target_byte_order == BFD_ENDIAN_LITTLE)
    {
      memory[7] = (val >> 56) & 0xff;
      memory[6] = (val >> 48) & 0xff;
      memory[5] = (val >> 40) & 0xff;
      memory[4] = (val >> 32) & 0xff;
      memory[3] = (val >> 24) & 0xff;
      memory[2] = (val >> 16) & 0xff;
      memory[1] = (val >> 8) & 0xff;
      memory[0] = val & 0xff;
    }
  else
    {
      memory[0] = (val >> 56) & 0xff;
      memory[1] = (val >> 48) & 0xff;
      memory[2] = (val >> 40) & 0xff;
      memory[3] = (val >> 32) & 0xff;
      memory[4] = (val >> 24) & 0xff;
      memory[5] = (val >> 16) & 0xff;
      memory[6] = (val >> 8) & 0xff;
      memory[7] = val & 0xff;
    }

  return 8;
}

static int
bpf_reg_set (SIM_CPU *cpu, int rn, const void *buf, int length)
{
  const unsigned char *memory = buf;

  if (length != 8 || rn >= 11)
    return 0;

  if (current_target_byte_order == BFD_ENDIAN_LITTLE)
    bpf_regs[rn] = (((uint64_t) memory[7] << 56)
                    | ((uint64_t) memory[6] << 48)
                    | ((uint64_t) memory[5] << 40)
                    | ((uint64_t) memory[4] << 32)
                    | ((uint64_t) memory[3] << 24)
                    | ((uint64_t) memory[2] << 16)
                    | ((uint64_t) memory[1] << 8)
                    | ((uint64_t) memory[0]));
  else
    bpf_regs[rn] = (((uint64_t) memory[0] << 56)
                    | ((uint64_t) memory[1] << 48)
                    | ((uint64_t) memory[2] << 40)
                    | ((uint64_t) memory[3] << 32)
                    | ((uint64_t) memory[4] << 24)
                    | ((uint64_t) memory[5] << 16)
                    | ((uint64_t) memory[6] << 8)
                    | ((uint64_t) memory[7]));
  return 8;
}

static sim_cia
bpf_pc_get (sim_cpu *cpu)
{
  return bpf_pc;
}

static void
bpf_pc_set (sim_cpu *cpu, sim_cia pc)
{
  bpf_pc = pc;
}


/***** Other global state.  ******/

static int64_t skb_data_offset;

/* String with the name of the section containing the BPF program to
   run.  */
static char *bpf_program_section = NULL;


/***** Handle BPF-specific command line options.  *****/

static SIM_RC bpf_option_handler (SIM_DESC, sim_cpu *, int, char *, int);

typedef enum
{
 OPTION_BPF_SET_PROGRAM = OPTION_START,
 OPTION_BPF_LIST_PROGRAMS,
 OPTION_BPF_VERIFY_PROGRAM,
 OPTION_BPF_SKB_DATA_OFFSET,
} BPF_OPTION;

static const OPTION bpf_options[] =
{
 { {"bpf-set-program", required_argument, NULL, OPTION_BPF_SET_PROGRAM},
   '\0', "SECTION_NAME", "Set the entry point",
   bpf_option_handler },
 { {"bpf-list-programs", no_argument, NULL, OPTION_BPF_LIST_PROGRAMS},
   '\0', "", "List loaded bpf programs",
   bpf_option_handler },
 { {"bpf-verify-program", required_argument, NULL, OPTION_BPF_VERIFY_PROGRAM},
   '\0', "PROGRAM", "Run the verifier on the given BPF program",
   bpf_option_handler },
 { {"skb-data-offset", required_argument, NULL, OPTION_BPF_SKB_DATA_OFFSET},
   '\0', "OFFSET", "Configure offsetof(struct sk_buff, data)",
   bpf_option_handler },

 { {NULL, no_argument, NULL, 0}, '\0', NULL, NULL, NULL, NULL }
};

static SIM_RC
bpf_option_handler (SIM_DESC sd, sim_cpu *cpu ATTRIBUTE_UNUSED, int opt,
                    char *arg, int is_command ATTRIBUTE_UNUSED)
{
  switch ((BPF_OPTION) opt)
    {
    case OPTION_BPF_VERIFY_PROGRAM:
      /* XXX call the verifier. */
      sim_io_printf (sd, "Verifying BPF program %s...\n", arg);
      break;

    case OPTION_BPF_LIST_PROGRAMS:
      /* XXX list programs.  */
      sim_io_printf (sd, "BPF programs available:\n");
      break;

    case OPTION_BPF_SET_PROGRAM:
      /* XXX: check that the section exists and tell the user about a
         new start_address.  */
      bpf_program_section = xstrdup (arg);
      break;

    case OPTION_BPF_SKB_DATA_OFFSET:
      skb_data_offset = strtoul (arg, NULL, 0);
      break;

    default:
      sim_io_eprintf (sd, "Unknown option `%s'\n", arg);
      return SIM_RC_FAIL;
    }

  return SIM_RC_OK;
}


/***** Instruction decoding.  *****/

/* Decoded BPF instruction.   */

struct bpf_insn
{
  enum bpf_insn_id id;
  int size; /*  Instruction size in bytes.  */
  bpf_reg dst;
  bpf_reg src;
  int16_t offset16;
  int32_t imm32;
  int64_t imm64;
};

/* Read an instruction word at the given PC.  Note that we need to
   return a big-endian word.  */

static bpf_insn_word
bpf_read_insn_word (SIM_CPU *cpu, uint64_t pc)
{
  bpf_insn_word word = sim_core_read_unaligned_8 (cpu, 0, read_map, pc);

  if (current_target_byte_order == BFD_ENDIAN_LITTLE)
    word = endian_le2h_8 (word);
  else
    word = endian_be2h_8 (word);

  return endian_h2be_8 (word);
}

/* Decode and return a BPF instruction at the given PC.  Return 0 if
   no valid instruction is found, 1 otherwise.  */

static int ATTRIBUTE_UNUSED
decode (SIM_CPU *cpu, uint64_t pc, struct bpf_insn *insn)
{
  const struct bpf_opcode *opcode;
  bpf_insn_word word;
  const char *p;
  enum bpf_endian endian
    = (current_target_byte_order == BFD_ENDIAN_LITTLE
       ? BPF_ENDIAN_LITTLE : BPF_ENDIAN_BIG);

  /* Initialize the insn struct.  */
  memset (insn, 0, sizeof (struct bpf_insn));

  /* Read a 64-bit instruction word at PC.  */
  word = bpf_read_insn_word (cpu, pc);

  /* See if it is a valid instruction and get the opcodes.  */
  opcode = bpf_match_insn (word, endian, BPF_V4);
  if (!opcode)
    return 0;

  insn->id = opcode->id;
  insn->size = 8;

  /* Extract operands using the instruction as a guide.  */
  for (p = opcode->normal; *p != '\0';)
    {
      if (*p == '%')
        {
          if (*(p + 1) == '%')
            p += 2;
          else if (strncmp (p, "%dr", 3) == 0)
            {
              insn->dst = bpf_extract_dst (word, endian);
              p += 3;
            }
          else if (strncmp (p, "%sr", 3) == 0)
            {
              insn->src = bpf_extract_src (word, endian);
              p += 3;
            }
          else if (strncmp (p, "%dw", 3) == 0)
            {
              insn->dst = bpf_extract_dst (word, endian);
              p += 3;
            }
          else if (strncmp (p, "%sw", 3) == 0)
            {
              insn->src = bpf_extract_src (word, endian);
              p += 3;
            }
          else if (strncmp (p, "%i32", 4) == 0
                   || strncmp (p, "%d32", 4) == 0)

            {
              insn->imm32 = bpf_extract_imm32 (word, endian);
              p += 4;
            }
          else if (strncmp (p, "%o16", 4) == 0
                   || strncmp (p, "%d16", 4) == 0)
            {
              insn->offset16 = bpf_extract_offset16 (word, endian);
              p += 4;
            }
          else if (strncmp (p, "%i64", 4) == 0)
            {
              bpf_insn_word word2;
              /* XXX PC + 8 */
              word2 = bpf_read_insn_word (cpu, pc + 8);
              insn->imm64 = bpf_extract_imm64 (word, word2, endian);
              insn->size = 16;
              p += 4;
            }
          else if (strncmp (p, "%w", 2) == 0
                   || strncmp (p, "%W", 2) == 0)
            {
              /* Ignore these templates.  */
              p += 2;
            }
          else
            /* Malformed opcode template.  */
            /* XXX ignore unknown tags? */
            assert (0);
        }
      else
        p += 1;
    }

  return 1;
}


/***** Instruction semantics.  *****/

static void
bpf_call (SIM_CPU *cpu, int32_t disp32, uint8_t src)
{
  /* eBPF supports two kind of CALL instructions: the so called pseudo
     calls ("bpf to bpf") and external calls ("bpf to helper").

     Both kind of calls use the same instruction (CALL).  However,
     external calls are constructed by passing a constant argument to
     the instruction, that identifies the helper, whereas pseudo calls
     result from expressions involving symbols.

     We distinguish calls from pseudo-calls with the later having a 1
     stored in the SRC field of the instruction.  */

  if (src == 1)
    {
      /* This is a pseudo-call.  */

      /* XXX allocate a new stack frame and transfer control.  For
         that we need to analyze the target function, like the kernel
         verifier does.  We better populate a cache
         (function_start_address -> frame_size) so we avoid
         calculating this more than once.  But it is easier to just
         allocate the maximum stack size per stack frame? */
      /* XXX note that disp32 is PC-relative in number of 64-bit
         words, _minus one_.  */
    }
  else
    {
      /* This is a call to a helper.
         DISP32 contains the helper number.  */

      switch (disp32) {
        /* case TRACE_PRINTK: */
        case 7:
          bpf_trace_printk (cpu);
          break;
        default:;
      }
    }
}

static int
execute (SIM_CPU *cpu, struct bpf_insn *insn)
{
  uint64_t next_pc = bpf_pc + insn->size;

/* Displacements in instructions are encoded in number of 64-bit
   words _minus one_, and not in bytes.  */
#define DISP(OFFSET) (((OFFSET) + 1) * 8)

/* For debugging.  */
#define BPF_TRACE(STR)                          \
  do                                            \
    {                                           \
    if (0)                                      \
      printf ("%s", (STR));                     \
    }                                           \
  while (0)
  
  switch (insn->id)
    {
      /* Instruction to trap to GDB.  */
    case BPF_INSN_BRKPT:
      BPF_TRACE ("BPF_INSN_BRKPT\n");
      sim_engine_halt (CPU_STATE (cpu), cpu,
                       NULL, bpf_pc, sim_stopped, SIM_SIGTRAP);
      break;
      /* ALU instructions.  */
    case BPF_INSN_ADDR:
      BPF_TRACE ("BPF_INSN_ADDR\n");
      bpf_regs[insn->dst] += bpf_regs[insn->src];
      break;
    case BPF_INSN_ADDI:
      BPF_TRACE ("BPF_INSN_ADDI\n");
      bpf_regs[insn->dst] += insn->imm32;
      break;
    case BPF_INSN_SUBR:
      BPF_TRACE ("BPF_INSN_SUBR\n");
      bpf_regs[insn->dst] -= bpf_regs[insn->src];
      break;
    case BPF_INSN_SUBI:
      BPF_TRACE ("BPF_INSN_SUBI\n");
      bpf_regs[insn->dst] -= insn->imm32;
      break;
    case BPF_INSN_MULR:
      BPF_TRACE ("BPF_INSN_MULR\n");
      bpf_regs[insn->dst] *= bpf_regs[insn->src];
      break;
    case BPF_INSN_MULI:
      BPF_TRACE ("BPF_INSN_MULI\n");
      bpf_regs[insn->dst] *= insn->imm32;
      break;
    case BPF_INSN_DIVR:
      BPF_TRACE ("BPF_INSN_DIVR\n");
      if (bpf_regs[insn->src] == 0)
        sim_engine_halt (CPU_STATE (cpu), cpu, NULL, bpf_pc, sim_signalled, SIM_SIGFPE);
      bpf_regs[insn->dst] /= bpf_regs[insn->src];
      break;
    case BPF_INSN_DIVI:
      BPF_TRACE ("BPF_INSN_DIVI\n");
      if (insn->imm32 == 0)
        sim_engine_halt (CPU_STATE (cpu), cpu, NULL, bpf_pc, sim_signalled, SIM_SIGFPE);
      bpf_regs[insn->dst] /= insn->imm32;
      break;
    case BPF_INSN_MODR:
      BPF_TRACE ("BPF_INSN_MODR\n");
      if (bpf_regs[insn->src] == 0)
        sim_engine_halt (CPU_STATE (cpu), cpu, NULL, bpf_pc, sim_signalled, SIM_SIGFPE);
      bpf_regs[insn->dst] %= bpf_regs[insn->src];
      break;
    case BPF_INSN_MODI:
      BPF_TRACE ("BPF_INSN_MODI\n");
      if (insn->imm32 == 0)
        sim_engine_halt (CPU_STATE (cpu), cpu, NULL, bpf_pc, sim_signalled, SIM_SIGFPE);
      bpf_regs[insn->dst] %= insn->imm32;
      break;
    case BPF_INSN_ORR:
      BPF_TRACE ("BPF_INSN_ORR\n");
      bpf_regs[insn->dst] |= bpf_regs[insn->src];
      break;
    case BPF_INSN_ORI:
      BPF_TRACE ("BPF_INSN_ORI\n");
      bpf_regs[insn->dst] |= insn->imm32;
      break;
    case BPF_INSN_ANDR:
      BPF_TRACE ("BPF_INSN_ANDR\n");
      bpf_regs[insn->dst] &= bpf_regs[insn->src];
      break;
    case BPF_INSN_ANDI:
      BPF_TRACE ("BPF_INSN_ANDI\n");
      bpf_regs[insn->dst] &= insn->imm32;
      break;
    case BPF_INSN_XORR:
      BPF_TRACE ("BPF_INSN_XORR\n");
      bpf_regs[insn->dst] ^= bpf_regs[insn->src];
      break;
    case BPF_INSN_XORI:
      BPF_TRACE ("BPF_INSN_XORI\n");
      bpf_regs[insn->dst] ^= insn->imm32;
      break;
    case BPF_INSN_SDIVR:
      BPF_TRACE ("BPF_INSN_SDIVR\n");
      if (bpf_regs[insn->src] == 0)
        sim_engine_halt (CPU_STATE (cpu), cpu, NULL, bpf_pc, sim_signalled, SIM_SIGFPE);
      bpf_regs[insn->dst] = (int64_t) bpf_regs[insn->dst] / (int64_t) bpf_regs[insn->src];
      break;
    case BPF_INSN_SDIVI:
      BPF_TRACE ("BPF_INSN_SDIVI\n");
      if (insn->imm32 == 0)
        sim_engine_halt (CPU_STATE (cpu), cpu, NULL, bpf_pc, sim_signalled, SIM_SIGFPE);
      bpf_regs[insn->dst] = (int64_t) bpf_regs[insn->dst] / (int64_t) insn->imm32;
      break;
    case BPF_INSN_SMODR:
      BPF_TRACE ("BPF_INSN_SMODR\n");
      if (bpf_regs[insn->src] == 0)
        sim_engine_halt (CPU_STATE (cpu), cpu, NULL, bpf_pc, sim_signalled, SIM_SIGFPE);
      bpf_regs[insn->dst] = (int64_t) bpf_regs[insn->dst] % (int64_t) bpf_regs[insn->src];
      break;
    case BPF_INSN_SMODI:
      BPF_TRACE ("BPF_INSN_SMODI\n");
      if (insn->imm32 == 0)
        sim_engine_halt (CPU_STATE (cpu), cpu, NULL, bpf_pc, sim_signalled, SIM_SIGFPE);
      bpf_regs[insn->dst] = (int64_t) bpf_regs[insn->dst] % (int64_t) insn->imm32;
      break;
    case BPF_INSN_NEGR:
      BPF_TRACE ("BPF_INSN_NEGR\n");
      bpf_regs[insn->dst] = - (int64_t) bpf_regs[insn->dst];
      break;
    case BPF_INSN_LSHR:
      BPF_TRACE ("BPF_INSN_LSHR\n");
      bpf_regs[insn->dst] <<= bpf_regs[insn->src];
      break;
    case BPF_INSN_LSHI:
      BPF_TRACE ("BPF_INSN_LSHI\n");
      bpf_regs[insn->dst] <<= insn->imm32;
      break;
    case BPF_INSN_RSHR:
      BPF_TRACE ("BPF_INSN_RSHR\n");
      bpf_regs[insn->dst] >>= bpf_regs[insn->src];
      break;
    case BPF_INSN_RSHI:
      BPF_TRACE ("BPF_INSN_RSHI\n");
      bpf_regs[insn->dst] >>= insn->imm32;
      break;
    case BPF_INSN_ARSHR:
      BPF_TRACE ("BPF_INSN_ARSHR\n");
      bpf_regs[insn->dst] = (int64_t) bpf_regs[insn->dst] >> bpf_regs[insn->src];
      break;
    case BPF_INSN_ARSHI:
      BPF_TRACE ("BPF_INSN_ARSHI\n");
      bpf_regs[insn->dst] = (int64_t) bpf_regs[insn->dst] >> insn->imm32;
      break;
    case BPF_INSN_MOVR:
      BPF_TRACE ("BPF_INSN_MOVR\n");
      bpf_regs[insn->dst] = bpf_regs[insn->src];
      break;
    case BPF_INSN_MOVI:
      BPF_TRACE ("BPF_INSN_MOVI\n");
      bpf_regs[insn->dst] = insn->imm32;
      break;
      /* ALU32 instructions.  */
    case BPF_INSN_ADD32R:
      BPF_TRACE ("BPF_INSN_ADD32R\n");
      bpf_regs[insn->dst] = (int32_t) bpf_regs[insn->dst] + (int32_t) bpf_regs[insn->src];
      break;
    case BPF_INSN_ADD32I:
      BPF_TRACE ("BPF_INSN_ADD32I\n");
      bpf_regs[insn->dst] = (int32_t) bpf_regs[insn->dst] + insn->imm32;
      break;
    case BPF_INSN_SUB32R:
      BPF_TRACE ("BPF_INSN_SUB32R\n");
      bpf_regs[insn->dst] = (int32_t) bpf_regs[insn->dst] - (int32_t) bpf_regs[insn->src];
      break;
    case BPF_INSN_SUB32I:
      BPF_TRACE ("BPF_INSN_SUB32I\n");
      bpf_regs[insn->dst] = (int32_t) bpf_regs[insn->dst] - insn->imm32;
      break;
    case BPF_INSN_MUL32R:
      BPF_TRACE ("BPF_INSN_MUL32R\n");
      bpf_regs[insn->dst] = (int32_t) bpf_regs[insn->dst] * (int32_t) bpf_regs[insn->src];
      break;
    case BPF_INSN_MUL32I:
      BPF_TRACE ("BPF_INSN_MUL32I\n");
      bpf_regs[insn->dst] = (int32_t) bpf_regs[insn->dst] * (int32_t) insn->imm32;
      break;
    case BPF_INSN_DIV32R:
      BPF_TRACE ("BPF_INSN_DIV32R\n");
      if (bpf_regs[insn->src] == 0)
        sim_engine_halt (CPU_STATE (cpu), cpu, NULL, bpf_pc, sim_signalled, SIM_SIGFPE);
      bpf_regs[insn->dst] = (uint32_t) bpf_regs[insn->dst] / (uint32_t) bpf_regs[insn->src];
      break;
    case BPF_INSN_DIV32I:
      BPF_TRACE ("BPF_INSN_DIV32I\n");
      if (insn->imm32 == 0)
        sim_engine_halt (CPU_STATE (cpu), cpu, NULL, bpf_pc, sim_signalled, SIM_SIGFPE);
      bpf_regs[insn->dst] = (uint32_t) bpf_regs[insn->dst] / (uint32_t) insn->imm32;
      break;
    case BPF_INSN_MOD32R:
      BPF_TRACE ("BPF_INSN_MOD32R\n");
      if (bpf_regs[insn->src] == 0)
        sim_engine_halt (CPU_STATE (cpu), cpu, NULL, bpf_pc, sim_signalled, SIM_SIGFPE);
      bpf_regs[insn->dst] = (uint32_t) bpf_regs[insn->dst] % (uint32_t) bpf_regs[insn->src];
      break;
    case BPF_INSN_MOD32I:
      BPF_TRACE ("BPF_INSN_MOD32I\n");
      if (insn->imm32 == 0)
        sim_engine_halt (CPU_STATE (cpu), cpu, NULL, bpf_pc, sim_signalled, SIM_SIGFPE);
      bpf_regs[insn->dst] = (uint32_t) bpf_regs[insn->dst] % (uint32_t) insn->imm32;
      break;
    case BPF_INSN_OR32R:
      BPF_TRACE ("BPF_INSN_OR32R\n");
      bpf_regs[insn->dst] = (uint32_t) bpf_regs[insn->dst] | (int32_t) bpf_regs[insn->src];
      break;
    case BPF_INSN_OR32I:
      BPF_TRACE ("BPF_INSN_OR32I\n");
      bpf_regs[insn->dst] = (uint32_t) bpf_regs[insn->dst] | (int32_t) insn->imm32;
      break;
    case BPF_INSN_AND32R:
      BPF_TRACE ("BPF_INSN_AND32R\n");
      bpf_regs[insn->dst] = (uint32_t) bpf_regs[insn->dst] & (int32_t) bpf_regs[insn->src];
      break;
    case BPF_INSN_AND32I:
      BPF_TRACE ("BPF_INSN_AND32I\n");
      bpf_regs[insn->dst] = (uint32_t) bpf_regs[insn->dst] & (int32_t) insn->imm32;
      break;
    case BPF_INSN_XOR32R:
      BPF_TRACE ("BPF_INSN_XOR32R\n");
      bpf_regs[insn->dst] = (uint32_t) bpf_regs[insn->dst] ^ (int32_t) bpf_regs[insn->src];
      break;
    case BPF_INSN_XOR32I:
      BPF_TRACE ("BPF_INSN_XOR32I\n");
      bpf_regs[insn->dst] = (uint32_t) bpf_regs[insn->dst] ^ (int32_t) insn->imm32;
      break;
    case BPF_INSN_SDIV32R:
      BPF_TRACE ("BPF_INSN_SDIV32R\n");
      if (bpf_regs[insn->src] == 0)
        sim_engine_halt (CPU_STATE (cpu), cpu, NULL, bpf_pc, sim_signalled, SIM_SIGFPE);
      bpf_regs[insn->dst] = (int32_t) bpf_regs[insn->dst] / (int32_t) bpf_regs[insn->src];
      break;
    case BPF_INSN_SDIV32I:
      BPF_TRACE ("BPF_INSN_SDIV32I\n");
      if (insn->imm32 == 0)
        sim_engine_halt (CPU_STATE (cpu), cpu, NULL, bpf_pc, sim_signalled, SIM_SIGFPE);
      bpf_regs[insn->dst] = (int32_t) bpf_regs[insn->dst] / (int32_t) insn->imm32;
      break;
    case BPF_INSN_SMOD32R:
      BPF_TRACE ("BPF_INSN_SMOD32R\n");
      if (bpf_regs[insn->src] == 0)
        sim_engine_halt (CPU_STATE (cpu), cpu, NULL, bpf_pc, sim_signalled, SIM_SIGFPE);
      bpf_regs[insn->dst] = (int32_t) bpf_regs[insn->dst] % (int32_t) bpf_regs[insn->src];
      break;
    case BPF_INSN_SMOD32I:
      BPF_TRACE ("BPF_INSN_SMOD32I\n");
      if (insn->imm32 == 0)
        sim_engine_halt (CPU_STATE (cpu), cpu, NULL, bpf_pc, sim_signalled, SIM_SIGFPE);
      bpf_regs[insn->dst] = (int32_t) bpf_regs[insn->dst] % (int32_t) insn->imm32;
      break;
    case BPF_INSN_NEG32R:
      BPF_TRACE ("BPF_INSN_NEG32R\n");
      bpf_regs[insn->dst] = (uint32_t) (- (int32_t) bpf_regs[insn->dst]);
      break;
    case BPF_INSN_LSH32R:
      BPF_TRACE ("BPF_INSN_LSH32R\n");
      bpf_regs[insn->dst] = (uint32_t) bpf_regs[insn->dst] << bpf_regs[insn->src];
      break;
    case BPF_INSN_LSH32I:
      BPF_TRACE ("BPF_INSN_LSH32I\n");
      bpf_regs[insn->dst] = (uint32_t) bpf_regs[insn->dst] << insn->imm32;
      break;
    case BPF_INSN_RSH32R:
      BPF_TRACE ("BPF_INSN_RSH32R\n");
      bpf_regs[insn->dst] = (uint32_t) bpf_regs[insn->dst] >> bpf_regs[insn->src];
      break;
    case BPF_INSN_RSH32I:
      BPF_TRACE ("BPF_INSN_RSH32I\n");
      bpf_regs[insn->dst] = (uint32_t) bpf_regs[insn->dst] >> insn->imm32;
      break;
    case BPF_INSN_ARSH32R:
      BPF_TRACE ("BPF_INSN_ARSH32R\n");
      bpf_regs[insn->dst] = (uint32_t)((int32_t)(uint32_t) bpf_regs[insn->dst] >> bpf_regs[insn->src]);
      break;
    case BPF_INSN_ARSH32I:
      BPF_TRACE ("BPF_INSN_ARSH32I\n");
      bpf_regs[insn->dst] = (uint32_t)((int32_t)(uint32_t) bpf_regs[insn->dst] >> insn->imm32);
      break;
    case BPF_INSN_MOV32R:
      BPF_TRACE ("BPF_INSN_MOV32R\n");
      bpf_regs[insn->dst] = (uint32_t) bpf_regs[insn->src];
      break;
    case BPF_INSN_MOV32I:
      BPF_TRACE ("BPF_INSN_MOV32I\n");
      bpf_regs[insn->dst] = (uint32_t) insn->imm32;
      break;
      /* Endianness conversion instructions.  */
    case BPF_INSN_ENDLE16:
      BPF_TRACE ("BPF_INSN_ENDLE16\n");
      bpf_regs[insn->dst] = endian_h2le_2 (endian_t2h_2 (bpf_regs[insn->dst]));
      break;
    case BPF_INSN_ENDLE32:
      BPF_TRACE ("BPF_INSN_ENDLE32\n");
      bpf_regs[insn->dst] = endian_h2le_4 (endian_t2h_4 (bpf_regs[insn->dst]));
      break;
    case BPF_INSN_ENDLE64:
      BPF_TRACE ("BPF_INSN_ENDLE64\n");
      bpf_regs[insn->dst] = endian_h2le_8 (endian_t2h_8 (bpf_regs[insn->dst]));
      break;
    case BPF_INSN_ENDBE16:
      BPF_TRACE ("BPF_INSN_ENDBE16\n");
      bpf_regs[insn->dst] = endian_h2be_2 (endian_t2h_2 (bpf_regs[insn->dst]));
      break;
    case BPF_INSN_ENDBE32:
      BPF_TRACE ("BPF_INSN_ENDBE32\n");
      bpf_regs[insn->dst] = endian_h2be_4 (endian_t2h_4 (bpf_regs[insn->dst]));
      break;
    case BPF_INSN_ENDBE64:
      BPF_TRACE ("BPF_INSN_ENDBE64\n");
      bpf_regs[insn->dst] = endian_h2be_8 (endian_t2h_8 (bpf_regs[insn->dst]));
      break;
      /* 64-bit load instruction.  */
    case BPF_INSN_LDDW:
      BPF_TRACE ("BPF_INSN_LDDW\n");
      bpf_regs[insn->dst] = insn->imm64;
      break;
      /* Indirect load instructions.  */
    case BPF_INSN_LDINDB:
      BPF_TRACE ("BPF_INSN_LDINDB\n");
      bpf_regs[BPF_R0] = bpf_read_u8 (cpu,
                                      bpf_read_u64 (cpu, bpf_regs[BPF_R6] + skb_data_offset)
                                      + bpf_regs[insn->src] + insn->imm32);
      break;
    case BPF_INSN_LDINDH:
      BPF_TRACE ("BPF_INSN_LDINDH\n");
      bpf_regs[BPF_R0] = bpf_read_u16 (cpu,
                                       bpf_read_u64 (cpu, bpf_regs[BPF_R6] + skb_data_offset)
                                       + bpf_regs[insn->src] + insn->imm32);
      break;
    case BPF_INSN_LDINDW:
      BPF_TRACE ("BPF_INSN_LDINDW\n");
      bpf_regs[BPF_R0] = bpf_read_u32 (cpu,
                                       bpf_read_u64 (cpu, bpf_regs[BPF_R6] + skb_data_offset)
                                       + bpf_regs[insn->src] + insn->imm32);
      break;
    case BPF_INSN_LDINDDW:
      BPF_TRACE ("BPF_INSN_LDINDDW\n");
      bpf_regs[BPF_R0] = bpf_read_u64 (cpu,
                                       bpf_read_u64 (cpu, bpf_regs[BPF_R6] + skb_data_offset)
                                       + bpf_regs[insn->src] + insn->imm32);
      break;
      /* Absolute load instructions.  */
    case BPF_INSN_LDABSB:
      BPF_TRACE ("BPF_INSN_LDABSB\n");
      bpf_regs[BPF_R0] = bpf_read_u8 (cpu,
                                      bpf_read_u64 (cpu, bpf_regs[BPF_R6] + skb_data_offset)
                                      + insn->imm32);
      break;
    case BPF_INSN_LDABSH:
      BPF_TRACE ("BPF_INSN_LDABSH\n");
      bpf_regs[BPF_R0] = bpf_read_u16 (cpu,
                                       bpf_read_u64 (cpu, bpf_regs[BPF_R6] + skb_data_offset)
                                       + insn->imm32);
      break;
    case BPF_INSN_LDABSW:
      BPF_TRACE ("BPF_INSN_LDABSW\n");
      bpf_regs[BPF_R0] = bpf_read_u32 (cpu,
                                       bpf_read_u64 (cpu, bpf_regs[BPF_R6] + skb_data_offset)
                                       + insn->imm32);
      break;
    case BPF_INSN_LDABSDW:
      BPF_TRACE ("BPF_INSN_LDABSDW\n");
      bpf_regs[BPF_R0] = bpf_read_u64 (cpu,
                                       bpf_read_u64 (cpu, bpf_regs[BPF_R6] + skb_data_offset)
                                       + insn->imm32);
      break;
      /* Generic load instructions (to register.)  */
    case BPF_INSN_LDXB:
      BPF_TRACE ("BPF_INSN_LDXB\n");
      bpf_regs[insn->dst] = (int8_t) bpf_read_u8 (cpu,
                                                  bpf_regs[insn->src] + insn->offset16);
      break;
    case BPF_INSN_LDXH:
      BPF_TRACE ("BPF_INSN_LDXH\n");
      bpf_regs[insn->dst] = (int16_t) bpf_read_u16 (cpu,
                                                    bpf_regs[insn->src] + insn->offset16);
      break;
    case BPF_INSN_LDXW:
      BPF_TRACE ("BPF_INSN_LDXW\n");
      bpf_regs[insn->dst] = (int32_t) bpf_read_u32 (cpu,
                                                    bpf_regs[insn->src] + insn->offset16);
      break;
    case BPF_INSN_LDXDW:
      BPF_TRACE ("BPF_INSN_LDXDW\n");
      bpf_regs[insn->dst] = bpf_read_u64 (cpu,
                                          bpf_regs[insn->src] + insn->offset16);
      break;
      /* Generic store instructions (from register.)  */
    case BPF_INSN_STXBR:
      BPF_TRACE ("BPF_INSN_STXBR\n");
      bpf_write_u8 (cpu,
                    bpf_regs[insn->dst] + insn->offset16,
                    bpf_regs[insn->src]);
      break;
    case BPF_INSN_STXHR:
      BPF_TRACE ("BPF_INSN_STXHR\n");
      bpf_write_u16 (cpu,
                     bpf_regs[insn->dst] + insn->offset16,
                     bpf_regs[insn->src]);
      break;
    case BPF_INSN_STXWR:
      BPF_TRACE ("BPF_INSN_STXWR\n");
      bpf_write_u32 (cpu,
                     bpf_regs[insn->dst] + insn->offset16,
                     bpf_regs[insn->src]);
      break;
    case BPF_INSN_STXDWR:
      BPF_TRACE ("BPF_INSN_STXDWR\n");
      bpf_write_u64 (cpu,
                     bpf_regs[insn->dst] + insn->offset16,
                     bpf_regs[insn->src]);
      break;
      /* Generic store instructions (from 32-bit immediate.) */
    case BPF_INSN_STXBI:
      BPF_TRACE ("BPF_INSN_STXBI\n");
      bpf_write_u8 (cpu,
                    bpf_regs[insn->dst] + insn->offset16,
                    insn->imm32);
      break;
    case BPF_INSN_STXHI:
      BPF_TRACE ("BPF_INSN_STXHI\n");
      bpf_write_u16 (cpu,
                     bpf_regs[insn->dst] + insn->offset16,
                     insn->imm32);
      break;
    case BPF_INSN_STXWI:
      BPF_TRACE ("BPF_INSN_STXWI\n");
      bpf_write_u32 (cpu,
                     bpf_regs[insn->dst] + insn->offset16,
                     insn->imm32);
      break;
    case BPF_INSN_STXDWI:
      BPF_TRACE ("BPF_INSN_STXDWI\n");
      bpf_write_u64 (cpu,
                     bpf_regs[insn->dst] + insn->offset16,
                     insn->imm32);
      break;
      /* Compare-and-jump instructions (reg OP reg).  */
    case BPF_INSN_JAR:
      BPF_TRACE ("BPF_INSN_JAR\n");
      next_pc = bpf_pc + DISP (insn->offset16);
      break;
    case BPF_INSN_JEQR:
      BPF_TRACE ("BPF_INSN_JEQR\n");
      if (bpf_regs[insn->dst] == bpf_regs[insn->src])
        next_pc = bpf_pc + DISP (insn->offset16);
      break;
    case BPF_INSN_JGTR:
      BPF_TRACE ("BPF_INSN_JGTR\n");
      if (bpf_regs[insn->dst] > bpf_regs[insn->src])
        next_pc = bpf_pc + DISP (insn->offset16);
      break;
    case BPF_INSN_JSGTR:
      BPF_TRACE ("BPF_INSN_JSGTR\n");
      if ((int64_t) bpf_regs[insn->dst] > (int64_t) bpf_regs[insn->src])
        next_pc = bpf_pc + DISP (insn->offset16);
      break;
    case BPF_INSN_JGER:
      BPF_TRACE ("BPF_INSN_JGER\n");
      if (bpf_regs[insn->dst] >= bpf_regs[insn->src])
        next_pc = bpf_pc + DISP (insn->offset16);
      break;
    case BPF_INSN_JSGER:
      BPF_TRACE ("BPF_INSN_JSGER\n");
      if ((int64_t) bpf_regs[insn->dst] >= (int64_t) bpf_regs[insn->src])
        next_pc = bpf_pc + DISP (insn->offset16);
      break;
    case BPF_INSN_JLTR:
      BPF_TRACE ("BPF_INSN_JLTR\n");
      if (bpf_regs[insn->dst] < bpf_regs[insn->src])
        next_pc = bpf_pc + DISP (insn->offset16);
      break;
    case BPF_INSN_JSLTR:
      BPF_TRACE ("BPF_INSN_JSLTR\n");
      if ((int64_t) bpf_regs[insn->dst] < (int64_t) bpf_regs[insn->src])
        next_pc = bpf_pc + DISP (insn->offset16);
      break;
    case BPF_INSN_JLER:
      BPF_TRACE ("BPF_INSN_JLER\n");
      if (bpf_regs[insn->dst] <= bpf_regs[insn->src])
        next_pc = bpf_pc + DISP (insn->offset16);
      break;
    case BPF_INSN_JSLER:
      BPF_TRACE ("BPF_INSN_JSLER\n");
      if ((int64_t) bpf_regs[insn->dst] <= (int64_t) bpf_regs[insn->src])
        next_pc = bpf_pc + DISP (insn->offset16);
      break;
    case BPF_INSN_JSETR:
      BPF_TRACE ("BPF_INSN_JSETR\n");
      if (bpf_regs[insn->dst] & bpf_regs[insn->src])
        next_pc = bpf_pc + DISP (insn->offset16);
      break;
    case BPF_INSN_JNER:
      BPF_TRACE ("BPF_INSN_JNER\n");
      if (bpf_regs[insn->dst] != bpf_regs[insn->src])
        next_pc = bpf_pc + DISP (insn->offset16);
      break;
    case BPF_INSN_CALLR:
      BPF_TRACE ("BPF_INSN_CALLR\n");
      bpf_call (cpu, DISP (bpf_regs[insn->dst]), insn->src);
      break;
    case BPF_INSN_CALL:
      BPF_TRACE ("BPF_INSN_CALL\n");
      bpf_call (cpu, insn->imm32, insn->src);
      break;
    case BPF_INSN_EXIT:
      BPF_TRACE ("BPF_INSN_EXIT\n");
      {
        SIM_DESC sd = CPU_STATE (cpu);
        printf ("exit %" PRId64 " (0x%" PRIx64 ")\n",
                bpf_regs[BPF_R0], bpf_regs[BPF_R0]);
        sim_engine_halt (sd, cpu, NULL, bpf_pc,
                         sim_exited, 0 /* sigrc */);
        break;
      }
      /* Compare-and-jump instructions (reg OP imm).  */
    case BPF_INSN_JEQI:
      BPF_TRACE ("BPF_INSN_JEQI\n");
      if (bpf_regs[insn->dst] == insn->imm32)
        next_pc = bpf_pc + DISP (insn->offset16);
      break;
    case BPF_INSN_JGTI:
      BPF_TRACE ("BPF_INSN_JGTI\n");
      if (bpf_regs[insn->dst] > insn->imm32)
        next_pc = bpf_pc + DISP (insn->offset16);
      break;
    case BPF_INSN_JSGTI:
      BPF_TRACE ("BPF_INSN_JSGTI\n");
      if ((int64_t) bpf_regs[insn->dst] > insn->imm32)
        next_pc = bpf_pc + DISP (insn->offset16);
      break;
    case BPF_INSN_JGEI:
      BPF_TRACE ("BPF_INSN_JGEI\n");
      if (bpf_regs[insn->dst] >= insn->imm32)
        next_pc = bpf_pc + DISP (insn->offset16);
      break;
    case BPF_INSN_JSGEI:
      BPF_TRACE ("BPF_INSN_JSGEI\n");
      if ((int64_t) bpf_regs[insn->dst] >= (int64_t) insn->imm32)
        next_pc = bpf_pc + DISP (insn->offset16);
      break;
    case BPF_INSN_JLTI:
      BPF_TRACE ("BPF_INSN_JLTI\n");
      if (bpf_regs[insn->dst] < insn->imm32)
        next_pc = bpf_pc + DISP (insn->offset16);
      break;
    case BPF_INSN_JSLTI:
      BPF_TRACE ("BPF_INSN_JSLTI\n");
      if ((int64_t) bpf_regs[insn->dst] < (int64_t) insn->imm32)
        next_pc = bpf_pc + DISP (insn->offset16);
      break;
    case BPF_INSN_JLEI:
      BPF_TRACE ("BPF_INSN_JLEI\n");
      if (bpf_regs[insn->dst] <= insn->imm32)
        next_pc = bpf_pc + DISP (insn->offset16);
      break;
    case BPF_INSN_JSLEI:
      BPF_TRACE ("BPF_INSN_JSLEI\n");
      if ((int64_t) bpf_regs[insn->dst] <= (int64_t) insn->imm32)
        next_pc = bpf_pc + DISP (insn->offset16);
      break;
    case BPF_INSN_JSETI:
      BPF_TRACE ("BPF_INSN_JSETI\n");
      if (bpf_regs[insn->dst] & insn->imm32)
        next_pc = bpf_pc + DISP (insn->offset16);
      break;
    case BPF_INSN_JNEI:
      BPF_TRACE ("BPF_INSN_JNEI\n");
      if (bpf_regs[insn->dst] != insn->imm32)
        next_pc = bpf_pc + DISP (insn->offset16);
      break;
      /* 32-bit compare-and-jump instructions (reg OP reg).  */
    case BPF_INSN_JEQ32R:
      BPF_TRACE ("BPF_INSN_JEQ32R\n");
      if ((uint32_t) bpf_regs[insn->dst] == (uint32_t) bpf_regs[insn->src])
        next_pc = bpf_pc + DISP (insn->offset16);
      break;
    case BPF_INSN_JGT32R:
      BPF_TRACE ("BPF_INSN_JGT32R\n");
      if ((uint32_t) bpf_regs[insn->dst] > (uint32_t) bpf_regs[insn->src])
        next_pc = bpf_pc + DISP (insn->offset16);
      break;
    case BPF_INSN_JSGT32R:
      BPF_TRACE ("BPF_INSN_JSGT32R\n");
      if ((int32_t) bpf_regs[insn->dst] > (int32_t) bpf_regs[insn->src])
        next_pc = bpf_pc + DISP (insn->offset16);
      break;
    case BPF_INSN_JGE32R:
      BPF_TRACE ("BPF_INSN_JGE32R\n");
      if ((uint32_t) bpf_regs[insn->dst] >= (uint32_t) bpf_regs[insn->src])
        next_pc = bpf_pc + DISP (insn->offset16);
      break;
    case BPF_INSN_JSGE32R:
      BPF_TRACE ("BPF_INSN_JSGE32R\n");
      if ((int32_t) bpf_regs[insn->dst] >= (int32_t) bpf_regs[insn->src])
        next_pc = bpf_pc + DISP (insn->offset16);
      break;
    case BPF_INSN_JLT32R:
      BPF_TRACE ("BPF_INSN_JLT32R\n");
      if ((uint32_t) bpf_regs[insn->dst] < (uint32_t) bpf_regs[insn->src])
        next_pc = bpf_pc + DISP (insn->offset16);
      break;
    case BPF_INSN_JSLT32R:
      BPF_TRACE ("BPF_INSN_JSLT32R\n");
      if ((int32_t) bpf_regs[insn->dst] < (int32_t) bpf_regs[insn->src])
        next_pc = bpf_pc + DISP (insn->offset16);
      break;
    case BPF_INSN_JLE32R:
      BPF_TRACE ("BPF_INSN_JLE32R\n");
      if ((uint32_t) bpf_regs[insn->dst] <= (uint32_t) bpf_regs[insn->src])
        next_pc = bpf_pc + DISP (insn->offset16);
      break;
    case BPF_INSN_JSLE32R:
      BPF_TRACE ("BPF_INSN_JSLE32R\n");
      if ((int32_t) bpf_regs[insn->dst] <= (int32_t) bpf_regs[insn->src])
        next_pc = bpf_pc + DISP (insn->offset16);
      break;
    case BPF_INSN_JSET32R:
      BPF_TRACE ("BPF_INSN_JSET32R\n");
      if ((uint32_t) bpf_regs[insn->dst] & (uint32_t) bpf_regs[insn->src])
        next_pc = bpf_pc + DISP (insn->offset16);
      break;
    case BPF_INSN_JNE32R:
      BPF_TRACE ("BPF_INSN_JNE32R\n");
      if ((uint32_t) bpf_regs[insn->dst] != (uint32_t) bpf_regs[insn->src])
        next_pc = bpf_pc + DISP (insn->offset16);
      break;
      /* 32-bit compare-and-jump instructions (reg OP imm).  */
    case BPF_INSN_JEQ32I:
      BPF_TRACE ("BPF_INSN_JEQ32I\n");
      if ((uint32_t) bpf_regs[insn->dst] == insn->imm32)
        next_pc = bpf_pc + DISP (insn->offset16);
      break;
    case BPF_INSN_JGT32I:
      BPF_TRACE ("BPF_INSN_JGT32I\n");
      if ((uint32_t) bpf_regs[insn->dst] > insn->imm32)
        next_pc = bpf_pc + DISP (insn->offset16);
      break;
    case BPF_INSN_JSGT32I:
      BPF_TRACE ("BPF_INSN_JSGT32I\n");
      if ((int32_t) bpf_regs[insn->dst] > insn->imm32)
        next_pc = bpf_pc + DISP (insn->offset16);
      break;
    case BPF_INSN_JGE32I:
      BPF_TRACE ("BPF_INSN_JGE32I\n");
      if ((uint32_t) bpf_regs[insn->dst] >= insn->imm32)
        next_pc = bpf_pc + DISP (insn->offset16);
      break;
    case BPF_INSN_JSGE32I:
      BPF_TRACE ("BPF_INSN_JSGE32I\n");
      if ((int32_t) bpf_regs[insn->dst] >= (int32_t) insn->imm32)
        next_pc = bpf_pc + DISP (insn->offset16);
      break;
    case BPF_INSN_JLT32I:
      BPF_TRACE ("BPF_INSN_JLT32I\n");
      if ((uint32_t) bpf_regs[insn->dst] < insn->imm32)
        next_pc = bpf_pc + DISP (insn->offset16);
      break;
    case BPF_INSN_JSLT32I:
      BPF_TRACE ("BPF_INSN_JSLT32I\n");
      if ((int32_t) bpf_regs[insn->dst] < (int32_t) insn->imm32)
        next_pc = bpf_pc + DISP (insn->offset16);
      break;
    case BPF_INSN_JLE32I:
      BPF_TRACE ("BPF_INSN_JLE32I\n");
      if ((uint32_t) bpf_regs[insn->dst] <= insn->imm32)
        next_pc = bpf_pc + DISP (insn->offset16);
      break;
    case BPF_INSN_JSLE32I:
      BPF_TRACE ("BPF_INSN_JSLE32I\n");
      if ((int32_t) bpf_regs[insn->dst] <= (int32_t) insn->imm32)
        next_pc = bpf_pc + DISP (insn->offset16);
      break;
    case BPF_INSN_JSET32I:
      BPF_TRACE ("BPF_INSN_JSET32I\n");
      if ((uint32_t) bpf_regs[insn->dst] & insn->imm32)
        next_pc = bpf_pc + DISP (insn->offset16);
      break;
    case BPF_INSN_JNE32I:
      BPF_TRACE ("BPF_INSN_JNE32I\n");
      if ((uint32_t) bpf_regs[insn->dst] != insn->imm32)
        next_pc = bpf_pc + DISP (insn->offset16);
      break;
      /* Atomic instructions.  */
    case BPF_INSN_AADD:
      BPF_TRACE ("BPF_INSN_AADD\n");
      bpf_write_u64 (cpu,
                     bpf_regs[insn->dst] + insn->offset16,
                     bpf_read_u64 (cpu, bpf_regs[insn->dst] + insn->offset16)
                     + bpf_regs[insn->src]);
      break;
    case BPF_INSN_AADD32:
      BPF_TRACE ("BPF_INSN_AADD32\n");
      bpf_write_u32 (cpu,
                     bpf_regs[insn->dst] + insn->offset16,
                     (int32_t) bpf_read_u32 (cpu, bpf_regs[insn->dst] + insn->offset16)
                     + bpf_regs[insn->src]);
      break;
      /* XXX Atomic instructions with fetching.  */
    default: /* XXX */
    case BPF_NOINSN:
      BPF_TRACE ("BPF_NOINSN\n");
      return 0;
      break;
    }

  /* Set new PC.  */
  bpf_pc = next_pc;

  return 1;
}

/* Entry points.  */

SIM_RC
sim_create_inferior (SIM_DESC sd, struct bfd *abfd,
                     char * const *argv, char * const *env)
{
  SIM_CPU *cpu = STATE_CPU (sd, 0);
  bfd_vma addr;

  /* Determine the start address.

     XXX acknowledge bpf_program_section.  If it is NULL, emit a
     warning explaining that we are using the ELF file start address,
     which often is not what is actually wanted.  */
  if (abfd != NULL)
    addr = bfd_get_start_address (abfd);
  else
    addr = 0;

  sim_pc_set (cpu, addr);

  return SIM_RC_OK;
}

/* Like sim_state_free, but free the cpu buffers as well.  */

static void
bpf_free_state (SIM_DESC sd)
{
  if (STATE_MODULES (sd) != NULL)
    sim_module_uninstall (sd);

  sim_cpu_free_all (sd);
  sim_state_free (sd);
}

/* Create an instance of the simulator.  */

SIM_DESC
sim_open (SIM_OPEN_KIND kind, host_callback *cb,
          struct bfd *abfd, char * const *argv)
{
  SIM_DESC sd = sim_state_alloc_extra (kind, cb, sizeof (struct bpf_sim_state));
  SIM_ASSERT (STATE_MAGIC (sd) == SIM_MAGIC_NUMBER);

  /* Set default options before parsing user options.  */
  current_target_byte_order = BFD_ENDIAN_LITTLE;

  if (sim_cpu_alloc_all_extra (sd, 0, sizeof (struct bpf_sim_state)) != SIM_RC_OK)
    goto error;

  if (sim_pre_argv_init (sd, argv[0]) != SIM_RC_OK)
    goto error;

  /* Add the BPF-specific option list to the simulator.  */
  if (sim_add_option_table (sd, NULL, bpf_options) != SIM_RC_OK)
    {
      bpf_free_state (sd);
      return 0;
    }

  /* The parser will print an error message for us, so we silently return.  */
  if (sim_parse_args (sd, argv) != SIM_RC_OK)
    goto error;

  /* Check for/establish the a reference program image.  */
  if (sim_analyze_program (sd, STATE_PROG_FILE (sd), abfd) != SIM_RC_OK)
    goto error;

  /* Configure/verify the target byte order and other runtime
     configuration options.  */
  if (sim_config (sd) != SIM_RC_OK)
    goto error;

  if (sim_post_argv_init (sd) != SIM_RC_OK)
    goto error;

  /* Initialize properties of the simulated CPU.  */

  assert (MAX_NR_PROCESSORS == 1);
  {
    SIM_CPU *cpu = STATE_CPU (sd, i);

    cpu = STATE_CPU (sd, 0);
    CPU_PC_FETCH (cpu) = bpf_pc_get;
    CPU_PC_STORE (cpu) = bpf_pc_set;
    CPU_REG_FETCH (cpu) = bpf_reg_get;
    CPU_REG_STORE (cpu) = bpf_reg_set;
  }

  return sd;

 error:
      bpf_free_state (sd);
      return NULL;
}

void
sim_engine_run (SIM_DESC sd,
                int next_cpu_nr ATTRIBUTE_UNUSED,
                int nr_cpus ATTRIBUTE_UNUSED,
                int siggnal ATTRIBUTE_UNUSED)
{
  SIM_CPU *cpu = STATE_CPU (sd, 0);
  struct bpf_insn insn;

  while (1)
    {
      if (!decode (cpu, bpf_pc, &insn))
        {
          sim_io_eprintf (sd, "couldn't decode instruction at PC 0x%" PRIx64 "\n",
                          bpf_pc);
          break;
        }

      if (!execute (cpu, &insn))
        {
          sim_io_eprintf (sd, "couldn' execute instruction at PC 0x%" PRIx64 "\n",
                          bpf_pc);
          break;
        }
    }
}
