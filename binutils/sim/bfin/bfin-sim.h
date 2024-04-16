/* Simulator for Analog Devices Blackfin processors.

   Copyright (C) 2005-2024 Free Software Foundation, Inc.
   Contributed by Analog Devices, Inc.

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

#ifndef _BFIN_SIM_H_
#define _BFIN_SIM_H_

#include <stdbool.h>
#include <stdint.h>

typedef uint8_t bu8;
typedef uint16_t bu16;
typedef uint32_t bu32;
typedef uint64_t bu40;
typedef uint64_t bu64;
typedef int8_t bs8;
typedef int16_t bs16;
typedef int32_t bs32;
typedef int64_t bs40;
typedef int64_t bs64;

#include "machs.h"

/* For dealing with parallel instructions, we must avoid changing our register
   file until all parallel insns have been simulated.  This queue of stores
   can be used to delay a modification.
   XXX: Should go and convert all 32 bit insns to use this.  */
struct store {
  bu32 *addr;
  bu32 val;
};

enum bfin_parallel_group {
  BFIN_PARALLEL_NONE,
  BFIN_PARALLEL_GROUP0,	/* 32bit slot.  */
  BFIN_PARALLEL_GROUP1,	/* 16bit group1.  */
  BFIN_PARALLEL_GROUP2,	/* 16bit group2.  */
};

/* The KSP/USP handling wrt SP may not follow the hardware exactly (the hw
   looks at current mode and uses either SP or USP based on that.  We instead
   always operate on SP and mirror things in KSP and USP.  During a CEC
   transition, we take care of syncing the values.  This lowers the simulation
   complexity and speeds things up a bit.  */
struct bfin_cpu_state
{
  bu32 dpregs[16], iregs[4], mregs[4], bregs[4], lregs[4], cycles[3];
  bu32 ax[2], aw[2];
  bu32 lt[2], lc[2], lb[2];
  bu32 ksp, usp, seqstat, syscfg, rets, reti, retx, retn, rete;
  bu32 pc, emudat[2];
  /* These ASTAT flags need not be bu32, but it makes pointers easier.  */
  bu32 ac0, ac0_copy, ac1, an, aq;
  union { struct { bu32 av0;  bu32 av1;  }; bu32 av [2]; };
  union { struct { bu32 av0s; bu32 av1s; }; bu32 avs[2]; };
  bu32 az, cc, v, v_copy, vs;
  bu32 rnd_mod;
  bu32 v_internal;
  bu32 astat_reserved;

  /* Set by an instruction emulation function if we performed a jump.  We
     cannot compare oldpc to newpc as this ignores the "jump 0;" case.  */
  bool did_jump;

  /* Used by the CEC to figure out where to return to.  */
  bu32 insn_len;

  /* How many cycles did this insn take to complete ?  */
  bu32 cycle_delay;

  /* The pc currently being interpreted in parallel insns.  */
  bu32 multi_pc;

  /* Some insns are valid in group1, and others in group2, so we
     need to keep track of the exact slot we're processing.  */
  enum bfin_parallel_group group;

  /* Needed for supporting the DISALGNEXCPT instruction */
  int dis_algn_expt;

  /* See notes above for struct store.  */
  struct store stores[20];
  int n_stores;

#if (WITH_HW)
  /* Cache heavily used CPU-specific device pointers.  */
  void *cec_cache;
  void *evt_cache;
  void *mmu_cache;
  void *trace_cache;
#endif
};

#define REG_H_L(h, l)	(((h) & 0xffff0000) | ((l) & 0x0000ffff))

