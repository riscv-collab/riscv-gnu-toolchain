/* Simulator for TI MSP430 and MSP430X

   Copyright (C) 2013-2024 Free Software Foundation, Inc.
   Contributed by Red Hat.
   Based on sim/bfin/bfin-sim.c which was contributed by Analog Devices, Inc.

   This file is part of simulators.

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include <assert.h>
#include "opcode/msp430-decode.h"
#include "sim-main.h"
#include "sim-options.h"
#include "sim-signal.h"
#include "sim-syscall.h"
#include "msp430-sim.h"

static sim_cia
msp430_pc_fetch (SIM_CPU *cpu)
{
  struct msp430_cpu_state *msp430_cpu = MSP430_SIM_CPU (cpu);

  return msp430_cpu->regs[0];
}

static void
msp430_pc_store (SIM_CPU *cpu, sim_cia newpc)
{
  struct msp430_cpu_state *msp430_cpu = MSP430_SIM_CPU (cpu);

  msp430_cpu->regs[0] = newpc;
}

static int
msp430_reg_fetch (SIM_CPU *cpu, int regno, void *buf, int len)
{
  struct msp430_cpu_state *msp430_cpu = MSP430_SIM_CPU (cpu);
  unsigned char *memory = buf;

  if (0 <= regno && regno < 16)
    {
      if (len == 2)
	{
	  int val = msp430_cpu->regs[regno];
	  memory[0] = val & 0xff;
	  memory[1] = (val >> 8) & 0xff;
	  return 0;
	}
      else if (len == 4)
	{
	  int val = msp430_cpu->regs[regno];
	  memory[0] = val & 0xff;
	  memory[1] = (val >> 8) & 0xff;
	  memory[2] = (val >> 16) & 0x0f; /* Registers are only 20 bits wide.  */
	  memory[3] = 0;
	  return 0;
	}
      else
	return -1;
    }
  else
    return -1;
}

static int
msp430_reg_store (SIM_CPU *cpu, int regno, const void *buf, int len)
{
  struct msp430_cpu_state *msp430_cpu = MSP430_SIM_CPU (cpu);
  const unsigned char *memory = buf;

  if (0 <= regno && regno < 16)
    {
      if (len == 2)
	{
	  msp430_cpu->regs[regno] = (memory[1] << 8) | memory[0];
	  return len;
	}

      if (len == 4)
	{
	  msp430_cpu->regs[regno] = ((memory[2] << 16) & 0xf0000)
				     | (memory[1] << 8) | memory[0];
	  return len;
	}
    }

  return -1;
}

SIM_DESC
sim_open (SIM_OPEN_KIND kind,
	  struct host_callback_struct *callback,
	  struct bfd *abfd,
	  char * const *argv)
{
  SIM_DESC sd = sim_state_alloc (kind, callback);
  struct msp430_cpu_state *msp430_cpu;
  char c;
  int i;

  /* Initialise the simulator.  */

  /* Set default options before parsing user options.  */
  current_target_byte_order = BFD_ENDIAN_LITTLE;

  if (sim_cpu_alloc_all_extra (sd, 0, sizeof (struct msp430_cpu_state))
      != SIM_RC_OK)
    {
      sim_state_free (sd);
      return 0;
    }

  if (sim_pre_argv_init (sd, argv[0]) != SIM_RC_OK)
    {
      sim_state_free (sd);
      return 0;
    }

  if (sim_parse_args (sd, argv) != SIM_RC_OK)
    {
      sim_state_free (sd);
      return 0;
    }

  /* Allocate memory if none specified by user.
     Note - these values match the memory regions in the libgloss/msp430/msp430[xl]-sim.ld scripts.  */
  if (sim_core_read_buffer (sd, STATE_CPU (sd, 0), read_map, &c, 0x2, 1) == 0)
    sim_do_commandf (sd, "memory-region 0,0x20"); /* Needed by the GDB testsuite.  */
  if (sim_core_read_buffer (sd, STATE_CPU (sd, 0), read_map, &c, 0x500, 1) == 0)
    sim_do_commandf (sd, "memory-region 0x500,0xfac0");  /* RAM and/or ROM */
  if (sim_core_read_buffer (sd, STATE_CPU (sd, 0), read_map, &c, 0xfffe, 1) == 0)
    sim_do_commandf (sd, "memory-region 0xffc0,0x40"); /* VECTORS.  */
  if (sim_core_read_buffer (sd, STATE_CPU (sd, 0), read_map, &c, 0x10000, 1) == 0)
    sim_do_commandf (sd, "memory-region 0x10000,0x80000"); /* HIGH FLASH RAM.  */
  if (sim_core_read_buffer (sd, STATE_CPU (sd, 0), read_map, &c, 0x90000, 1) == 0)
    sim_do_commandf (sd, "memory-region 0x90000,0x70000"); /* HIGH ROM.  */

  /* Check for/establish the a reference program image.  */
  if (sim_analyze_program (sd, STATE_PROG_FILE (sd), abfd) != SIM_RC_OK)
    {
      sim_state_free (sd);
      return 0;
    }

  /* Establish any remaining configuration options.  */
  if (sim_config (sd) != SIM_RC_OK)
    {
      sim_state_free (sd);
      return 0;
    }

  if (sim_post_argv_init (sd) != SIM_RC_OK)
    {
      sim_state_free (sd);
      return 0;
    }

  /* CPU specific initialization.  */
  for (i = 0; i < MAX_NR_PROCESSORS; ++i)
    {
      SIM_CPU *cpu = STATE_CPU (sd, i);

      CPU_PC_FETCH (cpu) = msp430_pc_fetch;
      CPU_PC_STORE (cpu) = msp430_pc_store;
      CPU_REG_FETCH (cpu) = msp430_reg_fetch;
      CPU_REG_STORE (cpu) = msp430_reg_store;

      msp430_cpu = MSP430_SIM_CPU (cpu);
      msp430_cpu->cio_breakpoint = trace_sym_value (sd, "C$$IO$$");
      msp430_cpu->cio_buffer = trace_sym_value (sd, "__CIOBUF__");
      if (msp430_cpu->cio_buffer == -1)
	msp430_cpu->cio_buffer = trace_sym_value (sd, "_CIOBUF_");
    }

  return sd;
}

