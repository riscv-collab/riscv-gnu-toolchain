/* Simulator option handling.
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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bfd.h"
#include "environ.h"
#include "hashtab.h"
#include "libiberty.h"

#include "sim-main.h"
#include "sim-options.h"
#include "sim-io.h"
#include "sim-assert.h"
#include "version.h"

/* Add a set of options to the simulator.
   TABLE is an array of OPTIONS terminated by a NULL `opt.name' entry.
   This is intended to be called by modules in their `install' handler.  */

SIM_RC
sim_add_option_table (SIM_DESC sd, sim_cpu *cpu, const OPTION *table)
{
  struct option_list *ol = ((struct option_list *)
			    xmalloc (sizeof (struct option_list)));

  /* Note: The list is constructed in the reverse order we're called so
     later calls will override earlier ones (in case that ever happens).
     This is the intended behaviour.  */

  if (cpu)
    {
      ol->next = CPU_OPTIONS (cpu);
      ol->options = table;
      CPU_OPTIONS (cpu) = ol;
    }
  else
    {
      ol->next = STATE_OPTIONS (sd);
      ol->options = table;
      STATE_OPTIONS (sd) = ol;
    }

  return SIM_RC_OK;
}

/* Standard option table.
   Modules may specify additional ones.
   The caller of sim_parse_args may also specify additional options
   by calling sim_add_option_table first.  */

static DECLARE_OPTION_HANDLER (standard_option_handler);

/* FIXME: We shouldn't print in --help output options that aren't usable.
   Some fine tuning will be necessary.  One can either move less general
   options to another table or use a HAVE_FOO macro to ifdef out unavailable
   options.  */

/* ??? One might want to conditionally compile out the entries that
   aren't enabled.  There's a distinction, however, between options a
   simulator can't support and options that haven't been configured in.
   Certainly options a simulator can't support shouldn't appear in the
   output of --help.  Whether the same thing applies to options that haven't
   been configured in or not isn't something I can get worked up over.
   [Note that conditionally compiling them out might simply involve moving
   the option to another table.]
   If you decide to conditionally compile them out as well, delete this
   comment and add a comment saying that that is the rule.  */

typedef enum {
  OPTION_DEBUG_INSN = OPTION_START,
  OPTION_DEBUG_FILE,
  OPTION_DO_COMMAND,
  OPTION_ARCHITECTURE,
  OPTION_TARGET,
  OPTION_TARGET_INFO,
  OPTION_ARCHITECTURE_INFO,
  OPTION_ENVIRONMENT,
  OPTION_ALIGNMENT,
  OPTION_VERBOSE,
  OPTION_ENDIAN,
  OPTION_DEBUG,
  OPTION_HELP,
  OPTION_VERSION,
  OPTION_LOAD_LMA,
  OPTION_LOAD_VMA,
  OPTION_SYSROOT,
  OPTION_ARGV0,
  OPTION_ENV_SET,
  OPTION_ENV_UNSET,
  OPTION_ENV_CLEAR,
} STANDARD_OPTIONS;

