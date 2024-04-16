/* Tracing functionality for remote targets in custom GDB protocol

   Copyright (C) 1997-2024 Free Software Foundation, Inc.

   This file is part of GDB.

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

#include "defs.h"
#include "arch-utils.h"
#include "symtab.h"
#include "frame.h"
#include "gdbtypes.h"
#include "expression.h"
#include "gdbcmd.h"
#include "value.h"
#include "target.h"
#include "target-dcache.h"
#include "language.h"
#include "inferior.h"
#include "breakpoint.h"
#include "tracepoint.h"
#include "linespec.h"
#include "regcache.h"
#include "completer.h"
#include "block.h"
#include "dictionary.h"
#include "observable.h"
#include "user-regs.h"
#include "valprint.h"
#include "gdbcore.h"
#include "objfiles.h"
#include "filenames.h"
#include "gdbthread.h"
#include "stack.h"
#include "remote.h"
#include "source.h"
#include "ax.h"
#include "ax-gdb.h"
#include "memrange.h"
#include "cli/cli-utils.h"
#include "probe.h"
#include "gdbsupport/filestuff.h"
#include "gdbsupport/rsp-low.h"
#include "tracefile.h"
#include "location.h"
#include <algorithm>
#include "cli/cli-style.h"
#include "expop.h"
#include "gdbsupport/buildargv.h"
#include "interps.h"

#include <unistd.h>

/* Maximum length of an agent aexpression.
   This accounts for the fact that packets are limited to 400 bytes
   (which includes everything -- including the checksum), and assumes
   the worst case of maximum length for each of the pieces of a
   continuation packet.

   NOTE: expressions get bin2hex'ed otherwise this would be twice as
   large.  (400 - 31)/2 == 184 */
#define MAX_AGENT_EXPR_LEN	184

/* 
   Tracepoint.c:

   This module defines the following debugger commands:
   trace            : set a tracepoint on a function, line, or address.
   info trace       : list all debugger-defined tracepoints.
   delete trace     : delete one or more tracepoints.
   enable trace     : enable one or more tracepoints.
   disable trace    : disable one or more tracepoints.
   actions          : specify actions to be taken at a tracepoint.
   passcount        : specify a pass count for a tracepoint.
   tstart           : start a trace experiment.
   tstop            : stop a trace experiment.
   tstatus          : query the status of a trace experiment.
   tfind            : find a trace frame in the trace buffer.
   tdump            : print everything collected at the current tracepoint.
   save-tracepoints : write tracepoint setup into a file.

   This module defines the following user-visible debugger variables:
   $trace_frame : sequence number of trace frame currently being debugged.
   $trace_line  : source line of trace frame currently being debugged.
   $trace_file  : source file of trace frame currently being debugged.
   $tracepoint  : tracepoint number of trace frame currently being debugged.
 */


/* ======= Important global variables: ======= */

/* The list of all trace state variables.  We don't retain pointers to
   any of these for any reason - API is by name or number only - so it
   works to have a vector of objects.  */

static std::vector<trace_state_variable> tvariables;

/* The next integer to assign to a variable.  */

static int next_tsv_number = 1;

/* Number of last traceframe collected.  */
static int traceframe_number;

/* Tracepoint for last traceframe collected.  */
static int tracepoint_number;

/* The traceframe info of the current traceframe.  NULL if we haven't
   yet attempted to fetch it, or if the target does not support
   fetching this object, or if we're not inspecting a traceframe
   presently.  */
static traceframe_info_up current_traceframe_info;

/* Tracing command lists.  */
static struct cmd_list_element *tfindlist;

/* List of expressions to collect by default at each tracepoint hit.  */
std::string default_collect;

static bool disconnected_tracing;

/* This variable controls whether we ask the target for a linear or
   circular trace buffer.  */

static bool circular_trace_buffer;

/* This variable is the requested trace buffer size, or -1 to indicate
   that we don't care and leave it up to the target to set a size.  */

static int trace_buffer_size = -1;

/* Textual notes applying to the current and/or future trace runs.  */

static std::string trace_user;

/* Textual notes applying to the current and/or future trace runs.  */

static std::string trace_notes;

/* Textual notes applying to the stopping of a trace.  */

static std::string trace_stop_notes;

/* support routines */

struct collection_list;

static counted_command_line all_tracepoint_actions (tracepoint *);

static struct trace_status trace_status;

const char *stop_reason_names[] = {
  "tunknown",
  "tnotrun",
  "tstop",
  "tfull",
  "tdisconnected",
  "tpasscount",
  "terror"
};

struct trace_status *
current_trace_status (void)
{
  return &trace_status;
}

/* Free and clear the traceframe info cache of the current
   traceframe.  */

static void
clear_traceframe_info (void)
{
  current_traceframe_info = NULL;
}

/* Set traceframe number to NUM.  */
static void
set_traceframe_num (int num)
{
  traceframe_number = num;
  set_internalvar_integer (lookup_internalvar ("trace_frame"), num);
}

/* Set tracepoint number to NUM.  */
static void
set_tracepoint_num (int num)
{
  tracepoint_number = num;
  set_internalvar_integer (lookup_internalvar ("tracepoint"), num);
}

/* Set externally visible debug variables for querying/printing
   the traceframe context (line, function, file).  */

static void
set_traceframe_context (frame_info_ptr trace_frame)
{
  CORE_ADDR trace_pc;
  struct symbol *traceframe_fun;
  symtab_and_line traceframe_sal;

  /* Save as globals for internal use.  */
  if (trace_frame != NULL
      && get_frame_pc_if_available (trace_frame, &trace_pc))
    {
      traceframe_sal = find_pc_line (trace_pc, 0);
      traceframe_fun = find_pc_function (trace_pc);

      /* Save linenumber as "$trace_line", a debugger variable visible to
	 users.  */
      set_internalvar_integer (lookup_internalvar ("trace_line"),
			       traceframe_sal.line);
    }
  else
    {
      traceframe_fun = NULL;
      set_internalvar_integer (lookup_internalvar ("trace_line"), -1);
    }

  /* Save func name as "$trace_func", a debugger variable visible to
     users.  */
  if (traceframe_fun == NULL
      || traceframe_fun->linkage_name () == NULL)
    clear_internalvar (lookup_internalvar ("trace_func"));
  else
    set_internalvar_string (lookup_internalvar ("trace_func"),
			    traceframe_fun->linkage_name ());

  /* Save file name as "$trace_file", a debugger variable visible to
     users.  */
  if (traceframe_sal.symtab == NULL)
    clear_internalvar (lookup_internalvar ("trace_file"));
  else
    set_internalvar_string (lookup_internalvar ("trace_file"),
			symtab_to_filename_for_display (traceframe_sal.symtab));
}

/* Create a new trace state variable with the given name.  */

struct trace_state_variable *
create_trace_state_variable (const char *name)
{
  tvariables.emplace_back (name, next_tsv_number++);
  return &tvariables.back ();
}

/* Look for a trace state variable of the given name.  */

struct trace_state_variable *
find_trace_state_variable (const char *name)
{
  for (trace_state_variable &tsv : tvariables)
    if (tsv.name == name)
      return &tsv;

  return NULL;
}

/* Look for a trace state variable of the given number.  Return NULL if
   not found.  */

struct trace_state_variable *
find_trace_state_variable_by_number (int number)
{
  for (trace_state_variable &tsv : tvariables)
    if (tsv.number == number)
      return &tsv;

  return NULL;
}

static void
delete_trace_state_variable (const char *name)
{
  for (auto it = tvariables.begin (); it != tvariables.end (); it++)
    if (it->name == name)
      {
	interps_notify_tsv_deleted (&*it);
	tvariables.erase (it);
	return;
      }

  warning (_("No trace variable named \"$%s\", not deleting"), name);
}

/* Throws an error if NAME is not valid syntax for a trace state
   variable's name.  */

void
validate_trace_state_variable_name (const char *name)
{
  const char *p;

  if (*name == '\0')
    error (_("Must supply a non-empty variable name"));

  /* All digits in the name is reserved for value history
     references.  */
  for (p = name; isdigit (*p); p++)
    ;
  if (*p == '\0')
    error (_("$%s is not a valid trace state variable name"), name);

  for (p = name; isalnum (*p) || *p == '_'; p++)
    ;
  if (*p != '\0')
    error (_("$%s is not a valid trace state variable name"), name);
}

/* The 'tvariable' command collects a name and optional expression to
   evaluate into an initial value.  */

static void
trace_variable_command (const char *args, int from_tty)
{
  LONGEST initval = 0;
  struct trace_state_variable *tsv;
  const char *name_start, *p;

  if (!args || !*args)
    error_no_arg (_("Syntax is $NAME [ = EXPR ]"));

  /* Only allow two syntaxes; "$name" and "$name=value".  */
  p = skip_spaces (args);

  if (*p++ != '$')
    error (_("Name of trace variable should start with '$'"));

  name_start = p;
  while (isalnum (*p) || *p == '_')
    p++;
  std::string name (name_start, p - name_start);

  p = skip_spaces (p);
  if (*p != '=' && *p != '\0')
    error (_("Syntax must be $NAME [ = EXPR ]"));

  validate_trace_state_variable_name (name.c_str ());

  if (*p == '=')
    initval = value_as_long (parse_and_eval (++p));

  /* If the variable already exists, just change its initial value.  */
  tsv = find_trace_state_variable (name.c_str ());
  if (tsv)
    {
      if (tsv->initial_value != initval)
	{
	  tsv->initial_value = initval;
	  interps_notify_tsv_modified (tsv);
	}
      gdb_printf (_("Trace state variable $%s "
		    "now has initial value %s.\n"),
		  tsv->name.c_str (), plongest (tsv->initial_value));
      return;
    }

  /* Create a new variable.  */
  tsv = create_trace_state_variable (name.c_str ());
  tsv->initial_value = initval;

  interps_notify_tsv_created (tsv);

  gdb_printf (_("Trace state variable $%s "
		"created, with initial value %s.\n"),
	      tsv->name.c_str (), plongest (tsv->initial_value));
}

static void
delete_trace_variable_command (const char *args, int from_tty)
{
  if (args == NULL)
    {
      if (query (_("Delete all trace state variables? ")))
	tvariables.clear ();
      dont_repeat ();
      interps_notify_tsv_deleted (nullptr);
      return;
    }

  gdb_argv argv (args);

  for (char *arg : argv)
    {
      if (*arg == '$')
	delete_trace_state_variable (arg + 1);
      else
	warning (_("Name \"%s\" not prefixed with '$', ignoring"), arg);
    }

  dont_repeat ();
}

void
tvariables_info_1 (void)
{
  struct ui_out *uiout = current_uiout;

  /* Try to acquire values from the target.  */
  for (trace_state_variable &tsv : tvariables)
    tsv.value_known
      = target_get_trace_state_variable_value (tsv.number, &tsv.value);

  {
    ui_out_emit_table table_emitter (uiout, 3, tvariables.size (),
				     "trace-variables");
    uiout->table_header (15, ui_left, "name", "Name");
    uiout->table_header (11, ui_left, "initial", "Initial");
    uiout->table_header (11, ui_left, "current", "Current");

    uiout->table_body ();

    for (const trace_state_variable &tsv : tvariables)
      {
	const char *c;

	ui_out_emit_tuple tuple_emitter (uiout, "variable");

	uiout->field_string ("name", std::string ("$") + tsv.name);
	uiout->field_string ("initial", plongest (tsv.initial_value));

	ui_file_style style;
	if (tsv.value_known)
	  c = plongest (tsv.value);
	else if (uiout->is_mi_like_p ())
	  /* For MI, we prefer not to use magic string constants, but rather
	     omit the field completely.  The difference between unknown and
	     undefined does not seem important enough to represent.  */
	  c = NULL;
	else if (current_trace_status ()->running || traceframe_number >= 0)
	  {
	    /* The value is/was defined, but we don't have it.  */
	    c = "<unknown>";
	    style = metadata_style.style ();
	  }
	else
	  {
	    /* It is not meaningful to ask about the value.  */
	    c = "<undefined>";
	    style = metadata_style.style ();
	  }
	if (c)
	  uiout->field_string ("current", c, style);
	uiout->text ("\n");
      }
  }

  if (tvariables.empty ())
    uiout->text (_("No trace state variables.\n"));
}

/* List all the trace state variables.  */

static void
info_tvariables_command (const char *args, int from_tty)
{
  tvariables_info_1 ();
}

/* Stash definitions of tsvs into the given file.  */

void
save_trace_state_variables (struct ui_file *fp)
{
  for (const trace_state_variable &tsv : tvariables)
    {
      gdb_printf (fp, "tvariable $%s", tsv.name.c_str ());
      if (tsv.initial_value)
	gdb_printf (fp, " = %s", plongest (tsv.initial_value));
      gdb_printf (fp, "\n");
    }
}

/* ACTIONS functions: */

/* The three functions:
   collect_pseudocommand, 
   while_stepping_pseudocommand, and 
   end_actions_pseudocommand
   are placeholders for "commands" that are actually ONLY to be used
   within a tracepoint action list.  If the actual function is ever called,
   it means that somebody issued the "command" at the top level,
   which is always an error.  */

static void
end_actions_pseudocommand (const char *args, int from_tty)
{
  error (_("This command cannot be used at the top level."));
}

static void
while_stepping_pseudocommand (const char *args, int from_tty)
{
  error (_("This command can only be used in a tracepoint actions list."));
}

static void
collect_pseudocommand (const char *args, int from_tty)
{
  error (_("This command can only be used in a tracepoint actions list."));
}

static void
teval_pseudocommand (const char *args, int from_tty)
{
  error (_("This command can only be used in a tracepoint actions list."));
}

/* Parse any collection options, such as /s for strings.  */

const char *
decode_agent_options (const char *exp, int *trace_string)
{
  struct value_print_options opts;

  *trace_string = 0;

  if (*exp != '/')
    return exp;

  /* Call this to borrow the print elements default for collection
     size.  */
  get_user_print_options (&opts);

  exp++;
  if (*exp == 's')
    {
      if (target_supports_string_tracing ())
	{
	  /* Allow an optional decimal number giving an explicit maximum
	     string length, defaulting it to the "print characters" value;
	     so "collect/s80 mystr" gets at most 80 bytes of string.  */
	  *trace_string = get_print_max_chars (&opts);
	  exp++;
	  if (*exp >= '0' && *exp <= '9')
	    *trace_string = atoi (exp);
	  while (*exp >= '0' && *exp <= '9')
	    exp++;
	}
      else
	error (_("Target does not support \"/s\" option for string tracing."));
    }
  else
    error (_("Undefined collection format \"%c\"."), *exp);

  exp = skip_spaces (exp);

  return exp;
}

/* Enter a list of actions for a tracepoint.  */
static void
actions_command (const char *args, int from_tty)
{
  struct tracepoint *t;

  t = get_tracepoint_by_number (&args, NULL);
  if (t)
    {
      std::string tmpbuf =
	string_printf ("Enter actions for tracepoint %d, one per line.",
		       t->number);

      counted_command_line l = read_command_lines (tmpbuf.c_str (),
						   from_tty, 1,
						   [=] (const char *line)
						     {
						       validate_actionline (line, t);
						     });
      breakpoint_set_commands (t, std::move (l));
    }
  /* else just return */
}

/* Report the results of checking the agent expression, as errors or
   internal errors.  */