SIM_RC
sim_create_inferior (SIM_DESC sd,
		     struct bfd *abfd,
		     char * const *argv,
		     char * const *env)
{
  unsigned char resetv[2];
  int new_pc;

  /* Set the PC to the default reset vector if available.  */
  sim_core_read_buffer (sd, STATE_CPU (sd, 0), read_map, resetv, 0xfffe, 2);
  new_pc = resetv[0] + 256 * resetv[1];

  /* If the reset vector isn't initialized, then use the ELF entry.  */
  if (abfd != NULL && !new_pc)
    new_pc = bfd_get_start_address (abfd);

  sim_pc_set (STATE_CPU (sd, 0), new_pc);
  msp430_pc_store (STATE_CPU (sd, 0), new_pc);

  return SIM_RC_OK;
}

typedef struct
{
  SIM_DESC sd;
  int gb_addr;
} Get_Byte_Local_Data;

static int
msp430_getbyte (void *vld)
{
  Get_Byte_Local_Data *ld = (Get_Byte_Local_Data *)vld;
  char buf[1];
  SIM_DESC sd = ld->sd;

  sim_core_read_buffer (sd, STATE_CPU (sd, 0), read_map, buf, ld->gb_addr, 1);
  ld->gb_addr ++;
  return buf[0];
}

#define REG(N) MSP430_SIM_CPU (STATE_CPU (sd, 0))->regs[N]
#define PC REG(MSR_PC)
#define SP REG(MSR_SP)
#define SR REG(MSR_SR)

static const char *
register_names[] =
{
  "PC", "SP", "SR", "CG", "R4", "R5", "R6", "R7", "R8",
  "R9", "R10", "R11", "R12", "R13", "R14", "R15"
};

static void
trace_reg_put (SIM_DESC sd, int n, unsigned int v)
{
  TRACE_REGISTER (STATE_CPU (sd, 0), "PUT: %#x -> %s", v, register_names[n]);
  REG (n) = v;
}

static unsigned int
trace_reg_get (SIM_DESC sd, int n)
{
  TRACE_REGISTER (STATE_CPU (sd, 0), "GET: %s -> %#x", register_names[n], REG (n));
  return REG (n);
}

#define REG_PUT(N,V) trace_reg_put (sd, N, V)
#define REG_GET(N)   trace_reg_get (sd, N)

/* Hardware multiply (and accumulate) support.  */

static unsigned int
zero_ext (unsigned int v, unsigned int bits)
{
  v &= ((1 << bits) - 1);
  return v;
}

static signed long long
sign_ext (signed long long v, unsigned int bits)
{
  signed long long sb = 1LL << (bits-1);	/* Sign bit.  */
  signed long long mb = (1LL << (bits-1)) - 1LL; /* Mantissa bits.  */

  if (v & sb)
    v = v | ~mb;
  else
    v = v & mb;
  return v;
}

static int
get_op (SIM_DESC sd, MSP430_Opcode_Decoded *opc, int n)
{
  MSP430_Opcode_Operand *op = opc->op + n;
  int rv = 0;
  int addr;
  unsigned char buf[4];
  int incval = 0;

  switch (op->type)
    {
    case MSP430_Operand_Immediate:
      rv =  op->addend;
      break;
    case MSP430_Operand_Register:
      rv = REG_GET (op->reg);
      break;
    case MSP430_Operand_Indirect:
    case MSP430_Operand_Indirect_Postinc:
      addr = op->addend;
      if (op->reg != MSR_None)
	{
	  int reg = REG_GET (op->reg);
	  int sign = opc->ofs_430x ? 20 : 16;

	  /* Index values are signed.  */
	  if (addr & (1 << (sign - 1)))
	    addr |= -(1 << sign);

	  addr += reg;

	  /* For MSP430 instructions the sum is limited to 16 bits if the
	     address in the index register is less than 64k even if we are
	     running on an MSP430X CPU.  This is for MSP430 compatibility.  */
	  if (reg < 0x10000 && ! opc->ofs_430x)
	    {
	      if (addr >= 0x10000)
		fprintf (stderr, " XXX WRAPPING ADDRESS %x on read\n", addr);

	      addr &= 0xffff;
	    }
	}
      addr &= 0xfffff;
      switch (opc->size)
	{
	case 8:
	  sim_core_read_buffer (sd, STATE_CPU (sd, 0), read_map, buf, addr, 1);
	  rv = buf[0];
	  break;
	case 16:
	  sim_core_read_buffer (sd, STATE_CPU (sd, 0), read_map, buf, addr, 2);
	  rv = buf[0] | (buf[1] << 8);
	  break;
	case 20:
	case 32:
	  sim_core_read_buffer (sd, STATE_CPU (sd, 0), read_map, buf, addr, 4);
	  rv = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
	  break;
	default:
	  assert (! opc->size);
	  break;
	}
#if 0
      /* Hack - MSP430X5438 serial port status register.  */
      if (addr == 0x5dd)
	rv = 2;
#endif
      if ((addr >= 0x130 && addr <= 0x15B)
	  || (addr >= 0x4C0 && addr <= 0x4EB))
	{
	  switch (addr)
	    {
	    case 0x4CA:
	    case 0x13A:
	      switch (HWMULT (sd, hwmult_type))
		{
		case UNSIGN_MAC_32:
		case UNSIGN_32:
		  rv = zero_ext (HWMULT (sd, hwmult_result), 16);
		  break;
		case SIGN_MAC_32:
		case SIGN_32:
		  rv = sign_ext (HWMULT (sd, hwmult_signed_result), 16);
		  break;
		}
	      break;

	    case 0x4CC:
	    case 0x13C:
	      switch (HWMULT (sd, hwmult_type))
		{
		case UNSIGN_MAC_32:
		case UNSIGN_32:
		  rv = zero_ext (HWMULT (sd, hwmult_result) >> 16, 16);
		  break;

		case SIGN_MAC_32:
		case SIGN_32:
		  rv = sign_ext (HWMULT (sd, hwmult_signed_result) >> 16, 16);
		  break;
		}
	      break;

	    case 0x4CE:
	    case 0x13E:
	      switch (HWMULT (sd, hwmult_type))
		{
		case UNSIGN_32:
		  rv = 0;
		  break;
		case SIGN_32:
		  rv = HWMULT (sd, hwmult_signed_result) < 0 ? -1 : 0;
		  break;
		case UNSIGN_MAC_32:
		  rv = 0; /* FIXME: Should be carry of last accumulate.  */
		  break;
		case SIGN_MAC_32:
		  rv = HWMULT (sd, hwmult_signed_accumulator) < 0 ? -1 : 0;
		  break;
		}
	      break;

	    case 0x4E4:
	    case 0x154:
	      rv = zero_ext (HWMULT (sd, hw32mult_result), 16);
	      break;

	    case 0x4E6:
	    case 0x156:
	      rv = zero_ext (HWMULT (sd, hw32mult_result) >> 16, 16);
	      break;

	    case 0x4E8:
	    case 0x158:
	      rv = zero_ext (HWMULT (sd, hw32mult_result) >> 32, 16);
	      break;

	    case 0x4EA:
	    case 0x15A:
	      switch (HWMULT (sd, hw32mult_type))
		{
		case UNSIGN_64: rv = zero_ext (HWMULT (sd, hw32mult_result) >> 48, 16); break;
		case   SIGN_64: rv = sign_ext (HWMULT (sd, hw32mult_result) >> 48, 16); break;
		}
	      break;

	    default:
	      fprintf (stderr, "unimplemented HW MULT read from %x!\n", addr);
	      break;
	    }
	}

      TRACE_MEMORY (STATE_CPU (sd, 0), "GET: [%#x].%d -> %#x", addr, opc->size,
		    rv);
      break;

    default:
      fprintf (stderr, "invalid operand %d type %d\n", n, op->type);
      abort ();
    }

  switch (opc->size)
    {
    case 8:
      rv &= 0xff;
      incval = 1;
      break;
    case 16:
      rv &= 0xffff;
      incval = 2;
      break;
    case 20:
      rv &= 0xfffff;
      incval = 4;
      break;
    case 32:
      rv &= 0xffffffff;
      incval = 4;
      break;
    }

  if (op->type == MSP430_Operand_Indirect_Postinc)
    REG_PUT (op->reg, REG_GET (op->reg) + incval);

  return rv;
}

