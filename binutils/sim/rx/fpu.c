/* fpu.c --- FPU emulator for stand-alone RX simulator.

Copyright (C) 2008-2024 Free Software Foundation, Inc.
Contributed by Red Hat, Inc.

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

/* This must come before any other includes.  */
#include "defs.h"

#include <stdio.h>
#include <stdlib.h>

#include "cpu.h"
#include "fpu.h"

/* FPU encodings are as follows:

   S EXPONENT MANTISSA
   1 12345678 12345678901234567890123

   0 00000000 00000000000000000000000	+0
   1 00000000 00000000000000000000000	-0

   X 00000000 00000000000000000000001	Denormals
   X 00000000 11111111111111111111111
 
   X 00000001 XXXXXXXXXXXXXXXXXXXXXXX	Normals
   X 11111110 XXXXXXXXXXXXXXXXXXXXXXX

   0 11111111 00000000000000000000000	+Inf
   1 11111111 00000000000000000000000	-Inf

   X 11111111 0XXXXXXXXXXXXXXXXXXXXXX	SNaN (X != 0)
   X 11111111 1XXXXXXXXXXXXXXXXXXXXXX	QNaN (X != 0)

*/

#define trace 0
#define tprintf if (trace) printf

/* Some magic numbers.  */
#define PLUS_MAX   0x7f7fffffUL
#define MINUS_MAX  0xff7fffffUL
#define PLUS_INF   0x7f800000UL
#define MINUS_INF  0xff800000UL
#define PLUS_ZERO  0x00000000UL
#define MINUS_ZERO 0x80000000UL

#define FP_RAISE(e) fp_raise(FPSWBITS_C##e)
static void
fp_raise (int mask)
{
  regs.r_fpsw |= mask;
  if (mask != FPSWBITS_CE)
    {
      if (regs.r_fpsw & (mask << FPSW_CESH))
	regs.r_fpsw |= (mask << FPSW_CFSH);
      if (regs.r_fpsw & FPSWBITS_FMASK)
	regs.r_fpsw |= FPSWBITS_FSUM;
      else
	regs.r_fpsw &= ~FPSWBITS_FSUM;
    }
}

/* We classify all numbers as one of these.  They correspond to the
   rows/colums in the exception tables.  */
typedef enum {
  FP_NORMAL,
  FP_PZERO,
  FP_NZERO,
  FP_PINFINITY,
  FP_NINFINITY,
  FP_DENORMAL,
  FP_QNAN,
  FP_SNAN
} FP_Type;

#if defined DEBUG0
static const char *fpt_names[] = {
  "Normal", "+0", "-0", "+Inf", "-Inf", "Denormal", "QNaN", "SNaN"
};
#endif

#define EXP_BIAS  127
#define EXP_ZERO -127
#define EXP_INF   128

#define MANT_BIAS 0x00080000UL

typedef struct {
  int exp;
  unsigned int mant; /* 24 bits */
  char type;
  char sign;
  fp_t orig_value;
} FP_Parts;

static void
fp_explode (fp_t f, FP_Parts *p)
{
  int exp, mant, sign;

  exp = ((f & 0x7f800000UL) >> 23);
  mant = f & 0x007fffffUL;
  sign = f & 0x80000000UL;
  /*printf("explode: %08x %x %2x %6x\n", f, sign, exp, mant);*/

  p->sign = sign ? -1 : 1;
  p->exp = exp - EXP_BIAS;
  p->orig_value = f;
  p->mant = mant | 0x00800000UL;

  if (p->exp == EXP_ZERO)
    {
      if (regs.r_fpsw & FPSWBITS_DN)
	mant = 0;
      if (mant)
	p->type = FP_DENORMAL;
      else
	{
	  p->mant = 0;
	  p->type = sign ? FP_NZERO : FP_PZERO;
	}
    }
  else if (p->exp == EXP_INF)
    {
      if (mant == 0)
	p->type = sign ? FP_NINFINITY : FP_PINFINITY;
      else if (mant & 0x00400000UL)
	p->type = FP_QNAN;
      else
	p->type = FP_SNAN;
    }
  else
    p->type = FP_NORMAL;
}

