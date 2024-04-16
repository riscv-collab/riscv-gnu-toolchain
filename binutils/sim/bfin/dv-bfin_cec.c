/* Blackfin Core Event Controller (CEC) model.

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

#include <strings.h>

#include "sim-main.h"
#include "sim-signal.h"
#include "devices.h"
#include "dv-bfin_cec.h"
#include "dv-bfin_evt.h"
#include "dv-bfin_mmu.h"

struct bfin_cec
{
  bu32 base;
  SIM_CPU *cpu;
  struct hw *me;
  struct hw_event *pending;

  /* Order after here is important -- matches hardware MMR layout.  */
  bu32 evt_override, imask, ipend, ilat, iprio;
};
#define mmr_base()      offsetof(struct bfin_cec, evt_override)
#define mmr_offset(mmr) (offsetof(struct bfin_cec, mmr) - mmr_base())

static const char * const mmr_names[] =
{
  "EVT_OVERRIDE", "IMASK", "IPEND", "ILAT", "IPRIO",
};
#define mmr_name(off) mmr_names[(off) / 4]

static void _cec_raise (SIM_CPU *, struct bfin_cec *, int);

static void
bfin_cec_hw_event_callback (struct hw *me, void *data)
{
  struct bfin_cec *cec = data;
  hw_event_queue_deschedule (me, cec->pending);
  _cec_raise (cec->cpu, cec, -1);
  cec->pending = NULL;
}
static void
bfin_cec_check_pending (struct hw *me, struct bfin_cec *cec)
{
  if (cec->pending)
    return;
  cec->pending = hw_event_queue_schedule (me, 0, bfin_cec_hw_event_callback, cec);
}
static void
_cec_check_pending (SIM_CPU *cpu, struct bfin_cec *cec)
{
  bfin_cec_check_pending (cec->me, cec);
}

static void
_cec_imask_write (struct bfin_cec *cec, bu32 value)
{
  cec->imask = (value & IVG_MASKABLE_B) | (cec->imask & IVG_UNMASKABLE_B);
}

static unsigned
bfin_cec_io_write_buffer (struct hw *me, const void *source,
			  int space, address_word addr, unsigned nr_bytes)
{
  struct bfin_cec *cec = hw_data (me);
  bu32 mmr_off;
  bu32 value;

  /* Invalid access mode is higher priority than missing register.  */
  if (!dv_bfin_mmr_require_32 (me, addr, nr_bytes, true))
    return 0;

  value = dv_load_4 (source);
  mmr_off = addr - cec->base;

  HW_TRACE_WRITE ();

  switch (mmr_off)
    {
    case mmr_offset(evt_override):
      cec->evt_override = value;
      break;
    case mmr_offset(imask):
      _cec_imask_write (cec, value);
      bfin_cec_check_pending (me, cec);
      break;
    case mmr_offset(ipend):
      /* Read-only register.  */
      break;
    case mmr_offset(ilat):
      dv_w1c_4 (&cec->ilat, value, 0xffee);
      break;
    case mmr_offset(iprio):
      cec->iprio = (value & IVG_UNMASKABLE_B);
      break;
    }

  return nr_bytes;
}

static unsigned
bfin_cec_io_read_buffer (struct hw *me, void *dest,
			 int space, address_word addr, unsigned nr_bytes)
{
  struct bfin_cec *cec = hw_data (me);
  bu32 mmr_off;
  bu32 *valuep;

  /* Invalid access mode is higher priority than missing register.  */
  if (!dv_bfin_mmr_require_32 (me, addr, nr_bytes, false))
    return 0;

  mmr_off = addr - cec->base;
  valuep = (void *)((uintptr_t)cec + mmr_base() + mmr_off);

  HW_TRACE_READ ();

  dv_store_4 (dest, *valuep);

  return nr_bytes;
}