static int
put_op (SIM_DESC sd, MSP430_Opcode_Decoded *opc, int n, int val)
{
  MSP430_Opcode_Operand *op = opc->op + n;
  int rv = 0;
  int addr;
  unsigned char buf[4];
  int incval = 0;

  switch (opc->size)
    {
    case 8:
      val &= 0xff;
      break;
    case 16:
      val &= 0xffff;
      break;
    case 20:
      val &= 0xfffff;
      break;
    case 32:
      val &= 0xffffffff;
      break;
    }

  switch (op->type)
    {
    case MSP430_Operand_Register:
      REG (op->reg) = val;
      REG_PUT (op->reg, val);
      break;
    case MSP430_Operand_Indirect:
    case MSP430_Operand_Indirect_Postinc:
      addr = op->addend;
      if (op->reg != MSR_None)
	{
	  int reg = REG_GET (op->reg);
	  int sign = opc->ofs_430x ? 20 : 16;

	  /* Index values are signed.  */
	  if (addr & (1 << (sign - 1)))
	    addr |= -(1 << sign);

	  addr += reg;

	  /* For MSP430 instructions the sum is limited to 16 bits if the
	     address in the index register is less than 64k even if we are
	     running on an MSP430X CPU.  This is for MSP430 compatibility.  */
	  if (reg < 0x10000 && ! opc->ofs_430x)
	    {
	      if (addr >= 0x10000)
		fprintf (stderr, " XXX WRAPPING ADDRESS %x on write\n", addr);
		
	      addr &= 0xffff;
	    }
	}
      addr &= 0xfffff;

      TRACE_MEMORY (STATE_CPU (sd, 0), "PUT: [%#x].%d <- %#x", addr, opc->size,
		    val);
#if 0
      /* Hack - MSP430X5438 serial port transmit register.  */
      if (addr == 0x5ce)
	putchar (val);
#endif
      if ((addr >= 0x130 && addr <= 0x15B)
	  || (addr >= 0x4C0 && addr <= 0x4EB))
	{
	  signed int a,b;

	  /* Hardware Multiply emulation.  */
	  assert (opc->size == 16);

	  switch (addr)
	    {
	    case 0x4C0:
	    case 0x130:
	      HWMULT (sd, hwmult_op1) = val;
	      HWMULT (sd, hwmult_type) = UNSIGN_32;
	      break;

	    case 0x4C2:
	    case 0x132:
	      HWMULT (sd, hwmult_op1) = val;
	      HWMULT (sd, hwmult_type) = SIGN_32;
	      break;

	    case 0x4C4:
	    case 0x134:
	      HWMULT (sd, hwmult_op1) = val;
	      HWMULT (sd, hwmult_type) = UNSIGN_MAC_32;
	      break;

	    case 0x4C6:
	    case 0x136:
	      HWMULT (sd, hwmult_op1) = val;
	      HWMULT (sd, hwmult_type) = SIGN_MAC_32;
	      break;

	    case 0x4C8:
	    case 0x138:
	      HWMULT (sd, hwmult_op2) = val;
	      switch (HWMULT (sd, hwmult_type))
		{
		case UNSIGN_32:
		  a = HWMULT (sd, hwmult_op1);
		  b = HWMULT (sd, hwmult_op2);
		  /* For unsigned 32-bit multiplication of 16-bit operands, an
		     explicit cast is required to prevent any implicit
		     sign-extension.  */
		  HWMULT (sd, hwmult_result) = (uint32_t) a * (uint32_t) b;
		  HWMULT (sd, hwmult_signed_result) = a * b;
		  HWMULT (sd, hwmult_accumulator) = HWMULT (sd, hwmult_signed_accumulator) = 0;
		  break;

		case SIGN_32:
		  a = sign_ext (HWMULT (sd, hwmult_op1), 16);
		  b = sign_ext (HWMULT (sd, hwmult_op2), 16);
		  HWMULT (sd, hwmult_signed_result) = a * b;
		  HWMULT (sd, hwmult_result) = (uint32_t) a * (uint32_t) b;
		  HWMULT (sd, hwmult_accumulator) = HWMULT (sd, hwmult_signed_accumulator) = 0;
		  break;

		case UNSIGN_MAC_32:
		  a = HWMULT (sd, hwmult_op1);
		  b = HWMULT (sd, hwmult_op2);
		  HWMULT (sd, hwmult_accumulator)
		    += (uint32_t) a * (uint32_t) b;
		  HWMULT (sd, hwmult_signed_accumulator) += a * b;
		  HWMULT (sd, hwmult_result) = HWMULT (sd, hwmult_accumulator);
		  HWMULT (sd, hwmult_signed_result) = HWMULT (sd, hwmult_signed_accumulator);
		  break;

		case SIGN_MAC_32:
		  a = sign_ext (HWMULT (sd, hwmult_op1), 16);
		  b = sign_ext (HWMULT (sd, hwmult_op2), 16);
		  HWMULT (sd, hwmult_accumulator)
		    += (uint32_t) a * (uint32_t) b;
		  HWMULT (sd, hwmult_signed_accumulator) += a * b;
		  HWMULT (sd, hwmult_result) = HWMULT (sd, hwmult_accumulator);
		  HWMULT (sd, hwmult_signed_result) = HWMULT (sd, hwmult_signed_accumulator);
		  break;
		}
	      break;

	    case 0x4CA:
	    case 0x13A:
	      /* Copy into LOW result...  */
	      switch (HWMULT (sd, hwmult_type))
		{
		case UNSIGN_MAC_32:
		case UNSIGN_32:
		  HWMULT (sd, hwmult_accumulator) = HWMULT (sd, hwmult_result) = zero_ext (val, 16);
		  HWMULT (sd, hwmult_signed_accumulator) = sign_ext (val, 16);
		  break;
		case SIGN_MAC_32:
		case SIGN_32:
		  HWMULT (sd, hwmult_signed_accumulator) = HWMULT (sd, hwmult_result) = sign_ext (val, 16);
		  HWMULT (sd, hwmult_accumulator) = zero_ext (val, 16);
		  break;
		}
	      break;
		
	    case 0x4D0:
	    case 0x140:
	      HWMULT (sd, hw32mult_op1) = val;
	      HWMULT (sd, hw32mult_type) = UNSIGN_64;
	      break;

	    case 0x4D2:
	    case 0x142:
	      HWMULT (sd, hw32mult_op1) = (HWMULT (sd, hw32mult_op1) & 0xFFFF) | (val << 16);
	      break;

	    case 0x4D4:
	    case 0x144:
	      HWMULT (sd, hw32mult_op1) = val;
	      HWMULT (sd, hw32mult_type) = SIGN_64;
	      break;

	    case 0x4D6:
	    case 0x146:
	      HWMULT (sd, hw32mult_op1) = (HWMULT (sd, hw32mult_op1) & 0xFFFF) | (val << 16);
	      break;

	    case 0x4E0:
	    case 0x150:
	      HWMULT (sd, hw32mult_op2) = val;
	      break;

	    case 0x4E2:
	    case 0x152:
	      HWMULT (sd, hw32mult_op2) = (HWMULT (sd, hw32mult_op2) & 0xFFFF) | (val << 16);
	      switch (HWMULT (sd, hw32mult_type))
		{
		case UNSIGN_64:
		  HWMULT (sd, hw32mult_result)
		    = (uint64_t) HWMULT (sd, hw32mult_op1)
		    * (uint64_t) HWMULT (sd, hw32mult_op2);
		  break;
		case SIGN_64:
		  HWMULT (sd, hw32mult_result)
		    = sign_ext (HWMULT (sd, hw32mult_op1), 32)
		    * sign_ext (HWMULT (sd, hw32mult_op2), 32);
		  break;
		}
	      break;

	    default:
	      fprintf (stderr, "unimplemented HW MULT write to %x!\n", addr);
	      break;
	    }
	}

      switch (opc->size)
	{
	case 8:
	  buf[0] = val;
	  sim_core_write_buffer (sd, STATE_CPU (sd, 0), write_map, buf, addr, 1);
	  break;
	case 16:
	  buf[0] = val;
	  buf[1] = val >> 8;
	  sim_core_write_buffer (sd, STATE_CPU (sd, 0), write_map, buf, addr, 2);
	  break;
	case 20:
	case 32:
	  buf[0] = val;
	  buf[1] = val >> 8;
	  buf[2] = val >> 16;
	  buf[3] = val >> 24;
	  sim_core_write_buffer (sd, STATE_CPU (sd, 0), write_map, buf, addr, 4);
	  break;
	default:
	  assert (! opc->size);
	  break;
	}
      break;
    default:
      fprintf (stderr, "invalid operand %d type %d\n", n, op->type);
      abort ();
    }

  switch (opc->size)
    {
    case 8:
      rv &= 0xff;
      incval = 1;
      break;
    case 16:
      rv &= 0xffff;
      incval = 2;
      break;
    case 20:
      rv &= 0xfffff;
      incval = 4;
      break;
    case 32:
      rv &= 0xffffffff;
      incval = 4;
      break;
    }

  if (op->type == MSP430_Operand_Indirect_Postinc)
    {
      int new_val = REG_GET (op->reg) + incval;
      /* SP is always word-aligned.  */
      if (op->reg == MSR_SP && (new_val & 1))
	new_val ++;
      REG_PUT (op->reg, new_val);
    }

  return rv;
}

