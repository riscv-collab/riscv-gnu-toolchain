/* Common hooks for RISC-V.
   Copyright (C) 1989-2014 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "common/common-target.h"
#include "common/common-target-def.h"
#include "opts.h"
#include "flags.h"
#include "errors.h"

/* Parse a RISC-V ISA string into an option mask.  */

static void
riscv_parse_arch_string (const char *isa, int *flags)
{
  const char *p = isa;

  if (strncmp (p, "RV32", 4) == 0)
    *flags |= MASK_32BIT, p += 4;
  else if (strncmp (p, "RV64", 4) == 0)
    *flags &= ~MASK_32BIT, p += 4;

  if (*p++ != 'I')
    {
      error ("-march=%s: ISA strings must begin with I, RV32I, or RV64I", isa);
      return;
    }

  *flags &= ~MASK_MULDIV;
  if (*p == 'M')
    *flags |= MASK_MULDIV, p++;

  *flags &= ~MASK_ATOMIC;
  if (*p == 'A')
    *flags |= MASK_ATOMIC, p++;

  *flags |= MASK_SOFT_FLOAT_ABI;
  if (*p == 'F')
    *flags &= ~MASK_SOFT_FLOAT_ABI, p++;

  if (*p == 'D')
    {
      p++;
      if (!TARGET_HARD_FLOAT)
	{
	  error ("-march=%s: the D extension requires the F extension", isa);
	  return;
	}
    }
  else if (TARGET_HARD_FLOAT)
    {
      error ("-march=%s: single-precision-only is not yet supported", isa);
      return;
    }

  *flags &= ~MASK_RVC;
  if (*p == 'C')
    *flags |= MASK_RVC, p++;

  if (*p)
    {
      error ("-march=%s: unsupported ISA substring %s", isa, p);
      return;
    }
}

static int
riscv_flags_from_arch_string (const char *isa)
{
  int flags = 0;
  riscv_parse_arch_string (isa, &flags);
  return flags;
}

/* Implement TARGET_HANDLE_OPTION.  */

static bool
riscv_handle_option (struct gcc_options *opts,
		     struct gcc_options *opts_set ATTRIBUTE_UNUSED,
		     const struct cl_decoded_option *decoded,
		     location_t loc ATTRIBUTE_UNUSED)
{
  switch (decoded->opt_index)
    {
    case OPT_march_:
      riscv_parse_arch_string (decoded->arg, &opts->x_target_flags);
      return true;

    default:
      return true;
    }
}

/* Implement TARGET_OPTION_OPTIMIZATION_TABLE.  */
static const struct default_options riscv_option_optimization_table[] =
  {
    { OPT_LEVELS_1_PLUS, OPT_fsection_anchors, NULL, 1 },
    { OPT_LEVELS_1_PLUS, OPT_fomit_frame_pointer, NULL, 1 },
    { OPT_LEVELS_NONE, 0, NULL, 0 }
  };

#undef TARGET_OPTION_OPTIMIZATION_TABLE
#define TARGET_OPTION_OPTIMIZATION_TABLE riscv_option_optimization_table

#undef TARGET_DEFAULT_TARGET_FLAGS
#define TARGET_DEFAULT_TARGET_FLAGS				\
  (TARGET_DEFAULT						\
   | riscv_flags_from_arch_string (RISCV_ARCH_STRING_DEFAULT)	\
   | (TARGET_64BIT_DEFAULT ? 0 : MASK_32BIT))

#undef TARGET_HANDLE_OPTION
#define TARGET_HANDLE_OPTION riscv_handle_option

struct gcc_targetm_common targetm_common = TARGETM_COMMON_INITIALIZER;
