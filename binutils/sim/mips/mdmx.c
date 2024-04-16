/* Simulation code for the MIPS MDMX ASE.
   Copyright (C) 2002-2024 Free Software Foundation, Inc.
   Contributed by Ed Satterthwaite and Chris Demetriou, of Broadcom
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

/* This must come before any other includes.  */
#include "defs.h"

#include <stdio.h>

#include "sim-main.h"

/* Within mdmx.c we refer to the sim_cpu directly. */
#define CPU cpu
#define SD  (CPU_STATE(CPU))

/* XXX FIXME: temporary hack while the impact of making unpredictable()
   a "normal" (non-igen) function is evaluated.  */
#undef Unpredictable
#define Unpredictable() unpredictable_action (cpu, cia)

/* MDMX Representations

   An 8-bit packed byte element (OB) is always unsigned.
   The 24-bit accumulators are signed and are represented as 32-bit
   signed values, which are reduced to 24-bit signed values prior to
   Round and Clamp operations.

   A 16-bit packed halfword element (QH) is always signed.
   The 48-bit accumulators are signed and are represented as 64-bit
   signed values, which are reduced to 48-bit signed values prior to
   Round and Clamp operations.

   The code below assumes a 2's-complement representation of signed
   quantities.  Care is required to clear extended sign bits when
   repacking fields.

   The code (and the code for arithmetic shifts in mips.igen) also makes
   the (not guaranteed portable) assumption that right shifts of signed
   quantities in C do sign extension.  */

typedef uint64_t unsigned48;
#define MASK48 (UNSIGNED64 (0xffffffffffff))

typedef uint32_t unsigned24;
#define MASK24 (UNSIGNED32 (0xffffff))

typedef enum {
  mdmx_ob,          /* OB (octal byte) */
  mdmx_qh           /* QH (quad half-word) */
} MX_fmt;

typedef enum {
  sel_elem,         /* element select */
  sel_vect,         /* vector select */
  sel_imm           /* immediate select */
} VT_select;

#define OB_MAX  ((uint8_t)0xFF)
#define QH_MIN  ((int16_t)0x8000)
#define QH_MAX  ((int16_t)0x7FFF)

#define OB_CLAMP(x)  ((uint8_t)((x) > OB_MAX ? OB_MAX : (x)))
#define QH_CLAMP(x)  ((int16_t)((x) < QH_MIN ? QH_MIN : \
                                ((x) > QH_MAX ? QH_MAX : (x))))

#define MX_FMT(fmtsel) (((fmtsel) & 0x1) == 0 ? mdmx_ob : mdmx_qh)
#define MX_VT(fmtsel)  (((fmtsel) & 0x10) == 0 ?    sel_elem : \
                       (((fmtsel) & 0x18) == 0x10 ? sel_vect : sel_imm))

#define QH_ELEM(v,fmtsel) \
        ((int16_t)(((v) >> (((fmtsel) & 0xC) << 2)) & 0xFFFF))
#define OB_ELEM(v,fmtsel) \
        ((uint8_t)(((v) >> (((fmtsel) & 0xE) << 2)) & 0xFF))


typedef int16_t (*QH_FUNC)(int16_t, int16_t);
typedef uint8_t (*OB_FUNC)(uint8_t, uint8_t);

/* vectorized logical operators */

static int16_t
AndQH(int16_t ts, int16_t tt)
{
  return (int16_t)((uint16_t)ts & (uint16_t)tt);
}

static uint8_t
AndOB(uint8_t ts, uint8_t tt)
{
  return ts & tt;
}

static int16_t
NorQH(int16_t ts, int16_t tt)
{
  return (int16_t)(((uint16_t)ts | (uint16_t)tt) ^ 0xFFFF);
}

static uint8_t
NorOB(uint8_t ts, uint8_t tt)
{
  return (ts | tt) ^ 0xFF;
}

static int16_t
OrQH(int16_t ts, int16_t tt)
{
  return (int16_t)((uint16_t)ts | (uint16_t)tt);
}

static uint8_t
OrOB(uint8_t ts, uint8_t tt)
{
  return ts | tt;
}

static int16_t
XorQH(int16_t ts, int16_t tt)
{
  return (int16_t)((uint16_t)ts ^ (uint16_t)tt);
}

static uint8_t
XorOB(uint8_t ts, uint8_t tt)
{
  return ts ^ tt;
}

static int16_t
SLLQH(int16_t ts, int16_t tt)
{
  uint32_t s = (uint32_t)tt & 0xF;
  return (int16_t)(((uint32_t)ts << s) & 0xFFFF);
}

static uint8_t
SLLOB(uint8_t ts, uint8_t tt)
{
  uint32_t s = tt & 0x7;
  return (ts << s) & 0xFF;
}

static int16_t
SRLQH(int16_t ts, int16_t tt)
{
  uint32_t s = (uint32_t)tt & 0xF;
  return (int16_t)((uint16_t)ts >> s);
}

static uint8_t
SRLOB(uint8_t ts, uint8_t tt)
{
  uint32_t s = tt & 0x7;
  return ts >> s;
}


/* Vectorized arithmetic operators.  */