static void
mem_put_val (SIM_DESC sd, int addr, int val, int bits)
{
  MSP430_Opcode_Decoded opc;

  opc.size = bits;
  opc.op[0].type = MSP430_Operand_Indirect;
  opc.op[0].addend = addr;
  opc.op[0].reg = MSR_None;
  put_op (sd, &opc, 0, val);
}

static int
mem_get_val (SIM_DESC sd, int addr, int bits)
{
  MSP430_Opcode_Decoded opc;

  opc.size = bits;
  opc.op[0].type = MSP430_Operand_Indirect;
  opc.op[0].addend = addr;
  opc.op[0].reg = MSR_None;
  return get_op (sd, &opc, 0);
}

#define CIO_OPEN    (0xF0)
#define CIO_CLOSE   (0xF1)
#define CIO_READ    (0xF2)
#define CIO_WRITE   (0xF3)
#define CIO_LSEEK   (0xF4)
#define CIO_UNLINK  (0xF5)
#define CIO_GETENV  (0xF6)
#define CIO_RENAME  (0xF7)
#define CIO_GETTIME (0xF8)
#define CIO_GETCLK  (0xF9)
#define CIO_SYNC    (0xFF)

#define CIO_I(n) (parms[(n)] + parms[(n)+1] * 256)
#define CIO_L(n) (parms[(n)] + parms[(n)+1] * 256 \
		  + parms[(n)+2] * 65536 + parms[(n)+3] * 16777216)

