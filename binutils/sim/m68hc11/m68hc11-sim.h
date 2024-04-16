/* sim-main.h -- Simulator for Motorola 68HC11 & 68HC12
   Copyright (C) 1999-2024 Free Software Foundation, Inc.
   Written by Stephane Carrez (stcarrez@nerim.fr)

This file is part of the GNU simulators.

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

#ifndef M68HC11_SIM_H
#define M68HC11_SIM_H

#include "sim-basics.h"
#include "sim-base.h"

#include "opcode/m68hc11.h"

#include "sim/sim.h"
#include "opcode/m68hc11.h"
#include "sim-signal.h"
#include "sim-types.h"

#include "interrupts.h"
#include <setjmp.h>

/* Specifies the level of mapping for the IO, EEprom, nvram and external
   RAM.  IO registers are mapped over everything and the external RAM
   is last (ie, it can be hidden by everything above it in the list).  */
enum m68hc11_map_level
{
  M6811_IO_LEVEL,
  M6811_EEPROM_LEVEL,
  M6811_NVRAM_LEVEL,
  M6811_RAM_LEVEL
};

enum cpu_type
{
  CPU_M6811,
  CPU_M6812
};

#define X_REGNUM 	0
#define D_REGNUM	1
#define Y_REGNUM        2
#define SP_REGNUM 	3
#define PC_REGNUM 	4
#define A_REGNUM        5
#define B_REGNUM        6
#define PSW_REGNUM 	7
#define PAGE_REGNUM     8
#define Z_REGNUM        9

typedef struct m6811_regs {
    unsigned short      d;
    unsigned short      ix;
    unsigned short      iy;
    unsigned short      sp;
    unsigned short      pc;
    unsigned char       ccr;
  unsigned short      page;
} m6811_regs;


/* Description of 68HC11 IO registers.  Such description is only provided
   for the info command to display the current setting of IO registers
   from GDB.  */
struct io_reg_desc
{
  int        mask;
  const char *short_name;
  const char *long_name;
};
typedef struct io_reg_desc io_reg_desc;

extern void print_io_reg_desc (SIM_DESC sd, io_reg_desc *desc, int val,
			       int mode);
extern void print_io_byte (SIM_DESC sd, const char *name,
			   io_reg_desc *desc, uint8_t val, uint16_t addr);
extern void print_io_word (SIM_DESC sd, const char *name,
			   io_reg_desc *desc, uint16_t val, uint16_t addr);


/* List of special 68HC11&68HC12 instructions that are not handled by the
   'gencode.c' generator.  These complex instructions are implemented
   by 'cpu_special'.  */
enum M6811_Special
{
  /* 68HC11 instructions.  */
  M6811_DAA,
  M6811_EMUL_SYSCALL,
  M6811_ILLEGAL,
  M6811_RTI,
  M6811_STOP,
  M6811_SWI,
  M6811_TEST,
  M6811_WAI,

  /* 68HC12 instructions.  */
  M6812_BGND,
  M6812_CALL,
  M6812_CALL_INDIRECT,
  M6812_IDIVS,
  M6812_EDIV,
  M6812_EDIVS,
  M6812_EMACS,
  M6812_EMUL,
  M6812_EMULS,
  M6812_ETBL,
  M6812_MEM,
  M6812_REV,
  M6812_REVW,
  M6812_RTC,
  M6812_RTI,
  M6812_WAV
};

#define M6811_MAX_PORTS (0x03f+1)
#define M6812_MAX_PORTS (0x3ff+1)
#define MAX_PORTS       (M6812_MAX_PORTS)

typedef void (* cpu_interp) (sim_cpu *);

struct m68hc11_sim_cpu {
  /* CPU registers.  */
  struct m6811_regs     cpu_regs;

  /* CPU interrupts.  */
  struct interrupts     cpu_interrupts;

  /* Pointer to the interpretor routine.  */
  cpu_interp            cpu_interpretor;

  /* Pointer to the architecture currently configured in the simulator.  */
  const struct bfd_arch_info  *cpu_configured_arch;

  /* CPU absolute cycle time.  The cycle time is updated after
     each instruction, by the number of cycles taken by the instruction.
     It is cleared only when reset occurs.  */
  int64_t              cpu_absolute_cycle;