static int16_t
AddQH(int16_t ts, int16_t tt)
{
  int32_t t = (int32_t)ts + (int32_t)tt;
  return QH_CLAMP(t);
}

static uint8_t
AddOB(uint8_t ts, uint8_t tt)
{
  uint32_t t = (uint32_t)ts + (uint32_t)tt;
  return OB_CLAMP(t);
}

static int16_t
SubQH(int16_t ts, int16_t tt)
{
  int32_t t = (int32_t)ts - (int32_t)tt;
  return QH_CLAMP(t);
}

static uint8_t
SubOB(uint8_t ts, uint8_t tt)
{
  int32_t t;
  t = (int32_t)ts - (int32_t)tt;
  if (t < 0)
    t = 0;
  return (uint8_t)t;
}

static int16_t
MinQH(int16_t ts, int16_t tt)
{
  return (ts < tt ? ts : tt);
}

static uint8_t
MinOB(uint8_t ts, uint8_t tt)
{
  return (ts < tt ? ts : tt);
}

static int16_t
MaxQH(int16_t ts, int16_t tt)
{
  return (ts > tt ? ts : tt);
}

static uint8_t
MaxOB(uint8_t ts, uint8_t tt)
{
  return (ts > tt ? ts : tt);
}

static int16_t
MulQH(int16_t ts, int16_t tt)
{
  int32_t t = (int32_t)ts * (int32_t)tt;
  return QH_CLAMP(t);
}

static uint8_t
MulOB(uint8_t ts, uint8_t tt)
{
  uint32_t t = (uint32_t)ts * (uint32_t)tt;
  return OB_CLAMP(t);
}

/* "msgn" and "sra" are defined only for QH format.  */

static int16_t
MsgnQH(int16_t ts, int16_t tt)
{
  int16_t t;
  if (ts < 0)
    t = (tt == QH_MIN ? QH_MAX : -tt);
  else if (ts == 0)
    t = 0;
  else
    t = tt;
  return t;
}

static int16_t
SRAQH(int16_t ts, int16_t tt)
{
  uint32_t s = (uint32_t)tt & 0xF;
  return (int16_t)((int32_t)ts >> s);
}


/* "pabsdiff" and "pavg" are defined only for OB format.  */

static uint8_t
AbsDiffOB(uint8_t ts, uint8_t tt)
{
  return (ts >= tt ? ts - tt : tt - ts);
}

static uint8_t
AvgOB(uint8_t ts, uint8_t tt)
{
  return ((uint32_t)ts + (uint32_t)tt + 1) >> 1;
}


/* Dispatch tables for operations that update a CPR.  */

static const QH_FUNC qh_func[] = {
  AndQH,  NorQH,  OrQH,   XorQH, SLLQH, SRLQH,
  AddQH,  SubQH,  MinQH,  MaxQH,
  MulQH,  MsgnQH, SRAQH,  NULL,  NULL
};

static const OB_FUNC ob_func[] = {
  AndOB,  NorOB,  OrOB,   XorOB, SLLOB, SRLOB,
  AddOB,  SubOB,  MinOB,  MaxOB,
  MulOB,  NULL,   NULL,   AbsDiffOB, AvgOB
};

/* Auxiliary functions for CPR updates.  */

/* Vector mapping for QH format.  */
static uint64_t
qh_vector_op(uint64_t v1, uint64_t v2, QH_FUNC func)
{
  uint64_t result = 0;
  int  i;
  int16_t h, h1, h2;

  for (i = 0; i < 64; i += 16)
    {
      h1 = (int16_t)(v1 & 0xFFFF);  v1 >>= 16;
      h2 = (int16_t)(v2 & 0xFFFF);  v2 >>= 16;
      h = (*func)(h1, h2);
      result |= ((uint64_t)((uint16_t)h) << i);
    }
  return result;
}

static uint64_t
qh_map_op(uint64_t v1, int16_t h2, QH_FUNC func)
{
  uint64_t result = 0;
  int  i;
  int16_t h, h1;

  for (i = 0; i < 64; i += 16)
    {
      h1 = (int16_t)(v1 & 0xFFFF);  v1 >>= 16;
      h = (*func)(h1, h2);
      result |= ((uint64_t)((uint16_t)h) << i);
    }
  return result;
}


/* Vector operations for OB format.  */

static uint64_t
ob_vector_op(uint64_t v1, uint64_t v2, OB_FUNC func)
{
  uint64_t result = 0;
  int  i;
  uint8_t b, b1, b2;

  for (i = 0; i < 64; i += 8)
    {
      b1 = v1 & 0xFF;  v1 >>= 8;
      b2 = v2 & 0xFF;  v2 >>= 8;
      b = (*func)(b1, b2);
      result |= ((uint64_t)b << i);
    }
  return result;
}

static uint64_t
ob_map_op(uint64_t v1, uint8_t b2, OB_FUNC func)
{
  uint64_t result = 0;
  int  i;
  uint8_t b, b1;

  for (i = 0; i < 64; i += 8)
    {
      b1 = v1 & 0xFF;  v1 >>= 8;
      b = (*func)(b1, b2);
      result |= ((uint64_t)b << i);
    }
  return result;
}