static void
report_agent_reqs_errors (struct agent_expr *aexpr)
{
  /* All of the "flaws" are serious bytecode generation issues that
     should never occur.  */
  if (aexpr->flaw != agent_flaw_none)
    internal_error (_("expression is malformed"));

  /* If analysis shows a stack underflow, GDB must have done something
     badly wrong in its bytecode generation.  */
  if (aexpr->min_height < 0)
    internal_error (_("expression has min height < 0"));

  /* Issue this error if the stack is predicted to get too deep.  The
     limit is rather arbitrary; a better scheme might be for the
     target to report how much stack it will have available.  The
     depth roughly corresponds to parenthesization, so a limit of 20
     amounts to 20 levels of expression nesting, which is actually
     a pretty big hairy expression.  */
  if (aexpr->max_height > 20)
    error (_("Expression is too complicated."));
}

/* Call ax_reqs on AEXPR and raise an error if something is wrong.  */

static void
finalize_tracepoint_aexpr (struct agent_expr *aexpr)
{
  ax_reqs (aexpr);

  if (aexpr->buf.size () > MAX_AGENT_EXPR_LEN)
    error (_("Expression is too complicated."));

  report_agent_reqs_errors (aexpr);
}

/* worker function */
void
validate_actionline (const char *line, tracepoint *t)
{
  struct cmd_list_element *c;
  const char *tmp_p;
  const char *p;

  /* If EOF is typed, *line is NULL.  */
  if (line == NULL)
    return;

  p = skip_spaces (line);

  /* Symbol lookup etc.  */
  if (*p == '\0')	/* empty line: just prompt for another line.  */
    return;

  if (*p == '#')		/* comment line */
    return;

  c = lookup_cmd (&p, cmdlist, "", NULL, -1, 1);
  if (c == 0)
    error (_("`%s' is not a tracepoint action, or is ambiguous."), p);

  if (cmd_simple_func_eq (c, collect_pseudocommand))
    {
      int trace_string = 0;

      if (*p == '/')
	p = decode_agent_options (p, &trace_string);

      do
	{			/* Repeat over a comma-separated list.  */
	  QUIT;			/* Allow user to bail out with ^C.  */
	  p = skip_spaces (p);

	  if (*p == '$')	/* Look for special pseudo-symbols.  */
	    {
	      if (0 == strncasecmp ("reg", p + 1, 3)
		  || 0 == strncasecmp ("arg", p + 1, 3)
		  || 0 == strncasecmp ("loc", p + 1, 3)
		  || 0 == strncasecmp ("_ret", p + 1, 4)
		  || 0 == strncasecmp ("_sdata", p + 1, 6))
		{
		  p = strchr (p, ',');
		  continue;
		}
	      /* else fall thru, treat p as an expression and parse it!  */
	    }
	  tmp_p = p;
	  for (bp_location &loc : t->locations ())
	    {
	      p = tmp_p;
	      expression_up exp = parse_exp_1 (&p, loc.address,
					       block_for_pc (loc.address),
					       PARSER_COMMA_TERMINATES);

	      if (exp->first_opcode () == OP_VAR_VALUE)
		{
		  symbol *sym;
		  expr::var_value_operation *vvop
		    = (gdb::checked_static_cast<expr::var_value_operation *>
		       (exp->op.get ()));
		  sym = vvop->get_symbol ();

		  if (sym->aclass () == LOC_CONST)
		    {
		      error (_("constant `%s' (value %s) "
			       "will not be collected."),
			     sym->print_name (),
			     plongest (sym->value_longest ()));
		    }
		  else if (sym->aclass () == LOC_OPTIMIZED_OUT)
		    {
		      error (_("`%s' is optimized away "
			       "and cannot be collected."),
			     sym->print_name ());
		    }
		}

	      /* We have something to collect, make sure that the expr to
		 bytecode translator can handle it and that it's not too
		 long.  */
	      agent_expr_up aexpr = gen_trace_for_expr (loc.address,
							exp.get (),
							trace_string);

	      finalize_tracepoint_aexpr (aexpr.get ());
	    }
	}
      while (p && *p++ == ',');
    }

  else if (cmd_simple_func_eq (c, teval_pseudocommand))
    {
      do
	{			/* Repeat over a comma-separated list.  */
	  QUIT;			/* Allow user to bail out with ^C.  */
	  p = skip_spaces (p);

	  tmp_p = p;
	  for (bp_location &loc : t->locations ())
	    {
	      p = tmp_p;

	      /* Only expressions are allowed for this action.  */
	      expression_up exp = parse_exp_1 (&p, loc.address,
					       block_for_pc (loc.address),
					       PARSER_COMMA_TERMINATES);

	      /* We have something to evaluate, make sure that the expr to
		 bytecode translator can handle it and that it's not too
		 long.  */
	      agent_expr_up aexpr = gen_eval_for_expr (loc.address, exp.get ());

	      finalize_tracepoint_aexpr (aexpr.get ());
	    }
	}
      while (p && *p++ == ',');
    }

  else if (cmd_simple_func_eq (c, while_stepping_pseudocommand))
    {
      char *endp;

      p = skip_spaces (p);
      t->step_count = strtol (p, &endp, 0);
      if (endp == p || t->step_count == 0)
	error (_("while-stepping step count `%s' is malformed."), line);
      p = endp;
    }

  else if (cmd_simple_func_eq (c, end_actions_pseudocommand))
    ;

  else
    error (_("`%s' is not a supported tracepoint action."), line);
}

enum {
  memrange_absolute = -1
};

/* MEMRANGE functions: */

/* Compare memranges for std::sort.  */

static bool
memrange_comp (const memrange &a, const memrange &b)
{
  if (a.type == b.type)
    {
      if (a.type == memrange_absolute)
	return (bfd_vma) a.start < (bfd_vma) b.start;
      else
	return a.start < b.start;
    }

  return a.type < b.type;
}

/* Sort the memrange list using std::sort, and merge adjacent memranges.  */

static void
memrange_sortmerge (std::vector<memrange> &memranges)
{
  if (!memranges.empty ())
    {
      int a, b;

      std::sort (memranges.begin (), memranges.end (), memrange_comp);

      for (a = 0, b = 1; b < memranges.size (); b++)
	{
	  /* If memrange b overlaps or is adjacent to memrange a,
	     merge them.  */
	  if (memranges[a].type == memranges[b].type
	      && memranges[b].start <= memranges[a].end)
	    {
	      if (memranges[b].end > memranges[a].end)
		memranges[a].end = memranges[b].end;
	      continue;		/* next b, same a */
	    }
	  a++;			/* next a */
	  if (a != b)
	    memranges[a] = memranges[b];
	}
      memranges.resize (a + 1);
    }
}

/* Add remote register number REGNO to the collection list mask.  */

void
collection_list::add_remote_register (unsigned int regno)
{
  if (info_verbose)
    gdb_printf ("collect register %d\n", regno);

  m_regs_mask.at (regno / 8) |= 1 << (regno % 8);
}

/* Add all the registers from the mask in AEXPR to the mask in the
   collection list.  Registers in the AEXPR mask are already remote
   register numbers.  */

void
collection_list::add_ax_registers (struct agent_expr *aexpr)
{
  for (int ndx1 = 0; ndx1 < aexpr->reg_mask.size (); ndx1++)
    {
      QUIT;	/* Allow user to bail out with ^C.  */
      if (aexpr->reg_mask[ndx1])
	{
	  /* It's used -- record it.  */
	  add_remote_register (ndx1);
	}
    }
}

/* If REGNO is raw, add its corresponding remote register number to
   the mask.  If REGNO is a pseudo-register, figure out the necessary
   registers using a temporary agent expression, and add it to the
   list if it needs more than just a mask.  */

void
collection_list::add_local_register (struct gdbarch *gdbarch,
				     unsigned int regno,
				     CORE_ADDR scope)
{
  if (regno < gdbarch_num_regs (gdbarch))
    {
      int remote_regno = gdbarch_remote_register_number (gdbarch, regno);

      if (remote_regno < 0)
	error (_("Can't collect register %d"), regno);

      add_remote_register (remote_regno);
    }
  else
    {
      agent_expr_up aexpr (new agent_expr (gdbarch, scope));

      ax_reg_mask (aexpr.get (), regno);

      finalize_tracepoint_aexpr (aexpr.get ());

      add_ax_registers (aexpr.get ());

      /* Usually ax_reg_mask for a pseudo-regiser only sets the
	 corresponding raw registers in the ax mask, but if this isn't
	 the case add the expression that is generated to the
	 collection list.  */
      if (aexpr->buf.size () > 0)
	add_aexpr (std::move (aexpr));
    }
}

/* Add a memrange to a collection list.  */

void
collection_list::add_memrange (struct gdbarch *gdbarch,
			       int type, bfd_signed_vma base,
			       unsigned long len, CORE_ADDR scope)
{
  if (info_verbose)
    gdb_printf ("(%d,%s,%ld)\n", type, paddress (gdbarch, base), len);

  /* type: memrange_absolute == memory, other n == basereg */
  /* base: addr if memory, offset if reg relative.  */
  /* len: we actually save end (base + len) for convenience */
  m_memranges.emplace_back (type, base, base + len);

  if (type != memrange_absolute)    /* Better collect the base register!  */
    add_local_register (gdbarch, type, scope);
}

/* Add a symbol to a collection list.  */

void
collection_list::collect_symbol (struct symbol *sym,
				 struct gdbarch *gdbarch,
				 long frame_regno, long frame_offset,
				 CORE_ADDR scope,
				 int trace_string)
{
  unsigned long len;
  unsigned int reg;
  bfd_signed_vma offset;
  int treat_as_expr = 0;

  len = check_typedef (sym->type ())->length ();
  switch (sym->aclass ())
    {
    default:
      gdb_printf ("%s: don't know symbol class %d\n",
		  sym->print_name (), sym->aclass ());
      break;
    case LOC_CONST:
      gdb_printf ("constant %s (value %s) will not be collected.\n",
		  sym->print_name (), plongest (sym->value_longest ()));
      break;
    case LOC_STATIC:
      offset = sym->value_address ();
      if (info_verbose)
	{
	  gdb_printf ("LOC_STATIC %s: collect %ld bytes at %s.\n",
		      sym->print_name (), len,
		      paddress (gdbarch, offset));
	}
      /* A struct may be a C++ class with static fields, go to general
	 expression handling.  */
      if (sym->type ()->code () == TYPE_CODE_STRUCT)
	treat_as_expr = 1;
      else
	add_memrange (gdbarch, memrange_absolute, offset, len, scope);
      break;
    case LOC_REGISTER:
      reg = SYMBOL_REGISTER_OPS (sym)->register_number (sym, gdbarch);
      if (info_verbose)
	gdb_printf ("LOC_REG[parm] %s: ", sym->print_name ());
      add_local_register (gdbarch, reg, scope);
      /* Check for doubles stored in two registers.  */
      /* FIXME: how about larger types stored in 3 or more regs?  */
      if (sym->type ()->code () == TYPE_CODE_FLT &&
	  len > register_size (gdbarch, reg))
	add_local_register (gdbarch, reg + 1, scope);
      break;
    case LOC_REF_ARG:
      gdb_printf ("Sorry, don't know how to do LOC_REF_ARG yet.\n");
      gdb_printf ("       (will not collect %s)\n", sym->print_name ());
      break;
    case LOC_ARG:
      reg = frame_regno;
      offset = frame_offset + sym->value_longest ();
      if (info_verbose)
	{
	  gdb_printf ("LOC_LOCAL %s: Collect %ld bytes at offset %s"
		      " from frame ptr reg %d\n", sym->print_name (), len,
		      paddress (gdbarch, offset), reg);
	}
      add_memrange (gdbarch, reg, offset, len, scope);
      break;
    case LOC_REGPARM_ADDR:
      reg = sym->value_longest ();
      offset = 0;
      if (info_verbose)
	{
	  gdb_printf ("LOC_REGPARM_ADDR %s: Collect %ld bytes at offset %s"
		      " from reg %d\n", sym->print_name (), len,
		      paddress (gdbarch, offset), reg);
	}
      add_memrange (gdbarch, reg, offset, len, scope);
      break;
    case LOC_LOCAL:
      reg = frame_regno;
      offset = frame_offset + sym->value_longest ();
      if (info_verbose)
	{
	  gdb_printf ("LOC_LOCAL %s: Collect %ld bytes at offset %s"
		      " from frame ptr reg %d\n", sym->print_name (), len,
		      paddress (gdbarch, offset), reg);
	}
      add_memrange (gdbarch, reg, offset, len, scope);
      break;

    case LOC_UNRESOLVED:
      treat_as_expr = 1;
      break;

    case LOC_OPTIMIZED_OUT:
      gdb_printf ("%s has been optimized out of existence.\n",
		  sym->print_name ());
      break;

    case LOC_COMPUTED:
      treat_as_expr = 1;
      break;
    }

  /* Expressions are the most general case.  */
  if (treat_as_expr)
    {
      agent_expr_up aexpr = gen_trace_for_var (scope, gdbarch,
					       sym, trace_string);

      /* It can happen that the symbol is recorded as a computed
	 location, but it's been optimized away and doesn't actually
	 have a location expression.  */
      if (!aexpr)
	{
	  gdb_printf ("%s has been optimized out of existence.\n",
		      sym->print_name ());
	  return;
	}

      finalize_tracepoint_aexpr (aexpr.get ());

      /* Take care of the registers.  */
      add_ax_registers (aexpr.get ());

      add_aexpr (std::move (aexpr));
    }
}

void
collection_list::add_wholly_collected (const char *print_name)
{
  m_wholly_collected.push_back (print_name);
}

/* Add all locals (or args) symbols to collection list.  */

void
collection_list::add_local_symbols (struct gdbarch *gdbarch, CORE_ADDR pc,
				    long frame_regno, long frame_offset, int type,
				    int trace_string)
{
  const struct block *block;
  int count = 0;

  auto do_collect_symbol = [&] (const char *print_name,
				struct symbol *sym)
    {
      collect_symbol (sym, gdbarch, frame_regno,
		      frame_offset, pc, trace_string);
      count++;
      add_wholly_collected (print_name);
    };

  if (type == 'L')
    {
      block = block_for_pc (pc);
      if (block == NULL)
	{
	  warning (_("Can't collect locals; "
		     "no symbol table info available.\n"));
	  return;
	}

      iterate_over_block_local_vars (block, do_collect_symbol);
      if (count == 0)
	warning (_("No locals found in scope."));
    }
  else
    {
      CORE_ADDR fn_pc = get_pc_function_start (pc);
      block = block_for_pc (fn_pc);
      if (block == NULL)
	{
	  warning (_("Can't collect args; no symbol table info available."));
	  return;
	}

      iterate_over_block_arg_vars (block, do_collect_symbol);
      if (count == 0)
	warning (_("No args found in scope."));
    }
}

void
collection_list::add_static_trace_data ()
{
  if (info_verbose)
    gdb_printf ("collect static trace data\n");
  m_strace_data = true;
}

collection_list::collection_list ()
  : m_strace_data (false)
{
  int max_remote_regno = 0;
  for (int i = 0; i < gdbarch_num_regs (current_inferior ()->arch ()); i++)
    {
      int remote_regno = (gdbarch_remote_register_number
			  (current_inferior ()->arch (), i));

      if (remote_regno >= 0 && remote_regno > max_remote_regno)
	max_remote_regno = remote_regno;
    }

  m_regs_mask.resize ((max_remote_regno / 8) + 1);

  m_memranges.reserve (128);
  m_aexprs.reserve (128);
}

/* Reduce a collection list to string form (for gdb protocol).  */