  /* Number of cycles to increment after the current instruction.
     This is also the number of ticks for the generic event scheduler.  */
  uint8_t                 cpu_current_cycle;
  int                   cpu_emul_syscall;
  int                   cpu_is_initialized;
  int                   cpu_running;
  int                   cpu_check_memory;
  int                   cpu_stop_on_interrupt;

  /* When this is set, start execution of program at address specified
     in the ELF header.  This is used for testing some programs that do not
     have an interrupt table linked with them.  Programs created during the
     GCC validation are like this. A normal 68HC11 does not behave like
     this (unless there is some OS or downloadable feature).  */
  int                   cpu_use_elf_start;

  /* The starting address specified in ELF header.  */
  int                   cpu_elf_start;

  uint16_t                cpu_insn_pc;

  /* CPU frequency.  This is the quartz frequency.  It is divided by 4 to
     get the cycle time.  This is used for the timer rate and for the baud
     rate generation.  */
  unsigned long         cpu_frequency;

  /* The mode in which the CPU is configured (MODA and MODB pins).  */
  unsigned int          cpu_mode;
  const char*           cpu_start_mode;

  /* The cpu being configured.  */
  enum cpu_type         cpu_type;

  /* Initial value of the CONFIG register.  */
  uint8_t                 cpu_config;
  uint8_t                 cpu_use_local_config;

  uint8_t                 ios[MAX_PORTS];

  /* Memory bank parameters which describe how the memory bank window
     is mapped in memory and how to convert it in virtual address.  */
  uint16_t                bank_start;
  uint16_t                bank_end;
  address_word          bank_virtual;
  unsigned              bank_shift;


  struct hw            *hw_cpu;
};
#define M68HC11_SIM_CPU(cpu) ((struct m68hc11_sim_cpu *) CPU_ARCH_DATA (cpu))

/* Returns the cpu absolute cycle time (A virtual counter incremented
   at each 68HC11 E clock).  */
#define cpu_current_cycle(cpu)    (M68HC11_SIM_CPU (cpu)->cpu_absolute_cycle)
#define cpu_add_cycles(cpu, T)    (M68HC11_SIM_CPU (cpu)->cpu_current_cycle += (int64_t) (T))
#define cpu_is_running(cpu)       (M68HC11_SIM_CPU (cpu)->cpu_running)

/* Get the IO/RAM base addresses depending on the M6811_INIT register.  */
#define cpu_get_io_base(cpu) \
  (((uint16_t)((M68HC11_SIM_CPU (cpu)->ios[M6811_INIT]) & 0x0F)) << 12)
#define cpu_get_reg_base(cpu) \
  (((uint16_t)((M68HC11_SIM_CPU (cpu)->ios[M6811_INIT]) & 0xF0)) << 8)

/* Returns the different CPU registers.  */
#define cpu_get_ccr(cpu)          (M68HC11_SIM_CPU (cpu)->cpu_regs.ccr)
#define cpu_get_pc(cpu)           (M68HC11_SIM_CPU (cpu)->cpu_regs.pc)
#define cpu_get_d(cpu)            (M68HC11_SIM_CPU (cpu)->cpu_regs.d)
#define cpu_get_x(cpu)            (M68HC11_SIM_CPU (cpu)->cpu_regs.ix)
#define cpu_get_y(cpu)            (M68HC11_SIM_CPU (cpu)->cpu_regs.iy)
#define cpu_get_sp(cpu)           (M68HC11_SIM_CPU (cpu)->cpu_regs.sp)
#define cpu_get_a(cpu)            ((M68HC11_SIM_CPU (cpu)->cpu_regs.d >> 8) & 0x0FF)
#define cpu_get_b(cpu)            (M68HC11_SIM_CPU (cpu)->cpu_regs.d & 0x0FF)
#define cpu_get_page(cpu)         (M68HC11_SIM_CPU (cpu)->cpu_regs.page)

/* 68HC12 specific and Motorola internal registers.  */
#define cpu_get_tmp3(cpu)         (0)
#define cpu_get_tmp2(cpu)         (0)

