/*> cp1.c <*/
/* MIPS Simulator FPU (CoProcessor 1) support.
   Copyright (C) 2002-2024 Free Software Foundation, Inc.
   Originally created by Cygnus Solutions.  Extensive modifications,
   including paired-single operation support and MIPS-3D support
   contributed by Ed Satterthwaite and Chris Demetriou, of Broadcom
   Corporation (SiByte).

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

/* XXX: The following notice should be removed as soon as is practical:  */
/* Floating Point Support for gdb MIPS simulators

   This file is part of the MIPS sim

		THIS SOFTWARE IS NOT COPYRIGHTED
   (by Cygnus.)

   Cygnus offers the following for use in the public domain.  Cygnus
   makes no warranty with regard to the software or it's performance
   and the user accepts the software "AS IS" with all faults.

   CYGNUS DISCLAIMS ANY WARRANTIES, EXPRESS OR IMPLIED, WITH REGARD TO
   THIS SOFTWARE INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.

   (Originally, this code was in interp.c)
*/

/* This must come before any other includes.  */
#include "defs.h"

#include "sim-main.h"

#include <stdlib.h>

/* Within cp1.c we refer to sim_cpu directly.  */
#define CPU cpu
#define SD CPU_STATE(cpu)

/*-- FPU support routines ---------------------------------------------------*/

/* Numbers are held in normalized form. The SINGLE and DOUBLE binary
   formats conform to ANSI/IEEE Std 754-1985.

   SINGLE precision floating:
      seeeeeeeefffffffffffffffffffffff
        s =  1bit  = sign
        e =  8bits = exponent
        f = 23bits = fraction

   SINGLE precision fixed:
      siiiiiiiiiiiiiiiiiiiiiiiiiiiiiii
        s =  1bit  = sign
        i = 31bits = integer

   DOUBLE precision floating:
      seeeeeeeeeeeffffffffffffffffffffffffffffffffffffffffffffffffffff
        s =  1bit  = sign
        e = 11bits = exponent
        f = 52bits = fraction

   DOUBLE precision fixed:
      siiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii
        s =  1bit  = sign
        i = 63bits = integer

   PAIRED SINGLE precision floating:
      seeeeeeeefffffffffffffffffffffffseeeeeeeefffffffffffffffffffffff
      |         upper                ||         lower                |
        s =  1bit  = sign
        e =  8bits = exponent
        f = 23bits = fraction
    Note: upper = [63..32], lower = [31..0]
 */

/* Extract packed single values:  */
#define FP_PS_upper(v) (((v) >> 32) & (unsigned)0xFFFFFFFF)
#define FP_PS_lower(v) ((v) & (unsigned)0xFFFFFFFF)
#define FP_PS_cat(u,l) (((uint64_t)((u) & (unsigned)0xFFFFFFFF) << 32) \
                        | (uint64_t)((l) & 0xFFFFFFFF))

/* Explicit QNaN values.  */
#define FPQNaN_SINGLE   (0x7FBFFFFF)
#define FPQNaN_WORD     (0x7FFFFFFF)
#define FPQNaN_DOUBLE   (UNSIGNED64 (0x7FF7FFFFFFFFFFFF))
#define FPQNaN_LONG     (UNSIGNED64 (0x7FFFFFFFFFFFFFFF))
#define FPQNaN_PS       (FP_PS_cat (FPQNaN_SINGLE, FPQNaN_SINGLE))

static void update_fcsr (sim_cpu *, address_word, sim_fpu_status);

static const char *fpu_format_name (FP_formats fmt);
#ifdef DEBUG
static const char *fpu_rounding_mode_name (int rm);
#endif

uword64
value_fpr (sim_cpu *cpu,
	   address_word cia,
	   int fpr,
	   FP_formats fmt)
{
  uword64 value = 0;
  int err = 0;

  /* Treat unused register values, as fixed-point 64bit values.  */
  if (fmt == fmt_unknown)
    {
#if 1
      /* If request to read data as "unknown", then use the current
	 encoding:  */
      fmt = FPR_STATE[fpr];
#else
      fmt = fmt_long;
#endif
    }

  /* For values not yet accessed, set to the desired format.  */
  if (fmt < fmt_uninterpreted && fmt != fmt_dc32)
    {
      if (FPR_STATE[fpr] == fmt_uninterpreted)
	{
	  FPR_STATE[fpr] = fmt;
#ifdef DEBUG
	  printf ("DBG: Register %d was fmt_uninterpreted. Now %s\n", fpr,
		  fpu_format_name (fmt));
#endif /* DEBUG */
	}
      else if (fmt != FPR_STATE[fpr]
	       && !(fmt == fmt_single
		    && FPR_STATE[fpr] == fmt_double
		    && (FGR[fpr] == 0 || FGR[fpr] == 0xFFFFFFFF)))
	{
	  sim_io_eprintf (SD, "FPR %d (format %s) being accessed with format %s - setting to unknown (PC = 0x%s)\n",
			  fpr, fpu_format_name (FPR_STATE[fpr]),
			  fpu_format_name (fmt), pr_addr (cia));
	  FPR_STATE[fpr] = fmt_unknown;
	}
    }

  if (FPR_STATE[fpr] == fmt_unknown)
    {
      /* Set QNaN value:  */
      switch (fmt)
	{
	case fmt_single:  value = FPQNaN_SINGLE;  break;
	case fmt_double:  value = FPQNaN_DOUBLE;  break;
	case fmt_word:    value = FPQNaN_WORD;    break;
	case fmt_long:    value = FPQNaN_LONG;    break;
	case fmt_ps:      value = FPQNaN_PS;      break;
	default:          err = -1;               break;
	}
    }
  else if (SizeFGR () == 64)
    {
      switch (fmt)
	{
	case fmt_uninterpreted_32:
	case fmt_single:
	case fmt_word:
	case fmt_dc32:
	  value = (FGR[fpr] & 0xFFFFFFFF);
	  break;

	case fmt_uninterpreted_64:
	case fmt_uninterpreted:
	case fmt_double:
	case fmt_long:
	case fmt_ps:
	  value = FGR[fpr];
	  break;

	default:
	  err = -1;
	  break;
	}
    }
  else
    {
      switch (fmt)
	{
	case fmt_uninterpreted_32:
	case fmt_single:
	case fmt_word:
	  value = (FGR[fpr] & 0xFFFFFFFF);
	  break;

	case fmt_uninterpreted_64:
	case fmt_uninterpreted:
	case fmt_double:
	case fmt_long:
	  if ((fpr & 1) == 0)
	    {
	      /* Even register numbers only.  */
#ifdef DEBUG
	      printf ("DBG: ValueFPR: FGR[%d] = %s, FGR[%d] = %s\n",
		      fpr + 1, pr_uword64 ((uword64) FGR[fpr+1]),
		      fpr, pr_uword64 ((uword64) FGR[fpr]));
#endif
	      value = ((((uword64) FGR[fpr+1]) << 32)
		       | (FGR[fpr] & 0xFFFFFFFF));
	    }
	  else
	    {
	      SignalException (ReservedInstruction, 0);
	    }
	  break;

	case fmt_ps:
	  SignalException (ReservedInstruction, 0);
	  break;

	default:
	  err = -1;
	  break;
	}
    }

  if (err)
    SignalExceptionSimulatorFault ("Unrecognised FP format in ValueFPR ()");

#ifdef DEBUG
  printf ("DBG: ValueFPR: fpr = %d, fmt = %s, value = 0x%s : PC = 0x%s : SizeFGR () = %d\n",
	  fpr, fpu_format_name (fmt), pr_uword64 (value), pr_addr (cia),
	  SizeFGR ());
#endif /* DEBUG */

  return (value);
}