static const struct hw_port_descriptor bfin_cec_ports[] =
{
  { "emu",   IVG_EMU,   0, input_port, },
  { "rst",   IVG_RST,   0, input_port, },
  { "nmi",   IVG_NMI,   0, input_port, },
  { "evx",   IVG_EVX,   0, input_port, },
  { "ivhw",  IVG_IVHW,  0, input_port, },
  { "ivtmr", IVG_IVTMR, 0, input_port, },
  { "ivg7",  IVG7,      0, input_port, },
  { "ivg8",  IVG8,      0, input_port, },
  { "ivg9",  IVG9,      0, input_port, },
  { "ivg10", IVG10,     0, input_port, },
  { "ivg11", IVG11,     0, input_port, },
  { "ivg12", IVG12,     0, input_port, },
  { "ivg13", IVG13,     0, input_port, },
  { "ivg14", IVG14,     0, input_port, },
  { "ivg15", IVG15,     0, input_port, },
  { NULL, 0, 0, 0, },
};

static void
bfin_cec_port_event (struct hw *me, int my_port, struct hw *source,
		     int source_port, int level)
{
  struct bfin_cec *cec = hw_data (me);
  _cec_raise (cec->cpu, cec, my_port);
}

static void
attach_bfin_cec_regs (struct hw *me, struct bfin_cec *cec)
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

  if (attach_size != BFIN_COREMMR_CEC_SIZE)
    hw_abort (me, "\"reg\" size must be %#x", BFIN_COREMMR_CEC_SIZE);

  hw_attach_address (hw_parent (me),
		     0, attach_space, attach_address, attach_size, me);

  cec->base = attach_address;
  /* XXX: should take from the device tree.  */
  cec->cpu = STATE_CPU (hw_system (me), 0);
  cec->me = me;
}

static void
bfin_cec_finish (struct hw *me)
{
  struct bfin_cec *cec;

  cec = HW_ZALLOC (me, struct bfin_cec);

  set_hw_data (me, cec);
  set_hw_io_read_buffer (me, bfin_cec_io_read_buffer);
  set_hw_io_write_buffer (me, bfin_cec_io_write_buffer);
  set_hw_ports (me, bfin_cec_ports);
  set_hw_port_event (me, bfin_cec_port_event);

  attach_bfin_cec_regs (me, cec);

  /* Initialize the CEC.  */
  cec->imask = IVG_UNMASKABLE_B;
  cec->ipend = IVG_RST_B | IVG_IRPTEN_B;
}

const struct hw_descriptor dv_bfin_cec_descriptor[] =
{
  {"bfin_cec", bfin_cec_finish,},
  {NULL, NULL},
};

static const char * const excp_decoded[] =
{
  [VEC_SYS        ] = "Custom exception 0 (system call)",
  [VEC_EXCPT01    ] = "Custom exception 1 (software breakpoint)",
  [VEC_EXCPT02    ] = "Custom exception 2 (KGDB hook)",
  [VEC_EXCPT03    ] = "Custom exception 3 (userspace stack overflow)",
  [VEC_EXCPT04    ] = "Custom exception 4 (dump trace buffer)",
  [VEC_EXCPT05    ] = "Custom exception 5",
  [VEC_EXCPT06    ] = "Custom exception 6",
  [VEC_EXCPT07    ] = "Custom exception 7",
  [VEC_EXCPT08    ] = "Custom exception 8",
  [VEC_EXCPT09    ] = "Custom exception 9",
  [VEC_EXCPT10    ] = "Custom exception 10",
  [VEC_EXCPT11    ] = "Custom exception 11",
  [VEC_EXCPT12    ] = "Custom exception 12",
  [VEC_EXCPT13    ] = "Custom exception 13",
  [VEC_EXCPT14    ] = "Custom exception 14",
  [VEC_EXCPT15    ] = "Custom exception 15",
  [VEC_STEP       ] = "Hardware single step",
  [VEC_OVFLOW     ] = "Trace buffer overflow",
  [VEC_UNDEF_I    ] = "Undefined instruction",
  [VEC_ILGAL_I    ] = "Illegal instruction combo (multi-issue)",
  [VEC_CPLB_VL    ] = "DCPLB protection violation",
  [VEC_MISALI_D   ] = "Unaligned data access",
  [VEC_UNCOV      ] = "Unrecoverable event (double fault)",
  [VEC_CPLB_M     ] = "DCPLB miss",
  [VEC_CPLB_MHIT  ] = "Multiple DCPLB hit",
  [VEC_WATCH      ] = "Watchpoint match",
  [VEC_ISTRU_VL   ] = "ADSP-BF535 only",
  [VEC_MISALI_I   ] = "Unaligned instruction access",
  [VEC_CPLB_I_VL  ] = "ICPLB protection violation",
  [VEC_CPLB_I_M   ] = "ICPLB miss",
  [VEC_CPLB_I_MHIT] = "Multiple ICPLB hit",
  [VEC_ILL_RES    ] = "Illegal supervisor resource",
};