/* Primary entry for operations that update CPRs.  */
uint64_t
mdmx_cpr_op(sim_cpu *cpu,
	    address_word cia,
	    int op,
	    uint64_t op1,
	    int vt,
	    MX_fmtsel fmtsel)
{
  uint64_t op2;
  uint64_t result = 0;

  switch (MX_FMT (fmtsel))
    {
    case mdmx_qh:
      switch (MX_VT (fmtsel))
	{
	case sel_elem:
	  op2 = ValueFPR(vt, fmt_mdmx);
	  result = qh_map_op(op1, QH_ELEM(op2, fmtsel), qh_func[op]);
	  break;
	case sel_vect:
	  result = qh_vector_op(op1, ValueFPR(vt, fmt_mdmx), qh_func[op]);
	  break;
	case sel_imm:
	  result = qh_map_op(op1, vt, qh_func[op]);
	  break;
	}
      break;
    case mdmx_ob:
      switch (MX_VT (fmtsel))
	{
	case sel_elem:
	  op2 = ValueFPR(vt, fmt_mdmx);
	  result = ob_map_op(op1, OB_ELEM(op2, fmtsel), ob_func[op]);
	  break;
	case sel_vect:
	  result = ob_vector_op(op1, ValueFPR(vt, fmt_mdmx), ob_func[op]);
	  break;
	case sel_imm:
	  result = ob_map_op(op1, vt, ob_func[op]);
	  break;
	}
      break;
    default:
      Unpredictable ();
    }

  return result;
}


/* Operations that update CCs */

static void
qh_vector_test(sim_cpu *cpu, uint64_t v1, uint64_t v2, int cond)
{
  int  i;
  int16_t h1, h2;
  int  boolean;

  for (i = 0; i < 4; i++)
    {
      h1 = (int16_t)(v1 & 0xFFFF);  v1 >>= 16;
      h2 = (int16_t)(v2 & 0xFFFF);  v2 >>= 16;
      boolean = ((cond & MX_C_EQ) && (h1 == h2)) ||
	((cond & MX_C_LT) && (h1 < h2));
      SETFCC(i, boolean);
    }
}

static void
qh_map_test(sim_cpu *cpu, uint64_t v1, int16_t h2, int cond)
{
  int  i;
  int16_t h1;
  int  boolean;

  for (i = 0; i < 4; i++)
    {
      h1 = (int16_t)(v1 & 0xFFFF);  v1 >>= 16;
      boolean = ((cond & MX_C_EQ) && (h1 == h2)) ||
	((cond & MX_C_LT) && (h1 < h2));
      SETFCC(i, boolean);
    }
}

static void
ob_vector_test(sim_cpu *cpu, uint64_t v1, uint64_t v2, int cond)
{
  int  i;
  uint8_t b1, b2;
  int  boolean;

  for (i = 0; i < 8; i++)
    {
      b1 = v1 & 0xFF;  v1 >>= 8;
      b2 = v2 & 0xFF;  v2 >>= 8;
      boolean = ((cond & MX_C_EQ) && (b1 == b2)) ||
	((cond & MX_C_LT) && (b1 < b2));
      SETFCC(i, boolean);
    }
}

static void
ob_map_test(sim_cpu *cpu, uint64_t v1, uint8_t b2, int cond)
{
  int  i;
  uint8_t b1;
  int  boolean;

  for (i = 0; i < 8; i++)
    {
      b1 = (uint8_t)(v1 & 0xFF);  v1 >>= 8;
      boolean = ((cond & MX_C_EQ) && (b1 == b2)) ||
	((cond & MX_C_LT) && (b1 < b2));
      SETFCC(i, boolean);
    }
}


void
mdmx_cc_op(sim_cpu *cpu,
	   address_word cia,
	   int cond,
	   uint64_t v1,
	   int vt,
	   MX_fmtsel fmtsel)
{
  uint64_t op2;

  switch (MX_FMT (fmtsel))
    {
    case mdmx_qh:
      switch (MX_VT (fmtsel))
	{
	case sel_elem:
	  op2 = ValueFPR(vt, fmt_mdmx);
	  qh_map_test(cpu, v1, QH_ELEM(op2, fmtsel), cond);
	  break;
	case sel_vect:
	  qh_vector_test(cpu, v1, ValueFPR(vt, fmt_mdmx), cond);
	  break;
	case sel_imm:
	  qh_map_test(cpu, v1, vt, cond);
	  break;
	}
      break;
    case mdmx_ob:
      switch (MX_VT (fmtsel))
	{
	case sel_elem:
	  op2 = ValueFPR(vt, fmt_mdmx);
	  ob_map_test(cpu, v1, OB_ELEM(op2, fmtsel), cond);
	  break;
	case sel_vect:
	  ob_vector_test(cpu, v1, ValueFPR(vt, fmt_mdmx), cond);
	  break;
	case sel_imm:
	  ob_map_test(cpu, v1, vt, cond);
	  break;
	}
      break;
    default:
      Unpredictable ();
    }
}


/* Pick operations.  */