std::vector<std::string>
collection_list::stringify ()
{
  gdb::char_vector temp_buf (2048);

  int count;
  char *end;
  long i;
  std::vector<std::string> str_list;

  if (m_strace_data)
    {
      if (info_verbose)
	gdb_printf ("\nCollecting static trace data\n");
      end = temp_buf.data ();
      *end++ = 'L';
      str_list.emplace_back (temp_buf.data (), end - temp_buf.data ());
    }

  for (i = m_regs_mask.size () - 1; i > 0; i--)
    if (m_regs_mask[i] != 0)    /* Skip leading zeroes in regs_mask.  */
      break;
  if (m_regs_mask[i] != 0)	/* Prepare to send regs_mask to the stub.  */
    {
      if (info_verbose)
	gdb_printf ("\nCollecting registers (mask): 0x");

      /* One char for 'R', one for the null terminator and two per
	 mask byte.  */
      std::size_t new_size = (i + 1) * 2 + 2;
      if (new_size > temp_buf.size ())
	temp_buf.resize (new_size);

      end = temp_buf.data ();
      *end++ = 'R';
      for (; i >= 0; i--)
	{
	  QUIT;			/* Allow user to bail out with ^C.  */
	  if (info_verbose)
	    gdb_printf ("%02X", m_regs_mask[i]);

	  end = pack_hex_byte (end, m_regs_mask[i]);
	}
      *end = '\0';

      str_list.emplace_back (temp_buf.data ());
    }
  if (info_verbose)
    gdb_printf ("\n");
  if (!m_memranges.empty () && info_verbose)
    gdb_printf ("Collecting memranges: \n");
  for (i = 0, count = 0, end = temp_buf.data ();
       i < m_memranges.size (); i++)
    {
      QUIT;			/* Allow user to bail out with ^C.  */
      if (info_verbose)
	{
	  gdb_printf ("(%d, %s, %ld)\n", 
		      m_memranges[i].type,
		      paddress (current_inferior ()->arch (),
				m_memranges[i].start),
		      (long) (m_memranges[i].end
			      - m_memranges[i].start));
	}
      if (count + 27 > MAX_AGENT_EXPR_LEN)
	{
	  str_list.emplace_back (temp_buf.data (), count);
	  count = 0;
	  end = temp_buf.data ();
	}

      {
	bfd_signed_vma length
	  = m_memranges[i].end - m_memranges[i].start;

	/* The "%X" conversion specifier expects an unsigned argument,
	   so passing -1 (memrange_absolute) to it directly gives you
	   "FFFFFFFF" (or more, depending on sizeof (unsigned)).
	   Special-case it.  */
	if (m_memranges[i].type == memrange_absolute)
	  sprintf (end, "M-1,%s,%lX", phex_nz (m_memranges[i].start, 0),
		   (long) length);
	else
	  sprintf (end, "M%X,%s,%lX", m_memranges[i].type,
		   phex_nz (m_memranges[i].start, 0), (long) length);
      }

      count += strlen (end);
      end = temp_buf.data () + count;
    }

  for (i = 0; i < m_aexprs.size (); i++)
    {
      QUIT;			/* Allow user to bail out with ^C.  */
      if ((count + 10 + 2 * m_aexprs[i]->buf.size ()) > MAX_AGENT_EXPR_LEN)
	{
	  str_list.emplace_back (temp_buf.data (), count);
	  count = 0;
	  end = temp_buf.data ();
	}
      sprintf (end, "X%08X,", (int) m_aexprs[i]->buf.size ());
      end += 10;		/* 'X' + 8 hex digits + ',' */
      count += 10;

      end += 2 * bin2hex (m_aexprs[i]->buf.data (), end,
			  m_aexprs[i]->buf.size ());
      count += 2 * m_aexprs[i]->buf.size ();
    }

  if (count != 0)
    {
      str_list.emplace_back (temp_buf.data (), count);
      count = 0;
      end = temp_buf.data ();
    }

  return str_list;
}

/* Add the expression STR to M_COMPUTED.  */

void
collection_list::append_exp (std::string &&str)
{
  m_computed.push_back (std::move (str));
}

void
collection_list::finish ()
{
  memrange_sortmerge (m_memranges);
}

static void
encode_actions_1 (struct command_line *action,
		  struct bp_location *tloc,
		  int frame_reg,
		  LONGEST frame_offset,
		  struct collection_list *collect,
		  struct collection_list *stepping_list)
{
  const char *action_exp;
  int i;
  struct value *tempval;
  struct cmd_list_element *cmd;

  for (; action; action = action->next)
    {
      QUIT;			/* Allow user to bail out with ^C.  */
      action_exp = action->line;
      action_exp = skip_spaces (action_exp);

      cmd = lookup_cmd (&action_exp, cmdlist, "", NULL, -1, 1);
      if (cmd == 0)
	error (_("Bad action list item: %s"), action_exp);

      if (cmd_simple_func_eq (cmd, collect_pseudocommand))
	{
	  int trace_string = 0;

	  if (*action_exp == '/')
	    action_exp = decode_agent_options (action_exp, &trace_string);

	  do
	    {			/* Repeat over a comma-separated list.  */
	      QUIT;		/* Allow user to bail out with ^C.  */
	      action_exp = skip_spaces (action_exp);
	      gdbarch *arch = current_inferior ()->arch ();

	      if (0 == strncasecmp ("$reg", action_exp, 4))
		{
		  for (i = 0; i < gdbarch_num_regs (arch);
		       i++)
		    {
		      int remote_regno = (gdbarch_remote_register_number
					  (arch, i));

		      /* Ignore arch regnos without a corresponding
			 remote regno.  This can happen for regnos not
			 in the tdesc.  */
		      if (remote_regno >= 0)
			collect->add_remote_register (remote_regno);
		    }
		  action_exp = strchr (action_exp, ',');	/* more? */
		}
	      else if (0 == strncasecmp ("$arg", action_exp, 4))
		{
		  collect->add_local_symbols (arch,
					      tloc->address,
					      frame_reg,
					      frame_offset,
					      'A',
					      trace_string);
		  action_exp = strchr (action_exp, ',');	/* more? */
		}
	      else if (0 == strncasecmp ("$loc", action_exp, 4))
		{
		  collect->add_local_symbols (arch,
					      tloc->address,
					      frame_reg,
					      frame_offset,
					      'L',
					      trace_string);
		  action_exp = strchr (action_exp, ',');	/* more? */
		}
	      else if (0 == strncasecmp ("$_ret", action_exp, 5))
		{
		  agent_expr_up aexpr
		    = gen_trace_for_return_address (tloc->address,
						    arch, trace_string);

		  finalize_tracepoint_aexpr (aexpr.get ());

		  /* take care of the registers */
		  collect->add_ax_registers (aexpr.get ());

		  collect->add_aexpr (std::move (aexpr));
		  action_exp = strchr (action_exp, ',');	/* more? */
		}
	      else if (0 == strncasecmp ("$_sdata", action_exp, 7))
		{
		  collect->add_static_trace_data ();
		  action_exp = strchr (action_exp, ',');	/* more? */
		}
	      else
		{
		  unsigned long addr;

		  const char *exp_start = action_exp;
		  expression_up exp = parse_exp_1 (&action_exp, tloc->address,
						   block_for_pc (tloc->address),
						   PARSER_COMMA_TERMINATES);

		  switch (exp->first_opcode ())
		    {
		    case OP_REGISTER:
		      {
			expr::register_operation *regop
			  = (gdb::checked_static_cast<expr::register_operation *>
			     (exp->op.get ()));
			const char *name = regop->get_name ();

			i = user_reg_map_name_to_regnum (arch,
							 name, strlen (name));
			if (i == -1)
			  internal_error (_("Register $%s not available"),
					  name);
			if (info_verbose)
			  gdb_printf ("OP_REGISTER: ");
			collect->add_local_register (arch, i, tloc->address);
			break;
		      }

		    case UNOP_MEMVAL:
		      {
			/* Safe because we know it's a simple expression.  */
			tempval = exp->evaluate ();
			addr = tempval->address ();
			expr::unop_memval_operation *memop
			  = (gdb::checked_static_cast<expr::unop_memval_operation *>
			     (exp->op.get ()));
			struct type *type = memop->get_type ();
			/* Initialize the TYPE_LENGTH if it is a typedef.  */
			check_typedef (type);
			collect->add_memrange (arch,
					       memrange_absolute, addr,
					       type->length (),
					       tloc->address);
			collect->append_exp (std::string (exp_start,
							  action_exp));
		      }
		      break;

		    case OP_VAR_VALUE:
		      {
			expr::var_value_operation *vvo
			  = (gdb::checked_static_cast<expr::var_value_operation *>
			     (exp->op.get ()));
			struct symbol *sym = vvo->get_symbol ();
			const char *name = sym->natural_name ();

			collect->collect_symbol (sym,
						 arch,
						 frame_reg,
						 frame_offset,
						 tloc->address,
						 trace_string);
			collect->add_wholly_collected (name);
		      }
		      break;

		    default:	/* Full-fledged expression.  */
		      agent_expr_up aexpr = gen_trace_for_expr (tloc->address,
								exp.get (),
								trace_string);

		      finalize_tracepoint_aexpr (aexpr.get ());

		      /* Take care of the registers.  */
		      collect->add_ax_registers (aexpr.get ());

		      collect->add_aexpr (std::move (aexpr));
		      collect->append_exp (std::string (exp_start,
							action_exp));
		      break;
		    }		/* switch */
		}		/* do */
	    }
	  while (action_exp && *action_exp++ == ',');
	}			/* if */
      else if (cmd_simple_func_eq (cmd, teval_pseudocommand))
	{
	  do
	    {			/* Repeat over a comma-separated list.  */
	      QUIT;		/* Allow user to bail out with ^C.  */
	      action_exp = skip_spaces (action_exp);

		{
		  expression_up exp = parse_exp_1 (&action_exp, tloc->address,
						   block_for_pc (tloc->address),
						   PARSER_COMMA_TERMINATES);

		  agent_expr_up aexpr = gen_eval_for_expr (tloc->address,
							   exp.get ());

		  finalize_tracepoint_aexpr (aexpr.get ());

		  /* Even though we're not officially collecting, add
		     to the collect list anyway.  */
		  collect->add_aexpr (std::move (aexpr));
		}		/* do */
	    }
	  while (action_exp && *action_exp++ == ',');
	}			/* if */
      else if (cmd_simple_func_eq (cmd, while_stepping_pseudocommand))
	{
	  /* We check against nested while-stepping when setting
	     breakpoint action, so no way to run into nested
	     here.  */
	  gdb_assert (stepping_list);

	  encode_actions_1 (action->body_list_0.get (), tloc, frame_reg,
			    frame_offset, stepping_list, NULL);
	}
      else
	error (_("Invalid tracepoint command '%s'"), action->line);
    }				/* for */
}

/* Encode actions of tracepoint TLOC->owner and fill TRACEPOINT_LIST
   and STEPPING_LIST.  */

void
encode_actions (struct bp_location *tloc,
		struct collection_list *tracepoint_list,
		struct collection_list *stepping_list)
{
  int frame_reg;
  LONGEST frame_offset;

  gdbarch_virtual_frame_pointer (tloc->gdbarch,
				 tloc->address, &frame_reg, &frame_offset);

  tracepoint *t = gdb::checked_static_cast<tracepoint *> (tloc->owner);
  counted_command_line actions = all_tracepoint_actions (t);
  encode_actions_1 (actions.get (), tloc, frame_reg, frame_offset,
		    tracepoint_list, stepping_list);
  encode_actions_1 (breakpoint_commands (tloc->owner), tloc,
		    frame_reg, frame_offset, tracepoint_list, stepping_list);

  tracepoint_list->finish ();
  stepping_list->finish ();
}

/* Render all actions into gdb protocol.  */

void
encode_actions_rsp (struct bp_location *tloc,
		    std::vector<std::string> *tdp_actions,
		    std::vector<std::string> *stepping_actions)
{
  struct collection_list tracepoint_list, stepping_list;

  encode_actions (tloc, &tracepoint_list, &stepping_list);

  *tdp_actions = tracepoint_list.stringify ();
  *stepping_actions = stepping_list.stringify ();
}

void
collection_list::add_aexpr (agent_expr_up aexpr)
{
  m_aexprs.push_back (std::move (aexpr));
}

static void
process_tracepoint_on_disconnect (void)
{
  int has_pending_p = 0;

  /* Check whether we still have pending tracepoint.  If we have, warn the
     user that pending tracepoint will no longer work.  */
  for (breakpoint &b : all_tracepoints ())
    {
      if (!b.has_locations ())
	{
	  has_pending_p = 1;
	  break;
	}
      else
	{
	  for (bp_location &loc1 : b.locations ())
	    {
	      if (loc1.shlib_disabled)
		{
		  has_pending_p = 1;
		  break;
		}
	    }

	  if (has_pending_p)
	    break;
	}
    }

  if (has_pending_p)
    warning (_("Pending tracepoints will not be resolved while"
	       " GDB is disconnected\n"));
}

/* Reset local state of tracing.  */

void
trace_reset_local_state (void)
{
  set_traceframe_num (-1);
  set_tracepoint_num (-1);
  set_traceframe_context (NULL);
  clear_traceframe_info ();
}

void
start_tracing (const char *notes)
{
  int any_enabled = 0, num_to_download = 0;
  int ret;

  auto tracepoint_range = all_tracepoints ();

  /* No point in tracing without any tracepoints...  */
  if (tracepoint_range.begin () == tracepoint_range.end ())
    error (_("No tracepoints defined, not starting trace"));

  for (breakpoint &b : tracepoint_range)
    {
      if (b.enable_state == bp_enabled)
	any_enabled = 1;

      if ((b.type == bp_fast_tracepoint
	   ? may_insert_fast_tracepoints
	   : may_insert_tracepoints))
	++num_to_download;
      else
	warning (_("May not insert %stracepoints, skipping tracepoint %d"),
		 (b.type == bp_fast_tracepoint ? "fast " : ""), b.number);
    }

  if (!any_enabled)
    {
      if (target_supports_enable_disable_tracepoint ())
	warning (_("No tracepoints enabled"));
      else
	{
	  /* No point in tracing with only disabled tracepoints that
	     cannot be re-enabled.  */
	  error (_("No tracepoints enabled, not starting trace"));
	}
    }

  if (num_to_download <= 0)
    error (_("No tracepoints that may be downloaded, not starting trace"));

  target_trace_init ();

  for (breakpoint &b : tracepoint_range)
    {
      tracepoint &t = gdb::checked_static_cast<tracepoint &> (b);
      int bp_location_downloaded = 0;

      /* Clear `inserted' flag.  */
      for (bp_location &loc : b.locations ())
	loc.inserted = 0;

      if ((b.type == bp_fast_tracepoint
	   ? !may_insert_fast_tracepoints
	   : !may_insert_tracepoints))
	continue;

      t.number_on_target = 0;

      for (bp_location &loc : b.locations ())
	{
	  /* Since tracepoint locations are never duplicated, `inserted'
	     flag should be zero.  */
	  gdb_assert (!loc.inserted);

	  target_download_tracepoint (&loc);

	  loc.inserted = 1;
	  bp_location_downloaded = 1;
	}

      t.number_on_target = b.number;

      for (bp_location &loc : b.locations ())
	if (loc.probe.prob != NULL)
	  loc.probe.prob->set_semaphore (loc.probe.objfile, loc.gdbarch);

      if (bp_location_downloaded)
	notify_breakpoint_modified (&b);
    }

  /* Send down all the trace state variables too.  */
  for (const trace_state_variable &tsv : tvariables)
    target_download_trace_state_variable (tsv);
  
  /* Tell target to treat text-like sections as transparent.  */
  target_trace_set_readonly_regions ();
  /* Set some mode flags.  */
  target_set_disconnected_tracing (disconnected_tracing);
  target_set_circular_trace_buffer (circular_trace_buffer);
  target_set_trace_buffer_size (trace_buffer_size);

  if (!notes)
    notes = trace_notes.c_str ();

  ret = target_set_trace_notes (trace_user.c_str (), notes, NULL);

  if (!ret && (!trace_user.empty () || notes))
    warning (_("Target does not support trace user/notes, info ignored"));

  /* Now insert traps and begin collecting data.  */
  target_trace_start ();

  /* Reset our local state.  */
  trace_reset_local_state ();
  current_trace_status()->running = 1;
}

