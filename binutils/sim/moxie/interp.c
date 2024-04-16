/* Simulator for the moxie processor
   Copyright (C) 2008-2024 Free Software Foundation, Inc.
   Contributed by Anthony Green

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

#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>
#include "bfd.h"
#include "libiberty.h"
#include "sim/sim.h"

#include "sim-main.h"
#include "sim-base.h"
#include "sim-options.h"
#include "sim-io.h"
#include "sim-signal.h"
#include "target-newlib-syscall.h"

#include "moxie-sim.h"

/* Extract the signed 10-bit offset from a 16-bit branch
   instruction.  */
#define INST2OFFSET(o) ((((signed short)((o & ((1<<10)-1))<<6))>>6)<<1)

#define EXTRACT_WORD(addr) \
  ((sim_core_read_aligned_1 (scpu, cia, read_map, addr) << 24) \
   + (sim_core_read_aligned_1 (scpu, cia, read_map, addr+1) << 16) \
   + (sim_core_read_aligned_1 (scpu, cia, read_map, addr+2) << 8) \
   + (sim_core_read_aligned_1 (scpu, cia, read_map, addr+3)))

#define EXTRACT_OFFSET(addr)						\
  (unsigned int)							\
  (((signed short)							\
    ((sim_core_read_aligned_1 (scpu, cia, read_map, addr) << 8)		\
     + (sim_core_read_aligned_1 (scpu, cia, read_map, addr+1))) << 16) >> 16)

static unsigned long
moxie_extract_unsigned_integer (const unsigned char *addr, int len)
{
  unsigned long retval;
  unsigned char * p;
  unsigned char * startaddr = (unsigned char *)addr;
  unsigned char * endaddr = startaddr + len;
 
  if (len > (int) sizeof (unsigned long))
    printf ("That operation is not available on integers of more than %zu bytes.",
	    sizeof (unsigned long));
 
  /* Start at the most significant end of the integer, and work towards
     the least significant.  */
  retval = 0;

  for (p = endaddr; p > startaddr;)
    retval = (retval << 8) | * -- p;
  
  return retval;
}

static void
moxie_store_unsigned_integer (unsigned char *addr, int len, unsigned long val)
{
  unsigned char * p;
  unsigned char * startaddr = (unsigned char *)addr;
  unsigned char * endaddr = startaddr + len;

  for (p = endaddr; p > startaddr;)
    {
      * -- p = val & 0xff;
      val >>= 8;
    }
}

/* The machine state.

   This state is maintained in host byte order.  The fetch/store
   register functions must translate between host byte order and the
   target processor byte order.  Keeping this data in target byte
   order simplifies the register read/write functions.  Keeping this
   data in native order improves the performance of the simulator.
   Simulation speed is deemed more important.  */

#define NUM_MOXIE_REGS 17 /* Including PC */
#define NUM_MOXIE_SREGS 256 /* The special registers */
#define PC_REGNO     16

/* The ordering of the moxie_regset structure is matched in the
   gdb/config/moxie/tm-moxie.h file in the REGISTER_NAMES macro.  */
/* TODO: This should be moved to sim-main.h:_sim_cpu.  */
struct moxie_regset
{
  int32_t	     regs[NUM_MOXIE_REGS + 1]; /* primary registers */
  int32_t	     sregs[256];           /* special registers */
  int32_t	     cc;                   /* the condition code reg */
  unsigned long long insts;                /* instruction counter */
};

#define CC_GT  1<<0
#define CC_LT  1<<1
#define CC_EQ  1<<2
#define CC_GTU 1<<3
#define CC_LTU 1<<4

/* TODO: This should be moved to sim-main.h:moxie_sim_cpu.  */
union
{
  struct moxie_regset asregs;
  int32_t asints [1];		/* but accessed larger... */
} cpu;

static void
set_initial_gprs (void)
{
  int i;
  
  /* Set up machine just out of reset.  */
  cpu.asregs.regs[PC_REGNO] = 0;
  
  /* Clean out the register contents.  */
  for (i = 0; i < NUM_MOXIE_REGS; i++)
    cpu.asregs.regs[i] = 0;
  for (i = 0; i < NUM_MOXIE_SREGS; i++)
    cpu.asregs.sregs[i] = 0;
}

/* Write a 1 byte value to memory.  */

static INLINE void
wbat (sim_cpu *scpu, int32_t pc, int32_t x, int32_t v)
{
  address_word cia = CPU_PC_GET (scpu);
  
  sim_core_write_aligned_1 (scpu, cia, write_map, x, v);
}

/* Write a 2 byte value to memory.  */

static INLINE void
wsat (sim_cpu *scpu, int32_t pc, int32_t x, int32_t v)
{
  address_word cia = CPU_PC_GET (scpu);
  
  sim_core_write_aligned_2 (scpu, cia, write_map, x, v);
}

/* Write a 4 byte value to memory.  */

static INLINE void
wlat (sim_cpu *scpu, int32_t pc, int32_t x, int32_t v)
{
  address_word cia = CPU_PC_GET (scpu);
	
  sim_core_write_aligned_4 (scpu, cia, write_map, x, v);
}

/* Read 2 bytes from memory.  */