static uint64_t
qh_vector_pick(sim_cpu *cpu, uint64_t v1, uint64_t v2, int tf)
{
  uint64_t result = 0;
  int  i, s;
  uint16_t h;

  s = 0;
  for (i = 0; i < 4; i++)
    {
      h = ((GETFCC(i) == tf) ? (v1 & 0xFFFF) : (v2 & 0xFFFF));
      v1 >>= 16;  v2 >>= 16;
      result |= ((uint64_t)h << s);
      s += 16;
    }
  return result;
}

static uint64_t
qh_map_pick(sim_cpu *cpu, uint64_t v1, int16_t h2, int tf)
{
  uint64_t result = 0;
  int  i, s;
  uint16_t h;

  s = 0;
  for (i = 0; i < 4; i++)
    {
      h = (GETFCC(i) == tf) ? (v1 & 0xFFFF) : (uint16_t)h2;
      v1 >>= 16;
      result |= ((uint64_t)h << s);
      s += 16;
    }
  return result;
}

static uint64_t
ob_vector_pick(sim_cpu *cpu, uint64_t v1, uint64_t v2, int tf)
{
  uint64_t result = 0;
  int  i, s;
  uint8_t b;

  s = 0;
  for (i = 0; i < 8; i++)
    {
      b = (GETFCC(i) == tf) ? (v1 & 0xFF) : (v2 & 0xFF);
      v1 >>= 8;  v2 >>= 8;
      result |= ((uint64_t)b << s);
      s += 8;
    }
  return result;
}

static uint64_t
ob_map_pick(sim_cpu *cpu, uint64_t v1, uint8_t b2, int tf)
{
  uint64_t result = 0;
  int  i, s;
  uint8_t b;

  s = 0;
  for (i = 0; i < 8; i++)
    {
      b = (GETFCC(i) == tf) ? (v1 & 0xFF) : b2;
      v1 >>= 8;
      result |= ((uint64_t)b << s);
      s += 8;
    }
  return result;
}


uint64_t
mdmx_pick_op(sim_cpu *cpu,
	     address_word cia,
	     int tf,
	     uint64_t v1,
	     int vt,
	     MX_fmtsel fmtsel)
{
  uint64_t result = 0;
  uint64_t op2;

  switch (MX_FMT (fmtsel))
    {
    case mdmx_qh:
      switch (MX_VT (fmtsel))
	{
	case sel_elem:
	  op2 = ValueFPR(vt, fmt_mdmx);
	  result = qh_map_pick(cpu, v1, QH_ELEM(op2, fmtsel), tf);
	  break;
	case sel_vect:
	  result = qh_vector_pick(cpu, v1, ValueFPR(vt, fmt_mdmx), tf);
	  break;
	case sel_imm:
	  result = qh_map_pick(cpu, v1, vt, tf);
	  break;
	}
      break;
    case mdmx_ob:
      switch (MX_VT (fmtsel))
	{
	case sel_elem:
	  op2 = ValueFPR(vt, fmt_mdmx);
	  result = ob_map_pick(cpu, v1, OB_ELEM(op2, fmtsel), tf);
	  break;
	case sel_vect:
	  result = ob_vector_pick(cpu, v1, ValueFPR(vt, fmt_mdmx), tf);
	  break;
	case sel_imm:
	  result = ob_map_pick(cpu, v1, vt, tf);
	  break;
	}
      break;
    default:
      Unpredictable ();
    }
  return result;
}


/* Accumulators.  */

typedef void (*QH_ACC)(signed48 *a, int16_t ts, int16_t tt);

static void
AccAddAQH(signed48 *a, int16_t ts, int16_t tt)
{
  *a += (signed48)ts + (signed48)tt;
}

static void
AccAddLQH(signed48 *a, int16_t ts, int16_t tt)
{
  *a = (signed48)ts + (signed48)tt;
}

static void
AccMulAQH(signed48 *a, int16_t ts, int16_t tt)
{
  *a += (signed48)ts * (signed48)tt;
}

static void
AccMulLQH(signed48 *a, int16_t ts, int16_t tt)
{
  *a = (signed48)ts * (signed48)tt;
}

static void
SubMulAQH(signed48 *a, int16_t ts, int16_t tt)
{
  *a -= (signed48)ts * (signed48)tt;
}

static void
SubMulLQH(signed48 *a, int16_t ts, int16_t tt)
{
  *a = -((signed48)ts * (signed48)tt);
}

static void
AccSubAQH(signed48 *a, int16_t ts, int16_t tt)
{
  *a += (signed48)ts - (signed48)tt;
}

static void
AccSubLQH(signed48 *a, int16_t ts, int16_t tt)
{
  *a =  (signed48)ts - (signed48)tt;
}


typedef void (*OB_ACC)(signed24 *acc, uint8_t ts, uint8_t tt);

static void
AccAddAOB(signed24 *a, uint8_t ts, uint8_t tt)
{
  *a += (signed24)ts + (signed24)tt;
}

static void
AccAddLOB(signed24 *a, uint8_t ts, uint8_t tt)
{
  *a = (signed24)ts + (signed24)tt;
}

static void
AccMulAOB(signed24 *a, uint8_t ts, uint8_t tt)
{
  *a += (signed24)ts * (signed24)tt;
}