#define cpu_set_d(cpu, val)       (M68HC11_SIM_CPU (cpu)->cpu_regs.d = (val))
#define cpu_set_x(cpu, val)       (M68HC11_SIM_CPU (cpu)->cpu_regs.ix = (val))
#define cpu_set_y(cpu, val)       (M68HC11_SIM_CPU (cpu)->cpu_regs.iy = (val))
#define cpu_set_page(cpu, val)    (M68HC11_SIM_CPU (cpu)->cpu_regs.page = (val))

/* 68HC12 specific and Motorola internal registers.  */
#define cpu_set_tmp3(cpu, val)    (0)
#define cpu_set_tmp2(cpu, val)    (void) (0)

#if 0
/* This is a function in m68hc11_sim.c to keep track of the frame.  */
#define cpu_set_sp(cpu, val)      (M68HC11_SIM_CPU (cpu)->cpu_regs.sp = (val))
#endif

#define cpu_set_pc(cpu, val)      (M68HC11_SIM_CPU (cpu)->cpu_regs.pc = (val))

#define cpu_set_a(cpu, val)  \
  cpu_set_d(cpu, ((val) << 8) | cpu_get_b (cpu))
#define cpu_set_b(cpu, val)  \
  cpu_set_d(cpu, ((cpu_get_a (cpu)) << 8) | ((val) & 0x0FF))

#define cpu_set_ccr(cpu, val)     (M68HC11_SIM_CPU (cpu)->cpu_regs.ccr = (val))
#define cpu_get_ccr_H(cpu)        ((cpu_get_ccr (cpu) & M6811_H_BIT) ? 1 : 0)
#define cpu_get_ccr_X(cpu)        ((cpu_get_ccr (cpu) & M6811_X_BIT) ? 1 : 0)
#define cpu_get_ccr_S(cpu)        ((cpu_get_ccr (cpu) & M6811_S_BIT) ? 1 : 0)
#define cpu_get_ccr_N(cpu)        ((cpu_get_ccr (cpu) & M6811_N_BIT) ? 1 : 0)
#define cpu_get_ccr_V(cpu)        ((cpu_get_ccr (cpu) & M6811_V_BIT) ? 1 : 0)
#define cpu_get_ccr_C(cpu)        ((cpu_get_ccr (cpu) & M6811_C_BIT) ? 1 : 0)
#define cpu_get_ccr_Z(cpu)        ((cpu_get_ccr (cpu) & M6811_Z_BIT) ? 1 : 0)
#define cpu_get_ccr_I(cpu)        ((cpu_get_ccr (cpu) & M6811_I_BIT) ? 1 : 0)

#define cpu_set_ccr_flag(S, B, V) \
  cpu_set_ccr (S, (cpu_get_ccr (S) & ~(B)) | ((V) ? (B) : 0))

#define cpu_set_ccr_H(cpu, val)   cpu_set_ccr_flag (cpu, M6811_H_BIT, val)
#define cpu_set_ccr_X(cpu, val)   cpu_set_ccr_flag (cpu, M6811_X_BIT, val)
#define cpu_set_ccr_S(cpu, val)   cpu_set_ccr_flag (cpu, M6811_S_BIT, val)
#define cpu_set_ccr_N(cpu, val)   cpu_set_ccr_flag (cpu, M6811_N_BIT, val)
#define cpu_set_ccr_V(cpu, val)   cpu_set_ccr_flag (cpu, M6811_V_BIT, val)
#define cpu_set_ccr_C(cpu, val)   cpu_set_ccr_flag (cpu, M6811_C_BIT, val)
#define cpu_set_ccr_Z(cpu, val)   cpu_set_ccr_flag (cpu, M6811_Z_BIT, val)
#define cpu_set_ccr_I(cpu, val)   cpu_set_ccr_flag (cpu, M6811_I_BIT, val)

extern void cpu_memory_exception (sim_cpu *cpu,
                                  SIM_SIGNAL excep,
                                  uint16_t addr,
                                  const char *message);

STATIC_INLINE UNUSED address_word
phys_to_virt (sim_cpu *cpu, address_word addr)
{
  struct m68hc11_sim_cpu *m68hc11_cpu = M68HC11_SIM_CPU (cpu);

  if (addr >= m68hc11_cpu->bank_start && addr < m68hc11_cpu->bank_end)
    return ((address_word) (addr - m68hc11_cpu->bank_start)
            + (((address_word) m68hc11_cpu->cpu_regs.page) << m68hc11_cpu->bank_shift)
            + m68hc11_cpu->bank_virtual);
  else
    return (address_word) (addr);
}