void
store_fpr (sim_cpu *cpu,
	   address_word cia,
	   int fpr,
	   FP_formats fmt,
	   uword64 value)
{
  int err = 0;

#ifdef DEBUG
  printf ("DBG: StoreFPR: fpr = %d, fmt = %s, value = 0x%s : PC = 0x%s : SizeFGR () = %d, \n",
	  fpr, fpu_format_name (fmt), pr_uword64 (value), pr_addr (cia),
	  SizeFGR ());
#endif /* DEBUG */

  if (SizeFGR () == 64)
    {
      switch (fmt)
	{
	case fmt_uninterpreted_32:
	  fmt = fmt_uninterpreted;
	  ATTRIBUTE_FALLTHROUGH;
	case fmt_single:
	case fmt_word:
	  if (STATE_VERBOSE_P (SD))
	    sim_io_eprintf (SD,
			    "Warning: PC 0x%s: interp.c store_fpr DEADCODE\n",
			    pr_addr (cia));
	  FGR[fpr] = (((uword64) 0xDEADC0DE << 32) | (value & 0xFFFFFFFF));
	  FPR_STATE[fpr] = fmt;
	  break;

	case fmt_uninterpreted_64:
	  fmt = fmt_uninterpreted;
	  ATTRIBUTE_FALLTHROUGH;
	case fmt_uninterpreted:
	case fmt_double:
	case fmt_long:
	case fmt_ps:
	  FGR[fpr] = value;
	  FPR_STATE[fpr] = fmt;
	  break;

	default:
	  FPR_STATE[fpr] = fmt_unknown;
	  err = -1;
	  break;
	}
    }
  else
    {
      switch (fmt)
	{
	case fmt_uninterpreted_32:
	  fmt = fmt_uninterpreted;
	  ATTRIBUTE_FALLTHROUGH;
	case fmt_single:
	case fmt_word:
	  FGR[fpr] = (value & 0xFFFFFFFF);
	  FPR_STATE[fpr] = fmt;
	  break;

	case fmt_uninterpreted_64:
	  fmt = fmt_uninterpreted;
	  ATTRIBUTE_FALLTHROUGH;
	case fmt_uninterpreted:
	case fmt_double:
	case fmt_long:
	  if ((fpr & 1) == 0)
	    {
	      /* Even register numbers only.  */
	      FGR[fpr+1] = (value >> 32);
	      FGR[fpr] = (value & 0xFFFFFFFF);
	      FPR_STATE[fpr + 1] = fmt;
	      FPR_STATE[fpr] = fmt;
	    }
	  else
	    {
	      FPR_STATE[fpr] = fmt_unknown;
	      FPR_STATE[fpr ^ 1] = fmt_unknown;
	      SignalException (ReservedInstruction, 0);
	    }
	  break;

	case fmt_ps:
	  FPR_STATE[fpr] = fmt_unknown;
	  SignalException (ReservedInstruction, 0);
	  break;

	default:
	  FPR_STATE[fpr] = fmt_unknown;
	  err = -1;
	  break;
	}
    }

  if (err)
    SignalExceptionSimulatorFault ("Unrecognised FP format in StoreFPR ()");

#ifdef DEBUG
  printf ("DBG: StoreFPR: fpr[%d] = 0x%s (format %s)\n",
	  fpr, pr_uword64 (FGR[fpr]), fpu_format_name (fmt));
#endif /* DEBUG */

  return;
}


/* CP1 control/status register access functions.  */

void
test_fcsr (sim_cpu *cpu,
	   address_word cia)
{
  unsigned int cause;

  cause = (FCSR & fcsr_CAUSE_mask) >> fcsr_CAUSE_shift;
  if ((cause & ((FCSR & fcsr_ENABLES_mask) >> fcsr_ENABLES_shift)) != 0
      || (cause & (1 << UO)))
    {
      SignalExceptionFPE();
    }
}

unsigned_word
value_fcr(sim_cpu *cpu,
	  address_word cia,
	  int fcr)
{
  uint32_t value = 0;

  switch (fcr)
    {
    case 0:  /* FP Implementation and Revision Register.  */
      value = FCR0;
      break;
    case 25:  /* FP Condition Codes Register (derived from FCSR).  */
      value = (FCR31 & fcsr_FCC_mask) >> fcsr_FCC_shift;
      value = (value & 0x1) | (value >> 1);   /* Close FCC gap.  */
      break;
    case 26:  /* FP Exceptions Register (derived from FCSR).  */
      value = FCR31 & (fcsr_CAUSE_mask | fcsr_FLAGS_mask);
      break;
    case 28:  /* FP Enables Register (derived from FCSR).  */
      value = FCR31 & (fcsr_ENABLES_mask | fcsr_RM_mask);
      if ((FCR31 & fcsr_FS) != 0)
	value |= fenr_FS;
      break;
    case 31:  /* FP Control/Status Register (FCSR).  */
      value = FCR31 & ~fcsr_ZERO_mask;
      break;
    }

  return (EXTEND32 (value));
}

void
store_fcr(sim_cpu *cpu,
	  address_word cia,
	  int fcr,
	  unsigned_word value)
{
  uint32_t v;

  v = VL4_8(value);
  switch (fcr)
    {
    case 25:  /* FP Condition Codes Register (stored into FCSR).  */
      v = (v << 1) | (v & 0x1);             /* Adjust for FCC gap.  */
      FCR31 &= ~fcsr_FCC_mask;
      FCR31 |= ((v << fcsr_FCC_shift) & fcsr_FCC_mask);
      break;
    case 26:  /* FP Exceptions Register (stored into FCSR).  */
      FCR31 &= ~(fcsr_CAUSE_mask | fcsr_FLAGS_mask);
      FCR31 |= (v & (fcsr_CAUSE_mask | fcsr_FLAGS_mask));
      test_fcsr(cpu, cia);
      break;
    case 28:  /* FP Enables Register (stored into FCSR).  */
      if ((v & fenr_FS) != 0)
	v |= fcsr_FS;
      else
	v &= ~fcsr_FS;
      FCR31 &= (fcsr_FCC_mask | fcsr_CAUSE_mask | fcsr_FLAGS_mask);
      FCR31 |= (v & (fcsr_FS | fcsr_ENABLES_mask | fcsr_RM_mask));
      test_fcsr(cpu, cia);
      break;
    case 31:  /* FP Control/Status Register (FCSR).  */
      FCR31 = v & ~fcsr_ZERO_mask;
      test_fcsr(cpu, cia);
      break;
    }
}

static void
update_fcsr (sim_cpu *cpu,
	     address_word cia,
	     sim_fpu_status status)
{
  FCSR &= ~fcsr_CAUSE_mask;

  if (status != 0)
    {
      unsigned int cause = 0;

      /* map between sim_fpu codes and MIPS FCSR */
      if (status & (sim_fpu_status_invalid_snan
		    | sim_fpu_status_invalid_isi
		    | sim_fpu_status_invalid_idi
		    | sim_fpu_status_invalid_zdz
		    | sim_fpu_status_invalid_imz
		    | sim_fpu_status_invalid_cmp
		    | sim_fpu_status_invalid_sqrt
		    | sim_fpu_status_invalid_cvi))
	cause |= (1 << IO);
      if (status & sim_fpu_status_invalid_div0)
	cause |= (1 << DZ);
      if (status & sim_fpu_status_overflow)
	cause |= (1 << OF);
      if (status & sim_fpu_status_underflow)
	cause |= (1 << UF);
      if (status & sim_fpu_status_inexact)
	cause |= (1 << IR);
#if 0 /* Not yet.  */
      /* Implicit clearing of other bits by unimplemented done by callers.  */
      if (status & sim_fpu_status_unimplemented)
	cause |= (1 << UO);
#endif

      FCSR |= (cause << fcsr_CAUSE_shift);
      test_fcsr (cpu, cia);
      FCSR |= ((cause & ~(1 << UO)) << fcsr_FLAGS_shift);
    }
  return;
}