static void
AccMulLOB(signed24 *a, uint8_t ts, uint8_t tt)
{
  *a = (signed24)ts * (signed24)tt;
}

static void
SubMulAOB(signed24 *a, uint8_t ts, uint8_t tt)
{
  *a -= (signed24)ts * (signed24)tt;
}

static void
SubMulLOB(signed24 *a, uint8_t ts, uint8_t tt)
{
  *a = -((signed24)ts * (signed24)tt);
}

static void
AccSubAOB(signed24 *a, uint8_t ts, uint8_t tt)
{
  *a += (signed24)ts - (signed24)tt;
}

static void
AccSubLOB(signed24 *a, uint8_t ts, uint8_t tt)
{
  *a = (signed24)ts - (signed24)tt;
}

static void
AccAbsDiffOB(signed24 *a, uint8_t ts, uint8_t tt)
{
  uint8_t t = (ts >= tt ? ts - tt : tt - ts);
  *a += (signed24)t;
}


/* Dispatch tables for operations that update a CPR.  */

static const QH_ACC qh_acc[] = {
  AccAddAQH, AccAddLQH, AccMulAQH, AccMulLQH,
  SubMulAQH, SubMulLQH, AccSubAQH, AccSubLQH,
  NULL
};

static const OB_ACC ob_acc[] = {
  AccAddAOB, AccAddLOB, AccMulAOB, AccMulLOB,
  SubMulAOB, SubMulLOB, AccSubAOB, AccSubLOB,
  AccAbsDiffOB
};


static void
qh_vector_acc(signed48 a[], uint64_t v1, uint64_t v2, QH_ACC acc)
{
  int  i;
  int16_t h1, h2;

  for (i = 0; i < 4; i++)
    {
      h1 = (int16_t)(v1 & 0xFFFF);  v1 >>= 16;
      h2 = (int16_t)(v2 & 0xFFFF);  v2 >>= 16;
      (*acc)(&a[i], h1, h2);
    }
}

static void
qh_map_acc(signed48 a[], uint64_t v1, int16_t h2, QH_ACC acc)
{
  int  i;
  int16_t h1;

  for (i = 0; i < 4; i++)
    {
      h1 = (int16_t)(v1 & 0xFFFF);  v1 >>= 16;
      (*acc)(&a[i], h1, h2);
    }
}

static void
ob_vector_acc(signed24 a[], uint64_t v1, uint64_t v2, OB_ACC acc)
{
  int  i;
  uint8_t  b1, b2;

  for (i = 0; i < 8; i++)
    {
      b1 = v1 & 0xFF;  v1 >>= 8;
      b2 = v2 & 0xFF;  v2 >>= 8;
      (*acc)(&a[i], b1, b2);
    }
}

static void
ob_map_acc(signed24 a[], uint64_t v1, uint8_t b2, OB_ACC acc)
{
  int  i;
  uint8_t b1;

  for (i = 0; i < 8; i++)
    {
      b1 = v1 & 0xFF;  v1 >>= 8;
      (*acc)(&a[i], b1, b2);
    }
}


/* Primary entry for operations that accumulate */
void
mdmx_acc_op(sim_cpu *cpu,
	    address_word cia,
	    int op,
	    uint64_t op1,
	    int vt,
	    MX_fmtsel fmtsel)
{
  uint64_t op2;

  switch (MX_FMT (fmtsel))
    {
    case mdmx_qh:
      switch (MX_VT (fmtsel))
	{
	case sel_elem:
	  op2 = ValueFPR(vt, fmt_mdmx);
	  qh_map_acc(ACC.qh, op1, QH_ELEM(op2, fmtsel), qh_acc[op]);
	  break;
	case sel_vect:
	  qh_vector_acc(ACC.qh, op1, ValueFPR(vt, fmt_mdmx), qh_acc[op]);
	  break;
	case sel_imm:
	  qh_map_acc(ACC.qh, op1, vt, qh_acc[op]);
	  break;
	}
      break;
    case mdmx_ob:
      switch (MX_VT (fmtsel))
	{
	case sel_elem:
	  op2 = ValueFPR(vt, fmt_mdmx);
	  ob_map_acc(ACC.ob, op1, OB_ELEM(op2, fmtsel), ob_acc[op]);
	  break;
	case sel_vect:
	  ob_vector_acc(ACC.ob, op1, ValueFPR(vt, fmt_mdmx), ob_acc[op]);
	  break;
	case sel_imm:
	  ob_map_acc(ACC.ob, op1, vt, ob_acc[op]);
	  break;
	}
      break;
    default:
      Unpredictable ();
    }
}


/* Reading and writing accumulator (no conversion).  */

uint64_t
mdmx_rac_op(sim_cpu *cpu,
	    address_word cia,
	    int op,
	    int fmt)
{
  uint64_t    result;
  unsigned int  shift;
  int           i;

  shift = op;          /* L = 00, M = 01, H = 10.  */
  result = 0;

  switch (fmt)
    {
    case MX_FMT_QH:
      shift <<= 4;              /* 16 bits per element.  */
      for (i = 3; i >= 0; --i)
	{
	  result <<= 16;
	  result |= ((ACC.qh[i] >> shift) & 0xFFFF);
	}
      break;
    case MX_FMT_OB:
      shift <<= 3;              /*  8 bits per element.  */
      for (i = 7; i >= 0; --i)
	{
	  result <<= 8;
	  result |= ((ACC.ob[i] >> shift) & 0xFF);
	}
      break;
    default:
      Unpredictable ();
    }
  return result;
}

