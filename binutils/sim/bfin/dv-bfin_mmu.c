/* Blackfin Memory Management Unit (MMU) model.

   Copyright (C) 2010-2024 Free Software Foundation, Inc.
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

/* This must come before any other includes.  */
#include "defs.h"

#include "sim-main.h"
#include "sim-options.h"
#include "devices.h"
#include "dv-bfin_mmu.h"
#include "dv-bfin_cec.h"

/* XXX: Should this really be two blocks of registers ?  PRM describes
        these as two Content Addressable Memory (CAM) blocks.  */

struct bfin_mmu
{
  bu32 base;

  /* Order after here is important -- matches hardware MMR layout.  */
  bu32 sram_base_address;

  bu32 dmem_control, dcplb_fault_status, dcplb_fault_addr;
  char _dpad0[0x100 - 0x0 - (4 * 4)];
  bu32 dcplb_addr[16];
  char _dpad1[0x200 - 0x100 - (4 * 16)];
  bu32 dcplb_data[16];
  char _dpad2[0x300 - 0x200 - (4 * 16)];
  bu32 dtest_command;
  char _dpad3[0x400 - 0x300 - (4 * 1)];
  bu32 dtest_data[2];

  char _dpad4[0x1000 - 0x400 - (4 * 2)];

  bu32 idk;	/* Filler MMR; hardware simply ignores.  */
  bu32 imem_control, icplb_fault_status, icplb_fault_addr;
  char _ipad0[0x100 - 0x0 - (4 * 4)];
  bu32 icplb_addr[16];
  char _ipad1[0x200 - 0x100 - (4 * 16)];
  bu32 icplb_data[16];
  char _ipad2[0x300 - 0x200 - (4 * 16)];
  bu32 itest_command;
  char _ipad3[0x400 - 0x300 - (4 * 1)];
  bu32 itest_data[2];
};
#define mmr_base()      offsetof(struct bfin_mmu, sram_base_address)
#define mmr_offset(mmr) (offsetof(struct bfin_mmu, mmr) - mmr_base())
#define mmr_idx(mmr)    (mmr_offset (mmr) / 4)

static const char * const mmr_names[BFIN_COREMMR_MMU_SIZE / 4] =
{
  "SRAM_BASE_ADDRESS", "DMEM_CONTROL", "DCPLB_FAULT_STATUS", "DCPLB_FAULT_ADDR",
  [mmr_idx (dcplb_addr[0])] = "DCPLB_ADDR0",
  "DCPLB_ADDR1", "DCPLB_ADDR2", "DCPLB_ADDR3", "DCPLB_ADDR4", "DCPLB_ADDR5",
  "DCPLB_ADDR6", "DCPLB_ADDR7", "DCPLB_ADDR8", "DCPLB_ADDR9", "DCPLB_ADDR10",
  "DCPLB_ADDR11", "DCPLB_ADDR12", "DCPLB_ADDR13", "DCPLB_ADDR14", "DCPLB_ADDR15",
  [mmr_idx (dcplb_data[0])] = "DCPLB_DATA0",
  "DCPLB_DATA1", "DCPLB_DATA2", "DCPLB_DATA3", "DCPLB_DATA4", "DCPLB_DATA5",
  "DCPLB_DATA6", "DCPLB_DATA7", "DCPLB_DATA8", "DCPLB_DATA9", "DCPLB_DATA10",
  "DCPLB_DATA11", "DCPLB_DATA12", "DCPLB_DATA13", "DCPLB_DATA14", "DCPLB_DATA15",
  [mmr_idx (dtest_command)] = "DTEST_COMMAND",
  [mmr_idx (dtest_data[0])] = "DTEST_DATA0", "DTEST_DATA1",
  [mmr_idx (imem_control)] = "IMEM_CONTROL", "ICPLB_FAULT_STATUS", "ICPLB_FAULT_ADDR",
  [mmr_idx (icplb_addr[0])] = "ICPLB_ADDR0",
  "ICPLB_ADDR1", "ICPLB_ADDR2", "ICPLB_ADDR3", "ICPLB_ADDR4", "ICPLB_ADDR5",
  "ICPLB_ADDR6", "ICPLB_ADDR7", "ICPLB_ADDR8", "ICPLB_ADDR9", "ICPLB_ADDR10",
  "ICPLB_ADDR11", "ICPLB_ADDR12", "ICPLB_ADDR13", "ICPLB_ADDR14", "ICPLB_ADDR15",
  [mmr_idx (icplb_data[0])] = "ICPLB_DATA0",
  "ICPLB_DATA1", "ICPLB_DATA2", "ICPLB_DATA3", "ICPLB_DATA4", "ICPLB_DATA5",
  "ICPLB_DATA6", "ICPLB_DATA7", "ICPLB_DATA8", "ICPLB_DATA9", "ICPLB_DATA10",
  "ICPLB_DATA11", "ICPLB_DATA12", "ICPLB_DATA13", "ICPLB_DATA14", "ICPLB_DATA15",
  [mmr_idx (itest_command)] = "ITEST_COMMAND",
  [mmr_idx (itest_data[0])] = "ITEST_DATA0", "ITEST_DATA1",
};
#define mmr_name(off) (mmr_names[(off) / 4] ? : "<INV>")

