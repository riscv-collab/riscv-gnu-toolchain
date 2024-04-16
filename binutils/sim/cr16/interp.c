/* Simulation code for the CR16 processor.
   Copyright (C) 2008-2024 Free Software Foundation, Inc.
   Contributed by M Ranga Swami Reddy <MR.Swami.Reddy@nsc.com>

   This file is part of GDB, the GNU debugger.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
 
   You should have received a copy of the GNU General Public License 
   along with this program. If not, see <http://www.gnu.org/licenses/>.  */

/* This must come before any other includes.  */
#include "defs.h"

#include <inttypes.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include "bfd.h"
#include "sim/callback.h"
#include "sim/sim.h"

#include "sim-main.h"
#include "sim-options.h"
#include "sim-signal.h"

#include "sim/sim-cr16.h"
#include "gdb/signals.h"
#include "opcode/cr16.h"

#include "target-newlib-syscall.h"

#include "cr16-sim.h"

struct _state State;

int cr16_debug;

uint32_t OP[4];
uint32_t sign_flag;

static struct hash_entry *lookup_hash (SIM_DESC, SIM_CPU *, uint64_t ins, int size);
static void get_operands (operand_desc *s, uint64_t mcode, int isize, int nops);

#define MAX_HASH  16

struct hash_entry
{
  struct hash_entry *next;
  uint32_t opcode;
  uint32_t mask;
  int format;
  int size;
  struct simops *ops;
};

struct hash_entry hash_table[MAX_HASH+1];

INLINE static long
hash(unsigned long long insn, int format)
{ 
  unsigned int i = 4;
  if (format)
    {
      while ((insn >> i) != 0) i +=4;

      return ((insn >> (i-4)) & 0xf); /* Use last 4 bits as hask key.  */
    }
  return ((insn & 0xF)); /* Use last 4 bits as hask key.  */
}


INLINE static struct hash_entry *
lookup_hash (SIM_DESC sd, SIM_CPU *cpu, uint64_t ins, int size)
{
  uint32_t mask;
  struct hash_entry *h;

  h = &hash_table[hash(ins,1)];


  mask = (((1 << (32 - h->mask)) -1) << h->mask);

 /* Adjuest mask for branch with 2 word instructions.  */
  if (streq(h->ops->mnemonic,"b") && h->size == 2)
    mask = 0xff0f0000;


  while ((ins & mask) != (BIN(h->opcode, h->mask)))
    {
      if (h->next == NULL)
	sim_engine_halt (sd, cpu, NULL, PC, sim_stopped, SIM_SIGILL);
      h = h->next;

      mask = (((1 << (32 - h->mask)) -1) << h->mask);
     /* Adjuest mask for branch with 2 word instructions.  */
     if ((streq(h->ops->mnemonic,"b")) && h->size == 2)
       mask = 0xff0f0000;

     }
   return (h);
}

