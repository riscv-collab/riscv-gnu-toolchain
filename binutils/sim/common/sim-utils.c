/* Miscellaneous simulator utilities.
   Copyright (C) 1997-2024 Free Software Foundation, Inc.
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

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif
#include <sys/time.h> /* needed by sys/resource.h */

#include "bfd.h"
#include "libiberty.h"

#include "sim-main.h"
#include "sim-assert.h"
#include "sim-utils.h"

/* Allocate zero filled memory with xcalloc - xcalloc aborts if the
   allocation fails.  */

void *
zalloc (unsigned long size)
{
  return xcalloc (1, size);
}

/* Allocate a sim_state struct.  */

SIM_DESC
sim_state_alloc_extra (SIM_OPEN_KIND kind, host_callback *callback,
		       size_t extra_bytes)
{
  SIM_DESC sd = ZALLOC (struct sim_state);

  STATE_MAGIC (sd) = SIM_MAGIC_NUMBER;
  STATE_CALLBACK (sd) = callback;
  STATE_OPEN_KIND (sd) = kind;

  if (extra_bytes)
    STATE_ARCH_DATA (sd) = zalloc (extra_bytes);

#if 0
  {
    int cpu_nr;

    /* Initialize the back link from the cpu struct to the state struct.  */
    /* ??? I can envision a design where the state struct contains an array
       of pointers to cpu structs, rather than an array of structs themselves.
       Implementing this is trickier as one may not know what to allocate until
       one has parsed the args.  Parsing the args twice wouldn't be unreasonable,
       IMHO.  If the state struct ever does contain an array of pointers then we
       can't do this here.
       ??? See also sim_post_argv_init*/
    for (cpu_nr = 0; cpu_nr < MAX_NR_PROCESSORS; cpu_nr++)
      {
	CPU_STATE (STATE_CPU (sd, cpu_nr)) = sd;
	CPU_INDEX (STATE_CPU (sd, cpu_nr)) = cpu_nr;
      }
  }
#endif

#ifdef SIM_STATE_INIT
  SIM_STATE_INIT (sd);
#endif

  return sd;
}

/* Free a sim_state struct.  */

void
sim_state_free (SIM_DESC sd)
{
  ASSERT (STATE_MAGIC (sd) == SIM_MAGIC_NUMBER);

#ifdef SIM_STATE_FREE
  SIM_STATE_FREE (sd);
#endif

  free (STATE_PROG_FILE (sd));
  free (STATE_PROG_ARGV0 (sd));
  freeargv (STATE_PROG_ENVP (sd));
  free (sd);
}

/* Return a pointer to the cpu data for CPU_NAME, or NULL if not found.  */

sim_cpu *
sim_cpu_lookup (SIM_DESC sd, const char *cpu_name)
{
  int i;

  for (i = 0; i < MAX_NR_PROCESSORS; ++i)
    if (strcmp (cpu_name, CPU_NAME (STATE_CPU (sd, i))) == 0)
      return STATE_CPU (sd, i);
  return NULL;
}

/* Return the prefix to use for a CPU specific message (typically an
   error message).  */

const char *
sim_cpu_msg_prefix (sim_cpu *cpu)
{
  static char *prefix;

  if (MAX_NR_PROCESSORS == 1)
    return "";

  if (prefix == NULL)
    {
      SIM_DESC sd = CPU_STATE (cpu);
      int maxlen = 0;
      int i;

      for (i = 0; i < MAX_NR_PROCESSORS; ++i)
	{
	  int len = strlen (CPU_NAME (STATE_CPU (sd, i)));
	  if (len > maxlen)
	    maxlen = len;
	}
      prefix = (char *) xmalloc (maxlen + 5);
    }
  sprintf (prefix, "%s: ", CPU_NAME (cpu));

  return prefix;
}

/* Cover fn to sim_io_eprintf.  */

void
sim_io_eprintf_cpu (sim_cpu *cpu, const char *fmt, ...)
{
  SIM_DESC sd = CPU_STATE (cpu);
  va_list ap;

  va_start (ap, fmt);
  sim_io_eprintf (sd, "%s", sim_cpu_msg_prefix (cpu));
  sim_io_evprintf (sd, fmt, ap);
  va_end (ap);
}

/* Turn VALUE into a string with commas.  */

char *
sim_add_commas (char *buf, int sizeof_buf, unsigned long value)
{
  int comma = 3;
  char *endbuf = buf + sizeof_buf - 1;

  *--endbuf = '\0';
  do {
    if (comma-- == 0)
      {
	*--endbuf = ',';
	comma = 2;
      }

    *--endbuf = (value % 10) + '0';
  } while ((value /= 10) != 0);

  return endbuf;
}

/* Analyze PROG_NAME/PROG_BFD and set these fields in the state struct:
   STATE_ARCHITECTURE, if not set already and can be determined from the bfd
   STATE_PROG_BFD
   STATE_START_ADDR
   STATE_TEXT_SECTION
   STATE_TEXT_START
   STATE_TEXT_END

   PROG_NAME is the file name of the executable or NULL.
   PROG_BFD is its bfd or NULL.

   If both PROG_NAME and PROG_BFD are NULL, this function returns immediately.
   If PROG_BFD is not NULL, PROG_NAME is ignored.

   Implicit inputs: STATE_MY_NAME(sd), STATE_TARGET(sd),
                    STATE_ARCHITECTURE(sd).

   A new bfd is created so the app isn't required to keep its copy of the
   bfd open.  */