static bool bfin_mmu_skip_cplbs = false;

static unsigned
bfin_mmu_io_write_buffer (struct hw *me, const void *source,
			  int space, address_word addr, unsigned nr_bytes)
{
  struct bfin_mmu *mmu = hw_data (me);
  bu32 mmr_off;
  bu32 value;
  bu32 *valuep;

  /* Invalid access mode is higher priority than missing register.  */
  if (!dv_bfin_mmr_require_32 (me, addr, nr_bytes, true))
    return 0;

  value = dv_load_4 (source);

  mmr_off = addr - mmu->base;
  valuep = (void *)((uintptr_t)mmu + mmr_base() + mmr_off);

  HW_TRACE_WRITE ();

  switch (mmr_off)
    {
    case mmr_offset(dmem_control):
    case mmr_offset(imem_control):
      /* XXX: IMC/DMC bit should add/remove L1 cache regions ...  */
    case mmr_offset(dtest_data[0]) ... mmr_offset(dtest_data[1]):
    case mmr_offset(itest_data[0]) ... mmr_offset(itest_data[1]):
    case mmr_offset(dcplb_addr[0]) ... mmr_offset(dcplb_addr[15]):
    case mmr_offset(dcplb_data[0]) ... mmr_offset(dcplb_data[15]):
    case mmr_offset(icplb_addr[0]) ... mmr_offset(icplb_addr[15]):
    case mmr_offset(icplb_data[0]) ... mmr_offset(icplb_data[15]):
      *valuep = value;
      break;
    case mmr_offset(sram_base_address):
    case mmr_offset(dcplb_fault_status):
    case mmr_offset(dcplb_fault_addr):
    case mmr_offset(idk):
    case mmr_offset(icplb_fault_status):
    case mmr_offset(icplb_fault_addr):
      /* Discard writes to these.  */
      break;
    case mmr_offset(itest_command):
      /* XXX: Not supported atm.  */
      if (value)
	hw_abort (me, "ITEST_COMMAND unimplemented");
      break;
    case mmr_offset(dtest_command):
      /* Access L1 memory indirectly.  */
      *valuep = value;
      if (value)
	{
	  bu32 sram_addr = mmu->sram_base_address   |
	    ((value >> (26 - 11)) & (1 << 11)) | /* addr bit 11 (Way0/Way1)   */
	    ((value >> (24 - 21)) & (1 << 21)) | /* addr bit 21 (Data/Inst)   */
	    ((value >> (23 - 15)) & (1 << 15)) | /* addr bit 15 (Data Bank)   */
	    ((value >> (16 - 12)) & (3 << 12)) | /* addr bits 13:12 (Subbank) */
	    (value & 0x47F8);                    /* addr bits 14 & 10:3       */

	  if (!(value & TEST_DATA_ARRAY))
	    hw_abort (me, "DTEST_COMMAND tag array unimplemented");
	  if (value & 0xfa7cb801)
	    hw_abort (me, "DTEST_COMMAND bits undefined");

	  if (value & TEST_WRITE)
	    sim_write (hw_system (me), sram_addr, mmu->dtest_data, 8);
	  else
	    sim_read (hw_system (me), sram_addr, mmu->dtest_data, 8);
	}
      break;
    default:
      dv_bfin_mmr_invalid (me, addr, nr_bytes, true);
      return 0;
    }

  return nr_bytes;
}