void
mdmx_wacl(sim_cpu *cpu,
	  address_word cia,
	  int fmt,
	  uint64_t vs,
	  uint64_t vt)
{
  int           i;

  switch (fmt)
    {
    case MX_FMT_QH:
      for (i = 0; i < 4; i++)
	{
	  int32_t  s = (int16_t)(vs & 0xFFFF);
	  ACC.qh[i] = ((signed48)s << 16) | (vt & 0xFFFF);
	  vs >>= 16;  vt >>= 16;
	}
      break;
    case MX_FMT_OB:
      for (i = 0; i < 8; i++)
	{
	  int16_t  s = (int8_t)(vs & 0xFF);
	  ACC.ob[i] = ((signed24)s << 8) | (vt & 0xFF);
	  vs >>= 8;   vt >>= 8;
	}
      break;
    default:
      Unpredictable ();
    }
}

void
mdmx_wach(sim_cpu *cpu,
	  address_word cia,
	  int fmt,
	  uint64_t vs)
{
  int           i;

  switch (fmt)
    {
    case MX_FMT_QH:
      for (i = 0; i < 4; i++)
	{
	  int32_t  s = (int16_t)(vs & 0xFFFF);
	  ACC.qh[i] &= ~((signed48)0xFFFF << 32);
	  ACC.qh[i] |=  ((signed48)s << 32);
	  vs >>= 16;
	}
      break;
    case MX_FMT_OB:
      for (i = 0; i < 8; i++)
	{
	  ACC.ob[i] &= ~((signed24)0xFF << 16);
	  ACC.ob[i] |=  ((signed24)(vs & 0xFF) << 16);
	  vs >>= 8;
	}
      break;
    default:
      Unpredictable ();
    }
}


/* Reading and writing accumulator (rounding conversions).
   Enumerating function guarantees s >= 0 for QH ops.  */

typedef int16_t (*QH_ROUND)(signed48 a, int16_t s);

#define QH_BIT(n)  ((unsigned48)1 << (n))
#define QH_ONES(n) (((unsigned48)1 << (n))-1)

static int16_t
RNASQH(signed48 a, int16_t s)
{
  signed48 t;
  int16_t result = 0;

  if (s > 48)
    result = 0;
  else
    {
      t = (a >> s);
      if ((a & QH_BIT(47)) == 0)
	{
	  if (s > 0 && ((a >> (s-1)) & 1) == 1)
	    t++;
	  if (t > QH_MAX)
	    t = QH_MAX;
	}
      else
	{
	  if (s > 0 && ((a >> (s-1)) & 1) == 1)
	    {
	      if (s > 1 && ((unsigned48)a & QH_ONES(s-1)) != 0)
		t++;
	    }
	  if (t < QH_MIN)
	    t = QH_MIN;
	}
      result = (int16_t)t;
    }
  return result;
}

static int16_t
RNAUQH(signed48 a, int16_t s)
{
  unsigned48 t;
  int16_t result;

  if (s > 48)
    result = 0;
  else if (s == 48)
    result = ((unsigned48)a & MASK48) >> 47;
  else
    {
      t = ((unsigned48)a & MASK48) >> s;
      if (s > 0 && ((a >> (s-1)) & 1) == 1)
	t++;
      if (t > 0xFFFF)
	t = 0xFFFF;
      result = (int16_t)t;
    }
  return result;
}

static int16_t
RNESQH(signed48 a, int16_t s)
{
  signed48 t;
  int16_t result = 0;

  if (s > 47)
    result = 0;
  else
    {
      t = (a >> s);
      if (s > 0 && ((a >> (s-1)) & 1) == 1)
	{
	  if (s == 1 || (a & QH_ONES(s-1)) == 0)
	    t += t & 1;
	  else
	    t += 1;
	}
      if ((a & QH_BIT(47)) == 0)
	{
	  if (t > QH_MAX)
	    t = QH_MAX;
	}
      else
	{
	  if (t < QH_MIN)
	    t = QH_MIN;
	}
      result = (int16_t)t;
    }
  return result;
}

static int16_t
RNEUQH(signed48 a, int16_t s)
{
  unsigned48 t;
  int16_t result;

  if (s > 48)
    result = 0;
  else if (s == 48)
    result = ((unsigned48)a > QH_BIT(47) ? 1 : 0);
  else
    {
      t = ((unsigned48)a & MASK48) >> s;
      if (s > 0 && ((a >> (s-1)) & 1) == 1)
	{
	  if (s > 1 && (a & QH_ONES(s-1)) != 0)
	    t++;
	  else
	    t += t & 1;
	}
      if (t > 0xFFFF)
	t = 0xFFFF;
      result = (int16_t)t;
    }
  return result;
}