static INLINE int
rsat (sim_cpu *scpu, int32_t pc, int32_t x)
{
  address_word cia = CPU_PC_GET (scpu);
  
  return (sim_core_read_aligned_2 (scpu, cia, read_map, x));
}

/* Read 1 byte from memory.  */

static INLINE int
rbat (sim_cpu *scpu, int32_t pc, int32_t x)
{
  address_word cia = CPU_PC_GET (scpu);
  
  return (sim_core_read_aligned_1 (scpu, cia, read_map, x));
}

/* Read 4 bytes from memory.  */

static INLINE int
rlat (sim_cpu *scpu, int32_t pc, int32_t x)
{
  address_word cia = CPU_PC_GET (scpu);
  
  return (sim_core_read_aligned_4 (scpu, cia, read_map, x));
}

#define CHECK_FLAG(T,H) if (tflags & T) { hflags |= H; tflags ^= T; }

static unsigned int
convert_target_flags (unsigned int tflags)
{
  unsigned int hflags = 0x0;

  CHECK_FLAG(0x0001, O_WRONLY);
  CHECK_FLAG(0x0002, O_RDWR);
  CHECK_FLAG(0x0008, O_APPEND);
  CHECK_FLAG(0x0200, O_CREAT);
  CHECK_FLAG(0x0400, O_TRUNC);
  CHECK_FLAG(0x0800, O_EXCL);
  CHECK_FLAG(0x2000, O_SYNC);

  if (tflags != 0x0)
    fprintf (stderr, 
	     "Simulator Error: problem converting target open flags for host.  0x%x\n", 
	     tflags);

  return hflags;
}

/* TODO: Split this up into finger trace levels than just insn.  */
#define MOXIE_TRACE_INSN(str) \
  TRACE_INSN (scpu, "0x%08x, %s, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x", \
	      opc, str, cpu.asregs.regs[0], cpu.asregs.regs[1], \
	      cpu.asregs.regs[2], cpu.asregs.regs[3], cpu.asregs.regs[4], \
	      cpu.asregs.regs[5], cpu.asregs.regs[6], cpu.asregs.regs[7], \
	      cpu.asregs.regs[8], cpu.asregs.regs[9], cpu.asregs.regs[10], \
	      cpu.asregs.regs[11], cpu.asregs.regs[12], cpu.asregs.regs[13], \
	      cpu.asregs.regs[14], cpu.asregs.regs[15])