static unsigned
bfin_mmu_io_read_buffer (struct hw *me, void *dest,
			 int space, address_word addr, unsigned nr_bytes)
{
  struct bfin_mmu *mmu = hw_data (me);
  bu32 mmr_off;
  bu32 *valuep;

  /* Invalid access mode is higher priority than missing register.  */
  if (!dv_bfin_mmr_require_32 (me, addr, nr_bytes, false))
    return 0;

  mmr_off = addr - mmu->base;
  valuep = (void *)((uintptr_t)mmu + mmr_base() + mmr_off);

  HW_TRACE_READ ();

  switch (mmr_off)
    {
    case mmr_offset(dmem_control):
    case mmr_offset(imem_control):
    case mmr_offset(dtest_command):
    case mmr_offset(dtest_data[0]) ... mmr_offset(dtest_data[2]):
    case mmr_offset(itest_command):
    case mmr_offset(itest_data[0]) ... mmr_offset(itest_data[2]):
      /* XXX: should do something here.  */
    case mmr_offset(dcplb_addr[0]) ... mmr_offset(dcplb_addr[15]):
    case mmr_offset(dcplb_data[0]) ... mmr_offset(dcplb_data[15]):
    case mmr_offset(icplb_addr[0]) ... mmr_offset(icplb_addr[15]):
    case mmr_offset(icplb_data[0]) ... mmr_offset(icplb_data[15]):
    case mmr_offset(sram_base_address):
    case mmr_offset(dcplb_fault_status):
    case mmr_offset(dcplb_fault_addr):
    case mmr_offset(idk):
    case mmr_offset(icplb_fault_status):
    case mmr_offset(icplb_fault_addr):
      dv_store_4 (dest, *valuep);
      break;
    default:
      dv_bfin_mmr_invalid (me, addr, nr_bytes, false);
      return 0;
    }

  return nr_bytes;
}

static void
attach_bfin_mmu_regs (struct hw *me, struct bfin_mmu *mmu)
{
  address_word attach_address;
  int attach_space;
  unsigned attach_size;
  reg_property_spec reg;

  if (hw_find_property (me, "reg") == NULL)
    hw_abort (me, "Missing \"reg\" property");

  if (!hw_find_reg_array_property (me, "reg", 0, &reg))
    hw_abort (me, "\"reg\" property must contain three addr/size entries");

  hw_unit_address_to_attach_address (hw_parent (me),
				     &reg.address,
				     &attach_space, &attach_address, me);
  hw_unit_size_to_attach_size (hw_parent (me), &reg.size, &attach_size, me);

  if (attach_size != BFIN_COREMMR_MMU_SIZE)
    hw_abort (me, "\"reg\" size must be %#x", BFIN_COREMMR_MMU_SIZE);

  hw_attach_address (hw_parent (me),
		     0, attach_space, attach_address, attach_size, me);

  mmu->base = attach_address;
}