static int16_t
RZSQH(signed48 a, int16_t s)
{
  signed48 t;
  int16_t result = 0;

  if (s > 47)
    result = 0;
  else
    {
      t = (a >> s);
      if ((a & QH_BIT(47)) == 0)
	{
	  if (t > QH_MAX)
	    t = QH_MAX;
	}
      else
	{
	  if (t < QH_MIN)
	    t = QH_MIN;
	}
      result = (int16_t)t;
    }
  return result;
}

static int16_t
RZUQH(signed48 a, int16_t s)
{
  unsigned48 t;
  int16_t result = 0;

  if (s > 48)
    result = 0;
  else if (s == 48)
    result = ((unsigned48)a > QH_BIT(47) ? 1 : 0);
  else
    {
      t = ((unsigned48)a & MASK48) >> s;
      if (t > 0xFFFF)
	t = 0xFFFF;
      result = (int16_t)t;
    }
  return result;
}


typedef uint8_t (*OB_ROUND)(signed24 a, uint8_t s);

#define OB_BIT(n)  ((unsigned24)1 << (n))
#define OB_ONES(n) (((unsigned24)1 << (n))-1)

static uint8_t
RNAUOB(signed24 a, uint8_t s)
{
  uint8_t result;
  unsigned24 t;

  if (s > 24)
    result = 0;
  else if (s == 24)
    result = ((unsigned24)a & MASK24) >> 23;
  else
    {
      t = ((unsigned24)a & MASK24) >> s;
      if (s > 0 && ((a >> (s-1)) & 1) == 1)
	t ++;
      result = OB_CLAMP(t);
    }
  return result;
}

static uint8_t
RNEUOB(signed24 a, uint8_t s)
{
  uint8_t result;
  unsigned24 t;

  if (s > 24)
    result = 0;
  else if (s == 24)
    result = (((unsigned24)a & MASK24) > OB_BIT(23) ? 1 : 0);
  else
    {
      t = ((unsigned24)a & MASK24) >> s;
      if (s > 0 && ((a >> (s-1)) & 1) == 1)
	{
	  if (s > 1 && (a & OB_ONES(s-1)) != 0)
	    t++;
	  else
	    t += t & 1;
	}
      result = OB_CLAMP(t);
    }
  return result;
}

static uint8_t
RZUOB(signed24 a, uint8_t s)
{
  uint8_t result;
  unsigned24 t;

  if (s >= 24)
    result = 0;
  else
    {
      t = ((unsigned24)a & MASK24) >> s;
      result = OB_CLAMP(t);
    }
  return result;
}


static const QH_ROUND qh_round[] = {
  RNASQH, RNAUQH, RNESQH, RNEUQH, RZSQH,  RZUQH
};

static const OB_ROUND ob_round[] = {
  NULL,   RNAUOB, NULL,   RNEUOB, NULL,   RZUOB
};


static uint64_t
qh_vector_round(sim_cpu *cpu, address_word cia, uint64_t v2, QH_ROUND round)
{
  uint64_t result = 0;
  int  i, s;
  int16_t h, h2;

  s = 0;
  for (i = 0; i < 4; i++)
    {
      h2 = (int16_t)(v2 & 0xFFFF);
      if (h2 >= 0)
	h = (*round)(ACC.qh[i], h2);
      else
	{
	  UnpredictableResult ();
	  h = 0xdead;
	}
      v2 >>= 16;
      result |= ((uint64_t)((uint16_t)h) << s);
      s += 16;
    }
  return result;
}

static uint64_t
qh_map_round(sim_cpu *cpu, address_word cia, int16_t h2, QH_ROUND round)
{
  uint64_t result = 0;
  int  i, s;
  int16_t  h;

  s = 0;
  for (i = 0; i < 4; i++)
    {
      if (h2 >= 0)
	h = (*round)(ACC.qh[i], h2);
      else
	{
	  UnpredictableResult ();
	  h = 0xdead;
	}
      result |= ((uint64_t)((uint16_t)h) << s);
      s += 16;
    }
  return result;
}

static uint64_t
ob_vector_round(sim_cpu *cpu, address_word cia, uint64_t v2, OB_ROUND round)
{
  uint64_t result = 0;
  int  i, s;
  uint8_t b, b2;

  s = 0;
  for (i = 0; i < 8; i++)
    {
      b2 = v2 & 0xFF;  v2 >>= 8;
      b = (*round)(ACC.ob[i], b2);
      result |= ((uint64_t)b << s);
      s += 8;
    }
  return result;
}

static uint64_t
ob_map_round(sim_cpu *cpu, address_word cia, uint8_t b2, OB_ROUND round)
{
  uint64_t result = 0;
  int  i, s;
  uint8_t b;

  s = 0;
  for (i = 0; i < 8; i++)
    {
      b = (*round)(ACC.ob[i], b2);
      result |= ((uint64_t)b << s);
      s += 8;
    }
  return result;
}