/* The tstart command requests the target to start a new trace run.
   The command passes any arguments it has to the target verbatim, as
   an optional "trace note".  This is useful as for instance a warning
   to other users if the trace runs disconnected, and you don't want
   anybody else messing with the target.  */

static void
tstart_command (const char *args, int from_tty)
{
  dont_repeat ();	/* Like "run", dangerous to repeat accidentally.  */

  if (current_trace_status ()->running)
    {
      if (from_tty
	  && !query (_("A trace is running already.  Start a new run? ")))
	error (_("New trace run not started."));
    }

  start_tracing (args);
}

/* The tstop command stops the tracing run.  The command passes any
   supplied arguments to the target verbatim as a "stop note"; if the
   target supports trace notes, then it will be reported back as part
   of the trace run's status.  */

static void
tstop_command (const char *args, int from_tty)
{
  if (!current_trace_status ()->running)
    error (_("Trace is not running."));

  stop_tracing (args);
}

void
stop_tracing (const char *note)
{
  int ret;

  target_trace_stop ();

  for (breakpoint &t : all_tracepoints ())
    {
      if ((t.type == bp_fast_tracepoint
	   ? !may_insert_fast_tracepoints
	   : !may_insert_tracepoints))
	continue;

      for (bp_location &loc : t.locations ())
	{
	  /* GDB can be totally absent in some disconnected trace scenarios,
	     but we don't really care if this semaphore goes out of sync.
	     That's why we are decrementing it here, but not taking care
	     in other places.  */
	  if (loc.probe.prob != NULL)
	    loc.probe.prob->clear_semaphore (loc.probe.objfile, loc.gdbarch);
	}
    }

  if (!note)
    note = trace_stop_notes.c_str ();

  ret = target_set_trace_notes (NULL, NULL, note);

  if (!ret && note)
    warning (_("Target does not support trace notes, note ignored"));

  /* Should change in response to reply?  */
  current_trace_status ()->running = 0;
}

/* tstatus command */
static void
tstatus_command (const char *args, int from_tty)
{
  struct trace_status *ts = current_trace_status ();
  int status;
  
  status = target_get_trace_status (ts);

  if (status == -1)
    {
      if (ts->filename != NULL)
	gdb_printf (_("Using a trace file.\n"));
      else
	{
	  gdb_printf (_("Trace can not be run on this target.\n"));
	  return;
	}
    }

  if (!ts->running_known)
    {
      gdb_printf (_("Run/stop status is unknown.\n"));
    }
  else if (ts->running)
    {
      gdb_printf (_("Trace is running on the target.\n"));
    }
  else
    {
      switch (ts->stop_reason)
	{
	case trace_never_run:
	  gdb_printf (_("No trace has been run on the target.\n"));
	  break;
	case trace_stop_command:
	  if (ts->stop_desc)
	    gdb_printf (_("Trace stopped by a tstop command (%s).\n"),
			ts->stop_desc);
	  else
	    gdb_printf (_("Trace stopped by a tstop command.\n"));
	  break;
	case trace_buffer_full:
	  gdb_printf (_("Trace stopped because the buffer was full.\n"));
	  break;
	case trace_disconnected:
	  gdb_printf (_("Trace stopped because of disconnection.\n"));
	  break;
	case tracepoint_passcount:
	  gdb_printf (_("Trace stopped by tracepoint %d.\n"),
		      ts->stopping_tracepoint);
	  break;
	case tracepoint_error:
	  if (ts->stopping_tracepoint)
	    gdb_printf (_("Trace stopped by an "
			  "error (%s, tracepoint %d).\n"),
			ts->stop_desc, ts->stopping_tracepoint);
	  else
	    gdb_printf (_("Trace stopped by an error (%s).\n"),
			ts->stop_desc);
	  break;
	case trace_stop_reason_unknown:
	  gdb_printf (_("Trace stopped for an unknown reason.\n"));
	  break;
	default:
	  gdb_printf (_("Trace stopped for some other reason (%d).\n"),
		      ts->stop_reason);
	  break;
	}
    }

  if (ts->traceframes_created >= 0
      && ts->traceframe_count != ts->traceframes_created)
    {
      gdb_printf (_("Buffer contains %d trace "
		    "frames (of %d created total).\n"),
		  ts->traceframe_count, ts->traceframes_created);
    }
  else if (ts->traceframe_count >= 0)
    {
      gdb_printf (_("Collected %d trace frames.\n"),
		  ts->traceframe_count);
    }

  if (ts->buffer_free >= 0)
    {
      if (ts->buffer_size >= 0)
	{
	  gdb_printf (_("Trace buffer has %d bytes of %d bytes free"),
		      ts->buffer_free, ts->buffer_size);
	  if (ts->buffer_size > 0)
	    gdb_printf (_(" (%d%% full)"),
			((int) ((((long long) (ts->buffer_size
					       - ts->buffer_free)) * 100)
				/ ts->buffer_size)));
	  gdb_printf (_(".\n"));
	}
      else
	gdb_printf (_("Trace buffer has %d bytes free.\n"),
		    ts->buffer_free);
    }

  if (ts->disconnected_tracing)
    gdb_printf (_("Trace will continue if GDB disconnects.\n"));
  else
    gdb_printf (_("Trace will stop if GDB disconnects.\n"));

  if (ts->circular_buffer)
    gdb_printf (_("Trace buffer is circular.\n"));

  if (ts->user_name && strlen (ts->user_name) > 0)
    gdb_printf (_("Trace user is %s.\n"), ts->user_name);

  if (ts->notes && strlen (ts->notes) > 0)
    gdb_printf (_("Trace notes: %s.\n"), ts->notes);

  /* Now report on what we're doing with tfind.  */
  if (traceframe_number >= 0)
    gdb_printf (_("Looking at trace frame %d, tracepoint %d.\n"),
		traceframe_number, tracepoint_number);
  else
    gdb_printf (_("Not looking at any trace frame.\n"));

  /* Report start/stop times if supplied.  */
  if (ts->start_time)
    {
      if (ts->stop_time)
	{
	  LONGEST run_time = ts->stop_time - ts->start_time;

	  /* Reporting a run time is more readable than two long numbers.  */
	  gdb_printf (_("Trace started at %ld.%06ld secs, stopped %ld.%06ld secs later.\n"),
		      (long int) (ts->start_time / 1000000),
		      (long int) (ts->start_time % 1000000),
		      (long int) (run_time / 1000000),
		      (long int) (run_time % 1000000));
	}
      else
	gdb_printf (_("Trace started at %ld.%06ld secs.\n"),
		    (long int) (ts->start_time / 1000000),
		    (long int) (ts->start_time % 1000000));
    }
  else if (ts->stop_time)
    gdb_printf (_("Trace stopped at %ld.%06ld secs.\n"),
		(long int) (ts->stop_time / 1000000),
		(long int) (ts->stop_time % 1000000));

  /* Now report any per-tracepoint status available.  */
  for (breakpoint &b : all_tracepoints ())
    {
      tracepoint *t = gdb::checked_static_cast<tracepoint *> (&b);
      target_get_tracepoint_status (t, nullptr);
    }
}

/* Report the trace status to uiout, in a way suitable for MI, and not
   suitable for CLI.  If ON_STOP is true, suppress a few fields that
   are not meaningful in the -trace-stop response.

   The implementation is essentially parallel to trace_status_command, but
   merging them will result in unreadable code.  */
void
trace_status_mi (int on_stop)
{
  struct ui_out *uiout = current_uiout;
  struct trace_status *ts = current_trace_status ();
  int status;

  status = target_get_trace_status (ts);

  if (status == -1 && ts->filename == NULL)
    {
      uiout->field_string ("supported", "0");
      return;
    }

  if (ts->filename != NULL)
    uiout->field_string ("supported", "file");
  else if (!on_stop)
    uiout->field_string ("supported", "1");

  if (ts->filename != NULL)
    uiout->field_string ("trace-file", ts->filename);

  gdb_assert (ts->running_known);

  if (ts->running)
    {
      uiout->field_string ("running", "1");

      /* Unlike CLI, do not show the state of 'disconnected-tracing' variable.
	 Given that the frontend gets the status either on -trace-stop, or from
	 -trace-status after re-connection, it does not seem like this
	 information is necessary for anything.  It is not necessary for either
	 figuring the vital state of the target nor for navigation of trace
	 frames.  If the frontend wants to show the current state is some
	 configure dialog, it can request the value when such dialog is
	 invoked by the user.  */
    }
  else
    {
      const char *stop_reason = NULL;
      int stopping_tracepoint = -1;

      if (!on_stop)
	uiout->field_string ("running", "0");

      if (ts->stop_reason != trace_stop_reason_unknown)
	{
	  switch (ts->stop_reason)
	    {
	    case trace_stop_command:
	      stop_reason = "request";
	      break;
	    case trace_buffer_full:
	      stop_reason = "overflow";
	      break;
	    case trace_disconnected:
	      stop_reason = "disconnection";
	      break;
	    case tracepoint_passcount:
	      stop_reason = "passcount";
	      stopping_tracepoint = ts->stopping_tracepoint;
	      break;
	    case tracepoint_error:
	      stop_reason = "error";
	      stopping_tracepoint = ts->stopping_tracepoint;
	      break;
	    }
	  
	  if (stop_reason)
	    {
	      uiout->field_string ("stop-reason", stop_reason);
	      if (stopping_tracepoint != -1)
		uiout->field_signed ("stopping-tracepoint",
				     stopping_tracepoint);
	      if (ts->stop_reason == tracepoint_error)
		uiout->field_string ("error-description",
				     ts->stop_desc);
	    }
	}
    }

  if (ts->traceframe_count != -1)
    uiout->field_signed ("frames", ts->traceframe_count);
  if (ts->traceframes_created != -1)
    uiout->field_signed ("frames-created", ts->traceframes_created);
  if (ts->buffer_size != -1)
    uiout->field_signed ("buffer-size", ts->buffer_size);
  if (ts->buffer_free != -1)
    uiout->field_signed ("buffer-free", ts->buffer_free);

  uiout->field_signed ("disconnected",  ts->disconnected_tracing);
  uiout->field_signed ("circular",  ts->circular_buffer);

  uiout->field_string ("user-name", ts->user_name);
  uiout->field_string ("notes", ts->notes);

  {
    char buf[100];

    xsnprintf (buf, sizeof buf, "%ld.%06ld",
	       (long int) (ts->start_time / 1000000),
	       (long int) (ts->start_time % 1000000));
    uiout->field_string ("start-time", buf);
    xsnprintf (buf, sizeof buf, "%ld.%06ld",
	       (long int) (ts->stop_time / 1000000),
	       (long int) (ts->stop_time % 1000000));
    uiout->field_string ("stop-time", buf);
  }
}

/* Check if a trace run is ongoing.  If so, and FROM_TTY, query the
   user if she really wants to detach.  */

void
query_if_trace_running (int from_tty)
{
  if (!from_tty)
    return;

  /* It can happen that the target that was tracing went away on its
     own, and we didn't notice.  Get a status update, and if the
     current target doesn't even do tracing, then assume it's not
     running anymore.  */
  if (target_get_trace_status (current_trace_status ()) < 0)
    current_trace_status ()->running = 0;

  /* If running interactively, give the user the option to cancel and
     then decide what to do differently with the run.  Scripts are
     just going to disconnect and let the target deal with it,
     according to how it's been instructed previously via
     disconnected-tracing.  */
  if (current_trace_status ()->running)
    {
      process_tracepoint_on_disconnect ();

      if (current_trace_status ()->disconnected_tracing)
	{
	  if (!query (_("Trace is running and will "
			"continue after detach; detach anyway? ")))
	    error (_("Not confirmed."));
	}
      else
	{
	  if (!query (_("Trace is running but will "
			"stop on detach; detach anyway? ")))
	    error (_("Not confirmed."));
	}
    }
}

/* This function handles the details of what to do about an ongoing
   tracing run if the user has asked to detach or otherwise disconnect
   from the target.  */

void
disconnect_tracing (void)
{
  /* Also we want to be out of tfind mode, otherwise things can get
     confusing upon reconnection.  Just use these calls instead of
     full tfind_1 behavior because we're in the middle of detaching,
     and there's no point to updating current stack frame etc.  */
  trace_reset_local_state ();
}

/* Worker function for the various flavors of the tfind command.  */
void
tfind_1 (enum trace_find_type type, int num,
	 CORE_ADDR addr1, CORE_ADDR addr2,
	 int from_tty)
{
  int target_frameno = -1, target_tracept = -1;
  struct frame_id old_frame_id = null_frame_id;
  struct tracepoint *tp;
  struct ui_out *uiout = current_uiout;

  /* Only try to get the current stack frame if we have a chance of
     succeeding.  In particular, if we're trying to get a first trace
     frame while all threads are running, it's not going to succeed,
     so leave it with a default value and let the frame comparison
     below (correctly) decide to print out the source location of the
     trace frame.  */
  if (!(type == tfind_number && num == -1)
      && (has_stack_frames () || traceframe_number >= 0))
    old_frame_id = get_frame_id (get_current_frame ());

  target_frameno = target_trace_find (type, num, addr1, addr2,
				      &target_tracept);
  
  if (type == tfind_number
      && num == -1
      && target_frameno == -1)
    {
      /* We told the target to get out of tfind mode, and it did.  */
    }
  else if (target_frameno == -1)
    {
      /* A request for a non-existent trace frame has failed.
	 Our response will be different, depending on FROM_TTY:

	 If FROM_TTY is true, meaning that this command was 
	 typed interactively by the user, then give an error
	 and DO NOT change the state of traceframe_number etc.

	 However if FROM_TTY is false, meaning that we're either
	 in a script, a loop, or a user-defined command, then 
	 DON'T give an error, but DO change the state of
	 traceframe_number etc. to invalid.

	 The rationale is that if you typed the command, you
	 might just have committed a typo or something, and you'd
	 like to NOT lose your current debugging state.  However
	 if you're in a user-defined command or especially in a
	 loop, then you need a way to detect that the command
	 failed WITHOUT aborting.  This allows you to write
	 scripts that search thru the trace buffer until the end,
	 and then continue on to do something else.  */
  
      if (from_tty)
	error (_("Target failed to find requested trace frame."));
      else
	{
	  if (info_verbose)
	    gdb_printf ("End of trace buffer.\n");
#if 0 /* dubious now?  */
	  /* The following will not recurse, since it's
	     special-cased.  */
	  tfind_command ("-1", from_tty);
#endif
	}
    }
  
  tp = get_tracepoint_by_number_on_target (target_tracept);

  reinit_frame_cache ();
  target_dcache_invalidate (current_program_space->aspace);

  set_tracepoint_num (tp ? tp->number : target_tracept);

  if (target_frameno != get_traceframe_number ())
    interps_notify_traceframe_changed (target_frameno, tracepoint_number);

  set_current_traceframe (target_frameno);

  if (target_frameno == -1)
    set_traceframe_context (NULL);
  else
    set_traceframe_context (get_current_frame ());

  if (traceframe_number >= 0)
    {
      /* Use different branches for MI and CLI to make CLI messages
	 i18n-eable.  */
      if (uiout->is_mi_like_p ())
	{
	  uiout->field_string ("found", "1");
	  uiout->field_signed ("tracepoint", tracepoint_number);
	  uiout->field_signed ("traceframe", traceframe_number);
	}
      else
	{
	  gdb_printf (_("Found trace frame %d, tracepoint %d\n"),
		      traceframe_number, tracepoint_number);
	}
    }
  else
    {
      if (uiout->is_mi_like_p ())
	uiout->field_string ("found", "0");
      else if (type == tfind_number && num == -1)
	gdb_printf (_("No longer looking at any trace frame\n"));
      else /* This case may never occur, check.  */
	gdb_printf (_("No trace frame found\n"));
    }

  /* If we're in nonstop mode and getting out of looking at trace
     frames, there won't be any current frame to go back to and
     display.  */
  if (from_tty
      && (has_stack_frames () || traceframe_number >= 0))
    {
      enum print_what print_what;

      /* NOTE: in imitation of the step command, try to determine
	 whether we have made a transition from one function to
	 another.  If so, we'll print the "stack frame" (ie. the new
	 function and it's arguments) -- otherwise we'll just show the
	 new source line.  */

      if (old_frame_id == get_frame_id (get_current_frame ()))
	print_what = SRC_LINE;
      else
	print_what = SRC_AND_LOC;

      print_stack_frame (get_selected_frame (NULL), 1, print_what, 1);
      do_displays ();
    }
}