STATIC_INLINE UNUSED uint8_t
memory_read8 (sim_cpu *cpu, uint16_t addr)
{
  uint8_t val;

  if (sim_core_read_buffer (CPU_STATE (cpu), cpu, 0, &val, addr, 1) != 1)
    {
      cpu_memory_exception (cpu, SIM_SIGSEGV, addr,
                            "Read error");
    }
  return val;
}

STATIC_INLINE UNUSED void
memory_write8 (sim_cpu *cpu, uint16_t addr, uint8_t val)
{
  if (sim_core_write_buffer (CPU_STATE (cpu), cpu, 0, &val, addr, 1) != 1)
    {
      cpu_memory_exception (cpu, SIM_SIGSEGV, addr,
                            "Write error");
    }
}

STATIC_INLINE UNUSED uint16_t
memory_read16 (sim_cpu *cpu, uint16_t addr)
{
  uint8_t b[2];

  if (sim_core_read_buffer (CPU_STATE (cpu), cpu, 0, b, addr, 2) != 2)
    {
      cpu_memory_exception (cpu, SIM_SIGSEGV, addr,
                            "Read error");
    }
  return (((uint16_t) (b[0])) << 8) | ((uint16_t) b[1]);
}

STATIC_INLINE UNUSED void
memory_write16 (sim_cpu *cpu, uint16_t addr, uint16_t val)
{
  uint8_t b[2];

  b[0] = val >> 8;
  b[1] = val;
  if (sim_core_write_buffer (CPU_STATE (cpu), cpu, 0, b, addr, 2) != 2)
    {
      cpu_memory_exception (cpu, SIM_SIGSEGV, addr,
                            "Write error");
    }
}
extern void
cpu_ccr_update_tst8 (sim_cpu *cpu, uint8_t val);

STATIC_INLINE UNUSED void
cpu_ccr_update_tst16 (sim_cpu *cpu, uint16_t val)
{
  cpu_set_ccr_V (cpu, 0);
  cpu_set_ccr_N (cpu, val & 0x8000 ? 1 : 0);
  cpu_set_ccr_Z (cpu, val == 0 ? 1 : 0);
}

STATIC_INLINE UNUSED void
cpu_ccr_update_shift8 (sim_cpu *cpu, uint8_t val)
{
  cpu_set_ccr_N (cpu, val & 0x80 ? 1 : 0);
  cpu_set_ccr_Z (cpu, val == 0 ? 1 : 0);
  cpu_set_ccr_V (cpu, cpu_get_ccr_N (cpu) ^ cpu_get_ccr_C (cpu));
}

STATIC_INLINE UNUSED void
cpu_ccr_update_shift16 (sim_cpu *cpu, uint16_t val)
{
  cpu_set_ccr_N (cpu, val & 0x8000 ? 1 : 0);
  cpu_set_ccr_Z (cpu, val == 0 ? 1 : 0);
  cpu_set_ccr_V (cpu, cpu_get_ccr_N (cpu) ^ cpu_get_ccr_C (cpu));
}

STATIC_INLINE UNUSED void
cpu_ccr_update_add8 (sim_cpu *cpu, uint8_t r, uint8_t a, uint8_t b)
{
  cpu_set_ccr_C (cpu, ((a & b) | (b & ~r) | (a & ~r)) & 0x80 ? 1 : 0);
  cpu_set_ccr_V (cpu, ((a & b & ~r) | (~a & ~b & r)) & 0x80 ? 1 : 0);
  cpu_set_ccr_Z (cpu, r == 0);
  cpu_set_ccr_N (cpu, r & 0x80 ? 1 : 0);
}


STATIC_INLINE UNUSED void
cpu_ccr_update_sub8 (sim_cpu *cpu, uint8_t r, uint8_t a, uint8_t b)
{
  cpu_set_ccr_C (cpu, ((~a & b) | (b & r) | (~a & r)) & 0x80 ? 1 : 0);
  cpu_set_ccr_V (cpu, ((a & ~b & ~r) | (~a & b & r)) & 0x80 ? 1 : 0);
  cpu_set_ccr_Z (cpu, r == 0);
  cpu_set_ccr_N (cpu, r & 0x80 ? 1 : 0);
}