INLINE static void
get_operands (operand_desc *s, uint64_t ins, int isize, int nops)
{
  uint32_t i, opn = 0, start_bit = 0, op_type = 0; 
  int32_t op_size = 0;

  if (isize == 1) /* Trunkcate the extra 16 bits of INS.  */
    ins = ins >> 16;

  for (i=0; i < 4; ++i,++opn)
    {
      if (s[opn].op_type == dummy) break;

      op_type = s[opn].op_type;
      start_bit = s[opn].shift;
      op_size = cr16_optab[op_type].bit_size;

      switch (op_type)
        {
          case imm3: case imm4: case imm5: case imm6:
            {
             if (isize == 1)
               OP[i] = ((ins >> 4) & ((1 << op_size) -1));
             else
               OP[i] = ((ins >> (32 - start_bit)) & ((1 << op_size) -1));

             if (OP[i] & ((long)1 << (op_size -1))) 
               {
                 sign_flag = 1;
                 OP[i] = ~(OP[i]) + 1;
               }
             OP[i] = (unsigned long int)(OP[i] & (((long)1 << op_size) -1));
            }
            break;

          case uimm3: case uimm3_1: case uimm4_1:
             switch (isize)
               {
              case 1:
               OP[i] = ((ins >> 4) & ((1 << op_size) -1)); break;
              case 2:
               OP[i] = ((ins >> (32 - start_bit)) & ((1 << op_size) -1));break;
              default: /* for case 3.  */
               OP[i] = ((ins >> (16 + start_bit)) & ((1 << op_size) -1)); break;
               break;
               }
            break;

          case uimm4:
            switch (isize)
              {
              case 1:
                 if (start_bit == 20)
                   OP[i] = ((ins >> 4) & ((1 << op_size) -1));
                 else
                   OP[i] = (ins & ((1 << op_size) -1));
                 break;
              case 2:
                 OP[i] = ((ins >> start_bit) & ((1 << op_size) -1));
                 break;
              case 3:
                 OP[i] = ((ins >> (start_bit + 16)) & ((1 << op_size) -1));
                 break;
              default:
                 OP[i] = ((ins >> start_bit) & ((1 << op_size) -1));
                 break;
              }
            break;

          case imm16: case uimm16:
            OP[i] = ins & 0xFFFF;
            break;

          case uimm20: case imm20:
            OP[i] = ins & (((long)1 << op_size) - 1);
            break;

          case imm32: case uimm32:
            OP[i] = ins & 0xFFFFFFFF;
            break;

          case uimm5: break; /*NOT USED.  */
            OP[i] = ins & ((1 << op_size) - 1); break;

          case disps5: 
            OP[i] = (ins >> 4) & ((1 << 4) - 1); 
            OP[i] = (OP[i] * 2) + 2;
            if (OP[i] & ((long)1 << 5)) 
              {
                sign_flag = 1;
                OP[i] = ~(OP[i]) + 1;
                OP[i] = (unsigned long int)(OP[i] & 0x1F);
              }
            break;

          case dispe9: 
            OP[i] = ((((ins >> 8) & 0xf) << 4) | (ins & 0xf)); 
            OP[i] <<= 1;
            if (OP[i] & ((long)1 << 8)) 
              {
                sign_flag = 1;
                OP[i] = ~(OP[i]) + 1;
                OP[i] = (unsigned long int)(OP[i] & 0xFF);
              }
            break;

          case disps17: 
            OP[i] = (ins & 0xFFFF);
            if (OP[i] & 1) 
              {
                OP[i] = (OP[i] & 0xFFFE);
                sign_flag = 1;
                OP[i] = ~(OP[i]) + 1;
                OP[i] = (unsigned long int)(OP[i] & 0xFFFF);
              }
            break;

          case disps25: 
            if (isize == 2)
              OP[i] = (ins & 0xFFFFFF);
            else 
              OP[i] = (ins & 0xFFFF) | (((ins >> 24) & 0xf) << 16) |
                      (((ins >> 16) & 0xf) << 20);

            if (OP[i] & 1) 
              {
                OP[i] = (OP[i] & 0xFFFFFE);
                sign_flag = 1;
                OP[i] = ~(OP[i]) + 1;
                OP[i] = (unsigned long int)(OP[i] & 0xFFFFFF);
              }
            break;

          case abs20:
            if (isize == 3)
              OP[i] = (ins) & 0xFFFFF; 
            else
              OP[i] = (ins >> start_bit) & 0xFFFFF;
            break;
          case abs24:
            if (isize == 3)
              OP[i] = ((ins & 0xFFFF) | (((ins >> 16) & 0xf) << 20)
                       | (((ins >> 24) & 0xf) << 16));
            else
              OP[i] = (ins >> 16) & 0xFFFFFF;
            break;

          case rra:
          case rbase: break; /* NOT USED.  */
          case rbase_disps20:  case rbase_dispe20:
          case rpbase_disps20: case rpindex_disps20:
            OP[i] = ((((ins >> 24)&0xf) << 16)|((ins) & 0xFFFF));
            OP[++i] = (ins >> 16) & 0xF;     /* get 4 bit for reg.  */
            break;
          case rpbase_disps0:
            OP[i] = 0;                       /* 4 bit disp const.  */
            OP[++i] = (ins) & 0xF;           /* get 4 bit for reg.  */
            break;
          case rpbase_dispe4:
            OP[i] = ((ins >> 8) & 0xF) * 2;  /* 4 bit disp const.   */
            OP[++i] = (ins) & 0xF;           /* get 4 bit for reg.  */
            break;
          case rpbase_disps4:
            OP[i] = ((ins >> 8) & 0xF);      /* 4 bit disp const.  */
            OP[++i] = (ins) & 0xF;           /* get 4 bit for reg.  */
            break;
          case rpbase_disps16:
            OP[i] = (ins) & 0xFFFF;
            OP[++i] = (ins >> 16) & 0xF;     /* get 4 bit for reg.  */
            break;
          case rpindex_disps0:
            OP[i] = 0;
            OP[++i] = (ins >> 4) & 0xF;      /* get 4 bit for reg.  */
            OP[++i] = (ins >> 8) & 0x1;      /* get 1 bit for index-reg.  */
            break;
          case rpindex_disps14:
            OP[i] = (ins) & 0x3FFF;
            OP[++i] = (ins >> 14) & 0x1;     /* get 1 bit for index-reg.  */
            OP[++i] = (ins >> 16) & 0xF;     /* get 4 bit for reg.  */
            break;
          case rindex7_abs20:
          case rindex8_abs20:
            OP[i] = (ins) & 0xFFFFF;
            OP[++i] = (ins >> 24) & 0x1;     /* get 1 bit for index-reg.  */
            OP[++i] = (ins >> 20) & 0xF;     /* get 4 bit for reg.  */
            break;
          case regr: case regp: case pregr: case pregrp:
              switch(isize)
                {
                  case 1: 
                    if (start_bit == 20) OP[i] = (ins >> 4) & 0xF;
                    else if (start_bit == 16) OP[i] = ins & 0xF;
                    break;
                  case 2: OP[i] = (ins >>  start_bit) & 0xF; break;
                  case 3: OP[i] = (ins >> (start_bit + 16)) & 0xF; break;
                }
               break;
          case cc: 
            {
              if (isize == 1) OP[i] = (ins >> 4) & 0xF;
              else if (isize == 2)  OP[i] = (ins >> start_bit)  & 0xF;
              else  OP[i] = (ins >> (start_bit + 16)) & 0xF; 
              break;
            }
          default: break;
        }
     
      /* For ESC on uimm4_1 operand.  */
      if (op_type == uimm4_1)
        if (OP[i] == 9)
           OP[i] = -1;

      /* For increment by 1.  */
      if ((op_type == pregr) || (op_type == pregrp))
          OP[i] += 1;
   }
  /* FIXME: for tracing, update values that need to be updated each
            instruction decode cycle */
  State.trace.psw = PSR;
}