/* Error on looking at traceframes while trace is running.  */

void
check_trace_running (struct trace_status *status)
{
  if (status->running && status->filename == NULL)
    error (_("May not look at trace frames while trace is running."));
}

/* trace_find_command takes a trace frame number n, 
   sends "QTFrame:<n>" to the target, 
   and accepts a reply that may contain several optional pieces
   of information: a frame number, a tracepoint number, and an
   indication of whether this is a trap frame or a stepping frame.

   The minimal response is just "OK" (which indicates that the 
   target does not give us a frame number or a tracepoint number).
   Instead of that, the target may send us a string containing
   any combination of:
   F<hexnum>    (gives the selected frame number)
   T<hexnum>    (gives the selected tracepoint number)
 */

/* tfind command */
static void
tfind_command_1 (const char *args, int from_tty)
{ /* This should only be called with a numeric argument.  */
  int frameno = -1;

  check_trace_running (current_trace_status ());
  
  if (args == 0 || *args == 0)
    { /* TFIND with no args means find NEXT trace frame.  */
      if (traceframe_number == -1)
	frameno = 0;	/* "next" is first one.  */
      else
	frameno = traceframe_number + 1;
    }
  else if (0 == strcmp (args, "-"))
    {
      if (traceframe_number == -1)
	error (_("not debugging trace buffer"));
      else if (from_tty && traceframe_number == 0)
	error (_("already at start of trace buffer"));
      
      frameno = traceframe_number - 1;
      }
  /* A hack to work around eval's need for fp to have been collected.  */
  else if (0 == strcmp (args, "-1"))
    frameno = -1;
  else
    frameno = parse_and_eval_long (args);

  if (frameno < -1)
    error (_("invalid input (%d is less than zero)"), frameno);

  tfind_1 (tfind_number, frameno, 0, 0, from_tty);
}

static void
tfind_command (const char *args, int from_tty)
{
  tfind_command_1 (args, from_tty);
}

/* tfind end */
static void
tfind_end_command (const char *args, int from_tty)
{
  tfind_command_1 ("-1", from_tty);
}

/* tfind start */
static void
tfind_start_command (const char *args, int from_tty)
{
  tfind_command_1 ("0", from_tty);
}

/* tfind pc command */
static void
tfind_pc_command (const char *args, int from_tty)
{
  CORE_ADDR pc;

  check_trace_running (current_trace_status ());

  if (args == 0 || *args == 0)
    pc = regcache_read_pc (get_thread_regcache (inferior_thread ()));
  else
    pc = parse_and_eval_address (args);

  tfind_1 (tfind_pc, 0, pc, 0, from_tty);
}

/* tfind tracepoint command */
static void
tfind_tracepoint_command (const char *args, int from_tty)
{
  int tdp;
  struct tracepoint *tp;

  check_trace_running (current_trace_status ());

  if (args == 0 || *args == 0)
    {
      if (tracepoint_number == -1)
	error (_("No current tracepoint -- please supply an argument."));
      else
	tdp = tracepoint_number;	/* Default is current TDP.  */
    }
  else
    tdp = parse_and_eval_long (args);

  /* If we have the tracepoint on hand, use the number that the
     target knows about (which may be different if we disconnected
     and reconnected).  */
  tp = get_tracepoint (tdp);
  if (tp)
    tdp = tp->number_on_target;

  tfind_1 (tfind_tp, tdp, 0, 0, from_tty);
}

/* TFIND LINE command:

   This command will take a sourceline for argument, just like BREAK
   or TRACE (ie. anything that "decode_line_1" can handle).

   With no argument, this command will find the next trace frame 
   corresponding to a source line OTHER THAN THE CURRENT ONE.  */

static void
tfind_line_command (const char *args, int from_tty)
{
  check_trace_running (current_trace_status ());

  symtab_and_line sal;
  if (args == 0 || *args == 0)
    {
      sal = find_pc_line (get_frame_pc (get_current_frame ()), 0);
    }
  else
    {
      std::vector<symtab_and_line> sals
	= decode_line_with_current_source (args, DECODE_LINE_FUNFIRSTLINE);
      sal = sals[0];
    }

  if (sal.symtab == 0)
    error (_("No line number information available."));

  CORE_ADDR start_pc, end_pc;
  if (sal.line > 0 && find_line_pc_range (sal, &start_pc, &end_pc))
    {
      if (start_pc == end_pc)
	{
	  gdb_printf ("Line %d of \"%s\"",
		      sal.line,
		      symtab_to_filename_for_display (sal.symtab));
	  gdb_stdout->wrap_here (2);
	  gdb_printf (" is at address ");
	  print_address (get_current_arch (), start_pc, gdb_stdout);
	  gdb_stdout->wrap_here (2);
	  gdb_printf (" but contains no code.\n");
	  sal = find_pc_line (start_pc, 0);
	  if (sal.line > 0
	      && find_line_pc_range (sal, &start_pc, &end_pc)
	      && start_pc != end_pc)
	    gdb_printf ("Attempting to find line %d instead.\n",
			sal.line);
	  else
	    error (_("Cannot find a good line."));
	}
      }
  else
    {
      /* Is there any case in which we get here, and have an address
	 which the user would want to see?  If we have debugging
	 symbols and no line numbers?  */
      error (_("Line number %d is out of range for \"%s\"."),
	     sal.line, symtab_to_filename_for_display (sal.symtab));
    }

  /* Find within range of stated line.  */
  if (args && *args)
    tfind_1 (tfind_range, 0, start_pc, end_pc - 1, from_tty);
  else
    tfind_1 (tfind_outside, 0, start_pc, end_pc - 1, from_tty);
}

/* tfind range command */
static void
tfind_range_command (const char *args, int from_tty)
{
  static CORE_ADDR start, stop;
  const char *tmp;

  check_trace_running (current_trace_status ());

  if (args == 0 || *args == 0)
    { /* XXX FIXME: what should default behavior be?  */
      gdb_printf ("Usage: tfind range STARTADDR, ENDADDR\n");
      return;
    }

  if (0 != (tmp = strchr (args, ',')))
    {
      std::string start_addr (args, tmp);
      ++tmp;
      tmp = skip_spaces (tmp);
      start = parse_and_eval_address (start_addr.c_str ());
      stop = parse_and_eval_address (tmp);
    }
  else
    {			/* No explicit end address?  */
      start = parse_and_eval_address (args);
      stop = start + 1;	/* ??? */
    }

  tfind_1 (tfind_range, 0, start, stop, from_tty);
}

/* tfind outside command */
static void
tfind_outside_command (const char *args, int from_tty)
{
  CORE_ADDR start, stop;
  const char *tmp;

  if (current_trace_status ()->running
      && current_trace_status ()->filename == NULL)
    error (_("May not look at trace frames while trace is running."));

  if (args == 0 || *args == 0)
    { /* XXX FIXME: what should default behavior be?  */
      gdb_printf ("Usage: tfind outside STARTADDR, ENDADDR\n");
      return;
    }

  if (0 != (tmp = strchr (args, ',')))
    {
      std::string start_addr (args, tmp);
      ++tmp;
      tmp = skip_spaces (tmp);
      start = parse_and_eval_address (start_addr.c_str ());
      stop = parse_and_eval_address (tmp);
    }
  else
    {			/* No explicit end address?  */
      start = parse_and_eval_address (args);
      stop = start + 1;	/* ??? */
    }

  tfind_1 (tfind_outside, 0, start, stop, from_tty);
}

/* info scope command: list the locals for a scope.  */
static void
info_scope_command (const char *args_in, int from_tty)
{
  struct bound_minimal_symbol msym;
  const struct block *block;
  const char *symname;
  const char *save_args = args_in;
  int j, count = 0;
  struct gdbarch *gdbarch;
  int regno;
  const char *args = args_in;

  if (args == 0 || *args == 0)
    error (_("requires an argument (function, "
	     "line or *addr) to define a scope"));

  location_spec_up locspec = string_to_location_spec (&args,
						      current_language);
  std::vector<symtab_and_line> sals
    = decode_line_1 (locspec.get (), DECODE_LINE_FUNFIRSTLINE,
		     NULL, NULL, 0);
  if (sals.empty ())
    {
      /* Presumably decode_line_1 has already warned.  */
      return;
    }

  /* Resolve line numbers to PC.  */
  resolve_sal_pc (&sals[0]);
  block = block_for_pc (sals[0].pc);

  while (block != 0)
    {
      QUIT;			/* Allow user to bail out with ^C.  */
      for (struct symbol *sym : block_iterator_range (block))
	{
	  QUIT;			/* Allow user to bail out with ^C.  */
	  if (count == 0)
	    gdb_printf ("Scope for %s:\n", save_args);
	  count++;

	  symname = sym->print_name ();
	  if (symname == NULL || *symname == '\0')
	    continue;		/* Probably botched, certainly useless.  */

	  gdbarch = sym->arch ();

	  gdb_printf ("Symbol %s is ", symname);

	  if (SYMBOL_COMPUTED_OPS (sym) != NULL)
	    SYMBOL_COMPUTED_OPS (sym)->describe_location (sym,
							  block->entry_pc (),
							  gdb_stdout);
	  else
	    {
	      switch (sym->aclass ())
		{
		default:
		case LOC_UNDEF:	/* Messed up symbol?  */
		  gdb_printf ("a bogus symbol, class %d.\n",
			      sym->aclass ());
		  count--;		/* Don't count this one.  */
		  continue;
		case LOC_CONST:
		  gdb_printf ("a constant with value %s (%s)",
			      plongest (sym->value_longest ()),
			      hex_string (sym->value_longest ()));
		  break;
		case LOC_CONST_BYTES:
		  gdb_printf ("constant bytes: ");
		  if (sym->type ())
		    for (j = 0; j < sym->type ()->length (); j++)
		      gdb_printf (" %02x", (unsigned) sym->value_bytes ()[j]);
		  break;
		case LOC_STATIC:
		  gdb_printf ("in static storage at address ");
		  gdb_printf ("%s", paddress (gdbarch, sym->value_address ()));
		  break;
		case LOC_REGISTER:
		  /* GDBARCH is the architecture associated with the objfile
		     the symbol is defined in; the target architecture may be
		     different, and may provide additional registers.  However,
		     we do not know the target architecture at this point.
		     We assume the objfile architecture will contain all the
		     standard registers that occur in debug info in that
		     objfile.  */
		  regno = SYMBOL_REGISTER_OPS (sym)->register_number (sym,
								      gdbarch);

		  if (sym->is_argument ())
		    gdb_printf ("an argument in register $%s",
				gdbarch_register_name (gdbarch, regno));
		  else
		    gdb_printf ("a local variable in register $%s",
				gdbarch_register_name (gdbarch, regno));
		  break;
		case LOC_ARG:
		  gdb_printf ("an argument at stack/frame offset %s",
			      plongest (sym->value_longest ()));
		  break;
		case LOC_LOCAL:
		  gdb_printf ("a local variable at frame offset %s",
			      plongest (sym->value_longest ()));
		  break;
		case LOC_REF_ARG:
		  gdb_printf ("a reference argument at offset %s",
			      plongest (sym->value_longest ()));
		  break;
		case LOC_REGPARM_ADDR:
		  /* Note comment at LOC_REGISTER.  */
		  regno = SYMBOL_REGISTER_OPS (sym)->register_number (sym,
								      gdbarch);
		  gdb_printf ("the address of an argument, in register $%s",
			      gdbarch_register_name (gdbarch, regno));
		  break;
		case LOC_TYPEDEF:
		  gdb_printf ("a typedef.\n");
		  continue;
		case LOC_LABEL:
		  gdb_printf ("a label at address ");
		  gdb_printf ("%s", paddress (gdbarch, sym->value_address ()));
		  break;
		case LOC_BLOCK:
		  gdb_printf ("a function at address ");
		  gdb_printf ("%s",
			      paddress (gdbarch,
					sym->value_block ()->entry_pc ()));
		  break;
		case LOC_UNRESOLVED:
		  msym = lookup_minimal_symbol (sym->linkage_name (),
						NULL, NULL);
		  if (msym.minsym == NULL)
		    gdb_printf ("Unresolved Static");
		  else
		    {
		      gdb_printf ("static storage at address ");
		      gdb_printf ("%s",
				  paddress (gdbarch, msym.value_address ()));
		    }
		  break;
		case LOC_OPTIMIZED_OUT:
		  gdb_printf ("optimized out.\n");
		  continue;
		case LOC_COMPUTED:
		  gdb_assert_not_reached ("LOC_COMPUTED variable missing a method");
		}
	    }
	  if (sym->type ())
	    {
	      struct type *t = check_typedef (sym->type ());

	      gdb_printf (", length %s.\n", pulongest (t->length ()));
	    }
	}
      if (block->function ())
	break;
      else
	block = block->superblock ();
    }
  if (count <= 0)
    gdb_printf ("Scope for %s contains no locals or arguments.\n",
		save_args);
}

/* Helper for trace_dump_command.  Dump the action list starting at
   ACTION.  STEPPING_ACTIONS is true if we're iterating over the
   actions of the body of a while-stepping action.  STEPPING_FRAME is
   set if the current traceframe was determined to be a while-stepping
   traceframe.  */