#define CEC_STATE(cpu) DV_STATE_CACHED (cpu, cec)

#define __cec_get_ivg(val) (ffs ((val) & ~IVG_IRPTEN_B) - 1)
#define _cec_get_ivg(cec) __cec_get_ivg ((cec)->ipend & ~IVG_EMU_B)

int
cec_get_ivg (SIM_CPU *cpu)
{
  switch (STATE_ENVIRONMENT (CPU_STATE (cpu)))
    {
    case OPERATING_ENVIRONMENT:
      return _cec_get_ivg (CEC_STATE (cpu));
    default:
      return IVG_USER;
    }
}

static bool
_cec_is_supervisor_mode (struct bfin_cec *cec)
{
  return (cec->ipend & ~(IVG_EMU_B | IVG_IRPTEN_B));
}
bool
cec_is_supervisor_mode (SIM_CPU *cpu)
{
  switch (STATE_ENVIRONMENT (CPU_STATE (cpu)))
    {
    case OPERATING_ENVIRONMENT:
      return _cec_is_supervisor_mode (CEC_STATE (cpu));
    case USER_ENVIRONMENT:
      return false;
    default:
      return true;
    }
}
static bool
_cec_is_user_mode (struct bfin_cec *cec)
{
  return !_cec_is_supervisor_mode (cec);
}
bool
cec_is_user_mode (SIM_CPU *cpu)
{
  return !cec_is_supervisor_mode (cpu);
}
static void
_cec_require_supervisor (SIM_CPU *cpu, struct bfin_cec *cec)
{
  if (_cec_is_user_mode (cec))
    cec_exception (cpu, VEC_ILL_RES);
}
void
cec_require_supervisor (SIM_CPU *cpu)
{
  /* Do not call _cec_require_supervisor() to avoid CEC_STATE()
     as that macro requires OS operating mode.  */
  if (cec_is_user_mode (cpu))
    cec_exception (cpu, VEC_ILL_RES);
}

#define excp_to_sim_halt(reason, sigrc) \
  sim_engine_halt (CPU_STATE (cpu), cpu, NULL, PCREG, reason, sigrc)
