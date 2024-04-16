/* Simulator hardware option handling.
   Copyright (C) 1998-2024 Free Software Foundation, Inc.
   Contributed by Cygnus Support and Andrew Cagney.

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

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "sim-main.h"
#include "sim-assert.h"
#include "sim-options.h"
#include "sim/callback.h"

#include "sim-hw.h"

#include "hw-tree.h"
#include "hw-device.h"
#include "hw-main.h"
#include "hw-base.h"

struct sim_hw {
  struct hw *tree;
  int trace_p;
  int info_p;
  /* if called from a processor */
  sim_cpu *cpu;
  sim_cia cia;
};


struct hw *
sim_hw_parse (struct sim_state *sd,
	      const char *fmt,
	      ...)
{
  struct hw *current;
  va_list ap;
  va_start (ap, fmt);
  current = hw_tree_vparse (STATE_HW (sd)->tree, fmt, ap);
  va_end (ap);
  return current;
}

struct printer {
  struct sim_state *file;
  void (*print) (struct sim_state *, const char *, va_list ap);
};

static void
do_print (void *file, const char *fmt, ...)
{
  struct printer *p = file;
  va_list ap;
  va_start (ap, fmt);
  p->print (p->file, fmt, ap);
  va_end (ap);
}

void
sim_hw_print (struct sim_state *sd,
	      void (*print) (struct sim_state *, const char *, va_list ap))
{
  struct printer p;
  p.file = sd;
  p.print = print;
  hw_tree_print (STATE_HW (sd)->tree, do_print, &p);
}




/* command line options. */

enum {
  OPTION_HW_INFO = OPTION_START,
  OPTION_HW_TRACE,
  OPTION_HW_DEVICE,
  OPTION_HW_LIST,
  OPTION_HW_FILE,
};

static DECLARE_OPTION_HANDLER (hw_option_handler);

static const OPTION hw_options[] =
{
  { {"hw-info", no_argument, NULL, OPTION_HW_INFO },
      '\0', NULL, "List configurable hw regions",
      hw_option_handler, NULL },
  { {"info-hw", no_argument, NULL, OPTION_HW_INFO },
      '\0', NULL, NULL,
      hw_option_handler, NULL },

  { {"hw-trace", optional_argument, NULL, OPTION_HW_TRACE },
      '\0', "on|off", "Trace all hardware devices",
      hw_option_handler, NULL },
  { {"trace-hw", optional_argument, NULL, OPTION_HW_TRACE },
      '\0', NULL, NULL,
      hw_option_handler, NULL },

  { {"hw-device", required_argument, NULL, OPTION_HW_DEVICE },
      '\0', "DEVICE", "Add the specified device",
      hw_option_handler, NULL },

  { {"hw-list", no_argument, NULL, OPTION_HW_LIST },
      '\0', NULL, "List the device tree",
      hw_option_handler, NULL },

  { {"hw-file", required_argument, NULL, OPTION_HW_FILE },
      '\0', "FILE", "Add the devices listed in the file",
      hw_option_handler, NULL },

  { {NULL, no_argument, NULL, 0}, '\0', NULL, NULL, NULL, NULL }
};



/* Copied from ../ppc/psim.c:psim_merge_device_file() */

static SIM_RC
merge_device_file (struct sim_state *sd,
		   const char *file_name)
{
  FILE *description;
  struct hw *current = STATE_HW (sd)->tree;
  char *device_path = NULL;
  size_t buf_size = 0;
  ssize_t device_path_len;

  /* try opening the file */
  description = fopen (file_name, "r");
  if (description == NULL)
    {
      perror (file_name);
      return SIM_RC_FAIL;
    }

  while ((device_path_len = getline (&device_path, &buf_size, description)) > 0)
    {
      char *device;
      char *next_line = NULL;

      if (device_path[device_path_len - 1] == '\n')
	device_path[--device_path_len] = '\0';

      /* skip comments ("#" or ";") and blank lines lines */
      for (device = device_path;
	   *device != '\0' && isspace (*device);
	   device++);
      if (device[0] == '#'
	  || device[0] == ';'
	  || device[0] == '\0')
	continue;

      /* merge any appended lines */
      while (device_path[device_path_len - 1] == '\\')
	{
	  size_t next_buf_size = 0;
	  ssize_t next_line_len;

	  /* zap the `\' at the end of the line */
	  device_path[--device_path_len] = '\0';

	  /* get the next line */
	  next_line_len = getline (&next_line, &next_buf_size, description);
	  if (next_line_len <= 0)
	    break;

	  if (next_line[next_line_len - 1] == '\n')
	    next_line[--next_line_len] = '\0';

	  /* append the next line */
	  if (buf_size - device_path_len <= next_line_len)
	    {
	      ptrdiff_t offset = device - device_path;

	      buf_size += next_buf_size;
	      device_path = xrealloc (device_path, buf_size);
	      device = device_path + offset;
	    }
	  memcpy (device_path + device_path_len, next_line,
		  next_line_len + 1);
	  device_path_len += next_line_len;
	}
      free (next_line);

      /* parse this line */
      current = hw_tree_parse (current, "%s", device);
    }

  free (device_path);
  fclose (description);
  return SIM_RC_OK;
}