#define DREG(x)		(BFIN_CPU_STATE.dpregs[x])
#define PREG(x)		(BFIN_CPU_STATE.dpregs[x + 8])
#define SPREG		PREG (6)
#define FPREG		PREG (7)
#define IREG(x)		(BFIN_CPU_STATE.iregs[x])
#define MREG(x)		(BFIN_CPU_STATE.mregs[x])
#define BREG(x)		(BFIN_CPU_STATE.bregs[x])
#define LREG(x)		(BFIN_CPU_STATE.lregs[x])
#define AXREG(x)	(BFIN_CPU_STATE.ax[x])
#define AWREG(x)	(BFIN_CPU_STATE.aw[x])
#define CCREG		(BFIN_CPU_STATE.cc)
#define LCREG(x)	(BFIN_CPU_STATE.lc[x])
#define LTREG(x)	(BFIN_CPU_STATE.lt[x])
#define LBREG(x)	(BFIN_CPU_STATE.lb[x])
#define CYCLESREG	(BFIN_CPU_STATE.cycles[0])
#define CYCLES2REG	(BFIN_CPU_STATE.cycles[1])
#define CYCLES2SHDREG	(BFIN_CPU_STATE.cycles[2])
#define KSPREG		(BFIN_CPU_STATE.ksp)
#define USPREG		(BFIN_CPU_STATE.usp)
#define SEQSTATREG	(BFIN_CPU_STATE.seqstat)
#define SYSCFGREG	(BFIN_CPU_STATE.syscfg)
#define RETSREG		(BFIN_CPU_STATE.rets)
#define RETIREG		(BFIN_CPU_STATE.reti)
#define RETXREG		(BFIN_CPU_STATE.retx)
#define RETNREG		(BFIN_CPU_STATE.retn)
#define RETEREG		(BFIN_CPU_STATE.rete)
#define PCREG		(BFIN_CPU_STATE.pc)
#define EMUDAT_INREG	(BFIN_CPU_STATE.emudat[0])
#define EMUDAT_OUTREG	(BFIN_CPU_STATE.emudat[1])
#define INSN_LEN	(BFIN_CPU_STATE.insn_len)
#define PARALLEL_GROUP	(BFIN_CPU_STATE.group)
#define CYCLE_DELAY	(BFIN_CPU_STATE.cycle_delay)
#define DIS_ALGN_EXPT	(BFIN_CPU_STATE.dis_algn_expt)

#define EXCAUSE_SHIFT		0
#define EXCAUSE_MASK		(0x3f << EXCAUSE_SHIFT)
#define EXCAUSE			((SEQSTATREG & EXCAUSE_MASK) >> EXCAUSE_SHIFT)
#define HWERRCAUSE_SHIFT	14
#define HWERRCAUSE_MASK		(0x1f << HWERRCAUSE_SHIFT)
#define HWERRCAUSE		((SEQSTATREG & HWERRCAUSE_MASK) >> HWERRCAUSE_SHIFT)