uint64_t
mdmx_round_op(sim_cpu *cpu,
	      address_word cia,
	      int rm,
	      int vt,
	      MX_fmtsel fmtsel)
{
  uint64_t op2;
  uint64_t result = 0;

  switch (MX_FMT (fmtsel))
    {
    case mdmx_qh:
      switch (MX_VT (fmtsel))
	{
	case sel_elem:
	  op2 = ValueFPR(vt, fmt_mdmx);
	  result = qh_map_round(cpu, cia, QH_ELEM(op2, fmtsel), qh_round[rm]);
	  break;
	case sel_vect:
	  op2 = ValueFPR(vt, fmt_mdmx);
	  result = qh_vector_round(cpu, cia, op2, qh_round[rm]);
	  break;
	case sel_imm:
	  result = qh_map_round(cpu, cia, vt, qh_round[rm]);
	  break;
	}
      break;
    case mdmx_ob:
      switch (MX_VT (fmtsel))
	{
	case sel_elem:
	  op2 = ValueFPR(vt, fmt_mdmx);
	  result = ob_map_round(cpu, cia, OB_ELEM(op2, fmtsel), ob_round[rm]);
	  break;
	case sel_vect:
	  op2 = ValueFPR(vt, fmt_mdmx);
	  result = ob_vector_round(cpu, cia, op2, ob_round[rm]);
	  break;
	case sel_imm:
	  result = ob_map_round(cpu, cia, vt, ob_round[rm]);
	  break;
	}
      break;
    default:
      Unpredictable ();
    }

  return result;
}


/* Shuffle operation.  */

typedef struct {
  enum {vs, ss, vt} source;
  unsigned int      index;
} sh_map;

static const sh_map ob_shuffle[][8] = {
  /* MDMX 2.0 encodings (3-4, 6-7).  */
  /* vr5400   encoding  (5), otherwise.  */
  {                                                              }, /* RSVD */
  {{vt,4}, {vs,4}, {vt,5}, {vs,5}, {vt,6}, {vs,6}, {vt,7}, {vs,7}}, /* RSVD */
  {{vt,0}, {vs,0}, {vt,1}, {vs,1}, {vt,2}, {vs,2}, {vt,3}, {vs,3}}, /* RSVD */
  {{vs,0}, {ss,0}, {vs,1}, {ss,1}, {vs,2}, {ss,2}, {vs,3}, {ss,3}}, /* upsl */
  {{vt,1}, {vt,3}, {vt,5}, {vt,7}, {vs,1}, {vs,3}, {vs,5}, {vs,7}}, /* pach */
  {{vt,0}, {vt,2}, {vt,4}, {vt,6}, {vs,0}, {vs,2}, {vs,4}, {vs,6}}, /* pacl */
  {{vt,4}, {vs,4}, {vt,5}, {vs,5}, {vt,6}, {vs,6}, {vt,7}, {vs,7}}, /* mixh */
  {{vt,0}, {vs,0}, {vt,1}, {vs,1}, {vt,2}, {vs,2}, {vt,3}, {vs,3}}  /* mixl */
};

static const sh_map qh_shuffle[][4] = {
  {{vt,2}, {vs,2}, {vt,3}, {vs,3}},  /* mixh */
  {{vt,0}, {vs,0}, {vt,1}, {vs,1}},  /* mixl */
  {{vt,1}, {vt,3}, {vs,1}, {vs,3}},  /* pach */
  {                              },  /* RSVD */
  {{vt,1}, {vs,0}, {vt,3}, {vs,2}},  /* bfla */
  {                              },  /* RSVD */
  {{vt,2}, {vt,3}, {vs,2}, {vs,3}},  /* repa */
  {{vt,0}, {vt,1}, {vs,0}, {vs,1}}   /* repb */
};


uint64_t
mdmx_shuffle(sim_cpu *cpu,
	     address_word cia,
	     int shop,
	     uint64_t op1,
	     uint64_t op2)
{
  uint64_t result = 0;
  int  i, s;
  int  op;

  if ((shop & 0x3) == 0x1)       /* QH format.  */
    {
      op = shop >> 2;
      s = 0;
      for (i = 0; i < 4; i++)
	{
	  uint64_t v;

	  switch (qh_shuffle[op][i].source)
	    {
	    case vs:
	      v = op1;
	      break;
	    case vt:
	      v = op2;
	      break;
	    default:
	      Unpredictable ();
	      v = 0;
	    }
	  result |= (((v >> 16*qh_shuffle[op][i].index) & 0xFFFF) << s);
	  s += 16;
	}
    }
  else if ((shop & 0x1) == 0x0)  /* OB format.  */
    {
      op = shop >> 1;
      s = 0;
      for (i = 0; i < 8; i++)
	{
	  uint8_t b;
	  unsigned int ishift = 8*ob_shuffle[op][i].index;

	  switch (ob_shuffle[op][i].source)
	    {
	    case vs:
	      b = (op1 >> ishift) & 0xFF;
	      break;
	    case ss:
	      b = ((op1 >> ishift) & 0x80) ? 0xFF : 0;
	      break;
	    case vt:
	      b = (op2 >> ishift) & 0xFF;
	      break;
	    default:
	      Unpredictable ();
	      b = 0;
	    }
	  result |= ((uint64_t)b << s);
	  s += 8;
	}
    }
  else
    Unpredictable ();

  return result;
}