static void
trace_dump_actions (struct command_line *action,
		    int stepping_actions, int stepping_frame,
		    int from_tty)
{
  const char *action_exp, *next_comma;

  for (; action != NULL; action = action->next)
    {
      struct cmd_list_element *cmd;

      QUIT;			/* Allow user to bail out with ^C.  */
      action_exp = action->line;
      action_exp = skip_spaces (action_exp);

      /* The collection actions to be done while stepping are
	 bracketed by the commands "while-stepping" and "end".  */

      if (*action_exp == '#')	/* comment line */
	continue;

      cmd = lookup_cmd (&action_exp, cmdlist, "", NULL, -1, 1);
      if (cmd == 0)
	error (_("Bad action list item: %s"), action_exp);

      if (cmd_simple_func_eq (cmd, while_stepping_pseudocommand))
	{
	  gdb_assert (action->body_list_1 == nullptr);
	  trace_dump_actions (action->body_list_0.get (),
			      1, stepping_frame, from_tty);
	}
      else if (cmd_simple_func_eq (cmd, collect_pseudocommand))
	{
	  /* Display the collected data.
	     For the trap frame, display only what was collected at
	     the trap.  Likewise for stepping frames, display only
	     what was collected while stepping.  This means that the
	     two boolean variables, STEPPING_FRAME and
	     STEPPING_ACTIONS should be equal.  */
	  if (stepping_frame == stepping_actions)
	    {
	      int trace_string = 0;

	      if (*action_exp == '/')
		action_exp = decode_agent_options (action_exp, &trace_string);

	      do
		{		/* Repeat over a comma-separated list.  */
		  QUIT;		/* Allow user to bail out with ^C.  */
		  if (*action_exp == ',')
		    action_exp++;
		  action_exp = skip_spaces (action_exp);

		  next_comma = strchr (action_exp, ',');

		  if (0 == strncasecmp (action_exp, "$reg", 4))
		    registers_info (NULL, from_tty);
		  else if (0 == strncasecmp (action_exp, "$_ret", 5))
		    ;
		  else if (0 == strncasecmp (action_exp, "$loc", 4))
		    info_locals_command (NULL, from_tty);
		  else if (0 == strncasecmp (action_exp, "$arg", 4))
		    info_args_command (NULL, from_tty);
		  else
		    {		/* variable */
		      std::string contents;
		      const char *exp = action_exp;
		      if (next_comma != NULL)
			{
			  size_t len = next_comma - action_exp;
			  contents = std::string (action_exp, len);
			  exp = contents.c_str ();
			}

		      gdb_printf ("%s = ", exp);
		      output_command (exp, from_tty);
		      gdb_printf ("\n");
		    }
		  action_exp = next_comma;
		}
	      while (action_exp && *action_exp == ',');
	    }
	}
    }
}

/* Return bp_location of the tracepoint associated with the current
   traceframe.  Set *STEPPING_FRAME_P to 1 if the current traceframe
   is a stepping traceframe.  */

struct bp_location *
get_traceframe_location (int *stepping_frame_p)
{
  struct tracepoint *t;
  struct regcache *regcache;

  if (tracepoint_number == -1)
    error (_("No current trace frame."));

  t = get_tracepoint (tracepoint_number);

  if (t == NULL)
    error (_("No known tracepoint matches 'current' tracepoint #%d."),
	   tracepoint_number);

  /* The current frame is a trap frame if the frame PC is equal to the
     tracepoint PC.  If not, then the current frame was collected
     during single-stepping.  */
  regcache = get_thread_regcache (inferior_thread ());

  /* If the traceframe's address matches any of the tracepoint's
     locations, assume it is a direct hit rather than a while-stepping
     frame.  (FIXME this is not reliable, should record each frame's
     type.)  */
  for (bp_location &tloc : t->locations ())
    if (tloc.address == regcache_read_pc (regcache))
      {
	*stepping_frame_p = 0;
	return &tloc;
      }

  /* If this is a stepping frame, we don't know which location
     triggered.  The first is as good (or bad) a guess as any...  */
  *stepping_frame_p = 1;
  return &t->first_loc ();
}

/* Return the default collect actions of a tracepoint T.  */

static counted_command_line
all_tracepoint_actions (tracepoint *t)
{
  counted_command_line actions (nullptr, command_lines_deleter ());

  /* If there are default expressions to collect, make up a collect
     action and prepend to the action list to encode.  Note that since
     validation is per-tracepoint (local var "xyz" might be valid for
     one tracepoint and not another, etc), we make up the action on
     the fly, and don't cache it.  */
  if (!default_collect.empty ())
    {
      gdb::unique_xmalloc_ptr<char> default_collect_line
	= xstrprintf ("collect %s", default_collect.c_str ());

      validate_actionline (default_collect_line.get (), t);
      actions.reset (new struct command_line (simple_control,
					      default_collect_line.release ()),
		     command_lines_deleter ());
    }

  return actions;
}

/* The tdump command.  */

static void
tdump_command (const char *args, int from_tty)
{
  int stepping_frame = 0;
  struct bp_location *loc;

  /* This throws an error is not inspecting a trace frame.  */
  loc = get_traceframe_location (&stepping_frame);

  gdb_printf ("Data collected at tracepoint %d, trace frame %d:\n",
	      tracepoint_number, traceframe_number);

  /* This command only makes sense for the current frame, not the
     selected frame.  */
  scoped_restore_current_thread restore_thread;

  select_frame (get_current_frame ());

  tracepoint *t = gdb::checked_static_cast<tracepoint *> (loc->owner);
  counted_command_line actions = all_tracepoint_actions (t);

  trace_dump_actions (actions.get (), 0, stepping_frame, from_tty);
  trace_dump_actions (breakpoint_commands (loc->owner), 0, stepping_frame,
		      from_tty);
}

/* Encode a piece of a tracepoint's source-level definition in a form
   that is suitable for both protocol and saving in files.  */
/* This version does not do multiple encodes for long strings; it should
   return an offset to the next piece to encode.  FIXME  */

int
encode_source_string (int tpnum, ULONGEST addr,
		      const char *srctype, const char *src,
		      char *buf, int buf_size)
{
  if (80 + strlen (srctype) > buf_size)
    error (_("Buffer too small for source encoding"));
  sprintf (buf, "%x:%s:%s:%x:%x:",
	   tpnum, phex_nz (addr, sizeof (addr)),
	   srctype, 0, (int) strlen (src));
  if (strlen (buf) + strlen (src) * 2 >= buf_size)
    error (_("Source string too long for buffer"));
  bin2hex ((gdb_byte *) src, buf + strlen (buf), strlen (src));
  return -1;
}

/* Tell the target what to do with an ongoing tracing run if GDB
   disconnects for some reason.  */

static void
set_disconnected_tracing (const char *args, int from_tty,
			  struct cmd_list_element *c)
{
  target_set_disconnected_tracing (disconnected_tracing);
}

static void
set_circular_trace_buffer (const char *args, int from_tty,
			   struct cmd_list_element *c)
{
  target_set_circular_trace_buffer (circular_trace_buffer);
}

static void
set_trace_buffer_size (const char *args, int from_tty,
			   struct cmd_list_element *c)
{
  target_set_trace_buffer_size (trace_buffer_size);
}

static void
set_trace_user (const char *args, int from_tty,
		struct cmd_list_element *c)
{
  int ret;

  ret = target_set_trace_notes (trace_user.c_str (), NULL, NULL);

  if (!ret)
    warning (_("Target does not support trace notes, user ignored"));
}

static void
set_trace_notes (const char *args, int from_tty,
		 struct cmd_list_element *c)
{
  int ret;

  ret = target_set_trace_notes (NULL, trace_notes.c_str (), NULL);

  if (!ret)
    warning (_("Target does not support trace notes, note ignored"));
}

static void
set_trace_stop_notes (const char *args, int from_tty,
		      struct cmd_list_element *c)
{
  int ret;

  ret = target_set_trace_notes (NULL, NULL, trace_stop_notes.c_str ());

  if (!ret)
    warning (_("Target does not support trace notes, stop note ignored"));
}

int
get_traceframe_number (void)
{
  return traceframe_number;
}

int
get_tracepoint_number (void)
{
  return tracepoint_number;
}

/* Make the traceframe NUM be the current trace frame.  Does nothing
   if NUM is already current.  */

void
set_current_traceframe (int num)
{
  int newnum;

  if (traceframe_number == num)
    {
      /* Nothing to do.  */
      return;
    }

  newnum = target_trace_find (tfind_number, num, 0, 0, NULL);

  if (newnum != num)
    warning (_("could not change traceframe"));

  set_traceframe_num (newnum);

  /* Changing the traceframe changes our view of registers and of the
     frame chain.  */
  registers_changed ();

  clear_traceframe_info ();
}

scoped_restore_current_traceframe::scoped_restore_current_traceframe ()
: m_traceframe_number (traceframe_number)
{}

/* Given a number and address, return an uploaded tracepoint with that
   number, creating if necessary.  */

struct uploaded_tp *
get_uploaded_tp (int num, ULONGEST addr, struct uploaded_tp **utpp)
{
  struct uploaded_tp *utp;

  for (utp = *utpp; utp; utp = utp->next)
    if (utp->number == num && utp->addr == addr)
      return utp;

  utp = new uploaded_tp;
  utp->number = num;
  utp->addr = addr;
  utp->next = *utpp;
  *utpp = utp;

  return utp;
}

void
free_uploaded_tps (struct uploaded_tp **utpp)
{
  struct uploaded_tp *next_one;

  while (*utpp)
    {
      next_one = (*utpp)->next;
      delete *utpp;
      *utpp = next_one;
    }
}

/* Given a number and address, return an uploaded tracepoint with that
   number, creating if necessary.  */

struct uploaded_tsv *
get_uploaded_tsv (int num, struct uploaded_tsv **utsvp)
{
  struct uploaded_tsv *utsv;

  for (utsv = *utsvp; utsv; utsv = utsv->next)
    if (utsv->number == num)
      return utsv;

  utsv = XCNEW (struct uploaded_tsv);
  utsv->number = num;
  utsv->next = *utsvp;
  *utsvp = utsv;

  return utsv;
}

void
free_uploaded_tsvs (struct uploaded_tsv **utsvp)
{
  struct uploaded_tsv *next_one;

  while (*utsvp)
    {
      next_one = (*utsvp)->next;
      xfree (*utsvp);
      *utsvp = next_one;
    }
}

/* FIXME this function is heuristic and will miss the cases where the
   conditional is semantically identical but differs in whitespace,
   such as "x == 0" vs "x==0".  */

static int
cond_string_is_same (char *str1, char *str2)
{
  if (str1 == NULL || str2 == NULL)
    return (str1 == str2);

  return (strcmp (str1, str2) == 0);
}

/* Look for an existing tracepoint that seems similar enough to the
   uploaded one.  Enablement isn't compared, because the user can
   toggle that freely, and may have done so in anticipation of the
   next trace run.  Return the location of matched tracepoint.  */

static struct bp_location *
find_matching_tracepoint_location (struct uploaded_tp *utp)
{
  for (breakpoint &b : all_tracepoints ())
    {
      tracepoint &t = gdb::checked_static_cast<tracepoint &> (b);

      if (b.type == utp->type
	  && t.step_count == utp->step
	  && t.pass_count == utp->pass
	  && cond_string_is_same (t.cond_string.get (),
				  utp->cond_string.get ())
	  /* FIXME also test actions.  */
	  )
	{
	  /* Scan the locations for an address match.  */
	  for (bp_location &loc : b.locations ())
	    if (loc.address == utp->addr)
	      return &loc;
	}
    }
  return NULL;
}

/* Given a list of tracepoints uploaded from a target, attempt to
   match them up with existing tracepoints, and create new ones if not
   found.  */

void
merge_uploaded_tracepoints (struct uploaded_tp **uploaded_tps)
{
  struct uploaded_tp *utp;
  /* A set of tracepoints which are modified.  */
  std::vector<breakpoint *> modified_tp;

  /* Look for GDB tracepoints that match up with our uploaded versions.  */
  for (utp = *uploaded_tps; utp; utp = utp->next)
    {
      struct bp_location *loc;
      struct tracepoint *t;

      loc = find_matching_tracepoint_location (utp);
      if (loc)
	{
	  int found = 0;

	  /* Mark this location as already inserted.  */
	  loc->inserted = 1;
	  t = gdb::checked_static_cast<tracepoint *> (loc->owner);
	  gdb_printf (_("Assuming tracepoint %d is same "
			"as target's tracepoint %d at %s.\n"),
		      loc->owner->number, utp->number,
		      paddress (loc->gdbarch, utp->addr));

	  /* The tracepoint LOC->owner was modified (the location LOC
	     was marked as inserted in the target).  Save it in
	     MODIFIED_TP if not there yet.  The 'breakpoint-modified'
	     observers will be notified later once for each tracepoint
	     saved in MODIFIED_TP.  */
	  for (breakpoint *b : modified_tp)
	    if (b == loc->owner)
	      {
		found = 1;
		break;
	      }
	  if (!found)
	    modified_tp.push_back (loc->owner);
	}
      else
	{
	  t = create_tracepoint_from_upload (utp);
	  if (t)
	    gdb_printf (_("Created tracepoint %d for "
			  "target's tracepoint %d at %s.\n"),
			t->number, utp->number,
			paddress (get_current_arch (), utp->addr));
	  else
	    gdb_printf (_("Failed to create tracepoint for target's "
			  "tracepoint %d at %s, skipping it.\n"),
			utp->number,
			paddress (get_current_arch (), utp->addr));
	}
      /* Whether found or created, record the number used by the
	 target, to help with mapping target tracepoints back to their
	 counterparts here.  */
      if (t)
	t->number_on_target = utp->number;
    }

  /* Notify 'breakpoint-modified' observer that at least one of B's
     locations was changed.  */
  for (breakpoint *b : modified_tp)
    notify_breakpoint_modified (b);

  free_uploaded_tps (uploaded_tps);
}

/* Trace state variables don't have much to identify them beyond their
   name, so just use that to detect matches.  */

static struct trace_state_variable *
find_matching_tsv (struct uploaded_tsv *utsv)
{
  if (!utsv->name)
    return NULL;

  return find_trace_state_variable (utsv->name);
}

static struct trace_state_variable *
create_tsv_from_upload (struct uploaded_tsv *utsv)
{
  const char *namebase;
  std::string buf;
  int try_num = 0;
  struct trace_state_variable *tsv;

  if (utsv->name)
    {
      namebase = utsv->name;
      buf = namebase;
    }
  else
    {
      namebase = "__tsv";
      buf = string_printf ("%s_%d", namebase, try_num++);
    }

  /* Fish for a name that is not in use.  */
  /* (should check against all internal vars?)  */
  while (find_trace_state_variable (buf.c_str ()))
    buf = string_printf ("%s_%d", namebase, try_num++);

  /* We have an available name, create the variable.  */
  tsv = create_trace_state_variable (buf.c_str ());
  tsv->initial_value = utsv->initial_value;
  tsv->builtin = utsv->builtin;

  interps_notify_tsv_created (tsv);

  return tsv;
}

/* Given a list of uploaded trace state variables, try to match them
   up with existing variables, or create additional ones.  */

void
merge_uploaded_trace_state_variables (struct uploaded_tsv **uploaded_tsvs)
{
  struct uploaded_tsv *utsv;
  int highest;

  /* Most likely some numbers will have to be reassigned as part of
     the merge, so clear them all in anticipation.  */
  for (trace_state_variable &tsv : tvariables)
    tsv.number = 0;

  for (utsv = *uploaded_tsvs; utsv; utsv = utsv->next)
    {
      struct trace_state_variable *tsv = find_matching_tsv (utsv);
      if (tsv)
	{
	  if (info_verbose)
	    gdb_printf (_("Assuming trace state variable $%s "
			  "is same as target's variable %d.\n"),
			tsv->name.c_str (), utsv->number);
	}
      else
	{
	  tsv = create_tsv_from_upload (utsv);
	  if (info_verbose)
	    gdb_printf (_("Created trace state variable "
			  "$%s for target's variable %d.\n"),
			tsv->name.c_str (), utsv->number);
	}
      /* Give precedence to numberings that come from the target.  */
      if (tsv)
	tsv->number = utsv->number;
    }

  /* Renumber everything that didn't get a target-assigned number.  */
  highest = 0;
  for (const trace_state_variable &tsv : tvariables)
    highest = std::max (tsv.number, highest);

  ++highest;
  for (trace_state_variable &tsv : tvariables)
    if (tsv.number == 0)
      tsv.number = highest++;

  free_uploaded_tsvs (uploaded_tsvs);
}