static void
bfin_mmu_finish (struct hw *me)
{
  struct bfin_mmu *mmu;

  mmu = HW_ZALLOC (me, struct bfin_mmu);

  set_hw_data (me, mmu);
  set_hw_io_read_buffer (me, bfin_mmu_io_read_buffer);
  set_hw_io_write_buffer (me, bfin_mmu_io_write_buffer);

  attach_bfin_mmu_regs (me, mmu);

  /* Initialize the MMU.  */
  mmu->sram_base_address = 0xff800000 - 0;
			   /*(4 * 1024 * 1024 * CPU_INDEX (hw_system_cpu (me)));*/
  mmu->dmem_control = 0x00000001;
  mmu->imem_control = 0x00000001;
}

const struct hw_descriptor dv_bfin_mmu_descriptor[] =
{
  {"bfin_mmu", bfin_mmu_finish,},
  {NULL, NULL},
};

/* Device option parsing.  */

static DECLARE_OPTION_HANDLER (bfin_mmu_option_handler);

enum {
  OPTION_MMU_SKIP_TABLES = OPTION_START,
};

static const OPTION bfin_mmu_options[] =
{
  { {"mmu-skip-cplbs", no_argument, NULL, OPTION_MMU_SKIP_TABLES },
      '\0', NULL, "Skip parsing of CPLB tables (big speed increase)",
      bfin_mmu_option_handler, NULL },

  { {NULL, no_argument, NULL, 0}, '\0', NULL, NULL, NULL, NULL }
};

static SIM_RC
bfin_mmu_option_handler (SIM_DESC sd, sim_cpu *current_cpu, int opt,
			 char *arg, int is_command)
{
  switch (opt)
    {
    case OPTION_MMU_SKIP_TABLES:
      bfin_mmu_skip_cplbs = true;
      return SIM_RC_OK;

    default:
      sim_io_eprintf (sd, "Unknown Blackfin MMU option %d\n", opt);
      return SIM_RC_FAIL;
    }
}

/* Provide a prototype to silence -Wmissing-prototypes.  */
extern MODULE_INIT_FN sim_install_bfin_mmu;

SIM_RC
sim_install_bfin_mmu (SIM_DESC sd)
{
  SIM_ASSERT (STATE_MAGIC (sd) == SIM_MAGIC_NUMBER);
  return sim_add_option_table (sd, NULL, bfin_mmu_options);
}

#define MMU_STATE(cpu) DV_STATE_CACHED (cpu, mmu)

static void
_mmu_log_ifault (SIM_CPU *cpu, struct bfin_mmu *mmu, bu32 pc, bool supv)
{
  mmu->icplb_fault_addr = pc;
  mmu->icplb_fault_status = supv << 17;
}

void
mmu_log_ifault (SIM_CPU *cpu)
{
  _mmu_log_ifault (cpu, MMU_STATE (cpu), PCREG, cec_get_ivg (cpu) >= 0);
}

static void
_mmu_log_fault (SIM_CPU *cpu, struct bfin_mmu *mmu, bu32 addr, bool write,
		bool inst, bool miss, bool supv, bool dag1, bu32 faults)
{
  bu32 *fault_status, *fault_addr;

  /* No logging in non-OS mode.  */
  if (!mmu)
    return;

  fault_status = inst ? &mmu->icplb_fault_status : &mmu->dcplb_fault_status;
  fault_addr = inst ? &mmu->icplb_fault_addr : &mmu->dcplb_fault_addr;
  /* ICPLB regs always get updated.  */
  if (!inst)
    _mmu_log_ifault (cpu, mmu, PCREG, supv);

  *fault_addr = addr;
  *fault_status =
	(miss << 19) |
	(dag1 << 18) |
	(supv << 17) |
	(write << 16) |
	faults;
}