void
sim_engine_run (SIM_DESC sd,
		int next_cpu_nr, /* ignore  */
		int nr_cpus, /* ignore  */
		int siggnal) /* ignore  */
{
  int32_t pc, opc;
  unsigned short inst;
  sim_cpu *scpu = STATE_CPU (sd, 0); /* FIXME */
  address_word cia = CPU_PC_GET (scpu);

  pc = cpu.asregs.regs[PC_REGNO];

  /* Run instructions here. */
  do 
    {
      opc = pc;

      /* Fetch the instruction at pc.  */
      inst = (sim_core_read_aligned_1 (scpu, cia, read_map, pc) << 8)
	+ sim_core_read_aligned_1 (scpu, cia, read_map, pc+1);

      /* Decode instruction.  */
      if (inst & (1 << 15))
	{
	  if (inst & (1 << 14))
	    {
	      /* This is a Form 3 instruction.  */
	      int opcode = (inst >> 10 & 0xf);

	      switch (opcode)
		{
		case 0x00: /* beq */
		  {
		    MOXIE_TRACE_INSN ("beq");
		    if (cpu.asregs.cc & CC_EQ)
		      pc += INST2OFFSET(inst);
		  }
		  break;
		case 0x01: /* bne */
		  {
		    MOXIE_TRACE_INSN ("bne");
		    if (! (cpu.asregs.cc & CC_EQ))
		      pc += INST2OFFSET(inst);
		  }
		  break;
		case 0x02: /* blt */
		  {
		    MOXIE_TRACE_INSN ("blt");
		    if (cpu.asregs.cc & CC_LT)
		      pc += INST2OFFSET(inst);
		  }		  break;
		case 0x03: /* bgt */
		  {
		    MOXIE_TRACE_INSN ("bgt");
		    if (cpu.asregs.cc & CC_GT)
		      pc += INST2OFFSET(inst);
		  }
		  break;
		case 0x04: /* bltu */
		  {
		    MOXIE_TRACE_INSN ("bltu");
		    if (cpu.asregs.cc & CC_LTU)
		      pc += INST2OFFSET(inst);
		  }
		  break;
		case 0x05: /* bgtu */
		  {
		    MOXIE_TRACE_INSN ("bgtu");
		    if (cpu.asregs.cc & CC_GTU)
		      pc += INST2OFFSET(inst);
		  }
		  break;
		case 0x06: /* bge */
		  {
		    MOXIE_TRACE_INSN ("bge");
		    if (cpu.asregs.cc & (CC_GT | CC_EQ))
		      pc += INST2OFFSET(inst);
		  }
		  break;
		case 0x07: /* ble */
		  {
		    MOXIE_TRACE_INSN ("ble");
		    if (cpu.asregs.cc & (CC_LT | CC_EQ))
		      pc += INST2OFFSET(inst);
		  }
		  break;
		case 0x08: /* bgeu */
		  {
		    MOXIE_TRACE_INSN ("bgeu");
		    if (cpu.asregs.cc & (CC_GTU | CC_EQ))
		      pc += INST2OFFSET(inst);
		  }
		  break;
		case 0x09: /* bleu */
		  {
		    MOXIE_TRACE_INSN ("bleu");
		    if (cpu.asregs.cc & (CC_LTU | CC_EQ))
		      pc += INST2OFFSET(inst);
		  }
		  break;
		default:
		  {
		    MOXIE_TRACE_INSN ("SIGILL3");
		    sim_engine_halt (sd, scpu, NULL, pc, sim_stopped, SIM_SIGILL);
		    break;
		  }
		}
	    }
	  else
	    {
	      /* This is a Form 2 instruction.  */
	      int opcode = (inst >> 12 & 0x3);
	      switch (opcode)
		{
		case 0x00: /* inc */
		  {
		    int a = (inst >> 8) & 0xf;
		    unsigned av = cpu.asregs.regs[a];
		    unsigned v = (inst & 0xff);

		    MOXIE_TRACE_INSN ("inc");
		    cpu.asregs.regs[a] = av + v;
		  }
		  break;
		case 0x01: /* dec */
		  {
		    int a = (inst >> 8) & 0xf;
		    unsigned av = cpu.asregs.regs[a];
		    unsigned v = (inst & 0xff);

		    MOXIE_TRACE_INSN ("dec");
		    cpu.asregs.regs[a] = av - v;
		  }
		  break;
		case 0x02: /* gsr */
		  {
		    int a = (inst >> 8) & 0xf;
		    unsigned v = (inst & 0xff);

		    MOXIE_TRACE_INSN ("gsr");
		    cpu.asregs.regs[a] = cpu.asregs.sregs[v];
		  }
		  break;
		case 0x03: /* ssr */
		  {
		    int a = (inst >> 8) & 0xf;
		    unsigned v = (inst & 0xff);

		    MOXIE_TRACE_INSN ("ssr");
		    cpu.asregs.sregs[v] = cpu.asregs.regs[a];
		  }
		  break;
		default:
		  MOXIE_TRACE_INSN ("SIGILL2");
		  sim_engine_halt (sd, scpu, NULL, pc, sim_stopped, SIM_SIGILL);
		  break;
		}
	    }
	}
      else
	{
	  /* This is a Form 1 instruction.  */
	  int opcode = inst >> 8;
	  switch (opcode)
	    {
	    case 0x00: /* bad */
	      opc = opcode;
	      MOXIE_TRACE_INSN ("SIGILL0");
	      sim_engine_halt (sd, scpu, NULL, pc, sim_stopped, SIM_SIGILL);
	      break;
	    case 0x01: /* ldi.l (immediate) */
	      {
		int reg = (inst >> 4) & 0xf;
		unsigned int val = EXTRACT_WORD(pc+2);

		MOXIE_TRACE_INSN ("ldi.l");
		cpu.asregs.regs[reg] = val;
		pc += 4;
	      }
	      break;
	    case 0x02: /* mov (register-to-register) */
	      {
		int dest  = (inst >> 4) & 0xf;
		int src = (inst ) & 0xf;

		MOXIE_TRACE_INSN ("mov");
		cpu.asregs.regs[dest] = cpu.asregs.regs[src];
	      }
	      break;
 	    case 0x03: /* jsra */
 	      {
 		unsigned int fn = EXTRACT_WORD(pc+2);
 		unsigned int sp = cpu.asregs.regs[1];

		MOXIE_TRACE_INSN ("jsra");
 		/* Save a slot for the static chain.  */
		sp -= 4;

 		/* Push the return address.  */
		sp -= 4;
 		wlat (scpu, opc, sp, pc + 6);
 		
 		/* Push the current frame pointer.  */
 		sp -= 4;
 		wlat (scpu, opc, sp, cpu.asregs.regs[0]);
 
 		/* Uncache the stack pointer and set the pc and $fp.  */
		cpu.asregs.regs[1] = sp;
		cpu.asregs.regs[0] = sp;
 		pc = fn - 2;
 	      }
 	      break;
 	    case 0x04: /* ret */
 	      {
 		unsigned int sp = cpu.asregs.regs[0];

		MOXIE_TRACE_INSN ("ret");
 
 		/* Pop the frame pointer.  */
 		cpu.asregs.regs[0] = rlat (scpu, opc, sp);
 		sp += 4;
 		
 		/* Pop the return address.  */
 		pc = rlat (scpu, opc, sp) - 2;
 		sp += 4;

		/* Skip over the static chain slot.  */
		sp += 4;
 
 		/* Uncache the stack pointer.  */
 		cpu.asregs.regs[1] = sp;
  	      }
  	      break;
	    case 0x05: /* add.l */
	      {
		int a = (inst >> 4) & 0xf;
		int b = inst & 0xf;
		unsigned av = cpu.asregs.regs[a];
		unsigned bv = cpu.asregs.regs[b];

		MOXIE_TRACE_INSN ("add.l");
		cpu.asregs.regs[a] = av + bv;
	      }
	      break;
	    case 0x06: /* push */
	      {
		int a = (inst >> 4) & 0xf;
		int b = inst & 0xf;
		int sp = cpu.asregs.regs[a] - 4;

		MOXIE_TRACE_INSN ("push");
		wlat (scpu, opc, sp, cpu.asregs.regs[b]);
		cpu.asregs.regs[a] = sp;
	      }
	      break;
	    case 0x07: /* pop */
	      {
		int a = (inst >> 4) & 0xf;
		int b = inst & 0xf;
		int sp = cpu.asregs.regs[a];

		MOXIE_TRACE_INSN ("pop");
		cpu.asregs.regs[b] = rlat (scpu, opc, sp);
		cpu.asregs.regs[a] = sp + 4;
	      }
	      break;
	    case 0x08: /* lda.l */
	      {
		int reg = (inst >> 4) & 0xf;
		unsigned int addr = EXTRACT_WORD(pc+2);

		MOXIE_TRACE_INSN ("lda.l");
		cpu.asregs.regs[reg] = rlat (scpu, opc, addr);
		pc += 4;
	      }
	      break;
	    case 0x09: /* sta.l */
	      {
		int reg = (inst >> 4) & 0xf;
		unsigned int addr = EXTRACT_WORD(pc+2);

		MOXIE_TRACE_INSN ("sta.l");
		wlat (scpu, opc, addr, cpu.asregs.regs[reg]);
		pc += 4;
	      }
	      break;
	    case 0x0a: /* ld.l (register indirect) */
	      {
		int src  = inst & 0xf;
		int dest = (inst >> 4) & 0xf;
		int xv;

		MOXIE_TRACE_INSN ("ld.l");
		xv = cpu.asregs.regs[src];
		cpu.asregs.regs[dest] = rlat (scpu, opc, xv);
	      }
	      break;
	    case 0x0b: /* st.l */
	      {
		int dest = (inst >> 4) & 0xf;
		int val  = inst & 0xf;

		MOXIE_TRACE_INSN ("st.l");
		wlat (scpu, opc, cpu.asregs.regs[dest], cpu.asregs.regs[val]);
	      }
	      break;
	    case 0x0c: /* ldo.l */
	      {
		unsigned int addr = EXTRACT_OFFSET(pc+2);
		int a = (inst >> 4) & 0xf;
		int b = inst & 0xf;

		MOXIE_TRACE_INSN ("ldo.l");
		addr += cpu.asregs.regs[b];
		cpu.asregs.regs[a] = rlat (scpu, opc, addr);
		pc += 2;
	      }
	      break;
	    case 0x0d: /* sto.l */
	      {
		unsigned int addr = EXTRACT_OFFSET(pc+2);
		int a = (inst >> 4) & 0xf;
		int b = inst & 0xf;

		MOXIE_TRACE_INSN ("sto.l");
		addr += cpu.asregs.regs[a];
		wlat (scpu, opc, addr, cpu.asregs.regs[b]);
		pc += 2;
	      }
	      break;
	    case 0x0e: /* cmp */
	      {
		int a  = (inst >> 4) & 0xf;
		int b  = inst & 0xf;
		int cc = 0;
		int va = cpu.asregs.regs[a];
		int vb = cpu.asregs.regs[b]; 

		MOXIE_TRACE_INSN ("cmp");
		if (va == vb)
		  cc = CC_EQ;
		else
		  {
		    cc |= (va < vb ? CC_LT : 0);
		    cc |= (va > vb ? CC_GT : 0);
		    cc |= ((unsigned int) va < (unsigned int) vb ? CC_LTU : 0);
		    cc |= ((unsigned int) va > (unsigned int) vb ? CC_GTU : 0);
		  }

		cpu.asregs.cc = cc;
	      }
	      break;
	    case 0x0f: /* nop */
	      break;
	    case 0x10: /* sex.b */
	      {
		int a = (inst >> 4) & 0xf;
		int b = inst & 0xf;
		signed char bv = cpu.asregs.regs[b];

		MOXIE_TRACE_INSN ("sex.b");
		cpu.asregs.regs[a] = (int) bv;
	      }
	      break;
	    case 0x11: /* sex.s */
	      {
		int a = (inst >> 4) & 0xf;
		int b = inst & 0xf;
		signed short bv = cpu.asregs.regs[b];

		MOXIE_TRACE_INSN ("sex.s");
		cpu.asregs.regs[a] = (int) bv;
	      }
	      break;
	    case 0x12: /* zex.b */
	      {
		int a = (inst >> 4) & 0xf;
		int b = inst & 0xf;
		signed char bv = cpu.asregs.regs[b];

		MOXIE_TRACE_INSN ("zex.b");
		cpu.asregs.regs[a] = (int) bv & 0xff;
	      }
	      break;
	    case 0x13: /* zex.s */
	      {
		int a = (inst >> 4) & 0xf;
		int b = inst & 0xf;
		signed short bv = cpu.asregs.regs[b];

		MOXIE_TRACE_INSN ("zex.s");
		cpu.asregs.regs[a] = (int) bv & 0xffff;
	      }
	      break;
	    case 0x14: /* umul.x */
	      {
		int a = (inst >> 4) & 0xf;
		int b = inst & 0xf;
		unsigned av = cpu.asregs.regs[a];
		unsigned bv = cpu.asregs.regs[b];
		unsigned long long r = 
		  (unsigned long long) av * (unsigned long long) bv;

		MOXIE_TRACE_INSN ("umul.x");
		cpu.asregs.regs[a] = r >> 32;
	      }
	      break;
	    case 0x15: /* mul.x */
	      {
		int a = (inst >> 4) & 0xf;
		int b = inst & 0xf;
		unsigned av = cpu.asregs.regs[a];
		unsigned bv = cpu.asregs.regs[b];
		signed long long r = 
		  (signed long long) av * (signed long long) bv;

		MOXIE_TRACE_INSN ("mul.x");
		cpu.asregs.regs[a] = r >> 32;
	      }
	      break;
	    case 0x16: /* bad */
	    case 0x17: /* bad */
	    case 0x18: /* bad */
	      {
		opc = opcode;
		MOXIE_TRACE_INSN ("SIGILL0");
		sim_engine_halt (sd, scpu, NULL, pc, sim_stopped, SIM_SIGILL);
		break;
	      }
	    case 0x19: /* jsr */
	      {
		unsigned int fn = cpu.asregs.regs[(inst >> 4) & 0xf];
		unsigned int sp = cpu.asregs.regs[1];

		MOXIE_TRACE_INSN ("jsr");

 		/* Save a slot for the static chain.  */
		sp -= 4;

		/* Push the return address.  */
		sp -= 4;
		wlat (scpu, opc, sp, pc + 2);
		
		/* Push the current frame pointer.  */
		sp -= 4;
		wlat (scpu, opc, sp, cpu.asregs.regs[0]);

		/* Uncache the stack pointer and set the fp & pc.  */
		cpu.asregs.regs[1] = sp;
		cpu.asregs.regs[0] = sp;
		pc = fn - 2;
	      }
	      break;
	    case 0x1a: /* jmpa */
	      {
		unsigned int tgt = EXTRACT_WORD(pc+2);

		MOXIE_TRACE_INSN ("jmpa");
		pc = tgt - 2;
	      }
	      break;
	    case 0x1b: /* ldi.b (immediate) */
	      {
		int reg = (inst >> 4) & 0xf;
		unsigned int val = EXTRACT_WORD(pc+2);

		MOXIE_TRACE_INSN ("ldi.b");
		cpu.asregs.regs[reg] = val;
		pc += 4;
	      }
	      break;
	    case 0x1c: /* ld.b (register indirect) */
	      {
		int src  = inst & 0xf;
		int dest = (inst >> 4) & 0xf;
		int xv;

		MOXIE_TRACE_INSN ("ld.b");
		xv = cpu.asregs.regs[src];
		cpu.asregs.regs[dest] = rbat (scpu, opc, xv);
	      }
	      break;
	    case 0x1d: /* lda.b */
	      {
		int reg = (inst >> 4) & 0xf;
		unsigned int addr = EXTRACT_WORD(pc+2);

		MOXIE_TRACE_INSN ("lda.b");
		cpu.asregs.regs[reg] = rbat (scpu, opc, addr);
		pc += 4;
	      }
	      break;
	    case 0x1e: /* st.b */
	      {
		int dest = (inst >> 4) & 0xf;
		int val  = inst & 0xf;

		MOXIE_TRACE_INSN ("st.b");
		wbat (scpu, opc, cpu.asregs.regs[dest], cpu.asregs.regs[val]);
	      }
	      break;
	    case 0x1f: /* sta.b */
	      {
		int reg = (inst >> 4) & 0xf;
		unsigned int addr = EXTRACT_WORD(pc+2);

		MOXIE_TRACE_INSN ("sta.b");
		wbat (scpu, opc, addr, cpu.asregs.regs[reg]);
		pc += 4;
	      }
	      break;
	    case 0x20: /* ldi.s (immediate) */
	      {
		int reg = (inst >> 4) & 0xf;

		unsigned int val = EXTRACT_WORD(pc+2);

		MOXIE_TRACE_INSN ("ldi.s");
		cpu.asregs.regs[reg] = val;
		pc += 4;
	      }
	      break;
	    case 0x21: /* ld.s (register indirect) */
	      {
		int src  = inst & 0xf;
		int dest = (inst >> 4) & 0xf;
		int xv;

		MOXIE_TRACE_INSN ("ld.s");
		xv = cpu.asregs.regs[src];
		cpu.asregs.regs[dest] = rsat (scpu, opc, xv);
	      }
	      break;
	    case 0x22: /* lda.s */
	      {
		int reg = (inst >> 4) & 0xf;
		unsigned int addr = EXTRACT_WORD(pc+2);

		MOXIE_TRACE_INSN ("lda.s");
		cpu.asregs.regs[reg] = rsat (scpu, opc, addr);
		pc += 4;
	      }
	      break;
	    case 0x23: /* st.s */
	      {
		int dest = (inst >> 4) & 0xf;
		int val  = inst & 0xf;

		MOXIE_TRACE_INSN ("st.s");
		wsat (scpu, opc, cpu.asregs.regs[dest], cpu.asregs.regs[val]);
	      }
	      break;
	    case 0x24: /* sta.s */
	      {
		int reg = (inst >> 4) & 0xf;
		unsigned int addr = EXTRACT_WORD(pc+2);

		MOXIE_TRACE_INSN ("sta.s");
		wsat (scpu, opc, addr, cpu.asregs.regs[reg]);
		pc += 4;
	      }
	      break;
	    case 0x25: /* jmp */
	      {
		int reg = (inst >> 4) & 0xf;

		MOXIE_TRACE_INSN ("jmp");
		pc = cpu.asregs.regs[reg] - 2;
	      }
	      break;
	    case 0x26: /* and */
	      {
		int a = (inst >> 4) & 0xf;
		int b = inst & 0xf;
		int av, bv;

		MOXIE_TRACE_INSN ("and");
		av = cpu.asregs.regs[a];
		bv = cpu.asregs.regs[b];
		cpu.asregs.regs[a] = av & bv;
	      }
	      break;
	    case 0x27: /* lshr */
	      {
		int a = (inst >> 4) & 0xf;
		int b = inst & 0xf;
		int av = cpu.asregs.regs[a];
		int bv = cpu.asregs.regs[b];

		MOXIE_TRACE_INSN ("lshr");
		cpu.asregs.regs[a] = (unsigned) ((unsigned) av >> bv);
	      }
	      break;
	    case 0x28: /* ashl */
	      {
		int a = (inst >> 4) & 0xf;
		int b = inst & 0xf;
		int av = cpu.asregs.regs[a];
		int bv = cpu.asregs.regs[b];

		MOXIE_TRACE_INSN ("ashl");
		cpu.asregs.regs[a] = av << bv;
	      }
	      break;
	    case 0x29: /* sub.l */
	      {
		int a = (inst >> 4) & 0xf;
		int b = inst & 0xf;
		unsigned av = cpu.asregs.regs[a];
		unsigned bv = cpu.asregs.regs[b];

		MOXIE_TRACE_INSN ("sub.l");
		cpu.asregs.regs[a] = av - bv;
	      }
	      break;
	    case 0x2a: /* neg */
	      {
		int a  = (inst >> 4) & 0xf;
		int b  = inst & 0xf;
		int bv = cpu.asregs.regs[b];

		MOXIE_TRACE_INSN ("neg");
		cpu.asregs.regs[a] = - bv;
	      }
	      break;
	    case 0x2b: /* or */
	      {
		int a = (inst >> 4) & 0xf;
		int b = inst & 0xf;
		int av, bv;

		MOXIE_TRACE_INSN ("or");
		av = cpu.asregs.regs[a];
		bv = cpu.asregs.regs[b];
		cpu.asregs.regs[a] = av | bv;
	      }
	      break;
	    case 0x2c: /* not */
	      {
		int a = (inst >> 4) & 0xf;
		int b = inst & 0xf;
		int bv = cpu.asregs.regs[b];

		MOXIE_TRACE_INSN ("not");
		cpu.asregs.regs[a] = 0xffffffff ^ bv;
	      }
	      break;
	    case 0x2d: /* ashr */
	      {
		int a  = (inst >> 4) & 0xf;
		int b  = inst & 0xf;
		int av = cpu.asregs.regs[a];
		int bv = cpu.asregs.regs[b];

		MOXIE_TRACE_INSN ("ashr");
		cpu.asregs.regs[a] = av >> bv;
	      }
	      break;
	    case 0x2e: /* xor */
	      {
		int a = (inst >> 4) & 0xf;
		int b = inst & 0xf;
		int av, bv;

		MOXIE_TRACE_INSN ("xor");
		av = cpu.asregs.regs[a];
		bv = cpu.asregs.regs[b];
		cpu.asregs.regs[a] = av ^ bv;
	      }
	      break;
	    case 0x2f: /* mul.l */
	      {
		int a = (inst >> 4) & 0xf;
		int b = inst & 0xf;
		unsigned av = cpu.asregs.regs[a];
		unsigned bv = cpu.asregs.regs[b];

		MOXIE_TRACE_INSN ("mul.l");
		cpu.asregs.regs[a] = av * bv;
	      }
	      break;
	    case 0x30: /* swi */
	      {
		unsigned int inum = EXTRACT_WORD(pc+2);

		MOXIE_TRACE_INSN ("swi");
		/* Set the special registers appropriately.  */
		cpu.asregs.sregs[2] = 3; /* MOXIE_EX_SWI */
	        cpu.asregs.sregs[3] = inum;
		switch (inum)
		  {
		  case TARGET_NEWLIB_SYS_exit:
		    {
		      sim_engine_halt (sd, scpu, NULL, pc, sim_exited,
				       cpu.asregs.regs[2]);
		      break;
		    }
		  case TARGET_NEWLIB_SYS_open:
		    {
		      char fname[1024];
		      int mode = (int) convert_target_flags ((unsigned) cpu.asregs.regs[3]);
		      int fd;
		      sim_core_read_buffer (sd, scpu, read_map, fname,
					    cpu.asregs.regs[2], 1024);
		      fd = sim_io_open (sd, fname, mode);
		      /* FIXME - set errno */
		      cpu.asregs.regs[2] = fd;
		      break;
		    }
		  case TARGET_NEWLIB_SYS_read:
		    {
		      int fd = cpu.asregs.regs[2];
		      unsigned len = (unsigned) cpu.asregs.regs[4];
		      char *buf = malloc (len);
		      cpu.asregs.regs[2] = sim_io_read (sd, fd, buf, len);
		      sim_core_write_buffer (sd, scpu, write_map, buf,
					     cpu.asregs.regs[3], len);
		      free (buf);
		      break;
		    }
		  case TARGET_NEWLIB_SYS_write:
		    {
		      char *str;
		      /* String length is at 0x12($fp) */
		      unsigned count, len = (unsigned) cpu.asregs.regs[4];
		      str = malloc (len);
		      sim_core_read_buffer (sd, scpu, read_map, str,
					    cpu.asregs.regs[3], len);
		      count = sim_io_write (sd, cpu.asregs.regs[2], str, len);
		      free (str);
		      cpu.asregs.regs[2] = count;
		      break;
		    }
		  case TARGET_NEWLIB_SYS_unlink:
		    {
		      char fname[1024];
		      int fd;
		      sim_core_read_buffer (sd, scpu, read_map, fname,
					    cpu.asregs.regs[2], 1024);
		      fd = sim_io_unlink (sd, fname);
		      /* FIXME - set errno */
		      cpu.asregs.regs[2] = fd;
		      break;
		    }
		  case 0xffffffff: /* Linux System Call */
		    {
		      unsigned int handler = cpu.asregs.sregs[1];
		      unsigned int sp = cpu.asregs.regs[1];

		      /* Save a slot for the static chain.  */
		      sp -= 4;

		      /* Push the return address.  */
		      sp -= 4;
		      wlat (scpu, opc, sp, pc + 6);
		
		      /* Push the current frame pointer.  */
		      sp -= 4;
		      wlat (scpu, opc, sp, cpu.asregs.regs[0]);

		      /* Uncache the stack pointer and set the fp & pc.  */
		      cpu.asregs.regs[1] = sp;
		      cpu.asregs.regs[0] = sp;
		      pc = handler - 6;
		    }
		  default:
		    break;
		  }
		pc += 4;
	      }
	      break;
	    case 0x31: /* div.l */
	      {
		int a = (inst >> 4) & 0xf;
		int b = inst & 0xf;
		int av = cpu.asregs.regs[a];
		int bv = cpu.asregs.regs[b];

		MOXIE_TRACE_INSN ("div.l");
		cpu.asregs.regs[a] = av / bv;
	      }
	      break;
	    case 0x32: /* udiv.l */
	      {
		int a = (inst >> 4) & 0xf;
		int b = inst & 0xf;
		unsigned int av = cpu.asregs.regs[a];
		unsigned int bv = cpu.asregs.regs[b];

		MOXIE_TRACE_INSN ("udiv.l");
		cpu.asregs.regs[a] = (av / bv);
	      }
	      break;
	    case 0x33: /* mod.l */
	      {
		int a = (inst >> 4) & 0xf;
		int b = inst & 0xf;
		int av = cpu.asregs.regs[a];
		int bv = cpu.asregs.regs[b];

		MOXIE_TRACE_INSN ("mod.l");
		cpu.asregs.regs[a] = av % bv;
	      }
	      break;
	    case 0x34: /* umod.l */
	      {
		int a = (inst >> 4) & 0xf;
		int b = inst & 0xf;
		unsigned int av = cpu.asregs.regs[a];
		unsigned int bv = cpu.asregs.regs[b];

		MOXIE_TRACE_INSN ("umod.l");
		cpu.asregs.regs[a] = (av % bv);
	      }
	      break;
	    case 0x35: /* brk */
	      MOXIE_TRACE_INSN ("brk");
	      sim_engine_halt (sd, scpu, NULL, pc, sim_stopped, SIM_SIGTRAP);
	      pc -= 2; /* Adjust pc */
	      break;
	    case 0x36: /* ldo.b */
	      {
		unsigned int addr = EXTRACT_OFFSET(pc+2);
		int a = (inst >> 4) & 0xf;
		int b = inst & 0xf;

		MOXIE_TRACE_INSN ("ldo.b");
		addr += cpu.asregs.regs[b];
		cpu.asregs.regs[a] = rbat (scpu, opc, addr);
		pc += 2;
	      }
	      break;
	    case 0x37: /* sto.b */
	      {
		unsigned int addr = EXTRACT_OFFSET(pc+2);
		int a = (inst >> 4) & 0xf;
		int b = inst & 0xf;

		MOXIE_TRACE_INSN ("sto.b");
		addr += cpu.asregs.regs[a];
		wbat (scpu, opc, addr, cpu.asregs.regs[b]);
		pc += 2;
	      }
	      break;
	    case 0x38: /* ldo.s */
	      {
		unsigned int addr = EXTRACT_OFFSET(pc+2);
		int a = (inst >> 4) & 0xf;
		int b = inst & 0xf;

		MOXIE_TRACE_INSN ("ldo.s");
		addr += cpu.asregs.regs[b];
		cpu.asregs.regs[a] = rsat (scpu, opc, addr);
		pc += 2;
	      }
	      break;
	    case 0x39: /* sto.s */
	      {
		unsigned int addr = EXTRACT_OFFSET(pc+2);
		int a = (inst >> 4) & 0xf;
		int b = inst & 0xf;

		MOXIE_TRACE_INSN ("sto.s");
		addr += cpu.asregs.regs[a];
		wsat (scpu, opc, addr, cpu.asregs.regs[b]);
		pc += 2;
	      }
	      break;
	    default:
	      opc = opcode;
	      MOXIE_TRACE_INSN ("SIGILL1");
	      sim_engine_halt (sd, scpu, NULL, pc, sim_stopped, SIM_SIGILL);
	      break;
	    }
	}

      cpu.asregs.insts++;
      pc += 2;
      cpu.asregs.regs[PC_REGNO] = pc;

      if (sim_events_tick (sd))
	sim_events_process (sd);

    } while (1);
}