static const OPTION standard_options[] =
{
  { {"verbose", no_argument, NULL, OPTION_VERBOSE},
      'v', NULL, "Verbose output",
      standard_option_handler, NULL },

  { {"endian", required_argument, NULL, OPTION_ENDIAN},
      'E', "B|big|L|little", "Set endianness",
      standard_option_handler, NULL },

  /* This option isn't supported unless all choices are supported in keeping
     with the goal of not printing in --help output things the simulator can't
     do [as opposed to things that just haven't been configured in].  */
  { {"environment", required_argument, NULL, OPTION_ENVIRONMENT},
      '\0', "user|virtual|operating", "Set running environment",
      standard_option_handler },

  { {"alignment", required_argument, NULL, OPTION_ALIGNMENT},
      '\0', "strict|nonstrict|forced", "Set memory access alignment",
      standard_option_handler },

  { {"debug", no_argument, NULL, OPTION_DEBUG},
      'D', NULL, "Print debugging messages",
      standard_option_handler },
  { {"debug-insn", no_argument, NULL, OPTION_DEBUG_INSN},
      '\0', NULL, "Print instruction debugging messages",
      standard_option_handler },
  { {"debug-file", required_argument, NULL, OPTION_DEBUG_FILE},
      '\0', "FILE NAME", "Specify debugging output file",
      standard_option_handler },

  { {"do-command", required_argument, NULL, OPTION_DO_COMMAND},
      '\0', "COMMAND", ""/*undocumented*/,
      standard_option_handler },

  { {"help", no_argument, NULL, OPTION_HELP},
      'h', NULL, "Print help information",
      standard_option_handler },
  { {"version", no_argument, NULL, OPTION_VERSION},
      '\0', NULL, "Print version information",
      standard_option_handler },

  { {"architecture", required_argument, NULL, OPTION_ARCHITECTURE},
      '\0', "MACHINE", "Specify the architecture to use",
      standard_option_handler },
  { {"architecture-info", no_argument, NULL, OPTION_ARCHITECTURE_INFO},
      '\0', NULL, "List supported architectures",
      standard_option_handler },
  { {"info-architecture", no_argument, NULL, OPTION_ARCHITECTURE_INFO},
      '\0', NULL, NULL,
      standard_option_handler },

  { {"target", required_argument, NULL, OPTION_TARGET},
      '\0', "BFDNAME", "Specify the object-code format for the object files",
      standard_option_handler },
  { {"target-info", no_argument, NULL, OPTION_TARGET_INFO},
      '\0', NULL, "List supported targets", standard_option_handler },
  { {"info-target", no_argument, NULL, OPTION_TARGET_INFO},
      '\0', NULL, NULL, standard_option_handler },

  { {"load-lma", no_argument, NULL, OPTION_LOAD_LMA},
      '\0', NULL,
    "Use VMA or LMA addresses when loading image (default LMA)",
      standard_option_handler, "load-{lma,vma}" },
  { {"load-vma", no_argument, NULL, OPTION_LOAD_VMA},
      '\0', NULL, "", standard_option_handler,  "" },

  { {"sysroot", required_argument, NULL, OPTION_SYSROOT},
      '\0', "SYSROOT",
    "Root for system calls with absolute file-names and cwd at start",
      standard_option_handler, NULL },

  { {"argv0", required_argument, NULL, OPTION_ARGV0},
      '\0', "ARGV0", "Set argv[0] to the specified string",
      standard_option_handler, NULL },

  { {"env-set", required_argument, NULL, OPTION_ENV_SET},
      '\0', "VAR=VAL", "Set the variable in the program's environment",
      standard_option_handler, NULL },
  { {"env-unset", required_argument, NULL, OPTION_ENV_UNSET},
      '\0', "VAR", "Unset the variable in the program's environment",
      standard_option_handler, NULL },
  { {"env-clear", no_argument, NULL, OPTION_ENV_CLEAR},
      '\0', NULL, "Clear the program's environment",
      standard_option_handler, NULL },

  { {NULL, no_argument, NULL, 0}, '\0', NULL, NULL, NULL, NULL }
};

static SIM_RC
env_set (SIM_DESC sd, const char *arg)
{
  int i, varlen;
  char *eq;
  char **envp;

  if (STATE_PROG_ENVP (sd) == NULL)
    STATE_PROG_ENVP (sd) = dupargv (environ);

  eq = strchr (arg, '=');
  if (eq == NULL)
    {
      sim_io_eprintf (sd, "invalid syntax when setting env var `%s'"
		      ": missing value", arg);
      return SIM_RC_FAIL;
    }
  /* Include the = in the comparison below.  */
  varlen = eq - arg + 1;

  /* If we can find an existing variable, replace it.  */
  envp = STATE_PROG_ENVP (sd);
  for (i = 0; envp[i]; ++i)
    {
      if (strncmp (envp[i], arg, varlen) == 0)
	{
	  free (envp[i]);
	  envp[i] = xstrdup (arg);
	  break;
	}
    }

  /* If we didn't find the var, add it.  */
  if (envp[i] == NULL)
    {
      envp = xrealloc (envp, (i + 2) * sizeof (char *));
      envp[i] = xstrdup (arg);
      envp[i + 1] = NULL;
      STATE_PROG_ENVP (sd) = envp;
  }

  return SIM_RC_OK;
}