static sim_fpu_round
rounding_mode(int rm)
{
  sim_fpu_round round;

  switch (rm)
    {
    case FP_RM_NEAREST:
      /* Round result to nearest representable value. When two
	 representable values are equally near, round to the value
	 that has a least significant bit of zero (i.e. is even).  */
      round = sim_fpu_round_near;
      break;
    case FP_RM_TOZERO:
      /* Round result to the value closest to, and not greater in
	 magnitude than, the result.  */
      round = sim_fpu_round_zero;
      break;
    case FP_RM_TOPINF:
      /* Round result to the value closest to, and not less than,
	 the result.  */
      round = sim_fpu_round_up;
      break;
    case FP_RM_TOMINF:
      /* Round result to the value closest to, and not greater than,
	 the result.  */
      round = sim_fpu_round_down;
      break;
    default:
      round = 0;
      fprintf (stderr, "Bad switch\n");
      abort ();
    }
  return round;
}

/* When the FS bit is set, MIPS processors return zero for
   denormalized results and optionally replace denormalized inputs
   with zero.  When FS is clear, some implementation trap on input
   and/or output, while other perform the operation in hardware.  */
static sim_fpu_denorm
denorm_mode(sim_cpu *cpu)
{
  sim_fpu_denorm denorm;

  /* XXX: FIXME: Eventually should be CPU model dependent.  */
  if (GETFS())
    denorm = sim_fpu_denorm_zero;
  else
    denorm = 0;
  return denorm;
}


/* Comparison operations.  */

static sim_fpu_status
fp_test(uint64_t op1,
	uint64_t op2,
	FP_formats fmt,
	int abs,
	int cond,
	int *condition)
{
  sim_fpu wop1;
  sim_fpu wop2;
  sim_fpu_status status = 0;
  int  less, equal, unordered;

  /* The format type has already been checked:  */
  switch (fmt)
    {
    case fmt_single:
      {
	sim_fpu_32to (&wop1, op1);
	sim_fpu_32to (&wop2, op2);
	break;
      }
    case fmt_double:
      {
	sim_fpu_64to (&wop1, op1);
	sim_fpu_64to (&wop2, op2);
	break;
      }
    default:
      fprintf (stderr, "Bad switch\n");
      abort ();
    }

  if (sim_fpu_is_nan (&wop1) || sim_fpu_is_nan (&wop2))
    {
      if ((cond & (1 << 3))
	  || sim_fpu_is_snan (&wop1) || sim_fpu_is_snan (&wop2))
	status = sim_fpu_status_invalid_snan;
      less = 0;
      equal = 0;
      unordered = 1;
    }
  else
    {
      if (abs)
	{
	  status |= sim_fpu_abs (&wop1, &wop1);
	  status |= sim_fpu_abs (&wop2, &wop2);
	}
      equal = sim_fpu_is_eq (&wop1, &wop2);
      less = !equal && sim_fpu_is_lt (&wop1, &wop2);
      unordered = 0;
    }
  *condition = (((cond & (1 << 2)) && less)
		|| ((cond & (1 << 1)) && equal)
		|| ((cond & (1 << 0)) && unordered));
  return status;
}

static const int sim_fpu_class_mips_mapping[] = {
  FP_R6CLASS_SNAN, /* SIM_FPU_IS_SNAN = 1, Noisy not-a-number  */
  FP_R6CLASS_QNAN, /* SIM_FPU_IS_QNAN = 2, Quiet not-a-number  */
  FP_R6CLASS_NEGINF, /* SIM_FPU_IS_NINF = 3, -infinity  */
  FP_R6CLASS_POSINF, /* SIM_FPU_IS_PINF = 4, +infinity  */
  FP_R6CLASS_NEGNORM, /* SIM_FPU_IS_NNUMBER = 5, -num - [-MAX .. -MIN]  */
  FP_R6CLASS_POSNORM, /* SIM_FPU_IS_PNUMBER = 6, +num - [+MIN .. +MAX]  */
  FP_R6CLASS_NEGSUB, /* SIM_FPU_IS_NDENORM = 7, -denorm - (MIN .. 0)  */
  FP_R6CLASS_POSSUB, /* SIM_FPU_IS_PDENORM = 8, +denorm - (0 .. MIN)  */
  FP_R6CLASS_NEGZERO, /* SIM_FPU_IS_NZERO = 9, -0  */
  FP_R6CLASS_POSZERO /* SIM_FPU_IS_PZERO = 10, +0  */
};

uint64_t
fp_classify (sim_cpu *cpu,
	     address_word cia,
	     uint64_t op,
	     FP_formats fmt)
{
  sim_fpu wop;

  switch (fmt)
    {
    case fmt_single:
      sim_fpu_32to (&wop, op);
      break;
    case fmt_double:
      sim_fpu_64to (&wop, op);
      break;
    default:
      sim_io_error (SD, "Bad switch\n");
    }
  return sim_fpu_class_mips_mapping[sim_fpu_classify (&wop) - 1];
}

int
fp_rint (sim_cpu *cpu,
	 address_word cia,
	 uint64_t op,
	 uint64_t *ans,
	 FP_formats fmt)
{
  sim_fpu wop = {0}, wtemp = {0}, wmagic = {0}, wans = {0};
  int status = 0;
  sim_fpu_round round = rounding_mode (GETRM());

  switch (fmt)
    {
    case fmt_single:
      sim_fpu_32to (&wop, op);
      sim_fpu_32to (&wmagic, 0x4b000000);
      break;
    case fmt_double:
      sim_fpu_64to (&wop, op);
      sim_fpu_64to (&wmagic, 0x4330000000000000);
      break;
    default:
      sim_io_error (SD, "Bad switch\n");
    }

  if (sim_fpu_is_nan (&wop) || sim_fpu_is_infinity (&wop))
    {
      status = sim_fpu_status_invalid_cvi;
      update_fcsr (cpu, cia, status);
      return status;
    }

  switch (fmt)
    {
    case fmt_single:
      if (sim_fpu_is_ge (&wop, &wmagic))
	wans = wop;
      else
	{
	  sim_fpu_add (&wtemp, &wop, &wmagic);
	  sim_fpu_round_32 (&wtemp, round, sim_fpu_denorm_default);
	  sim_fpu_sub (&wans, &wtemp, &wmagic);
	}
      sim_fpu_to32 ((uint32_t *) ans, &wans);
      break;
    case fmt_double:
      if (sim_fpu_is_ge (&wop, &wmagic))
	wans = wop;
      else
	{
	  sim_fpu_add (&wtemp, &wop, &wmagic);
	  sim_fpu_round_64 (&wtemp, round, sim_fpu_denorm_default);
	  sim_fpu_sub (&wans, &wtemp, &wmagic);
	}
      sim_fpu_to64 (ans, &wans);
      break;
    default:
      sim_io_error (SD, "Bad switch\n");
    }

  if (*ans != op && status == 0)
    status = sim_fpu_status_inexact;

  update_fcsr (cpu, cia, status);
  return status;
}