static fp_t
fp_implode (FP_Parts *p)
{
  int exp, mant;

  exp = p->exp + EXP_BIAS;
  mant = p->mant;
  /*printf("implode: exp %d mant 0x%x\n", exp, mant);*/
  if (p->type == FP_NORMAL)
    {
      while (mant
	     && exp > 0
	     && mant < 0x00800000UL)
	{
	  mant <<= 1;
	  exp --;
	}
      while (mant > 0x00ffffffUL)
	{
	  mant >>= 1;
	  exp ++;
	}
      if (exp < 0)
	{
	  /* underflow */
	  exp = 0;
	  mant = 0;
	  FP_RAISE (E);
	}
      if (exp >= 255)
	{
	  /* overflow */
	  exp = 255;
	  mant = 0;
	  FP_RAISE (O);
	}
    }
  mant &= 0x007fffffUL;
  exp &= 0xff;
  mant |= exp << 23;
  if (p->sign < 0)
    mant |= 0x80000000UL;

  return mant;
}

typedef union {
  unsigned long long ll;
  double d;
} U_d_ll;

static int checked_format = 0;

/* We assume a double format like this:
   S[1] E[11] M[52]
*/

static double
fp_to_double (FP_Parts *p)
{
  U_d_ll u;

  if (!checked_format)
    {
      u.d = 1.5;
      if (u.ll != 0x3ff8000000000000ULL)
	abort ();
      u.d = -225;
      if (u.ll != 0xc06c200000000000ULL)
	abort ();
      u.d = 10.1;
      if (u.ll != 0x4024333333333333ULL)
	abort ();
      checked_format = 1;
    }

  u.ll = 0;
  if (p->sign < 0)
    u.ll |= (1ULL << 63);
  /* Make sure a zero encoding stays a zero.  */
  if (p->exp != -EXP_BIAS)
    u.ll |= ((unsigned long long)p->exp + 1023ULL) << 52;
  u.ll |= (unsigned long long) (p->mant & 0x007fffffUL) << (52 - 23);
  return u.d;
}

static void
double_to_fp (double d, FP_Parts *p)
{
  int exp;
  U_d_ll u;
  int sign;

  u.d = d;

  sign = (u.ll & 0x8000000000000000ULL) ? 1 : 0;
  exp = u.ll >> 52;
  exp = (exp & 0x7ff);

  if (exp == 0)
    {
      /* A generated denormal should show up as an underflow, not
	 here.  */
      if (sign)
	fp_explode (MINUS_ZERO, p);
      else
	fp_explode (PLUS_ZERO, p);
      return;
    }

  exp = exp - 1023;
  if ((exp + EXP_BIAS) > 254)
    {
      FP_RAISE (O);
      switch (regs.r_fpsw & FPSWBITS_RM)
	{
	case FPRM_NEAREST:
	  if (sign)
	    fp_explode (MINUS_INF, p);
	  else
	    fp_explode (PLUS_INF, p);
	  break;
	case FPRM_ZERO:
	  if (sign)
	    fp_explode (MINUS_MAX, p);
	  else
	    fp_explode (PLUS_MAX, p);
	  break;
	case FPRM_PINF:
	  if (sign)
	    fp_explode (MINUS_MAX, p);
	  else
	    fp_explode (PLUS_INF, p);
	  break;
	case FPRM_NINF:
	  if (sign)
	    fp_explode (MINUS_INF, p);
	  else
	    fp_explode (PLUS_MAX, p);
	  break;
	}
      return;
    }
  if ((exp + EXP_BIAS) < 1)
    {
      if (sign)
	fp_explode (MINUS_ZERO, p);
      else
	fp_explode (PLUS_ZERO, p);
      FP_RAISE (U);
    }

  p->sign = sign ? -1 : 1;
  p->exp = exp;
  p->mant = u.ll >> (52-23) & 0x007fffffUL;
  p->mant |= 0x00800000UL;
  p->type = FP_NORMAL;

  if (u.ll & 0x1fffffffULL)
    {
      switch (regs.r_fpsw & FPSWBITS_RM)
	{
	case FPRM_NEAREST:
	  if (u.ll & 0x10000000ULL)
	    p->mant ++;
	  break;
	case FPRM_ZERO:
	  break;
	case FPRM_PINF:
	  if (sign == 1)
	    p->mant ++;
	  break;
	case FPRM_NINF:
	  if (sign == -1)
	    p->mant ++;
	  break;
	}
      FP_RAISE (X);
    }

}