static SIM_RC
standard_option_handler (SIM_DESC sd, sim_cpu *cpu, int opt,
			 char *arg, int is_command)
{
  int i,n;

  switch ((STANDARD_OPTIONS) opt)
    {
    case OPTION_VERBOSE:
      STATE_VERBOSE_P (sd) = 1;
      break;

    case OPTION_ENDIAN:
      if (strcmp (arg, "big") == 0 || strcmp (arg, "B") == 0)
	{
	  if (WITH_TARGET_BYTE_ORDER == BFD_ENDIAN_LITTLE)
	    {
	      sim_io_eprintf (sd, "Simulator compiled for little endian only.\n");
	      return SIM_RC_FAIL;
	    }
	  /* FIXME:wip: Need to set something in STATE_CONFIG.  */
	  current_target_byte_order = BFD_ENDIAN_BIG;
	}
      else if (strcmp (arg, "little") == 0 || strcmp (arg, "L") == 0)
	{
	  if (WITH_TARGET_BYTE_ORDER == BFD_ENDIAN_BIG)
	    {
	      sim_io_eprintf (sd, "Simulator compiled for big endian only.\n");
	      return SIM_RC_FAIL;
	    }
	  /* FIXME:wip: Need to set something in STATE_CONFIG.  */
	  current_target_byte_order = BFD_ENDIAN_LITTLE;
	}
      else
	{
	  sim_io_eprintf (sd, "Invalid endian specification `%s'\n", arg);
	  return SIM_RC_FAIL;
	}
      break;

    case OPTION_ENVIRONMENT:
      if (strcmp (arg, "user") == 0)
	STATE_ENVIRONMENT (sd) = USER_ENVIRONMENT;
      else if (strcmp (arg, "virtual") == 0)
	STATE_ENVIRONMENT (sd) = VIRTUAL_ENVIRONMENT;
      else if (strcmp (arg, "operating") == 0)
	STATE_ENVIRONMENT (sd) = OPERATING_ENVIRONMENT;
      else
	{
	  sim_io_eprintf (sd, "Invalid environment specification `%s'\n", arg);
	  return SIM_RC_FAIL;
	}
      if (WITH_ENVIRONMENT != ALL_ENVIRONMENT
	  && WITH_ENVIRONMENT != STATE_ENVIRONMENT (sd))
	{
	  const char *type;
	  switch (WITH_ENVIRONMENT)
	    {
	    case USER_ENVIRONMENT: type = "user"; break;
	    case VIRTUAL_ENVIRONMENT: type = "virtual"; break;
	    case OPERATING_ENVIRONMENT: type = "operating"; break;
	    default: abort ();
	    }
	  sim_io_eprintf (sd, "Simulator compiled for the %s environment only.\n",
			  type);
	  return SIM_RC_FAIL;
	}
      break;

    case OPTION_ALIGNMENT:
      if (strcmp (arg, "strict") == 0)
	{
	  if (WITH_ALIGNMENT == 0 || WITH_ALIGNMENT == STRICT_ALIGNMENT)
	    {
	      current_alignment = STRICT_ALIGNMENT;
	      break;
	    }
	}
      else if (strcmp (arg, "nonstrict") == 0)
	{
	  if (WITH_ALIGNMENT == 0 || WITH_ALIGNMENT == NONSTRICT_ALIGNMENT)
	    {
	      current_alignment = NONSTRICT_ALIGNMENT;
	      break;
	    }
	}
      else if (strcmp (arg, "forced") == 0)
	{
	  if (WITH_ALIGNMENT == 0 || WITH_ALIGNMENT == FORCED_ALIGNMENT)
	    {
	      current_alignment = FORCED_ALIGNMENT;
	      break;
	    }
	}
      else
	{
	  sim_io_eprintf (sd, "Invalid alignment specification `%s'\n", arg);
	  return SIM_RC_FAIL;
	}
      switch (WITH_ALIGNMENT)
	{
	case STRICT_ALIGNMENT:
	  sim_io_eprintf (sd, "Simulator compiled for strict alignment only.\n");
	  break;
	case NONSTRICT_ALIGNMENT:
	  sim_io_eprintf (sd, "Simulator compiled for nonstrict alignment only.\n");
	  break;
	case FORCED_ALIGNMENT:
	  sim_io_eprintf (sd, "Simulator compiled for forced alignment only.\n");
	  break;
	default: abort ();
	}
      return SIM_RC_FAIL;

    case OPTION_DEBUG:
      if (! WITH_DEBUG)
	sim_io_eprintf (sd, "Debugging not compiled in, `-D' ignored\n");
      else
	{
	  for (n = 0; n < MAX_NR_PROCESSORS; ++n)
	    for (i = 0; i < MAX_DEBUG_VALUES; ++i)
	      CPU_DEBUG_FLAGS (STATE_CPU (sd, n))[i] = 1;
	}
      break;

    case OPTION_DEBUG_INSN :
      if (! WITH_DEBUG)
	sim_io_eprintf (sd, "Debugging not compiled in, `--debug-insn' ignored\n");
      else
	{
	  for (n = 0; n < MAX_NR_PROCESSORS; ++n)
	    CPU_DEBUG_FLAGS (STATE_CPU (sd, n))[DEBUG_INSN_IDX] = 1;
	}
      break;

    case OPTION_DEBUG_FILE :
      if (! WITH_DEBUG)
	sim_io_eprintf (sd, "Debugging not compiled in, `--debug-file' ignored\n");
      else
	{
	  FILE *f = fopen (arg, "w");

	  if (f == NULL)
	    {
	      sim_io_eprintf (sd, "Unable to open debug output file `%s'\n", arg);
	      return SIM_RC_FAIL;
	    }
	  for (n = 0; n < MAX_NR_PROCESSORS; ++n)
	    CPU_DEBUG_FILE (STATE_CPU (sd, n)) = f;
	}
      break;

    case OPTION_DO_COMMAND:
      sim_do_command (sd, arg);
      break;

    case OPTION_ARCHITECTURE:
      {
	const struct bfd_arch_info *ap = bfd_scan_arch (arg);
	if (ap == NULL)
	  {
	    sim_io_eprintf (sd, "Architecture `%s' unknown\n", arg);
	    return SIM_RC_FAIL;
	  }
	STATE_ARCHITECTURE (sd) = ap;
	break;
      }

    case OPTION_ARCHITECTURE_INFO:
      {
	const char **list = bfd_arch_list ();
	const char **lp;
	if (list == NULL)
	  abort ();
	sim_io_printf (sd, "Possible architectures:");
	for (lp = list; *lp != NULL; lp++)
	  sim_io_printf (sd, " %s", *lp);
	sim_io_printf (sd, "\n");
	free (list);
	break;
      }

    case OPTION_TARGET:
      {
	STATE_TARGET (sd) = xstrdup (arg);
	break;
      }

    case OPTION_TARGET_INFO:
      {
	const char **list = bfd_target_list ();
	const char **lp;
	if (list == NULL)
	  abort ();
	sim_io_printf (sd, "Possible targets:");
	for (lp = list; *lp != NULL; lp++)
	  sim_io_printf (sd, " %s", *lp);
	sim_io_printf (sd, "\n");
	free (list);
	break;
      }

    case OPTION_LOAD_LMA:
      {
	STATE_LOAD_AT_LMA_P (sd) = 1;
	break;
      }

    case OPTION_LOAD_VMA:
      {
	STATE_LOAD_AT_LMA_P (sd) = 0;
	break;
      }

    case OPTION_HELP:
      sim_print_help (sd, is_command);
      if (STATE_OPEN_KIND (sd) == SIM_OPEN_STANDALONE)
	exit (0);
      /* FIXME: 'twould be nice to do something similar if gdb.  */
      break;

    case OPTION_VERSION:
      sim_print_version (sd, is_command);
      if (STATE_OPEN_KIND (sd) == SIM_OPEN_STANDALONE)
	exit (0);
      break;

    case OPTION_SYSROOT:
      /* Don't leak memory in the odd event that there's lots of
	 --sysroot=... options.  We treat "" specially since this
	 is the statically initialized value and cannot free it.  */
      if (simulator_sysroot[0] != '\0')
	free (simulator_sysroot);
      if (arg[0] != '\0')
	simulator_sysroot = xstrdup (arg);
      else
	simulator_sysroot = "";
      break;

    case OPTION_ARGV0:
      free (STATE_PROG_ARGV0 (sd));
      STATE_PROG_ARGV0 (sd) = xstrdup (arg);
      break;

    case OPTION_ENV_SET:
      return env_set (sd, arg);

    case OPTION_ENV_UNSET:
      {
	int varlen;
	char **envp;

	if (STATE_PROG_ENVP (sd) == NULL)
	  STATE_PROG_ENVP (sd) = dupargv (environ);

	varlen = strlen (arg);

	/* If we can find an existing variable, replace it.  */
	envp = STATE_PROG_ENVP (sd);
	for (i = 0; envp[i]; ++i)
	  {
	    char *env = envp[i];

	    if (strncmp (env, arg, varlen) == 0
		&& (env[varlen] == '\0' || env[varlen] == '='))
	      {
		free (envp[i]);
		break;
	      }
	  }

	/* If we clear the var, shift the array down.  */
	for (; envp[i]; ++i)
	  envp[i] = envp[i + 1];

	break;
      }

    case OPTION_ENV_CLEAR:
      freeargv (STATE_PROG_ENVP (sd));
      STATE_PROG_ENVP (sd) = xmalloc (sizeof (char *));
      STATE_PROG_ENVP (sd)[0] = NULL;
      break;
    }

  return SIM_RC_OK;
}