void
fp_cmp(sim_cpu *cpu,
       address_word cia,
       uint64_t op1,
       uint64_t op2,
       FP_formats fmt,
       int abs,
       int cond,
       int cc)
{
  sim_fpu_status status = 0;

  /* The format type should already have been checked.  The FCSR is
     updated before the condition codes so that any exceptions will
     be signalled before the condition codes are changed.  */
  switch (fmt)
    {
    case fmt_single:
    case fmt_double:
      {
	int result;
	status = fp_test(op1, op2, fmt, abs, cond, &result);
	update_fcsr (cpu, cia, status);
	SETFCC (cc, result);
	break;
      }
    case fmt_ps:
      {
	int result0, result1;
	status  = fp_test(FP_PS_lower (op1), FP_PS_lower (op2), fmt_single,
			  abs, cond, &result0);
	status |= fp_test(FP_PS_upper (op1), FP_PS_upper (op2), fmt_single,
			  abs, cond, &result1);
	update_fcsr (cpu, cia, status);
	SETFCC (cc, result0);
	SETFCC (cc+1, result1);
	break;
      }
    default:
      sim_io_error (SD, "Bad switch\n");
    }
}

uint64_t
fp_r6_cmp (sim_cpu *cpu,
	   address_word cia,
	   uint64_t op1,
	   uint64_t op2,
	   FP_formats fmt,
	   int cond)
{
  sim_fpu wop1, wop2;
  int result = 0;

  switch (fmt)
    {
    case fmt_single:
      sim_fpu_32to (&wop1, op1);
      sim_fpu_32to (&wop2, op2);
      break;
    case fmt_double:
      sim_fpu_64to (&wop1, op1);
      sim_fpu_64to (&wop2, op2);
      break;
    default:
      sim_io_error (SD, "Bad switch\n");
    }

  switch (cond)
    {
    case FP_R6CMP_AF:
      result = 0;
      break;
    case FP_R6CMP_UN:
      result = sim_fpu_is_un (&wop1, &wop2);
      break;
    case FP_R6CMP_OR:
      result = sim_fpu_is_or (&wop1, &wop2);
      break;
    case FP_R6CMP_EQ:
      result = sim_fpu_is_eq (&wop1, &wop2);
      break;
    case FP_R6CMP_NE:
      result = sim_fpu_is_ne (&wop1, &wop2);
      break;
    case FP_R6CMP_LT:
      result = sim_fpu_is_lt (&wop1, &wop2);
      break;
    case FP_R6CMP_LE:
      result = sim_fpu_is_le (&wop1, &wop2);
      break;
    case FP_R6CMP_UEQ:
      result = sim_fpu_is_un (&wop1, &wop2) || sim_fpu_is_eq (&wop1, &wop2);
      break;
    case FP_R6CMP_UNE:
      result = sim_fpu_is_un (&wop1, &wop2) || sim_fpu_is_ne (&wop1, &wop2);
      break;
    case FP_R6CMP_ULT:
      result = sim_fpu_is_un (&wop1, &wop2) || sim_fpu_is_lt (&wop1, &wop2);
      break;
    case FP_R6CMP_ULE:
      result = sim_fpu_is_un (&wop1, &wop2) || sim_fpu_is_le (&wop1, &wop2);
      break;
    default:
      update_fcsr (cpu, cia, sim_fpu_status_invalid_cmp);
      break;
    }

  if (result)
    {
      switch (fmt)
	{
	case fmt_single:
	  return 0xFFFFFFFF;
	case fmt_double:
	  return 0xFFFFFFFFFFFFFFFF;
	default:
	  sim_io_error (SD, "Bad switch\n");
	}
     }
   else
     return 0;
}

/* Basic arithmetic operations.  */

static uint64_t
fp_unary(sim_cpu *cpu,
	 address_word cia,
	 int (*sim_fpu_op)(sim_fpu *, const sim_fpu *),
	 uint64_t op,
	 FP_formats fmt)
{
  sim_fpu wop = {0};
  sim_fpu ans;
  sim_fpu_round round = rounding_mode (GETRM());
  sim_fpu_denorm denorm = denorm_mode (cpu);
  sim_fpu_status status = 0;
  uint64_t result = 0;

  /* The format type has already been checked: */
  switch (fmt)
    {
    case fmt_single:
      {
	uint32_t res;
	sim_fpu_32to (&wop, op);
	status |= (*sim_fpu_op) (&ans, &wop);
	status |= sim_fpu_round_32 (&ans, round, denorm);
	sim_fpu_to32 (&res, &ans);
	result = res;
	break;
      }
    case fmt_double:
      {
	uint64_t res;
	sim_fpu_64to (&wop, op);
	status |= (*sim_fpu_op) (&ans, &wop);
	status |= sim_fpu_round_64 (&ans, round, denorm);
	sim_fpu_to64 (&res, &ans);
	result = res;
	break;
      }
    case fmt_ps:
      {
	int status_u = 0, status_l = 0;
	uint32_t res_u, res_l;
	sim_fpu_32to (&wop, FP_PS_upper(op));
	status_u |= (*sim_fpu_op) (&ans, &wop);
	sim_fpu_to32 (&res_u, &ans);
	sim_fpu_32to (&wop, FP_PS_lower(op));
	status_l |= (*sim_fpu_op) (&ans, &wop);
	sim_fpu_to32 (&res_l, &ans);
	result = FP_PS_cat(res_u, res_l);
	status = status_u | status_l;
	break;
      }
    default:
      sim_io_error (SD, "Bad switch\n");
    }

  update_fcsr (cpu, cia, status);
  return result;
}

static uint64_t
fp_binary(sim_cpu *cpu,
	  address_word cia,
	  int (*sim_fpu_op)(sim_fpu *, const sim_fpu *, const sim_fpu *),
	  uint64_t op1,
	  uint64_t op2,
	  FP_formats fmt)
{
  sim_fpu wop1 = {0};
  sim_fpu wop2 = {0};
  sim_fpu ans  = {0};
  sim_fpu_round round = rounding_mode (GETRM());
  sim_fpu_denorm denorm = denorm_mode (cpu);
  sim_fpu_status status = 0;
  uint64_t result = 0;

  /* The format type has already been checked: */
  switch (fmt)
    {
    case fmt_single:
      {
	uint32_t res;
	sim_fpu_32to (&wop1, op1);
	sim_fpu_32to (&wop2, op2);
	status |= (*sim_fpu_op) (&ans, &wop1, &wop2);
	status |= sim_fpu_round_32 (&ans, round, denorm);
	sim_fpu_to32 (&res, &ans);
	result = res;
	break;
      }
    case fmt_double:
      {
	uint64_t res;
	sim_fpu_64to (&wop1, op1);
	sim_fpu_64to (&wop2, op2);
	status |= (*sim_fpu_op) (&ans, &wop1, &wop2);
	status |= sim_fpu_round_64 (&ans, round, denorm);
	sim_fpu_to64 (&res, &ans);
	result = res;
	break;
      }
    case fmt_ps:
      {
	int status_u = 0, status_l = 0;
	uint32_t res_u, res_l;
	sim_fpu_32to (&wop1, FP_PS_upper(op1));
	sim_fpu_32to (&wop2, FP_PS_upper(op2));
	status_u |= (*sim_fpu_op) (&ans, &wop1, &wop2);
	sim_fpu_to32 (&res_u, &ans);
	sim_fpu_32to (&wop1, FP_PS_lower(op1));
	sim_fpu_32to (&wop2, FP_PS_lower(op2));
	status_l |= (*sim_fpu_op) (&ans, &wop1, &wop2);
	sim_fpu_to32 (&res_l, &ans);
	result = FP_PS_cat(res_u, res_l);
	status = status_u | status_l;
	break;
      }
    default:
      sim_io_error (SD, "Bad switch\n");
    }

  update_fcsr (cpu, cia, status);
  return result;
}