static int
moxie_reg_store (SIM_CPU *scpu, int rn, const void *memory, int length)
{
  if (rn < NUM_MOXIE_REGS && rn >= 0)
    {
      if (length == 4)
	{
	  long ival;
	  
	  /* misalignment safe */
	  ival = moxie_extract_unsigned_integer (memory, 4);
	  cpu.asints[rn] = ival;
	}

      return 4;
    }
  else
    return 0;
}

static int
moxie_reg_fetch (SIM_CPU *scpu, int rn, void *memory, int length)
{
  if (rn < NUM_MOXIE_REGS && rn >= 0)
    {
      if (length == 4)
	{
	  long ival = cpu.asints[rn];

	  /* misalignment-safe */
	  moxie_store_unsigned_integer (memory, 4, ival);
	}
      
      return 4;
    }
  else
    return 0;
}

static sim_cia
moxie_pc_get (sim_cpu *cpu)
{
  return MOXIE_SIM_CPU (cpu)->registers[PCIDX];
}

static void
moxie_pc_set (sim_cpu *cpu, sim_cia pc)
{
  MOXIE_SIM_CPU (cpu)->registers[PCIDX] = pc;
}

static void
free_state (SIM_DESC sd)
{
  if (STATE_MODULES (sd) != NULL)
    sim_module_uninstall (sd);
  sim_cpu_free_all (sd);
  sim_state_free (sd);
}

