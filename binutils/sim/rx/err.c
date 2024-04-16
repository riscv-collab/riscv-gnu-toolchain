/* err.c --- handle errors for RX simulator.

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

#include "err.h"

static unsigned char ee_actions[SIM_ERR_NUM_ERRORS];

static enum execution_error last_error;

static void
ee_overrides (void)
{
  /* GCC may initialize a bitfield by reading the uninitialized byte,
     masking in the bitfield, and writing the byte back out.  */
  ee_actions[SIM_ERR_READ_UNWRITTEN_BYTES] = SIM_ERRACTION_IGNORE;
  /* This breaks stack unwinding for exceptions because it leaves
     MC_PUSHED_PC tags in the unwound stack frames.  */
  ee_actions[SIM_ERR_CORRUPT_STACK] = SIM_ERRACTION_IGNORE;
}

void
execution_error_init_standalone (void)
{
  int i;

  for (i = 0; i < SIM_ERR_NUM_ERRORS; i++)
    ee_actions[i] = SIM_ERRACTION_EXIT;

  ee_overrides ();
}

void
execution_error_init_debugger (void)
{
  int i;

  for (i = 0; i < SIM_ERR_NUM_ERRORS; i++)
    ee_actions[i] = SIM_ERRACTION_DEBUG;

  ee_overrides ();
}

void
execution_error_warn_all (void)
{
  int i;

  for (i = 0; i < SIM_ERR_NUM_ERRORS; i++)
    ee_actions[i] = SIM_ERRACTION_WARN;
}

void
execution_error_ignore_all (void)
{
  int i;

  for (i = 0; i < SIM_ERR_NUM_ERRORS; i++)
    ee_actions[i] = SIM_ERRACTION_IGNORE;
}

void
execution_error (enum execution_error num, unsigned long address)
{
  if (ee_actions[num] != SIM_ERRACTION_IGNORE)
    last_error = num;

  if (ee_actions[num] == SIM_ERRACTION_EXIT
      || ee_actions[num] == SIM_ERRACTION_WARN)
    {
      switch (num)
	{
	case SIM_ERR_READ_UNWRITTEN_PAGES:
	case SIM_ERR_READ_UNWRITTEN_BYTES:
	  printf("Read from unwritten memory at 0x%lx\n", address);
	  break;

	case SIM_ERR_NULL_POINTER_DEREFERENCE:
	  printf ("NULL pointer dereference\n");
	  break;

	case SIM_ERR_CORRUPT_STACK:
	  printf ("Stack corruption detected at 0x%lx\n", address);
	  break;

	default:
	  printf ("Unknown execution error %d\n", num);
	  exit (1);
	}
    }

  if (ee_actions[num] == SIM_ERRACTION_EXIT)
    exit (1);
}

enum execution_error
execution_error_get_last_error (void)
{
  return last_error;
}

void
execution_error_clear_last_error (void)
{
  last_error = SIM_ERR_NONE;
}

void
execution_error_set_action (enum execution_error num, enum execution_error_action act)
{
  ee_actions[num] = act;
}