/* Add the standard option list to the simulator.  */

SIM_RC
standard_install (SIM_DESC sd)
{
  SIM_ASSERT (STATE_MAGIC (sd) == SIM_MAGIC_NUMBER);
  if (sim_add_option_table (sd, NULL, standard_options) != SIM_RC_OK)
    return SIM_RC_FAIL;
  STATE_LOAD_AT_LMA_P (sd) = 1;
  return SIM_RC_OK;
}

/* Return non-zero if arg is a duplicate argument.
   If ARG is NULL, initialize.  */

static int
dup_arg_p (const char *arg)
{
  static htab_t arg_table = NULL;
  void **slot;

  if (arg == NULL)
    {
      if (arg_table == NULL)
	arg_table = htab_create_alloc (10, htab_hash_string,
				       htab_eq_string, NULL,
				       xcalloc, free);
      htab_empty (arg_table);
      return 0;
    }

  slot = htab_find_slot (arg_table, arg, INSERT);
  if (*slot != NULL)
    return 1;
  *slot = (void *) arg;
  return 0;
}

/* Called by sim_open to parse the arguments.  */

SIM_RC
sim_parse_args (SIM_DESC sd, char * const *argv)
{
  int c, i, argc, num_opts, save_opterr;
  char *p, *short_options;
  /* The `val' option struct entry is dynamically assigned for options that
     only come in the long form.  ORIG_VAL is used to get the original value
     back.  */
  int *orig_val;
  struct option *lp, *long_options;
  const struct option_list *ol;
  const OPTION *opt;
  OPTION_HANDLER **handlers;
  sim_cpu **opt_cpu;
  SIM_RC result = SIM_RC_OK;

  /* Count the number of arguments.  */
  argc = countargv (argv);

  /* Count the number of options.  */
  num_opts = 0;
  for (ol = STATE_OPTIONS (sd); ol != NULL; ol = ol->next)
    for (opt = ol->options; OPTION_VALID_P (opt); ++opt)
      ++num_opts;
  for (i = 0; i < MAX_NR_PROCESSORS; ++i)
    for (ol = CPU_OPTIONS (STATE_CPU (sd, i)); ol != NULL; ol = ol->next)
      for (opt = ol->options; OPTION_VALID_P (opt); ++opt)
	++num_opts;

  /* Initialize duplicate argument checker.  */
  (void) dup_arg_p (NULL);

  /* Build the option table for getopt.  */

  long_options = NZALLOC (struct option, num_opts + 1);
  lp = long_options;
  short_options = NZALLOC (char, num_opts * 3 + 1);
  p = short_options;
  handlers = NZALLOC (OPTION_HANDLER *, OPTION_START + num_opts);
  orig_val = NZALLOC (int, OPTION_START + num_opts);
  opt_cpu = NZALLOC (sim_cpu *, OPTION_START + num_opts);

  /* Set '+' as first char so argument permutation isn't done.  This
     is done to stop getopt_long returning options that appear after
     the target program.  Such options should be passed unchanged into
     the program image. */
  *p++ = '+';

  for (i = OPTION_START, ol = STATE_OPTIONS (sd); ol != NULL; ol = ol->next)
    for (opt = ol->options; OPTION_VALID_P (opt); ++opt)
      {
	if (dup_arg_p (opt->opt.name))
	  continue;
	if (opt->shortopt != 0)
	  {
	    *p++ = opt->shortopt;
	    if (opt->opt.has_arg == required_argument)
	      *p++ = ':';
	    else if (opt->opt.has_arg == optional_argument)
	      { *p++ = ':'; *p++ = ':'; }
	    handlers[(unsigned char) opt->shortopt] = opt->handler;
	    if (opt->opt.val != 0)
	      orig_val[(unsigned char) opt->shortopt] = opt->opt.val;
	    else
	      orig_val[(unsigned char) opt->shortopt] = opt->shortopt;
	  }
	if (opt->opt.name != NULL)
	  {
	    *lp = opt->opt;
	    /* Dynamically assign `val' numbers for long options. */
	    lp->val = i++;
	    handlers[lp->val] = opt->handler;
	    orig_val[lp->val] = opt->opt.val;
	    opt_cpu[lp->val] = NULL;
	    ++lp;
	  }
      }

  for (c = 0; c < MAX_NR_PROCESSORS; ++c)
    {
      sim_cpu *cpu = STATE_CPU (sd, c);
      for (ol = CPU_OPTIONS (cpu); ol != NULL; ol = ol->next)
	for (opt = ol->options; OPTION_VALID_P (opt); ++opt)
	  {
#if 0 /* Each option is prepended with --<cpuname>- so this greatly cuts down
	 on the need for dup_arg_p checking.  Maybe in the future it'll be
	 needed so this is just commented out, and not deleted.  */
	    if (dup_arg_p (opt->opt.name))
	      continue;
#endif
	    /* Don't allow short versions of cpu specific options for now.  */
	    if (opt->shortopt != 0)
	      {
		sim_io_eprintf (sd, "internal error, short cpu specific option");
		result = SIM_RC_FAIL;
		break;
	      }
	    if (opt->opt.name != NULL)
	      {
		char *name;
		*lp = opt->opt;
		/* Prepend --<cpuname>- to the option.  */
		if (asprintf (&name, "%s-%s", CPU_NAME (cpu), lp->name) < 0)
		  {
		    sim_io_eprintf (sd, "internal error, out of memory");
		    result = SIM_RC_FAIL;
		    break;
		  }
		lp->name = name;
		/* Dynamically assign `val' numbers for long options. */
		lp->val = i++;
		handlers[lp->val] = opt->handler;
		orig_val[lp->val] = opt->opt.val;
		opt_cpu[lp->val] = cpu;
		++lp;
	      }
	  }
    }

  /* Terminate the short and long option lists.  */
  *p = 0;
  lp->name = NULL;

  /* Ensure getopt is initialized.  */
  optind = 0;

  /* Do not lot getopt throw errors for us.  But don't mess with the state for
     any callers higher up by saving/restoring it.  */
  save_opterr = opterr;
  opterr = 0;

  while (1)
    {
      int longind, optc;

      optc = getopt_long (argc, argv, short_options, long_options, &longind);
      if (optc == -1)
	{
	  if (STATE_OPEN_KIND (sd) == SIM_OPEN_STANDALONE)
	    {
	      char **new_argv;

	      free (STATE_PROG_FILE (sd));
	      STATE_PROG_FILE (sd) = NULL;

	      /* Handle any inline variables if -- wasn't used.  */
	      if (argv[optind] != NULL && optind > 0
		  && strcmp (argv[optind - 1], "--") != 0)
		{
		  while (1)
		    {
		      const char *arg = argv[optind];

		      if (strchr (arg, '=') == NULL)
			break;

		      env_set (sd, arg);
		      ++optind;
		    }
		}

	      new_argv = dupargv (argv + optind);
	      freeargv (STATE_PROG_ARGV (sd));
	      STATE_PROG_ARGV (sd) = new_argv;

	      /* Skip steps when argc == 0.  */
	      if (argv[optind] != NULL)
		{
		  STATE_PROG_FILE (sd) = xstrdup (argv[optind]);

		  if (STATE_PROG_ARGV0 (sd) != NULL)
		    {
		      free (new_argv[0]);
		      new_argv[0] = xstrdup (STATE_PROG_ARGV0 (sd));
		    }
		}
	    }
	  break;
	}
      if (optc == '?')
	{
	  /* If getopt rejects a short option, optopt is set to the bad char.
	     If it rejects a long option, we have to look at optind.  In the
	     short option case, argv could be multiple short options.  */
	  const char *badopt;
	  char optbuf[3];

	  if (optopt)
	    {
	      sprintf (optbuf, "-%c", optopt);
	      badopt = optbuf;
	    }
	  else
	    badopt = argv[optind - 1];

	  sim_io_eprintf (sd,
			  "%s: unrecognized option '%s'\n"
			  "Use --help for a complete list of options.\n",
			  STATE_MY_NAME (sd), badopt);

	  result = SIM_RC_FAIL;
	  break;
	}

      if ((*handlers[optc]) (sd, opt_cpu[optc], orig_val[optc], optarg, 0/*!is_command*/) == SIM_RC_FAIL)
	{
	  result = SIM_RC_FAIL;
	  break;
	}
    }

  opterr = save_opterr;

  free (long_options);
  free (short_options);
  free (handlers);
  free (opt_cpu);
  free (orig_val);
  return result;
}