/* Common MAC code for single operands (.s or .d), defers setting FCSR.  */
static sim_fpu_status
inner_mac(int (*sim_fpu_op)(sim_fpu *, const sim_fpu *, const sim_fpu *),
	  uint64_t op1,
	  uint64_t op2,
	  uint64_t op3,
	  int scale,
	  int negate,
	  FP_formats fmt,
	  sim_fpu_round round,
	  sim_fpu_denorm denorm,
	  uint64_t *result)
{
  sim_fpu wop1;
  sim_fpu wop2;
  sim_fpu ans;
  sim_fpu_status status = 0;
  sim_fpu_status op_status;
  uint64_t temp = 0;

  switch (fmt)
    {
    case fmt_single:
      {
	uint32_t res;
	sim_fpu_32to (&wop1, op1);
	sim_fpu_32to (&wop2, op2);
	status |= sim_fpu_mul (&ans, &wop1, &wop2);
	if (scale != 0 && sim_fpu_is_number (&ans))  /* number or denorm */
	  ans.normal_exp += scale;
	status |= sim_fpu_round_32 (&ans, round, denorm);
	wop1 = ans;
	op_status = 0;
	sim_fpu_32to (&wop2, op3);
	op_status |= (*sim_fpu_op) (&ans, &wop1, &wop2);
	op_status |= sim_fpu_round_32 (&ans, round, denorm);
	status |= op_status;
	if (negate)
	  {
	    wop1 = ans;
	    op_status = sim_fpu_neg (&ans, &wop1);
	    op_status |= sim_fpu_round_32 (&ans, round, denorm);
	    status |= op_status;
	  }
	sim_fpu_to32 (&res, &ans);
	temp = res;
	break;
      }
    case fmt_double:
      {
	uint64_t res;
	sim_fpu_64to (&wop1, op1);
	sim_fpu_64to (&wop2, op2);
	status |= sim_fpu_mul (&ans, &wop1, &wop2);
	if (scale != 0 && sim_fpu_is_number (&ans))  /* number or denorm */
	  ans.normal_exp += scale;
	status |= sim_fpu_round_64 (&ans, round, denorm);
	wop1 = ans;
	op_status = 0;
	sim_fpu_64to (&wop2, op3);
	op_status |= (*sim_fpu_op) (&ans, &wop1, &wop2);
	op_status |= sim_fpu_round_64 (&ans, round, denorm);
	status |= op_status;
	if (negate)
	  {
	    wop1 = ans;
	    op_status = sim_fpu_neg (&ans, &wop1);
	    op_status |= sim_fpu_round_64 (&ans, round, denorm);
	    status |= op_status;
	  }
	sim_fpu_to64 (&res, &ans);
	temp = res;
	break;
      }
    default:
      fprintf (stderr, "Bad switch\n");
      abort ();
    }
  *result = temp;
  return status;
}

/* Common implementation of madd, nmadd, msub, nmsub that does
   intermediate rounding per spec.  Also used for recip2 and rsqrt2,
   which are transformed into equivalent nmsub operations.  The scale
   argument is an adjustment to the exponent of the intermediate
   product op1*op2.  It is currently non-zero for rsqrt2 (-1), which
   requires an effective division by 2. */
static uint64_t
fp_mac(sim_cpu *cpu,
       address_word cia,
       int (*sim_fpu_op)(sim_fpu *, const sim_fpu *, const sim_fpu *),
       uint64_t op1,
       uint64_t op2,
       uint64_t op3,
       int scale,
       int negate,
       FP_formats fmt)
{
  sim_fpu_round round = rounding_mode (GETRM());
  sim_fpu_denorm denorm = denorm_mode (cpu);
  sim_fpu_status status = 0;
  uint64_t result = 0;

  /* The format type has already been checked: */
  switch (fmt)
    {
    case fmt_single:
    case fmt_double:
      status = inner_mac(sim_fpu_op, op1, op2, op3, scale,
			 negate, fmt, round, denorm, &result);
      break;
    case fmt_ps:
      {
	int status_u, status_l;
	uint64_t result_u, result_l;
	status_u = inner_mac(sim_fpu_op, FP_PS_upper(op1), FP_PS_upper(op2),
			     FP_PS_upper(op3), scale, negate, fmt_single,
			     round, denorm, &result_u);
	status_l = inner_mac(sim_fpu_op, FP_PS_lower(op1), FP_PS_lower(op2),
			     FP_PS_lower(op3), scale, negate, fmt_single,
			     round, denorm, &result_l);
	result = FP_PS_cat(result_u, result_l);
	status = status_u | status_l;
	break;
      }
    default:
      sim_io_error (SD, "Bad switch\n");
    }

  update_fcsr (cpu, cia, status);
  return result;
}

/* Common FMAC code for .s, .d. Defers setting FCSR to caller.  */
static sim_fpu_status
inner_fmac (sim_cpu *cpu,
	    int (*sim_fpu_op) (sim_fpu *, const sim_fpu *, const sim_fpu *),
	    uint64_t op1,
	    uint64_t op2,
	    uint64_t op3,
	    sim_fpu_round round,
	    sim_fpu_denorm denorm,
	    FP_formats fmt,
	    uint64_t *result)
{
  sim_fpu wop1, wop2, ans;
  sim_fpu_status status = 0;
  sim_fpu_status op_status;
  uint32_t t32 = 0;
  uint64_t t64 = 0;

  switch (fmt)
    {
    case fmt_single:
      sim_fpu_32to (&wop1, op1);
      sim_fpu_32to (&wop2, op2);
      status |= sim_fpu_mul (&ans, &wop1, &wop2);
      wop1 = ans;
      op_status = 0;
      sim_fpu_32to (&wop2, op3);
      op_status |= (*sim_fpu_op) (&ans, &wop2, &wop1);
      op_status |= sim_fpu_round_32 (&ans, round, denorm);
      status |= op_status;
      sim_fpu_to32 (&t32, &ans);
      t64 = t32;
      break;
    case fmt_double:
      sim_fpu_64to (&wop1, op1);
      sim_fpu_64to (&wop2, op2);
      status |= sim_fpu_mul (&ans, &wop1, &wop2);
      wop1 = ans;
      op_status = 0;
      sim_fpu_64to (&wop2, op3);
      op_status |= (*sim_fpu_op) (&ans, &wop2, &wop1);
      op_status |= sim_fpu_round_64 (&ans, round, denorm);
      status |= op_status;
      sim_fpu_to64 (&t64, &ans);
      break;
    default:
      sim_io_error (SD, "Bad switch\n");
    }

  *result = t64;
  return status;
}

static uint64_t
fp_fmac (sim_cpu *cpu,
	 address_word cia,
	 int (*sim_fpu_op) (sim_fpu *, const sim_fpu *, const sim_fpu *),
	 uint64_t op1,
	 uint64_t op2,
	 uint64_t op3,
	 FP_formats fmt)
{
  sim_fpu_round round = rounding_mode (GETRM());
  sim_fpu_denorm denorm = denorm_mode (cpu);
  sim_fpu_status status = 0;
  uint64_t result = 0;

  switch (fmt)
    {
    case fmt_single:
    case fmt_double:
      status = inner_fmac (cpu, sim_fpu_op, op1, op2, op3,
			   round, denorm, fmt, &result);
      break;
    default:
      sim_io_error (SD, "Bad switch\n");
    }

  update_fcsr (cpu, cia, status);
  return result;
}