static SIM_RC
hw_option_handler (struct sim_state *sd, sim_cpu *cpu, int opt,
		   char *arg, int is_command)
{
  switch (opt)
    {

    case OPTION_HW_INFO:
      {
	/* delay info until after the tree is finished */
	STATE_HW (sd)->info_p = 1;
	return SIM_RC_OK;
	break;
      }

    case OPTION_HW_TRACE:
      {
	if (arg == NULL)
	  {
	    STATE_HW (sd)->trace_p = 1;
	  }
	else if (strcmp (arg, "yes") == 0
		 || strcmp (arg, "on") == 0)
	  {
	    STATE_HW (sd)->trace_p = 1;
	  }
	else if (strcmp (arg, "no") == 0
		 || strcmp (arg, "off") == 0)
	  {
	    STATE_HW (sd)->trace_p = 0;
	  }
	else
	  {
	    sim_io_eprintf (sd, "Option --hw-trace ignored\n");
	    /* set tracing on all devices */
	    return SIM_RC_FAIL;
	  }
	/* FIXME: Not very nice - see also hw-base.c */
	if (STATE_HW (sd)->trace_p)
	  hw_tree_parse (STATE_HW (sd)->tree, "/global-trace? true");
	return SIM_RC_OK;
	break;
      }

    case OPTION_HW_DEVICE:
      {
	hw_tree_parse (STATE_HW (sd)->tree, "%s", arg);
	return SIM_RC_OK;
      }

    case OPTION_HW_LIST:
      {
	sim_hw_print (sd, sim_io_vprintf);
	return SIM_RC_OK;
      }

    case OPTION_HW_FILE:
      {
	return merge_device_file (sd, arg);
      }

    default:
      sim_io_eprintf (sd, "Unknown hw option %d\n", opt);
      return SIM_RC_FAIL;

    }

  return SIM_RC_FAIL;
}


/* "hw" module install handler.

   This is called via sim_module_install to install the "hw" subsystem
   into the simulator.  */

static MODULE_INIT_FN sim_hw_init;
static MODULE_UNINSTALL_FN sim_hw_uninstall;

/* Provide a prototype to silence -Wmissing-prototypes.  */
SIM_RC sim_install_hw (struct sim_state *sd);

/* Establish this object.  */
SIM_RC
sim_install_hw (struct sim_state *sd)
{
  SIM_ASSERT (STATE_MAGIC (sd) == SIM_MAGIC_NUMBER);
  sim_add_option_table (sd, NULL, hw_options);
  sim_module_add_uninstall_fn (sd, sim_hw_uninstall);
  sim_module_add_init_fn (sd, sim_hw_init);
  STATE_HW (sd) = ZALLOC (struct sim_hw);
  STATE_HW (sd)->tree = hw_tree_create (sd, "core");
  return SIM_RC_OK;
}


static SIM_RC
sim_hw_init (struct sim_state *sd)
{
  /* FIXME: anything needed? */
  hw_tree_finish (STATE_HW (sd)->tree);
  if (STATE_HW (sd)->info_p)
    sim_hw_print (sd, sim_io_vprintf);
  return SIM_RC_OK;
}

/* Uninstall the "hw" subsystem from the simulator.  */

static void
sim_hw_uninstall (struct sim_state *sd)
{
  hw_tree_delete (STATE_HW (sd)->tree);
  free (STATE_HW (sd));
  STATE_HW (sd) = NULL;
}



/* Data transfers to/from the hardware device tree.  There are several
   cases. */


/* CPU: The simulation is running and the current CPU/CIA
   initiates a data transfer. */

void
sim_cpu_hw_io_read_buffer (sim_cpu *cpu,
			   sim_cia cia,
			   struct hw *hw,
			   void *dest,
			   int space,
			   unsigned_word addr,
			   unsigned nr_bytes)
{
  SIM_DESC sd = CPU_STATE (cpu);
  STATE_HW (sd)->cpu = cpu;
  STATE_HW (sd)->cia = cia;
  if (hw_io_read_buffer (hw, dest, space, addr, nr_bytes) != nr_bytes)
    sim_engine_abort (sd, cpu, cia, "broken CPU read");
}