/* Utility of sim_print_help to print a list of option tables.  */

static void
print_help (SIM_DESC sd, sim_cpu *cpu, const struct option_list *ol, int is_command)
{
  const OPTION *opt;

  for ( ; ol != NULL; ol = ol->next)
    for (opt = ol->options; OPTION_VALID_P (opt); ++opt)
      {
	const int indent = 30;
	int comma, len;
	const OPTION *o;

	if (dup_arg_p (opt->opt.name))
	  continue;

	if (opt->doc == NULL)
	  continue;

	if (opt->doc_name != NULL && opt->doc_name [0] == '\0')
	  continue;

	sim_io_printf (sd, "  ");

	comma = 0;
	len = 2;

	/* list any short options (aliases) for the current OPT */
	if (!is_command)
	  {
	    o = opt;
	    do
	      {
		if (o->shortopt != '\0')
		  {
		    sim_io_printf (sd, "%s-%c", comma ? ", " : "", o->shortopt);
		    len += (comma ? 2 : 0) + 2;
		    if (o->arg != NULL)
		      {
			if (o->opt.has_arg == optional_argument)
			  {
			    sim_io_printf (sd, "[%s]", o->arg);
			    len += 1 + strlen (o->arg) + 1;
			  }
			else
			  {
			    sim_io_printf (sd, " %s", o->arg);
			    len += 1 + strlen (o->arg);
			  }
		      }
		    comma = 1;
		  }
		++o;
	      }
	    while (OPTION_VALID_P (o) && o->doc == NULL);
	  }

	/* list any long options (aliases) for the current OPT */
	o = opt;
	do
	  {
	    const char *name;
	    const char *cpu_prefix = cpu ? CPU_NAME (cpu) : NULL;
	    if (o->doc_name != NULL)
	      name = o->doc_name;
	    else
	      name = o->opt.name;
	    if (name != NULL)
	      {
		sim_io_printf (sd, "%s%s%s%s%s",
			       comma ? ", " : "",
			       is_command ? "" : "--",
			       cpu ? cpu_prefix : "",
			       cpu ? "-" : "",
			       name);
		len += ((comma ? 2 : 0)
			+ (is_command ? 0 : 2)
			+ strlen (name));
		if (o->arg != NULL)
		  {
		    if (o->opt.has_arg == optional_argument)
		      {
			sim_io_printf (sd, "[=%s]", o->arg);
			len += 2 + strlen (o->arg) + 1;
		      }
		    else
		      {
			sim_io_printf (sd, " %s", o->arg);
			len += 1 + strlen (o->arg);
		      }
		  }
		comma = 1;
	      }
	    ++o;
	  }
	while (OPTION_VALID_P (o) && o->doc == NULL);

	if (len >= indent)
	  {
	    sim_io_printf (sd, "\n%*s", indent, "");
	  }
	else
	  sim_io_printf (sd, "%*s", indent - len, "");

	/* print the description, word wrap long lines */
	{
	  const char *chp = opt->doc;
	  unsigned doc_width = 80 - indent;
	  while (strlen (chp) >= doc_width) /* some slack */
	    {
	      const char *end = chp + doc_width - 1;
	      while (end > chp && !isspace (*end))
		end --;
	      if (end == chp)
		end = chp + doc_width - 1;
	      /* The cast should be ok - its distances between to
                 points in a string.  */
	      sim_io_printf (sd, "%.*s\n%*s", (int) (end - chp), chp, indent,
			     "");
	      chp = end;
	      while (isspace (*chp) && *chp != '\0')
		chp++;
	    }
	  sim_io_printf (sd, "%s\n", chp);
	}
      }
}

