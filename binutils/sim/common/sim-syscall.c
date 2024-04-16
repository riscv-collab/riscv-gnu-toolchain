/* Simulator system call support.

   Copyright 2002-2024 Free Software Foundation, Inc.

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

#include <errno.h>

#include "ansidecl.h"

#include "sim-main.h"
#include "sim-syscall.h"
#include "sim/callback.h"

/* Read/write functions for system call interface.  */

int
sim_syscall_read_mem (host_callback *cb ATTRIBUTE_UNUSED, struct cb_syscall *sc,
		      unsigned long taddr, char *buf, int bytes)
{
  SIM_DESC sd = (SIM_DESC) sc->p1;
  SIM_CPU *cpu = (SIM_CPU *) sc->p2;

  TRACE_MEMORY (cpu, "READ (syscall) %i bytes @ 0x%08lx", bytes, taddr);

  return sim_core_read_buffer (sd, cpu, read_map, buf, taddr, bytes);
}

int
sim_syscall_write_mem (host_callback *cb ATTRIBUTE_UNUSED, struct cb_syscall *sc,
		       unsigned long taddr, const char *buf, int bytes)
{
  SIM_DESC sd = (SIM_DESC) sc->p1;
  SIM_CPU *cpu = (SIM_CPU *) sc->p2;

  TRACE_MEMORY (cpu, "WRITE (syscall) %i bytes @ 0x%08lx", bytes, taddr);

  return sim_core_write_buffer (sd, cpu, write_map, buf, taddr, bytes);
}

/* Main syscall callback for simulators.  */

void
sim_syscall_multi (SIM_CPU *cpu, int func, long arg1, long arg2, long arg3,
		   long arg4, long *result, long *result2, int *errcode)
{
  SIM_DESC sd = CPU_STATE (cpu);
  host_callback *cb = STATE_CALLBACK (sd);
  CB_SYSCALL sc;
  const char unknown_syscall[] = "<UNKNOWN SYSCALL>";
  const char *syscall;

  CB_SYSCALL_INIT (&sc);

  sc.func = func;
  sc.arg1 = arg1;
  sc.arg2 = arg2;
  sc.arg3 = arg3;
  sc.arg4 = arg4;

  sc.p1 = sd;
  sc.p2 = cpu;
  sc.read_mem = sim_syscall_read_mem;
  sc.write_mem = sim_syscall_write_mem;

  if (cb_syscall (cb, &sc) != CB_RC_OK)
    {
      /* The cb_syscall func never returns an error, so this is more of a
	 sanity check.  */
      sim_engine_abort (sd, cpu, sim_pc_get (cpu), "cb_syscall failed");
    }

  syscall = cb_target_str_syscall (cb, func);
  if (!syscall)
    syscall = unknown_syscall;

  if (sc.result == -1)
    TRACE_SYSCALL (cpu, "%s[%i](%#lx, %#lx, %#lx) = %li (error = %s[%i])",
		   syscall, func, arg1, arg2, arg3, sc.result,
		   cb_target_str_errno (cb, sc.errcode), sc.errcode);
  else
    TRACE_SYSCALL (cpu, "%s[%i](%#lx, %#lx, %#lx) = %li",
		   syscall, func, arg1, arg2, arg3, sc.result);

  /* Handle syscalls that affect engine behavior.  */
  switch (cb_target_to_host_syscall (cb, func))
    {
    case CB_SYS_exit:
      sim_engine_halt (sd, cpu, NULL, sim_pc_get (cpu), sim_exited, arg1);
      break;

    case CB_SYS_kill:
      /* TODO: Need to translate target signal to sim signal, but the sim
	 doesn't yet have such a mapping layer.  */
      if (arg1 == (*cb->getpid) (cb))
	sim_engine_halt (sd, cpu, NULL, sim_pc_get (cpu), sim_signalled, arg2);
      break;
    }

  *result = sc.result;
  *result2 = sc.result2;
  *errcode = sc.errcode;
}

long
sim_syscall (SIM_CPU *cpu, int func, long arg1, long arg2, long arg3, long arg4)
{
  long result, result2;
  int errcode;

  sim_syscall_multi (cpu, func, arg1, arg2, arg3, arg4, &result, &result2,
		     &errcode);
  if (result == -1)
    return -errcode;
  else
    return result;
}
