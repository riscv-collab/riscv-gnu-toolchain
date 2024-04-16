/* Miscellaneous simulator utilities.

   Copyright (C) 2005-2024 Free Software Foundation, Inc.
   Contributed by Analog Devices, Inc. and Stephane Carrez.

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

#include "symcat.h"

#include "sim-main.h"
#include "sim-module.h"
#include "sim/sim.h"

/* Generic implementation of sim_close that works with simulators that use
   sim-module for all custom runtime options.  */

#ifndef SIM_CLOSE_HOOK
# define SIM_CLOSE_HOOK(sd, quitting)
#endif

void
sim_close (SIM_DESC sd, int quitting)
{
  SIM_CLOSE_HOOK (sd, quitting);

  /* If cgen is active, close it down.  */
#ifdef CGEN_ARCH
# define cgen_cpu_close XCONCAT2 (CGEN_ARCH,_cgen_cpu_close)
  cgen_cpu_close (CPU_CPU_DESC (STATE_CPU (sd, 0)));
#endif

  /* Shut down all registered/active modules.  */
  sim_module_uninstall (sd);

  /* Ensure that any resources allocated through the callback
     mechanism are released.  */
  sim_io_shutdown (sd);

  /* Break down all of the cpus.  */
  sim_cpu_free_all (sd);

  /* Finally break down the sim state itself.  */
  sim_state_free (sd);
}