typedef enum {
  eNR,		/* Use the normal result.  */
  ePZ, eNZ,	/* +- zero */
  eSZ,		/* signed zero - XOR signs of ops together.  */
  eRZ,		/* +- zero depending on rounding mode.  */
  ePI, eNI,	/* +- Infinity */
  eSI,		/* signed infinity - XOR signs of ops together.  */
  eQN, eSN,	/* Quiet/Signalling NANs */
  eIn,		/* Invalid.  */
  eUn,		/* Unimplemented.  */
  eDZ,		/* Divide-by-zero.  */
  eLT,		/* less than */
  eGT,		/* greater than */
  eEQ,		/* equal to */
} FP_ExceptionCases;

#if defined DEBUG0
static const char *ex_names[] = {
  "NR", "PZ", "NZ", "SZ", "RZ", "PI", "NI", "SI", "QN", "SN", "IN", "Un", "DZ", "LT", "GT", "EQ"
};
#endif

/* This checks for all exceptional cases (not all FP exceptions) and
   returns TRUE if it is providing the result in *c.  If it returns
   FALSE, the caller should do the "normal" operation.  */
static int
check_exceptions (FP_Parts *a, FP_Parts *b, fp_t *c,
		  FP_ExceptionCases ex_tab[5][5], 
		  FP_ExceptionCases *case_ret)
{
  FP_ExceptionCases fpec;

  if (a->type == FP_SNAN
      || b->type == FP_SNAN)
    fpec = eIn;
  else if (a->type == FP_QNAN
	   || b->type == FP_QNAN)
    fpec = eQN;
  else if (a->type == FP_DENORMAL
	   || b->type == FP_DENORMAL)
    fpec = eUn;
  else
    fpec = ex_tab[(int)(a->type)][(int)(b->type)];

  /*printf("%s %s -> %s\n", fpt_names[(int)(a->type)], fpt_names[(int)(b->type)], ex_names[(int)(fpec)]);*/

  if (case_ret)
    *case_ret = fpec;

  switch (fpec)
    {
    case eNR:	/* Use the normal result.  */
      return 0;

    case ePZ:	/* + zero */
      *c = 0x00000000;
      return 1;

    case eNZ:	/* - zero */
      *c = 0x80000000;
      return 1;

    case eSZ:	/* signed zero */
      *c = (a->sign == b->sign) ? PLUS_ZERO : MINUS_ZERO;
      return 1;

    case eRZ:	/* +- zero depending on rounding mode.  */
      if ((regs.r_fpsw & FPSWBITS_RM) == FPRM_NINF)
	*c = 0x80000000;
      else
	*c = 0x00000000;
      return 1;

    case ePI:	/* + Infinity */
      *c = 0x7F800000;
      return 1;

    case eNI:	/* - Infinity */
      *c = 0xFF800000;
      return 1;

    case eSI:	/* sign Infinity */
      *c = (a->sign == b->sign) ? PLUS_INF : MINUS_INF;
      return 1;

    case eQN:	/* Quiet NANs */
      if (a->type == FP_QNAN)
	*c = a->orig_value;
      else
	*c = b->orig_value;
      return 1;

    case eSN:	/* Signalling NANs */
      if (a->type == FP_SNAN)
	*c = a->orig_value;
      else
	*c = b->orig_value;
      FP_RAISE (V);
      return 1;

    case eIn:	/* Invalid.  */
      FP_RAISE (V);
      if (a->type == FP_SNAN)
	*c = a->orig_value | 0x00400000;
      else if  (b->type == FP_SNAN)
	*c = b->orig_value | 0x00400000;
      else
	*c = 0x7fc00000;
      return 1;

    case eUn:	/* Unimplemented.  */
      FP_RAISE (E);
      return 1;

    case eDZ:	/* Division-by-zero.  */
      *c = (a->sign == b->sign) ? PLUS_INF : MINUS_INF;
      FP_RAISE (Z);
      return 1;

    default:
      return 0;
    }
}

#define CHECK_EXCEPTIONS(FPPa, FPPb, fpc, ex_tab) \
  if (check_exceptions (&FPPa, &FPPb, &fpc, ex_tab, 0))	\
    return fpc;

/* For each operation, we have two tables of how nonnormal cases are
   handled.  The DN=0 case is first, followed by the DN=1 case, with
   each table using the following layout: */

static FP_ExceptionCases ex_add_tab[5][5] = {
  /* N   +0   -0   +In  -In */
  { eNR, eNR, eNR, ePI, eNI }, /* Normal */
  { eNR, ePZ, eRZ, ePI, eNI }, /* +0   */
  { eNR, eRZ, eNZ, ePI, eNI }, /* -0   */
  { ePI, ePI, ePI, ePI, eIn }, /* +Inf */
  { eNI, eNI, eNI, eIn, eNI }, /* -Inf */
};

