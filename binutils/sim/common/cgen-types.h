/* Types for Cpu tools GENerated simulators.
   Copyright (C) 1996-2024 Free Software Foundation, Inc.
   Contributed by Cygnus Support.

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

/* This file is not included with cgen-sim.h as it defines types
   needed by sim-base.h.  */

#ifndef CGEN_TYPES_H
#define CGEN_TYPES_H

/* Miscellaneous cgen configury defined here as this file gets
   included soon enough.  */

/* Indicate we support --{profile,trace}-{range,function}.  */
#undef SIM_HAVE_ADDR_RANGE
#define SIM_HAVE_ADDR_RANGE

#ifdef __GNUC__
#undef DI_FN_SUPPORT
#else
#define DI_FN_SUPPORT
#endif

/* Mode support.  */

/* Common mode types.  */
/* ??? Target specific modes.  */
typedef enum mode_type {
  MODE_VOID, MODE_BI,
  MODE_QI, MODE_HI, MODE_SI, MODE_DI,
  MODE_UQI, MODE_UHI, MODE_USI, MODE_UDI,
  MODE_SF, MODE_DF, MODE_XF, MODE_TF,
  MODE_TARGET_MAX /* = MODE_TF? */,
  /* These are host modes.  */
  MODE_INT, MODE_UINT, MODE_PTR, /*??? MODE_ADDR, MODE_IADDR,*/
  MODE_MAX
} MODE_TYPE;

#define MAX_TARGET_MODES ((int) MODE_TARGET_MAX)
#define MAX_MODES ((int) MODE_MAX)

extern const char * const cgen_mode_names[];
#define MODE_NAME(m) (cgen_mode_names[m])

/* Some system headers define VOID already, so undo it.  */
#undef VOID
typedef void VOID;
typedef unsigned char BI;
typedef int8_t QI;
typedef int16_t HI;
typedef int32_t SI;
typedef uint8_t UQI;
typedef uint16_t UHI;
typedef uint32_t USI;

typedef int64_t DI;
typedef uint64_t UDI;
#define GETLODI(di) ((SI) (di))
#define GETHIDI(di) ((SI) ((UDI) (di) >> 32))
#define SETLODI(di, val) ((di) = (((di) & 0xffffffff00000000LL) | (USI) (val)))
#define SETHIDI(di, val) ((di) = (((di) & 0xffffffffLL) | (((UDI) (val)) << 32)))
#define MAKEDI(hi, lo) ((DI) (((UDI) (hi) << 32) | (UDI) (USI) (lo)))

/* These are used to record extracted raw data from an instruction, among other
   things.  It must be a host data type, and not a target one.  */
typedef int INT;
typedef unsigned int UINT;

typedef unsigned_address ADDR;  /* FIXME: wip*/
typedef unsigned_address IADDR; /* FIXME: wip*/

/* fp types are in cgen-fpu.h */

#endif /* CGEN_TYPES_H */