#define _SET_CORE32REG_IDX(reg, p, x, val) \
  do { \
    bu32 __v = (val); \
    TRACE_REGISTER (cpu, "wrote "#p"%i = %#x", x, __v); \
    reg = __v; \
  } while (0)
#define SET_DREG(x, val) _SET_CORE32REG_IDX (DREG (x), R, x, val)
#define SET_PREG(x, val) _SET_CORE32REG_IDX (PREG (x), P, x, val)
#define SET_IREG(x, val) _SET_CORE32REG_IDX (IREG (x), I, x, val)
#define SET_MREG(x, val) _SET_CORE32REG_IDX (MREG (x), M, x, val)
#define SET_BREG(x, val) _SET_CORE32REG_IDX (BREG (x), B, x, val)
#define SET_LREG(x, val) _SET_CORE32REG_IDX (LREG (x), L, x, val)
#define SET_LCREG(x, val) _SET_CORE32REG_IDX (LCREG (x), LC, x, val)
#define SET_LTREG(x, val) _SET_CORE32REG_IDX (LTREG (x), LT, x, val)
#define SET_LBREG(x, val) _SET_CORE32REG_IDX (LBREG (x), LB, x, val)

#define SET_DREG_L_H(x, l, h) SET_DREG (x, REG_H_L (h, l))
#define SET_DREG_L(x, l) SET_DREG (x, REG_H_L (DREG (x), l))
#define SET_DREG_H(x, h) SET_DREG (x, REG_H_L (h, DREG (x)))

#define _SET_CORE32REG_ALU(reg, p, x, val) \
  do { \
    bu32 __v = (val); \
    TRACE_REGISTER (cpu, "wrote A%i"#p" = %#x", x, __v); \
    reg = __v; \
  } while (0)
#define SET_AXREG(x, val) _SET_CORE32REG_ALU (AXREG (x), X, x, val)
#define SET_AWREG(x, val) _SET_CORE32REG_ALU (AWREG (x), W, x, val)

#define SET_AREG(x, val) \
  do { \
    bu40 __a = (val); \
    SET_AXREG (x, (__a >> 32) & 0xff); \
    SET_AWREG (x, __a); \
  } while (0)
#define SET_AREG32(x, val) \
  do { \
    SET_AWREG (x, val); \
    SET_AXREG (x, -(AWREG (x) >> 31)); \
  } while (0)

#define _SET_CORE32REG(reg, val) \
  do { \
    bu32 __v = (val); \
    TRACE_REGISTER (cpu, "wrote "#reg" = %#x", __v); \
    reg##REG = __v; \
  } while (0)
#define SET_FPREG(val) _SET_CORE32REG (FP, val)
#define SET_SPREG(val) _SET_CORE32REG (SP, val)
#define SET_CYCLESREG(val) _SET_CORE32REG (CYCLES, val)
#define SET_CYCLES2REG(val) _SET_CORE32REG (CYCLES2, val)
#define SET_CYCLES2SHDREG(val) _SET_CORE32REG (CYCLES2SHD, val)
#define SET_KSPREG(val) _SET_CORE32REG (KSP, val)
#define SET_USPREG(val) _SET_CORE32REG (USP, val)
#define SET_SYSCFGREG(val) _SET_CORE32REG (SYSCFG, val)
#define SET_RETSREG(val) _SET_CORE32REG (RETS, val)
#define SET_RETIREG(val) _SET_CORE32REG (RETI, val)
#define SET_RETXREG(val) _SET_CORE32REG (RETX, val)
#define SET_RETNREG(val) _SET_CORE32REG (RETN, val)
#define SET_RETEREG(val) _SET_CORE32REG (RETE, val)
#define SET_PCREG(val) _SET_CORE32REG (PC, val)

#define _SET_CORE32REGFIELD(reg, field, val, mask, shift) \
  do { \
    bu32 __f = (val); \
    bu32 __v = ((reg##REG) & ~(mask)) | (__f << (shift)); \
    TRACE_REGISTER (cpu, "wrote "#field" = %#x ("#reg" = %#x)", __f, __v); \
    reg##REG = __v; \
  } while (0)
#define SET_SEQSTATREG(val)   _SET_CORE32REG (SEQSTAT, val)
#define SET_EXCAUSE(excp)     _SET_CORE32REGFIELD (SEQSTAT, EXCAUSE, excp, EXCAUSE_MASK, EXCAUSE_SHIFT)
#define SET_HWERRCAUSE(hwerr) _SET_CORE32REGFIELD (SEQSTAT, HWERRCAUSE, hwerr, HWERRCAUSE_MASK, HWERRCAUSE_SHIFT)

#define AZ_BIT		0
#define AN_BIT		1
#define AC0_COPY_BIT	2
#define V_COPY_BIT	3
#define CC_BIT		5
#define AQ_BIT		6
#define RND_MOD_BIT	8
#define AC0_BIT		12
#define AC1_BIT		13
#define AV0_BIT		16
#define AV0S_BIT	17
#define AV1_BIT		18
#define AV1S_BIT	19
#define V_BIT		24
#define VS_BIT		25
#define ASTAT_DEFINED_BITS \
  ((1 << AZ_BIT) | (1 << AN_BIT) | (1 << AC0_COPY_BIT) | (1 << V_COPY_BIT) \
  |(1 << CC_BIT) | (1 << AQ_BIT) \
  |(1 << RND_MOD_BIT) \
  |(1 << AC0_BIT) | (1 << AC1_BIT) \
  |(1 << AV0_BIT) | (1 << AV0S_BIT) | (1 << AV1_BIT) | (1 << AV1S_BIT) \
  |(1 << V_BIT) | (1 << VS_BIT))

#define ASTATREG(field) (BFIN_CPU_STATE.field)
#define ASTAT_DEPOSIT(field, bit) (ASTATREG(field) << (bit))
#define ASTAT \
  (ASTAT_DEPOSIT(az,       AZ_BIT)       \
  |ASTAT_DEPOSIT(an,       AN_BIT)       \
  |ASTAT_DEPOSIT(ac0_copy, AC0_COPY_BIT) \
  |ASTAT_DEPOSIT(v_copy,   V_COPY_BIT)   \
  |ASTAT_DEPOSIT(cc,       CC_BIT)       \
  |ASTAT_DEPOSIT(aq,       AQ_BIT)       \
  |ASTAT_DEPOSIT(rnd_mod,  RND_MOD_BIT)  \
  |ASTAT_DEPOSIT(ac0,      AC0_BIT)      \
  |ASTAT_DEPOSIT(ac1,      AC1_BIT)      \
  |ASTAT_DEPOSIT(av0,      AV0_BIT)      \
  |ASTAT_DEPOSIT(av0s,     AV0S_BIT)     \
  |ASTAT_DEPOSIT(av1,      AV1_BIT)      \
  |ASTAT_DEPOSIT(av1s,     AV1S_BIT)     \
  |ASTAT_DEPOSIT(v,        V_BIT)        \
  |ASTAT_DEPOSIT(vs,       VS_BIT)       \
  |ASTATREG(astat_reserved))

#define ASTAT_EXTRACT(a, bit)     (((a) >> bit) & 1)
#define _SET_ASTAT(a, field, bit) (ASTATREG(field) = ASTAT_EXTRACT(a, bit))
#define SET_ASTAT(a) \
  do { \
    TRACE_REGISTER (cpu, "wrote ASTAT = %#x", a); \
    _SET_ASTAT(a, az,       AZ_BIT); \
    _SET_ASTAT(a, an,       AN_BIT); \
    _SET_ASTAT(a, ac0_copy, AC0_COPY_BIT); \
    _SET_ASTAT(a, v_copy,   V_COPY_BIT); \
    _SET_ASTAT(a, cc,       CC_BIT); \
    _SET_ASTAT(a, aq,       AQ_BIT); \
    _SET_ASTAT(a, rnd_mod,  RND_MOD_BIT); \
    _SET_ASTAT(a, ac0,      AC0_BIT); \
    _SET_ASTAT(a, ac1,      AC1_BIT); \
    _SET_ASTAT(a, av0,      AV0_BIT); \
    _SET_ASTAT(a, av0s,     AV0S_BIT); \
    _SET_ASTAT(a, av1,      AV1_BIT); \
    _SET_ASTAT(a, av1s,     AV1S_BIT); \
    _SET_ASTAT(a, v,        V_BIT); \
    _SET_ASTAT(a, vs,       VS_BIT); \
    ASTATREG(astat_reserved) = (a) & ~ASTAT_DEFINED_BITS; \
  } while (0)
#define SET_ASTATREG(field, val) \
  do { \
    int __v = !!(val); \
    TRACE_REGISTER (cpu, "wrote ASTAT["#field"] = %i", __v); \
    ASTATREG (field) = __v; \
    if (&ASTATREG (field) == &ASTATREG (ac0)) \
      { \
	TRACE_REGISTER (cpu, "wrote ASTAT["#field"_copy] = %i", __v); \
	ASTATREG (ac0_copy) = __v; \
      } \
    else if (&ASTATREG (field) == &ASTATREG (v)) \
      { \
	TRACE_REGISTER (cpu, "wrote ASTAT["#field"_copy] = %i", __v); \
	ASTATREG (v_copy) = __v; \
      } \
  } while (0)
#define SET_CCREG(val) SET_ASTATREG (cc, val)

#define SYSCFG_SSSTEP	(1 << 0)
#define SYSCFG_CCEN	(1 << 1)
#define SYSCFG_SNEN	(1 << 2)

#define __PUT_MEM(taddr, v, size) \
do { \
  bu##size __v = (v); \
  bu32 __taddr = (taddr); \
  int __cnt, __bytes = size / 8; \
  mmu_check_addr (cpu, __taddr, true, false, __bytes); \
  __cnt = sim_core_write_buffer (CPU_STATE(cpu), cpu, write_map, \
				 (void *)&__v, __taddr, __bytes); \
  if (__cnt != __bytes) \
    mmu_process_fault (cpu, __taddr, true, false, false, true); \
  BFIN_TRACE_CORE (cpu, __taddr, __bytes, write_map, __v); \
} while (0)
#define PUT_BYTE(taddr, v) __PUT_MEM(taddr, v, 8)
#define PUT_WORD(taddr, v) __PUT_MEM(taddr, v, 16)
#define PUT_LONG(taddr, v) __PUT_MEM(taddr, v, 32)

#define __GET_MEM(taddr, size, inst, map) \
({ \
  bu##size __ret; \
  bu32 __taddr = (taddr); \
  int __cnt, __bytes = size / 8; \
  mmu_check_addr (cpu, __taddr, false, inst, __bytes); \
  __cnt = sim_core_read_buffer (CPU_STATE(cpu), cpu, map, \
				(void *)&__ret, __taddr, __bytes); \
  if (__cnt != __bytes) \
    mmu_process_fault (cpu, __taddr, false, inst, false, true); \
  BFIN_TRACE_CORE (cpu, __taddr, __bytes, map, __ret); \
  __ret; \
})
#define _GET_MEM(taddr, size) __GET_MEM(taddr, size, false, read_map)
#define GET_BYTE(taddr) _GET_MEM(taddr, 8)
#define GET_WORD(taddr) _GET_MEM(taddr, 16)
#define GET_LONG(taddr) _GET_MEM(taddr, 32)

#define IFETCH(taddr) __GET_MEM(taddr, 16, true, exec_map)
#define IFETCH_CHECK(taddr) mmu_check_addr (cpu, taddr, false, true, 2)

extern void bfin_syscall (SIM_CPU *);
extern bu32 interp_insn_bfin (SIM_CPU *, bu32);
extern bu32 hwloop_get_next_pc (SIM_CPU *, bu32, bu32);

/* Defines for Blackfin memory layouts.  */
#define BFIN_ASYNC_BASE           0x20000000
#define BFIN_SYSTEM_MMR_BASE      0xFFC00000
#define BFIN_CORE_MMR_BASE        0xFFE00000
#define BFIN_L1_SRAM_SCRATCH      0xFFB00000
#define BFIN_L1_SRAM_SCRATCH_SIZE 0x1000
#define BFIN_L1_SRAM_SCRATCH_END  (BFIN_L1_SRAM_SCRATCH + BFIN_L1_SRAM_SCRATCH_SIZE)

#define BFIN_L1_CACHE_BYTES       32

#define BFIN_CPU_STATE (*(struct bfin_cpu_state *) CPU_ARCH_DATA (cpu))
#define STATE_BOARD_DATA(sd) ((struct bfin_board_data *) STATE_ARCH_DATA (sd))

#include "dv-bfin_trace.h"

#undef CLAMP
#define CLAMP(a, b, c) min (max (a, b), c)

/* TODO: Move all this trace logic to the common code.  */
#define BFIN_TRACE_CORE(cpu, addr, size, map, val) \
  do { \
    TRACE_CORE (cpu, "%cBUS %s %i bytes @ 0x%08x: 0x%0*x", \
		map == exec_map ? 'I' : 'D', \
		map == write_map ? "STORE" : "FETCH", \
		size, addr, size * 2, val); \
    PROFILE_COUNT_CORE (cpu, addr, size, map); \
  } while (0)
#define BFIN_TRACE_BRANCH(cpu, oldpc, newpc, hwloop, fmt, ...) \
  do { \
    TRACE_BRANCH (cpu, fmt " to %#x", ## __VA_ARGS__, newpc); \
    if (STATE_ENVIRONMENT (CPU_STATE (cpu)) == OPERATING_ENVIRONMENT) \
      bfin_trace_queue (cpu, oldpc, newpc, hwloop); \
  } while (0)

/* Default memory size.  */
#define BFIN_DEFAULT_MEM_SIZE (128 * 1024 * 1024)

#endif