void
cec_exception (SIM_CPU *cpu, int excp)
{
  SIM_DESC sd = CPU_STATE (cpu);
  int sigrc = -1;

  TRACE_EVENTS (cpu, "processing exception %#x in EVT%i", excp,
		cec_get_ivg (cpu));

  /* Ideally what would happen here for real hardware exceptions (not
     fake sim ones) is that:
      - For service exceptions (excp <= 0x11):
         RETX is the _next_ PC which can be tricky with jumps/hardware loops/...
      - For error exceptions (excp > 0x11):
         RETX is the _current_ PC (i.e. the one causing the exception)
      - PC is loaded with EVT3 MMR
      - ILAT/IPEND in CEC is updated depending on current IVG level
      - the fault address MMRs get updated with data/instruction info
      - Execution continues on in the EVT3 handler  */

  /* Handle simulator exceptions first.  */
  switch (excp)
    {
    case VEC_SIM_HLT:
      excp_to_sim_halt (sim_exited, 0);
      return;
    case VEC_SIM_ABORT:
      excp_to_sim_halt (sim_exited, 1);
      return;
    case VEC_SIM_TRAP:
      /* GDB expects us to step over EMUEXCPT.  */
      /* XXX: What about hwloops and EMUEXCPT at the end?
              Pretty sure gdb doesn't handle this already...  */
      SET_PCREG (PCREG + 2);
      /* Only trap when we are running in gdb.  */
      if (STATE_OPEN_KIND (sd) == SIM_OPEN_DEBUG)
	excp_to_sim_halt (sim_stopped, SIM_SIGTRAP);
      return;
    case VEC_SIM_DBGA:
      /* If running in gdb, simply trap.  */
      if (STATE_OPEN_KIND (sd) == SIM_OPEN_DEBUG)
	excp_to_sim_halt (sim_stopped, SIM_SIGTRAP);
      else
	excp_to_sim_halt (sim_exited, 2);
    }

  if (excp <= 0x3f)
    {
      SET_EXCAUSE (excp);
      if (STATE_ENVIRONMENT (sd) == OPERATING_ENVIRONMENT)
	{
	  /* ICPLB regs always get updated.  */
	  /* XXX: Should optimize this call path ...  */
	  if (excp != VEC_MISALI_I && excp != VEC_MISALI_D
	      && excp != VEC_CPLB_I_M && excp != VEC_CPLB_M
	      && excp != VEC_CPLB_I_VL && excp != VEC_CPLB_VL
	      && excp != VEC_CPLB_I_MHIT && excp != VEC_CPLB_MHIT)
	    mmu_log_ifault (cpu);
	  _cec_raise (cpu, CEC_STATE (cpu), IVG_EVX);
	  /* We need to restart the engine so that we don't return
	     and continue processing this bad insn.  */
	  if (EXCAUSE >= 0x20)
	    sim_engine_restart (sd, cpu, NULL, PCREG);
	  return;
	}
    }

  TRACE_EVENTS (cpu, "running virtual exception handler");

  switch (excp)
    {
    case VEC_SYS:
      bfin_syscall (cpu);
      break;

    case VEC_EXCPT01:	/* Userspace gdb breakpoint.  */
      sigrc = SIM_SIGTRAP;
      break;

    case VEC_UNDEF_I:	/* Undefined instruction.  */
      sigrc = SIM_SIGILL;
      break;

    case VEC_ILL_RES:	/* Illegal supervisor resource.  */
    case VEC_MISALI_I:	/* Misaligned instruction.  */
      sigrc = SIM_SIGBUS;
      break;

    case VEC_CPLB_M:
    case VEC_CPLB_I_M:
      sigrc = SIM_SIGSEGV;
      break;

    default:
      sim_io_eprintf (sd, "Unhandled exception %#x at 0x%08x (%s)\n",
		      excp, PCREG, excp_decoded[excp]);
      sigrc = SIM_SIGILL;
      break;
    }

  if (sigrc != -1)
    excp_to_sim_halt (sim_stopped, sigrc);
}

bu32 cec_cli (SIM_CPU *cpu)
{
  struct bfin_cec *cec;
  bu32 old_mask;

  if (STATE_ENVIRONMENT (CPU_STATE (cpu)) != OPERATING_ENVIRONMENT)
    return 0;

  cec = CEC_STATE (cpu);
  _cec_require_supervisor (cpu, cec);

  /* XXX: what about IPEND[4] ?  */
  old_mask = cec->imask;
  _cec_imask_write (cec, 0);

  TRACE_EVENTS (cpu, "CLI changed IMASK from %#x to %#x", old_mask, cec->imask);

  return old_mask;
}

void cec_sti (SIM_CPU *cpu, bu32 ints)
{
  struct bfin_cec *cec;
  bu32 old_mask;

  if (STATE_ENVIRONMENT (CPU_STATE (cpu)) != OPERATING_ENVIRONMENT)
    return;

  cec = CEC_STATE (cpu);
  _cec_require_supervisor (cpu, cec);

  /* XXX: what about IPEND[4] ?  */
  old_mask = cec->imask;
  _cec_imask_write (cec, ints);

  TRACE_EVENTS (cpu, "STI changed IMASK from %#x to %#x", old_mask, cec->imask);

  /* Check for pending interrupts that are now enabled.  */
  _cec_check_pending (cpu, cec);
}

static void
cec_irpten_enable (SIM_CPU *cpu, struct bfin_cec *cec)
{
  /* Globally mask interrupts.  */
  TRACE_EVENTS (cpu, "setting IPEND[4] to globally mask interrupts");
  cec->ipend |= IVG_IRPTEN_B;
}