/* Print help messages for the options.  */

void
sim_print_help (SIM_DESC sd, int is_command)
{
  if (STATE_OPEN_KIND (sd) == SIM_OPEN_STANDALONE)
    sim_io_printf (sd,
		   "Usage: %s [options] [VAR=VAL|--] program [program args]\n",
		   STATE_MY_NAME (sd));

  /* Initialize duplicate argument checker.  */
  (void) dup_arg_p (NULL);

  if (STATE_OPEN_KIND (sd) == SIM_OPEN_STANDALONE)
    sim_io_printf (sd, "Options:\n");
  else
    sim_io_printf (sd, "Commands:\n");

  print_help (sd, NULL, STATE_OPTIONS (sd), is_command);
  sim_io_printf (sd, "\n");

  /* Print cpu-specific options.  */
  {
    int i;

    for (i = 0; i < MAX_NR_PROCESSORS; ++i)
      {
	sim_cpu *cpu = STATE_CPU (sd, i);
	if (CPU_OPTIONS (cpu) == NULL)
	  continue;
	sim_io_printf (sd, "CPU %s specific options:\n", CPU_NAME (cpu));
	print_help (sd, cpu, CPU_OPTIONS (cpu), is_command);
	sim_io_printf (sd, "\n");
      }
  }

  sim_io_printf (sd, "Note: Depending on the simulator configuration some %ss\n",
		 STATE_OPEN_KIND (sd) == SIM_OPEN_STANDALONE ? "option" : "command");
  sim_io_printf (sd, "      may not be applicable\n");

  if (STATE_OPEN_KIND (sd) == SIM_OPEN_STANDALONE)
    {
      sim_io_printf (sd, "\n");
      sim_io_printf (sd,
		     "VAR=VAL         Environment variables to set.  "
		     "Ignored if -- is used.\n");
      sim_io_printf (sd, "program args    Arguments to pass to simulated program.\n");
      sim_io_printf (sd, "                Note: Very few simulators support this.\n");
    }
}

