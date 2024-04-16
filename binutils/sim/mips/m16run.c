/*  This file is part of the program psim.

    Copyright (C) 1998, Andrew Cagney <cagney@highland.com.au>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, see <http://www.gnu.org/licenses/>.

    */

/* This must come before any other includes.  */
#include "defs.h"

#include "sim-main.h"
#include "m16_idecode.h"
#include "m32_idecode.h"
#include "bfd.h"
#include "sim-engine.h"


#define SD sd
#define CPU cpu

void
sim_engine_run (SIM_DESC sd,
		int next_cpu_nr,
		int nr_cpus, /* ignore */
		int siggnal) /* ignore */
{
  sim_cpu *cpu = STATE_CPU (sd, next_cpu_nr);
  address_word cia = CPU_PC_GET (cpu);

  while (1)
    {
      address_word nia;

#if defined (ENGINE_ISSUE_PREFIX_HOOK)
      ENGINE_ISSUE_PREFIX_HOOK ();
#endif

      if ((cia & 1))
	{
	  m16_instruction_word instruction_0 = IMEM16 (cia);
	  nia = m16_idecode_issue (sd, instruction_0, cia);
	}
      else
	{
	  m32_instruction_word instruction_0 = IMEM32 (cia);
	  nia = m32_idecode_issue (sd, instruction_0, cia);
	}

      /* Update the instruction address */
      cia = nia;

      /* process any events */
      if (sim_events_tick (sd))
        {
          CPU_PC_SET (CPU, cia);
          sim_events_process (sd);
	  cia = CPU_PC_GET (CPU);
        }

    }
}