static void
_mmu_process_fault (SIM_CPU *cpu, struct bfin_mmu *mmu, bu32 addr, bool write,
		    bool inst, bool unaligned, bool miss, bool supv, bool dag1)
{
  int excp;

  /* See order in mmu_check_addr() */
  if (unaligned)
    excp = inst ? VEC_MISALI_I : VEC_MISALI_D;
  else if (addr >= BFIN_SYSTEM_MMR_BASE)
    excp = VEC_ILL_RES;
  else if (!mmu)
    excp = inst ? VEC_CPLB_I_M : VEC_CPLB_M;
  else
    {
      /* Misses are hardware errors.  */
      cec_hwerr (cpu, HWERR_EXTERN_ADDR);
      return;
    }

  _mmu_log_fault (cpu, mmu, addr, write, inst, miss, supv, dag1, 0);
  cec_exception (cpu, excp);
}

void
mmu_process_fault (SIM_CPU *cpu, bu32 addr, bool write, bool inst,
		   bool unaligned, bool miss)
{
  SIM_DESC sd = CPU_STATE (cpu);
  struct bfin_mmu *mmu;

  if (STATE_ENVIRONMENT (sd) != OPERATING_ENVIRONMENT)
    mmu = NULL;
  else
    mmu = MMU_STATE (cpu);

  _mmu_process_fault (cpu, mmu, addr, write, inst, unaligned, miss,
		      cec_is_supervisor_mode (cpu),
		      BFIN_CPU_STATE.multi_pc == PCREG + 6);
}

/* Return values:
    -2: no known problems
    -1: valid
     0: miss
     1: protection violation
     2: multiple hits
     3: unaligned
     4: miss; hwerr  */
static int
mmu_check_implicit_addr (SIM_CPU *cpu, bu32 addr, bool inst, int size,
			 bool supv, bool dag1)
{
  bool l1 = ((addr & 0xFF000000) == 0xFF000000);
  bu32 amask = (addr & 0xFFF00000);

  if (addr & (size - 1))
    return 3;

  /* MMRs may never be executable or accessed from usermode.  */
  if (addr >= BFIN_SYSTEM_MMR_BASE)
    {
      if (inst)
	return 0;
      else if (!supv || dag1)
	return 1;
      else
	return -1;
    }
  else if (inst)
    {
      /* Some regions are not executable.  */
      /* XXX: Should this be in the model data ?  Core B 561 ?  */
      if (l1)
	return (amask == 0xFFA00000) ? -1 : 1;
    }
  else
    {
      /* Some regions are not readable.  */
      /* XXX: Should this be in the model data ?  Core B 561 ?  */
      if (l1)
	return (amask != 0xFFA00000) ? -1 : 4;
    }

  return -2;
}

/* Exception order per the PRM (first has highest):
     Inst Multiple CPLB Hits
     Inst Misaligned Access
     Inst Protection Violation
     Inst CPLB Miss
  Only the alignment matters in non-OS mode though.  */