STATIC_INLINE UNUSED void
cpu_ccr_update_add16 (sim_cpu *cpu, uint16_t r, uint16_t a, uint16_t b)
{
  cpu_set_ccr_C (cpu, ((a & b) | (b & ~r) | (a & ~r)) & 0x8000 ? 1 : 0);
  cpu_set_ccr_V (cpu, ((a & b & ~r) | (~a & ~b & r)) & 0x8000 ? 1 : 0);
  cpu_set_ccr_Z (cpu, r == 0);
  cpu_set_ccr_N (cpu, r & 0x8000 ? 1 : 0);
}

STATIC_INLINE UNUSED void
cpu_ccr_update_sub16 (sim_cpu *cpu, uint16_t r, uint16_t a, uint16_t b)
{
  cpu_set_ccr_C (cpu, ((~a & b) | (b & r) | (~a & r)) & 0x8000 ? 1 : 0);
  cpu_set_ccr_V (cpu, ((a & ~b & ~r) | (~a & b & r)) & 0x8000 ? 1 : 0);
  cpu_set_ccr_Z (cpu, r == 0);
  cpu_set_ccr_N (cpu, r & 0x8000 ? 1 : 0);
}

/* Push and pop instructions for 68HC11 (next-available stack mode).  */
STATIC_INLINE UNUSED void
cpu_m68hc11_push_uint8 (sim_cpu *cpu, uint8_t val)
{
  struct m68hc11_sim_cpu *m68hc11_cpu = M68HC11_SIM_CPU (cpu);
  uint16_t addr = m68hc11_cpu->cpu_regs.sp;

  memory_write8 (cpu, addr, val);
  m68hc11_cpu->cpu_regs.sp = addr - 1;
}

STATIC_INLINE UNUSED void
cpu_m68hc11_push_uint16 (sim_cpu *cpu, uint16_t val)
{
  struct m68hc11_sim_cpu *m68hc11_cpu = M68HC11_SIM_CPU (cpu);
  uint16_t addr = m68hc11_cpu->cpu_regs.sp - 1;

  memory_write16 (cpu, addr, val);
  m68hc11_cpu->cpu_regs.sp = addr - 1;
}

STATIC_INLINE UNUSED uint8_t
cpu_m68hc11_pop_uint8 (sim_cpu *cpu)
{
  struct m68hc11_sim_cpu *m68hc11_cpu = M68HC11_SIM_CPU (cpu);
  uint16_t addr = m68hc11_cpu->cpu_regs.sp;
  uint8_t val;

  val = memory_read8 (cpu, addr + 1);
  m68hc11_cpu->cpu_regs.sp = addr + 1;
  return val;
}

STATIC_INLINE UNUSED uint16_t
cpu_m68hc11_pop_uint16 (sim_cpu *cpu)
{
  struct m68hc11_sim_cpu *m68hc11_cpu = M68HC11_SIM_CPU (cpu);
  uint16_t addr = m68hc11_cpu->cpu_regs.sp;
  uint16_t val;

  val = memory_read16 (cpu, addr + 1);
  m68hc11_cpu->cpu_regs.sp = addr + 2;
  return val;
}

/* Push and pop instructions for 68HC12 (last-used stack mode).  */
STATIC_INLINE UNUSED void
cpu_m68hc12_push_uint8 (sim_cpu *cpu, uint8_t val)
{
  struct m68hc11_sim_cpu *m68hc11_cpu = M68HC11_SIM_CPU (cpu);
  uint16_t addr = m68hc11_cpu->cpu_regs.sp;

  addr --;
  memory_write8 (cpu, addr, val);
  m68hc11_cpu->cpu_regs.sp = addr;
}

STATIC_INLINE UNUSED void
cpu_m68hc12_push_uint16 (sim_cpu *cpu, uint16_t val)
{
  struct m68hc11_sim_cpu *m68hc11_cpu = M68HC11_SIM_CPU (cpu);
  uint16_t addr = m68hc11_cpu->cpu_regs.sp;

  addr -= 2;
  memory_write16 (cpu, addr, val);
  m68hc11_cpu->cpu_regs.sp = addr;
}