static void
cec_irpten_disable (SIM_CPU *cpu, struct bfin_cec *cec)
{
  /* Clear global interrupt mask.  */
  TRACE_EVENTS (cpu, "clearing IPEND[4] to not globally mask interrupts");
  cec->ipend &= ~IVG_IRPTEN_B;
}

static void
_cec_raise (SIM_CPU *cpu, struct bfin_cec *cec, int ivg)
{
  SIM_DESC sd = CPU_STATE (cpu);
  int curr_ivg = _cec_get_ivg (cec);
  bool snen;
  bool irpten;

  TRACE_EVENTS (cpu, "processing request for EVT%i while at EVT%i",
		ivg, curr_ivg);

  irpten = (cec->ipend & IVG_IRPTEN_B);
  snen = (SYSCFGREG & SYSCFG_SNEN);

  if (curr_ivg == -1)
    curr_ivg = IVG_USER;

  /* Just check for higher latched interrupts.  */
  if (ivg == -1)
    {
      if (irpten)
	goto done; /* All interrupts are masked anyways.  */

      ivg = __cec_get_ivg (cec->ilat & cec->imask);
      if (ivg < 0)
	goto done; /* Nothing latched.  */

      if (ivg > curr_ivg)
	goto done; /* Nothing higher latched.  */

      if (!snen && ivg == curr_ivg)
	goto done; /* Self nesting disabled.  */

      /* Still here, so fall through to raise to higher pending.  */
    }

  cec->ilat |= (1 << ivg);

  if (ivg <= IVG_EVX)
    {
      /* These two are always processed.  */
      if (ivg == IVG_EMU || ivg == IVG_RST)
	goto process_int;

      /* Anything lower might trigger a double fault.  */
      if (curr_ivg <= ivg)
	{
	  /* Double fault ! :(  */
	  SET_EXCAUSE (VEC_UNCOV);
	  /* XXX: SET_RETXREG (...);  */
	  sim_io_error (sd, "%s: double fault at 0x%08x ! :(", __func__, PCREG);
	  excp_to_sim_halt (sim_stopped, SIM_SIGABRT);
	}

      /* No double fault -> always process.  */
      goto process_int;
    }
  else if (irpten && curr_ivg != IVG_USER)
    {
      /* Interrupts are globally masked.  */
    }
  else if (!(cec->imask & (1 << ivg)))
    {
      /* This interrupt is masked.  */
    }
  else if (ivg < curr_ivg || (snen && ivg == curr_ivg))
    {
      /* Do transition!  */
      bu32 oldpc;

 process_int:
      cec->ipend |= (1 << ivg);
      cec->ilat &= ~(1 << ivg);

      /* Interrupts are processed in between insns which means the return
         point is the insn-to-be-executed (which is the current PC).  But
         exceptions are handled while executing an insn, so we may have to
         advance the PC ourselves when setting RETX.
         XXX: Advancing the PC should only be for "service" exceptions, and
              handling them after executing the insn should be OK, which
              means we might be able to use the event interface for it.  */

      oldpc = PCREG;
      switch (ivg)
	{
	case IVG_EMU:
	  /* Signal the JTAG ICE.  */
	  /* XXX: what happens with 'raise 0' ?  */
	  SET_RETEREG (oldpc);
	  excp_to_sim_halt (sim_stopped, SIM_SIGTRAP);
	  /* XXX: Need an easy way for gdb to signal it isnt here.  */
	  cec->ipend &= ~IVG_EMU_B;
	  break;
	case IVG_RST:
	  /* Have the core reset simply exit (i.e. "shutdown").  */
	  excp_to_sim_halt (sim_exited, 0);
	  break;
	case IVG_NMI:
	  /* XXX: Should check this.  */
	  SET_RETNREG (oldpc);
	  break;
	case IVG_EVX:
	  /* Non-service exceptions point to the excepting instruction.  */
	  if (EXCAUSE >= 0x20)
	    SET_RETXREG (oldpc);
	  else
	    {
	      bu32 nextpc = hwloop_get_next_pc (cpu, oldpc, INSN_LEN);
	      SET_RETXREG (nextpc);
	    }

	  break;
	case IVG_IRPTEN:
	  /* XXX: what happens with 'raise 4' ?  */
	  sim_io_error (sd, "%s: what to do with 'raise 4' ?", __func__);
	  break;
	default:
	  SET_RETIREG (oldpc | (ivg == curr_ivg ? 1 : 0));
	  break;
	}

      /* If EVT_OVERRIDE is in effect (IVG7+), use the reset address.  */
      if ((cec->evt_override & 0xff80) & (1 << ivg))
	SET_PCREG (cec_get_reset_evt (cpu));
      else
	SET_PCREG (cec_get_evt (cpu, ivg));

      BFIN_TRACE_BRANCH (cpu, oldpc, PCREG, -1, "CEC changed PC (to EVT%i):", ivg);
      BFIN_CPU_STATE.did_jump = true;

      /* Enable the global interrupt mask upon interrupt entry.  */
      if (ivg >= IVG_IVHW)
	cec_irpten_enable (cpu, cec);
    }

  /* When moving between states, don't let internal states bleed through.  */
  DIS_ALGN_EXPT &= ~1;

  /* When going from user to super, we set LSB in LB regs to avoid
     misbehavior and/or malicious code.
     Also need to load SP alias with KSP.  */
  if (curr_ivg == IVG_USER)
    {
      int i;
      for (i = 0; i < 2; ++i)
	if (!(LBREG (i) & 1))
	  SET_LBREG (i, LBREG (i) | 1);
      SET_USPREG (SPREG);
      SET_SPREG (KSPREG);
    }

 done:
  TRACE_EVENTS (cpu, "now at EVT%i", _cec_get_ivg (cec));
}