fp_t
rxfp_add (fp_t fa, fp_t fb)
{
  FP_Parts a, b, c;
  fp_t rv;
  double da, db;

  fp_explode (fa, &a);
  fp_explode (fb, &b);
  CHECK_EXCEPTIONS (a, b, rv, ex_add_tab);

  da = fp_to_double (&a);
  db = fp_to_double (&b);
  tprintf("%g + %g = %g\n", da, db, da+db);

  double_to_fp (da+db, &c);
  rv = fp_implode (&c);
  return rv;
}

static FP_ExceptionCases ex_sub_tab[5][5] = {
  /* N   +0   -0   +In  -In */
  { eNR, eNR, eNR, eNI, ePI }, /* Normal */
  { eNR, eRZ, ePZ, eNI, ePI }, /* +0   */
  { eNR, eNZ, eRZ, eNI, ePI }, /* -0   */
  { ePI, ePI, ePI, eIn, ePI }, /* +Inf */
  { eNI, eNI, eNI, eNI, eIn }, /* -Inf */
};

fp_t
rxfp_sub (fp_t fa, fp_t fb)
{
  FP_Parts a, b, c;
  fp_t rv;
  double da, db;

  fp_explode (fa, &a);
  fp_explode (fb, &b);
  CHECK_EXCEPTIONS (a, b, rv, ex_sub_tab);

  da = fp_to_double (&a);
  db = fp_to_double (&b);
  tprintf("%g - %g = %g\n", da, db, da-db);

  double_to_fp (da-db, &c);
  rv = fp_implode (&c);

  return rv;
}

static FP_ExceptionCases ex_mul_tab[5][5] = {
  /* N   +0   -0   +In  -In */
  { eNR, eNR, eNR, eSI, eSI }, /* Normal */
  { eNR, ePZ, eNZ, eIn, eIn }, /* +0   */
  { eNR, eNZ, ePZ, eIn, eIn }, /* -0   */
  { eSI, eIn, eIn, ePI, eNI }, /* +Inf */
  { eSI, eIn, eIn, eNI, ePI }, /* -Inf */
};

fp_t
rxfp_mul (fp_t fa, fp_t fb)
{
  FP_Parts a, b, c;
  fp_t rv;
  double da, db;

  fp_explode (fa, &a);
  fp_explode (fb, &b);
  CHECK_EXCEPTIONS (a, b, rv, ex_mul_tab);

  da = fp_to_double (&a);
  db = fp_to_double (&b);
  tprintf("%g x %g = %g\n", da, db, da*db);

  double_to_fp (da*db, &c);
  rv = fp_implode (&c);

  return rv;
}

static FP_ExceptionCases ex_div_tab[5][5] = {
  /* N   +0   -0   +In  -In */
  { eNR, eDZ, eDZ, eSZ, eSZ }, /* Normal */
  { eSZ, eIn, eIn, ePZ, eNZ }, /* +0   */
  { eSZ, eIn, eIn, eNZ, ePZ }, /* -0   */
  { eSI, ePI, eNI, eIn, eIn }, /* +Inf */
  { eSI, eNI, ePI, eIn, eIn }, /* -Inf */
};

fp_t
rxfp_div (fp_t fa, fp_t fb)
{
  FP_Parts a, b, c;
  fp_t rv;
  double da, db;

  fp_explode (fa, &a);
  fp_explode (fb, &b);
  CHECK_EXCEPTIONS (a, b, rv, ex_div_tab);

  da = fp_to_double (&a);
  db = fp_to_double (&b);
  tprintf("%g / %g = %g\n", da, db, da/db);

  double_to_fp (da/db, &c);
  rv = fp_implode (&c);

  return rv;
}

static FP_ExceptionCases ex_cmp_tab[5][5] = {
  /* N   +0   -0   +In  -In */
  { eNR, eNR, eNR, eLT, eGT }, /* Normal */
  { eNR, eEQ, eEQ, eLT, eGT }, /* +0   */
  { eNR, eEQ, eEQ, eLT, eGT }, /* -0   */
  { eGT, eGT, eGT, eEQ, eGT }, /* +Inf */
  { eLT, eLT, eLT, eLT, eEQ }, /* -Inf */
};