/* Parse the part of trace status syntax that is shared between
   the remote protocol and the trace file reader.  */

void
parse_trace_status (const char *line, struct trace_status *ts)
{
  const char *p = line, *p1, *p2, *p3, *p_temp;
  int end;
  ULONGEST val;

  ts->running_known = 1;
  ts->running = (*p++ == '1');
  ts->stop_reason = trace_stop_reason_unknown;
  xfree (ts->stop_desc);
  ts->stop_desc = NULL;
  ts->traceframe_count = -1;
  ts->traceframes_created = -1;
  ts->buffer_free = -1;
  ts->buffer_size = -1;
  ts->disconnected_tracing = 0;
  ts->circular_buffer = 0;
  xfree (ts->user_name);
  ts->user_name = NULL;
  xfree (ts->notes);
  ts->notes = NULL;
  ts->start_time = ts->stop_time = 0;

  while (*p++)
    {
      p1 = strchr (p, ':');
      if (p1 == NULL)
	error (_("Malformed trace status, at %s\n\
Status line: '%s'\n"), p, line);
      p3 = strchr (p, ';');
      if (p3 == NULL)
	p3 = p + strlen (p);
      if (strncmp (p, stop_reason_names[trace_buffer_full], p1 - p) == 0)
	{
	  p = unpack_varlen_hex (++p1, &val);
	  ts->stop_reason = trace_buffer_full;
	}
      else if (strncmp (p, stop_reason_names[trace_never_run], p1 - p) == 0)
	{
	  p = unpack_varlen_hex (++p1, &val);
	  ts->stop_reason = trace_never_run;
	}
      else if (strncmp (p, stop_reason_names[tracepoint_passcount],
			p1 - p) == 0)
	{
	  p = unpack_varlen_hex (++p1, &val);
	  ts->stop_reason = tracepoint_passcount;
	  ts->stopping_tracepoint = val;
	}
      else if (strncmp (p, stop_reason_names[trace_stop_command], p1 - p) == 0)
	{
	  p2 = strchr (++p1, ':');
	  if (!p2 || p2 > p3)
	    {
	      /*older style*/
	      p2 = p1;
	    }
	  else if (p2 != p1)
	    {
	      ts->stop_desc = (char *) xmalloc (strlen (line));
	      end = hex2bin (p1, (gdb_byte *) ts->stop_desc, (p2 - p1) / 2);
	      ts->stop_desc[end] = '\0';
	    }
	  else
	    ts->stop_desc = xstrdup ("");

	  p = unpack_varlen_hex (++p2, &val);
	  ts->stop_reason = trace_stop_command;
	}
      else if (strncmp (p, stop_reason_names[trace_disconnected], p1 - p) == 0)
	{
	  p = unpack_varlen_hex (++p1, &val);
	  ts->stop_reason = trace_disconnected;
	}
      else if (strncmp (p, stop_reason_names[tracepoint_error], p1 - p) == 0)
	{
	  p2 = strchr (++p1, ':');
	  if (p2 != p1)
	    {
	      ts->stop_desc = (char *) xmalloc ((p2 - p1) / 2 + 1);
	      end = hex2bin (p1, (gdb_byte *) ts->stop_desc, (p2 - p1) / 2);
	      ts->stop_desc[end] = '\0';
	    }
	  else
	    ts->stop_desc = xstrdup ("");

	  p = unpack_varlen_hex (++p2, &val);
	  ts->stopping_tracepoint = val;
	  ts->stop_reason = tracepoint_error;
	}
      else if (strncmp (p, "tframes", p1 - p) == 0)
	{
	  p = unpack_varlen_hex (++p1, &val);
	  ts->traceframe_count = val;
	}
      else if (strncmp (p, "tcreated", p1 - p) == 0)
	{
	  p = unpack_varlen_hex (++p1, &val);
	  ts->traceframes_created = val;
	}
      else if (strncmp (p, "tfree", p1 - p) == 0)
	{
	  p = unpack_varlen_hex (++p1, &val);
	  ts->buffer_free = val;
	}
      else if (strncmp (p, "tsize", p1 - p) == 0)
	{
	  p = unpack_varlen_hex (++p1, &val);
	  ts->buffer_size = val;
	}
      else if (strncmp (p, "disconn", p1 - p) == 0)
	{
	  p = unpack_varlen_hex (++p1, &val);
	  ts->disconnected_tracing = val;
	}
      else if (strncmp (p, "circular", p1 - p) == 0)
	{
	  p = unpack_varlen_hex (++p1, &val);
	  ts->circular_buffer = val;
	}
      else if (strncmp (p, "starttime", p1 - p) == 0)
	{
	  p = unpack_varlen_hex (++p1, &val);
	  ts->start_time = val;
	}
      else if (strncmp (p, "stoptime", p1 - p) == 0)
	{
	  p = unpack_varlen_hex (++p1, &val);
	  ts->stop_time = val;
	}
      else if (strncmp (p, "username", p1 - p) == 0)
	{
	  ++p1;
	  ts->user_name = (char *) xmalloc (strlen (p) / 2);
	  end = hex2bin (p1, (gdb_byte *) ts->user_name, (p3 - p1)  / 2);
	  ts->user_name[end] = '\0';
	  p = p3;
	}
      else if (strncmp (p, "notes", p1 - p) == 0)
	{
	  ++p1;
	  ts->notes = (char *) xmalloc (strlen (p) / 2);
	  end = hex2bin (p1, (gdb_byte *) ts->notes, (p3 - p1) / 2);
	  ts->notes[end] = '\0';
	  p = p3;
	}
      else
	{
	  /* Silently skip unknown optional info.  */
	  p_temp = strchr (p1 + 1, ';');
	  if (p_temp)
	    p = p_temp;
	  else
	    /* Must be at the end.  */
	    break;
	}
    }
}

void
parse_tracepoint_status (const char *p, tracepoint *tp,
			 struct uploaded_tp *utp)
{
  ULONGEST uval;

  p = unpack_varlen_hex (p, &uval);
  if (tp)
    tp->hit_count += uval;
  else
    utp->hit_count += uval;
  p = unpack_varlen_hex (p + 1, &uval);
  if (tp)
    tp->traceframe_usage += uval;
  else
    utp->traceframe_usage += uval;
  /* Ignore any extra, allowing for future extensions.  */
}

/* Given a line of text defining a part of a tracepoint, parse it into
   an "uploaded tracepoint".  */

void
parse_tracepoint_definition (const char *line, struct uploaded_tp **utpp)
{
  const char *p;
  char piece;
  ULONGEST num, addr, step, pass, orig_size, xlen, start;
  int enabled, end;
  enum bptype type;
  const char *srctype;
  char *buf;
  struct uploaded_tp *utp = NULL;

  p = line;
  /* Both tracepoint and action definitions start with the same number
     and address sequence.  */
  piece = *p++;
  p = unpack_varlen_hex (p, &num);
  p++;  /* skip a colon */
  p = unpack_varlen_hex (p, &addr);
  p++;  /* skip a colon */
  if (piece == 'T')
    {
      gdb::unique_xmalloc_ptr<char[]> cond;

      enabled = (*p++ == 'E');
      p++;  /* skip a colon */
      p = unpack_varlen_hex (p, &step);
      p++;  /* skip a colon */
      p = unpack_varlen_hex (p, &pass);
      type = bp_tracepoint;
      /* Thumb through optional fields.  */
      while (*p == ':')
	{
	  p++;  /* skip a colon */
	  if (*p == 'F')
	    {
	      type = bp_fast_tracepoint;
	      p++;
	      p = unpack_varlen_hex (p, &orig_size);
	    }
	  else if (*p == 'S')
	    {
	      type = bp_static_tracepoint;
	      p++;
	    }
	  else if (*p == 'X')
	    {
	      p++;
	      p = unpack_varlen_hex (p, &xlen);
	      p++;  /* skip a comma */
	      cond.reset ((char *) xmalloc (2 * xlen + 1));
	      strncpy (&cond[0], p, 2 * xlen);
	      cond[2 * xlen] = '\0';
	      p += 2 * xlen;
	    }
	  else
	    warning (_("Unrecognized char '%c' in tracepoint "
		       "definition, skipping rest"), *p);
	}
      utp = get_uploaded_tp (num, addr, utpp);
      utp->type = type;
      utp->enabled = enabled;
      utp->step = step;
      utp->pass = pass;
      utp->cond = std::move (cond);
    }
  else if (piece == 'A')
    {
      utp = get_uploaded_tp (num, addr, utpp);
      utp->actions.emplace_back (xstrdup (p));
    }
  else if (piece == 'S')
    {
      utp = get_uploaded_tp (num, addr, utpp);
      utp->step_actions.emplace_back (xstrdup (p));
    }
  else if (piece == 'Z')
    {
      /* Parse a chunk of source form definition.  */
      utp = get_uploaded_tp (num, addr, utpp);
      srctype = p;
      p = strchr (p, ':');
      p++;  /* skip a colon */
      p = unpack_varlen_hex (p, &start);
      p++;  /* skip a colon */
      p = unpack_varlen_hex (p, &xlen);
      p++;  /* skip a colon */

      buf = (char *) alloca (strlen (line));

      end = hex2bin (p, (gdb_byte *) buf, strlen (p) / 2);
      buf[end] = '\0';

      if (startswith (srctype, "at:"))
	utp->at_string.reset (xstrdup (buf));
      else if (startswith (srctype, "cond:"))
	utp->cond_string.reset (xstrdup (buf));
      else if (startswith (srctype, "cmd:"))
	utp->cmd_strings.emplace_back (xstrdup (buf));
    }
  else if (piece == 'V')
    {
      utp = get_uploaded_tp (num, addr, utpp);

      parse_tracepoint_status (p, NULL, utp);
    }
  else
    {
      /* Don't error out, the target might be sending us optional
	 info that we don't care about.  */
      warning (_("Unrecognized tracepoint piece '%c', ignoring"), piece);
    }
}

/* Convert a textual description of a trace state variable into an
   uploaded object.  */

void
parse_tsv_definition (const char *line, struct uploaded_tsv **utsvp)
{
  const char *p;
  char *buf;
  ULONGEST num, initval, builtin;
  int end;
  struct uploaded_tsv *utsv = NULL;

  buf = (char *) alloca (strlen (line));

  p = line;
  p = unpack_varlen_hex (p, &num);
  p++; /* skip a colon */
  p = unpack_varlen_hex (p, &initval);
  p++; /* skip a colon */
  p = unpack_varlen_hex (p, &builtin);
  p++; /* skip a colon */
  end = hex2bin (p, (gdb_byte *) buf, strlen (p) / 2);
  buf[end] = '\0';

  utsv = get_uploaded_tsv (num, utsvp);
  utsv->initial_value = initval;
  utsv->builtin = builtin;
  utsv->name = xstrdup (buf);
}

/* Given a line of text defining a static tracepoint marker, parse it
   into a "static tracepoint marker" object.  Throws an error is
   parsing fails.  If PP is non-null, it points to one past the end of
   the parsed marker definition.  */

void
parse_static_tracepoint_marker_definition (const char *line, const char **pp,
					   static_tracepoint_marker *marker)
{
  const char *p, *endp;
  ULONGEST addr;

  p = line;
  p = unpack_varlen_hex (p, &addr);
  p++;  /* skip a colon */

  marker->gdbarch = current_inferior ()->arch ();
  marker->address = (CORE_ADDR) addr;

  endp = strchr (p, ':');
  if (endp == NULL)
    error (_("bad marker definition: %s"), line);

  marker->str_id = hex2str (p, (endp - p) / 2);

  p = endp;
  p++; /* skip a colon */

  /* This definition may be followed by another one, separated by a comma.  */
  int hex_len;
  endp = strchr (p, ',');
  if (endp != nullptr)
    hex_len = endp - p;
  else
    hex_len = strlen (p);

  marker->extra = hex2str (p, hex_len / 2);

  if (pp != nullptr)
    *pp = p + hex_len;
}

/* Print MARKER to gdb_stdout.  */

static void
print_one_static_tracepoint_marker (int count,
				    const static_tracepoint_marker &marker)
{
  struct symbol *sym;

  struct ui_out *uiout = current_uiout;

  symtab_and_line sal;
  sal.pc = marker.address;

  std::vector<breakpoint *> tracepoints
    = static_tracepoints_here (marker.address);

  ui_out_emit_tuple tuple_emitter (uiout, "marker");

  /* A counter field to help readability.  This is not a stable
     identifier!  */
  uiout->field_signed ("count", count);

  uiout->field_string ("marker-id", marker.str_id);

  uiout->field_fmt ("enabled", "%c",
		    !tracepoints.empty () ? 'y' : 'n');
  uiout->spaces (2);

  int wrap_indent = 35;
  if (gdbarch_addr_bit (marker.gdbarch) <= 32)
    wrap_indent += 11;
  else
    wrap_indent += 19;

  const char *extra_field_indent = "         ";

  uiout->field_core_addr ("addr", marker.gdbarch, marker.address);

  sal = find_pc_line (marker.address, 0);
  sym = find_pc_sect_function (marker.address, NULL);
  if (sym)
    {
      uiout->text ("in ");
      uiout->field_string ("func", sym->print_name (),
			   function_name_style.style ());
      uiout->wrap_hint (wrap_indent);
      uiout->text (" at ");
    }
  else
    uiout->field_skip ("func");

  if (sal.symtab != NULL)
    {
      uiout->field_string ("file",
			   symtab_to_filename_for_display (sal.symtab),
			   file_name_style.style ());
      uiout->text (":");

      if (uiout->is_mi_like_p ())
	{
	  const char *fullname = symtab_to_fullname (sal.symtab);

	  uiout->field_string ("fullname", fullname);
	}
      else
	uiout->field_skip ("fullname");

      uiout->field_signed ("line", sal.line);
    }
  else
    {
      uiout->field_skip ("fullname");
      uiout->field_skip ("line");
    }

  uiout->text ("\n");
  uiout->text (extra_field_indent);
  uiout->text (_("Data: \""));
  uiout->field_string ("extra-data", marker.extra);
  uiout->text ("\"\n");

  if (!tracepoints.empty ())
    {
      int ix;

      {
	ui_out_emit_tuple inner_tuple_emitter (uiout, "tracepoints-at");

	uiout->text (extra_field_indent);
	uiout->text (_("Probed by static tracepoints: "));
	for (ix = 0; ix < tracepoints.size (); ix++)
	  {
	    if (ix > 0)
	      uiout->text (", ");
	    uiout->text ("#");
	    uiout->field_signed ("tracepoint-id", tracepoints[ix]->number);
	  }
      }

      if (uiout->is_mi_like_p ())
	uiout->field_signed ("number-of-tracepoints", tracepoints.size ());
      else
	uiout->text ("\n");
    }
}