static void
msp430_cio (SIM_DESC sd)
{
  /* A block of data at __CIOBUF__ describes the I/O operation to
     perform.  */
  sim_cpu *cpu = STATE_CPU (sd, 0);
  struct msp430_cpu_state *msp430_cpu = MSP430_SIM_CPU (cpu);
  unsigned char parms[8];
  long length;
  int command;
  unsigned char buffer[512];
  long ret_buflen = 0;
  long fd, len, rv;

  sim_core_read_buffer (sd, cpu, 0, parms, msp430_cpu->cio_buffer, 5);
  length = CIO_I (0);
  command = parms[2];

  sim_core_read_buffer (sd, cpu, 0, parms, msp430_cpu->cio_buffer + 3, 8);
  sim_core_read_buffer (sd, cpu, 0, buffer, msp430_cpu->cio_buffer + 11, length);

  switch (command)
    {
    case CIO_WRITE:
      fd = CIO_I (0);
      len = CIO_I (2);

      rv = write (fd, buffer, len);
      parms[0] = rv & 0xff;
      parms[1] = rv >> 8;

      break;
    }

  sim_core_write_buffer (sd, cpu, 0, parms, msp430_cpu->cio_buffer + 4, 8);
  if (ret_buflen)
    sim_core_write_buffer (sd, cpu, 0, buffer, msp430_cpu->cio_buffer + 12,
			   ret_buflen);
}

#define SRC     get_op (sd, opcode, 1)
#define DSRC    get_op (sd, opcode, 0)
#define DEST(V) put_op (sd, opcode, 0, (V))