void
rxfp_cmp (fp_t fa, fp_t fb)
{
  FP_Parts a, b;
  fp_t c;
  FP_ExceptionCases reason;
  int flags = 0;
  double da, db;

  fp_explode (fa, &a);
  fp_explode (fb, &b);

  if (check_exceptions (&a, &b, &c, ex_cmp_tab, &reason))
    {
      if (reason == eQN)
	{
	  /* Special case - incomparable.  */
	  set_flags (FLAGBIT_Z | FLAGBIT_S | FLAGBIT_O, FLAGBIT_O);
	  return;
	}
      return;
    }

  switch (reason)
    {
    case eEQ:
      flags = FLAGBIT_Z;
      break;
    case eLT:
      flags = FLAGBIT_S;
      break;
    case eGT:
      flags = 0;
      break;
    case eNR:
      da = fp_to_double (&a);
      db = fp_to_double (&b);
      tprintf("fcmp: %g cmp %g\n", da, db);
      if (da < db)
	flags = FLAGBIT_S;
      else if (da == db)
	flags = FLAGBIT_Z;
      else
	flags = 0;
      break;
    default:
      abort();
    }

  set_flags (FLAGBIT_Z | FLAGBIT_S | FLAGBIT_O, flags);
}

long
rxfp_ftoi (fp_t fa, int round_mode)
{
  FP_Parts a;
  fp_t rv;
  int sign;
  int whole_bits, frac_bits;

  fp_explode (fa, &a);
  sign = fa & 0x80000000UL;

  switch (a.type)
    {
    case FP_NORMAL:
      break;
    case FP_PZERO:
    case FP_NZERO:
      return 0;
    case FP_PINFINITY:
      FP_RAISE (V);
      return 0x7fffffffL;
    case FP_NINFINITY:
      FP_RAISE (V);
      return 0x80000000L;
    case FP_DENORMAL:
      FP_RAISE (E);
      return 0;
    case FP_QNAN:
    case FP_SNAN:
      FP_RAISE (V);
      return sign ? 0x80000000U : 0x7fffffff;
    }

  if (a.exp >= 31)
    {
      FP_RAISE (V);
      return sign ? 0x80000000U : 0x7fffffff;
    }

  a.exp -= 23;

  if (a.exp <= -25)
    {
      /* Less than 0.49999 */
      frac_bits = a.mant;
      whole_bits = 0;
    }
  else if (a.exp < 0)
    {
      frac_bits = a.mant << (32 + a.exp);
      whole_bits = a.mant >> (-a.exp);
    }
  else
    {
      frac_bits = 0;
      whole_bits = a.mant << a.exp;
    }

  if (frac_bits)
    {
      switch (round_mode & 3)
	{
	case FPRM_NEAREST:
	  if (frac_bits & 0x80000000UL)
	    whole_bits ++;
	  break;
	case FPRM_ZERO:
	  break;
	case FPRM_PINF:
	  if (!sign)
	    whole_bits ++;
	  break;
	case FPRM_NINF:
	  if (sign)
	    whole_bits ++;
	  break;
	}
    }

  rv = sign ? -whole_bits : whole_bits;
  
  return rv;
}

fp_t
rxfp_itof (long fa, int round_mode)
{
  fp_t rv;
  int sign = 0;
  unsigned int frac_bits;
  volatile unsigned int whole_bits;
  FP_Parts a = {0};

  if (fa == 0)
    return PLUS_ZERO;

  if (fa < 0)
    {
      fa = -fa;
      sign = 1;
      a.sign = -1;
    }
  else
    a.sign = 1;

  whole_bits = fa;
  a.exp = 31;

  while (! (whole_bits & 0x80000000UL))
    {
      a.exp --;
      whole_bits <<= 1;
    }
  frac_bits = whole_bits & 0xff;
  whole_bits = whole_bits >> 8;

  if (frac_bits)
    {
      /* We must round */
      switch (round_mode & 3)
	{
	case FPRM_NEAREST:
	  if (frac_bits & 0x80)
	    whole_bits ++;
	  break;
	case FPRM_ZERO:
	  break;
	case FPRM_PINF:
	  if (!sign)
	    whole_bits ++;
	  break;
	case FPRM_NINF:
	  if (sign)
	    whole_bits ++;
	  break;
	}
    }

  a.mant = whole_bits;
  if (whole_bits & 0xff000000UL)
    {
      a.mant >>= 1;
      a.exp ++;
    }

  rv = fp_implode (&a);
  return rv;
}