static int
_mmu_check_addr (SIM_CPU *cpu, bu32 addr, bool write, bool inst, int size)
{
  SIM_DESC sd = CPU_STATE (cpu);
  struct bfin_mmu *mmu;
  bu32 *mem_control, *cplb_addr, *cplb_data;
  bu32 faults;
  bool supv, do_excp, dag1;
  int i, hits;

  supv = cec_is_supervisor_mode (cpu);
  dag1 = (BFIN_CPU_STATE.multi_pc == PCREG + 6);

  if (STATE_ENVIRONMENT (sd) != OPERATING_ENVIRONMENT || bfin_mmu_skip_cplbs)
    {
      int ret = mmu_check_implicit_addr (cpu, addr, inst, size, supv, dag1);
      /* Valid hits and misses are OK in non-OS envs.  */
      if (ret < 0)
	return 0;
      _mmu_process_fault (cpu, NULL, addr, write, inst, (ret == 3), false, supv, dag1);
    }

  mmu = MMU_STATE (cpu);
  mem_control = inst ? &mmu->imem_control : &mmu->dmem_control;
  cplb_addr = inst ? &mmu->icplb_addr[0] : &mmu->dcplb_addr[0];
  cplb_data = inst ? &mmu->icplb_data[0] : &mmu->dcplb_data[0];

  faults = 0;
  hits = 0;
  do_excp = false;

  /* CPLBs disabled -> little to do.  */
  if (!(*mem_control & ENCPLB))
    {
      hits = 1;
      goto implicit_check;
    }

  /* Check all the CPLBs first.  */
  for (i = 0; i < 16; ++i)
    {
      const bu32 pages[4] = { 0x400, 0x1000, 0x100000, 0x400000 };
      bu32 addr_lo, addr_hi;

      /* Skip invalid entries.  */
      if (!(cplb_data[i] & CPLB_VALID))
	continue;

      /* See if this entry covers this address.  */
      addr_lo = cplb_addr[i];
      addr_hi = cplb_addr[i] + pages[(cplb_data[i] & PAGE_SIZE) >> 16];
      if (addr < addr_lo || addr >= addr_hi)
	continue;

      ++hits;
      faults |= (1 << i);
      if (write)
	{
	  if (!supv && !(cplb_data[i] & CPLB_USER_WR))
	    do_excp = true;
	  if (supv && !(cplb_data[i] & CPLB_SUPV_WR))
	    do_excp = true;
	  if ((cplb_data[i] & (CPLB_WT | CPLB_L1_CHBL | CPLB_DIRTY)) == CPLB_L1_CHBL)
	    do_excp = true;
	}
      else
	{
	  if (!supv && !(cplb_data[i] & CPLB_USER_RD))
	    do_excp = true;
	}
    }

  /* Handle default/implicit CPLBs.  */
  if (!do_excp && hits < 2)
    {
      int ihits;
 implicit_check:
      ihits = mmu_check_implicit_addr (cpu, addr, inst, size, supv, dag1);
      switch (ihits)
	{
	/* No faults and one match -> good to go.  */
	case -1: return 0;
	case -2:
	  if (hits == 1)
	    return 0;
	  break;
	case 4:
	  cec_hwerr (cpu, HWERR_EXTERN_ADDR);
	  return 0;
	default:
	  hits = ihits;
	}
    }
  else
    /* Normalize hit count so hits==2 is always multiple hit exception.  */
    hits = min (2, hits);

  _mmu_log_fault (cpu, mmu, addr, write, inst, hits == 0, supv, dag1, faults);

  if (inst)
    {
      int iexcps[] = { VEC_CPLB_I_M, VEC_CPLB_I_VL, VEC_CPLB_I_MHIT, VEC_MISALI_I };
      return iexcps[hits];
    }
  else
    {
      int dexcps[] = { VEC_CPLB_M, VEC_CPLB_VL, VEC_CPLB_MHIT, VEC_MISALI_D };
      return dexcps[hits];
    }
}

void
mmu_check_addr (SIM_CPU *cpu, bu32 addr, bool write, bool inst, int size)
{
  int excp = _mmu_check_addr (cpu, addr, write, inst, size);
  if (excp)
    cec_exception (cpu, excp);
}

void
mmu_check_cache_addr (SIM_CPU *cpu, bu32 addr, bool write, bool inst)
{
  bu32 cacheaddr;
  int excp;

  cacheaddr = addr & ~(BFIN_L1_CACHE_BYTES - 1);
  excp = _mmu_check_addr (cpu, cacheaddr, write, inst, BFIN_L1_CACHE_BYTES);
  if (excp == 0)
    return;

  /* Most exceptions are ignored with cache funcs.  */
  /* XXX: Not sure if we should be ignoring CPLB misses.  */
  if (inst)
    {
      if (excp == VEC_CPLB_I_VL)
	return;
    }
  else
    {
      if (excp == VEC_CPLB_VL)
	return;
    }
  cec_exception (cpu, excp);
}