/* Common rsqrt code for single operands (.s or .d), intermediate rounding.  */
static sim_fpu_status
inner_rsqrt(uint64_t op1,
	    FP_formats fmt,
	    sim_fpu_round round,
	    sim_fpu_denorm denorm,
	    uint64_t *result)
{
  sim_fpu wop1;
  sim_fpu ans;
  sim_fpu_status status = 0;
  sim_fpu_status op_status;
  uint64_t temp = 0;

  switch (fmt)
    {
    case fmt_single:
      {
	uint32_t res;
	sim_fpu_32to (&wop1, op1);
	status |= sim_fpu_sqrt (&ans, &wop1);
	status |= sim_fpu_round_32 (&ans, round, denorm);
	wop1 = ans;
	op_status = sim_fpu_inv (&ans, &wop1);
	op_status |= sim_fpu_round_32 (&ans, round, denorm);
	sim_fpu_to32 (&res, &ans);
	temp = res;
	status |= op_status;
	break;
      }
    case fmt_double:
      {
	uint64_t res;
	sim_fpu_64to (&wop1, op1);
	status |= sim_fpu_sqrt (&ans, &wop1);
	status |= sim_fpu_round_64 (&ans, round, denorm);
	wop1 = ans;
	op_status = sim_fpu_inv (&ans, &wop1);
	op_status |= sim_fpu_round_64 (&ans, round, denorm);
	sim_fpu_to64 (&res, &ans);
	temp = res;
	status |= op_status;
	break;
      }
    default:
      fprintf (stderr, "Bad switch\n");
      abort ();
    }
  *result = temp;
  return status;
}

static uint64_t
fp_inv_sqrt(sim_cpu *cpu,
	    address_word cia,
	    uint64_t op1,
	    FP_formats fmt)
{
  sim_fpu_round round = rounding_mode (GETRM());
  sim_fpu_denorm denorm = denorm_mode (cpu);
  sim_fpu_status status = 0;
  uint64_t result = 0;

  /* The format type has already been checked: */
  switch (fmt)
    {
    case fmt_single:
    case fmt_double:
      status = inner_rsqrt (op1, fmt, round, denorm, &result);
      break;
    case fmt_ps:
      {
	int status_u, status_l;
	uint64_t result_u, result_l;
	status_u = inner_rsqrt (FP_PS_upper(op1), fmt_single, round, denorm,
				&result_u);
	status_l = inner_rsqrt (FP_PS_lower(op1), fmt_single, round, denorm,
				&result_l);
	result = FP_PS_cat(result_u, result_l);
	status = status_u | status_l;
	break;
      }
    default:
      sim_io_error (SD, "Bad switch\n");
    }

  update_fcsr (cpu, cia, status);
  return result;
}


uint64_t
fp_abs(sim_cpu *cpu,
       address_word cia,
       uint64_t op,
       FP_formats fmt)
{
  return fp_unary(cpu, cia, &sim_fpu_abs, op, fmt);
}

uint64_t
fp_neg(sim_cpu *cpu,
       address_word cia,
       uint64_t op,
       FP_formats fmt)
{
  return fp_unary(cpu, cia, &sim_fpu_neg, op, fmt);
}

uint64_t
fp_add(sim_cpu *cpu,
       address_word cia,
       uint64_t op1,
       uint64_t op2,
       FP_formats fmt)
{
  return fp_binary(cpu, cia, &sim_fpu_add, op1, op2, fmt);
}

uint64_t
fp_sub(sim_cpu *cpu,
       address_word cia,
       uint64_t op1,
       uint64_t op2,
       FP_formats fmt)
{
  return fp_binary(cpu, cia, &sim_fpu_sub, op1, op2, fmt);
}

uint64_t
fp_mul(sim_cpu *cpu,
       address_word cia,
       uint64_t op1,
       uint64_t op2,
       FP_formats fmt)
{
  return fp_binary(cpu, cia, &sim_fpu_mul, op1, op2, fmt);
}

uint64_t
fp_div(sim_cpu *cpu,
       address_word cia,
       uint64_t op1,
       uint64_t op2,
       FP_formats fmt)
{
  return fp_binary(cpu, cia, &sim_fpu_div, op1, op2, fmt);
}

uint64_t
fp_min (sim_cpu *cpu,
	address_word cia,
	uint64_t op1,
	uint64_t op2,
	FP_formats fmt)
{
  return fp_binary (cpu, cia, &sim_fpu_min, op1, op2, fmt);
}

uint64_t
fp_max (sim_cpu *cpu,
	address_word cia,
	uint64_t op1,
	uint64_t op2,
	FP_formats fmt)
{
  return fp_binary (cpu, cia, &sim_fpu_max, op1, op2, fmt);
}

uint64_t
fp_mina (sim_cpu *cpu,
	 address_word cia,
	 uint64_t op1,
	 uint64_t op2,
	 FP_formats fmt)
{
  uint64_t ret;
  sim_fpu wop1 = {0}, wop2 = {0}, waop1, waop2, wans;
  sim_fpu_status status = 0;

  switch (fmt)
    {
    case fmt_single:
      sim_fpu_32to (&wop1, op1);
      sim_fpu_32to (&wop2, op2);
      break;
    case fmt_double:
      sim_fpu_64to (&wop1, op1);
      sim_fpu_64to (&wop2, op2);
      break;
    default:
      sim_io_error (SD, "Bad switch\n");
    }

  status |= sim_fpu_abs (&waop1, &wop1);
  status |= sim_fpu_abs (&waop2, &wop2);
  status |= sim_fpu_min (&wans, &waop1, &waop2);
  ret = (sim_fpu_is_eq (&wans, &waop1)) ? op1 : op2;

  update_fcsr (cpu, cia, status);
  return ret;
}

uint64_t
fp_maxa (sim_cpu *cpu,
	 address_word cia,
	 uint64_t op1,
	 uint64_t op2,
	 FP_formats fmt)
{
  uint64_t ret;
  sim_fpu wop1 = {0}, wop2 = {0}, waop1, waop2, wans;
  sim_fpu_status status = 0;

  switch (fmt)
    {
    case fmt_single:
      sim_fpu_32to (&wop1, op1);
      sim_fpu_32to (&wop2, op2);
      break;
    case fmt_double:
      sim_fpu_64to (&wop1, op1);
      sim_fpu_64to (&wop2, op2);
      break;
    default:
      sim_io_error (SD, "Bad switch\n");
    }

  status |= sim_fpu_abs (&waop1, &wop1);
  status |= sim_fpu_abs (&waop2, &wop2);
  status |= sim_fpu_max (&wans, &waop1, &waop2);
  ret = (sim_fpu_is_eq (&wans, &waop1)) ? op1 : op2;

  update_fcsr (cpu, cia, status);
  return ret;
}

uint64_t
fp_recip(sim_cpu *cpu,
         address_word cia,
         uint64_t op,
         FP_formats fmt)
{
  return fp_unary(cpu, cia, &sim_fpu_inv, op, fmt);
}

uint64_t
fp_sqrt(sim_cpu *cpu,
        address_word cia,
        uint64_t op,
        FP_formats fmt)
{
  return fp_unary(cpu, cia, &sim_fpu_sqrt, op, fmt);
}

uint64_t
fp_rsqrt(sim_cpu *cpu,
         address_word cia,
         uint64_t op,
         FP_formats fmt)
{
  return fp_inv_sqrt(cpu, cia, op, fmt);
}

uint64_t
fp_madd(sim_cpu *cpu,
        address_word cia,
        uint64_t op1,
        uint64_t op2,
        uint64_t op3,
        FP_formats fmt)
{
  return fp_mac(cpu, cia, &sim_fpu_add, op1, op2, op3, 0, 0, fmt);
}

uint64_t
fp_msub(sim_cpu *cpu,
        address_word cia,
        uint64_t op1,
        uint64_t op2,
        uint64_t op3,
        FP_formats fmt)
{
  return fp_mac(cpu, cia, &sim_fpu_sub, op1, op2, op3, 0, 0, fmt);
}