SIM_RC
sim_analyze_program (SIM_DESC sd, const char *prog_name, bfd *prog_bfd)
{
  asection *s;
  SIM_ASSERT (STATE_MAGIC (sd) == SIM_MAGIC_NUMBER);

  if (prog_bfd != NULL)
    {
      if (prog_bfd == STATE_PROG_BFD (sd))
	/* already analyzed */
	return SIM_RC_OK;
      else
	/* duplicate needed, save the name of the file to be re-opened */
	prog_name = bfd_get_filename (prog_bfd);
    }

  /* do we need to duplicate anything? */
  if (prog_name == NULL)
    return SIM_RC_OK;

  /* open a new copy of the prog_bfd */
  prog_bfd = bfd_openr (prog_name, STATE_TARGET (sd));
  if (prog_bfd == NULL)
    {
      sim_io_eprintf (sd, "%s: can't open \"%s\": %s\n",
		      STATE_MY_NAME (sd),
		      prog_name,
		      bfd_errmsg (bfd_get_error ()));
      return SIM_RC_FAIL;
    }
  if (!bfd_check_format (prog_bfd, bfd_object))
    {
      sim_io_eprintf (sd, "%s: \"%s\" is not an object file: %s\n",
		      STATE_MY_NAME (sd),
		      prog_name,
		      bfd_errmsg (bfd_get_error ()));
      bfd_close (prog_bfd);
      return SIM_RC_FAIL;
    }
  if (STATE_ARCHITECTURE (sd) != NULL)
    bfd_set_arch_info (prog_bfd, STATE_ARCHITECTURE (sd));
  else
    {
      if (bfd_get_arch (prog_bfd) != bfd_arch_unknown
	  && bfd_get_arch (prog_bfd) != bfd_arch_obscure)
	{
	  STATE_ARCHITECTURE (sd) = bfd_get_arch_info (prog_bfd);
	}
    }

  /* update the sim structure */
  if (STATE_PROG_BFD (sd) != NULL)
    bfd_close (STATE_PROG_BFD (sd));
  STATE_PROG_BFD (sd) = prog_bfd;
  STATE_START_ADDR (sd) = bfd_get_start_address (prog_bfd);

  for (s = prog_bfd->sections; s; s = s->next)
    if (strcmp (bfd_section_name (s), ".text") == 0)
      {
	STATE_TEXT_SECTION (sd) = s;
	STATE_TEXT_START (sd) = bfd_section_vma (s);
	STATE_TEXT_END (sd) = STATE_TEXT_START (sd) + bfd_section_size (s);
	break;
      }

  bfd_cache_close (prog_bfd);

  return SIM_RC_OK;
}

/* Simulator timing support.  */

/* Called before sim_elapsed_time_since to get a reference point.  */

SIM_ELAPSED_TIME
sim_elapsed_time_get (void)
{
#ifdef HAVE_GETRUSAGE
  struct rusage mytime;
  if (getrusage (RUSAGE_SELF, &mytime) == 0)
    return 1 + (SIM_ELAPSED_TIME) (((double) mytime.ru_utime.tv_sec * 1000) + (((double) mytime.ru_utime.tv_usec + 500) / 1000));
  return 1;
#else
#ifdef HAVE_TIME
  return 1 + (SIM_ELAPSED_TIME) time ((time_t) 0);
#else
  return 1;
#endif
#endif
}

/* Return the elapsed time in milliseconds since START.
   The actual time may be cpu usage (preferred) or wall clock.  */

unsigned long
sim_elapsed_time_since (SIM_ELAPSED_TIME start)
{
#ifdef HAVE_GETRUSAGE
  return sim_elapsed_time_get () - start;
#else
#ifdef HAVE_TIME
  return (sim_elapsed_time_get () - start) * 1000;
#else
  return 0;
#endif
#endif
}



/* do_command but with printf style formatting of the arguments */
void
sim_do_commandf (SIM_DESC sd,
		 const char *fmt,
		 ...)
{
  va_list ap;
  char *buf;
  int ret;

  va_start (ap, fmt);
  ret = vasprintf (&buf, fmt, ap);
  va_end (ap);

  if (ret < 0)
    {
      sim_io_eprintf (sd, "%s: asprintf failed for `%s'\n",
		      STATE_MY_NAME (sd), fmt);
      return;
    }

  sim_do_command (sd, buf);
  free (buf);
}


/* sim-basics.h defines a number of enumerations, convert each of them
   to a string representation */
const char *
map_to_str (unsigned map)
{
  switch (map)
    {
    case read_map: return "read";
    case write_map: return "write";
    case exec_map: return "exec";
    case io_map: return "io";
    default:
      {
	static char str[16];
	snprintf (str, sizeof(str), "(%ld)", (long) map);
	return str;
      }
    }
}

const char *
access_to_str (unsigned access)
{
  switch (access)
    {
    case access_invalid: return "invalid";
    case access_read: return "read";
    case access_write: return "write";
    case access_exec: return "exec";
    case access_io: return "io";
    case access_read_write: return "read_write";
    case access_read_exec: return "read_exec";
    case access_write_exec: return "write_exec";
    case access_read_write_exec: return "read_write_exec";
    case access_read_io: return "read_io";
    case access_write_io: return "write_io";
    case access_read_write_io: return "read_write_io";
    case access_exec_io: return "exec_io";
    case access_read_exec_io: return "read_exec_io";
    case access_write_exec_io: return "write_exec_io";
    case access_read_write_exec_io: return "read_write_exec_io";
    default:
      {
	static char str[16];
	snprintf (str, sizeof(str), "(%ld)", (long) access);
	return str;
      }
    }
}

const char *
transfer_to_str (unsigned transfer)
{
  switch (transfer)
    {
    case read_transfer: return "read";
    case write_transfer: return "write";
    default: return "(error)";
    }
}