SIM_DESC
sim_open (SIM_OPEN_KIND kind, host_callback *cb,
	  struct bfd *abfd, char * const *argv)
{
  int i;
  SIM_DESC sd = sim_state_alloc (kind, cb);
  SIM_ASSERT (STATE_MAGIC (sd) == SIM_MAGIC_NUMBER);

  /* Set default options before parsing user options.  */
  current_target_byte_order = BFD_ENDIAN_BIG;

  /* The cpu data is kept in a separately allocated chunk of memory.  */
  if (sim_cpu_alloc_all_extra (sd, 0, sizeof (struct moxie_sim_cpu))
      != SIM_RC_OK)
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

  sim_do_command(sd," memory region 0x00000000,0x4000000") ; 
  sim_do_command(sd," memory region 0xE0000000,0x10000") ; 

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

      CPU_REG_FETCH (cpu) = moxie_reg_fetch;
      CPU_REG_STORE (cpu) = moxie_reg_store;
      CPU_PC_FETCH (cpu) = moxie_pc_get;
      CPU_PC_STORE (cpu) = moxie_pc_set;

      set_initial_gprs ();	/* Reset the GPR registers.  */
    }

  return sd;
}

/* Load the device tree blob.  */

static void
load_dtb (SIM_DESC sd, const char *filename)
{
  int size = 0;
  FILE *f = fopen (filename, "rb");
  char *buf;
  sim_cpu *scpu = STATE_CPU (sd, 0); /* FIXME */ 

  /* Don't warn as the sim works fine w/out a device tree.  */
  if (f == NULL)
    return;
  fseek (f, 0, SEEK_END);
  size = ftell(f);
  fseek (f, 0, SEEK_SET);
  buf = alloca (size);
  if (size != fread (buf, 1, size, f))
    {
      sim_io_eprintf (sd, "ERROR: error reading ``%s''.\n", filename);
      fclose (f);
      return;
    }
  sim_core_write_buffer (sd, scpu, write_map, buf, 0xE0000000, size);
  cpu.asregs.sregs[9] = 0xE0000000;
  fclose (f);
}