/* Print version information.  */

void
sim_print_version (SIM_DESC sd, int is_command)
{
  sim_io_printf (sd, "GNU simulator %s%s\n", PKGVERSION, version);

  sim_io_printf (sd, "Copyright (C) 2024 Free Software Foundation, Inc.\n");

  /* Following the copyright is a brief statement that the program is
     free software, that users are free to copy and change it on
     certain conditions, that it is covered by the GNU GPL, and that
     there is no warranty.  */

  sim_io_printf (sd, "\
License GPLv3+: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>\
\nThis is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n");

  if (!is_command)
    return;

  sim_io_printf (sd, "This SIM was configured as:\n");
  sim_config_print (sd);

  if (REPORT_BUGS_TO[0])
    {
      sim_io_printf (sd, "For bug reporting instructions, please see:\n\
    %s.\n",
		     REPORT_BUGS_TO);
    }
  sim_io_printf (sd, "Find the SIM homepage & other documentation resources \
online at:\n    <https://sourceware.org/gdb/wiki/Sim/>.\n");
}

/* Utility of sim_args_command to find the closest match for a command.
   Commands that have "-" in them can be specified as separate words.
   e.g. sim memory-region 0x800000,0x4000
   or   sim memory region 0x800000,0x4000
   If CPU is non-null, use its option table list, otherwise use the main one.
   *PARGI is where to start looking in ARGV.  It is updated to point past
   the found option.  */

static const OPTION *
find_match (SIM_DESC sd, sim_cpu *cpu, char *argv[], int *pargi)
{
  const struct option_list *ol;
  const OPTION *opt;
  /* most recent option match */
  const OPTION *matching_opt = NULL;
  int matching_argi = -1;

  if (cpu)
    ol = CPU_OPTIONS (cpu);
  else
    ol = STATE_OPTIONS (sd);

  /* Skip passed elements specified by *PARGI.  */
  argv += *pargi;

  for ( ; ol != NULL; ol = ol->next)
    for (opt = ol->options; OPTION_VALID_P (opt); ++opt)
      {
	int argi = 0;
	const char *name = opt->opt.name;
	if (name == NULL)
	  continue;
	while (argv [argi] != NULL
	       && strncmp (name, argv [argi], strlen (argv [argi])) == 0)
	  {
	    name = &name [strlen (argv[argi])];
	    if (name [0] == '-')
	      {
		/* leading match ...<a-b-c>-d-e-f - continue search */
		name ++; /* skip `-' */
		argi ++;
		continue;
	      }
	    else if (name [0] == '\0')
	      {
		/* exact match ...<a-b-c-d-e-f> - better than before? */
		if (argi > matching_argi)
		  {
		    matching_argi = argi;
		    matching_opt = opt;
		  }
		break;
	      }
	    else
	      break;
	  }
      }

  *pargi = matching_argi;
  return matching_opt;
}