static void
info_static_tracepoint_markers_command (const char *arg, int from_tty)
{
  struct ui_out *uiout = current_uiout;
  std::vector<static_tracepoint_marker> markers
    = target_static_tracepoint_markers_by_strid (NULL);

  /* We don't have to check target_can_use_agent and agent's capability on
     static tracepoint here, in order to be compatible with older GDBserver.
     We don't check USE_AGENT is true or not, because static tracepoints
     don't work without in-process agent, so we don't bother users to type
     `set agent on' when to use static tracepoint.  */

  ui_out_emit_table table_emitter (uiout, 5, -1,
				   "StaticTracepointMarkersTable");

  uiout->table_header (7, ui_left, "counter", "Cnt");

  uiout->table_header (40, ui_left, "marker-id", "ID");

  uiout->table_header (3, ui_left, "enabled", "Enb");
  if (gdbarch_addr_bit (current_inferior ()->arch ()) <= 32)
    uiout->table_header (10, ui_left, "addr", "Address");
  else
    uiout->table_header (18, ui_left, "addr", "Address");
  uiout->table_header (40, ui_noalign, "what", "What");

  uiout->table_body ();

  for (int i = 0; i < markers.size (); i++)
    print_one_static_tracepoint_marker (i + 1, markers[i]);
}

/* The $_sdata convenience variable is a bit special.  We don't know
   for sure type of the value until we actually have a chance to fetch
   the data --- the size of the object depends on what has been
   collected.  We solve this by making $_sdata be an internalvar that
   creates a new value on access.  */

/* Return a new value with the correct type for the sdata object of
   the current trace frame.  Return a void value if there's no object
   available.  */

static struct value *
sdata_make_value (struct gdbarch *gdbarch, struct internalvar *var,
		  void *ignore)
{
  /* We need to read the whole object before we know its size.  */
  std::optional<gdb::byte_vector> buf
    = target_read_alloc (current_inferior ()->top_target (),
			 TARGET_OBJECT_STATIC_TRACE_DATA,
			 NULL);
  if (buf)
    {
      struct value *v;
      struct type *type;

      type = init_vector_type (builtin_type (gdbarch)->builtin_true_char,
			       buf->size ());
      v = value::allocate (type);
      memcpy (v->contents_raw ().data (), buf->data (), buf->size ());
      return v;
    }
  else
    return value::allocate (builtin_type (gdbarch)->builtin_void);
}

#if !defined(HAVE_LIBEXPAT)

struct std::unique_ptr<traceframe_info>
parse_traceframe_info (const char *tframe_info)
{
  static int have_warned;

  if (!have_warned)
    {
      have_warned = 1;
      warning (_("Can not parse XML trace frame info; XML support "
		 "was disabled at compile time"));
    }

  return NULL;
}

#else /* HAVE_LIBEXPAT */

#include "xml-support.h"

/* Handle the start of a <memory> element.  */

static void
traceframe_info_start_memory (struct gdb_xml_parser *parser,
			      const struct gdb_xml_element *element,
			      void *user_data,
			      std::vector<gdb_xml_value> &attributes)
{
  struct traceframe_info *info = (struct traceframe_info *) user_data;
  ULONGEST *start_p, *length_p;

  start_p
    = (ULONGEST *) xml_find_attribute (attributes, "start")->value.get ();
  length_p
    = (ULONGEST *) xml_find_attribute (attributes, "length")->value.get ();

  info->memory.emplace_back (*start_p, *length_p);
}

/* Handle the start of a <tvar> element.  */

static void
traceframe_info_start_tvar (struct gdb_xml_parser *parser,
			     const struct gdb_xml_element *element,
			     void *user_data,
			     std::vector<gdb_xml_value> &attributes)
{
  struct traceframe_info *info = (struct traceframe_info *) user_data;
  const char *id_attrib
    = (const char *) xml_find_attribute (attributes, "id")->value.get ();
  int id = gdb_xml_parse_ulongest (parser, id_attrib);

  info->tvars.push_back (id);
}

/* The allowed elements and attributes for an XML memory map.  */

static const struct gdb_xml_attribute memory_attributes[] = {
  { "start", GDB_XML_AF_NONE, gdb_xml_parse_attr_ulongest, NULL },
  { "length", GDB_XML_AF_NONE, gdb_xml_parse_attr_ulongest, NULL },
  { NULL, GDB_XML_AF_NONE, NULL, NULL }
};

static const struct gdb_xml_attribute tvar_attributes[] = {
  { "id", GDB_XML_AF_NONE, NULL, NULL },
  { NULL, GDB_XML_AF_NONE, NULL, NULL }
};

static const struct gdb_xml_element traceframe_info_children[] = {
  { "memory", memory_attributes, NULL,
    GDB_XML_EF_REPEATABLE | GDB_XML_EF_OPTIONAL,
    traceframe_info_start_memory, NULL },
  { "tvar", tvar_attributes, NULL,
    GDB_XML_EF_REPEATABLE | GDB_XML_EF_OPTIONAL,
    traceframe_info_start_tvar, NULL },
  { NULL, NULL, NULL, GDB_XML_EF_NONE, NULL, NULL }
};

static const struct gdb_xml_element traceframe_info_elements[] = {
  { "traceframe-info", NULL, traceframe_info_children, GDB_XML_EF_NONE,
    NULL, NULL },
  { NULL, NULL, NULL, GDB_XML_EF_NONE, NULL, NULL }
};

/* Parse a traceframe-info XML document.  */

traceframe_info_up
parse_traceframe_info (const char *tframe_info)
{
  traceframe_info_up result (new traceframe_info);

  if (gdb_xml_parse_quick (_("trace frame info"),
			   "traceframe-info.dtd", traceframe_info_elements,
			   tframe_info, result.get ()) == 0)
    return result;

  return NULL;
}

#endif /* HAVE_LIBEXPAT */

/* Returns the traceframe_info object for the current traceframe.
   This is where we avoid re-fetching the object from the target if we
   already have it cached.  */

struct traceframe_info *
get_traceframe_info (void)
{
  if (current_traceframe_info == NULL)
    current_traceframe_info = target_traceframe_info ();

  return current_traceframe_info.get ();
}

/* If the target supports the query, return in RESULT the set of
   collected memory in the current traceframe, found within the LEN
   bytes range starting at MEMADDR.  Returns true if the target
   supports the query, otherwise returns false, and RESULT is left
   undefined.  */

int
traceframe_available_memory (std::vector<mem_range> *result,
			     CORE_ADDR memaddr, ULONGEST len)
{
  struct traceframe_info *info = get_traceframe_info ();

  if (info != NULL)
    {
      result->clear ();

      for (mem_range &r : info->memory)
	if (mem_ranges_overlap (r.start, r.length, memaddr, len))
	  {
	    ULONGEST lo1, hi1, lo2, hi2;

	    lo1 = memaddr;
	    hi1 = memaddr + len;

	    lo2 = r.start;
	    hi2 = r.start + r.length;

	    CORE_ADDR start = std::max (lo1, lo2);
	    int length = std::min (hi1, hi2) - start;

	    result->emplace_back (start, length);
	  }

      normalize_mem_ranges (result);
      return 1;
    }

  return 0;
}

/* Implementation of `sdata' variable.  */

static const struct internalvar_funcs sdata_funcs =
{
  sdata_make_value,
  NULL
};

/* See tracepoint.h.  */
cmd_list_element *while_stepping_cmd_element = nullptr;

/* module initialization */
void _initialize_tracepoint ();
void
_initialize_tracepoint ()
{
  struct cmd_list_element *c;

  /* Explicitly create without lookup, since that tries to create a
     value with a void typed value, and when we get here, gdbarch
     isn't initialized yet.  At this point, we're quite sure there
     isn't another convenience variable of the same name.  */
  create_internalvar_type_lazy ("_sdata", &sdata_funcs, NULL);

  traceframe_number = -1;
  tracepoint_number = -1;

  add_info ("scope", info_scope_command,
	    _("List the variables local to a scope."));

  add_cmd ("tracepoints", class_trace,
	   _("Tracing of program execution without stopping the program."),
	   &cmdlist);

  add_com ("tdump", class_trace, tdump_command,
	   _("Print everything collected at the current tracepoint."));

  c = add_com ("tvariable", class_trace, trace_variable_command,_("\
Define a trace state variable.\n\
Argument is a $-prefixed name, optionally followed\n\
by '=' and an expression that sets the initial value\n\
at the start of tracing."));
  set_cmd_completer (c, expression_completer);

  add_cmd ("tvariable", class_trace, delete_trace_variable_command, _("\
Delete one or more trace state variables.\n\
Arguments are the names of the variables to delete.\n\
If no arguments are supplied, delete all variables."), &deletelist);
  /* FIXME add a trace variable completer.  */

  add_info ("tvariables", info_tvariables_command, _("\
Status of trace state variables and their values."));

  add_info ("static-tracepoint-markers",
	    info_static_tracepoint_markers_command, _("\
List target static tracepoints markers."));

  add_prefix_cmd ("tfind", class_trace, tfind_command, _("\
Select a trace frame.\n\
No argument means forward by one frame; '-' means backward by one frame."),
		  &tfindlist, 1, &cmdlist);

  add_cmd ("outside", class_trace, tfind_outside_command, _("\
Select a trace frame whose PC is outside the given range (exclusive).\n\
Usage: tfind outside ADDR1, ADDR2"),
	   &tfindlist);

  add_cmd ("range", class_trace, tfind_range_command, _("\
Select a trace frame whose PC is in the given range (inclusive).\n\
Usage: tfind range ADDR1, ADDR2"),
	   &tfindlist);

  add_cmd ("line", class_trace, tfind_line_command, _("\
Select a trace frame by source line.\n\
Argument can be a line number (with optional source file),\n\
a function name, or '*' followed by an address.\n\
Default argument is 'the next source line that was traced'."),
	   &tfindlist);

  add_cmd ("tracepoint", class_trace, tfind_tracepoint_command, _("\
Select a trace frame by tracepoint number.\n\
Default is the tracepoint for the current trace frame."),
	   &tfindlist);

  add_cmd ("pc", class_trace, tfind_pc_command, _("\
Select a trace frame by PC.\n\
Default is the current PC, or the PC of the current trace frame."),
	   &tfindlist);

  cmd_list_element *tfind_end_cmd
    = add_cmd ("end", class_trace, tfind_end_command, _("\
De-select any trace frame and resume 'live' debugging."), &tfindlist);

  add_alias_cmd ("none", tfind_end_cmd, class_trace, 0, &tfindlist);

  add_cmd ("start", class_trace, tfind_start_command,
	   _("Select the first trace frame in the trace buffer."),
	   &tfindlist);

  add_com ("tstatus", class_trace, tstatus_command,
	   _("Display the status of the current trace data collection."));

  add_com ("tstop", class_trace, tstop_command, _("\
Stop trace data collection.\n\
Usage: tstop [NOTES]...\n\
Any arguments supplied are recorded with the trace as a stop reason and\n\
reported by tstatus (if the target supports trace notes)."));

  add_com ("tstart", class_trace, tstart_command, _("\
Start trace data collection.\n\
Usage: tstart [NOTES]...\n\
Any arguments supplied are recorded with the trace as a note and\n\
reported by tstatus (if the target supports trace notes)."));

  add_com ("end", class_trace, end_actions_pseudocommand, _("\
Ends a list of commands or actions.\n\
Several GDB commands allow you to enter a list of commands or actions.\n\
Entering \"end\" on a line by itself is the normal way to terminate\n\
such a list.\n\n\
Note: the \"end\" command cannot be used at the gdb prompt."));

  while_stepping_cmd_element = add_com ("while-stepping", class_trace,
					while_stepping_pseudocommand, _("\
Specify single-stepping behavior at a tracepoint.\n\
Argument is number of instructions to trace in single-step mode\n\
following the tracepoint.  This command is normally followed by\n\
one or more \"collect\" commands, to specify what to collect\n\
while single-stepping.\n\n\
Note: this command can only be used in a tracepoint \"actions\" list."));

  add_com_alias ("ws", while_stepping_cmd_element, class_trace, 0);
  add_com_alias ("stepping", while_stepping_cmd_element, class_trace, 0);

  add_com ("collect", class_trace, collect_pseudocommand, _("\
Specify one or more data items to be collected at a tracepoint.\n\
Accepts a comma-separated list of (one or more) expressions.  GDB will\n\
collect all data (variables, registers) referenced by that expression.\n\
Also accepts the following special arguments:\n\
    $regs   -- all registers.\n\
    $args   -- all function arguments.\n\
    $locals -- all variables local to the block/function scope.\n\
    $_sdata -- static tracepoint data (ignored for non-static tracepoints).\n\
Note: this command can only be used in a tracepoint \"actions\" list."));

  add_com ("teval", class_trace, teval_pseudocommand, _("\
Specify one or more expressions to be evaluated at a tracepoint.\n\
Accepts a comma-separated list of (one or more) expressions.\n\
The result of each evaluation will be discarded.\n\
Note: this command can only be used in a tracepoint \"actions\" list."));

  add_com ("actions", class_trace, actions_command, _("\
Specify the actions to be taken at a tracepoint.\n\
Tracepoint actions may include collecting of specified data,\n\
single-stepping, or enabling/disabling other tracepoints,\n\
depending on target's capabilities."));

  add_setshow_string_cmd ("default-collect", class_trace,
			  &default_collect, _("\
Set the list of expressions to collect by default."), _("\
Show the list of expressions to collect by default."), NULL,
			  NULL, NULL,
			  &setlist, &showlist);

  add_setshow_boolean_cmd ("disconnected-tracing", no_class,
			   &disconnected_tracing, _("\
Set whether tracing continues after GDB disconnects."), _("\
Show whether tracing continues after GDB disconnects."), _("\
Use this to continue a tracing run even if GDB disconnects\n\
or detaches from the target.  You can reconnect later and look at\n\
trace data collected in the meantime."),
			   set_disconnected_tracing,
			   NULL,
			   &setlist,
			   &showlist);

  add_setshow_boolean_cmd ("circular-trace-buffer", no_class,
			   &circular_trace_buffer, _("\
Set target's use of circular trace buffer."), _("\
Show target's use of circular trace buffer."), _("\
Use this to make the trace buffer into a circular buffer,\n\
which will discard traceframes (oldest first) instead of filling\n\
up and stopping the trace run."),
			   set_circular_trace_buffer,
			   NULL,
			   &setlist,
			   &showlist);

  add_setshow_zuinteger_unlimited_cmd ("trace-buffer-size", no_class,
				       &trace_buffer_size, _("\
Set requested size of trace buffer."), _("\
Show requested size of trace buffer."), _("\
Use this to choose a size for the trace buffer.  Some targets\n\
may have fixed or limited buffer sizes.  Specifying \"unlimited\" or -1\n\
disables any attempt to set the buffer size and lets the target choose."),
				       set_trace_buffer_size, NULL,
				       &setlist, &showlist);

  add_setshow_string_cmd ("trace-user", class_trace,
			  &trace_user, _("\
Set the user name to use for current and future trace runs."), _("\
Show the user name to use for current and future trace runs."), NULL,
			  set_trace_user, NULL,
			  &setlist, &showlist);

  add_setshow_string_cmd ("trace-notes", class_trace,
			  &trace_notes, _("\
Set notes string to use for current and future trace runs."), _("\
Show the notes string to use for current and future trace runs."), NULL,
			  set_trace_notes, NULL,
			  &setlist, &showlist);

  add_setshow_string_cmd ("trace-stop-notes", class_trace,
			  &trace_stop_notes, _("\
Set notes string to use for future tstop commands."), _("\
Show the notes string to use for future tstop commands."), NULL,
			  set_trace_stop_notes, NULL,
			  &setlist, &showlist);
}