#define DO_ALU(OP,SOP,MORE)						\
  {									\
    int s1 = DSRC;							\
    int s2 = SRC;							\
    int result = s1 OP s2 MORE;						\
    TRACE_ALU (STATE_CPU (sd, 0), "ALU: %#x %s %#x %s = %#x", s1, SOP,	\
	       s2, #MORE, result); \
    DEST (result);							\
  }

#define SIGN   (1 << (opcode->size - 1))
#define POS(x) (((x) & SIGN) ? 0 : 1)
#define NEG(x) (((x) & SIGN) ? 1 : 0)

#define SX(v) sign_ext (v, opcode->size)
#define ZX(v) zero_ext (v, opcode->size)

static char *
flags2string (int f)
{
  static char buf[2][6];
  static int bi = 0;
  char *bp = buf[bi];

  bi = (bi + 1) % 2;

  bp[0] = f & MSP430_FLAG_V ? 'V' : '-';
  bp[1] = f & MSP430_FLAG_N ? 'N' : '-';
  bp[2] = f & MSP430_FLAG_Z ? 'Z' : '-';
  bp[3] = f & MSP430_FLAG_C ? 'C' : '-';
  bp[4] = 0;
  return bp;
}

/* Random number that won't show up in our usual logic.  */
#define MAGIC_OVERFLOW 0x55000F

static void
do_flags (SIM_DESC sd,
	  MSP430_Opcode_Decoded *opcode,
	  int vnz_val, /* Signed result.  */
	  int carry,
	  int overflow)
{
  int f = SR;
  int new_f = 0;
  int signbit = 1 << (opcode->size - 1);

  f &= ~opcode->flags_0;
  f &= ~opcode->flags_set;
  f |= opcode->flags_1;

  if (vnz_val & signbit)
    new_f |= MSP430_FLAG_N;
  if (! (vnz_val & ((signbit << 1) - 1)))
    new_f |= MSP430_FLAG_Z;
  if (overflow == MAGIC_OVERFLOW)
    {
      if (vnz_val != SX (vnz_val))
	new_f |= MSP430_FLAG_V;
    }
  else
    if (overflow)
      new_f |= MSP430_FLAG_V;
  if (carry)
    new_f |= MSP430_FLAG_C;

  new_f = f | (new_f & opcode->flags_set);
  if (SR != new_f)
    TRACE_ALU (STATE_CPU (sd, 0), "FLAGS: %s -> %s", flags2string (SR),
	       flags2string (new_f));
  else
    TRACE_ALU (STATE_CPU (sd, 0), "FLAGS: %s", flags2string (new_f));
  SR = new_f;
}

#define FLAGS(vnz,c)    do_flags (sd, opcode, vnz, c, MAGIC_OVERFLOW)
#define FLAGSV(vnz,c,v) do_flags (sd, opcode, vnz, c, v)

/* These two assume unsigned 16-bit (four digit) words.
   Mask off unwanted bits for byte operations.  */

static int
bcd_to_binary (int v)
{
  int r = (  ((v >>  0) & 0xf) * 1
	   + ((v >>  4) & 0xf) * 10
	   + ((v >>  8) & 0xf) * 100
	   + ((v >> 12) & 0xf) * 1000);
  return r;
}

static int
binary_to_bcd (int v)
{
  int r = ( ((v /    1) % 10) <<  0
	  | ((v /   10) % 10) <<  4
	  | ((v /  100) % 10) <<  8
	  | ((v / 1000) % 10) << 12);
  return r;
}

static const char *
cond_string (int cond)
{
  switch (cond)
    {
    case MSC_nz:
      return "NZ";
    case MSC_z:
      return "Z";
    case MSC_nc:
      return "NC";
    case MSC_c:
      return "C";
    case MSC_n:
      return "N";
    case MSC_ge:
      return "GE";
    case MSC_l:
      return "L";
    case MSC_true:
      return "MP";
    default:
      return "??";
    }
}

/* Checks a CALL to address CALL_ADDR.  If this is a special
   syscall address then the call is simulated and non-zero is
   returned.  Otherwise 0 is returned.  */

static int
maybe_perform_syscall (SIM_DESC sd, int call_addr)
{
  sim_cpu *cpu = STATE_CPU (sd, 0);
  struct msp430_cpu_state *msp430_cpu = MSP430_SIM_CPU (cpu);

  if (call_addr == 0x00160)
    {
      int i;

      for (i = 0; i < 16; i++)
	{
	  if (i % 4 == 0)
	    fprintf (stderr, "\t");
	  fprintf (stderr, "R%-2d %05x   ", i, msp430_cpu->regs[i]);
	  if (i % 4 == 3)
	    {
	      int sp = SP + (3 - (i / 4)) * 2;
	      unsigned char buf[2];

	      sim_core_read_buffer (sd, cpu, read_map, buf, sp, 2);

	      fprintf (stderr, "\tSP%+d: %04x", sp - SP,
		       buf[0] + buf[1] * 256);

	      if (i / 4 == 0)
		{
		  int flags = SR;

		  fprintf (stderr, flags & 0x100 ? "   V" : "   -");
		  fprintf (stderr, flags & 0x004 ? "N" : "-");
		  fprintf (stderr, flags & 0x002 ? "Z" : "-");
		  fprintf (stderr, flags & 0x001 ? "C" : "-");
		}

	      fprintf (stderr, "\n");
	    }
	}
      return 1;
    }

  if ((call_addr & ~0x3f) == 0x00180)
    {
      /* Syscall!  */
      int arg1, arg2, arg3, arg4;
      int syscall_num = call_addr & 0x3f;

      /* syscall_num == 2 is used for the variadic function "open".
	 The arguments are set up differently for variadic functions.
	 See slaa534.pdf distributed by TI.  */
      if (syscall_num == 2)
	{
	  arg1 = msp430_cpu->regs[12];
	  arg2 = mem_get_val (sd, SP, 16);
	  arg3 = mem_get_val (sd, SP + 2, 16);
	  arg4 = mem_get_val (sd, SP + 4, 16);
	}
      else
	{
	  arg1 = msp430_cpu->regs[12];
	  arg2 = msp430_cpu->regs[13];
	  arg3 = msp430_cpu->regs[14];
	  arg4 = msp430_cpu->regs[15];
	}

      msp430_cpu->regs[12] = sim_syscall (cpu, syscall_num, arg1, arg2, arg3,
					  arg4);
      return 1;
    }

  return 0;
}

static void
msp430_step_once (SIM_DESC sd)
{
  sim_cpu *cpu = STATE_CPU (sd, 0);
  struct msp430_cpu_state *msp430_cpu = MSP430_SIM_CPU (cpu);
  Get_Byte_Local_Data ld;
  int i;
  int opsize;
  unsigned int opcode_pc;
  MSP430_Opcode_Decoded opcode_buf;
  MSP430_Opcode_Decoded *opcode = &opcode_buf;
  int s1, s2, result;
  int u1 = 0, u2, uresult;
  int c = 0;
  int carry_to_use;
  int n_repeats;
  int rept;
  int op_bytes = 0, op_bits;

  PC &= 0xfffff;
  opcode_pc = PC;

  if (opcode_pc < 0x10)
    {
      fprintf (stderr, "Fault: PC(%#x) is less than 0x10\n", opcode_pc);
      sim_engine_halt (sd, cpu, NULL, msp430_cpu->regs[0], sim_exited, -1);
      return;
    }

  if (PC == msp430_cpu->cio_breakpoint && STATE_OPEN_KIND (sd) != SIM_OPEN_DEBUG)
    msp430_cio (sd);

  ld.sd = sd;
  ld.gb_addr = PC;
  opsize = msp430_decode_opcode (msp430_cpu->regs[0], opcode, msp430_getbyte,
				 &ld);
  PC += opsize;
  if (opsize <= 0)
    {
      fprintf (stderr, "Fault: undecodable opcode at %#x\n", opcode_pc);
      sim_engine_halt (sd, cpu, NULL, msp430_cpu->regs[0], sim_exited, -1);
      return;
    }

  if (opcode->repeat_reg)
    n_repeats = (msp430_cpu->regs[opcode->repeats] & 0x000f) + 1;
  else
    n_repeats = opcode->repeats + 1;

  op_bits = opcode->size;
  switch (op_bits)
    {
    case 8:
      op_bytes = 1;
      break;
    case 16:
      op_bytes = 2;
      break;
    case 20:
    case 32:
      op_bytes = 4;
      break;
    }

  if (TRACE_ANY_P (cpu))
    trace_prefix (sd, cpu, NULL_CIA, opcode_pc, TRACE_LINENUM_P (cpu), NULL,
		  0, " ");

  TRACE_DISASM (cpu, opcode_pc);

  carry_to_use = 0;
  switch (opcode->id)
    {
    case MSO_unknown:
      break;

      /* Double-operand instructions.  */
    case MSO_mov:
      if (opcode->n_bytes == 2
	  && opcode->op[0].type == MSP430_Operand_Register
	  && opcode->op[0].reg == MSR_CG
	  && opcode->op[1].type == MSP430_Operand_Immediate
	  && opcode->op[1].addend == 0
	  /* A 16-bit write of #0 is a NOP; an 8-bit write is a BRK.  */
	  && opcode->size == 8)
	{
	  /* This is the designated software breakpoint instruction.  */
	  PC -= opsize;
	  sim_engine_halt (sd, cpu, NULL, msp430_cpu->regs[0], sim_stopped,
			   SIM_SIGTRAP);
	}
      else
	{
	  /* Otherwise, do the move.  */
	  for (rept = 0; rept < n_repeats; rept ++)
	    {
	      DEST (SRC);
	    }
	}
      break;

    case MSO_addc:
      for (rept = 0; rept < n_repeats; rept ++)
	{
	  carry_to_use = (SR & MSP430_FLAG_C) ? 1 : 0;
	  u1 = DSRC;
	  u2 = SRC;
	  s1 = SX (u1);
	  s2 = SX (u2);
	  uresult = u1 + u2 + carry_to_use;
	  result = s1 + s2 + carry_to_use;
	  TRACE_ALU (cpu, "ADDC: %#x + %#x + %d = %#x",
		     u1, u2, carry_to_use, uresult);
	  DEST (result);
	  FLAGS (result, uresult != ZX (uresult));
	}
      break;

    case MSO_add:
      for (rept = 0; rept < n_repeats; rept ++)
	{
	  u1 = DSRC;
	  u2 = SRC;
	  s1 = SX (u1);
	  s2 = SX (u2);
	  uresult = u1 + u2;
	  result = s1 + s2;
	  TRACE_ALU (cpu, "ADD: %#x + %#x = %#x", u1, u2, uresult);
	  DEST (result);
	  FLAGS (result, uresult != ZX (uresult));
	}
      break;

    case MSO_subc:
      for (rept = 0; rept < n_repeats; rept ++)
	{
	  carry_to_use = (SR & MSP430_FLAG_C) ? 1 : 0;
	  u1 = DSRC;
	  u2 = SRC;
	  s1 = SX (u1);
	  s2 = SX (u2);
	  uresult = ZX (~u2) + u1 + carry_to_use;
	  result = s1 - s2 + (carry_to_use - 1);
	  TRACE_ALU (cpu, "SUBC: %#x - %#x + %d = %#x",
		     u1, u2, carry_to_use, uresult);
	  DEST (result);
	  FLAGS (result, uresult != ZX (uresult));
	}
      break;

    case MSO_sub:
      for (rept = 0; rept < n_repeats; rept ++)
	{
	  u1 = DSRC;
	  u2 = SRC;
	  s1 = SX (u1);
	  s2 = SX (u2);
	  uresult = ZX (~u2) + u1 + 1;
	  result = SX (uresult);
	  TRACE_ALU (cpu, "SUB: %#x - %#x = %#x",
		     u1, u2, uresult);
	  DEST (result);
	  FLAGS (result, uresult != ZX (uresult));
	}
      break;

    case MSO_cmp:
      for (rept = 0; rept < n_repeats; rept ++)
	{
	  u1 = DSRC;
	  u2 = SRC;
	  s1 = SX (u1);
	  s2 = SX (u2);
	  uresult = ZX (~u2) + u1 + 1;
	  result = s1 - s2;
	  TRACE_ALU (cpu, "CMP: %#x - %#x = %x",
		     u1, u2, uresult);
	  FLAGS (result, uresult != ZX (uresult));
	}
      break;

    case MSO_dadd:
      for (rept = 0; rept < n_repeats; rept ++)
	{
	  carry_to_use = (SR & MSP430_FLAG_C) ? 1 : 0;
	  u1 = DSRC;
	  u2 = SRC;
	  uresult = bcd_to_binary (u1) + bcd_to_binary (u2) + carry_to_use;
	  result = binary_to_bcd (uresult);
	  TRACE_ALU (cpu, "DADD: %#x + %#x + %d = %#x",
		     u1, u2, carry_to_use, result);
	  DEST (result);
	  FLAGS (result, uresult > ((opcode->size == 8) ? 99 : 9999));
	}
      break;

    case MSO_and:
      for (rept = 0; rept < n_repeats; rept ++)
	{
	  u1 = DSRC;
	  u2 = SRC;
	  uresult = u1 & u2;
	  TRACE_ALU (cpu, "AND: %#x & %#x = %#x",
		     u1, u2, uresult);
	  DEST (uresult);
	  FLAGS (uresult, uresult != 0);
	}
      break;

    case MSO_bit:
      for (rept = 0; rept < n_repeats; rept ++)
	{
	  u1 = DSRC;
	  u2 = SRC;
	  uresult = u1 & u2;
	  TRACE_ALU (cpu, "BIT: %#x & %#x -> %#x",
		     u1, u2, uresult);
	  FLAGS (uresult, uresult != 0);
	}
      break;

    case MSO_bic:
      for (rept = 0; rept < n_repeats; rept ++)
	{
	  u1 = DSRC;
	  u2 = SRC;
	  uresult = u1 & ~ u2;
	  TRACE_ALU (cpu, "BIC: %#x & ~ %#x = %#x",
		     u1, u2, uresult);
	  DEST (uresult);
	}
      break;

    case MSO_bis:
      for (rept = 0; rept < n_repeats; rept ++)
	{
	  u1 = DSRC;
	  u2 = SRC;
	  uresult = u1 | u2;
	  TRACE_ALU (cpu, "BIS: %#x | %#x = %#x",
		     u1, u2, uresult);
	  DEST (uresult);
	}
      break;

    case MSO_xor:
      for (rept = 0; rept < n_repeats; rept ++)
	{
	  s1 = 1 << (opcode->size - 1);
	  u1 = DSRC;
	  u2 = SRC;
	  uresult = u1 ^ u2;
	  TRACE_ALU (cpu, "XOR: %#x & %#x = %#x",
		     u1, u2, uresult);
	  DEST (uresult);
	  FLAGSV (uresult, uresult != 0, (u1 & s1) && (u2 & s1));
	}
      break;

    /* Single-operand instructions.  Note: the decoder puts the same
       operand in SRC as in DEST, for our convenience.  */

    case MSO_rrc:
      for (rept = 0; rept < n_repeats; rept ++)
	{
	  u1 = SRC;
	  carry_to_use = u1 & 1;
	  uresult = u1 >> 1;
	  /* If the ZC bit of the opcode is set, it means we are synthesizing
	     RRUX, so the carry bit must be ignored.  */
	  if (opcode->zc == 0 && (SR & MSP430_FLAG_C))
	    uresult |= (1 << (opcode->size - 1));
	  TRACE_ALU (cpu, "RRC: %#x >>= %#x",
		     u1, uresult);
	  DEST (uresult);
	  FLAGS (uresult, carry_to_use);
	}
      break;

    case MSO_swpb:
      for (rept = 0; rept < n_repeats; rept ++)
	{
	  u1 = SRC;
	  uresult = ((u1 >> 8) & 0x00ff) | ((u1 << 8) & 0xff00);
	  TRACE_ALU (cpu, "SWPB: %#x -> %#x",
		     u1, uresult);
	  DEST (uresult);
	}
      break;

    case MSO_rra:
      for (rept = 0; rept < n_repeats; rept ++)
	{
	  u1 = SRC;
	  c = u1 & 1;
	  s1 = 1 << (opcode->size - 1);
	  uresult = (u1 >> 1) | (u1 & s1);
	  TRACE_ALU (cpu, "RRA: %#x >>= %#x",
		     u1, uresult);
	  DEST (uresult);
	  FLAGS (uresult, c);
	}
      break;

    case MSO_rru:
      for (rept = 0; rept < n_repeats; rept ++)
	{
	  u1 = SRC;
	  c = u1 & 1;
	  uresult = (u1 >> 1);
	  TRACE_ALU (cpu, "RRU: %#x >>= %#x",
		     u1, uresult);
	  DEST (uresult);
	  FLAGS (uresult, c);
	}
      break;

    case MSO_sxt:
      for (rept = 0; rept < n_repeats; rept ++)
	{
	  u1 = SRC;
	  if (u1 & 0x80)
	    uresult = u1 | 0xfff00;
	  else
	    uresult = u1 & 0x000ff;
	  TRACE_ALU (cpu, "SXT: %#x -> %#x", u1, uresult);
	  DEST (uresult);
	  FLAGS (uresult, c);
	}
      break;

    case MSO_push:
      for (rept = 0; rept < n_repeats; rept ++)
	{
	  int new_sp;

	  new_sp = REG_GET (MSR_SP) - op_bytes;
	  /* SP is always word-aligned.  */
	  if (new_sp & 1)
	    new_sp --;
	  REG_PUT (MSR_SP, new_sp);
	  u1 = SRC;
	  mem_put_val (sd, SP, u1, op_bits);
	  if (opcode->op[1].type == MSP430_Operand_Register)
	    opcode->op[1].reg --;
	}
      break;

    case MSO_pop:
      for (rept = 0; rept < n_repeats; rept ++)
	{
	  int new_sp;

	  u1 = mem_get_val (sd, SP, op_bits);
	  DEST (u1);
	  if (opcode->op[0].type == MSP430_Operand_Register)
	    opcode->op[0].reg ++;
	  new_sp = REG_GET (MSR_SP) + op_bytes;
	  /* SP is always word-aligned.  */
	  if (new_sp & 1)
	    new_sp ++;
	  REG_PUT (MSR_SP, new_sp);
	}
      break;

    case MSO_call:
      u1 = SRC;

      if (maybe_perform_syscall (sd, u1))
	break;

      REG_PUT (MSR_SP, REG_GET (MSR_SP) - op_bytes);
      mem_put_val (sd, SP, PC, op_bits);
      TRACE_ALU (cpu, "CALL: func %#x ret %#x, sp %#x",
	         u1, PC, SP);
      REG_PUT (MSR_PC, u1);
      break;

    case MSO_reti:
      u1 = mem_get_val (sd, SP, 16);
      SR = u1 & 0xFF;
      SP += 2;
      PC = mem_get_val (sd, SP, 16);
      SP += 2;
      /* Emulate the RETI action of the 20-bit CPUX architecure.
	 This is safe for 16-bit CPU architectures as well, since the top
	 8-bits of SR will have been written to the stack here, and will
	 have been read as 0.  */
      PC |= (u1 & 0xF000) << 4;
      TRACE_ALU (cpu, "RETI: pc %#x sr %#x", PC, SR);
      break;

      /* Jumps.  */

    case MSO_jmp:
      i = SRC;
      switch (opcode->cond)
	{
	case MSC_nz:
	  u1 = (SR & MSP430_FLAG_Z) ? 0 : 1;
	  break;
	case MSC_z:
	  u1 = (SR & MSP430_FLAG_Z) ? 1 : 0;
	  break;
	case MSC_nc:
	  u1 = (SR & MSP430_FLAG_C) ? 0 : 1;
	  break;
	case MSC_c:
	  u1 = (SR & MSP430_FLAG_C) ? 1 : 0;
	  break;
	case MSC_n:
	  u1 = (SR & MSP430_FLAG_N) ? 1 : 0;
	  break;
	case MSC_ge:
	  u1 = (!!(SR & MSP430_FLAG_N) == !!(SR & MSP430_FLAG_V)) ? 1 : 0;
	  break;
	case MSC_l:
	  u1 = (!!(SR & MSP430_FLAG_N) == !!(SR & MSP430_FLAG_V)) ? 0 : 1;
	  break;
	case MSC_true:
	  u1 = 1;
	  break;
	}

      if (u1)
	{
	  TRACE_BRANCH (cpu, "J%s: pc %#x -> %#x sr %#x, taken",
			cond_string (opcode->cond), PC, i, SR);
	  PC = i;
	  if (PC == opcode_pc)
	    exit (0);
	}
      else
	TRACE_BRANCH (cpu, "J%s: pc %#x to %#x sr %#x, not taken",
		      cond_string (opcode->cond), PC, i, SR);
      break;

    default:
      fprintf (stderr, "error: unexpected opcode id %d\n", opcode->id);
      exit (1);
    }
}

void
sim_engine_run (SIM_DESC sd,
		int next_cpu_nr,
		int nr_cpus,
		int siggnal)
{
  while (1)
    {
      msp430_step_once (sd);
      if (sim_events_tick (sd))
	sim_events_process (sd);
    }
}