uint64_t
fp_fmadd (sim_cpu *cpu,
          address_word cia,
          uint64_t op1,
          uint64_t op2,
          uint64_t op3,
          FP_formats fmt)
{
  return fp_fmac (cpu, cia, &sim_fpu_add, op1, op2, op3, fmt);
}

uint64_t
fp_fmsub (sim_cpu *cpu,
          address_word cia,
          uint64_t op1,
          uint64_t op2,
          uint64_t op3,
          FP_formats fmt)
{
  return fp_fmac (cpu, cia, &sim_fpu_sub, op1, op2, op3, fmt);
}

uint64_t
fp_nmadd(sim_cpu *cpu,
         address_word cia,
         uint64_t op1,
         uint64_t op2,
         uint64_t op3,
         FP_formats fmt)
{
  return fp_mac(cpu, cia, &sim_fpu_add, op1, op2, op3, 0, 1, fmt);
}

uint64_t
fp_nmsub(sim_cpu *cpu,
         address_word cia,
         uint64_t op1,
         uint64_t op2,
         uint64_t op3,
         FP_formats fmt)
{
  return fp_mac(cpu, cia, &sim_fpu_sub, op1, op2, op3, 0, 1, fmt);
}


/* MIPS-3D ASE operations.  */

/* Variant of fp_binary for *r.ps MIPS-3D operations. */
static uint64_t
fp_binary_r(sim_cpu *cpu,
	    address_word cia,
	    int (*sim_fpu_op)(sim_fpu *, const sim_fpu *, const sim_fpu *),
	    uint64_t op1,
	    uint64_t op2)
{
  sim_fpu wop1;
  sim_fpu wop2;
  sim_fpu ans;
  sim_fpu_round round = rounding_mode (GETRM ());
  sim_fpu_denorm denorm = denorm_mode (cpu);
  sim_fpu_status status_u, status_l;
  uint64_t result;
  uint32_t res_u, res_l;

  /* The format must be fmt_ps.  */
  status_u = 0;
  sim_fpu_32to (&wop1, FP_PS_upper (op1));
  sim_fpu_32to (&wop2, FP_PS_lower (op1));
  status_u |= (*sim_fpu_op) (&ans, &wop1, &wop2);
  status_u |= sim_fpu_round_32 (&ans, round, denorm);
  sim_fpu_to32 (&res_u, &ans);
  status_l = 0;
  sim_fpu_32to (&wop1, FP_PS_upper (op2));
  sim_fpu_32to (&wop2, FP_PS_lower (op2));
  status_l |= (*sim_fpu_op) (&ans, &wop1, &wop2);
  status_l |= sim_fpu_round_32 (&ans, round, denorm);
  sim_fpu_to32 (&res_l, &ans);
  result = FP_PS_cat (res_u, res_l);

  update_fcsr (cpu, cia, status_u | status_l);
  return result;
}

uint64_t
fp_add_r(sim_cpu *cpu,
         address_word cia,
         uint64_t op1,
         uint64_t op2,
         FP_formats fmt)
{
  return fp_binary_r (cpu, cia, &sim_fpu_add, op1, op2);
}

uint64_t
fp_mul_r(sim_cpu *cpu,
         address_word cia,
         uint64_t op1,
         uint64_t op2,
         FP_formats fmt)
{
  return fp_binary_r (cpu, cia, &sim_fpu_mul, op1, op2);
}

#define NR_FRAC_GUARD   (60)
#define IMPLICIT_1 LSBIT64 (NR_FRAC_GUARD)

static int
fpu_inv1(sim_fpu *f, const sim_fpu *l)
{
  static const sim_fpu sim_fpu_one = {
    sim_fpu_class_number, 0, IMPLICIT_1, 0
  };
  int  status = 0;

  if (sim_fpu_is_zero (l))
    {
      *f = sim_fpu_maxfp;
      f->sign = l->sign;
      return sim_fpu_status_invalid_div0;
    }
  if (sim_fpu_is_infinity (l))
    {
      *f = sim_fpu_zero;
      f->sign = l->sign;
      return status;
    }
  status |= sim_fpu_div (f, &sim_fpu_one, l);
  return status;
}

static int
fpu_inv1_32(sim_fpu *f, const sim_fpu *l)
{
  if (sim_fpu_is_zero (l))
    {
      *f = sim_fpu_max32;
      f->sign = l->sign;
      return sim_fpu_status_invalid_div0;
    }
  return fpu_inv1 (f, l);
}

static int
fpu_inv1_64(sim_fpu *f, const sim_fpu *l)
{
  if (sim_fpu_is_zero (l))
    {
      *f = sim_fpu_max64;
      f->sign = l->sign;
      return sim_fpu_status_invalid_div0;
    }
  return fpu_inv1 (f, l);
}

uint64_t
fp_recip1(sim_cpu *cpu,
          address_word cia,
          uint64_t op,
          FP_formats fmt)
{
  switch (fmt)
    {
    case fmt_single:
    case fmt_ps:
      return fp_unary (cpu, cia, &fpu_inv1_32, op, fmt);
    case fmt_double:
      return fp_unary (cpu, cia, &fpu_inv1_64, op, fmt);
    }
  return 0;
}

uint64_t
fp_recip2(sim_cpu *cpu,
          address_word cia,
          uint64_t op1,
          uint64_t op2,
          FP_formats fmt)
{
  static const uint64_t one_single = UNSIGNED64 (0x3F800000);
  static const uint64_t one_double = UNSIGNED64 (0x3FF0000000000000);
  static const uint64_t one_ps = (UNSIGNED64 (0x3F800000) << 32 | UNSIGNED64 (0x3F800000));
  uint64_t one;

  /* Implemented as nmsub fd, 1, fs, ft.  */
  switch (fmt)
    {
    case fmt_single:  one = one_single;  break;
    case fmt_double:  one = one_double;  break;
    case fmt_ps:      one = one_ps;      break;
    default:          one = 0;           abort ();
    }
  return fp_mac (cpu, cia, &sim_fpu_sub, op1, op2, one, 0, 1, fmt);
}

static int
fpu_inv_sqrt1(sim_fpu *f, const sim_fpu *l)
{
  static const sim_fpu sim_fpu_one = {
    sim_fpu_class_number, 0, IMPLICIT_1, 0
  };
  int  status = 0;
  sim_fpu t;

  if (sim_fpu_is_zero (l))
    {
      *f = sim_fpu_maxfp;
      f->sign = l->sign;
      return sim_fpu_status_invalid_div0;
    }
  if (sim_fpu_is_infinity (l))
    {
      if (!l->sign)
	{
	  f->class = sim_fpu_class_zero;
	  f->sign = 0;
	}
      else
	{
	  *f = sim_fpu_qnan;
	  status = sim_fpu_status_invalid_sqrt;
	}
      return status;
    }
  status |= sim_fpu_sqrt (&t, l);
  status |= sim_fpu_div (f, &sim_fpu_one, &t);
  return status;
}

static int
fpu_inv_sqrt1_32(sim_fpu *f, const sim_fpu *l)
{
  if (sim_fpu_is_zero (l))
    {
      *f = sim_fpu_max32;
      f->sign = l->sign;
      return sim_fpu_status_invalid_div0;
    }
  return fpu_inv_sqrt1 (f, l);
}

static int
fpu_inv_sqrt1_64(sim_fpu *f, const sim_fpu *l)
{
  if (sim_fpu_is_zero (l))
    {
      *f = sim_fpu_max64;
      f->sign = l->sign;
      return sim_fpu_status_invalid_div0;
    }
  return fpu_inv_sqrt1 (f, l);
}