void
sim_cpu_hw_io_write_buffer (sim_cpu *cpu,
			    sim_cia cia,
			    struct hw *hw,
			    const void *source,
			    int space,
			    unsigned_word addr,
			    unsigned nr_bytes)
{
  SIM_DESC sd = CPU_STATE (cpu);
  STATE_HW (sd)->cpu = cpu;
  STATE_HW (sd)->cia = cia;
  if (hw_io_write_buffer (hw, source, space, addr, nr_bytes) != nr_bytes)
    sim_engine_abort (sd, cpu, cia, "broken CPU write");
}




/* SYSTEM: A data transfer is being initiated by the system. */

unsigned
sim_hw_io_read_buffer (struct sim_state *sd,
		       struct hw *hw,
		       void *dest,
		       int space,
		       unsigned_word addr,
		       unsigned nr_bytes)
{
  STATE_HW (sd)->cpu = NULL;
  return hw_io_read_buffer (hw, dest, space, addr, nr_bytes);
}

unsigned
sim_hw_io_write_buffer (struct sim_state *sd,
			struct hw *hw,
			const void *source,
			int space,
			unsigned_word addr,
			unsigned nr_bytes)
{
  STATE_HW (sd)->cpu = NULL;
  return hw_io_write_buffer (hw, source, space, addr, nr_bytes);
}



/* Abort the simulation specifying HW as the reason */

void
hw_vabort (struct hw *me,
	   const char *fmt,
	   va_list ap)
{
  int len;
  const char *name;
  char *msg;
  va_list cpy;

  /* find an identity */
  if (me != NULL && hw_path (me) != NULL && hw_path (me) [0] != '\0')
    name = hw_path (me);
  else if (me != NULL && hw_name (me) != NULL && hw_name (me)[0] != '\0')
    name = hw_name (me);
  else if (me != NULL && hw_family (me) != NULL && hw_family (me)[0] != '\0')
    name = hw_family (me);
  else
    name = "device";

  /* Expand FMT and AP into MSG buffer.  */
  va_copy (cpy, ap);
  len = vsnprintf (NULL, 0, fmt, cpy) + 1;
  va_end (cpy);
  msg = alloca (len);
  vsnprintf (msg, len, fmt, ap);

  /* report the problem */
  sim_engine_abort (hw_system (me),
		    STATE_HW (hw_system (me))->cpu,
		    STATE_HW (hw_system (me))->cia,
		    "%s: %s", name, msg);
}

void
hw_abort (struct hw *me,
	  const char *fmt,
	  ...)
{
  va_list ap;
  /* report the problem */
  va_start (ap, fmt);
  hw_vabort (me, fmt, ap);
  va_end (ap);
}

void
sim_hw_abort (struct sim_state *sd,
	      struct hw *me,
	      const char *fmt,
	      ...)
{
  va_list ap;
  va_start (ap, fmt);
  if (me == NULL)
    sim_engine_vabort (sd, NULL, NULL_CIA, fmt, ap);
  else
    hw_vabort (me, fmt, ap);
  va_end (ap);
}


/* MISC routines to tie HW into the rest of the system */

void
hw_halt (struct hw *me,
	 int reason,
	 int status)
{
  struct sim_state *sd = hw_system (me);
  struct sim_hw *sim = STATE_HW (sd);
  sim_engine_halt (sd, sim->cpu, NULL, sim->cia, reason, status);
}

struct _sim_cpu *
hw_system_cpu (struct hw *me)
{
  return STATE_HW (hw_system (me))->cpu;
}

void
hw_trace (struct hw *me,
	  const char *fmt,
	  ...)
{
  if (hw_trace_p (me)) /* to be sure, to be sure */
    {
      va_list ap;
      va_start (ap, fmt);
      sim_io_eprintf (hw_system (me), "%s: ", hw_path (me));
      sim_io_evprintf (hw_system (me), fmt, ap);
      sim_io_eprintf (hw_system (me), "\n");
      va_end (ap);
    }
}


/* Based on gdb-4.17/sim/ppc/main.c:sim_io_read_stdin() */

int
do_hw_poll_read (struct hw *me,
		 do_hw_poll_read_method *read,
		 int sim_io_fd,
		 void *buf,
		 unsigned sizeof_buf)
{
  int status = read (hw_system (me), sim_io_fd, buf, sizeof_buf);
  if (status > 0)
    return status;
  else if (status == 0 && sizeof_buf == 0)
    return 0;
  else if (status == 0)
    return HW_IO_EOF;
  else /* status < 0 */
    {
#ifdef EAGAIN
      if (STATE_CALLBACK (hw_system (me))->last_errno == EAGAIN)
	return HW_IO_NOT_READY;
      else
	return HW_IO_EOF;
#else
      return HW_IO_EOF;
#endif
    }
}