static bu32
cec_read_ret_reg (SIM_CPU *cpu, int ivg)
{
  switch (ivg)
    {
    case IVG_EMU: return RETEREG;
    case IVG_NMI: return RETNREG;
    case IVG_EVX: return RETXREG;
    default:      return RETIREG;
    }
}

void
cec_latch (SIM_CPU *cpu, int ivg)
{
  struct bfin_cec *cec;

  if (STATE_ENVIRONMENT (CPU_STATE (cpu)) != OPERATING_ENVIRONMENT)
    {
      bu32 oldpc = PCREG;
      SET_PCREG (cec_read_ret_reg (cpu, ivg));
      BFIN_TRACE_BRANCH (cpu, oldpc, PCREG, -1, "CEC changed PC");
      return;
    }

  cec = CEC_STATE (cpu);
  cec->ilat |= (1 << ivg);
  _cec_check_pending (cpu, cec);
}

void
cec_hwerr (SIM_CPU *cpu, int hwerr)
{
  SET_HWERRCAUSE (hwerr);
  cec_latch (cpu, IVG_IVHW);
}

void
cec_return (SIM_CPU *cpu, int ivg)
{
  SIM_DESC sd = CPU_STATE (cpu);
  struct bfin_cec *cec;
  bool snen;
  int curr_ivg;
  bu32 oldpc, newpc;

  oldpc = PCREG;

  BFIN_CPU_STATE.did_jump = true;
  if (STATE_ENVIRONMENT (sd) != OPERATING_ENVIRONMENT)
    {
      SET_PCREG (cec_read_ret_reg (cpu, ivg));
      BFIN_TRACE_BRANCH (cpu, oldpc, PCREG, -1, "CEC changed PC");
      return;
    }

  cec = CEC_STATE (cpu);

  /* XXX: This isn't entirely correct ...  */
  cec->ipend &= ~IVG_EMU_B;

  curr_ivg = _cec_get_ivg (cec);
  if (curr_ivg == -1)
    curr_ivg = IVG_USER;
  if (ivg == -1)
    ivg = curr_ivg;

  TRACE_EVENTS (cpu, "returning from EVT%i (should be EVT%i)", curr_ivg, ivg);

  /* Not allowed to return from usermode.  */
  if (curr_ivg == IVG_USER)
    cec_exception (cpu, VEC_ILL_RES);

  if (ivg > IVG15 || ivg < 0)
    sim_io_error (sd, "%s: ivg %i out of range !", __func__, ivg);

  _cec_require_supervisor (cpu, cec);

  switch (ivg)
    {
    case IVG_EMU:
      /* RTE -- only valid in emulation mode.  */
      /* XXX: What does the hardware do ?  */
      if (curr_ivg != IVG_EMU)
	cec_exception (cpu, VEC_ILL_RES);
      break;
    case IVG_NMI:
      /* RTN -- only valid in NMI.  */
      /* XXX: What does the hardware do ?  */
      if (curr_ivg != IVG_NMI)
	cec_exception (cpu, VEC_ILL_RES);
      break;
    case IVG_EVX:
      /* RTX -- only valid in exception.  */
      /* XXX: What does the hardware do ?  */
      if (curr_ivg != IVG_EVX)
	cec_exception (cpu, VEC_ILL_RES);
      break;
    default:
      /* RTI -- not valid in emulation, nmi, exception, or user.  */
      /* XXX: What does the hardware do ?  */
      if (curr_ivg == IVG_EMU || curr_ivg == IVG_NMI
	  || curr_ivg == IVG_EVX || curr_ivg == IVG_USER)
	cec_exception (cpu, VEC_ILL_RES);
      break;
    case IVG_IRPTEN:
      /* XXX: Is this even possible ?  */
      excp_to_sim_halt (sim_stopped, SIM_SIGABRT);
      break;
    }
  newpc = cec_read_ret_reg (cpu, ivg);

  /* XXX: Does this nested trick work on EMU/NMI/EVX ?  */
  snen = (newpc & 1);
  /* XXX: Delayed clear shows bad PCREG register trace above ?  */
  SET_PCREG (newpc & ~1);

  BFIN_TRACE_BRANCH (cpu, oldpc, PCREG, -1, "CEC changed PC (from EVT%i)", ivg);

  /* Update ipend after the BFIN_TRACE_BRANCH so dv-bfin_trace
     knows current CEC state wrt overflow.  */
  if (!snen)
    cec->ipend &= ~(1 << ivg);

  /* Disable global interrupt mask to let any interrupt take over, but
     only when we were already in a RTI level.  Only way we could have
     raised at that point is if it was cleared in the first place.  */
  if (ivg >= IVG_IVHW || ivg == IVG_RST)
    cec_irpten_disable (cpu, cec);

  /* When going from super to user, we clear LSB in LB regs in case
     it was set on the transition up.
     Also need to load SP alias with USP.  */
  if (_cec_get_ivg (cec) == -1)
    {
      int i;
      for (i = 0; i < 2; ++i)
	if (LBREG (i) & 1)
	  SET_LBREG (i, LBREG (i) & ~1);
      SET_KSPREG (SPREG);
      SET_SPREG (USPREG);
    }

  /* Check for pending interrupts before we return to usermode.  */
  _cec_check_pending (cpu, cec);
}

void
cec_push_reti (SIM_CPU *cpu)
{
  /* XXX: Need to check hardware with popped RETI value
     and bit 1 is set (when handling nested interrupts).
     Also need to check behavior wrt SNEN in SYSCFG.  */
  struct bfin_cec *cec;

  if (STATE_ENVIRONMENT (CPU_STATE (cpu)) != OPERATING_ENVIRONMENT)
    return;

  TRACE_EVENTS (cpu, "pushing RETI");

  cec = CEC_STATE (cpu);
  cec_irpten_disable (cpu, cec);
  /* Check for pending interrupts.  */
  _cec_check_pending (cpu, cec);
}

void
cec_pop_reti (SIM_CPU *cpu)
{
  /* XXX: Need to check hardware with popped RETI value
     and bit 1 is set (when handling nested interrupts).
     Also need to check behavior wrt SNEN in SYSCFG.  */
  struct bfin_cec *cec;

  if (STATE_ENVIRONMENT (CPU_STATE (cpu)) != OPERATING_ENVIRONMENT)
    return;

  TRACE_EVENTS (cpu, "popping RETI");

  cec = CEC_STATE (cpu);
  cec_irpten_enable (cpu, cec);
}