static int
do_run (SIM_DESC sd, SIM_CPU *cpu, uint64_t mcode)
{
  struct hash_entry *h;

#ifdef DEBUG
  if ((cr16_debug & DEBUG_INSTRUCTION) != 0)
    sim_io_printf (sd, "do_long 0x%" PRIx64 "\n", mcode);
#endif

   h = lookup_hash (sd, cpu, mcode, 1);

  if ((h == NULL) || (h->opcode == 0))
    return 0;

  if (h->size == 3)
    mcode = (mcode << 16) | RW (PC + 4);

  /* Re-set OP list.  */
  OP[0] = OP[1] = OP[2] = OP[3] = sign_flag = 0;

  /* for push/pop/pushrtn with RA instructions. */
  if ((h->format & REG_LIST) && (mcode & 0x800000))
    OP[2] = 1; /* Set 1 for RA operand.  */

  /* numops == 0 means, no operands.  */
  if (((h->ops) != NULL) && (((h->ops)->numops) != 0))
    get_operands ((h->ops)->operands, mcode, h->size, (h->ops)->numops);

  //State.ins_type = h->flags;

  (h->ops->func) (sd, cpu);

  return h->size;
}

static sim_cia
cr16_pc_get (sim_cpu *cpu)
{
  return PC;
}

static void
cr16_pc_set (sim_cpu *cpu, sim_cia pc)
{
  SIM_DESC sd = CPU_STATE (cpu);
  SET_PC (pc);
}

static void
free_state (SIM_DESC sd)
{
  if (STATE_MODULES (sd) != NULL)
    sim_module_uninstall (sd);
  sim_cpu_free_all (sd);
  sim_state_free (sd);
}

static int cr16_reg_fetch (SIM_CPU *, int, void *, int);
static int cr16_reg_store (SIM_CPU *, int, const void *, int);

SIM_DESC
sim_open (SIM_OPEN_KIND kind, struct host_callback_struct *cb,
	  struct bfd *abfd, char * const *argv)
{
  struct simops *s;
  struct hash_entry *h;
  static int init_p = 0;
  int i;
  SIM_DESC sd = sim_state_alloc (kind, cb);
  SIM_ASSERT (STATE_MAGIC (sd) == SIM_MAGIC_NUMBER);

  /* Set default options before parsing user options.  */
  current_target_byte_order = BFD_ENDIAN_LITTLE;
  cb->syscall_map = cb_cr16_syscall_map;