static char **
complete_option_list (char **ret, size_t *cnt, const struct option_list *ol,
		      const char *text, const char *word)
{
  const OPTION *opt = NULL;
  size_t len = strlen (word);

  for ( ; ol != NULL; ol = ol->next)
    for (opt = ol->options; OPTION_VALID_P (opt); ++opt)
      {
	const char *name = opt->opt.name;

	/* A long option to match against?  */
	if (!name)
	  continue;

	/* Does this option actually match?  */
	if (strncmp (name, word, len))
	  continue;

	ret = xrealloc (ret, ++*cnt * sizeof (ret[0]));
	ret[*cnt - 2] = xstrdup (name);
      }

  return ret;
}

/* All leading text is stored in @text, while the current word being
   completed is stored in @word.  Trailing text of @word is not.  */

char **
sim_complete_command (SIM_DESC sd, const char *text, const char *word)
{
  char **ret = NULL;
  size_t cnt = 1;
  sim_cpu *cpu;

  /* Only complete first word for now.  */
  if (text != word)
    return ret;

  cpu = STATE_CPU (sd, 0);
  if (cpu)
    ret = complete_option_list (ret, &cnt, CPU_OPTIONS (cpu), text, word);
  ret = complete_option_list (ret, &cnt, STATE_OPTIONS (sd), text, word);

  if (ret)
    ret[cnt - 1] = NULL;
  return ret;
}

SIM_RC
sim_args_command (SIM_DESC sd, const char *cmd)
{
  /* something to do? */
  if (cmd == NULL)
    return SIM_RC_OK; /* FIXME - perhaps help would be better */

  if (cmd [0] == '-')
    {
      /* user specified -<opt> ... form? */
      char **argv = buildargv (cmd);
      SIM_RC rc = sim_parse_args (sd, argv);
      freeargv (argv);
      return rc;
    }
  else
    {
      char **argv = buildargv (cmd);
      const OPTION *matching_opt = NULL;
      int matching_argi;
      sim_cpu *cpu;

      if (argv [0] == NULL)
	{
	  freeargv (argv);
	  return SIM_RC_OK; /* FIXME - perhaps help would be better */
	}

      /* First check for a cpu selector.  */
      {
	char *cpu_name = xstrdup (argv[0]);
	char *hyphen = strchr (cpu_name, '-');
	if (hyphen)
	  *hyphen = 0;
	cpu = sim_cpu_lookup (sd, cpu_name);
	if (cpu)
	  {
	    /* If <cpuname>-<command>, point argv[0] at <command>.  */
	    if (hyphen)
	      {
		matching_argi = 0;
		argv[0] += hyphen - cpu_name + 1;
	      }
	    else
	      matching_argi = 1;
	    matching_opt = find_match (sd, cpu, argv, &matching_argi);
	    /* If hyphen found restore argv[0].  */
	    if (hyphen)
	      argv[0] -= hyphen - cpu_name + 1;
	  }
	free (cpu_name);
      }

      /* If that failed, try the main table.  */
      if (matching_opt == NULL)
	{
	  matching_argi = 0;
	  matching_opt = find_match (sd, NULL, argv, &matching_argi);
	}

      if (matching_opt != NULL)
	{
	  switch (matching_opt->opt.has_arg)
	    {
	    case no_argument:
	      if (argv [matching_argi + 1] == NULL)
		matching_opt->handler (sd, cpu, matching_opt->opt.val,
				       NULL, 1/*is_command*/);
	      else
		sim_io_eprintf (sd, "Command `%s' takes no arguments\n",
				matching_opt->opt.name);
	      break;
	    case optional_argument:
	      if (argv [matching_argi + 1] == NULL)
		matching_opt->handler (sd, cpu, matching_opt->opt.val,
				       NULL, 1/*is_command*/);
	      else if (argv [matching_argi + 2] == NULL)
		matching_opt->handler (sd, cpu, matching_opt->opt.val,
				       argv [matching_argi + 1], 1/*is_command*/);
	      else
		sim_io_eprintf (sd, "Command `%s' requires no more than one argument\n",
				matching_opt->opt.name);
	      break;
	    case required_argument:
	      if (argv [matching_argi + 1] == NULL)
		sim_io_eprintf (sd, "Command `%s' requires an argument\n",
				matching_opt->opt.name);
	      else if (argv [matching_argi + 2] == NULL)
		matching_opt->handler (sd, cpu, matching_opt->opt.val,
				       argv [matching_argi + 1], 1/*is_command*/);
	      else
		sim_io_eprintf (sd, "Command `%s' requires only one argument\n",
				matching_opt->opt.name);
	    }
	  freeargv (argv);
	  return SIM_RC_OK;
	}

      freeargv (argv);
    }

  /* didn't find anything that remotly matched */
  return SIM_RC_FAIL;
}