SIM_RC
sim_create_inferior (SIM_DESC sd, struct bfd *prog_bfd,
		     char * const *argv, char * const *env)
{
  char * const *avp;
  int argc, i, tp;
  sim_cpu *scpu = STATE_CPU (sd, 0); /* FIXME */

  if (prog_bfd != NULL)
    cpu.asregs.regs[PC_REGNO] = bfd_get_start_address (prog_bfd);

  /* Copy args into target memory.  */
  avp = argv;
  for (argc = 0; avp && *avp; avp++)
    argc++;

  /* Target memory looks like this:
     0x00000000 zero word
     0x00000004 argc word
     0x00000008 start of argv
     .
     0x0000???? end of argv
     0x0000???? zero word 
     0x0000???? start of data pointed to by argv  */

  wlat (scpu, 0, 0, 0);
  wlat (scpu, 0, 4, argc);

  /* tp is the offset of our first argv data.  */
  tp = 4 + 4 + argc * 4 + 4;

  for (i = 0; i < argc; i++)
    {
      /* Set the argv value.  */
      wlat (scpu, 0, 4 + 4 + i * 4, tp);

      /* Store the string.  */
      sim_core_write_buffer (sd, scpu, write_map, argv[i],
			     tp, strlen(argv[i])+1);
      tp += strlen (argv[i]) + 1;
    }

  wlat (scpu, 0, 4 + 4 + i * 4, 0);

  load_dtb (sd, DTB);

  return SIM_RC_OK;
}
