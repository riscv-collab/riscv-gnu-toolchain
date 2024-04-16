/* Model support.
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

/* This must come before any other includes.  */
#include "defs.h"

#include "bfd.h"
#include "libiberty.h"

#include "sim-main.h"
#include "sim-model.h"
#include "sim-options.h"
#include "sim-io.h"
#include "sim-assert.h"

static void model_set (sim_cpu *, const SIM_MODEL *);

static DECLARE_OPTION_HANDLER (model_option_handler);

static MODULE_INIT_FN sim_model_init;

enum {
  OPTION_MODEL = OPTION_START,
  OPTION_MODEL_INFO,
};

static const OPTION model_options[] = {
  { {"model", required_argument, NULL, OPTION_MODEL},
      '\0', "MODEL", "Specify model to simulate",
      model_option_handler, NULL },

  { {"model-info", no_argument, NULL, OPTION_MODEL_INFO},
      '\0', NULL, "List selectable models",
      model_option_handler, NULL },
  { {"info-model", no_argument, NULL, OPTION_MODEL_INFO},
      '\0', NULL, NULL,
      model_option_handler, NULL },

  { {NULL, no_argument, NULL, 0}, '\0', NULL, NULL, NULL, NULL }
};

static SIM_RC
model_option_handler (SIM_DESC sd, sim_cpu *cpu, int opt,
		      char *arg, int is_command)
{
  switch (opt)
    {
    case OPTION_MODEL :
      {
	const SIM_MODEL *model = sim_model_lookup (sd, arg);
	if (! model)
	  {
	    sim_io_eprintf (sd, "unknown model `%s'\n", arg);
	    return SIM_RC_FAIL;
	  }
	STATE_MODEL_NAME (sd) = arg;
	sim_model_set (sd, cpu, model);
	break;
      }

    case OPTION_MODEL_INFO :
      {
	const SIM_MACH * const *machp;
	const SIM_MODEL *model;

	if (STATE_MACHS (sd) == NULL)
	  {
	    sim_io_printf (sd, "This target does not support any models\n");
	    return SIM_RC_FAIL;
	  }

	for (machp = STATE_MACHS(sd); *machp != NULL; ++machp)
	  {
	    sim_io_printf (sd, "Models for architecture `%s':\n",
			   MACH_NAME (*machp));
	    for (model = MACH_MODELS (*machp); MODEL_NAME (model) != NULL;
		 ++model)
	      sim_io_printf (sd, " %s", MODEL_NAME (model));
	    sim_io_printf (sd, "\n");
	  }
	break;
      }
    }

  return SIM_RC_OK;
}

SIM_RC
sim_model_install (SIM_DESC sd)
{
  SIM_ASSERT (STATE_MAGIC (sd) == SIM_MAGIC_NUMBER);

  sim_add_option_table (sd, NULL, model_options);
  sim_module_add_init_fn (sd, sim_model_init);

  return SIM_RC_OK;
}

/* Subroutine of sim_model_set to set the model for one cpu.  */

static void
model_set (sim_cpu *cpu, const SIM_MODEL *model)
{
  CPU_MACH (cpu) = MODEL_MACH (model);
  CPU_MODEL (cpu) = model;
  (* MACH_INIT_CPU (MODEL_MACH (model))) (cpu);
  (* MODEL_INIT (model)) (cpu);
}

/* Set the current model of CPU to MODEL.
   If CPU is NULL, all cpus are set to MODEL.  */

void
sim_model_set (SIM_DESC sd, sim_cpu *cpu, const SIM_MODEL *model)
{
  if (! cpu)
    {
      int c;

      for (c = 0; c < MAX_NR_PROCESSORS; ++c)
	if (STATE_CPU (sd, c))
	  model_set (STATE_CPU (sd, c), model);
    }
  else
    {
      model_set (cpu, model);
    }
}

/* Look up model named NAME.
   Result is pointer to MODEL entry or NULL if not found.  */