STATIC_INLINE UNUSED uint8_t
cpu_m68hc12_pop_uint8 (sim_cpu *cpu)
{
  struct m68hc11_sim_cpu *m68hc11_cpu = M68HC11_SIM_CPU (cpu);
  uint16_t addr = m68hc11_cpu->cpu_regs.sp;
  uint8_t val;

  val = memory_read8 (cpu, addr);
  m68hc11_cpu->cpu_regs.sp = addr + 1;
  return val;
}

STATIC_INLINE UNUSED uint16_t
cpu_m68hc12_pop_uint16 (sim_cpu *cpu)
{
  struct m68hc11_sim_cpu *m68hc11_cpu = M68HC11_SIM_CPU (cpu);
  uint16_t addr = m68hc11_cpu->cpu_regs.sp;
  uint16_t val;

  val = memory_read16 (cpu, addr);
  m68hc11_cpu->cpu_regs.sp = addr + 2;
  return val;
}

/* Fetch a 8/16 bit value and update the PC.  */
STATIC_INLINE UNUSED uint8_t
cpu_fetch8 (sim_cpu *cpu)
{
  struct m68hc11_sim_cpu *m68hc11_cpu = M68HC11_SIM_CPU (cpu);
  uint16_t addr = m68hc11_cpu->cpu_regs.pc;
  uint8_t val;

  val = memory_read8 (cpu, addr);
  m68hc11_cpu->cpu_regs.pc = addr + 1;
  return val;
}

STATIC_INLINE UNUSED uint16_t
cpu_fetch16 (sim_cpu *cpu)
{
  struct m68hc11_sim_cpu *m68hc11_cpu = M68HC11_SIM_CPU (cpu);
  uint16_t addr = m68hc11_cpu->cpu_regs.pc;
  uint16_t val;

  val = memory_read16 (cpu, addr);
  m68hc11_cpu->cpu_regs.pc = addr + 2;
  return val;
}

extern void cpu_call (sim_cpu *cpu, uint16_t addr);
extern void cpu_exg (sim_cpu *cpu, uint8_t code);
extern void cpu_dbcc (sim_cpu *cpu);
extern void cpu_special (sim_cpu *cpu, enum M6811_Special special);
extern void cpu_move8 (sim_cpu *cpu, uint8_t op);
extern void cpu_move16 (sim_cpu *cpu, uint8_t op);

extern uint16_t cpu_fetch_relbranch (sim_cpu *cpu);
extern uint16_t cpu_fetch_relbranch16 (sim_cpu *cpu);
extern void cpu_push_all (sim_cpu *cpu);
extern void cpu_single_step (sim_cpu *cpu);

extern void cpu_info (SIM_DESC sd, sim_cpu *cpu);

extern int cpu_initialize (SIM_DESC sd, sim_cpu *cpu);

/* Returns the address of a 68HC12 indexed operand.
   Pre and post modifications are handled on the source register.  */
extern uint16_t cpu_get_indexed_operand_addr (sim_cpu *cpu, int restricted);

extern void cpu_return (sim_cpu *cpu);
extern void cpu_set_sp (sim_cpu *cpu, uint16_t val);
extern int cpu_reset (sim_cpu *cpu);
extern int cpu_restart (sim_cpu *cpu);
extern void sim_memory_error (sim_cpu *cpu, SIM_SIGNAL excep,
			      uint16_t addr, const char *message, ...)
  ATTRIBUTE_PRINTF (4, 5);
extern void emul_os (int op, sim_cpu *cpu);
extern void cpu_interp_m6811 (sim_cpu *cpu);
extern void cpu_interp_m6812 (sim_cpu *cpu);

extern int m68hc11cpu_set_oscillator (SIM_DESC sd, const char *port,
				      double ton, double toff,
				      int64_t repeat);
extern int m68hc11cpu_clear_oscillator (SIM_DESC sd, const char *port);
extern void m68hc11cpu_set_port (struct hw *me, sim_cpu *cpu,
				 unsigned addr, uint8_t val);

extern void sim_board_reset (SIM_DESC sd);

#define PRINT_TIME  0x01
#define PRINT_CYCLE 0x02
extern const char *cycle_to_string (sim_cpu *cpu, int64_t t, int flags);

#endif