  /* The cpu data is kept in a separately allocated chunk of memory.  */
  if (sim_cpu_alloc_all (sd, 0) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  if (sim_pre_argv_init (sd, argv[0]) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  /* The parser will print an error message for us, so we silently return.  */
  if (sim_parse_args (sd, argv) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  /* Check for/establish the a reference program image.  */
  if (sim_analyze_program (sd, STATE_PROG_FILE (sd), abfd) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  /* Configure/verify the target byte order and other runtime
     configuration options.  */
  if (sim_config (sd) != SIM_RC_OK)
    {
      sim_module_uninstall (sd);
      return 0;
    }

  if (sim_post_argv_init (sd) != SIM_RC_OK)
    {
      /* Uninstall the modules to avoid memory leaks,
	 file descriptor leaks, etc.  */
      sim_module_uninstall (sd);
      return 0;
    }

  /* CPU specific initialization.  */
  for (i = 0; i < MAX_NR_PROCESSORS; ++i)
    {
      SIM_CPU *cpu = STATE_CPU (sd, i);

      CPU_REG_FETCH (cpu) = cr16_reg_fetch;
      CPU_REG_STORE (cpu) = cr16_reg_store;
      CPU_PC_FETCH (cpu) = cr16_pc_get;
      CPU_PC_STORE (cpu) = cr16_pc_set;
    }

  /* The CR16 has an interrupt controller at 0xFC00, but we don't currently
     handle that.  Revisit if anyone ever implements operating mode.  */
  /* cr16 memory: There are three separate cr16 memory regions IMEM,
     UMEM and DMEM.  The IMEM and DMEM are further broken down into
     blocks (very like VM pages).  This might not match the hardware,
     but it matches what the toolchain currently expects.  Ugh.  */
  sim_do_commandf (sd, "memory-size %#x", 20 * 1024 * 1024);

  /* put all the opcodes in the hash table.  */
  if (!init_p++)
    {
      for (s = Simops; s->func; s++)
        {
          switch(32 - s->mask)
            {
            case 0x4:
               h = &hash_table[hash(s->opcode, 0)]; 
               break;

            case 0x7:
               if (((s->opcode << 1) >> 4) != 0)
                  h = &hash_table[hash((s->opcode << 1) >> 4, 0)];
               else
                  h = &hash_table[hash((s->opcode << 1), 0)];
               break;

            case 0x8:
               if ((s->opcode >> 4) != 0)
                  h = &hash_table[hash(s->opcode >> 4, 0)];
               else
                  h = &hash_table[hash(s->opcode, 0)];
               break;

            case 0x9:
               if (((s->opcode  >> 1) >> 4) != 0)
                 h = &hash_table[hash((s->opcode >>1) >> 4, 0)]; 
               else 
                 h = &hash_table[hash((s->opcode >> 1), 0)]; 
               break;

            case 0xa:
               if ((s->opcode >> 8) != 0)
                 h = &hash_table[hash(s->opcode >> 8, 0)];
               else if ((s->opcode >> 4) != 0)
                 h = &hash_table[hash(s->opcode >> 4, 0)];
               else
                 h = &hash_table[hash(s->opcode, 0)]; 
               break;

            case 0xc:
               if ((s->opcode >> 8) != 0)
                 h = &hash_table[hash(s->opcode >> 8, 0)];
               else if ((s->opcode >> 4) != 0)
                 h = &hash_table[hash(s->opcode >> 4, 0)];
               else
                 h = &hash_table[hash(s->opcode, 0)];
               break;

            case 0xd:
               if (((s->opcode >> 1) >> 8) != 0)
                 h = &hash_table[hash((s->opcode >>1) >> 8, 0)];
               else if (((s->opcode >> 1) >> 4) != 0)
                 h = &hash_table[hash((s->opcode >>1) >> 4, 0)];
               else
                 h = &hash_table[hash((s->opcode >>1), 0)];
               break;

            case 0x10:
               if ((s->opcode >> 0xc) != 0)
                 h = &hash_table[hash(s->opcode >> 12, 0)]; 
               else if ((s->opcode >> 8) != 0)
                 h = &hash_table[hash(s->opcode >> 8, 0)];
               else if ((s->opcode >> 4) != 0)
                 h = &hash_table[hash(s->opcode >> 4, 0)];
               else 
                 h = &hash_table[hash(s->opcode, 0)];
               break;

            case 0x14:
               if ((s->opcode >> 16) != 0)
                 h = &hash_table[hash(s->opcode >> 16, 0)];
               else if ((s->opcode >> 12) != 0)
                 h = &hash_table[hash(s->opcode >> 12, 0)];
               else if ((s->opcode >> 8) != 0)
                 h = &hash_table[hash(s->opcode >> 8, 0)];
               else if ((s->opcode >> 4) != 0)
                 h = &hash_table[hash(s->opcode >> 4, 0)];
               else 
                 h = &hash_table[hash(s->opcode, 0)];
               break;

            default:
              continue;
            }
      
          /* go to the last entry in the chain.  */
          while (h->next)
            h = h->next;

          if (h->ops)
            {
              h->next = (struct hash_entry *) calloc(1,sizeof(struct hash_entry));
              if (!h->next)
                perror ("malloc failure");

              h = h->next;
            }
          h->ops = s;
          h->mask = s->mask;
          h->opcode = s->opcode;
          h->format = s->format;
          h->size = s->size;
        }
    }

  return sd;
}

static void
step_once (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t curr_ins_size = 0;
  uint64_t mcode = RLW (PC);

  State.pc_changed = 0;

  curr_ins_size = do_run (sd, cpu, mcode);

#if CR16_DEBUG
  sim_io_printf (sd, "INS: PC=0x%X, mcode=0x%X\n", PC, mcode);
#endif

  if (curr_ins_size == 0)
    sim_engine_halt (sd, cpu, NULL, PC, sim_exited, GPR (2));
  else if (!State.pc_changed)
    SET_PC (PC + (curr_ins_size * 2)); /* For word instructions.  */

#if 0
  /* Check for a breakpoint trap on this instruction.  This
     overrides any pending branches or loops */
  if (PSR_DB && PC == DBS)
    {
      SET_BPC (PC);
      SET_BPSR (PSR);
      SET_PC (SDBT_VECTOR_START);
    }
#endif

  /* Writeback all the DATA / PC changes */
  SLOT_FLUSH ();
}

void
sim_engine_run (SIM_DESC sd,
		int next_cpu_nr,  /* ignore  */
		int nr_cpus,      /* ignore  */
		int siggnal)
{
  sim_cpu *cpu;

  SIM_ASSERT (STATE_MAGIC (sd) == SIM_MAGIC_NUMBER);

  cpu = STATE_CPU (sd, 0);

  switch (siggnal)
    {
    case 0:
      break;
    case GDB_SIGNAL_BUS:
    case GDB_SIGNAL_SEGV:
      SET_PC (PC);
      SET_PSR (PSR);
      JMP (AE_VECTOR_START);
      SLOT_FLUSH ();
      break;
    case GDB_SIGNAL_ILL:
      SET_PC (PC);
      SET_PSR (PSR);
      SET_HW_PSR ((PSR & (PSR_C_BIT)));
      JMP (RIE_VECTOR_START);
      SLOT_FLUSH ();
      break;
    default:
      /* just ignore it */
      break;
    }

  while (1)
    {
      step_once (sd, cpu);
      if (sim_events_tick (sd))
	sim_events_process (sd);
    }
}

SIM_RC
sim_create_inferior (SIM_DESC sd, struct bfd *abfd,
		     char * const *argv, char * const *env)
{
  bfd_vma start_address;

  /* reset all state information */
  memset (&State, 0, sizeof (State));

  /* There was a hack here to copy the values of argc and argv into r0
     and r1.  The values were also saved into some high memory that
     won't be overwritten by the stack (0x7C00).  The reason for doing
     this was to allow the 'run' program to accept arguments.  Without
     the hack, this is not possible anymore.  If the simulator is run
     from the debugger, arguments cannot be passed in, so this makes
     no difference.  */

  /* set PC */
  if (abfd != NULL)
    start_address = bfd_get_start_address (abfd);
  else
    start_address = 0x0;
#ifdef DEBUG
  if (cr16_debug)
    sim_io_printf (sd, "sim_create_inferior:  PC=0x%" PRIx64 "\n",
		   (uint64_t) start_address);
#endif
  {
    SIM_CPU *cpu = STATE_CPU (sd, 0);
    SET_CREG (PC_CR, start_address);
  }

  SLOT_FLUSH ();
  return SIM_RC_OK;
}

static uint32_t
cr16_extract_unsigned_integer (const unsigned char *addr, int len)
{
  uint32_t retval;
  unsigned char * p;
  unsigned char * startaddr = (unsigned char *)addr;
  unsigned char * endaddr = startaddr + len;

  retval = 0;

  for (p = endaddr; p > startaddr;)
    retval = (retval << 8) | *--p;

  return retval;
}

static void
cr16_store_unsigned_integer (unsigned char *addr, int len, uint32_t val)
{
  unsigned char *p;
  unsigned char *startaddr = addr;
  unsigned char *endaddr = startaddr + len;

  for (p = startaddr; p < endaddr;)
    {
      *p++ = val & 0xff;
      val >>= 8;
    }
}

static int
cr16_reg_fetch (SIM_CPU *cpu, int rn, void *memory, int length)
{
  int size;
  switch ((enum sim_cr16_regs) rn)
    {
    case SIM_CR16_R0_REGNUM:
    case SIM_CR16_R1_REGNUM:
    case SIM_CR16_R2_REGNUM:
    case SIM_CR16_R3_REGNUM:
    case SIM_CR16_R4_REGNUM:
    case SIM_CR16_R5_REGNUM:
    case SIM_CR16_R6_REGNUM:
    case SIM_CR16_R7_REGNUM:
    case SIM_CR16_R8_REGNUM:
    case SIM_CR16_R9_REGNUM:
    case SIM_CR16_R10_REGNUM:
    case SIM_CR16_R11_REGNUM:
      cr16_store_unsigned_integer (memory, 2, GPR (rn - SIM_CR16_R0_REGNUM));
      size = 2;
      break;
    case SIM_CR16_R12_REGNUM:
    case SIM_CR16_R13_REGNUM:
    case SIM_CR16_R14_REGNUM:
    case SIM_CR16_R15_REGNUM:
      cr16_store_unsigned_integer (memory, 4, GPR (rn - SIM_CR16_R0_REGNUM));
      size = 4;
      break;
    case SIM_CR16_PC_REGNUM:
    case SIM_CR16_ISP_REGNUM:
    case SIM_CR16_USP_REGNUM:
    case SIM_CR16_INTBASE_REGNUM:
    case SIM_CR16_PSR_REGNUM:
    case SIM_CR16_CFG_REGNUM:
    case SIM_CR16_DBS_REGNUM:
    case SIM_CR16_DCR_REGNUM:
    case SIM_CR16_DSR_REGNUM:
    case SIM_CR16_CAR0_REGNUM:
    case SIM_CR16_CAR1_REGNUM:
      cr16_store_unsigned_integer (memory, 4, CREG (rn - SIM_CR16_PC_REGNUM));
      size = 4;
      break;
    default:
      size = 0;
      break;
    }
  return size;
}

static int
cr16_reg_store (SIM_CPU *cpu, int rn, const void *memory, int length)
{
  SIM_DESC sd = CPU_STATE (cpu);
  int size;
  switch ((enum sim_cr16_regs) rn)
    {
    case SIM_CR16_R0_REGNUM:
    case SIM_CR16_R1_REGNUM:
    case SIM_CR16_R2_REGNUM:
    case SIM_CR16_R3_REGNUM:
    case SIM_CR16_R4_REGNUM:
    case SIM_CR16_R5_REGNUM:
    case SIM_CR16_R6_REGNUM:
    case SIM_CR16_R7_REGNUM:
    case SIM_CR16_R8_REGNUM:
    case SIM_CR16_R9_REGNUM:
    case SIM_CR16_R10_REGNUM:
    case SIM_CR16_R11_REGNUM:
      SET_GPR (rn - SIM_CR16_R0_REGNUM, cr16_extract_unsigned_integer (memory, 2));
      size = 2;
      break;
    case SIM_CR16_R12_REGNUM:
    case SIM_CR16_R13_REGNUM:
    case SIM_CR16_R14_REGNUM:
    case SIM_CR16_R15_REGNUM:
      SET_GPR32 (rn - SIM_CR16_R0_REGNUM, cr16_extract_unsigned_integer (memory, 2));
      size = 4;
      break;
    case SIM_CR16_PC_REGNUM:
    case SIM_CR16_ISP_REGNUM:
    case SIM_CR16_USP_REGNUM:
    case SIM_CR16_INTBASE_REGNUM:
    case SIM_CR16_PSR_REGNUM:
    case SIM_CR16_CFG_REGNUM:
    case SIM_CR16_DBS_REGNUM:
    case SIM_CR16_DCR_REGNUM:
    case SIM_CR16_DSR_REGNUM:
    case SIM_CR16_CAR0_REGNUM:
    case SIM_CR16_CAR1_REGNUM:
      SET_CREG (rn - SIM_CR16_PC_REGNUM, cr16_extract_unsigned_integer (memory, 4));
      size = 4;
      break;
    default:
      size = 0;
      break;
    }
  SLOT_FLUSH ();
  return size;
}