const SIM_MODEL *
sim_model_lookup (SIM_DESC sd, const char *name)
{
  const SIM_MACH * const *machp;
  const SIM_MODEL *model;

  if (STATE_MACHS (sd) == NULL)
    return NULL;

  for (machp = STATE_MACHS (sd); *machp != NULL; ++machp)
    {
      for (model = MACH_MODELS (*machp); MODEL_NAME (model) != NULL; ++model)
	{
	  if (strcmp (MODEL_NAME (model), name) == 0)
	    return model;
	}
    }
  return NULL;
}

/* Look up machine named NAME.
   Result is pointer to MACH entry or NULL if not found.  */

const SIM_MACH *
sim_mach_lookup (SIM_DESC sd, const char *name)
{
  const SIM_MACH * const *machp;

  if (STATE_MACHS (sd) == NULL)
    return NULL;

  for (machp = STATE_MACHS (sd); *machp != NULL; ++machp)
    {
      if (strcmp (MACH_NAME (*machp), name) == 0)
	return *machp;
    }
  return NULL;
}

/* Look up a machine via its bfd name.
   Result is pointer to MACH entry or NULL if not found.  */

const SIM_MACH *
sim_mach_lookup_bfd_name (SIM_DESC sd, const char *name)
{
  const SIM_MACH * const *machp;

  if (STATE_MACHS (sd) == NULL)
    return NULL;

  for (machp = STATE_MACHS (sd); *machp != NULL; ++machp)
    {
      if (strcmp (MACH_BFD_NAME (*machp), name) == 0)
	return *machp;
    }
  return NULL;
}

/* Initialize model support.  */

static SIM_RC
sim_model_init (SIM_DESC sd)
{
  SIM_CPU *cpu;

  /* If both cpu model and state architecture are set, ensure they're
     compatible.  If only one is set, set the other.  If neither are set,
     use the default model.  STATE_ARCHITECTURE is the bfd_arch_info data
     for the selected "mach" (bfd terminology).  */

  /* Only check cpu 0.  STATE_ARCHITECTURE is for that one only.  */
  /* ??? At present this only supports homogeneous multiprocessors.  */
  cpu = STATE_CPU (sd, 0);

  if (! STATE_ARCHITECTURE (sd)
      && ! CPU_MACH (cpu)
      && STATE_MODEL_NAME (sd))
    {
      /* Set the default model.  */
      const SIM_MODEL *model = sim_model_lookup (sd, STATE_MODEL_NAME (sd));
      SIM_ASSERT (model != NULL);
      sim_model_set (sd, NULL, model);
    }

  if (STATE_ARCHITECTURE (sd)
      && CPU_MACH (cpu))
    {
      if (strcmp (STATE_ARCHITECTURE (sd)->printable_name,
		  MACH_BFD_NAME (CPU_MACH (cpu))) != 0)
	{
	  sim_io_eprintf (sd, "invalid model `%s' for `%s'\n",
			  MODEL_NAME (CPU_MODEL (cpu)),
			  STATE_ARCHITECTURE (sd)->printable_name);
	  return SIM_RC_FAIL;
	}
    }
  else if (STATE_ARCHITECTURE (sd) && STATE_MACHS (sd))
    {
      /* Use the default model for the selected machine.
	 The default model is the first one in the list.  */
      const SIM_MACH *mach =
	sim_mach_lookup_bfd_name (sd, STATE_ARCHITECTURE (sd)->printable_name);

      if (mach == NULL)
	{
	  sim_io_eprintf (sd, "unsupported machine `%s'\n",
			  STATE_ARCHITECTURE (sd)->printable_name);
	  return SIM_RC_FAIL;
	}
      sim_model_set (sd, NULL, MACH_MODELS (mach));
    }
  else if (CPU_MACH (cpu))
    {
      STATE_ARCHITECTURE (sd) = bfd_scan_arch (MACH_BFD_NAME (CPU_MACH (cpu)));
    }

  return SIM_RC_OK;
}