uint64_t
fp_rsqrt1(sim_cpu *cpu,
          address_word cia,
          uint64_t op,
          FP_formats fmt)
{
  switch (fmt)
    {
    case fmt_single:
    case fmt_ps:
      return fp_unary (cpu, cia, &fpu_inv_sqrt1_32, op, fmt);
    case fmt_double:
      return fp_unary (cpu, cia, &fpu_inv_sqrt1_64, op, fmt);
    }
  return 0;
}

uint64_t
fp_rsqrt2(sim_cpu *cpu,
          address_word cia,
          uint64_t op1,
          uint64_t op2,
          FP_formats fmt)
{
  static const uint64_t half_single = UNSIGNED64 (0x3F000000);
  static const uint64_t half_double = UNSIGNED64 (0x3FE0000000000000);
  static const uint64_t half_ps = (UNSIGNED64 (0x3F000000) << 32 | UNSIGNED64 (0x3F000000));
  uint64_t half;

  /* Implemented as (nmsub fd, 0.5, fs, ft)/2, where the divide is
     done by scaling the exponent during multiply.  */
  switch (fmt)
    {
    case fmt_single:  half = half_single;  break;
    case fmt_double:  half = half_double;  break;
    case fmt_ps:      half = half_ps;      break;
    default:          half = 0;            abort ();
    }
  return fp_mac (cpu, cia, &sim_fpu_sub, op1, op2, half, -1, 1, fmt);
}


/* Conversion operations.  */

uword64
convert (sim_cpu *cpu,
	 address_word cia,
	 int rm,
	 uword64 op,
	 FP_formats from,
	 FP_formats to)
{
  sim_fpu wop;
  sim_fpu_round round = rounding_mode (rm);
  sim_fpu_denorm denorm = denorm_mode (cpu);
  uint32_t result32;
  uint64_t result64;
  sim_fpu_status status = 0;

  /* Convert the input to sim_fpu internal format */
  switch (from)
    {
    case fmt_double:
      sim_fpu_64to (&wop, op);
      break;
    case fmt_single:
      sim_fpu_32to (&wop, op);
      break;
    case fmt_word:
      status = sim_fpu_i32to (&wop, op, round);
      break;
    case fmt_long:
      status = sim_fpu_i64to (&wop, op, round);
      break;
    default:
      sim_io_error (SD, "Bad switch\n");
    }

  /* Convert sim_fpu format into the output */
  /* The value WOP is converted to the destination format, rounding
     using mode RM. When the destination is a fixed-point format, then
     a source value of Infinity, NaN or one which would round to an
     integer outside the fixed point range then an IEEE Invalid Operation
     condition is raised.  Not used if destination format is PS.  */
  switch (to)
    {
    case fmt_single:
      status |= sim_fpu_round_32 (&wop, round, denorm);
      /* For a NaN, normalize mantissa bits (cvt.s.d can't preserve them) */
      if (sim_fpu_is_qnan (&wop))
	wop = sim_fpu_qnan;
      sim_fpu_to32 (&result32, &wop);
      result64 = result32;
      break;
    case fmt_double:
      status |= sim_fpu_round_64 (&wop, round, denorm);
      /* For a NaN, normalize mantissa bits (make cvt.d.s consistent) */
      if (sim_fpu_is_qnan (&wop))
	wop = sim_fpu_qnan;
      sim_fpu_to64 (&result64, &wop);
      break;
    case fmt_word:
      status |= sim_fpu_to32u (&result32, &wop, round);
      result64 = result32;
      break;
    case fmt_long:
      status |= sim_fpu_to64u (&result64, &wop, round);
      break;
    default:
      result64 = 0;
      sim_io_error (SD, "Bad switch\n");
    }

  update_fcsr (cpu, cia, status);
  return result64;
}

uint64_t
ps_lower(sim_cpu *cpu,
         address_word cia,
         uint64_t op)
{
  return FP_PS_lower (op);
}

uint64_t
ps_upper(sim_cpu *cpu,
         address_word cia,
         uint64_t op)
{
  return FP_PS_upper(op);
}

uint64_t
pack_ps(sim_cpu *cpu,
        address_word cia,
        uint64_t op1,
        uint64_t op2,
        FP_formats fmt)
{
  uint64_t result = 0;

  /* The registers must specify FPRs valid for operands of type
     "fmt". If they are not valid, the result is undefined. */

  /* The format type should already have been checked: */
  switch (fmt)
    {
    case fmt_single:
      {
	sim_fpu wop;
	uint32_t res_u, res_l;
	sim_fpu_32to (&wop, op1);
	sim_fpu_to32 (&res_u, &wop);
	sim_fpu_32to (&wop, op2);
	sim_fpu_to32 (&res_l, &wop);
	result = FP_PS_cat(res_u, res_l);
	break;
      }
    default:
      sim_io_error (SD, "Bad switch\n");
    }

  return result;
}

uint64_t
convert_ps (sim_cpu *cpu,
            address_word cia,
            int rm,
            uint64_t op,
            FP_formats from,
            FP_formats to)
{
  sim_fpu wop_u, wop_l;
  sim_fpu_round round = rounding_mode (rm);
  sim_fpu_denorm denorm = denorm_mode (cpu);
  uint32_t res_u, res_l;
  uint64_t result;
  sim_fpu_status status_u = 0, status_l = 0;

  /* As convert, but used only for paired values (formats PS, PW) */

  /* Convert the input to sim_fpu internal format */
  switch (from)
    {
    case fmt_word:   /* fmt_pw */
      sim_fpu_i32to (&wop_u, (op >> 32) & (unsigned)0xFFFFFFFF, round);
      sim_fpu_i32to (&wop_l, op & (unsigned)0xFFFFFFFF, round);
      break;
    case fmt_ps:
      sim_fpu_32to (&wop_u, FP_PS_upper(op));
      sim_fpu_32to (&wop_l, FP_PS_lower(op));
      break;
    default:
      sim_io_error (SD, "Bad switch\n");
    }

  /* Convert sim_fpu format into the output */
  switch (to)
    {
    case fmt_word:   /* fmt_pw */
      status_u |= sim_fpu_to32u (&res_u, &wop_u, round);
      status_l |= sim_fpu_to32u (&res_l, &wop_l, round);
      result = (((uint64_t)res_u) << 32) | (uint64_t)res_l;
      break;
    case fmt_ps:
      status_u |= sim_fpu_round_32 (&wop_u, round, denorm);
      status_l |= sim_fpu_round_32 (&wop_l, round, denorm);
      sim_fpu_to32 (&res_u, &wop_u);
      sim_fpu_to32 (&res_l, &wop_l);
      result = FP_PS_cat(res_u, res_l);
      break;
    default:
      result = 0;
      sim_io_error (SD, "Bad switch\n");
    }

  update_fcsr (cpu, cia, status_u | status_l);
  return result;
}

static const char *
fpu_format_name (FP_formats fmt)
{
  switch (fmt)
    {
    case fmt_single:
      return "single";
    case fmt_double:
      return "double";
    case fmt_word:
      return "word";
    case fmt_long:
      return "long";
    case fmt_ps:
      return "ps";
    case fmt_unknown:
      return "<unknown>";
    case fmt_uninterpreted:
      return "<uninterpreted>";
    case fmt_uninterpreted_32:
      return "<uninterpreted_32>";
    case fmt_uninterpreted_64:
      return "<uninterpreted_64>";
    default:
      return "<format error>";
    }
}

#ifdef DEBUG
static const char *
fpu_rounding_mode_name (int rm)
{
  switch (rm)
    {
    case FP_RM_NEAREST:
      return "Round";
    case FP_RM_TOZERO:
      return "Trunc";
    case FP_RM_TOPINF:
      return "Ceil";
    case FP_RM_TOMINF:
      return "Floor";
    default:
      return "<rounding mode error>";
    }
}
#endif /* DEBUG */
