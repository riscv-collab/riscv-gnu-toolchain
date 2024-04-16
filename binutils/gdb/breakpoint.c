/* Everything about breakpoints, for GDB.

   Copyright (C) 1986-2024 Free Software Foundation, Inc.

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
#include <ctype.h>
#include "hashtab.h"
#include "symtab.h"
#include "frame.h"
#include "breakpoint.h"
#include "tracepoint.h"
#include "gdbtypes.h"
#include "expression.h"
#include "gdbcore.h"
#include "gdbcmd.h"
#include "value.h"
#include "command.h"
#include "inferior.h"
#include "infrun.h"
#include "gdbthread.h"
#include "target.h"
#include "language.h"
#include "gdb-demangle.h"
#include "filenames.h"
#include "annotate.h"
#include "symfile.h"
#include "objfiles.h"
#include "source.h"
#include "linespec.h"
#include "completer.h"
#include "ui-out.h"
#include "cli/cli-script.h"
#include "block.h"
#include "solib.h"
#include "solist.h"
#include "observable.h"
#include "memattr.h"
#include "ada-lang.h"
#include "top.h"
#include "ui.h"
#include "valprint.h"
#include "jit.h"
#include "parser-defs.h"
#include "gdbsupport/gdb_regex.h"
#include "probe.h"
#include "cli/cli-utils.h"
#include "stack.h"
#include "ax-gdb.h"
#include "dummy-frame.h"
#include "interps.h"
#include "gdbsupport/format.h"
#include "thread-fsm.h"
#include "tid-parse.h"
#include "cli/cli-style.h"
#include "cli/cli-decode.h"
#include <unordered_set>

/* readline include files */
#include "readline/tilde.h"

/* readline defines this.  */
#undef savestring

#include "mi/mi-common.h"
#include "extension.h"
#include <algorithm>
#include "progspace-and-thread.h"
#include "gdbsupport/array-view.h"
#include <optional>
#include "gdbsupport/common-utils.h"

/* Prototypes for local functions.  */

static void map_breakpoint_numbers (const char *,
				    gdb::function_view<void (breakpoint *)>);

static void
  create_sals_from_location_spec_default (location_spec *locspec,
					  linespec_result *canonical);

static void create_breakpoints_sal (struct gdbarch *,
				    struct linespec_result *,
				    gdb::unique_xmalloc_ptr<char>,
				    gdb::unique_xmalloc_ptr<char>,
				    enum bptype,
				    enum bpdisp, int, int, int,
				    int,
				    int, int, int, unsigned);

static int can_use_hardware_watchpoint
    (const std::vector<value_ref_ptr> &vals);

static void mention (const breakpoint *);

static breakpoint *add_to_breakpoint_chain (std::unique_ptr<breakpoint> &&b);

static breakpoint *add_to_breakpoint_chain (std::unique_ptr<breakpoint> &&b);

static struct breakpoint *
  momentary_breakpoint_from_master (struct breakpoint *orig,
				    enum bptype type,
				    int loc_enabled, int thread);

static void breakpoint_adjustment_warning (CORE_ADDR, CORE_ADDR, int, bool);

static CORE_ADDR adjust_breakpoint_address (struct gdbarch *gdbarch,
					    CORE_ADDR bpaddr,
					    enum bptype bptype,
					    struct program_space *pspace);

static bool watchpoint_locations_match (const struct bp_location *loc1,
					const struct bp_location *loc2);

static bool breakpoint_locations_match (const struct bp_location *loc1,
					const struct bp_location *loc2,
					bool sw_hw_bps_match = false);

static bool breakpoint_location_address_match (struct bp_location *bl,
					       const struct address_space *aspace,
					       CORE_ADDR addr);

static bool breakpoint_location_address_range_overlap (struct bp_location *,
						       const address_space *,
						       CORE_ADDR, int);

static int remove_breakpoint (struct bp_location *);
static int remove_breakpoint_1 (struct bp_location *, enum remove_bp_reason);

static enum print_stop_action print_bp_stop_message (bpstat *bs);

static int hw_breakpoint_used_count (void);

static int hw_watchpoint_use_count (struct breakpoint *);

static int hw_watchpoint_used_count_others (struct breakpoint *except,
					    enum bptype type,
					    int *other_type_used);

static void enable_breakpoint_disp (struct breakpoint *, enum bpdisp,
				    int count);

static void decref_bp_location (struct bp_location **loc);

static std::vector<symtab_and_line> bkpt_probe_decode_location_spec
     (struct breakpoint *b,
      location_spec *locspec,
      struct program_space *search_pspace);

static bool bl_address_is_meaningful (bp_location *loc);

static int find_loc_num_by_location (const bp_location *loc);

/* update_global_location_list's modes of operation wrt to whether to
   insert locations now.  */
enum ugll_insert_mode
{
  /* Don't insert any breakpoint locations into the inferior, only
     remove already-inserted locations that no longer should be
     inserted.  Functions that delete a breakpoint or breakpoints
     should specify this mode, so that deleting a breakpoint doesn't
     have the side effect of inserting the locations of other
     breakpoints that are marked not-inserted, but should_be_inserted
     returns true on them.

     This behavior is useful is situations close to tear-down -- e.g.,
     after an exec, while the target still has execution, but
     breakpoint shadows of the previous executable image should *NOT*
     be restored to the new image; or before detaching, where the
     target still has execution and wants to delete breakpoints from
     GDB's lists, and all breakpoints had already been removed from
     the inferior.  */
  UGLL_DONT_INSERT,

  /* May insert breakpoints iff breakpoints_should_be_inserted_now
     claims breakpoints should be inserted now.  */
  UGLL_MAY_INSERT,

  /* Insert locations now, irrespective of
     breakpoints_should_be_inserted_now.  E.g., say all threads are
     stopped right now, and the user did "continue".  We need to
     insert breakpoints _before_ resuming the target, but
     UGLL_MAY_INSERT wouldn't insert them, because
     breakpoints_should_be_inserted_now returns false at that point,
     as no thread is running yet.  */
  UGLL_INSERT
};

/* Return a textual version of INSERT_MODE.  */

static const char *
ugll_insert_mode_text (ugll_insert_mode insert_mode)
{
/* Make sure the compiler warns if a new ugll_insert_mode enumerator is added
   but not handled here.  */
DIAGNOSTIC_PUSH
DIAGNOSTIC_ERROR_SWITCH
  switch (insert_mode)
    {
    case UGLL_DONT_INSERT:
      return "UGLL_DONT_INSERT";
    case UGLL_MAY_INSERT:
      return "UGLL_MAY_INSERT";
    case UGLL_INSERT:
      return "UGLL_INSERT";
    }
DIAGNOSTIC_POP

  gdb_assert_not_reached ("must handle all enum values");
}

/* Return a textual version of REASON.  */

static const char *
remove_bp_reason_str (remove_bp_reason reason)
{
/* Make sure the compiler warns if a new remove_bp_reason enumerator is added
   but not handled here.  */
DIAGNOSTIC_PUSH
DIAGNOSTIC_ERROR_SWITCH
  switch (reason)
    {
    case REMOVE_BREAKPOINT:
      return "regular remove";
    case DETACH_BREAKPOINT:
      return "detach";
    }
DIAGNOSTIC_POP

  gdb_assert_not_reached ("must handle all enum values");
}

/* Return a textual version of breakpoint location BL describing number,
   location and address.  */

static std::string
breakpoint_location_address_str (const bp_location *bl)
{
  std::string str = string_printf ("Breakpoint %d (%s) at address %s",
				   bl->owner->number,
				   host_address_to_string (bl),
				   paddress (bl->gdbarch, bl->address));

  std::string loc_string = bl->to_string ();
  if (!loc_string.empty ())
    str += string_printf (" %s", loc_string.c_str ());

  return str;
}

static void update_global_location_list (enum ugll_insert_mode);

static void update_global_location_list_nothrow (enum ugll_insert_mode);

static void insert_breakpoint_locations (void);

static void trace_pass_command (const char *, int);

static void set_tracepoint_count (int num);

static bool is_masked_watchpoint (const struct breakpoint *b);

/* Return true if B refers to a static tracepoint set by marker ("-m"),
   zero otherwise.  */

static bool strace_marker_p (struct breakpoint *b);

static void bkpt_probe_create_sals_from_location_spec
     (location_spec *locspec,
      struct linespec_result *canonical);
static void tracepoint_probe_create_sals_from_location_spec
     (location_spec *locspec,
      struct linespec_result *canonical);

const struct breakpoint_ops code_breakpoint_ops =
{
  create_sals_from_location_spec_default,
  create_breakpoints_sal,
};

/* Breakpoints set on probes.  */
static const struct breakpoint_ops bkpt_probe_breakpoint_ops =
{
  bkpt_probe_create_sals_from_location_spec,
  create_breakpoints_sal,
};

/* Tracepoints set on probes.  */
static const struct breakpoint_ops tracepoint_probe_breakpoint_ops =
{
  tracepoint_probe_create_sals_from_location_spec,
  create_breakpoints_sal,
};

/* Implementation of abstract dtors.  These must exist to satisfy the
   linker.  */

breakpoint::~breakpoint ()
{
}

code_breakpoint::~code_breakpoint ()
{
}

catchpoint::~catchpoint ()
{
}

/* The structure to be used in regular breakpoints.  */
struct ordinary_breakpoint : public code_breakpoint
{
  using code_breakpoint::code_breakpoint;

  int resources_needed (const struct bp_location *) override;
  enum print_stop_action print_it (const bpstat *bs) const override;
  void print_mention () const override;
  void print_recreate (struct ui_file *fp) const override;
};

/* Internal breakpoints.  These typically have a lifetime the same as
   the program, and they end up installed on the breakpoint chain with
   a negative breakpoint number.  They're visible in "maint info
   breakpoints", but not "info breakpoints".  */
struct internal_breakpoint : public code_breakpoint
{
  internal_breakpoint (struct gdbarch *gdbarch,
		       enum bptype type, CORE_ADDR address)
    : code_breakpoint (gdbarch, type)
  {
    symtab_and_line sal;
    sal.pc = address;
    sal.section = find_pc_overlay (sal.pc);
    sal.pspace = current_program_space;
    add_location (sal);

    pspace = current_program_space;
    disposition = disp_donttouch;
  }

  void re_set () override;
  void check_status (struct bpstat *bs) override;
  enum print_stop_action print_it (const bpstat *bs) const override;
  void print_mention () const override;
};

/* Momentary breakpoints.  These typically have a lifetime of some run
   control command only, are always thread-specific, and have 0 for
   breakpoint number.  I.e., there can be many momentary breakpoints
   on the breakpoint chain and they all same the same number (zero).
   They're visible in "maint info breakpoints", but not "info
   breakpoints".  */
struct momentary_breakpoint : public code_breakpoint
{
  momentary_breakpoint (struct gdbarch *gdbarch_, enum bptype bptype,
			program_space *pspace_,
			const struct frame_id &frame_id_,
			int thread_)
    : code_breakpoint (gdbarch_, bptype)
  {
    /* If FRAME_ID is valid, it should be a real frame, not an inlined
       or tail-called one.  */
    gdb_assert (!frame_id_artificial_p (frame_id));

    /* Momentary breakpoints are always thread-specific.  */
    gdb_assert (thread_ > 0);

    pspace = pspace_;
    enable_state = bp_enabled;
    disposition = disp_donttouch;
    frame_id = frame_id_;
    thread = thread_;

    /* The inferior should have been set by the parent constructor.  */
    gdb_assert (inferior == -1);
  }

  void re_set () override;
  void check_status (struct bpstat *bs) override;
  enum print_stop_action print_it (const bpstat *bs) const override;
  void print_mention () const override;
};

/* DPrintf breakpoints.  */
struct dprintf_breakpoint : public ordinary_breakpoint
{
  using ordinary_breakpoint::ordinary_breakpoint;

  void re_set () override;
  int breakpoint_hit (const struct bp_location *bl,
		      const address_space *aspace,
		      CORE_ADDR bp_addr,
		      const target_waitstatus &ws) override;
  void print_recreate (struct ui_file *fp) const override;
  void after_condition_true (struct bpstat *bs) override;
};

/* Ranged breakpoints.  */
struct ranged_breakpoint : public ordinary_breakpoint
{
  explicit ranged_breakpoint (struct gdbarch *gdbarch,
			      const symtab_and_line &sal_start,
			      int length,
			      location_spec_up start_locspec,
			      location_spec_up end_locspec)
    : ordinary_breakpoint (gdbarch, bp_hardware_breakpoint)
  {
    bp_location *bl = add_location (sal_start);
    bl->length = length;

    disposition = disp_donttouch;

    locspec = std::move (start_locspec);
    locspec_range_end = std::move (end_locspec);
  }

  int breakpoint_hit (const struct bp_location *bl,
		      const address_space *aspace,
		      CORE_ADDR bp_addr,
		      const target_waitstatus &ws) override;
  int resources_needed (const struct bp_location *) override;
  enum print_stop_action print_it (const bpstat *bs) const override;
  bool print_one (const bp_location **) const override;
  void print_one_detail (struct ui_out *) const override;
  void print_mention () const override;
  void print_recreate (struct ui_file *fp) const override;
};

/* Static tracepoints with marker (`-m').  */
struct static_marker_tracepoint : public tracepoint
{
  using tracepoint::tracepoint;

  std::vector<symtab_and_line> decode_location_spec
       (struct location_spec *locspec,
	struct program_space *search_pspace) override;
};

/* The style in which to perform a dynamic printf.  This is a user
   option because different output options have different tradeoffs;
   if GDB does the printing, there is better error handling if there
   is a problem with any of the arguments, but using an inferior
   function lets you have special-purpose printers and sending of
   output to the same place as compiled-in print functions.  */

static const char dprintf_style_gdb[] = "gdb";
static const char dprintf_style_call[] = "call";
static const char dprintf_style_agent[] = "agent";
static const char *const dprintf_style_enums[] = {
  dprintf_style_gdb,
  dprintf_style_call,
  dprintf_style_agent,
  NULL
};
static const char *dprintf_style = dprintf_style_gdb;

/* The function to use for dynamic printf if the preferred style is to
   call into the inferior.  The value is simply a string that is
   copied into the command, so it can be anything that GDB can
   evaluate to a callable address, not necessarily a function name.  */

static std::string dprintf_function = "printf";

/* The channel to use for dynamic printf if the preferred style is to
   call into the inferior; if a nonempty string, it will be passed to
   the call as the first argument, with the format string as the
   second.  As with the dprintf function, this can be anything that
   GDB knows how to evaluate, so in addition to common choices like
   "stderr", this could be an app-specific expression like
   "mystreams[curlogger]".  */

static std::string dprintf_channel;

/* True if dprintf commands should continue to operate even if GDB
   has disconnected.  */
static bool disconnected_dprintf = true;

struct command_line *
breakpoint_commands (struct breakpoint *b)
{
  return b->commands ? b->commands.get () : NULL;
}

/* Flag indicating that a command has proceeded the inferior past the
   current breakpoint.  */

static bool breakpoint_proceeded;

const char *
bpdisp_text (enum bpdisp disp)
{
  /* NOTE: the following values are a part of MI protocol and
     represent values of 'disp' field returned when inferior stops at
     a breakpoint.  */
  static const char * const bpdisps[] = {"del", "dstp", "dis", "keep"};

  return bpdisps[(int) disp];
}

/* Prototypes for exported functions.  */
/* If FALSE, gdb will not use hardware support for watchpoints, even
   if such is available.  */
static int can_use_hw_watchpoints;

static void
show_can_use_hw_watchpoints (struct ui_file *file, int from_tty,
			     struct cmd_list_element *c,
			     const char *value)
{
  gdb_printf (file,
	      _("Debugger's willingness to use "
		"watchpoint hardware is %s.\n"),
	      value);
}

/* If AUTO_BOOLEAN_FALSE, gdb will not attempt to create pending breakpoints.
   If AUTO_BOOLEAN_TRUE, gdb will automatically create pending breakpoints
   for unrecognized breakpoint locations.
   If AUTO_BOOLEAN_AUTO, gdb will query when breakpoints are unrecognized.  */
static enum auto_boolean pending_break_support;
static void
show_pending_break_support (struct ui_file *file, int from_tty,
			    struct cmd_list_element *c,
			    const char *value)
{
  gdb_printf (file,
	      _("Debugger's behavior regarding "
		"pending breakpoints is %s.\n"),
	      value);
}

/* If true, gdb will automatically use hardware breakpoints for breakpoints
   set with "break" but falling in read-only memory.
   If false, gdb will warn about such breakpoints, but won't automatically
   use hardware breakpoints.  */
static bool automatic_hardware_breakpoints;
static void
show_automatic_hardware_breakpoints (struct ui_file *file, int from_tty,
				     struct cmd_list_element *c,
				     const char *value)
{
  gdb_printf (file,
	      _("Automatic usage of hardware breakpoints is %s.\n"),
	      value);
}

/* If on, GDB keeps breakpoints inserted even if the inferior is
   stopped, and immediately inserts any new breakpoints as soon as
   they're created.  If off (default), GDB keeps breakpoints off of
   the target as long as possible.  That is, it delays inserting
   breakpoints until the next resume, and removes them again when the
   target fully stops.  This is a bit safer in case GDB crashes while
   processing user input.  */
static bool always_inserted_mode = false;

static void
show_always_inserted_mode (struct ui_file *file, int from_tty,
		     struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Always inserted breakpoint mode is %s.\n"),
	      value);
}

/* True if breakpoint debug output is enabled.  */
static bool debug_breakpoint = false;

/* Print a "breakpoint" debug statement.  */
#define breakpoint_debug_printf(fmt, ...) \
  debug_prefixed_printf_cond (debug_breakpoint, "breakpoint", fmt, \
			      ##__VA_ARGS__)

/* "show debug breakpoint" implementation.  */
static void
show_debug_breakpoint (struct ui_file *file, int from_tty,
		       struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Breakpoint location debugging is %s.\n"), value);
}

/* See breakpoint.h.  */

int
breakpoints_should_be_inserted_now (void)
{
  if (gdbarch_has_global_breakpoints (current_inferior ()->arch ()))
    {
      /* If breakpoints are global, they should be inserted even if no
	 thread under gdb's control is running, or even if there are
	 no threads under GDB's control yet.  */
      return 1;
    }
  else
    {
      if (always_inserted_mode)
	{
	  /* The user wants breakpoints inserted even if all threads
	     are stopped.  */
	  return 1;
	}

      for (inferior *inf : all_inferiors ())
	if (inf->has_execution ()
	    && threads_are_executing (inf->process_target ()))
	  return 1;

      /* Don't remove breakpoints yet if, even though all threads are
	 stopped, we still have events to process.  */
      for (thread_info *tp : all_non_exited_threads ())
	if (tp->resumed () && tp->has_pending_waitstatus ())
	  return 1;
    }
  return 0;
}

static const char condition_evaluation_both[] = "host or target";

/* Modes for breakpoint condition evaluation.  */
static const char condition_evaluation_auto[] = "auto";
static const char condition_evaluation_host[] = "host";
static const char condition_evaluation_target[] = "target";
static const char *const condition_evaluation_enums[] = {
  condition_evaluation_auto,
  condition_evaluation_host,
  condition_evaluation_target,
  NULL
};

/* Global that holds the current mode for breakpoint condition evaluation.  */
static const char *condition_evaluation_mode_1 = condition_evaluation_auto;

/* Global that we use to display information to the user (gets its value from
   condition_evaluation_mode_1.  */
static const char *condition_evaluation_mode = condition_evaluation_auto;

/* Translate a condition evaluation mode MODE into either "host"
   or "target".  This is used mostly to translate from "auto" to the
   real setting that is being used.  It returns the translated
   evaluation mode.  */

static const char *
translate_condition_evaluation_mode (const char *mode)
{
  if (mode == condition_evaluation_auto)
    {
      if (target_supports_evaluation_of_breakpoint_conditions ())
	return condition_evaluation_target;
      else
	return condition_evaluation_host;
    }
  else
    return mode;
}

/* Discovers what condition_evaluation_auto translates to.  */

static const char *
breakpoint_condition_evaluation_mode (void)
{
  return translate_condition_evaluation_mode (condition_evaluation_mode);
}

/* Return true if GDB should evaluate breakpoint conditions or false
   otherwise.  */

static bool
gdb_evaluates_breakpoint_condition_p (void)
{
  const char *mode = breakpoint_condition_evaluation_mode ();

  return (mode == condition_evaluation_host);
}

/* Are we executing breakpoint commands?  */
static int executing_breakpoint_commands;

/* Are overlay event breakpoints enabled? */
static int overlay_events_enabled;

/* See description in breakpoint.h. */
bool target_exact_watchpoints = false;

/* Chains of all breakpoints defined.  */

static intrusive_list<breakpoint> breakpoint_chain;

/* See breakpoint.h.  */

breakpoint_range
all_breakpoints ()
{
  return breakpoint_range (breakpoint_chain.begin (), breakpoint_chain.end ());
}

/* See breakpoint.h.  */

breakpoint_safe_range
all_breakpoints_safe ()
{
  return breakpoint_safe_range (all_breakpoints ());
}

/* See breakpoint.h.  */

tracepoint_range
all_tracepoints ()
{
  return tracepoint_range (tracepoint_iterator (breakpoint_chain.begin ()),
			   tracepoint_iterator (breakpoint_chain.end ()));
}

/* Array is sorted by bp_location_is_less_than - primarily by the ADDRESS.  */

static std::vector<bp_location *> bp_locations;

/* See breakpoint.h.  */

const std::vector<bp_location *> &
all_bp_locations ()
{
  return bp_locations;
}

/* Range to iterate over breakpoint locations at a given address.  */

struct bp_locations_at_addr_range
{
  using iterator = std::vector<bp_location *>::iterator;

  bp_locations_at_addr_range (CORE_ADDR addr)
  {
    struct compare
    {
      bool operator() (const bp_location *loc, CORE_ADDR addr_) const
      { return loc->address < addr_; }

      bool operator() (CORE_ADDR addr_, const bp_location *loc) const
      { return addr_ < loc->address; }
    };

    auto it_pair = std::equal_range (bp_locations.begin (), bp_locations.end (),
				     addr, compare ());

    m_begin = it_pair.first;
    m_end = it_pair.second;
  }

  iterator begin () const
  { return m_begin; }

  iterator end () const
  { return m_end; }

private:
  iterator m_begin;
  iterator m_end;
};

/* Return a range to iterate over all breakpoint locations exactly at address
   ADDR.

   If it's needed to iterate multiple times on the same range, it's possible
   to save the range in a local variable and use it multiple times:

     auto range = all_bp_locations_at_addr (addr);

     for (bp_location *loc : range)
       // use loc

     for (bp_location *loc : range)
       // use loc

   This saves a bit of time, as it avoids re-doing the binary searches to find
   the range's boundaries.  Just remember not to change the bp_locations vector
   in the mean time, as it could make the range's iterators stale.  */

static bp_locations_at_addr_range
all_bp_locations_at_addr (CORE_ADDR addr)
{
  return bp_locations_at_addr_range (addr);
}

/* Maximum alignment offset between bp_target_info.PLACED_ADDRESS and
   ADDRESS for the current elements of BP_LOCATIONS which get a valid
   result from bp_location_has_shadow.  You can use it for roughly
   limiting the subrange of BP_LOCATIONS to scan for shadow bytes for
   an address you need to read.  */

static CORE_ADDR bp_locations_placed_address_before_address_max;

/* Maximum offset plus alignment between bp_target_info.PLACED_ADDRESS
   + bp_target_info.SHADOW_LEN and ADDRESS for the current elements of
   BP_LOCATIONS which get a valid result from bp_location_has_shadow.
   You can use it for roughly limiting the subrange of BP_LOCATIONS to
   scan for shadow bytes for an address you need to read.  */

static CORE_ADDR bp_locations_shadow_len_after_address_max;

/* The locations that no longer correspond to any breakpoint, unlinked
   from the bp_locations array, but for which a hit may still be
   reported by a target.  */
static std::vector<bp_location *> moribund_locations;

/* Number of last breakpoint made.  */

static int breakpoint_count;

/* The value of `breakpoint_count' before the last command that
   created breakpoints.  If the last (break-like) command created more
   than one breakpoint, then the difference between BREAKPOINT_COUNT
   and PREV_BREAKPOINT_COUNT is more than one.  */
static int prev_breakpoint_count;

/* Number of last tracepoint made.  */

static int tracepoint_count;

static struct cmd_list_element *breakpoint_set_cmdlist;
static struct cmd_list_element *breakpoint_show_cmdlist;
struct cmd_list_element *save_cmdlist;

/* Return whether a breakpoint is an active enabled breakpoint.  */
static bool
breakpoint_enabled (struct breakpoint *b)
{
  return (b->enable_state == bp_enabled);
}

/* Set breakpoint count to NUM.  */

static void
set_breakpoint_count (int num)
{
  prev_breakpoint_count = breakpoint_count;
  breakpoint_count = num;
  set_internalvar_integer (lookup_internalvar ("bpnum"), num);
}

/* Used by `start_rbreak_breakpoints' below, to record the current
   breakpoint count before "rbreak" creates any breakpoint.  */
static int rbreak_start_breakpoint_count;

/* Called at the start an "rbreak" command to record the first
   breakpoint made.  */

scoped_rbreak_breakpoints::scoped_rbreak_breakpoints ()
{
  rbreak_start_breakpoint_count = breakpoint_count;
}

/* Called at the end of an "rbreak" command to record the last
   breakpoint made.  */

scoped_rbreak_breakpoints::~scoped_rbreak_breakpoints ()
{
  prev_breakpoint_count = rbreak_start_breakpoint_count;
}

/* Used in run_command to zero the hit count when a new run starts.  */

void
clear_breakpoint_hit_counts (void)
{
  for (breakpoint &b : all_breakpoints ())
    b.hit_count = 0;
}


/* Return the breakpoint with the specified number, or NULL
   if the number does not refer to an existing breakpoint.  */

struct breakpoint *
get_breakpoint (int num)
{
  for (breakpoint &b : all_breakpoints ())
    if (b.number == num)
      return &b;
  
  return nullptr;
}

/* Return TRUE if NUM refer to an existing breakpoint that has
   multiple code locations.  */

static bool
has_multiple_locations (int num)
{
  for (breakpoint &b : all_breakpoints ())
    if (b.number == num)
      return b.has_multiple_locations ();

  return false;
}



/* Mark locations as "conditions have changed" in case the target supports
   evaluating conditions on its side.  */

static void
mark_breakpoint_modified (struct breakpoint *b)
{
  /* This is only meaningful if the target is
     evaluating conditions and if the user has
     opted for condition evaluation on the target's
     side.  */
  if (gdb_evaluates_breakpoint_condition_p ()
      || !target_supports_evaluation_of_breakpoint_conditions ())
    return;

  if (!is_breakpoint (b))
    return;

  for (bp_location &loc : b->locations ())
    loc.condition_changed = condition_modified;
}

/* Mark location as "conditions have changed" in case the target supports
   evaluating conditions on its side.  */

static void
mark_breakpoint_location_modified (struct bp_location *loc)
{
  /* This is only meaningful if the target is
     evaluating conditions and if the user has
     opted for condition evaluation on the target's
     side.  */
  if (gdb_evaluates_breakpoint_condition_p ()
      || !target_supports_evaluation_of_breakpoint_conditions ())

    return;

  if (!is_breakpoint (loc->owner))
    return;

  loc->condition_changed = condition_modified;
}

/* Sets the condition-evaluation mode using the static global
   condition_evaluation_mode.  */

static void
set_condition_evaluation_mode (const char *args, int from_tty,
			       struct cmd_list_element *c)
{
  const char *old_mode, *new_mode;

  if ((condition_evaluation_mode_1 == condition_evaluation_target)
      && !target_supports_evaluation_of_breakpoint_conditions ())
    {
      condition_evaluation_mode_1 = condition_evaluation_mode;
      warning (_("Target does not support breakpoint condition evaluation.\n"
		 "Using host evaluation mode instead."));
      return;
    }

  new_mode = translate_condition_evaluation_mode (condition_evaluation_mode_1);
  old_mode = translate_condition_evaluation_mode (condition_evaluation_mode);

  /* Flip the switch.  Flip it even if OLD_MODE == NEW_MODE as one of the
     settings was "auto".  */
  condition_evaluation_mode = condition_evaluation_mode_1;

  /* Only update the mode if the user picked a different one.  */
  if (new_mode != old_mode)
    {
      /* If the user switched to a different evaluation mode, we
	 need to synch the changes with the target as follows:

	 "host" -> "target": Send all (valid) conditions to the target.
	 "target" -> "host": Remove all the conditions from the target.
      */

      if (new_mode == condition_evaluation_target)
	{
	  /* Mark everything modified and synch conditions with the
	     target.  */
	  for (bp_location *loc : all_bp_locations ())
	    mark_breakpoint_location_modified (loc);
	}
      else
	{
	  /* Manually mark non-duplicate locations to synch conditions
	     with the target.  We do this to remove all the conditions the
	     target knows about.  */
	  for (bp_location *loc : all_bp_locations ())
	    if (is_breakpoint (loc->owner) && loc->inserted)
	      loc->needs_update = 1;
	}

      /* Do the update.  */
      update_global_location_list (UGLL_MAY_INSERT);
    }

  return;
}

/* Shows the current mode of breakpoint condition evaluation.  Explicitly shows
   what "auto" is translating to.  */

static void
show_condition_evaluation_mode (struct ui_file *file, int from_tty,
				struct cmd_list_element *c, const char *value)
{
  if (condition_evaluation_mode == condition_evaluation_auto)
    gdb_printf (file,
		_("Breakpoint condition evaluation "
		  "mode is %s (currently %s).\n"),
		value,
		breakpoint_condition_evaluation_mode ());
  else
    gdb_printf (file, _("Breakpoint condition evaluation mode is %s.\n"),
		value);
}

/* Parse COND_STRING in the context of LOC and set as the condition
   expression of LOC.  BP_NUM is the number of LOC's owner, LOC_NUM is
   the number of LOC within its owner.  In case of parsing error, mark
   LOC as DISABLED_BY_COND.  In case of success, unset DISABLED_BY_COND.  */

static void
set_breakpoint_location_condition (const char *cond_string, bp_location *loc,
				   int bp_num, int loc_num)
{
  bool has_junk = false;
  try
    {
      expression_up new_exp = parse_exp_1 (&cond_string, loc->address,
					   block_for_pc (loc->address), 0);
      if (*cond_string != 0)
	has_junk = true;
      else
	{
	  loc->cond = std::move (new_exp);
	  if (loc->disabled_by_cond && loc->enabled)
	    gdb_printf (_("Breakpoint %d's condition is now valid at "
			  "location %d, enabling.\n"),
			bp_num, loc_num);

	  loc->disabled_by_cond = false;
	}
    }
  catch (const gdb_exception_error &e)
    {
      if (loc->enabled)
	{
	  /* Warn if a user-enabled location is now becoming disabled-by-cond.
	     BP_NUM is 0 if the breakpoint is being defined for the first
	     time using the "break ... if ..." command, and non-zero if
	     already defined.  */
	  if (bp_num != 0)
	    warning (_("failed to validate condition at location %d.%d, "
		       "disabling:\n  %s"), bp_num, loc_num, e.what ());
	  else
	    warning (_("failed to validate condition at location %d, "
		       "disabling:\n  %s"), loc_num, e.what ());
	}

      loc->disabled_by_cond = true;
    }

  if (has_junk)
    error (_("Garbage '%s' follows condition"), cond_string);
}

/* See breakpoint.h.  */

void
notify_breakpoint_modified (breakpoint *b)
{
  interps_notify_breakpoint_modified (b);
  gdb::observers::breakpoint_modified.notify (b);
}

void
set_breakpoint_condition (struct breakpoint *b, const char *exp,
			  int from_tty, bool force)
{
  if (*exp == 0)
    {
      b->cond_string.reset ();

      if (is_watchpoint (b))
	gdb::checked_static_cast<watchpoint *> (b)->cond_exp.reset ();
      else
	{
	  int loc_num = 1;
	  for (bp_location &loc : b->locations ())
	    {
	      loc.cond.reset ();
	      if (loc.disabled_by_cond && loc.enabled)
		gdb_printf (_("Breakpoint %d's condition is now valid at "
			      "location %d, enabling.\n"),
			    b->number, loc_num);
	      loc.disabled_by_cond = false;
	      loc_num++;

	      /* No need to free the condition agent expression
		 bytecode (if we have one).  We will handle this
		 when we go through update_global_location_list.  */
	    }
	}

      if (from_tty)
	gdb_printf (_("Breakpoint %d now unconditional.\n"), b->number);
    }
  else
    {
      if (is_watchpoint (b))
	{
	  innermost_block_tracker tracker;
	  const char *arg = exp;
	  expression_up new_exp = parse_exp_1 (&arg, 0, 0, 0, &tracker);
	  if (*arg != 0)
	    error (_("Junk at end of expression"));
	  watchpoint *w = gdb::checked_static_cast<watchpoint *> (b);
	  w->cond_exp = std::move (new_exp);
	  w->cond_exp_valid_block = tracker.block ();
	}
      else
	{
	  /* Parse and set condition expressions.  We make two passes.
	     In the first, we parse the condition string to see if it
	     is valid in at least one location.  If so, the condition
	     would be accepted.  So we go ahead and set the locations'
	     conditions.  In case no valid case is found, we throw
	     the error and the condition string will be rejected.
	     This two-pass approach is taken to avoid setting the
	     state of locations in case of a reject.  */
	  for (const bp_location &loc : b->locations ())
	    {
	      try
		{
		  const char *arg = exp;
		  parse_exp_1 (&arg, loc.address,
			       block_for_pc (loc.address), 0);
		  if (*arg != 0)
		    error (_("Junk at end of expression"));
		  break;
		}
	      catch (const gdb_exception_error &e)
		{
		  /* Condition string is invalid.  If this happens to
		     be the last loc, abandon (if not forced) or continue
		     (if forced).  */
		  if (&loc == &b->last_loc () && !force)
		    throw;
		}
	    }

	  /* If we reach here, the condition is valid at some locations.  */
	  int loc_num = 1;
	  for (bp_location &loc : b->locations ())
	    {
	      set_breakpoint_location_condition (exp, &loc, b->number, loc_num);
	      loc_num++;
	    }
	}

      /* We know that the new condition parsed successfully.  The
	 condition string of the breakpoint can be safely updated.  */
      b->cond_string = make_unique_xstrdup (exp);
      b->condition_not_parsed = 0;
    }
  mark_breakpoint_modified (b);

  notify_breakpoint_modified (b);
}

/* See breakpoint.h.  */

void
set_breakpoint_condition (int bpnum, const char *exp, int from_tty,
			  bool force)
{
  for (breakpoint &b : all_breakpoints ())
    if (b.number == bpnum)
      {
	/* Check if this breakpoint has a "stop" method implemented in an
	   extension language.  This method and conditions entered into GDB
	   from the CLI are mutually exclusive.  */
	const struct extension_language_defn *extlang
	  = get_breakpoint_cond_ext_lang (&b, EXT_LANG_NONE);

	if (extlang != NULL)
	  {
	    error (_("Only one stop condition allowed.  There is currently"
		     " a %s stop condition defined for this breakpoint."),
		   ext_lang_capitalized_name (extlang));
	  }
	set_breakpoint_condition (&b, exp, from_tty, force);

	if (is_breakpoint (&b))
	  update_global_location_list (UGLL_MAY_INSERT);

	return;
      }

  error (_("No breakpoint number %d."), bpnum);
}

/* The options for the "condition" command.  */

struct condition_command_opts
{
  /* For "-force".  */
  bool force_condition = false;
};

static const gdb::option::option_def condition_command_option_defs[] = {

  gdb::option::flag_option_def<condition_command_opts> {
    "force",
    [] (condition_command_opts *opts) { return &opts->force_condition; },
    N_("Set the condition even if it is invalid for all current locations."),
  },

};

/* Create an option_def_group for the "condition" options, with
   CC_OPTS as context.  */

static inline gdb::option::option_def_group
make_condition_command_options_def_group (condition_command_opts *cc_opts)
{
  return {{condition_command_option_defs}, cc_opts};
}

/* Completion for the "condition" command.  */

static void
condition_completer (struct cmd_list_element *cmd,
		     completion_tracker &tracker,
		     const char *text, const char * /*word*/)
{
  bool has_no_arguments = (*text == '\0');
  condition_command_opts cc_opts;
  const auto group = make_condition_command_options_def_group (&cc_opts);
  if (gdb::option::complete_options
      (tracker, &text, gdb::option::PROCESS_OPTIONS_UNKNOWN_IS_ERROR, group))
    return;

  text = skip_spaces (text);
  const char *space = skip_to_space (text);
  if (*space == '\0')
    {
      int len;

      if (text[0] == '$')
	{
	  tracker.advance_custom_word_point_by (1);
	  /* We don't support completion of history indices.  */
	  if (!isdigit (text[1]))
	    complete_internalvar (tracker, &text[1]);
	  return;
	}

      /* Suggest the "-force" flag if no arguments are given.  If
	 arguments were passed, they either already include the flag,
	 or we are beyond the point of suggesting it because it's
	 positionally the first argument.  */
      if (has_no_arguments)
	gdb::option::complete_on_all_options (tracker, group);

      /* We're completing the breakpoint number.  */
      len = strlen (text);

      for (breakpoint &b : all_breakpoints ())
	{
	  char number[50];

	  xsnprintf (number, sizeof (number), "%d", b.number);

	  if (strncmp (number, text, len) == 0)
	    tracker.add_completion (make_unique_xstrdup (number));
	}

      return;
    }

  /* We're completing the expression part.  Skip the breakpoint num.  */
  const char *exp_start = skip_spaces (space);
  tracker.advance_custom_word_point_by (exp_start - text);
  text = exp_start;
  const char *word = advance_to_expression_complete_word_point (tracker, text);
  expression_completer (cmd, tracker, text, word);
}

/* condition N EXP -- set break condition of breakpoint N to EXP.  */

static void
condition_command (const char *arg, int from_tty)
{
  const char *p;
  int bnum;

  if (arg == 0)
    error_no_arg (_("breakpoint number"));

  p = arg;

  /* Check if the "-force" flag was passed.  */
  condition_command_opts cc_opts;
  const auto group = make_condition_command_options_def_group (&cc_opts);
  gdb::option::process_options
    (&p, gdb::option::PROCESS_OPTIONS_UNKNOWN_IS_ERROR, group);

  bnum = get_number (&p);
  if (bnum == 0)
    error (_("Bad breakpoint argument: '%s'"), arg);

  set_breakpoint_condition (bnum, p, from_tty, cc_opts.force_condition);
}

/* Check that COMMAND do not contain commands that are suitable
   only for tracepoints and not suitable for ordinary breakpoints.
   Throw if any such commands is found.  */

static void
check_no_tracepoint_commands (struct command_line *commands)
{
  struct command_line *c;

  for (c = commands; c; c = c->next)
    {
      if (c->control_type == while_stepping_control)
	error (_("The 'while-stepping' command can "
		 "only be used for tracepoints"));

      check_no_tracepoint_commands (c->body_list_0.get ());
      check_no_tracepoint_commands (c->body_list_1.get ());

      /* Not that command parsing removes leading whitespace and comment
	 lines and also empty lines.  So, we only need to check for
	 command directly.  */
      if (strstr (c->line, "collect ") == c->line)
	error (_("The 'collect' command can only be used for tracepoints"));

      if (strstr (c->line, "teval ") == c->line)
	error (_("The 'teval' command can only be used for tracepoints"));
    }
}

struct longjmp_breakpoint : public momentary_breakpoint
{
  using momentary_breakpoint::momentary_breakpoint;

  ~longjmp_breakpoint () override;
};

/* Encapsulate tests for different types of tracepoints.  */

static bool
is_tracepoint_type (bptype type)
{
  return (type == bp_tracepoint
	  || type == bp_fast_tracepoint
	  || type == bp_static_tracepoint
	  || type == bp_static_marker_tracepoint);
}

/* See breakpoint.h.  */

bool
is_tracepoint (const struct breakpoint *b)
{
  return is_tracepoint_type (b->type);
}

/* Factory function to create an appropriate instance of breakpoint given
   TYPE.  */

template<typename... Arg>
static std::unique_ptr<code_breakpoint>
new_breakpoint_from_type (struct gdbarch *gdbarch, bptype type,
			  Arg&&... args)
{
  code_breakpoint *b;

  switch (type)
    {
    case bp_breakpoint:
    case bp_hardware_breakpoint:
      b = new ordinary_breakpoint (gdbarch, type,
				   std::forward<Arg> (args)...);
      break;

    case bp_fast_tracepoint:
    case bp_static_tracepoint:
    case bp_tracepoint:
      b = new tracepoint (gdbarch, type,
			  std::forward<Arg> (args)...);
      break;

    case bp_static_marker_tracepoint:
      b = new static_marker_tracepoint (gdbarch, type,
					std::forward<Arg> (args)...);
      break;

    case bp_dprintf:
      b = new dprintf_breakpoint (gdbarch, type,
				  std::forward<Arg> (args)...);
      break;

    default:
      gdb_assert_not_reached ("invalid type");
    }

  return std::unique_ptr<code_breakpoint> (b);
}

/* A helper function that validates that COMMANDS are valid for a
   breakpoint.  This function will throw an exception if a problem is
   found.  */

static void
validate_commands_for_breakpoint (struct breakpoint *b,
				  struct command_line *commands)
{
  if (is_tracepoint (b))
    {
      tracepoint *t = gdb::checked_static_cast<tracepoint *> (b);
      struct command_line *c;
      struct command_line *while_stepping = 0;

      /* Reset the while-stepping step count.  The previous commands
	 might have included a while-stepping action, while the new
	 ones might not.  */
      t->step_count = 0;

      /* We need to verify that each top-level element of commands is
	 valid for tracepoints, that there's at most one
	 while-stepping element, and that the while-stepping's body
	 has valid tracing commands excluding nested while-stepping.
	 We also need to validate the tracepoint action line in the
	 context of the tracepoint --- validate_actionline actually
	 has side effects, like setting the tracepoint's
	 while-stepping STEP_COUNT, in addition to checking if the
	 collect/teval actions parse and make sense in the
	 tracepoint's context.  */
      for (c = commands; c; c = c->next)
	{
	  if (c->control_type == while_stepping_control)
	    {
	      if (b->type == bp_fast_tracepoint)
		error (_("The 'while-stepping' command "
			 "cannot be used for fast tracepoint"));
	      else if (b->type == bp_static_tracepoint
		       || b->type == bp_static_marker_tracepoint)
		error (_("The 'while-stepping' command "
			 "cannot be used for static tracepoint"));

	      if (while_stepping)
		error (_("The 'while-stepping' command "
			 "can be used only once"));
	      else
		while_stepping = c;
	    }

	  validate_actionline (c->line, t);
	}
      if (while_stepping)
	{
	  struct command_line *c2;

	  gdb_assert (while_stepping->body_list_1 == nullptr);
	  c2 = while_stepping->body_list_0.get ();
	  for (; c2; c2 = c2->next)
	    {
	      if (c2->control_type == while_stepping_control)
		error (_("The 'while-stepping' command cannot be nested"));
	    }
	}
    }
  else
    {
      check_no_tracepoint_commands (commands);
    }
}

/* Return a vector of all the static tracepoints set at ADDR.  The
   caller is responsible for releasing the vector.  */

std::vector<breakpoint *>
static_tracepoints_here (CORE_ADDR addr)
{
  std::vector<breakpoint *> found;

  for (breakpoint &b : all_breakpoints ())
    if (b.type == bp_static_tracepoint
	|| b.type == bp_static_marker_tracepoint)
      {
	for (bp_location &loc : b.locations ())
	  if (loc.address == addr)
	    found.push_back (&b);
      }

  return found;
}

/* Set the command list of B to COMMANDS.  If breakpoint is tracepoint,
   validate that only allowed commands are included.  */

void
breakpoint_set_commands (struct breakpoint *b, 
			 counted_command_line &&commands)
{
  validate_commands_for_breakpoint (b, commands.get ());

  b->commands = std::move (commands);
  notify_breakpoint_modified (b);
}

/* Set the internal `silent' flag on the breakpoint.  Note that this
   is not the same as the "silent" that may appear in the breakpoint's
   commands.  */

void
breakpoint_set_silent (struct breakpoint *b, int silent)
{
  int old_silent = b->silent;

  b->silent = silent;
  if (old_silent != silent)
    notify_breakpoint_modified (b);
}

/* See breakpoint.h.  */

void
breakpoint_set_thread (struct breakpoint *b, int thread)
{
  /* THREAD should be -1, meaning no thread restriction, or it should be a
     valid global thread-id, which are greater than zero.  */
  gdb_assert (thread == -1 || thread > 0);

  /* It is not valid to set a thread restriction for a breakpoint that
     already has task or inferior restriction.  */
  gdb_assert (thread == -1 || (b->task == -1 && b->inferior == -1));

  int old_thread = b->thread;
  b->thread = thread;
  if (old_thread != thread)
    notify_breakpoint_modified (b);
}

/* See breakpoint.h.  */

void
breakpoint_set_inferior (struct breakpoint *b, int inferior)
{
  /* INFERIOR should be -1, meaning no inferior restriction, or it should
     be a valid inferior number, which are greater than zero.  */
  gdb_assert (inferior == -1 || inferior > 0);

  /* It is not valid to set an inferior restriction for a breakpoint that
     already has a task or thread restriction.  */
  gdb_assert (inferior == -1 || (b->task == -1 && b->thread == -1));

  int old_inferior = b->inferior;
  b->inferior = inferior;
  if (old_inferior != inferior)
    notify_breakpoint_modified (b);
}

/* See breakpoint.h.  */

void
breakpoint_set_task (struct breakpoint *b, int task)
{
  /* TASK should be -1, meaning no task restriction, or it should be a
     valid task-id, which are greater than zero.  */
  gdb_assert (task == -1 || task > 0);

  /* It is not valid to set a task restriction for a breakpoint that
     already has a thread or inferior restriction.  */
  gdb_assert (task == -1 || (b->thread == -1 && b->inferior == -1));

  int old_task = b->task;
  b->task = task;
  if (old_task != task)
    notify_breakpoint_modified (b);
}

static void
commands_command_1 (const char *arg, int from_tty,
		    struct command_line *control)
{
  counted_command_line cmd;
  /* cmd_read will be true once we have read cmd.  Note that cmd might still be
     NULL after the call to read_command_lines if the user provides an empty
     list of command by just typing "end".  */
  bool cmd_read = false;

  std::string new_arg;

  if (arg == NULL || !*arg)
    {
      /* Argument not explicitly given.  Synthesize it.  */
      if (breakpoint_count - prev_breakpoint_count > 1)
	new_arg = string_printf ("%d-%d", prev_breakpoint_count + 1,
				 breakpoint_count);
      else if (breakpoint_count > 0)
	new_arg = string_printf ("%d", breakpoint_count);
    }
  else
    {
      /* Create a copy of ARG.  This is needed because the "commands"
	 command may be coming from a script.  In that case, the read
	 line buffer is going to be overwritten in the lambda of
	 'map_breakpoint_numbers' below when reading the next line
	 before we are are done parsing the breakpoint numbers.  */
      new_arg = arg;
    }
  arg = new_arg.c_str ();

  map_breakpoint_numbers
    (arg, [&] (breakpoint *b)
     {
       if (!cmd_read)
	 {
	   gdb_assert (cmd == NULL);
	   if (control != NULL)
	     cmd = control->body_list_0;
	   else
	     {
	       std::string str
		 = string_printf (_("Type commands for breakpoint(s) "
				    "%s, one per line."),
				  arg);

	       auto do_validate = [=] (const char *line)
				  {
				    tracepoint *t
				      = gdb::checked_static_cast<tracepoint *> (b);
				    validate_actionline (line, t);
				  };
	       gdb::function_view<void (const char *)> validator;
	       if (is_tracepoint (b))
		 validator = do_validate;

	       cmd = read_command_lines (str.c_str (), from_tty, 1, validator);
	     }
	   cmd_read = true;
	 }

       /* If a breakpoint was on the list more than once, we don't need to
	  do anything.  */
       if (b->commands != cmd)
	 {
	   validate_commands_for_breakpoint (b, cmd.get ());
	   b->commands = cmd;
	   notify_breakpoint_modified (b);
	 }
     });
}

static void
commands_command (const char *arg, int from_tty)
{
  commands_command_1 (arg, from_tty, NULL);
}

/* Like commands_command, but instead of reading the commands from
   input stream, takes them from an already parsed command structure.

   This is used by cli-script.c to DTRT with breakpoint commands
   that are part of if and while bodies.  */
enum command_control_type
commands_from_control_command (const char *arg, struct command_line *cmd)
{
  commands_command_1 (arg, 0, cmd);
  return simple_control;
}

/* Return true if BL->TARGET_INFO contains valid information.  */

static bool
bp_location_has_shadow (struct bp_location *bl)
{
  if (bl->loc_type != bp_loc_software_breakpoint)
    return false;
  if (!bl->inserted)
    return false;
  if (bl->target_info.shadow_len == 0)
    /* BL isn't valid, or doesn't shadow memory.  */
    return false;
  return true;
}

/* Update BUF, which is LEN bytes read from the target address
   MEMADDR, by replacing a memory breakpoint with its shadowed
   contents.

   If READBUF is not NULL, this buffer must not overlap with the of
   the breakpoint location's shadow_contents buffer.  Otherwise, a
   failed assertion internal error will be raised.  */

static void
one_breakpoint_xfer_memory (gdb_byte *readbuf, gdb_byte *writebuf,
			    const gdb_byte *writebuf_org,
			    ULONGEST memaddr, LONGEST len,
			    struct bp_target_info *target_info,
			    struct gdbarch *gdbarch)
{
  /* Now do full processing of the found relevant range of elements.  */
  CORE_ADDR bp_addr = 0;
  int bp_size = 0;
  int bptoffset = 0;

  if (!breakpoint_address_match (target_info->placed_address_space, 0,
				 current_program_space->aspace.get (), 0))
    {
      /* The breakpoint is inserted in a different address space.  */
      return;
    }

  /* Addresses and length of the part of the breakpoint that
     we need to copy.  */
  bp_addr = target_info->placed_address;
  bp_size = target_info->shadow_len;

  if (bp_addr + bp_size <= memaddr)
    {
      /* The breakpoint is entirely before the chunk of memory we are
	 reading.  */
      return;
    }

  if (bp_addr >= memaddr + len)
    {
      /* The breakpoint is entirely after the chunk of memory we are
	 reading.  */
      return;
    }

  /* Offset within shadow_contents.  */
  if (bp_addr < memaddr)
    {
      /* Only copy the second part of the breakpoint.  */
      bp_size -= memaddr - bp_addr;
      bptoffset = memaddr - bp_addr;
      bp_addr = memaddr;
    }

  if (bp_addr + bp_size > memaddr + len)
    {
      /* Only copy the first part of the breakpoint.  */
      bp_size -= (bp_addr + bp_size) - (memaddr + len);
    }

  if (readbuf != NULL)
    {
      /* Verify that the readbuf buffer does not overlap with the
	 shadow_contents buffer.  */
      gdb_assert (target_info->shadow_contents >= readbuf + len
		  || readbuf >= (target_info->shadow_contents
				 + target_info->shadow_len));

      /* Update the read buffer with this inserted breakpoint's
	 shadow.  */
      memcpy (readbuf + bp_addr - memaddr,
	      target_info->shadow_contents + bptoffset, bp_size);
    }
  else
    {
      const unsigned char *bp;
      CORE_ADDR addr = target_info->reqstd_address;
      int placed_size;

      /* Update the shadow with what we want to write to memory.  */
      memcpy (target_info->shadow_contents + bptoffset,
	      writebuf_org + bp_addr - memaddr, bp_size);

      /* Determine appropriate breakpoint contents and size for this
	 address.  */
      bp = gdbarch_breakpoint_from_pc (gdbarch, &addr, &placed_size);

      /* Update the final write buffer with this inserted
	 breakpoint's INSN.  */
      memcpy (writebuf + bp_addr - memaddr, bp + bptoffset, bp_size);
    }
}

/* Update BUF, which is LEN bytes read from the target address MEMADDR,
   by replacing any memory breakpoints with their shadowed contents.

   If READBUF is not NULL, this buffer must not overlap with any of
   the breakpoint location's shadow_contents buffers.  Otherwise,
   a failed assertion internal error will be raised.

   The range of shadowed area by each bp_location is:
     bl->address - bp_locations_placed_address_before_address_max
     up to bl->address + bp_locations_shadow_len_after_address_max
   The range we were requested to resolve shadows for is:
     memaddr ... memaddr + len
   Thus the safe cutoff boundaries for performance optimization are
     memaddr + len <= (bl->address
		       - bp_locations_placed_address_before_address_max)
   and:
     bl->address + bp_locations_shadow_len_after_address_max <= memaddr  */

void
breakpoint_xfer_memory (gdb_byte *readbuf, gdb_byte *writebuf,
			const gdb_byte *writebuf_org,
			ULONGEST memaddr, LONGEST len)
{
  /* Left boundary, right boundary and median element of our binary
     search.  */
  unsigned bc_l, bc_r, bc;

  /* Find BC_L which is a leftmost element which may affect BUF
     content.  It is safe to report lower value but a failure to
     report higher one.  */

  bc_l = 0;
  bc_r = bp_locations.size ();
  while (bc_l + 1 < bc_r)
    {
      struct bp_location *bl;

      bc = (bc_l + bc_r) / 2;
      bl = bp_locations[bc];

      /* Check first BL->ADDRESS will not overflow due to the added
	 constant.  Then advance the left boundary only if we are sure
	 the BC element can in no way affect the BUF content (MEMADDR
	 to MEMADDR + LEN range).

	 Use the BP_LOCATIONS_SHADOW_LEN_AFTER_ADDRESS_MAX safety
	 offset so that we cannot miss a breakpoint with its shadow
	 range tail still reaching MEMADDR.  */

      if ((bl->address + bp_locations_shadow_len_after_address_max
	   >= bl->address)
	  && (bl->address + bp_locations_shadow_len_after_address_max
	      <= memaddr))
	bc_l = bc;
      else
	bc_r = bc;
    }

  /* Due to the binary search above, we need to make sure we pick the
     first location that's at BC_L's address.  E.g., if there are
     multiple locations at the same address, BC_L may end up pointing
     at a duplicate location, and miss the "master"/"inserted"
     location.  Say, given locations L1, L2 and L3 at addresses A and
     B:

      L1@A, L2@A, L3@B, ...

     BC_L could end up pointing at location L2, while the "master"
     location could be L1.  Since the `loc->inserted' flag is only set
     on "master" locations, we'd forget to restore the shadow of L1
     and L2.  */
  while (bc_l > 0
	 && bp_locations[bc_l]->address == bp_locations[bc_l - 1]->address)
    bc_l--;

  /* Now do full processing of the found relevant range of elements.  */

  for (bc = bc_l; bc < bp_locations.size (); bc++)
    {
      struct bp_location *bl = bp_locations[bc];

      /* bp_location array has BL->OWNER always non-NULL.  */
      if (bl->owner->type == bp_none)
	warning (_("reading through apparently deleted breakpoint #%d?"),
		 bl->owner->number);

      /* Performance optimization: any further element can no longer affect BUF
	 content.  */

      if (bl->address >= bp_locations_placed_address_before_address_max
	  && (memaddr + len
	      <= (bl->address
		  - bp_locations_placed_address_before_address_max)))
	break;

      if (!bp_location_has_shadow (bl))
	continue;

      one_breakpoint_xfer_memory (readbuf, writebuf, writebuf_org,
				  memaddr, len, &bl->target_info, bl->gdbarch);
    }
}

/* See breakpoint.h.  */

bool
is_breakpoint (const struct breakpoint *bpt)
{
  return (bpt->type == bp_breakpoint
	  || bpt->type == bp_hardware_breakpoint
	  || bpt->type == bp_dprintf);
}

/* Return true if BPT is of any hardware watchpoint kind.  */

static bool
is_hardware_watchpoint (const struct breakpoint *bpt)
{
  return (bpt->type == bp_hardware_watchpoint
	  || bpt->type == bp_read_watchpoint
	  || bpt->type == bp_access_watchpoint);
}

/* See breakpoint.h.  */

bool
is_watchpoint (const struct breakpoint *bpt)
{
  return (is_hardware_watchpoint (bpt)
	  || bpt->type == bp_watchpoint);
}

/* Returns true if the current thread and its running state are safe
   to evaluate or update watchpoint B.  Watchpoints on local
   expressions need to be evaluated in the context of the thread that
   was current when the watchpoint was created, and, that thread needs
   to be stopped to be able to select the correct frame context.
   Watchpoints on global expressions can be evaluated on any thread,
   and in any state.  It is presently left to the target allowing
   memory accesses when threads are running.  */

static bool
watchpoint_in_thread_scope (struct watchpoint *b)
{
  return (b->pspace == current_program_space
	  && (b->watchpoint_thread == null_ptid
	      || (inferior_ptid == b->watchpoint_thread
		  && !inferior_thread ()->executing ())));
}

/* Set watchpoint B to disp_del_at_next_stop, even including its possible
   associated bp_watchpoint_scope breakpoint.  */

static void
watchpoint_del_at_next_stop (struct watchpoint *w)
{
  if (w->related_breakpoint != w)
    {
      gdb_assert (w->related_breakpoint->type == bp_watchpoint_scope);
      gdb_assert (w->related_breakpoint->related_breakpoint == w);
      w->related_breakpoint->disposition = disp_del_at_next_stop;
      w->related_breakpoint->related_breakpoint = w->related_breakpoint;
      w->related_breakpoint = w;
    }
  w->disposition = disp_del_at_next_stop;
  disable_breakpoint (w);
}

/* Extract a bitfield value from value VAL using the bit parameters contained in
   watchpoint W.  */

static struct value *
extract_bitfield_from_watchpoint_value (struct watchpoint *w, struct value *val)
{
  struct value *bit_val;

  if (val == NULL)
    return NULL;

  bit_val = value::allocate (val->type ());

  val->unpack_bitfield (bit_val,
			w->val_bitpos,
			w->val_bitsize,
			val->contents_for_printing ().data (),
			val->offset ());

  return bit_val;
}

/* Allocate a dummy location and add it to B.  This is required
   because bpstat_stop_status requires a location to be able to report
   stops.  */

static void
add_dummy_location (struct breakpoint *b,
		    struct program_space *pspace)
{
  gdb_assert (!b->has_locations ());

  bp_location *loc = new bp_location (b, bp_loc_other);
  loc->pspace = pspace;
  b->add_location (*loc);
}

/* Assuming that B is a watchpoint:
   - Reparse watchpoint expression, if REPARSE is true
   - Evaluate expression and store the result in B->val
   - Evaluate the condition if there is one, and store the result
     in b->loc->cond.
   - Update the list of values that must be watched in B->loc.

   If the watchpoint disposition is disp_del_at_next_stop, then do
   nothing.  If this is local watchpoint that is out of scope, delete
   it.

   Even with `set breakpoint always-inserted on' the watchpoints are
   removed + inserted on each stop here.  Normal breakpoints must
   never be removed because they might be missed by a running thread
   when debugging in non-stop mode.  On the other hand, hardware
   watchpoints (is_hardware_watchpoint; processed here) are specific
   to each LWP since they are stored in each LWP's hardware debug
   registers.  Therefore, such LWP must be stopped first in order to
   be able to modify its hardware watchpoints.

   Hardware watchpoints must be reset exactly once after being
   presented to the user.  It cannot be done sooner, because it would
   reset the data used to present the watchpoint hit to the user.  And
   it must not be done later because it could display the same single
   watchpoint hit during multiple GDB stops.  Note that the latter is
   relevant only to the hardware watchpoint types bp_read_watchpoint
   and bp_access_watchpoint.  False hit by bp_hardware_watchpoint is
   not user-visible - its hit is suppressed if the memory content has
   not changed.

   The following constraints influence the location where we can reset
   hardware watchpoints:

   * target_stopped_by_watchpoint and target_stopped_data_address are
     called several times when GDB stops.

   [linux] 
   * Multiple hardware watchpoints can be hit at the same time,
     causing GDB to stop.  GDB only presents one hardware watchpoint
     hit at a time as the reason for stopping, and all the other hits
     are presented later, one after the other, each time the user
     requests the execution to be resumed.  Execution is not resumed
     for the threads still having pending hit event stored in
     LWP_INFO->STATUS.  While the watchpoint is already removed from
     the inferior on the first stop the thread hit event is kept being
     reported from its cached value by linux_nat_stopped_data_address
     until the real thread resume happens after the watchpoint gets
     presented and thus its LWP_INFO->STATUS gets reset.

   Therefore the hardware watchpoint hit can get safely reset on the
   watchpoint removal from inferior.  */

static void
update_watchpoint (struct watchpoint *b, bool reparse)
{
  bool within_current_scope;

  /* If this is a local watchpoint, we only want to check if the
     watchpoint frame is in scope if the current thread is the thread
     that was used to create the watchpoint.  */
  if (!watchpoint_in_thread_scope (b))
    return;

  if (b->disposition == disp_del_at_next_stop)
    return;

  std::optional<scoped_restore_selected_frame> restore_frame;

  /* Determine if the watchpoint is within scope.  */
  if (b->exp_valid_block == NULL)
    within_current_scope = true;
  else
    {
      frame_info_ptr fi = get_current_frame ();
      struct gdbarch *frame_arch = get_frame_arch (fi);
      CORE_ADDR frame_pc = get_frame_pc (fi);

      /* If we're at a point where the stack has been destroyed
	 (e.g. in a function epilogue), unwinding may not work
	 properly. Do not attempt to recreate locations at this
	 point.  See similar comments in watchpoint_check.  */
      if (gdbarch_stack_frame_destroyed_p (frame_arch, frame_pc))
	return;

      /* Save the current frame's ID so we can restore it after
	 evaluating the watchpoint expression on its own frame.  */
      /* FIXME drow/2003-09-09: It would be nice if evaluate_expression
	 took a frame parameter, so that we didn't have to change the
	 selected frame.  */
      restore_frame.emplace ();

      fi = frame_find_by_id (b->watchpoint_frame);
      within_current_scope = (fi != NULL);
      if (within_current_scope)
	select_frame (fi);
    }

  /* We don't free locations.  They are stored in the bp_location array
     and update_global_location_list will eventually delete them and
     remove breakpoints if needed.  */
  b->clear_locations ();

  if (within_current_scope && reparse)
    {
      const char *s;

      b->exp.reset ();
      s = (b->exp_string_reparse
	   ? b->exp_string_reparse.get ()
	   : b->exp_string.get ());
      b->exp = parse_exp_1 (&s, 0, b->exp_valid_block, 0);
      /* If the meaning of expression itself changed, the old value is
	 no longer relevant.  We don't want to report a watchpoint hit
	 to the user when the old value and the new value may actually
	 be completely different objects.  */
      b->val = NULL;
      b->val_valid = false;

      /* Note that unlike with breakpoints, the watchpoint's condition
	 expression is stored in the breakpoint object, not in the
	 locations (re)created below.  */
      if (b->cond_string != NULL)
	{
	  b->cond_exp.reset ();

	  s = b->cond_string.get ();
	  b->cond_exp = parse_exp_1 (&s, 0, b->cond_exp_valid_block, 0);
	}
    }

  /* If we failed to parse the expression, for example because
     it refers to a global variable in a not-yet-loaded shared library,
     don't try to insert watchpoint.  We don't automatically delete
     such watchpoint, though, since failure to parse expression
     is different from out-of-scope watchpoint.  */
  if (!target_has_execution ())
    {
      /* Without execution, memory can't change.  No use to try and
	 set watchpoint locations.  The watchpoint will be reset when
	 the target gains execution, through breakpoint_re_set.  */
      if (!can_use_hw_watchpoints)
	{
	  if (b->works_in_software_mode ())
	    b->type = bp_watchpoint;
	  else
	    error (_("Can't set read/access watchpoint when "
		     "hardware watchpoints are disabled."));
	}
    }
  else if (within_current_scope && b->exp)
    {
      std::vector<value_ref_ptr> val_chain;
      struct value *v, *result;
      struct program_space *frame_pspace;

      fetch_subexp_value (b->exp.get (), b->exp->op.get (), &v, &result,
			  &val_chain, false);

      /* Avoid setting b->val if it's already set.  The meaning of
	 b->val is 'the last value' user saw, and we should update
	 it only if we reported that last value to user.  As it
	 happens, the code that reports it updates b->val directly.
	 We don't keep track of the memory value for masked
	 watchpoints.  */
      if (!b->val_valid && !is_masked_watchpoint (b))
	{
	  if (b->val_bitsize != 0)
	    v = extract_bitfield_from_watchpoint_value (b, v);
	  b->val = release_value (v);
	  b->val_valid = true;
	}

      frame_pspace = get_frame_program_space (get_selected_frame (NULL));

      /* Look at each value on the value chain.  */
      gdb_assert (!val_chain.empty ());
      for (const value_ref_ptr &iter : val_chain)
	{
	  v = iter.get ();

	  /* If it's a memory location, and GDB actually needed
	     its contents to evaluate the expression, then we
	     must watch it.  If the first value returned is
	     still lazy, that means an error occurred reading it;
	     watch it anyway in case it becomes readable.  */
	  if (v->lval () == lval_memory
	      && (v == val_chain[0] || ! v->lazy ()))
	    {
	      struct type *vtype = check_typedef (v->type ());

	      /* We only watch structs and arrays if user asked
		 for it explicitly, never if they just happen to
		 appear in the middle of some value chain.  */
	      if (v == result
		  || (vtype->code () != TYPE_CODE_STRUCT
		      && vtype->code () != TYPE_CODE_ARRAY))
		{
		  CORE_ADDR addr;
		  enum target_hw_bp_type type;
		  int bitpos = 0, bitsize = 0;

		  if (v->bitsize () != 0)
		    {
		      /* Extract the bit parameters out from the bitfield
			 sub-expression.  */
		      bitpos = v->bitpos ();
		      bitsize = v->bitsize ();
		    }
		  else if (v == result && b->val_bitsize != 0)
		    {
		     /* If VAL_BITSIZE != 0 then RESULT is actually a bitfield
			lvalue whose bit parameters are saved in the fields
			VAL_BITPOS and VAL_BITSIZE.  */
		      bitpos = b->val_bitpos;
		      bitsize = b->val_bitsize;
		    }

		  addr = v->address ();
		  if (bitsize != 0)
		    {
		      /* Skip the bytes that don't contain the bitfield.  */
		      addr += bitpos / 8;
		    }

		  type = hw_write;
		  if (b->type == bp_read_watchpoint)
		    type = hw_read;
		  else if (b->type == bp_access_watchpoint)
		    type = hw_access;

		  bp_location *loc = b->allocate_location ();
		  loc->gdbarch = v->type ()->arch ();
		  loc->pspace = frame_pspace;
		  loc->address
		    = gdbarch_remove_non_address_bits (loc->gdbarch, addr);
		  b->add_location (*loc);

		  if (bitsize != 0)
		    {
		      /* Just cover the bytes that make up the bitfield.  */
		      loc->length = ((bitpos % 8) + bitsize + 7) / 8;
		    }
		  else
		    loc->length = v->type ()->length ();

		  loc->watchpoint_type = type;
		}
	    }
	}

      /* Helper function to bundle possibly emitting a warning along with
	 changing the type of B to bp_watchpoint.  */
      auto change_type_to_bp_watchpoint = [] (breakpoint *bp)
      {
	/* Only warn for breakpoints that have been assigned a +ve number,
	   anything else is either an internal watchpoint (which we don't
	   currently create) or has not yet been finalized, in which case
	   this change of type will be occurring before the user is told
	   the type of this watchpoint.  */
	if (bp->type == bp_hardware_watchpoint && bp->number > 0)
	  warning (_("watchpoint %d downgraded to software watchpoint"),
		   bp->number);
	bp->type = bp_watchpoint;
      };

      /* Change the type of breakpoint between hardware assisted or
	 an ordinary watchpoint depending on the hardware support and
	 free hardware slots.  Recheck the number of free hardware slots
	 as the value chain may have changed.  */
	{
	  int reg_cnt;
	  enum bp_loc_type loc_type;

	  reg_cnt = can_use_hardware_watchpoint (val_chain);

	  if (reg_cnt)
	    {
	      int i, target_resources_ok, other_type_used;
	      enum bptype type;

	      /* Use an exact watchpoint when there's only one memory region to be
		 watched, and only one debug register is needed to watch it.  */
	      b->exact = target_exact_watchpoints && reg_cnt == 1;

	      /* We need to determine how many resources are already
		 used for all other hardware watchpoints plus this one
		 to see if we still have enough resources to also fit
		 this watchpoint in as well.  */

	      /* If this is a software watchpoint, we try to turn it
		 to a hardware one -- count resources as if B was of
		 hardware watchpoint type.  */
	      type = b->type;
	      if (type == bp_watchpoint)
		type = bp_hardware_watchpoint;

	      /* This watchpoint may or may not have been placed on
		 the list yet at this point (it won't be in the list
		 if we're trying to create it for the first time,
		 through watch_command), so always account for it
		 manually.  */

	      /* Count resources used by all watchpoints except B.  */
	      i = hw_watchpoint_used_count_others (b, type, &other_type_used);

	      /* Add in the resources needed for B.  */
	      i += hw_watchpoint_use_count (b);

	      target_resources_ok
		= target_can_use_hardware_watchpoint (type, i, other_type_used);
	      if (target_resources_ok <= 0)
		{
		  bool sw_mode = b->works_in_software_mode ();

		  if (target_resources_ok == 0 && !sw_mode)
		    error (_("Target does not support this type of "
			     "hardware watchpoint."));
		  else if (target_resources_ok < 0 && !sw_mode)
		    error (_("There are not enough available hardware "
			     "resources for this watchpoint."));

		  /* Downgrade to software watchpoint.  */
		  change_type_to_bp_watchpoint (b);
		}
	      else
		{
		  /* If this was a software watchpoint, we've just
		     found we have enough resources to turn it to a
		     hardware watchpoint.  Otherwise, this is a
		     nop.  */
		  b->type = type;
		}
	    }
	  else if (!b->works_in_software_mode ())
	    {
	      if (!can_use_hw_watchpoints)
		error (_("Can't set read/access watchpoint when "
			 "hardware watchpoints are disabled."));
	      else
		error (_("Expression cannot be implemented with "
			 "read/access watchpoint."));
	    }
	  else
	    change_type_to_bp_watchpoint (b);

	  loc_type = (b->type == bp_watchpoint? bp_loc_software_watchpoint
		      : bp_loc_hardware_watchpoint);

	  for (bp_location &bl : b->locations ())
	    bl.loc_type = loc_type;
	}

      /* If a software watchpoint is not watching any memory, then the
	 above left it without any location set up.  But,
	 bpstat_stop_status requires a location to be able to report
	 stops, so make sure there's at least a dummy one.  */
      if (b->type == bp_watchpoint && !b->has_locations ())
	add_dummy_location (b, frame_pspace);
    }
  else if (!within_current_scope)
    {
      gdb_printf (_("\
Watchpoint %d deleted because the program has left the block\n\
in which its expression is valid.\n"),
		  b->number);
      watchpoint_del_at_next_stop (b);
    }
}

/* Returns true iff breakpoint location should be
   inserted in the inferior.  We don't differentiate the type of BL's owner
   (breakpoint vs. tracepoint), although insert_location in tracepoint's
   breakpoint_ops is not defined, because in insert_bp_location,
   tracepoint's insert_location will not be called.  */

static bool
should_be_inserted (struct bp_location *bl)
{
  if (bl->owner == NULL || !breakpoint_enabled (bl->owner))
    return false;

  if (bl->owner->disposition == disp_del_at_next_stop)
    return false;

  if (!bl->enabled || bl->disabled_by_cond
      || bl->shlib_disabled || bl->duplicate)
    return false;

  if (user_breakpoint_p (bl->owner) && bl->pspace->executing_startup)
    return false;

  /* This is set for example, when we're attached to the parent of a
     vfork, and have detached from the child.  The child is running
     free, and we expect it to do an exec or exit, at which point the
     OS makes the parent schedulable again (and the target reports
     that the vfork is done).  Until the child is done with the shared
     memory region, do not insert breakpoints in the parent, otherwise
     the child could still trip on the parent's breakpoints.  Since
     the parent is blocked anyway, it won't miss any breakpoint.  */
  if (bl->pspace->breakpoints_not_allowed)
    return false;

  /* Don't insert a breakpoint if we're trying to step past its
     location, except if the breakpoint is a single-step breakpoint,
     and the breakpoint's thread is the thread which is stepping past
     a breakpoint.  */
  if ((bl->loc_type == bp_loc_software_breakpoint
       || bl->loc_type == bp_loc_hardware_breakpoint)
      && stepping_past_instruction_at (bl->pspace->aspace.get (),
				       bl->address)
      /* The single-step breakpoint may be inserted at the location
	 we're trying to step if the instruction branches to itself.
	 However, the instruction won't be executed at all and it may
	 break the semantics of the instruction, for example, the
	 instruction is a conditional branch or updates some flags.
	 We can't fix it unless GDB is able to emulate the instruction
	 or switch to displaced stepping.  */
      && !(bl->owner->type == bp_single_step
	   && thread_is_stepping_over_breakpoint (bl->owner->thread)))
    {
      infrun_debug_printf ("skipping breakpoint: stepping past insn at: %s",
			   paddress (bl->gdbarch, bl->address));
      return false;
    }

  /* Don't insert watchpoints if we're trying to step past the
     instruction that triggered one.  */
  if ((bl->loc_type == bp_loc_hardware_watchpoint)
      && stepping_past_nonsteppable_watchpoint ())
    {
      infrun_debug_printf ("stepping past non-steppable watchpoint. "
			   "skipping watchpoint at %s:%d",
			   paddress (bl->gdbarch, bl->address), bl->length);
      return false;
    }

  return true;
}

/* Same as should_be_inserted but does the check assuming
   that the location is not duplicated.  */

static bool
unduplicated_should_be_inserted (struct bp_location *bl)
{
  scoped_restore restore_bl_duplicate
    = make_scoped_restore (&bl->duplicate, 0);

  return should_be_inserted (bl);
}

/* Parses a conditional described by an expression COND into an
   agent expression bytecode suitable for evaluation
   by the bytecode interpreter.  Return NULL if there was
   any error during parsing.  */

static agent_expr_up
parse_cond_to_aexpr (CORE_ADDR scope, struct expression *cond)
{
  if (cond == NULL)
    return NULL;

  agent_expr_up aexpr;

  /* We don't want to stop processing, so catch any errors
     that may show up.  */
  try
    {
      aexpr = gen_eval_for_expr (scope, cond);
    }

  catch (const gdb_exception_error &ex)
    {
      /* If we got here, it means the condition could not be parsed to a valid
	 bytecode expression and thus can't be evaluated on the target's side.
	 It's no use iterating through the conditions.  */
    }

  /* We have a valid agent expression.  */
  return aexpr;
}

/* Based on location BL, create a list of breakpoint conditions to be
   passed on to the target.  If we have duplicated locations with different
   conditions, we will add such conditions to the list.  The idea is that the
   target will evaluate the list of conditions and will only notify GDB when
   one of them is true.  */

static void
build_target_condition_list (struct bp_location *bl)
{
  bool null_condition_or_parse_error = false;
  int modified = bl->needs_update;

  /* Release conditions left over from a previous insert.  */
  bl->target_info.conditions.clear ();

  /* This is only meaningful if the target is
     evaluating conditions and if the user has
     opted for condition evaluation on the target's
     side.  */
  if (gdb_evaluates_breakpoint_condition_p ()
      || !target_supports_evaluation_of_breakpoint_conditions ())
    return;

  auto loc_range = all_bp_locations_at_addr (bl->address);

  /* Do a first pass to check for locations with no assigned
     conditions or conditions that fail to parse to a valid agent
     expression bytecode.  If any of these happen, then it's no use to
     send conditions to the target since this location will always
     trigger and generate a response back to GDB.  Note we consider
     all locations at the same address irrespective of type, i.e.,
     even if the locations aren't considered duplicates (e.g.,
     software breakpoint and hardware breakpoint at the same
     address).  */
  for (bp_location *loc : loc_range)
    {
      if (is_breakpoint (loc->owner) && loc->pspace->num == bl->pspace->num)
	{
	  if (modified)
	    {
	      /* Re-parse the conditions since something changed.  In that
		 case we already freed the condition bytecodes (see
		 force_breakpoint_reinsertion).  We just
		 need to parse the condition to bytecodes again.  */
	      loc->cond_bytecode = parse_cond_to_aexpr (bl->address,
							loc->cond.get ());
	    }

	  /* If we have a NULL bytecode expression, it means something
	     went wrong or we have a null condition expression.  */
	  if (!loc->cond_bytecode)
	    {
	      null_condition_or_parse_error = true;
	      break;
	    }
	}
    }

  /* If any of these happened, it means we will have to evaluate the conditions
     for the location's address on gdb's side.  It is no use keeping bytecodes
     for all the other duplicate locations, thus we free all of them here.

     This is so we have a finer control over which locations' conditions are
     being evaluated by GDB or the remote stub.  */
  if (null_condition_or_parse_error)
    {
      for (bp_location *loc : loc_range)
	{
	  if (is_breakpoint (loc->owner) && loc->pspace->num == bl->pspace->num)
	    {
	      /* Only go as far as the first NULL bytecode is
		 located.  */
	      if (!loc->cond_bytecode)
		return;

	      loc->cond_bytecode.reset ();
	    }
	}
    }

  /* No NULL conditions or failed bytecode generation.  Build a
     condition list for this location's address.  If we have software
     and hardware locations at the same address, they aren't
     considered duplicates, but we still merge all the conditions
     anyway, as it's simpler, and doesn't really make a practical
     difference.  */
  for (bp_location *loc : loc_range)
    if (loc->cond
	&& is_breakpoint (loc->owner)
	&& loc->pspace->num == bl->pspace->num
	&& loc->owner->enable_state == bp_enabled
	&& loc->enabled
	&& !loc->disabled_by_cond)
      {
	/* Add the condition to the vector.  This will be used later
	   to send the conditions to the target.  */
	bl->target_info.conditions.push_back (loc->cond_bytecode.get ());
      }

  return;
}

/* Parses a command described by string CMD into an agent expression
   bytecode suitable for evaluation by the bytecode interpreter.
   Return NULL if there was any error during parsing.  */

static agent_expr_up
parse_cmd_to_aexpr (CORE_ADDR scope, char *cmd)
{
  const char *cmdrest;
  const char *format_start, *format_end;
  struct gdbarch *gdbarch = get_current_arch ();

  if (cmd == NULL)
    return NULL;

  cmdrest = cmd;

  if (*cmdrest == ',')
    ++cmdrest;
  cmdrest = skip_spaces (cmdrest);

  if (*cmdrest++ != '"')
    error (_("No format string following the location"));

  format_start = cmdrest;

  format_pieces fpieces (&cmdrest);

  format_end = cmdrest;

  if (*cmdrest++ != '"')
    error (_("Bad format string, non-terminated '\"'."));
  
  cmdrest = skip_spaces (cmdrest);

  if (!(*cmdrest == ',' || *cmdrest == '\0'))
    error (_("Invalid argument syntax"));

  if (*cmdrest == ',')
    cmdrest++;
  cmdrest = skip_spaces (cmdrest);

  /* For each argument, make an expression.  */

  std::vector<struct expression *> argvec;
  while (*cmdrest != '\0')
    {
      const char *cmd1;

      cmd1 = cmdrest;
      expression_up expr = parse_exp_1 (&cmd1, scope, block_for_pc (scope),
					PARSER_COMMA_TERMINATES);
      argvec.push_back (expr.release ());
      cmdrest = cmd1;
      if (*cmdrest == ',')
	++cmdrest;
    }

  agent_expr_up aexpr;

  /* We don't want to stop processing, so catch any errors
     that may show up.  */
  try
    {
      aexpr = gen_printf (scope, gdbarch, 0, 0,
			  format_start, format_end - format_start,
			  argvec.size (), argvec.data ());
    }
  catch (const gdb_exception_error &ex)
    {
      /* If we got here, it means the command could not be parsed to a valid
	 bytecode expression and thus can't be evaluated on the target's side.
	 It's no use iterating through the other commands.  */
    }

  /* We have a valid agent expression, return it.  */
  return aexpr;
}

/* Based on location BL, create a list of breakpoint commands to be
   passed on to the target.  If we have duplicated locations with
   different commands, we will add any such to the list.  */

static void
build_target_command_list (struct bp_location *bl)
{
  bool null_command_or_parse_error = false;
  int modified = bl->needs_update;

  /* Clear commands left over from a previous insert.  */
  bl->target_info.tcommands.clear ();

  if (!target_can_run_breakpoint_commands ())
    return;

  /* For now, limit to agent-style dprintf breakpoints.  */
  if (dprintf_style != dprintf_style_agent)
    return;

  auto loc_range = all_bp_locations_at_addr (bl->address);

  /* For now, if we have any location at the same address that isn't a
     dprintf, don't install the target-side commands, as that would
     make the breakpoint not be reported to the core, and we'd lose
     control.  */
  for (bp_location *loc : loc_range)
    if (is_breakpoint (loc->owner)
	&& loc->pspace->num == bl->pspace->num
	&& loc->owner->type != bp_dprintf)
      return;

  /* Do a first pass to check for locations with no assigned
     conditions or conditions that fail to parse to a valid agent expression
     bytecode.  If any of these happen, then it's no use to send conditions
     to the target since this location will always trigger and generate a
     response back to GDB.  */
  for (bp_location *loc : loc_range)
    {
      if (is_breakpoint (loc->owner) && loc->pspace->num == bl->pspace->num)
	{
	  if (modified)
	    {
	      /* Re-parse the commands since something changed.  In that
		 case we already freed the command bytecodes (see
		 force_breakpoint_reinsertion).  We just
		 need to parse the command to bytecodes again.  */
	      loc->cmd_bytecode
		= parse_cmd_to_aexpr (bl->address,
				      loc->owner->extra_string.get ());
	    }

	  /* If we have a NULL bytecode expression, it means something
	     went wrong or we have a null command expression.  */
	  if (!loc->cmd_bytecode)
	    {
	      null_command_or_parse_error = true;
	      break;
	    }
	}
    }

  /* If anything failed, then we're not doing target-side commands,
     and so clean up.  */
  if (null_command_or_parse_error)
    {
      for (bp_location *loc : loc_range)
	if (is_breakpoint (loc->owner)
	    && loc->pspace->num == bl->pspace->num)
	  {
	    /* Only go as far as the first NULL bytecode is
	       located.  */
	    if (loc->cmd_bytecode == NULL)
	      return;

	    loc->cmd_bytecode.reset ();
	  }
    }

  /* No NULL commands or failed bytecode generation.  Build a command
     list for all duplicate locations at this location's address.
     Note that here we must care for whether the breakpoint location
     types are considered duplicates, otherwise, say, if we have a
     software and hardware location at the same address, the target
     could end up running the commands twice.  For the moment, we only
     support targets-side commands with dprintf, but it doesn't hurt
     to be pedantically correct in case that changes.  */
  for (bp_location *loc : loc_range)
    if (breakpoint_locations_match (bl, loc)
	&& loc->owner->extra_string
	&& is_breakpoint (loc->owner)
	&& loc->pspace->num == bl->pspace->num
	&& loc->owner->enable_state == bp_enabled
	&& loc->enabled
	&& !loc->disabled_by_cond)
      {
	/* Add the command to the vector.  This will be used later
	   to send the commands to the target.  */
	bl->target_info.tcommands.push_back (loc->cmd_bytecode.get ());
      }

  bl->target_info.persist = 0;
  /* Maybe flag this location as persistent.  */
  if (bl->owner->type == bp_dprintf && disconnected_dprintf)
    bl->target_info.persist = 1;
}

/* Return the kind of breakpoint on address *ADDR.  Get the kind
   of breakpoint according to ADDR except single-step breakpoint.
   Get the kind of single-step breakpoint according to the current
   registers state.  */

static int
breakpoint_kind (const struct bp_location *bl, CORE_ADDR *addr)
{
  if (bl->owner->type == bp_single_step)
    {
      struct thread_info *thr = find_thread_global_id (bl->owner->thread);
      struct regcache *regcache;

      regcache = get_thread_regcache (thr);

      return gdbarch_breakpoint_kind_from_current_state (bl->gdbarch,
							 regcache, addr);
    }
  else
    return gdbarch_breakpoint_kind_from_pc (bl->gdbarch, addr);
}

/* Rethrow the currently handled exception, if it's a TARGET_CLOSE_ERROR.
   E is either the currently handled exception, or a copy, or a sliced copy,
   so we can't rethrow that one, but we can use it to inspect the properties
   of the currently handled exception.  */

static void
rethrow_on_target_close_error (const gdb_exception &e)
{
  if (e.reason == 0)
    return;
  /* Can't set the breakpoint.  */

  if (e.error != TARGET_CLOSE_ERROR)
    return;

  /* If the target has closed then it will have deleted any breakpoints
     inserted within the target inferior, as a result any further attempts
     to interact with the breakpoint objects is not possible.  Just rethrow
     the error.  Don't use e to rethrow, to prevent object slicing of the
     exception.  */
  throw;
}

/* Insert a low-level "breakpoint" of some type.  BL is the breakpoint
   location.  Any error messages are printed to TMP_ERROR_STREAM; and
   DISABLED_BREAKS, and HW_BREAKPOINT_ERROR are used to report problems.
   Returns 0 for success, 1 if the bp_location type is not supported or
   -1 for failure.

   NOTE drow/2003-09-09: This routine could be broken down to an
   object-style method for each breakpoint or catchpoint type.  */
static int
insert_bp_location (struct bp_location *bl,
		    struct ui_file *tmp_error_stream,
		    int *disabled_breaks,
		    int *hw_breakpoint_error,
		    int *hw_bp_error_explained_already)
{
  gdb_exception bp_excpt;

  if (!should_be_inserted (bl) || (bl->inserted && !bl->needs_update))
    return 0;

  breakpoint_debug_printf ("%s", breakpoint_location_address_str (bl).c_str ());

  /* Note we don't initialize bl->target_info, as that wipes out
     the breakpoint location's shadow_contents if the breakpoint
     is still inserted at that location.  This in turn breaks
     target_read_memory which depends on these buffers when
     a memory read is requested at the breakpoint location:
     Once the target_info has been wiped, we fail to see that
     we have a breakpoint inserted at that address and thus
     read the breakpoint instead of returning the data saved in
     the breakpoint location's shadow contents.  */
  bl->target_info.reqstd_address = bl->address;
  bl->target_info.placed_address_space = bl->pspace->aspace.get ();
  bl->target_info.length = bl->length;

  /* When working with target-side conditions, we must pass all the conditions
     for the same breakpoint address down to the target since GDB will not
     insert those locations.  With a list of breakpoint conditions, the target
     can decide when to stop and notify GDB.  */

  if (is_breakpoint (bl->owner))
    {
      build_target_condition_list (bl);
      build_target_command_list (bl);
      /* Reset the modification marker.  */
      bl->needs_update = 0;
    }

  /* If "set breakpoint auto-hw" is "on" and a software breakpoint was
     set at a read-only address, then a breakpoint location will have
     been changed to hardware breakpoint before we get here.  If it is
     "off" however, error out before actually trying to insert the
     breakpoint, with a nicer error message.  */
  if (bl->loc_type == bp_loc_software_breakpoint
      && !automatic_hardware_breakpoints)
    {
      mem_region *mr = lookup_mem_region (bl->address);

      if (mr != nullptr && mr->attrib.mode != MEM_RW)
	{
	  gdb_printf (tmp_error_stream,
		      _("Cannot insert breakpoint %d.\n"
			"Cannot set software breakpoint "
			"at read-only address %s\n"),
		      bl->owner->number,
		      paddress (bl->gdbarch, bl->address));
	  return 1;
	}
    }

  if (bl->loc_type == bp_loc_software_breakpoint
      || bl->loc_type == bp_loc_hardware_breakpoint)
    {
      /* First check to see if we have to handle an overlay.  */
      if (overlay_debugging == ovly_off
	  || bl->section == NULL
	  || !(section_is_overlay (bl->section)))
	{
	  /* No overlay handling: just set the breakpoint.  */
	  try
	    {
	      int val;

	      val = bl->owner->insert_location (bl);
	      if (val)
		bp_excpt = gdb_exception {RETURN_ERROR, GENERIC_ERROR};
	    }
	  catch (gdb_exception &e)
	    {
	      rethrow_on_target_close_error (e);
	      bp_excpt = std::move (e);
	    }
	}
      else
	{
	  /* This breakpoint is in an overlay section.
	     Shall we set a breakpoint at the LMA?  */
	  if (!overlay_events_enabled)
	    {
	      /* Yes -- overlay event support is not active, 
		 so we must try to set a breakpoint at the LMA.
		 This will not work for a hardware breakpoint.  */
	      if (bl->loc_type == bp_loc_hardware_breakpoint)
		warning (_("hardware breakpoint %d not supported in overlay!"),
			 bl->owner->number);
	      else
		{
		  CORE_ADDR addr = overlay_unmapped_address (bl->address,
							     bl->section);
		  /* Set a software (trap) breakpoint at the LMA.  */
		  bl->overlay_target_info = bl->target_info;
		  bl->overlay_target_info.reqstd_address = addr;

		  /* No overlay handling: just set the breakpoint.  */
		  try
		    {
		      int val;

		      bl->overlay_target_info.kind
			= breakpoint_kind (bl, &addr);
		      bl->overlay_target_info.placed_address = addr;
		      val = target_insert_breakpoint (bl->gdbarch,
						      &bl->overlay_target_info);
		      if (val)
			bp_excpt
			  = gdb_exception {RETURN_ERROR, GENERIC_ERROR};
		    }
		  catch (gdb_exception &e)
		    {
		      rethrow_on_target_close_error (e);
		      bp_excpt = std::move (e);
		    }

		  if (bp_excpt.reason != 0)
		    gdb_printf (tmp_error_stream,
				"Overlay breakpoint %d "
				"failed: in ROM?\n",
				bl->owner->number);
		}
	    }
	  /* Shall we set a breakpoint at the VMA? */
	  if (section_is_mapped (bl->section))
	    {
	      /* Yes.  This overlay section is mapped into memory.  */
	      try
		{
		  int val;

		  val = bl->owner->insert_location (bl);
		  if (val)
		    bp_excpt = gdb_exception {RETURN_ERROR, GENERIC_ERROR};
		}
	      catch (gdb_exception_error &e)
		{
		  rethrow_on_target_close_error (e);
		  bp_excpt = std::move (e);
		}
	    }
	  else
	    {
	      /* No.  This breakpoint will not be inserted.  
		 No error, but do not mark the bp as 'inserted'.  */
	      return 0;
	    }
	}

      if (bp_excpt.reason != 0)
	{
	  /* Can't set the breakpoint.  */
	  gdb_assert (bl->owner != nullptr);

	  /* In some cases, we might not be able to insert a
	     breakpoint in a shared library that has already been
	     removed, but we have not yet processed the shlib unload
	     event.  Unfortunately, some targets that implement
	     breakpoint insertion themselves can't tell why the
	     breakpoint insertion failed (e.g., the remote target
	     doesn't define error codes), so we must treat generic
	     errors as memory errors.  */
	  if (bp_excpt.reason == RETURN_ERROR
	      && (bp_excpt.error == GENERIC_ERROR
		  || bp_excpt.error == MEMORY_ERROR)
	      && bl->loc_type == bp_loc_software_breakpoint
	      && (solib_name_from_address (bl->pspace, bl->address)
		  || shared_objfile_contains_address_p (bl->pspace,
							bl->address)))
	    {
	      /* See also: disable_breakpoints_in_shlibs.  */
	      bl->shlib_disabled = 1;
	      notify_breakpoint_modified (bl->owner);
	      if (!*disabled_breaks)
		{
		  gdb_printf (tmp_error_stream, 
			      "Cannot insert breakpoint %d.\n", 
			      bl->owner->number);
		  gdb_printf (tmp_error_stream, 
			      "Temporarily disabling shared "
			      "library breakpoints:\n");
		}
	      *disabled_breaks = 1;
	      gdb_printf (tmp_error_stream,
			  "breakpoint #%d\n", bl->owner->number);
	      return 0;
	    }
	  else
	    {
	      if (bl->loc_type == bp_loc_hardware_breakpoint)
		{
		  *hw_breakpoint_error = 1;
		  *hw_bp_error_explained_already = bp_excpt.message != NULL;
		  gdb_printf (tmp_error_stream,
			      "Cannot insert hardware breakpoint %d%s",
			      bl->owner->number,
			      bp_excpt.message ? ":" : ".\n");
		  if (bp_excpt.message != NULL)
		    gdb_printf (tmp_error_stream, "%s.\n",
				bp_excpt.what ());
		}
	      else
		{
		  if (bp_excpt.message == NULL)
		    {
		      std::string message
			= memory_error_message (TARGET_XFER_E_IO,
						bl->gdbarch, bl->address);

		      gdb_printf (tmp_error_stream,
				  "Cannot insert breakpoint %d.\n"
				  "%s\n",
				  bl->owner->number, message.c_str ());
		    }
		  else
		    {
		      gdb_printf (tmp_error_stream,
				  "Cannot insert breakpoint %d: %s\n",
				  bl->owner->number,
				  bp_excpt.what ());
		    }
		}
	      return 1;

	    }
	}
      else
	bl->inserted = 1;

      return 0;
    }

  else if (bl->loc_type == bp_loc_hardware_watchpoint
	   && bl->owner->disposition != disp_del_at_next_stop)
    {
      int val;

      val = bl->owner->insert_location (bl);

      /* If trying to set a read-watchpoint, and it turns out it's not
	 supported, try emulating one with an access watchpoint.  */
      if (val == 1 && bl->watchpoint_type == hw_read)
	{
	  /* But don't try to insert it, if there's already another
	     hw_access location that would be considered a duplicate
	     of this one.  */
	  for (bp_location *loc : all_bp_locations ())
	    if (loc != bl
		&& loc->watchpoint_type == hw_access
		&& watchpoint_locations_match (bl, loc))
	      {
		bl->duplicate = 1;
		bl->inserted = 1;
		bl->target_info = loc->target_info;
		bl->watchpoint_type = hw_access;
		val = 0;
		break;
	      }

	  if (val == 1)
	    {
	      bl->watchpoint_type = hw_access;
	      val = bl->owner->insert_location (bl);

	      if (val)
		/* Back to the original value.  */
		bl->watchpoint_type = hw_read;
	    }
	}

      bl->inserted = (val == 0);
    }

  else if (bl->owner->type == bp_catchpoint)
    {
      int val;

      val = bl->owner->insert_location (bl);
      if (val)
	{
	  bl->owner->enable_state = bp_disabled;

	  if (val == 1)
	    warning (_("\
Error inserting catchpoint %d: Your system does not support this type\n\
of catchpoint."), bl->owner->number);
	  else
	    warning (_("Error inserting catchpoint %d."), bl->owner->number);
	}

      bl->inserted = (val == 0);

      /* We've already printed an error message if there was a problem
	 inserting this catchpoint, and we've disabled the catchpoint,
	 so just return success.  */
      return 0;
    }

  return 0;
}

/* This function is called when program space PSPACE is about to be
   deleted.  It takes care of updating breakpoints to not reference
   PSPACE anymore.  */

void
breakpoint_program_space_exit (struct program_space *pspace)
{
  /* Remove any breakpoint that was set through this program space.  */
  for (breakpoint &b : all_breakpoints_safe ())
    if (b.pspace == pspace)
      delete_breakpoint (&b);

  /* Breakpoints set through other program spaces could have locations
     bound to PSPACE as well.  Remove those.  */
  for (bp_location *loc : all_bp_locations ())
    if (loc->pspace == pspace)
      {
	/* ALL_BP_LOCATIONS bp_location has LOC->OWNER always non-NULL.  */
	loc->owner->unadd_location (*loc);
      }

  /* Now update the global location list to permanently delete the
     removed locations above.  */
  update_global_location_list (UGLL_DONT_INSERT);
}

/* Make sure all breakpoints are inserted in inferior.
   Throws exception on any error.
   A breakpoint that is already inserted won't be inserted
   again, so calling this function twice is safe.  */
void
insert_breakpoints (void)
{
  for (breakpoint &bpt : all_breakpoints ())
    if (is_hardware_watchpoint (&bpt))
      {
	watchpoint &w = gdb::checked_static_cast<watchpoint &> (bpt);

	update_watchpoint (&w, false /* don't reparse.  */);
      }

  /* Updating watchpoints creates new locations, so update the global
     location list.  Explicitly tell ugll to insert locations and
     ignore breakpoints_always_inserted_mode.  Also,
     update_global_location_list tries to "upgrade" software
     breakpoints to hardware breakpoints to handle "set breakpoint
     auto-hw", so we need to call it even if we don't have new
     locations.  */
  update_global_location_list (UGLL_INSERT);
}

/* This is used when we need to synch breakpoint conditions between GDB and the
   target.  It is the case with deleting and disabling of breakpoints when using
   always-inserted mode.  */

static void
update_inserted_breakpoint_locations (void)
{
  int error_flag = 0;
  int val = 0;
  int disabled_breaks = 0;
  int hw_breakpoint_error = 0;
  int hw_bp_details_reported = 0;

  string_file tmp_error_stream;

  /* Explicitly mark the warning -- this will only be printed if
     there was an error.  */
  tmp_error_stream.puts ("Warning:\n");

  scoped_restore_current_pspace_and_thread restore_pspace_thread;

  for (bp_location *bl : all_bp_locations ())
    {
      /* We only want to update software breakpoints and hardware
	 breakpoints.  */
      if (!is_breakpoint (bl->owner))
	continue;

      /* We only want to update locations that are already inserted
	 and need updating.  This is to avoid unwanted insertion during
	 deletion of breakpoints.  */
      if (!bl->inserted || !bl->needs_update)
	continue;

      switch_to_program_space_and_thread (bl->pspace);

      /* For targets that support global breakpoints, there's no need
	 to select an inferior to insert breakpoint to.  In fact, even
	 if we aren't attached to any process yet, we should still
	 insert breakpoints.  */
      if (!gdbarch_has_global_breakpoints (current_inferior ()->arch ())
	  && (inferior_ptid == null_ptid || !target_has_execution ()))
	continue;

      val = insert_bp_location (bl, &tmp_error_stream, &disabled_breaks,
				    &hw_breakpoint_error, &hw_bp_details_reported);
      if (val)
	error_flag = val;
    }

  if (error_flag)
    {
      target_terminal::ours_for_output ();
      error (("%s"), tmp_error_stream.c_str ());
    }
}

/* Used when starting or continuing the program.  */

static void
insert_breakpoint_locations (void)
{
  int error_flag = 0;
  int val = 0;
  int disabled_breaks = 0;
  int hw_breakpoint_error = 0;
  int hw_bp_error_explained_already = 0;

  string_file tmp_error_stream;

  /* Explicitly mark the warning -- this will only be printed if
     there was an error.  */
  tmp_error_stream.puts ("Warning:\n");

  scoped_restore_current_pspace_and_thread restore_pspace_thread;

  for (bp_location *bl : all_bp_locations ())
    {
      if (!should_be_inserted (bl) || (bl->inserted && !bl->needs_update))
	continue;

      /* There is no point inserting thread-specific breakpoints if
	 the thread no longer exists.  ALL_BP_LOCATIONS bp_location
	 has BL->OWNER always non-NULL.  */
      if (bl->owner->thread != -1
	  && !valid_global_thread_id (bl->owner->thread))
	continue;

      /* Or inferior specific breakpoints if the inferior no longer
	 exists.  */
      if (bl->owner->inferior != -1
	  && !valid_global_inferior_id (bl->owner->inferior))
	continue;

      switch_to_program_space_and_thread (bl->pspace);

      /* For targets that support global breakpoints, there's no need
	 to select an inferior to insert breakpoint to.  In fact, even
	 if we aren't attached to any process yet, we should still
	 insert breakpoints.  */
      if (!gdbarch_has_global_breakpoints (current_inferior ()->arch ())
	  && (inferior_ptid == null_ptid || !target_has_execution ()))
	continue;

      val = insert_bp_location (bl, &tmp_error_stream, &disabled_breaks,
				    &hw_breakpoint_error, &hw_bp_error_explained_already);
      if (val)
	error_flag = val;
    }

  /* If we failed to insert all locations of a watchpoint, remove
     them, as half-inserted watchpoint is of limited use.  */
  for (breakpoint &bpt : all_breakpoints ())
    {
      bool some_failed = false;

      if (!is_hardware_watchpoint (&bpt))
	continue;

      if (!breakpoint_enabled (&bpt))
	continue;

      if (bpt.disposition == disp_del_at_next_stop)
	continue;

      for (bp_location &loc : bpt.locations ())
	if (!loc.inserted && should_be_inserted (&loc))
	  {
	    some_failed = true;
	    break;
	  }

      if (some_failed)
	{
	  for (bp_location &loc : bpt.locations ())
	    if (loc.inserted)
	      remove_breakpoint (&loc);

	  hw_breakpoint_error = 1;
	  tmp_error_stream.printf ("Could not insert "
				   "hardware watchpoint %d.\n",
				   bpt.number);
	  error_flag = -1;
	}
    }

  if (error_flag)
    {
      /* If a hardware breakpoint or watchpoint was inserted, add a
	 message about possibly exhausted resources.  */
      if (hw_breakpoint_error && !hw_bp_error_explained_already)
	{
	  tmp_error_stream.printf ("Could not insert hardware breakpoints:\n\
You may have requested too many hardware breakpoints/watchpoints.\n");
	}
      target_terminal::ours_for_output ();
      error (("%s"), tmp_error_stream.c_str ());
    }
}

/* Used when the program stops.
   Returns zero if successful, or non-zero if there was a problem
   removing a breakpoint location.  */

int
remove_breakpoints (void)
{
  int val = 0;

  for (bp_location *bl : all_bp_locations ())
    if (bl->inserted && !is_tracepoint (bl->owner))
      val |= remove_breakpoint (bl);

  return val;
}

/* When a thread exits, remove breakpoints that are related to
   that thread.  */

static void
remove_threaded_breakpoints (thread_info *tp,
			     std::optional<ULONGEST> /* exit_code */,
			     int /* silent */)
{
  for (breakpoint &b : all_breakpoints_safe ())
    {
      if (b.thread == tp->global_num && user_breakpoint_p (&b))
	{
	  gdb_printf (_("\
Thread-specific breakpoint %d deleted - thread %s no longer in the thread list.\n"),
		      b.number, print_thread_id (tp));
	  delete_breakpoint (&b);
       }
    }
}

/* Called when inferior INF has been removed from GDB.  Remove associated
   per-inferior breakpoints.  */

static void
remove_inferior_breakpoints (struct inferior *inf)
{
  for (breakpoint &b : all_breakpoints_safe ())
    {
      if (b.inferior == inf->num && user_breakpoint_p (&b))
	{
	  /* Tell the user the breakpoint has been deleted.  But only for
	     breakpoints that would not normally have been deleted at the
	     next stop anyway.  */
	  if (b.disposition != disp_del
	      && b.disposition != disp_del_at_next_stop)
	    gdb_printf (_("\
Inferior-specific breakpoint %d deleted - inferior %d has been removed.\n"),
			b.number, inf->num);
	  delete_breakpoint (&b);
       }
    }
}

/* See breakpoint.h.  */

void
remove_breakpoints_inf (inferior *inf)
{
  int val;

  breakpoint_debug_printf ("inf->num = %d", inf->num);

  for (bp_location *bl : all_bp_locations ())
    {
      if (bl->pspace != inf->pspace)
	continue;

      if (bl->inserted && !bl->target_info.persist)
	{
	  val = remove_breakpoint (bl);
	  if (val != 0)
	    return;
	}
    }
}

static int internal_breakpoint_number = -1;

/* Set the breakpoint number of B, depending on the value of INTERNAL.
   If INTERNAL is non-zero, the breakpoint number will be populated
   from internal_breakpoint_number and that variable decremented.
   Otherwise the breakpoint number will be populated from
   breakpoint_count and that value incremented.  Internal breakpoints
   do not set the internal var bpnum.  */
static void
set_breakpoint_number (int internal, struct breakpoint *b)
{
  if (internal)
    b->number = internal_breakpoint_number--;
  else
    {
      set_breakpoint_count (breakpoint_count + 1);
      b->number = breakpoint_count;
    }
}

/* Create a TYPE breakpoint on ADDRESS from an object file with GDBARCH.  */

static struct breakpoint *
create_internal_breakpoint (struct gdbarch *gdbarch,
			    CORE_ADDR address, enum bptype type)
{
  std::unique_ptr<internal_breakpoint> b
    (new internal_breakpoint (gdbarch, type, address));

  b->number = internal_breakpoint_number--;

  return add_to_breakpoint_chain (std::move (b));
}

/* Create a TYPE breakpoint on minimal symbol MSYM from an object file with
   GDBARCH.  */

static struct breakpoint *
create_internal_breakpoint (struct gdbarch *gdbarch,
			    struct bound_minimal_symbol &msym, enum bptype type)
{
  CORE_ADDR address;

  address = msym.value_address ();

  address = gdbarch_convert_from_func_ptr_addr
    (gdbarch, address, current_inferior ()->top_target ());

  /* Note that we're not using gdbarch_addr_bits_remove here, because that's
     related to addresses in $pc.  We're getting the address from the
     minimal symbol table.  */

  /* Is gdbarch_deprecated_function_start_offset needed here?  Or is that dealt
     with elsewhere?  Needs testing on vax.  */

  if (gdbarch_skip_entrypoint_p (gdbarch))
    address = gdbarch_skip_entrypoint (gdbarch, address);

  return create_internal_breakpoint (gdbarch, address, type);
}

static const char *const longjmp_names[] =
  {
    "longjmp", "_longjmp", "siglongjmp", "_siglongjmp"
  };
#define NUM_LONGJMP_NAMES ARRAY_SIZE(longjmp_names)

/* Per-objfile data private to breakpoint.c.  */
struct breakpoint_objfile_data
{
  /* Minimal symbol for "_ovly_debug_event" (if any).  */
  struct bound_minimal_symbol overlay_msym;

  /* Minimal symbol(s) for "longjmp", "siglongjmp", etc. (if any).  */
  struct bound_minimal_symbol longjmp_msym[NUM_LONGJMP_NAMES];

  /* True if we have looked for longjmp probes.  */
  int longjmp_searched = 0;

  /* SystemTap probe points for longjmp (if any).  These are non-owning
     references.  */
  std::vector<probe *> longjmp_probes;

  /* Minimal symbol for "std::terminate()" (if any).  */
  struct bound_minimal_symbol terminate_msym;

  /* Minimal symbol for "_Unwind_DebugHook" (if any).  */
  struct bound_minimal_symbol exception_msym;

  /* True if we have looked for exception probes.  */
  int exception_searched = 0;

  /* SystemTap probe points for unwinding (if any).  These are non-owning
     references.  */
  std::vector<probe *> exception_probes;
};

static const registry<objfile>::key<breakpoint_objfile_data>
  breakpoint_objfile_key;

/* Minimal symbol not found sentinel.  */
static struct minimal_symbol msym_not_found;

/* Returns TRUE if MSYM point to the "not found" sentinel.  */

static bool
msym_not_found_p (const struct minimal_symbol *msym)
{
  return msym == &msym_not_found;
}

/* Return per-objfile data needed by breakpoint.c.
   Allocate the data if necessary.  */

static struct breakpoint_objfile_data *
get_breakpoint_objfile_data (struct objfile *objfile)
{
  struct breakpoint_objfile_data *bp_objfile_data;

  bp_objfile_data = breakpoint_objfile_key.get (objfile);
  if (bp_objfile_data == NULL)
    bp_objfile_data = breakpoint_objfile_key.emplace (objfile);
  return bp_objfile_data;
}

static void
create_overlay_event_breakpoint (void)
{
  const char *const func_name = "_ovly_debug_event";

  for (objfile *objfile : current_program_space->objfiles ())
    {
      struct breakpoint *b;
      struct breakpoint_objfile_data *bp_objfile_data;
      CORE_ADDR addr;

      bp_objfile_data = get_breakpoint_objfile_data (objfile);

      if (msym_not_found_p (bp_objfile_data->overlay_msym.minsym))
	continue;

      if (bp_objfile_data->overlay_msym.minsym == NULL)
	{
	  struct bound_minimal_symbol m;

	  m = lookup_minimal_symbol_text (func_name, objfile);
	  if (m.minsym == NULL)
	    {
	      /* Avoid future lookups in this objfile.  */
	      bp_objfile_data->overlay_msym.minsym = &msym_not_found;
	      continue;
	    }
	  bp_objfile_data->overlay_msym = m;
	}

      addr = bp_objfile_data->overlay_msym.value_address ();
      b = create_internal_breakpoint (objfile->arch (), addr,
				      bp_overlay_event);
      b->locspec = new_explicit_location_spec_function (func_name);

      if (overlay_debugging == ovly_auto)
	{
	  b->enable_state = bp_enabled;
	  overlay_events_enabled = 1;
	}
      else
       {
	 b->enable_state = bp_disabled;
	 overlay_events_enabled = 0;
       }
    }
}

/* Install a master longjmp breakpoint for OBJFILE using a probe.  Return
   true if a breakpoint was installed.  */

static bool
create_longjmp_master_breakpoint_probe (objfile *objfile)
{
  struct gdbarch *gdbarch = objfile->arch ();
  struct breakpoint_objfile_data *bp_objfile_data
    = get_breakpoint_objfile_data (objfile);

  if (!bp_objfile_data->longjmp_searched)
    {
      std::vector<probe *> ret
	= find_probes_in_objfile (objfile, "libc", "longjmp");

      if (!ret.empty ())
	{
	  /* We are only interested in checking one element.  */
	  probe *p = ret[0];

	  if (!p->can_evaluate_arguments ())
	    {
	      /* We cannot use the probe interface here,
		 because it does not know how to evaluate
		 arguments.  */
	      ret.clear ();
	    }
	}
      bp_objfile_data->longjmp_probes = ret;
      bp_objfile_data->longjmp_searched = 1;
    }

  if (bp_objfile_data->longjmp_probes.empty ())
    return false;

  for (probe *p : bp_objfile_data->longjmp_probes)
    {
      struct breakpoint *b;

      b = create_internal_breakpoint (gdbarch,
				      p->get_relocated_address (objfile),
				      bp_longjmp_master);
      b->locspec = new_probe_location_spec ("-probe-stap libc:longjmp");
      b->enable_state = bp_disabled;
    }

  return true;
}

/* Install master longjmp breakpoints for OBJFILE using longjmp_names.
   Return true if at least one breakpoint was installed.  */

static bool
create_longjmp_master_breakpoint_names (objfile *objfile)
{
  struct gdbarch *gdbarch = objfile->arch ();
  if (!gdbarch_get_longjmp_target_p (gdbarch))
    return false;

  struct breakpoint_objfile_data *bp_objfile_data
    = get_breakpoint_objfile_data (objfile);
  unsigned int installed_bp = 0;

  for (int i = 0; i < NUM_LONGJMP_NAMES; i++)
    {
      struct breakpoint *b;
      const char *func_name;
      CORE_ADDR addr;

      if (msym_not_found_p (bp_objfile_data->longjmp_msym[i].minsym))
	continue;

      func_name = longjmp_names[i];
      if (bp_objfile_data->longjmp_msym[i].minsym == NULL)
	{
	  struct bound_minimal_symbol m;

	  m = lookup_minimal_symbol_text (func_name, objfile);
	  if (m.minsym == NULL)
	    {
	      /* Prevent future lookups in this objfile.  */
	      bp_objfile_data->longjmp_msym[i].minsym = &msym_not_found;
	      continue;
	    }
	  bp_objfile_data->longjmp_msym[i] = m;
	}

      addr = bp_objfile_data->longjmp_msym[i].value_address ();
      b = create_internal_breakpoint (gdbarch, addr, bp_longjmp_master);
      b->locspec = new_explicit_location_spec_function (func_name);
      b->enable_state = bp_disabled;
      installed_bp++;
    }

  return installed_bp > 0;
}

/* Create a master longjmp breakpoint.  */

static void
create_longjmp_master_breakpoint (void)
{
  scoped_restore_current_program_space restore_pspace;

  for (struct program_space *pspace : program_spaces)
    {
      set_current_program_space (pspace);

      for (objfile *obj : current_program_space->objfiles ())
	{
	  /* Skip separate debug object, it's handled in the loop below.  */
	  if (obj->separate_debug_objfile_backlink != nullptr)
	    continue;

	  /* Try a probe kind breakpoint on main objfile.  */
	  if (create_longjmp_master_breakpoint_probe (obj))
	    continue;

	  /* Try longjmp_names kind breakpoints on main and separate_debug
	     objfiles.  */
	  for (objfile *debug_objfile : obj->separate_debug_objfiles ())
	    if (create_longjmp_master_breakpoint_names (debug_objfile))
	      break;
	}
    }
}

/* Create a master std::terminate breakpoint.  */
static void
create_std_terminate_master_breakpoint (void)
{
  const char *const func_name = "std::terminate()";

  scoped_restore_current_program_space restore_pspace;
  scoped_restore_current_language save_language;
  set_language (language_cplus);

  for (struct program_space *pspace : program_spaces)
    {
      set_current_program_space (pspace);

      for (objfile *objfile : current_program_space->objfiles ())
	{
	  struct breakpoint *b;
	  struct breakpoint_objfile_data *bp_objfile_data;

	  bp_objfile_data = get_breakpoint_objfile_data (objfile);

	  if (msym_not_found_p (bp_objfile_data->terminate_msym.minsym))
	    continue;

	  if (bp_objfile_data->terminate_msym.minsym == NULL)
	    {
	      struct bound_minimal_symbol m;

	      m = lookup_minimal_symbol (func_name, NULL, objfile);
	      if (m.minsym == NULL || (m.minsym->type () != mst_text
				       && m.minsym->type () != mst_file_text))
		{
		  /* Prevent future lookups in this objfile.  */
		  bp_objfile_data->terminate_msym.minsym = &msym_not_found;
		  continue;
		}
	      bp_objfile_data->terminate_msym = m;
	    }

	  b = create_internal_breakpoint (objfile->arch (),
					  bp_objfile_data->terminate_msym,
					  bp_std_terminate_master);
	  b->locspec = new_explicit_location_spec_function (func_name);
	  b->enable_state = bp_disabled;
	}
    }
}

/* Install a master breakpoint on the unwinder's debug hook for OBJFILE using a
   probe.  Return true if a breakpoint was installed.  */

static bool
create_exception_master_breakpoint_probe (objfile *objfile)
{
  struct breakpoint *b;
  struct gdbarch *gdbarch;
  struct breakpoint_objfile_data *bp_objfile_data;

  bp_objfile_data = get_breakpoint_objfile_data (objfile);

  /* We prefer the SystemTap probe point if it exists.  */
  if (!bp_objfile_data->exception_searched)
    {
      std::vector<probe *> ret
	= find_probes_in_objfile (objfile, "libgcc", "unwind");

      if (!ret.empty ())
	{
	  /* We are only interested in checking one element.  */
	  probe *p = ret[0];

	  if (!p->can_evaluate_arguments ())
	    {
	      /* We cannot use the probe interface here, because it does
		 not know how to evaluate arguments.  */
	      ret.clear ();
	    }
	}
      bp_objfile_data->exception_probes = ret;
      bp_objfile_data->exception_searched = 1;
    }

  if (bp_objfile_data->exception_probes.empty ())
    return false;

  gdbarch = objfile->arch ();

  for (probe *p : bp_objfile_data->exception_probes)
    {
      b = create_internal_breakpoint (gdbarch,
				      p->get_relocated_address (objfile),
				      bp_exception_master);
      b->locspec = new_probe_location_spec ("-probe-stap libgcc:unwind");
      b->enable_state = bp_disabled;
    }

  return true;
}

/* Install a master breakpoint on the unwinder's debug hook for OBJFILE using
   _Unwind_DebugHook.  Return true if a breakpoint was installed.  */

static bool
create_exception_master_breakpoint_hook (objfile *objfile)
{
  const char *const func_name = "_Unwind_DebugHook";
  struct breakpoint *b;
  struct gdbarch *gdbarch;
  struct breakpoint_objfile_data *bp_objfile_data;

  bp_objfile_data = get_breakpoint_objfile_data (objfile);

  if (msym_not_found_p (bp_objfile_data->exception_msym.minsym))
    return false;

  gdbarch = objfile->arch ();

  if (bp_objfile_data->exception_msym.minsym == NULL)
    {
      struct bound_minimal_symbol debug_hook;

      debug_hook = lookup_minimal_symbol_text (func_name, objfile);
      if (debug_hook.minsym == NULL)
	{
	  bp_objfile_data->exception_msym.minsym = &msym_not_found;
	  return false;
	}

      bp_objfile_data->exception_msym = debug_hook;
    }

  b = create_internal_breakpoint (gdbarch, bp_objfile_data->exception_msym,
				  bp_exception_master);
  b->locspec = new_explicit_location_spec_function (func_name);
  b->enable_state = bp_disabled;

  return true;
}

/* Install a master breakpoint on the unwinder's debug hook.  */

static void
create_exception_master_breakpoint (void)
{
  for (objfile *obj : current_program_space->objfiles ())
    {
      /* Skip separate debug object.  */
      if (obj->separate_debug_objfile_backlink)
	continue;

      /* Try a probe kind breakpoint.  */
      if (create_exception_master_breakpoint_probe (obj))
	continue;

      /* Iterate over main and separate debug objects and try an
	 _Unwind_DebugHook kind breakpoint.  */
      for (objfile *debug_objfile : obj->separate_debug_objfiles ())
	if (create_exception_master_breakpoint_hook (debug_objfile))
	  break;
    }
}

/* Does B have a location spec?  */

static bool
breakpoint_location_spec_empty_p (const struct breakpoint *b)
{
  return (b->locspec != nullptr && b->locspec->empty_p ());
}

void
update_breakpoints_after_exec (void)
{
  /* We're about to delete breakpoints from GDB's lists.  If the
     INSERTED flag is true, GDB will try to lift the breakpoints by
     writing the breakpoints' "shadow contents" back into memory.  The
     "shadow contents" are NOT valid after an exec, so GDB should not
     do that.  Instead, the target is responsible from marking
     breakpoints out as soon as it detects an exec.  We don't do that
     here instead, because there may be other attempts to delete
     breakpoints after detecting an exec and before reaching here.  */
  for (bp_location *bploc : all_bp_locations ())
    if (bploc->pspace == current_program_space)
      gdb_assert (!bploc->inserted);

  for (breakpoint &b : all_breakpoints_safe ())
    {
      if (b.pspace != current_program_space)
	continue;

      /* Solib breakpoints must be explicitly reset after an exec().  */
      if (b.type == bp_shlib_event)
	{
	  delete_breakpoint (&b);
	  continue;
	}

      /* JIT breakpoints must be explicitly reset after an exec().  */
      if (b.type == bp_jit_event)
	{
	  delete_breakpoint (&b);
	  continue;
	}

      /* Thread event breakpoints must be set anew after an exec(),
	 as must overlay event and longjmp master breakpoints.  */
      if (b.type == bp_thread_event || b.type == bp_overlay_event
	  || b.type == bp_longjmp_master || b.type == bp_std_terminate_master
	  || b.type == bp_exception_master)
	{
	  delete_breakpoint (&b);
	  continue;
	}

      /* Step-resume breakpoints are meaningless after an exec().  */
      if (b.type == bp_step_resume || b.type == bp_hp_step_resume)
	{
	  delete_breakpoint (&b);
	  continue;
	}

      /* Just like single-step breakpoints.  */
      if (b.type == bp_single_step)
	{
	  delete_breakpoint (&b);
	  continue;
	}

      /* Longjmp and longjmp-resume breakpoints are also meaningless
	 after an exec.  */
      if (b.type == bp_longjmp || b.type == bp_longjmp_resume
	  || b.type == bp_longjmp_call_dummy
	  || b.type == bp_exception || b.type == bp_exception_resume)
	{
	  delete_breakpoint (&b);
	  continue;
	}

      if (b.type == bp_catchpoint)
	{
	  /* For now, none of the bp_catchpoint breakpoints need to
	     do anything at this point.  In the future, if some of
	     the catchpoints need to something, we will need to add
	     a new method, and call this method from here.  */
	  continue;
	}

      /* bp_finish is a special case.  The only way we ought to be able
	 to see one of these when an exec() has happened, is if the user
	 caught a vfork, and then said "finish".  Ordinarily a finish just
	 carries them to the call-site of the current callee, by setting
	 a temporary bp there and resuming.  But in this case, the finish
	 will carry them entirely through the vfork & exec.

	 We don't want to allow a bp_finish to remain inserted now.  But
	 we can't safely delete it, 'cause finish_command has a handle to
	 the bp on a bpstat, and will later want to delete it.  There's a
	 chance (and I've seen it happen) that if we delete the bp_finish
	 here, that its storage will get reused by the time finish_command
	 gets 'round to deleting the "use to be a bp_finish" breakpoint.
	 We really must allow finish_command to delete a bp_finish.

	 In the absence of a general solution for the "how do we know
	 it's safe to delete something others may have handles to?"
	 problem, what we'll do here is just uninsert the bp_finish, and
	 let finish_command delete it.

	 (We know the bp_finish is "doomed" in the sense that it's
	 momentary, and will be deleted as soon as finish_command sees
	 the inferior stopped.  So it doesn't matter that the bp's
	 address is probably bogus in the new a.out, unlike e.g., the
	 solib breakpoints.)  */

      if (b.type == bp_finish)
	{
	  continue;
	}

      /* Without a symbolic address, we have little hope of the
	 pre-exec() address meaning the same thing in the post-exec()
	 a.out.  */
      if (breakpoint_location_spec_empty_p (&b))
	{
	  delete_breakpoint (&b);
	  continue;
	}
    }
}

int
detach_breakpoints (ptid_t ptid)
{
  int val = 0;
  scoped_restore save_inferior_ptid = make_scoped_restore (&inferior_ptid);
  struct inferior *inf = current_inferior ();

  if (ptid.pid () == inferior_ptid.pid ())
    error (_("Cannot detach breakpoints of inferior_ptid"));

  /* Set inferior_ptid; remove_breakpoint_1 uses this global.  */
  inferior_ptid = ptid;
  for (bp_location *bl : all_bp_locations ())
    {
      if (bl->pspace != inf->pspace)
	continue;

      /* This function must physically remove breakpoints locations
	 from the specified ptid, without modifying the breakpoint
	 package's state.  Locations of type bp_loc_other and
	 bp_loc_software_watchpoint are only maintained at GDB side,
	 so there is no need to remove them.  Moreover, removing these
	 would modify the breakpoint package's state.  */
      if (bl->loc_type == bp_loc_other
	  || bl->loc_type == bp_loc_software_watchpoint)
	continue;

      if (bl->inserted)
	val |= remove_breakpoint_1 (bl, DETACH_BREAKPOINT);
    }

  return val;
}

/* Remove the breakpoint location BL from the current address space.
   Note that this is used to detach breakpoints from a child fork.
   When we get here, the child isn't in the inferior list, and neither
   do we have objects to represent its address space --- we should
   *not* look at bl->pspace->aspace here.  */

static int
remove_breakpoint_1 (struct bp_location *bl, enum remove_bp_reason reason)
{
  breakpoint_debug_printf ("%s due to %s",
			   breakpoint_location_address_str (bl).c_str (),
			   remove_bp_reason_str (reason));

  int val;

  /* BL is never in moribund_locations by our callers.  */
  gdb_assert (bl->owner != NULL);

  /* The type of none suggests that owner is actually deleted.
     This should not ever happen.  */
  gdb_assert (bl->owner->type != bp_none);

  if (bl->loc_type == bp_loc_software_breakpoint
      || bl->loc_type == bp_loc_hardware_breakpoint)
    {
      /* "Normal" instruction breakpoint: either the standard
	 trap-instruction bp (bp_breakpoint), or a
	 bp_hardware_breakpoint.  */

      /* First check to see if we have to handle an overlay.  */
      if (overlay_debugging == ovly_off
	  || bl->section == NULL
	  || !(section_is_overlay (bl->section)))
	{
	  /* No overlay handling: just remove the breakpoint.  */

	  /* If we're trying to uninsert a memory breakpoint that we
	     know is set in a dynamic object that is marked
	     shlib_disabled, then either the dynamic object was
	     removed with "remove-symbol-file" or with
	     "nosharedlibrary".  In the former case, we don't know
	     whether another dynamic object might have loaded over the
	     breakpoint's address -- the user might well let us know
	     about it next with add-symbol-file (the whole point of
	     add-symbol-file is letting the user manually maintain a
	     list of dynamically loaded objects).  If we have the
	     breakpoint's shadow memory, that is, this is a software
	     breakpoint managed by GDB, check whether the breakpoint
	     is still inserted in memory, to avoid overwriting wrong
	     code with stale saved shadow contents.  Note that HW
	     breakpoints don't have shadow memory, as they're
	     implemented using a mechanism that is not dependent on
	     being able to modify the target's memory, and as such
	     they should always be removed.  */
	  if (bl->shlib_disabled
	      && bl->target_info.shadow_len != 0
	      && !memory_validate_breakpoint (bl->gdbarch, &bl->target_info))
	    val = 0;
	  else
	    val = bl->owner->remove_location (bl, reason);
	}
      else
	{
	  /* This breakpoint is in an overlay section.
	     Did we set a breakpoint at the LMA?  */
	  if (!overlay_events_enabled)
	      {
		/* Yes -- overlay event support is not active, so we
		   should have set a breakpoint at the LMA.  Remove it.  
		*/
		/* Ignore any failures: if the LMA is in ROM, we will
		   have already warned when we failed to insert it.  */
		if (bl->loc_type == bp_loc_hardware_breakpoint)
		  target_remove_hw_breakpoint (bl->gdbarch,
					       &bl->overlay_target_info);
		else
		  target_remove_breakpoint (bl->gdbarch,
					    &bl->overlay_target_info,
					    reason);
	      }
	  /* Did we set a breakpoint at the VMA? 
	     If so, we will have marked the breakpoint 'inserted'.  */
	  if (bl->inserted)
	    {
	      /* Yes -- remove it.  Previously we did not bother to
		 remove the breakpoint if the section had been
		 unmapped, but let's not rely on that being safe.  We
		 don't know what the overlay manager might do.  */

	      /* However, we should remove *software* breakpoints only
		 if the section is still mapped, or else we overwrite
		 wrong code with the saved shadow contents.  */
	      if (bl->loc_type == bp_loc_hardware_breakpoint
		  || section_is_mapped (bl->section))
		val = bl->owner->remove_location (bl, reason);
	      else
		val = 0;
	    }
	  else
	    {
	      /* No -- not inserted, so no need to remove.  No error.  */
	      val = 0;
	    }
	}

      /* In some cases, we might not be able to remove a breakpoint in
	 a shared library that has already been removed, but we have
	 not yet processed the shlib unload event.  Similarly for an
	 unloaded add-symbol-file object - the user might not yet have
	 had the chance to remove-symbol-file it.  shlib_disabled will
	 be set if the library/object has already been removed, but
	 the breakpoint hasn't been uninserted yet, e.g., after
	 "nosharedlibrary" or "remove-symbol-file" with breakpoints
	 always-inserted mode.  */
      if (val
	  && (bl->loc_type == bp_loc_software_breakpoint
	      && (bl->shlib_disabled
		  || solib_name_from_address (bl->pspace, bl->address)
		  || shared_objfile_contains_address_p (bl->pspace,
							bl->address))))
	val = 0;

      if (val)
	return val;
      bl->inserted = (reason == DETACH_BREAKPOINT);
    }
  else if (bl->loc_type == bp_loc_hardware_watchpoint)
    {
      bl->inserted = (reason == DETACH_BREAKPOINT);
      bl->owner->remove_location (bl, reason);

      /* Failure to remove any of the hardware watchpoints comes here.  */
      if (reason == REMOVE_BREAKPOINT && bl->inserted)
	warning (_("Could not remove hardware watchpoint %d."),
		 bl->owner->number);
    }
  else if (bl->owner->type == bp_catchpoint
	   && breakpoint_enabled (bl->owner)
	   && !bl->duplicate)
    {
      val = bl->owner->remove_location (bl, reason);
      if (val)
	return val;

      bl->inserted = (reason == DETACH_BREAKPOINT);
    }

  return 0;
}

static int
remove_breakpoint (struct bp_location *bl)
{
  /* BL is never in moribund_locations by our callers.  */
  gdb_assert (bl->owner != NULL);

  /* The type of none suggests that owner is actually deleted.
     This should not ever happen.  */
  gdb_assert (bl->owner->type != bp_none);

  scoped_restore_current_pspace_and_thread restore_pspace_thread;

  switch_to_program_space_and_thread (bl->pspace);

  return remove_breakpoint_1 (bl, REMOVE_BREAKPOINT);
}

/* Clear the "inserted" flag in all breakpoints.  */

void
mark_breakpoints_out (void)
{
  for (bp_location *bl : all_bp_locations ())
    if (bl->pspace == current_program_space)
      bl->inserted = 0;
}

/* Clear the "inserted" flag in all breakpoints and delete any
   breakpoints which should go away between runs of the program.

   Plus other such housekeeping that has to be done for breakpoints
   between runs.

   Note: this function gets called at the end of a run (by
   generic_mourn_inferior) and when a run begins (by
   init_wait_for_inferior).  */



void
breakpoint_init_inferior (enum inf_context context)
{
  struct program_space *pspace = current_program_space;

  /* If breakpoint locations are shared across processes, then there's
     nothing to do.  */
  if (gdbarch_has_global_breakpoints (current_inferior ()->arch ()))
    return;

  mark_breakpoints_out ();

  for (breakpoint &b : all_breakpoints_safe ())
    {
      if (b.has_locations () && b.first_loc ().pspace != pspace)
	continue;

      switch (b.type)
	{
	case bp_call_dummy:
	case bp_longjmp_call_dummy:

	  /* If the call dummy breakpoint is at the entry point it will
	     cause problems when the inferior is rerun, so we better get
	     rid of it.  */

	case bp_watchpoint_scope:

	  /* Also get rid of scope breakpoints.  */

	case bp_shlib_event:

	  /* Also remove solib event breakpoints.  Their addresses may
	     have changed since the last time we ran the program.
	     Actually we may now be debugging against different target;
	     and so the solib backend that installed this breakpoint may
	     not be used in by the target.  E.g.,

	     (gdb) file prog-linux
	     (gdb) run               # native linux target
	     ...
	     (gdb) kill
	     (gdb) file prog-win.exe
	     (gdb) tar rem :9999     # remote Windows gdbserver.
	  */

	case bp_step_resume:

	  /* Also remove step-resume breakpoints.  */

	case bp_single_step:

	  /* Also remove single-step breakpoints.  */

	  delete_breakpoint (&b);
	  break;

	case bp_watchpoint:
	case bp_hardware_watchpoint:
	case bp_read_watchpoint:
	case bp_access_watchpoint:
	  {
	    watchpoint &w = gdb::checked_static_cast<watchpoint &> (b);

	    /* Likewise for watchpoints on local expressions.  */
	    if (w.exp_valid_block != NULL)
	      delete_breakpoint (&b);
	    else
	      {
		/* Get rid of existing locations, which are no longer
		   valid.  New ones will be created in
		   update_watchpoint, when the inferior is restarted.
		   The next update_global_location_list call will
		   garbage collect them.  */
		b.clear_locations ();

		if (context == inf_starting)
		  {
		    /* Reset val field to force reread of starting value in
		       insert_breakpoints.  */
		    w.val.reset (nullptr);
		    w.val_valid = false;
		  }
	      }
	  }
	  break;
	default:
	  break;
	}
    }

  /* Get rid of the moribund locations.  */
  for (bp_location *bl : moribund_locations)
    decref_bp_location (&bl);
  moribund_locations.clear ();
}

/* These functions concern about actual breakpoints inserted in the
   target --- to e.g. check if we need to do decr_pc adjustment or if
   we need to hop over the bkpt --- so we check for address space
   match, not program space.  */

/* breakpoint_here_p (PC) returns non-zero if an enabled breakpoint
   exists at PC.  It returns ordinary_breakpoint_here if it's an
   ordinary breakpoint, or permanent_breakpoint_here if it's a
   permanent breakpoint.
   - When continuing from a location with an ordinary breakpoint, we
     actually single step once before calling insert_breakpoints.
   - When continuing from a location with a permanent breakpoint, we
     need to use the `SKIP_PERMANENT_BREAKPOINT' macro, provided by
     the target, to advance the PC past the breakpoint.  */

enum breakpoint_here
breakpoint_here_p (const address_space *aspace, CORE_ADDR pc)
{
  bool any_breakpoint_here = false;

  for (bp_location *bl : all_bp_locations ())
    {
      if (bl->loc_type != bp_loc_software_breakpoint
	  && bl->loc_type != bp_loc_hardware_breakpoint)
	continue;

      /* ALL_BP_LOCATIONS bp_location has BL->OWNER always non-NULL.  */
      if ((breakpoint_enabled (bl->owner)
	   || bl->permanent)
	  && breakpoint_location_address_match (bl, aspace, pc))
	{
	  if (overlay_debugging 
	      && section_is_overlay (bl->section)
	      && !section_is_mapped (bl->section))
	    continue;		/* unmapped overlay -- can't be a match */
	  else if (bl->permanent)
	    return permanent_breakpoint_here;
	  else
	    any_breakpoint_here = true;
	}
    }

  return any_breakpoint_here ? ordinary_breakpoint_here : no_breakpoint_here;
}

/* See breakpoint.h.  */

int
breakpoint_in_range_p (const address_space *aspace,
		       CORE_ADDR addr, ULONGEST len)
{
  for (bp_location *bl : all_bp_locations ())
    {
      if (bl->loc_type != bp_loc_software_breakpoint
	  && bl->loc_type != bp_loc_hardware_breakpoint)
	continue;

      if ((breakpoint_enabled (bl->owner)
	   || bl->permanent)
	  && breakpoint_location_address_range_overlap (bl, aspace,
							addr, len))
	{
	  if (overlay_debugging
	      && section_is_overlay (bl->section)
	      && !section_is_mapped (bl->section))
	    {
	      /* Unmapped overlay -- can't be a match.  */
	      continue;
	    }

	  return 1;
	}
    }

  return 0;
}

/* Return true if there's a moribund breakpoint at PC.  */

int
moribund_breakpoint_here_p (const address_space *aspace, CORE_ADDR pc)
{
  for (bp_location *loc : moribund_locations)
    if (breakpoint_location_address_match (loc, aspace, pc))
      return 1;

  return 0;
}

/* Returns true iff BL is inserted at PC, in address space ASPACE.  */

static bool
bp_location_inserted_here_p (const struct bp_location *bl,
			     const address_space *aspace, CORE_ADDR pc)
{
  if (bl->inserted
      && breakpoint_address_match (bl->pspace->aspace.get (), bl->address,
				   aspace, pc))
    {
      /* An unmapped overlay can't be a match.  */
      return !(overlay_debugging
	       && section_is_overlay (bl->section)
	       && !section_is_mapped (bl->section));
    }
  return false;
}

/* Returns non-zero iff there's a breakpoint inserted at PC.  */

int
breakpoint_inserted_here_p (const address_space *aspace, CORE_ADDR pc)
{
  for (bp_location *bl : all_bp_locations_at_addr (pc))
    {
      if (bl->loc_type != bp_loc_software_breakpoint
	  && bl->loc_type != bp_loc_hardware_breakpoint)
	continue;

      if (bp_location_inserted_here_p (bl, aspace, pc))
	return 1;
    }
  return 0;
}

/* This function returns non-zero iff there is a software breakpoint
   inserted at PC.  */

int
software_breakpoint_inserted_here_p (const address_space *aspace,
				     CORE_ADDR pc)
{
  for (bp_location *bl : all_bp_locations_at_addr (pc))
    {
      if (bl->loc_type != bp_loc_software_breakpoint)
	continue;

      if (bp_location_inserted_here_p (bl, aspace, pc))
	return 1;
    }

  return 0;
}

/* See breakpoint.h.  */

int
hardware_breakpoint_inserted_here_p (const address_space *aspace,
				     CORE_ADDR pc)
{
  for (bp_location *bl : all_bp_locations_at_addr (pc))
    {
      if (bl->loc_type != bp_loc_hardware_breakpoint)
	continue;

      if (bp_location_inserted_here_p (bl, aspace, pc))
	return 1;
    }

  return 0;
}

int
hardware_watchpoint_inserted_in_range (const address_space *aspace,
				       CORE_ADDR addr, ULONGEST len)
{
  for (breakpoint &bpt : all_breakpoints ())
    {
      if (bpt.type != bp_hardware_watchpoint
	  && bpt.type != bp_access_watchpoint)
	continue;

      if (!breakpoint_enabled (&bpt))
	continue;

      for (bp_location &loc : bpt.locations ())
	if (loc.pspace->aspace.get () == aspace && loc.inserted)
	  {
	    CORE_ADDR l, h;

	    /* Check for intersection.  */
	    l = std::max<CORE_ADDR> (loc.address, addr);
	    h = std::min<CORE_ADDR> (loc.address + loc.length, addr + len);
	    if (l < h)
	      return 1;
	  }
    }
  return 0;
}

/* See breakpoint.h.  */

bool
is_catchpoint (struct breakpoint *b)
{
  return (b->type == bp_catchpoint);
}

/* Clear a bpstat so that it says we are not at any breakpoint.
   Also free any storage that is part of a bpstat.  */

void
bpstat_clear (bpstat **bsp)
{
  bpstat *p;
  bpstat *q;

  if (bsp == 0)
    return;
  p = *bsp;
  while (p != NULL)
    {
      q = p->next;
      delete p;
      p = q;
    }
  *bsp = NULL;
}

bpstat::bpstat (const bpstat &other)
  : next (NULL),
    bp_location_at (other.bp_location_at),
    breakpoint_at (other.breakpoint_at),
    commands (other.commands),
    print (other.print),
    stop (other.stop),
    print_it (other.print_it)
{
  if (other.old_val != NULL)
    old_val = release_value (other.old_val->copy ());
}

/* Return a copy of a bpstat.  Like "bs1 = bs2" but all storage that
   is part of the bpstat is copied as well.  */

bpstat *
bpstat_copy (bpstat *bs)
{
  bpstat *p = nullptr;
  bpstat *tmp;
  bpstat *retval = nullptr;

  if (bs == NULL)
    return bs;

  for (; bs != NULL; bs = bs->next)
    {
      tmp = new bpstat (*bs);

      if (p == NULL)
	/* This is the first thing in the chain.  */
	retval = tmp;
      else
	p->next = tmp;
      p = tmp;
    }
  p->next = NULL;
  return retval;
}

/* Find the bpstat associated with this breakpoint.  */

bpstat *
bpstat_find_breakpoint (bpstat *bsp, struct breakpoint *breakpoint)
{
  if (bsp == NULL)
    return NULL;

  for (; bsp != NULL; bsp = bsp->next)
    {
      if (bsp->breakpoint_at == breakpoint)
	return bsp;
    }
  return NULL;
}

/* See breakpoint.h.  */

bool
bpstat_explains_signal (bpstat *bsp, enum gdb_signal sig)
{
  for (; bsp != NULL; bsp = bsp->next)
    {
      if (bsp->breakpoint_at == NULL)
	{
	  /* A moribund location can never explain a signal other than
	     GDB_SIGNAL_TRAP.  */
	  if (sig == GDB_SIGNAL_TRAP)
	    return true;
	}
      else
	{
	  if (bsp->breakpoint_at->explains_signal (sig))
	    return true;
	}
    }

  return false;
}

/* See breakpoint.h.  */

int
bpstat_num (bpstat **bsp, int *num)
{
  struct breakpoint *b;

  if ((*bsp) == NULL)
    return 0;			/* No more breakpoint values */

  /* We assume we'll never have several bpstats that correspond to a
     single breakpoint -- otherwise, this function might return the
     same number more than once and this will look ugly.  */
  b = (*bsp)->breakpoint_at;
  *bsp = (*bsp)->next;
  if (b == NULL)
    return -1;			/* breakpoint that's been deleted since */

  *num = b->number;		/* We have its number */
  return 1;
}

/* See breakpoint.h  */

int
bpstat_locno (const bpstat *bs)
{
  const struct breakpoint *b = bs->breakpoint_at;
  const struct bp_location *bl = bs->bp_location_at.get ();

  if (b != nullptr && b->has_multiple_locations ())
    {
      int locno = 1;

      for (bp_location &loc : b->locations ())
	{
	  if (bl == &loc)
	    return locno;

	  ++locno;
	}

      warning (_("location number not found for breakpoint %d address %s."),
	       b->number, paddress (bl->gdbarch, bl->address));
    }

  return 0;
}

/* See breakpoint.h.  */

void
print_num_locno (const bpstat *bs, struct ui_out *uiout)
{
  struct breakpoint *b = bs->breakpoint_at;

  if (b == nullptr)
    uiout->text (_("deleted breakpoint"));
  else
    {
      uiout->field_signed ("bkptno", b->number);

      int locno = bpstat_locno (bs);
      if (locno != 0)
	uiout->message (".%pF", signed_field ("locno", locno));
    }
}

/* See breakpoint.h.  */

void
bpstat_clear_actions (void)
{
  bpstat *bs;

  if (inferior_ptid == null_ptid)
    return;

  thread_info *tp = inferior_thread ();
  for (bs = tp->control.stop_bpstat; bs != NULL; bs = bs->next)
    {
      bs->commands = NULL;
      bs->old_val.reset (nullptr);
    }
}

/* Called when a command is about to proceed the inferior.  */

static void
breakpoint_about_to_proceed (void)
{
  if (inferior_ptid != null_ptid)
    {
      struct thread_info *tp = inferior_thread ();

      /* Allow inferior function calls in breakpoint commands to not
	 interrupt the command list.  When the call finishes
	 successfully, the inferior will be standing at the same
	 breakpoint as if nothing happened.  */
      if (tp->control.in_infcall)
	return;
    }

  breakpoint_proceeded = 1;
}

/* Return true iff CMD as the first line of a command sequence is `silent'
   or its equivalent.  */

static bool
command_line_is_silent (struct command_line *cmd)
{
  return cmd && (strcmp ("silent", cmd->line) == 0);
}

/* Sets the $_hit_bpnum and $_hit_locno to bpnum and locno.
   A locno 0 is changed to 1 to e.g. let the user do
     (gdb) disable $_hit_bpnum.$_hit_locno
   for a single location breakpoint.  */

static void
set_hit_convenience_vars (int bpnum, int locno)
{
  set_internalvar_integer (lookup_internalvar ("_hit_bpnum"), bpnum);
  set_internalvar_integer (lookup_internalvar ("_hit_locno"),
			   (locno > 0 ? locno : 1));
}

/* Execute all the commands associated with all the breakpoints at
   this location.  Any of these commands could cause the process to
   proceed beyond this point, etc.  We look out for such changes by
   checking the global "breakpoint_proceeded" after each command.

   Returns true if a breakpoint command resumed the inferior.  In that
   case, it is the caller's responsibility to recall it again with the
   bpstat of the current thread.  */

static bool
bpstat_do_actions_1 (bpstat **bsp)
{
  bpstat *bs;
  bool again = false;

  /* Avoid endless recursion if a `source' command is contained
     in bs->commands.  */
  if (executing_breakpoint_commands)
    return false;

  scoped_restore save_executing
    = make_scoped_restore (&executing_breakpoint_commands, 1);

  scoped_restore preventer = prevent_dont_repeat ();

  /* This pointer will iterate over the list of bpstat's.  */
  bs = *bsp;

  /* The $_hit_* convenience variables are set before running the
     commands of BS.  In case we have several bs, after the loop,
     we set again the variables to the first printed bpnum and locno.
     For multiple breakpoints, this ensures the variables are set to the
     breakpoint printed for the user. */
  int printed_hit_bpnum = -1;
  int printed_hit_locno = -1;

  breakpoint_proceeded = 0;
  for (; bs != NULL; bs = bs->next)
    {
      struct command_line *cmd = NULL;

      /* Set the _hit_* convenience variables before running BS's commands.  */
      {
	const struct breakpoint *b = bs->breakpoint_at;
	if (b != nullptr)
	  {
	    int locno = bpstat_locno (bs);

	    set_hit_convenience_vars (b->number, locno);
	    if (printed_hit_locno == -1 && bs->print)
	      {
		printed_hit_bpnum = b->number;
		printed_hit_locno = locno;
	      }
	  }
      }

      /* Take ownership of the BSP's command tree, if it has one.

	 The command tree could legitimately contain commands like
	 'step' and 'next', which call clear_proceed_status, which
	 frees the bpstat BS and its command tree.  To make sure this doesn't
	 free the tree we're executing out from under us, we need to
	 take ownership of the tree ourselves.  Since a given bpstat's
	 commands are only executed once, we don't need to copy it; we
	 can clear the pointer in the bpstat, and make sure we free
	 the tree when we're done.  */
      counted_command_line ccmd = bs->commands;
      bs->commands = NULL;
      if (ccmd != NULL)
	cmd = ccmd.get ();
      if (command_line_is_silent (cmd))
	{
	  /* The action has been already done by bpstat_stop_status.  */
	  cmd = cmd->next;
	}

      while (cmd != NULL)
	{
	  execute_control_command (cmd);
	  /* After execute_control_command, if breakpoint_proceeded is true,
	     BS has been freed and cannot be accessed anymore.  */

	  if (breakpoint_proceeded)
	    break;
	  else
	    cmd = cmd->next;
	}

      if (breakpoint_proceeded)
	{
	  if (current_ui->async)
	    /* If we are in async mode, then the target might be still
	       running, not stopped at any breakpoint, so nothing for
	       us to do here -- just return to the event loop.  */
	    ;
	  else
	    /* In sync mode, when execute_control_command returns
	       we're already standing on the next breakpoint.
	       Breakpoint commands for that stop were not run, since
	       execute_command does not run breakpoint commands --
	       only command_line_handler does, but that one is not
	       involved in execution of breakpoint commands.  So, we
	       can now execute breakpoint commands.  It should be
	       noted that making execute_command do bpstat actions is
	       not an option -- in this case we'll have recursive
	       invocation of bpstat for each breakpoint with a
	       command, and can easily blow up GDB stack.  Instead, we
	       return true, which will trigger the caller to recall us
	       with the new stop_bpstat.  */
	    again = true;
	  break;
	}
    }

  /* Now that we have executed the commands of all bs, set the _hit_*
     convenience variables to the printed values.  */
  if (printed_hit_locno != -1)
    set_hit_convenience_vars (printed_hit_bpnum, printed_hit_locno);

  return again;
}

/* Helper for bpstat_do_actions.  Get the current thread, if there's
   one, is alive and has execution.  Return NULL otherwise.  */

static thread_info *
get_bpstat_thread ()
{
  if (inferior_ptid == null_ptid || !target_has_execution ())
    return NULL;

  thread_info *tp = inferior_thread ();
  if (tp->state == THREAD_EXITED || tp->executing ())
    return NULL;
  return tp;
}

void
bpstat_do_actions (void)
{
  auto cleanup_if_error = make_scope_exit (bpstat_clear_actions);
  thread_info *tp;

  /* Do any commands attached to breakpoint we are stopped at.  */
  while ((tp = get_bpstat_thread ()) != NULL)
    {
      /* Since in sync mode, bpstat_do_actions may resume the
	 inferior, and only return when it is stopped at the next
	 breakpoint, we keep doing breakpoint actions until it returns
	 false to indicate the inferior was not resumed.  */
      if (!bpstat_do_actions_1 (&tp->control.stop_bpstat))
	break;
    }

  cleanup_if_error.release ();
}

/* Print out the (old or new) value associated with a watchpoint.  */

static void
watchpoint_value_print (struct value *val, struct ui_file *stream)
{
  if (val == NULL)
    fprintf_styled (stream, metadata_style.style (), _("<unreadable>"));
  else
    {
      struct value_print_options opts;
      get_user_print_options (&opts);
      value_print (val, stream, &opts);
    }
}

/* Print the "Thread ID hit" part of "Thread ID hit Breakpoint N" if
   debugging multiple threads.  */

void
maybe_print_thread_hit_breakpoint (struct ui_out *uiout)
{
  if (uiout->is_mi_like_p ())
    return;

  uiout->text ("\n");

  if (show_thread_that_caused_stop ())
    {
      struct thread_info *thr = inferior_thread ();

      uiout->text ("Thread ");
      uiout->field_string ("thread-id", print_thread_id (thr));

      const char *name = thread_name (thr);
      if (name != NULL)
	{
	  uiout->text (" \"");
	  uiout->field_string ("name", name);
	  uiout->text ("\"");
	}

      uiout->text (" hit ");
    }
}

/* Generic routine for printing messages indicating why we
   stopped.  The behavior of this function depends on the value
   'print_it' in the bpstat structure.  Under some circumstances we
   may decide not to print anything here and delegate the task to
   normal_stop().  */

static enum print_stop_action
print_bp_stop_message (bpstat *bs)
{
  switch (bs->print_it)
    {
    case print_it_noop:
      /* Nothing should be printed for this bpstat entry.  */
      return PRINT_UNKNOWN;

    case print_it_done:
      /* We still want to print the frame, but we already printed the
	 relevant messages.  */
      return PRINT_SRC_AND_LOC;

    case print_it_normal:
      {
	struct breakpoint *b = bs->breakpoint_at;

	/* bs->breakpoint_at can be NULL if it was a momentary breakpoint
	   which has since been deleted.  */
	if (b == NULL)
	  return PRINT_UNKNOWN;

	/* Normal case.  Call the breakpoint's print_it method.  */
	return b->print_it (bs);
      }

    default:
      internal_error (_("print_bp_stop_message: unrecognized enum value"));
    }
}

/* See breakpoint.h.  */

void
print_solib_event (bool is_catchpoint)
{
  bool any_deleted = !current_program_space->deleted_solibs.empty ();
  bool any_added = !current_program_space->added_solibs.empty ();

  if (!is_catchpoint)
    {
      if (any_added || any_deleted)
	current_uiout->text (_("Stopped due to shared library event:\n"));
      else
	current_uiout->text (_("Stopped due to shared library event (no "
			       "libraries added or removed)\n"));
    }

  if (current_uiout->is_mi_like_p ())
    current_uiout->field_string ("reason",
				 async_reason_lookup (EXEC_ASYNC_SOLIB_EVENT));

  if (any_deleted)
    {
      current_uiout->text (_("  Inferior unloaded "));
      ui_out_emit_list list_emitter (current_uiout, "removed");
      for (int ix = 0; ix < current_program_space->deleted_solibs.size (); ix++)
	{
	  const std::string &name = current_program_space->deleted_solibs[ix];

	  if (ix > 0)
	    current_uiout->text ("    ");
	  current_uiout->field_string ("library", name);
	  current_uiout->text ("\n");
	}
    }

  if (any_added)
    {
      current_uiout->text (_("  Inferior loaded "));
      ui_out_emit_list list_emitter (current_uiout, "added");
      bool first = true;
      for (shobj *iter : current_program_space->added_solibs)
	{
	  if (!first)
	    current_uiout->text ("    ");
	  first = false;
	  current_uiout->field_string ("library", iter->so_name);
	  current_uiout->text ("\n");
	}
    }
}

/* Print a message indicating what happened.  This is called from
   normal_stop().  The input to this routine is the head of the bpstat
   list - a list of the eventpoints that caused this stop.  KIND is
   the target_waitkind for the stopping event.  This
   routine calls the generic print routine for printing a message
   about reasons for stopping.  This will print (for example) the
   "Breakpoint n," part of the output.  The return value of this
   routine is one of:

   PRINT_UNKNOWN: Means we printed nothing.
   PRINT_SRC_AND_LOC: Means we printed something, and expect subsequent
   code to print the location.  An example is 
   "Breakpoint 1, " which should be followed by
   the location.
   PRINT_SRC_ONLY: Means we printed something, but there is no need
   to also print the location part of the message.
   An example is the catch/throw messages, which
   don't require a location appended to the end.
   PRINT_NOTHING: We have done some printing and we don't need any 
   further info to be printed.  */

enum print_stop_action
bpstat_print (bpstat *bs, target_waitkind kind)
{
  enum print_stop_action val;

  /* Maybe another breakpoint in the chain caused us to stop.
     (Currently all watchpoints go on the bpstat whether hit or not.
     That probably could (should) be changed, provided care is taken
     with respect to bpstat_explains_signal).  */
  for (; bs; bs = bs->next)
    {
      val = print_bp_stop_message (bs);
      if (val == PRINT_SRC_ONLY 
	  || val == PRINT_SRC_AND_LOC
	  || val == PRINT_NOTHING)
	return val;
    }

  /* If we had hit a shared library event breakpoint,
     print_bp_stop_message would print out this message.  If we hit an
     OS-level shared library event, do the same thing.  */
  if (kind == TARGET_WAITKIND_LOADED)
    {
      print_solib_event (false);
      return PRINT_NOTHING;
    }

  /* We reached the end of the chain, or we got a null BS to start
     with and nothing was printed.  */
  return PRINT_UNKNOWN;
}

/* Evaluate the boolean expression EXP and return the result.  */

static bool
breakpoint_cond_eval (expression *exp)
{
  scoped_value_mark mark;
  return value_true (exp->evaluate ());
}

/* Allocate a new bpstat.  Link it to the FIFO list by BS_LINK_POINTER.  */

bpstat::bpstat (struct bp_location *bl, bpstat ***bs_link_pointer)
  : next (NULL),
    bp_location_at (bp_location_ref_ptr::new_reference (bl)),
    breakpoint_at (bl->owner),
    commands (NULL),
    print (0),
    stop (0),
    print_it (print_it_normal)
{
  **bs_link_pointer = this;
  *bs_link_pointer = &next;
}

bpstat::bpstat ()
  : next (NULL),
    breakpoint_at (NULL),
    commands (NULL),
    print (0),
    stop (0),
    print_it (print_it_normal)
{
}

/* The target has stopped with waitstatus WS.  Check if any hardware
   watchpoints have triggered, according to the target.  */

int
watchpoints_triggered (const target_waitstatus &ws)
{
  bool stopped_by_watchpoint = target_stopped_by_watchpoint ();
  CORE_ADDR addr;

  if (!stopped_by_watchpoint)
    {
      /* We were not stopped by a watchpoint.  Mark all watchpoints
	 as not triggered.  */
      for (breakpoint &b : all_breakpoints ())
	if (is_hardware_watchpoint (&b))
	  {
	    watchpoint &w = gdb::checked_static_cast<watchpoint &> (b);

	    w.watchpoint_triggered = watch_triggered_no;
	  }

      return 0;
    }

  if (!target_stopped_data_address (current_inferior ()->top_target (), &addr))
    {
      /* We were stopped by a watchpoint, but we don't know where.
	 Mark all watchpoints as unknown.  */
      for (breakpoint &b : all_breakpoints ())
	if (is_hardware_watchpoint (&b))
	  {
	    watchpoint &w = gdb::checked_static_cast<watchpoint &> (b);

	    w.watchpoint_triggered = watch_triggered_unknown;
	  }

      return 1;
    }

  /* The target could report the data address.  Mark watchpoints
     affected by this data address as triggered, and all others as not
     triggered.  */

  for (breakpoint &b : all_breakpoints ())
    if (is_hardware_watchpoint (&b))
      {
	watchpoint &w = gdb::checked_static_cast<watchpoint &> (b);

	w.watchpoint_triggered = watch_triggered_no;
	for (bp_location &loc : b.locations ())
	  {
	    if (is_masked_watchpoint (&b))
	      {
		CORE_ADDR newaddr = addr & w.hw_wp_mask;
		CORE_ADDR start = loc.address & w.hw_wp_mask;

		if (newaddr == start)
		  {
		    w.watchpoint_triggered = watch_triggered_yes;
		    break;
		  }
	      }
	    /* Exact match not required.  Within range is sufficient.  */
	    else if (target_watchpoint_addr_within_range
		       (current_inferior ()->top_target (), addr, loc.address,
			loc.length))
	      {
		w.watchpoint_triggered = watch_triggered_yes;
		break;
	      }
	  }
      }

  return 1;
}

/* Possible return values for watchpoint_check.  */
enum wp_check_result
  {
    /* The watchpoint has been deleted.  */
    WP_DELETED = 1,

    /* The value has changed.  */
    WP_VALUE_CHANGED = 2,

    /* The value has not changed.  */
    WP_VALUE_NOT_CHANGED = 3,

    /* Ignore this watchpoint, no matter if the value changed or not.  */
    WP_IGNORE = 4,
  };

#define BP_TEMPFLAG 1
#define BP_HARDWAREFLAG 2

/* Evaluate watchpoint condition expression and check if its value
   changed.  */

static wp_check_result
watchpoint_check (bpstat *bs)
{
  frame_info_ptr fr;
  bool within_current_scope;

  /* BS is built from an existing struct breakpoint.  */
  gdb_assert (bs->breakpoint_at != NULL);
  watchpoint *b = gdb::checked_static_cast<watchpoint *> (bs->breakpoint_at);

  /* If this is a local watchpoint, we only want to check if the
     watchpoint frame is in scope if the current thread is the thread
     that was used to create the watchpoint.  */
  if (!watchpoint_in_thread_scope (b))
    return WP_IGNORE;

  if (b->exp_valid_block == NULL)
    within_current_scope = true;
  else
    {
      frame_info_ptr frame = get_current_frame ();
      struct gdbarch *frame_arch = get_frame_arch (frame);
      CORE_ADDR frame_pc = get_frame_pc (frame);

      /* stack_frame_destroyed_p() returns a non-zero value if we're
	 still in the function but the stack frame has already been
	 invalidated.  Since we can't rely on the values of local
	 variables after the stack has been destroyed, we are treating
	 the watchpoint in that state as `not changed' without further
	 checking.  Don't mark watchpoints as changed if the current
	 frame is in an epilogue - even if they are in some other
	 frame, our view of the stack is likely to be wrong and
	 frame_find_by_id could error out.  */
      if (gdbarch_stack_frame_destroyed_p (frame_arch, frame_pc))
	return WP_IGNORE;

      fr = frame_find_by_id (b->watchpoint_frame);
      within_current_scope = (fr != NULL);

      /* If we've gotten confused in the unwinder, we might have
	 returned a frame that can't describe this variable.  */
      if (within_current_scope)
	{
	  struct symbol *function;

	  function = get_frame_function (fr);
	  if (function == NULL
	      || !function->value_block ()->contains (b->exp_valid_block))
	    within_current_scope = false;
	}

      if (within_current_scope)
	/* If we end up stopping, the current frame will get selected
	   in normal_stop.  So this call to select_frame won't affect
	   the user.  */
	select_frame (fr);
    }

  if (within_current_scope)
    {
      /* We use value_{,free_to_}mark because it could be a *long*
	 time before we return to the command level and call
	 free_all_values.  We can't call free_all_values because we
	 might be in the middle of evaluating a function call.  */

      struct value *mark;
      struct value *new_val;

      if (is_masked_watchpoint (b))
	/* Since we don't know the exact trigger address (from
	   stopped_data_address), just tell the user we've triggered
	   a mask watchpoint.  */
	return WP_VALUE_CHANGED;

      mark = value_mark ();
      fetch_subexp_value (b->exp.get (), b->exp->op.get (), &new_val,
			  NULL, NULL, false);

      if (b->val_bitsize != 0)
	new_val = extract_bitfield_from_watchpoint_value (b, new_val);

      /* We use value_equal_contents instead of value_equal because
	 the latter coerces an array to a pointer, thus comparing just
	 the address of the array instead of its contents.  This is
	 not what we want.  */
      if ((b->val != NULL) != (new_val != NULL)
	  || (b->val != NULL && !value_equal_contents (b->val.get (),
						       new_val)))
	{
	  bs->old_val = b->val;
	  b->val = release_value (new_val);
	  b->val_valid = true;
	  if (new_val != NULL)
	    value_free_to_mark (mark);
	  return WP_VALUE_CHANGED;
	}
      else
	{
	  /* Nothing changed.  */
	  value_free_to_mark (mark);
	  return WP_VALUE_NOT_CHANGED;
	}
    }
  else
    {
      /* This seems like the only logical thing to do because
	 if we temporarily ignored the watchpoint, then when
	 we reenter the block in which it is valid it contains
	 garbage (in the case of a function, it may have two
	 garbage values, one before and one after the prologue).
	 So we can't even detect the first assignment to it and
	 watch after that (since the garbage may or may not equal
	 the first value assigned).  */
      /* We print all the stop information in
	 breakpointprint_it, but in this case, by the time we
	 call breakpoint->print_it this bp will be deleted
	 already.  So we have no choice but print the information
	 here.  */

      SWITCH_THRU_ALL_UIS ()
	{
	  struct ui_out *uiout = current_uiout;

	  if (uiout->is_mi_like_p ())
	    uiout->field_string
	      ("reason", async_reason_lookup (EXEC_ASYNC_WATCHPOINT_SCOPE));
	  uiout->message ("\nWatchpoint %pF deleted because the program has "
			  "left the block in\n"
			  "which its expression is valid.\n",
			  signed_field ("wpnum", b->number));
	}

      /* Make sure the watchpoint's commands aren't executed.  */
      b->commands = NULL;
      watchpoint_del_at_next_stop (b);

      return WP_DELETED;
    }
}

/* Return true if it looks like target has stopped due to hitting
   breakpoint location BL.  This function does not check if we should
   stop, only if BL explains the stop.  */

static bool
bpstat_check_location (const struct bp_location *bl,
		       const address_space *aspace, CORE_ADDR bp_addr,
		       const target_waitstatus &ws)
{
  struct breakpoint *b = bl->owner;

  /* BL is from an existing breakpoint.  */
  gdb_assert (b != NULL);

  return b->breakpoint_hit (bl, aspace, bp_addr, ws);
}

/* Determine if the watched values have actually changed, and we
   should stop.  If not, set BS->stop to false.  */

static void
bpstat_check_watchpoint (bpstat *bs)
{
  const struct bp_location *bl;

  /* BS is built for existing struct breakpoint.  */
  bl = bs->bp_location_at.get ();
  gdb_assert (bl != NULL);
  watchpoint *b = gdb::checked_static_cast<watchpoint *> (bs->breakpoint_at);

  bool must_check_value = false;

  if (b->type == bp_watchpoint)
    /* For a software watchpoint, we must always check the
       watched value.  */
    must_check_value = true;
  else if (b->watchpoint_triggered == watch_triggered_yes)
    /* We have a hardware watchpoint (read, write, or access)
       and the target earlier reported an address watched by
       this watchpoint.  */
    must_check_value = true;
  else if (b->watchpoint_triggered == watch_triggered_unknown
	   && b->type == bp_hardware_watchpoint)
    /* We were stopped by a hardware watchpoint, but the target could
       not report the data address.  We must check the watchpoint's
       value.  Access and read watchpoints are out of luck; without
       a data address, we can't figure it out.  */
    must_check_value = true;

  if (must_check_value)
    {
      wp_check_result e;

      try
	{
	  e = watchpoint_check (bs);
	}
      catch (const gdb_exception_error &ex)
	{
	  exception_fprintf (gdb_stderr, ex,
			     "Error evaluating expression "
			     "for watchpoint %d\n",
			     b->number);

	  SWITCH_THRU_ALL_UIS ()
	    {
	      gdb_printf (_("Watchpoint %d deleted.\n"),
			  b->number);
	    }
	  watchpoint_del_at_next_stop (b);
	  e = WP_DELETED;
	}

      switch (e)
	{
	case WP_DELETED:
	  /* We've already printed what needs to be printed.  */
	  bs->print_it = print_it_done;
	  /* Stop.  */
	  break;
	case WP_IGNORE:
	  bs->print_it = print_it_noop;
	  bs->stop = false;
	  break;
	case WP_VALUE_CHANGED:
	  if (b->type == bp_read_watchpoint)
	    {
	      /* There are two cases to consider here:

		 1. We're watching the triggered memory for reads.
		 In that case, trust the target, and always report
		 the watchpoint hit to the user.  Even though
		 reads don't cause value changes, the value may
		 have changed since the last time it was read, and
		 since we're not trapping writes, we will not see
		 those, and as such we should ignore our notion of
		 old value.

		 2. We're watching the triggered memory for both
		 reads and writes.  There are two ways this may
		 happen:

		 2.1. This is a target that can't break on data
		 reads only, but can break on accesses (reads or
		 writes), such as e.g., x86.  We detect this case
		 at the time we try to insert read watchpoints.

		 2.2. Otherwise, the target supports read
		 watchpoints, but, the user set an access or write
		 watchpoint watching the same memory as this read
		 watchpoint.

		 If we're watching memory writes as well as reads,
		 ignore watchpoint hits when we find that the
		 value hasn't changed, as reads don't cause
		 changes.  This still gives false positives when
		 the program writes the same value to memory as
		 what there was already in memory (we will confuse
		 it for a read), but it's much better than
		 nothing.  */

	      int other_write_watchpoint = 0;

	      if (bl->watchpoint_type == hw_read)
		{
		  for (breakpoint &other_b : all_breakpoints ())
		    if (other_b.type == bp_hardware_watchpoint
			|| other_b.type == bp_access_watchpoint)
		      {
			watchpoint &other_w =
			  gdb::checked_static_cast<watchpoint &> (other_b);

			if (other_w.watchpoint_triggered
			    == watch_triggered_yes)
			  {
			    other_write_watchpoint = 1;
			    break;
			  }
		      }
		}

	      if (other_write_watchpoint
		  || bl->watchpoint_type == hw_access)
		{
		  /* We're watching the same memory for writes,
		     and the value changed since the last time we
		     updated it, so this trap must be for a write.
		     Ignore it.  */
		  bs->print_it = print_it_noop;
		  bs->stop = false;
		}
	    }
	  break;
	case WP_VALUE_NOT_CHANGED:
	  if (b->type == bp_hardware_watchpoint
	      || b->type == bp_watchpoint)
	    {
	      /* Don't stop: write watchpoints shouldn't fire if
		 the value hasn't changed.  */
	      bs->print_it = print_it_noop;
	      bs->stop = false;
	    }
	  /* Stop.  */
	  break;
	default:
	  /* Can't happen.  */
	  break;
	}
    }
  else	/* !must_check_value */
    {
      /* This is a case where some watchpoint(s) triggered, but
	 not at the address of this watchpoint, or else no
	 watchpoint triggered after all.  So don't print
	 anything for this watchpoint.  */
      bs->print_it = print_it_noop;
      bs->stop = false;
    }
}

/* For breakpoints that are currently marked as telling gdb to stop,
   check conditions (condition proper, frame, thread and ignore count)
   of breakpoint referred to by BS.  If we should not stop for this
   breakpoint, set BS->stop to 0.  */

static void
bpstat_check_breakpoint_conditions (bpstat *bs, thread_info *thread)
{
  INFRUN_SCOPED_DEBUG_ENTER_EXIT;

  const struct bp_location *bl;
  struct breakpoint *b;
  /* Assume stop.  */
  bool condition_result = true;
  struct expression *cond;

  gdb_assert (bs->stop);

  /* BS is built for existing struct breakpoint.  */
  bl = bs->bp_location_at.get ();
  gdb_assert (bl != NULL);
  b = bs->breakpoint_at;
  gdb_assert (b != NULL);

  infrun_debug_printf ("thread = %s, breakpoint %d.%d",
		       thread->ptid.to_string ().c_str (),
		       b->number, find_loc_num_by_location (bl));

  /* Even if the target evaluated the condition on its end and notified GDB, we
     need to do so again since GDB does not know if we stopped due to a
     breakpoint or a single step breakpoint.  */

  if (frame_id_p (b->frame_id)
      && b->frame_id != get_stack_frame_id (get_current_frame ()))
    {
      infrun_debug_printf ("incorrect frame %s not %s, not stopping",
			   get_stack_frame_id (get_current_frame ()).to_string ().c_str (),
			   b->frame_id.to_string ().c_str ());
      bs->stop = false;
      return;
    }

  /* If this is a thread/task-specific breakpoint, don't waste cpu
     evaluating the condition if this isn't the specified
     thread/task.  */
  if ((b->thread != -1 && b->thread != thread->global_num)
      || (b->inferior != -1 && b->inferior != thread->inf->num)
      || (b->task != -1 && b->task != ada_get_task_number (thread)))
    {
      infrun_debug_printf ("incorrect thread or task, not stopping");
      bs->stop = false;
      return;
    }

  /* Evaluate extension language breakpoints that have a "stop" method
     implemented.  */
  bs->stop = breakpoint_ext_lang_cond_says_stop (b);

  if (is_watchpoint (b))
    {
      watchpoint *w = gdb::checked_static_cast<watchpoint *> (b);

      cond = w->cond_exp.get ();
    }
  else
    cond = bl->cond.get ();

  if (cond != nullptr && b->disposition != disp_del_at_next_stop)
    {
      bool within_current_scope = true;

      /* We use scoped_value_mark because it could be a long time
	 before we return to the command level and call
	 free_all_values.  We can't call free_all_values because we
	 might be in the middle of evaluating a function call.  */
      scoped_value_mark mark;

      watchpoint *w = nullptr;
      if (is_watchpoint (b))
	w = gdb::checked_static_cast<watchpoint *> (b);

      /* Need to select the frame, with all that implies so that
	 the conditions will have the right context.  Because we
	 use the frame, we will not see an inlined function's
	 variables when we arrive at a breakpoint at the start
	 of the inlined function; the current frame will be the
	 call site.  */
      if (w == NULL || w->cond_exp_valid_block == NULL)
	select_frame (get_current_frame ());
      else
	{
	  frame_info_ptr frame;

	  /* For local watchpoint expressions, which particular
	     instance of a local is being watched matters, so we
	     keep track of the frame to evaluate the expression
	     in.  To evaluate the condition however, it doesn't
	     really matter which instantiation of the function
	     where the condition makes sense triggers the
	     watchpoint.  This allows an expression like "watch
	     global if q > 10" set in `func', catch writes to
	     global on all threads that call `func', or catch
	     writes on all recursive calls of `func' by a single
	     thread.  We simply always evaluate the condition in
	     the innermost frame that's executing where it makes
	     sense to evaluate the condition.  It seems
	     intuitive.  */
	  frame = block_innermost_frame (w->cond_exp_valid_block);
	  if (frame != NULL)
	    select_frame (frame);
	  else
	    within_current_scope = false;
	}
      if (within_current_scope)
	{
	  try
	    {
	      condition_result = breakpoint_cond_eval (cond);
	    }
	  catch (const gdb_exception_error &ex)
	    {
	      int locno = bpstat_locno (bs);
	      if (locno != 0)
		exception_fprintf
		  (gdb_stderr, ex,
		   "Error in testing condition for breakpoint %d.%d:\n",
		   b->number, locno);
	      else
		exception_fprintf
		  (gdb_stderr, ex,
		   "Error in testing condition for breakpoint %d:\n",
		   b->number);
	    }
	}
      else
	{
	  warning (_("Watchpoint condition cannot be tested "
		     "in the current scope"));
	  /* If we failed to set the right context for this
	     watchpoint, unconditionally report it.  */
	}
      /* FIXME-someday, should give breakpoint #.  */
    }

  if (cond != nullptr && !condition_result)
    {
      infrun_debug_printf ("condition_result = false, not stopping");
      bs->stop = false;
      return;
    }
  else if (b->ignore_count > 0)
    {
      infrun_debug_printf ("ignore count %d, not stopping",
			   b->ignore_count);
      b->ignore_count--;
      bs->stop = false;
      /* Increase the hit count even though we don't stop.  */
      ++(b->hit_count);
      notify_breakpoint_modified (b);
      return;
    }

  if (bs->stop)
    infrun_debug_printf ("stopping at this breakpoint");
  else
    infrun_debug_printf ("not stopping at this breakpoint");
}

/* Returns true if we need to track moribund locations of LOC's type
   on the current target.  */

static bool
need_moribund_for_location_type (const struct bp_location *loc)
{
  return ((loc->loc_type == bp_loc_software_breakpoint
	   && !target_supports_stopped_by_sw_breakpoint ())
	  || (loc->loc_type == bp_loc_hardware_breakpoint
	      && !target_supports_stopped_by_hw_breakpoint ()));
}

/* See breakpoint.h.  */

bpstat *
build_bpstat_chain (const address_space *aspace, CORE_ADDR bp_addr,
		    const target_waitstatus &ws)
{
  bpstat *bs_head = nullptr, **bs_link = &bs_head;

  for (breakpoint &b : all_breakpoints ())
    {
      if (!breakpoint_enabled (&b))
	continue;

      for (bp_location &bl : b.locations ())
	{
	  /* For hardware watchpoints, we look only at the first
	     location.  The watchpoint_check function will work on the
	     entire expression, not the individual locations.  For
	     read watchpoints, the watchpoints_triggered function has
	     checked all locations already.  */
	  if (b.type == bp_hardware_watchpoint && &bl != &b.first_loc ())
	    break;

	  if (!bl.enabled || bl.disabled_by_cond || bl.shlib_disabled)
	    continue;

	  if (!bpstat_check_location (&bl, aspace, bp_addr, ws))
	    continue;

	  /* Come here if it's a watchpoint, or if the break address
	     matches.  */

	  bpstat *bs = new bpstat (&bl, &bs_link);	/* Alloc a bpstat to
							   explain stop.  */

	  /* Assume we stop.  Should we find a watchpoint that is not
	     actually triggered, or if the condition of the breakpoint
	     evaluates as false, we'll reset 'stop' to 0.  */
	  bs->stop = true;
	  bs->print = true;

	  /* If this is a scope breakpoint, mark the associated
	     watchpoint as triggered so that we will handle the
	     out-of-scope event.  We'll get to the watchpoint next
	     iteration.  */
	  if (b.type == bp_watchpoint_scope && b.related_breakpoint != &b)
	    {
	      watchpoint *w
		= gdb::checked_static_cast<watchpoint *> (b.related_breakpoint);

	      w->watchpoint_triggered = watch_triggered_yes;
	    }
	}
    }

  /* Check if a moribund breakpoint explains the stop.  */
  if (!target_supports_stopped_by_sw_breakpoint ()
      || !target_supports_stopped_by_hw_breakpoint ())
    {
      for (bp_location *loc : moribund_locations)
	{
	  if (breakpoint_location_address_match (loc, aspace, bp_addr)
	      && need_moribund_for_location_type (loc))
	    {
	      bpstat *bs = new bpstat (loc, &bs_link);
	      /* For hits of moribund locations, we should just proceed.  */
	      bs->stop = false;
	      bs->print = false;
	      bs->print_it = print_it_noop;
	    }
	}
    }

  return bs_head;
}

/* See breakpoint.h.  */

bpstat *
bpstat_stop_status (const address_space *aspace,
		    CORE_ADDR bp_addr, thread_info *thread,
		    const target_waitstatus &ws,
		    bpstat *stop_chain)
{
  struct breakpoint *b = NULL;
  /* First item of allocated bpstat's.  */
  bpstat *bs_head = stop_chain;
  bpstat *bs;
  int need_remove_insert;
  int removed_any;

  /* First, build the bpstat chain with locations that explain a
     target stop, while being careful to not set the target running,
     as that may invalidate locations (in particular watchpoint
     locations are recreated).  Resuming will happen here with
     breakpoint conditions or watchpoint expressions that include
     inferior function calls.  */
  if (bs_head == NULL)
    bs_head = build_bpstat_chain (aspace, bp_addr, ws);

  /* A bit of special processing for shlib breakpoints.  We need to
     process solib loading here, so that the lists of loaded and
     unloaded libraries are correct before we handle "catch load" and
     "catch unload".  */
  for (bs = bs_head; bs != NULL; bs = bs->next)
    {
      if (bs->breakpoint_at && bs->breakpoint_at->type == bp_shlib_event)
	{
	  handle_solib_event ();
	  break;
	}
    }

  /* Now go through the locations that caused the target to stop, and
     check whether we're interested in reporting this stop to higher
     layers, or whether we should resume the target transparently.  */

  removed_any = 0;

  for (bs = bs_head; bs != NULL; bs = bs->next)
    {
      if (!bs->stop)
	continue;

      b = bs->breakpoint_at;
      b->check_status (bs);
      if (bs->stop)
	{
	  bpstat_check_breakpoint_conditions (bs, thread);

	  if (bs->stop)
	    {
	      ++(b->hit_count);

	      /* We will stop here.  */
	      if (b->disposition == disp_disable)
		{
		  --(b->enable_count);
		  if (b->enable_count <= 0)
		    b->enable_state = bp_disabled;
		  removed_any = 1;
		}
	      notify_breakpoint_modified (b);
	      if (b->silent)
		bs->print = false;
	      bs->commands = b->commands;
	      if (command_line_is_silent (bs->commands
					  ? bs->commands.get () : NULL))
		bs->print = false;

	      b->after_condition_true (bs);
	    }

	}

      /* Print nothing for this entry if we don't stop or don't
	 print.  */
      if (!bs->stop || !bs->print)
	bs->print_it = print_it_noop;
    }

  /* If we aren't stopping, the value of some hardware watchpoint may
     not have changed, but the intermediate memory locations we are
     watching may have.  Don't bother if we're stopping; this will get
     done later.  */
  need_remove_insert = 0;
  if (! bpstat_causes_stop (bs_head))
    for (bs = bs_head; bs != NULL; bs = bs->next)
      if (!bs->stop
	  && bs->breakpoint_at
	  && is_hardware_watchpoint (bs->breakpoint_at))
	{
	  watchpoint *w
	    = gdb::checked_static_cast<watchpoint *> (bs->breakpoint_at);

	  update_watchpoint (w, false /* don't reparse.  */);
	  need_remove_insert = 1;
	}

  if (need_remove_insert)
    update_global_location_list (UGLL_MAY_INSERT);
  else if (removed_any)
    update_global_location_list (UGLL_DONT_INSERT);

  return bs_head;
}

/* See breakpoint.h.  */

bpstat *
bpstat_stop_status_nowatch (const address_space *aspace, CORE_ADDR bp_addr,
			    thread_info *thread, const target_waitstatus &ws)
{
  gdb_assert (!target_stopped_by_watchpoint ());

  /* Clear all watchpoints' 'watchpoint_triggered' value from a
     previous stop to avoid confusing bpstat_stop_status.  */
  watchpoints_triggered (ws);

  return bpstat_stop_status (aspace, bp_addr, thread, ws);
}

static void
handle_jit_event (CORE_ADDR address)
{
  struct gdbarch *gdbarch;

  infrun_debug_printf ("handling bp_jit_event");

  /* Switch terminal for any messages produced by
     breakpoint_re_set.  */
  target_terminal::ours_for_output ();

  gdbarch = get_frame_arch (get_current_frame ());
  /* This event is caused by a breakpoint set in `jit_breakpoint_re_set`,
     thus it is expected that its objectfile can be found through
     minimal symbol lookup.  If it doesn't work (and assert fails), it
     most likely means that `jit_breakpoint_re_set` was changes and this
     function needs to be updated too.  */
  bound_minimal_symbol jit_bp_sym = lookup_minimal_symbol_by_pc (address);
  gdb_assert (jit_bp_sym.objfile != nullptr);
  objfile *objfile = jit_bp_sym.objfile;
  if (objfile->separate_debug_objfile_backlink)
    objfile = objfile->separate_debug_objfile_backlink;
  jit_event_handler (gdbarch, objfile);

  target_terminal::inferior ();
}

/* Prepare WHAT final decision for infrun.  */

/* Decide what infrun needs to do with this bpstat.  */

struct bpstat_what
bpstat_what (bpstat *bs_head)
{
  struct bpstat_what retval;
  bpstat *bs;

  retval.main_action = BPSTAT_WHAT_KEEP_CHECKING;
  retval.call_dummy = STOP_NONE;
  retval.is_longjmp = false;

  for (bs = bs_head; bs != NULL; bs = bs->next)
    {
      /* Extract this BS's action.  After processing each BS, we check
	 if its action overrides all we've seem so far.  */
      enum bpstat_what_main_action this_action = BPSTAT_WHAT_KEEP_CHECKING;
      enum bptype bptype;

      if (bs->breakpoint_at == NULL)
	{
	  /* I suspect this can happen if it was a momentary
	     breakpoint which has since been deleted.  */
	  bptype = bp_none;
	}
      else
	bptype = bs->breakpoint_at->type;

      switch (bptype)
	{
	case bp_none:
	  break;
	case bp_breakpoint:
	case bp_hardware_breakpoint:
	case bp_single_step:
	case bp_until:
	case bp_finish:
	case bp_shlib_event:
	  if (bs->stop)
	    {
	      if (bs->print)
		this_action = BPSTAT_WHAT_STOP_NOISY;
	      else
		this_action = BPSTAT_WHAT_STOP_SILENT;
	    }
	  else
	    this_action = BPSTAT_WHAT_SINGLE;
	  break;
	case bp_watchpoint:
	case bp_hardware_watchpoint:
	case bp_read_watchpoint:
	case bp_access_watchpoint:
	  if (bs->stop)
	    {
	      if (bs->print)
		this_action = BPSTAT_WHAT_STOP_NOISY;
	      else
		this_action = BPSTAT_WHAT_STOP_SILENT;
	    }
	  else
	    {
	      /* There was a watchpoint, but we're not stopping.
		 This requires no further action.  */
	    }
	  break;
	case bp_longjmp:
	case bp_longjmp_call_dummy:
	case bp_exception:
	  if (bs->stop)
	    {
	      this_action = BPSTAT_WHAT_SET_LONGJMP_RESUME;
	      retval.is_longjmp = bptype != bp_exception;
	    }
	  else
	    this_action = BPSTAT_WHAT_SINGLE;
	  break;
	case bp_longjmp_resume:
	case bp_exception_resume:
	  if (bs->stop)
	    {
	      this_action = BPSTAT_WHAT_CLEAR_LONGJMP_RESUME;
	      retval.is_longjmp = bptype == bp_longjmp_resume;
	    }
	  else
	    this_action = BPSTAT_WHAT_SINGLE;
	  break;
	case bp_step_resume:
	  if (bs->stop)
	    this_action = BPSTAT_WHAT_STEP_RESUME;
	  else
	    {
	      /* It is for the wrong frame.  */
	      this_action = BPSTAT_WHAT_SINGLE;
	    }
	  break;
	case bp_hp_step_resume:
	  if (bs->stop)
	    this_action = BPSTAT_WHAT_HP_STEP_RESUME;
	  else
	    {
	      /* It is for the wrong frame.  */
	      this_action = BPSTAT_WHAT_SINGLE;
	    }
	  break;
	case bp_watchpoint_scope:
	case bp_thread_event:
	case bp_overlay_event:
	case bp_longjmp_master:
	case bp_std_terminate_master:
	case bp_exception_master:
	  this_action = BPSTAT_WHAT_SINGLE;
	  break;
	case bp_catchpoint:
	  if (bs->stop)
	    {
	      if (bs->print)
		this_action = BPSTAT_WHAT_STOP_NOISY;
	      else
		this_action = BPSTAT_WHAT_STOP_SILENT;
	    }
	  else
	    {
	      /* Some catchpoints are implemented with breakpoints.
		 For those, we need to step over the breakpoint.  */
	      if (bs->bp_location_at->loc_type == bp_loc_software_breakpoint
		  || bs->bp_location_at->loc_type == bp_loc_hardware_breakpoint)
		this_action = BPSTAT_WHAT_SINGLE;
	    }
	  break;
	case bp_jit_event:
	  this_action = BPSTAT_WHAT_SINGLE;
	  break;
	case bp_call_dummy:
	  /* Make sure the action is stop (silent or noisy),
	     so infrun.c pops the dummy frame.  */
	  retval.call_dummy = STOP_STACK_DUMMY;
	  this_action = BPSTAT_WHAT_STOP_SILENT;
	  break;
	case bp_std_terminate:
	  /* Make sure the action is stop (silent or noisy),
	     so infrun.c pops the dummy frame.  */
	  retval.call_dummy = STOP_STD_TERMINATE;
	  this_action = BPSTAT_WHAT_STOP_SILENT;
	  break;
	case bp_tracepoint:
	case bp_fast_tracepoint:
	case bp_static_tracepoint:
	case bp_static_marker_tracepoint:
	  /* Tracepoint hits should not be reported back to GDB, and
	     if one got through somehow, it should have been filtered
	     out already.  */
	  internal_error (_("bpstat_what: tracepoint encountered"));
	  break;
	case bp_gnu_ifunc_resolver:
	  /* Step over it (and insert bp_gnu_ifunc_resolver_return).  */
	  this_action = BPSTAT_WHAT_SINGLE;
	  break;
	case bp_gnu_ifunc_resolver_return:
	  /* The breakpoint will be removed, execution will restart from the
	     PC of the former breakpoint.  */
	  this_action = BPSTAT_WHAT_KEEP_CHECKING;
	  break;

	case bp_dprintf:
	  if (bs->stop)
	    this_action = BPSTAT_WHAT_STOP_SILENT;
	  else
	    this_action = BPSTAT_WHAT_SINGLE;
	  break;

	default:
	  internal_error (_("bpstat_what: unhandled bptype %d"), (int) bptype);
	}

      retval.main_action = std::max (retval.main_action, this_action);
    }

  return retval;
}

void
bpstat_run_callbacks (bpstat *bs_head)
{
  bpstat *bs;

  for (bs = bs_head; bs != NULL; bs = bs->next)
    {
      struct breakpoint *b = bs->breakpoint_at;

      if (b == NULL)
	continue;
      switch (b->type)
	{
	case bp_jit_event:
	  handle_jit_event (bs->bp_location_at->address);
	  break;
	case bp_gnu_ifunc_resolver:
	  gnu_ifunc_resolver_stop
	    (gdb::checked_static_cast<code_breakpoint *> (b));
	  break;
	case bp_gnu_ifunc_resolver_return:
	  gnu_ifunc_resolver_return_stop
	    (gdb::checked_static_cast<code_breakpoint *> (b));
	  break;
	}
    }
}

/* See breakpoint.h.  */

bool
bpstat_should_step ()
{
  for (breakpoint &b : all_breakpoints ())
    if (breakpoint_enabled (&b)
	&& b.type == bp_watchpoint
	&& b.has_locations ())
      return true;

  return false;
}

/* See breakpoint.h.  */

bool
bpstat_causes_stop (bpstat *bs)
{
  for (; bs != NULL; bs = bs->next)
    if (bs->stop)
      return true;

  return false;
}



/* Compute a number of spaces suitable to indent the next line
   so it starts at the position corresponding to the table column
   named COL_NAME in the currently active table of UIOUT.  */

static int
wrap_indent_at_field (struct ui_out *uiout, const char *col_name)
{
  int i, total_width, width, align;
  const char *text;

  total_width = 0;
  for (i = 1; uiout->query_table_field (i, &width, &align, &text); i++)
    {
      if (strcmp (text, col_name) == 0)
	return total_width;

      total_width += width + 1;
    }

  return 0;
}

/* Determine if the locations of this breakpoint will have their conditions
   evaluated by the target, host or a mix of both.  Returns the following:

    "host": Host evals condition.
    "host or target": Host or Target evals condition.
    "target": Target evals condition.
*/

static const char *
bp_condition_evaluator (const breakpoint *b)
{
  char host_evals = 0;
  char target_evals = 0;

  if (!b)
    return NULL;

  if (!is_breakpoint (b))
    return NULL;

  if (gdb_evaluates_breakpoint_condition_p ()
      || !target_supports_evaluation_of_breakpoint_conditions ())
    return condition_evaluation_host;

  for (bp_location &bl : b->locations ())
    {
      if (bl.cond_bytecode)
	target_evals++;
      else
	host_evals++;
    }

  if (host_evals && target_evals)
    return condition_evaluation_both;
  else if (target_evals)
    return condition_evaluation_target;
  else
    return condition_evaluation_host;
}

/* Determine the breakpoint location's condition evaluator.  This is
   similar to bp_condition_evaluator, but for locations.  */

static const char *
bp_location_condition_evaluator (const struct bp_location *bl)
{
  if (bl && !is_breakpoint (bl->owner))
    return NULL;

  if (gdb_evaluates_breakpoint_condition_p ()
      || !target_supports_evaluation_of_breakpoint_conditions ())
    return condition_evaluation_host;

  if (bl && bl->cond_bytecode)
    return condition_evaluation_target;
  else
    return condition_evaluation_host;
}

/* Print the LOC location out of the list of B->LOC locations.  */

static void
print_breakpoint_location (const breakpoint *b, const bp_location *loc)
{
  struct ui_out *uiout = current_uiout;

  scoped_restore_current_program_space restore_pspace;

  if (loc != NULL && loc->shlib_disabled)
    loc = NULL;

  if (loc != NULL)
    set_current_program_space (loc->pspace);

  if (b->display_canonical)
    uiout->field_string ("what", b->locspec->to_string ());
  else if (loc && loc->symtab)
    {
      const struct symbol *sym = loc->symbol;

      if (sym)
	{
	  uiout->text ("in ");
	  uiout->field_string ("func", sym->print_name (),
			       function_name_style.style ());
	  uiout->text (" ");
	  uiout->wrap_hint (wrap_indent_at_field (uiout, "what"));
	  uiout->text ("at ");
	}
      uiout->field_string ("file",
			   symtab_to_filename_for_display (loc->symtab),
			   file_name_style.style ());
      uiout->text (":");

      if (uiout->is_mi_like_p ())
	uiout->field_string ("fullname", symtab_to_fullname (loc->symtab));
      
      uiout->field_signed ("line", loc->line_number);
    }
  else if (loc)
    {
      string_file stb;

      print_address_symbolic (loc->gdbarch, loc->address, &stb,
			      demangle, "");
      uiout->field_stream ("at", stb);
    }
  else
    {
      uiout->field_string ("pending", b->locspec->to_string ());
      /* If extra_string is available, it could be holding a condition
	 or dprintf arguments.  In either case, make sure it is printed,
	 too, but only for non-MI streams.  */
      if (!uiout->is_mi_like_p () && b->extra_string != NULL)
	{
	  if (b->type == bp_dprintf)
	    uiout->text (",");
	  else
	    uiout->text (" ");
	  uiout->text (b->extra_string.get ());
	}
    }

  if (loc && is_breakpoint (b)
      && breakpoint_condition_evaluation_mode () == condition_evaluation_target
      && bp_condition_evaluator (b) == condition_evaluation_both)
    {
      uiout->text (" (");
      uiout->field_string ("evaluated-by",
			   bp_location_condition_evaluator (loc));
      uiout->text (")");
    }
}

static const char *
bptype_string (enum bptype type)
{
  struct ep_type_description
    {
      enum bptype type;
      const char *description;
    };
  static struct ep_type_description bptypes[] =
  {
    {bp_none, "?deleted?"},
    {bp_breakpoint, "breakpoint"},
    {bp_hardware_breakpoint, "hw breakpoint"},
    {bp_single_step, "sw single-step"},
    {bp_until, "until"},
    {bp_finish, "finish"},
    {bp_watchpoint, "watchpoint"},
    {bp_hardware_watchpoint, "hw watchpoint"},
    {bp_read_watchpoint, "read watchpoint"},
    {bp_access_watchpoint, "acc watchpoint"},
    {bp_longjmp, "longjmp"},
    {bp_longjmp_resume, "longjmp resume"},
    {bp_longjmp_call_dummy, "longjmp for call dummy"},
    {bp_exception, "exception"},
    {bp_exception_resume, "exception resume"},
    {bp_step_resume, "step resume"},
    {bp_hp_step_resume, "high-priority step resume"},
    {bp_watchpoint_scope, "watchpoint scope"},
    {bp_call_dummy, "call dummy"},
    {bp_std_terminate, "std::terminate"},
    {bp_shlib_event, "shlib events"},
    {bp_thread_event, "thread events"},
    {bp_overlay_event, "overlay events"},
    {bp_longjmp_master, "longjmp master"},
    {bp_std_terminate_master, "std::terminate master"},
    {bp_exception_master, "exception master"},
    {bp_catchpoint, "catchpoint"},
    {bp_tracepoint, "tracepoint"},
    {bp_fast_tracepoint, "fast tracepoint"},
    {bp_static_tracepoint, "static tracepoint"},
    {bp_static_marker_tracepoint, "static marker tracepoint"},
    {bp_dprintf, "dprintf"},
    {bp_jit_event, "jit events"},
    {bp_gnu_ifunc_resolver, "STT_GNU_IFUNC resolver"},
    {bp_gnu_ifunc_resolver_return, "STT_GNU_IFUNC resolver return"},
  };

  if (((int) type >= (sizeof (bptypes) / sizeof (bptypes[0])))
      || ((int) type != bptypes[(int) type].type))
    internal_error (_("bptypes table does not describe type #%d."),
		    (int) type);

  return bptypes[(int) type].description;
}

/* For MI, output a field named 'thread-groups' with a list as the value.
   For CLI, prefix the list with the string 'inf'. */

static void
output_thread_groups (struct ui_out *uiout,
		      const char *field_name,
		      const std::vector<int> &inf_nums,
		      int mi_only)
{
  int is_mi = uiout->is_mi_like_p ();

  /* For backward compatibility, don't display inferiors in CLI unless
     there are several.  Always display them for MI. */
  if (!is_mi && mi_only)
    return;

  ui_out_emit_list list_emitter (uiout, field_name);

  for (size_t i = 0; i < inf_nums.size (); i++)
    {
      if (is_mi)
	{
	  char mi_group[10];

	  xsnprintf (mi_group, sizeof (mi_group), "i%d", inf_nums[i]);
	  uiout->field_string (NULL, mi_group);
	}
      else
	{
	  if (i == 0)
	    uiout->text (" inf ");
	  else
	    uiout->text (", ");
	
	  uiout->text (plongest (inf_nums[i]));
	}
    }
}

/* See breakpoint.h. */

bool fix_breakpoint_script_output_globally = false;

/* Print B to gdb_stdout.  If RAW_LOC, print raw breakpoint locations
   instead of going via breakpoint_ops::print_one.  This makes "maint
   info breakpoints" show the software breakpoint locations of
   catchpoints, which are considered internal implementation
   detail.  Returns true if RAW_LOC is false and if the breakpoint's
   print_one method did something; false otherwise.  */

static bool
print_one_breakpoint_location (struct breakpoint *b,
			       struct bp_location *loc,
			       int loc_number,
			       const bp_location **last_loc,
			       int allflag, bool raw_loc)
{
  struct command_line *l;
  static char bpenables[] = "nynny";

  struct ui_out *uiout = current_uiout;
  bool header_of_multiple = false;
  bool part_of_multiple = (loc != NULL);
  struct value_print_options opts;

  get_user_print_options (&opts);

  gdb_assert (!loc || loc_number != 0);
  /* See comment in print_one_breakpoint concerning treatment of
     breakpoints with single disabled location.  */
  if (loc == NULL 
      && (b->has_locations ()
	  && (b->has_multiple_locations ()
	      || !b->first_loc ().enabled || b->first_loc ().disabled_by_cond)))
    header_of_multiple = true;

  if (loc == NULL && b->has_locations ())
    loc = &b->first_loc ();

  annotate_record ();

  /* 1 */
  annotate_field (0);
  if (part_of_multiple)
    uiout->field_fmt ("number", "%d.%d", b->number, loc_number);
  else
    uiout->field_signed ("number", b->number);

  /* 2 */
  annotate_field (1);
  if (part_of_multiple)
    uiout->field_skip ("type");
  else
    uiout->field_string ("type", bptype_string (b->type));

  /* 3 */
  annotate_field (2);
  if (part_of_multiple)
    uiout->field_skip ("disp");
  else
    uiout->field_string ("disp", bpdisp_text (b->disposition));

  /* 4 */
  annotate_field (3);
  if (part_of_multiple)
    {
      /* For locations that are disabled because of an invalid
	 condition, display "N*" on the CLI, where "*" refers to a
	 footnote below the table.  For MI, simply display a "N"
	 without a footnote.  On the CLI, for enabled locations whose
	 breakpoint is disabled, display "y-".  */
      auto get_enable_state = [uiout, loc] () -> const char *
	{
	  if (uiout->is_mi_like_p ())
	    {
	      if (loc->disabled_by_cond)
		return "N";
	      else if (!loc->enabled)
		return "n";
	      else
		return "y";
	    }
	  else
	    {
	      if (loc->disabled_by_cond)
		return "N*";
	      else if (!loc->enabled)
		return "n";
	      else if (!breakpoint_enabled (loc->owner))
		return "y-";
	      else
		return "y";
	    }
	};
      uiout->field_string ("enabled", get_enable_state ());
    }
  else
    uiout->field_fmt ("enabled", "%c", bpenables[(int) b->enable_state]);

  /* 5 and 6 */
  bool result = false;
  if (!raw_loc && b->print_one (last_loc))
    result = true;
  else
    {
      if (is_watchpoint (b))
	{
	  watchpoint *w = gdb::checked_static_cast<watchpoint *> (b);

	  /* Field 4, the address, is omitted (which makes the columns
	     not line up too nicely with the headers, but the effect
	     is relatively readable).  */
	  if (opts.addressprint)
	    uiout->field_skip ("addr");
	  annotate_field (5);
	  uiout->field_string ("what", w->exp_string.get ());
	}
      else if (!is_catchpoint (b) || is_exception_catchpoint (b)
	       || is_ada_exception_catchpoint (b))
	{
	  if (opts.addressprint)
	    {
	      annotate_field (4);
	      if (header_of_multiple)
		uiout->field_string ("addr", "<MULTIPLE>",
				     metadata_style.style ());
	      else if (!b->has_locations () || loc->shlib_disabled)
		uiout->field_string ("addr", "<PENDING>",
				     metadata_style.style ());
	      else
		uiout->field_core_addr ("addr",
					loc->gdbarch, loc->address);
	    }
	  annotate_field (5);
	  if (!header_of_multiple)
	    print_breakpoint_location (b, loc);
	  if (b->has_locations ())
	    *last_loc = &b->first_loc ();
	}
    }

  if (loc != NULL && !header_of_multiple)
    {
      std::vector<int> inf_nums;
      int mi_only = 1;

      for (inferior *inf : all_inferiors ())
	{
	  if (inf->pspace == loc->pspace)
	    inf_nums.push_back (inf->num);
	}

      /* For backward compatibility, don't display inferiors in CLI unless
	 there are several.  Always display for MI. */
      if (allflag
	  || (!gdbarch_has_global_breakpoints (current_inferior ()->arch ())
	      && (program_spaces.size () > 1
		  || number_of_inferiors () > 1)
	      /* LOC is for existing B, it cannot be in
		 moribund_locations and thus having NULL OWNER.  */
	      && loc->owner->type != bp_catchpoint))
	mi_only = 0;
      output_thread_groups (uiout, "thread-groups", inf_nums, mi_only);
    }

  /* In the MI output, each location of a thread or task specific
     breakpoint includes the relevant thread or task ID.  This is done for
     backwards compatibility reasons.

     For the CLI output, the thread/task information is printed on a
     separate line, see the 'stop only in thread' and 'stop only in task'
     output below.  */
  if (part_of_multiple && uiout->is_mi_like_p ())
    {
      if (b->thread != -1)
	uiout->field_signed ("thread", b->thread);
      else if (b->task != -1)
	uiout->field_signed ("task", b->task);
      else if (b->inferior != -1)
	uiout->field_signed ("inferior", b->inferior);
    }

  uiout->text ("\n");

  if (!part_of_multiple)
    b->print_one_detail (uiout);

  if (part_of_multiple && frame_id_p (b->frame_id))
    {
      annotate_field (6);
      uiout->text ("\tstop only in stack frame at ");
      /* FIXME: cagney/2002-12-01: Shouldn't be poking around inside
	 the frame ID.  */
      uiout->field_core_addr ("frame",
			      b->gdbarch, b->frame_id.stack_addr);
      uiout->text ("\n");
    }
  
  if (!part_of_multiple && b->cond_string)
    {
      annotate_field (7);
      if (is_tracepoint (b))
	uiout->text ("\ttrace only if ");
      else
	uiout->text ("\tstop only if ");
      uiout->field_string ("cond", b->cond_string.get ());

      /* Print whether the target is doing the breakpoint's condition
	 evaluation.  If GDB is doing the evaluation, don't print anything.  */
      if (is_breakpoint (b)
	  && breakpoint_condition_evaluation_mode ()
	  == condition_evaluation_target)
	{
	  uiout->message (" (%pF evals)",
			  string_field ("evaluated-by",
					bp_condition_evaluator (b)));
	}
      uiout->text ("\n");
    }

  if (!part_of_multiple && b->thread != -1)
    {
      /* FIXME should make an annotation for this.  */
      uiout->text ("\tstop only in thread ");
      if (uiout->is_mi_like_p ())
	uiout->field_signed ("thread", b->thread);
      else
	{
	  struct thread_info *thr = find_thread_global_id (b->thread);

	  uiout->field_string ("thread", print_thread_id (thr));
	}
      uiout->text ("\n");
    }

  if (!part_of_multiple && b->task != -1)
    {
      uiout->text ("\tstop only in task ");
      uiout->field_signed ("task", b->task);
      uiout->text ("\n");
    }

  if (!part_of_multiple && b->inferior != -1)
    {
      uiout->text ("\tstop only in inferior ");
      uiout->field_signed ("inferior", b->inferior);
      uiout->text ("\n");
    }

  if (!part_of_multiple)
    {
      if (b->hit_count)
	{
	  /* FIXME should make an annotation for this.  */
	  if (is_catchpoint (b))
	    uiout->text ("\tcatchpoint");
	  else if (is_tracepoint (b))
	    uiout->text ("\ttracepoint");
	  else
	    uiout->text ("\tbreakpoint");
	  uiout->text (" already hit ");
	  uiout->field_signed ("times", b->hit_count);
	  if (b->hit_count == 1)
	    uiout->text (" time\n");
	  else
	    uiout->text (" times\n");
	}
      else
	{
	  /* Output the count also if it is zero, but only if this is mi.  */
	  if (uiout->is_mi_like_p ())
	    uiout->field_signed ("times", b->hit_count);
	}
    }

  if (!part_of_multiple && b->ignore_count)
    {
      annotate_field (8);
      uiout->message ("\tignore next %pF hits\n",
		      signed_field ("ignore", b->ignore_count));
    }

  /* Note that an enable count of 1 corresponds to "enable once"
     behavior, which is reported by the combination of enablement and
     disposition, so we don't need to mention it here.  */
  if (!part_of_multiple && b->enable_count > 1)
    {
      annotate_field (8);
      uiout->text ("\tdisable after ");
      /* Tweak the wording to clarify that ignore and enable counts
	 are distinct, and have additive effect.  */
      if (b->ignore_count)
	uiout->text ("additional ");
      else
	uiout->text ("next ");
      uiout->field_signed ("enable", b->enable_count);
      uiout->text (" hits\n");
    }

  if (!part_of_multiple && is_tracepoint (b))
    {
      tracepoint *tp = gdb::checked_static_cast<tracepoint *> (b);

      if (tp->traceframe_usage)
	{
	  uiout->text ("\ttrace buffer usage ");
	  uiout->field_signed ("traceframe-usage", tp->traceframe_usage);
	  uiout->text (" bytes\n");
	}
    }

  l = b->commands ? b->commands.get () : NULL;
  if (!part_of_multiple && l)
    {
      annotate_field (9);

      bool use_fixed_output =
	(uiout->test_flags (fix_breakpoint_script_output)
	 || fix_breakpoint_script_output_globally);

      std::optional<ui_out_emit_tuple> tuple_emitter;
      std::optional<ui_out_emit_list> list_emitter;

      if (use_fixed_output)
	list_emitter.emplace (uiout, "script");
      else
	tuple_emitter.emplace (uiout, "script");

      print_command_lines (uiout, l, 4);
    }

  if (is_tracepoint (b))
    {
      tracepoint *t = gdb::checked_static_cast<tracepoint *> (b);

      if (!part_of_multiple && t->pass_count)
	{
	  annotate_field (10);
	  uiout->text ("\tpass count ");
	  uiout->field_signed ("pass", t->pass_count);
	  uiout->text (" \n");
	}

      /* Don't display it when tracepoint or tracepoint location is
	 pending.   */
      if (!header_of_multiple && loc != NULL && !loc->shlib_disabled)
	{
	  annotate_field (11);

	  if (uiout->is_mi_like_p ())
	    uiout->field_string ("installed",
				 loc->inserted ? "y" : "n");
	  else
	    {
	      if (loc->inserted)
		uiout->text ("\t");
	      else
		uiout->text ("\tnot ");
	      uiout->text ("installed on target\n");
	    }
	}
    }

  if (uiout->is_mi_like_p () && !part_of_multiple)
    {
      if (is_watchpoint (b))
	{
	  watchpoint *w = gdb::checked_static_cast<watchpoint *> (b);

	  uiout->field_string ("original-location", w->exp_string.get ());
	}
      else if (b->locspec != nullptr)
	{
	  const char *str = b->locspec->to_string ();
	  if (str != nullptr)
	    uiout->field_string ("original-location", str);
	}
    }

  return result;
}

/* See breakpoint.h. */

bool fix_multi_location_breakpoint_output_globally = false;

static void
print_one_breakpoint (breakpoint *b, const bp_location **last_loc, int allflag)
{
  struct ui_out *uiout = current_uiout;
  bool use_fixed_output
    = (uiout->test_flags (fix_multi_location_breakpoint_output)
       || fix_multi_location_breakpoint_output_globally);

  std::optional<ui_out_emit_tuple> bkpt_tuple_emitter (std::in_place, uiout,
						       "bkpt");
  bool printed = print_one_breakpoint_location (b, NULL, 0, last_loc,
						allflag, false);

  /* The mi2 broken format: the main breakpoint tuple ends here, the locations
     are outside.  */
  if (!use_fixed_output)
    bkpt_tuple_emitter.reset ();

  /* If this breakpoint has custom print function,
     it's already printed.  Otherwise, print individual
     locations, if any.  */
  if (!printed || allflag)
    {
      /* If breakpoint has a single location that is disabled, we
	 print it as if it had several locations, since otherwise it's
	 hard to represent "breakpoint enabled, location disabled"
	 situation.

	 Note that while hardware watchpoints have several locations
	 internally, that's not a property exposed to users.

	 Likewise, while catchpoints may be implemented with
	 breakpoints (e.g., catch throw), that's not a property
	 exposed to users.  We do however display the internal
	 breakpoint locations with "maint info breakpoints".  */
      if (!is_hardware_watchpoint (b)
	  && (!is_catchpoint (b) || is_exception_catchpoint (b)
	      || is_ada_exception_catchpoint (b))
	  && (allflag
	      || (b->has_locations ()
		  && (b->has_multiple_locations ()
		      || !b->first_loc ().enabled
		      || b->first_loc ().disabled_by_cond))))
	{
	  std::optional<ui_out_emit_list> locations_list;

	  /* For MI version <= 2, keep the behavior where GDB outputs an invalid
	     MI record.  For later versions, place breakpoint locations in a
	     list.  */
	  if (uiout->is_mi_like_p () && use_fixed_output)
	    locations_list.emplace (uiout, "locations");

	  int n = 1;
	  for (bp_location &loc : b->locations ())
	    {
	      ui_out_emit_tuple loc_tuple_emitter (uiout, NULL);
	      print_one_breakpoint_location (b, &loc, n, last_loc,
					     allflag, allflag);
	      n++;
	    }
	}
    }
}

static int
breakpoint_address_bits (struct breakpoint *b)
{
  int print_address_bits = 0;

  for (bp_location &loc : b->locations ())
    {
      if (!bl_address_is_meaningful (&loc))
	continue;

      int addr_bit = gdbarch_addr_bit (loc.gdbarch);
      if (addr_bit > print_address_bits)
	print_address_bits = addr_bit;
    }

  return print_address_bits;
}

/* See breakpoint.h.  */

void
print_breakpoint (breakpoint *b)
{
  const bp_location *dummy_loc = nullptr;
  print_one_breakpoint (b, &dummy_loc, 0);
}

/* Return true if this breakpoint was set by the user, false if it is
   internal or momentary.  */

int
user_breakpoint_p (struct breakpoint *b)
{
  return b->number > 0;
}

/* See breakpoint.h.  */

int
pending_breakpoint_p (struct breakpoint *b)
{
  return !b->has_locations ();
}

/* Print information on breakpoints (including watchpoints and tracepoints).

   If non-NULL, BP_NUM_LIST is a list of numbers and number ranges as
   understood by number_or_range_parser.  Only breakpoints included in this
   list are then printed.

   If SHOW_INTERNAL is true, print internal breakpoints.

   If FILTER is non-NULL, call it on each breakpoint and only include the
   ones for which it returns true.

   Return the total number of breakpoints listed.  */

static int
breakpoint_1 (const char *bp_num_list, bool show_internal,
	      bool (*filter) (const struct breakpoint *))
{
  const bp_location *last_loc = nullptr;
  int nr_printable_breakpoints;
  struct value_print_options opts;
  int print_address_bits = 0;
  int print_type_col_width = 14;
  struct ui_out *uiout = current_uiout;
  bool has_disabled_by_cond_location = false;

  get_user_print_options (&opts);

  /* Compute the number of rows in the table, as well as the size
     required for address fields.  */
  nr_printable_breakpoints = 0;
  for (breakpoint &b : all_breakpoints ())
    {
      /* If we have a filter, only list the breakpoints it accepts.  */
      if (filter && !filter (&b))
	continue;

      /* If we have a BP_NUM_LIST string, it is a list of breakpoints to
	 accept.  Skip the others.  */
      if (bp_num_list != NULL && *bp_num_list != '\0')
	{
	  if (show_internal && parse_and_eval_long (bp_num_list) != b.number)
	    continue;
	  if (!show_internal && !number_is_in_list (bp_num_list, b.number))
	    continue;
	}

      if (show_internal || user_breakpoint_p (&b))
	{
	  int addr_bit, type_len;

	  addr_bit = breakpoint_address_bits (&b);
	  if (addr_bit > print_address_bits)
	    print_address_bits = addr_bit;

	  type_len = strlen (bptype_string (b.type));
	  if (type_len > print_type_col_width)
	    print_type_col_width = type_len;

	  nr_printable_breakpoints++;
	}
    }

  {
    ui_out_emit_table table_emitter (uiout,
				     opts.addressprint ? 6 : 5,
				     nr_printable_breakpoints,
				     "BreakpointTable");

    if (nr_printable_breakpoints > 0)
      annotate_breakpoints_headers ();
    if (nr_printable_breakpoints > 0)
      annotate_field (0);
    uiout->table_header (7, ui_left, "number", "Num"); /* 1 */
    if (nr_printable_breakpoints > 0)
      annotate_field (1);
    uiout->table_header (print_type_col_width, ui_left, "type", "Type"); /* 2 */
    if (nr_printable_breakpoints > 0)
      annotate_field (2);
    uiout->table_header (4, ui_left, "disp", "Disp"); /* 3 */
    if (nr_printable_breakpoints > 0)
      annotate_field (3);
    uiout->table_header (3, ui_left, "enabled", "Enb"); /* 4 */
    if (opts.addressprint)
      {
	if (nr_printable_breakpoints > 0)
	  annotate_field (4);
	if (print_address_bits <= 32)
	  uiout->table_header (10, ui_left, "addr", "Address"); /* 5 */
	else
	  uiout->table_header (18, ui_left, "addr", "Address"); /* 5 */
      }
    if (nr_printable_breakpoints > 0)
      annotate_field (5);
    uiout->table_header (40, ui_noalign, "what", "What"); /* 6 */
    uiout->table_body ();
    if (nr_printable_breakpoints > 0)
      annotate_breakpoints_table ();

    for (breakpoint &b : all_breakpoints ())
      {
	QUIT;
	/* If we have a filter, only list the breakpoints it accepts.  */
	if (filter && !filter (&b))
	  continue;

	/* If we have a BP_NUM_LIST string, it is a list of breakpoints to
	   accept.  Skip the others.  */

	if (bp_num_list != NULL && *bp_num_list != '\0')
	  {
	    if (show_internal)	/* maintenance info breakpoint */
	      {
		if (parse_and_eval_long (bp_num_list) != b.number)
		  continue;
	      }
	    else		/* all others */
	      {
		if (!number_is_in_list (bp_num_list, b.number))
		  continue;
	      }
	  }
	/* We only print out user settable breakpoints unless the
	   show_internal is set.  */
	if (show_internal || user_breakpoint_p (&b))
	  {
	    print_one_breakpoint (&b, &last_loc, show_internal);
	    for (bp_location &loc : b.locations ())
	      if (loc.disabled_by_cond)
		has_disabled_by_cond_location = true;
	  }
      }
  }

  if (nr_printable_breakpoints == 0)
    {
      /* If there's a filter, let the caller decide how to report
	 empty list.  */
      if (!filter)
	{
	  if (bp_num_list == NULL || *bp_num_list == '\0')
	    uiout->message ("No breakpoints or watchpoints.\n");
	  else
	    uiout->message ("No breakpoint or watchpoint matching '%s'.\n",
			    bp_num_list);
	}
    }
  else
    {
      if (last_loc && !server_command)
	set_next_address (last_loc->gdbarch, last_loc->address);

      if (has_disabled_by_cond_location && !uiout->is_mi_like_p ())
	uiout->message (_("(*): Breakpoint condition is invalid at this "
			  "location.\n"));
    }

  /* FIXME?  Should this be moved up so that it is only called when
     there have been breakpoints? */
  annotate_breakpoints_table_end ();

  return nr_printable_breakpoints;
}

/* Display the value of default-collect in a way that is generally
   compatible with the breakpoint list.  */

static void
default_collect_info (void)
{
  struct ui_out *uiout = current_uiout;

  /* If it has no value (which is frequently the case), say nothing; a
     message like "No default-collect." gets in user's face when it's
     not wanted.  */
  if (default_collect.empty ())
    return;

  /* The following phrase lines up nicely with per-tracepoint collect
     actions.  */
  uiout->text ("default collect ");
  uiout->field_string ("default-collect", default_collect);
  uiout->text (" \n");
}
  
static void
info_breakpoints_command (const char *args, int from_tty)
{
  breakpoint_1 (args, false, NULL);

  default_collect_info ();
}

static void
info_watchpoints_command (const char *args, int from_tty)
{
  int num_printed = breakpoint_1 (args, false, is_watchpoint);
  struct ui_out *uiout = current_uiout;

  if (num_printed == 0)
    {
      if (args == NULL || *args == '\0')
	uiout->message ("No watchpoints.\n");
      else
	uiout->message ("No watchpoint matching '%s'.\n", args);
    }
}

static void
maintenance_info_breakpoints (const char *args, int from_tty)
{
  breakpoint_1 (args, true, NULL);

  default_collect_info ();
}

static bool
breakpoint_has_pc (struct breakpoint *b,
		   struct program_space *pspace,
		   CORE_ADDR pc, struct obj_section *section)
{
  for (bp_location &bl : b->locations ())
    {
      if (bl.pspace == pspace
	  && bl.address == pc
	  && (!overlay_debugging || bl.section == section))
	return true;
    }
  return false;
}

/* See breakpoint.h.  */

void
describe_other_breakpoints (struct gdbarch *gdbarch,
			    struct program_space *pspace, CORE_ADDR pc,
			    struct obj_section *section, int thread)
{
  int others = 0;

  for (breakpoint &b : all_breakpoints ())
    others += (user_breakpoint_p (&b)
	       && breakpoint_has_pc (&b, pspace, pc, section));

  if (others > 0)
    {
      if (others == 1)
	gdb_printf (_("Note: breakpoint "));
      else /* if (others == ???) */
	gdb_printf (_("Note: breakpoints "));
      for (breakpoint &b : all_breakpoints ())
	if (user_breakpoint_p (&b)
	    && breakpoint_has_pc (&b, pspace, pc, section))
	  {
	    others--;
	    gdb_printf ("%d", b.number);
	    if (b.thread == -1 && thread != -1)
	      gdb_printf (" (all threads)");
	    else if (b.thread != -1)
	      {
		struct thread_info *thr = find_thread_global_id (b.thread);
		gdb_printf (" (thread %s)", print_thread_id (thr));
	      }
	    else if (b.task != -1)
	      gdb_printf (" (task %d)", b.task);
	    gdb_printf ("%s%s ",
			((b.enable_state == bp_disabled
			  || b.enable_state == bp_call_disabled)
			 ? " (disabled)"
			 : ""),
			(others > 1) ? "," 
			: ((others == 1) ? " and" : ""));
	  }
      current_uiout->message (_("also set at pc %ps.\n"),
			      styled_string (address_style.style (),
					     paddress (gdbarch, pc)));
    }
}


/* Return true iff it is meaningful to use the address member of LOC.
   For some breakpoint types, the locations' address members are
   irrelevant and it makes no sense to attempt to compare them to
   other addresses (or use them for any other purpose either).

   More specifically, software watchpoints and catchpoints that are
   not backed by breakpoints always have a zero valued location
   address and we don't want to mark breakpoints of any of these types
   to be a duplicate of an actual breakpoint location at address
   zero.  */

static bool
bl_address_is_meaningful (bp_location *loc)
{
  return loc->loc_type != bp_loc_other;
}

/* Assuming LOC1 and LOC2's owners are hardware watchpoints, returns
   true if LOC1 and LOC2 represent the same watchpoint location.  */

static bool
watchpoint_locations_match (const struct bp_location *loc1,
			    const struct bp_location *loc2)
{
  watchpoint *w1 = gdb::checked_static_cast<watchpoint *> (loc1->owner);
  watchpoint *w2 = gdb::checked_static_cast<watchpoint *> (loc2->owner);

  /* Both of them must exist.  */
  gdb_assert (w1 != NULL);
  gdb_assert (w2 != NULL);

  /* If the target can evaluate the condition expression in hardware,
     then we we need to insert both watchpoints even if they are at
     the same place.  Otherwise the watchpoint will only trigger when
     the condition of whichever watchpoint was inserted evaluates to
     true, not giving a chance for GDB to check the condition of the
     other watchpoint.  */
  if ((w1->cond_exp
       && target_can_accel_watchpoint_condition (loc1->address, 
						 loc1->length,
						 loc1->watchpoint_type,
						 w1->cond_exp.get ()))
      || (w2->cond_exp
	  && target_can_accel_watchpoint_condition (loc2->address, 
						    loc2->length,
						    loc2->watchpoint_type,
						    w2->cond_exp.get ())))
    return false;

  /* Note that this checks the owner's type, not the location's.  In
     case the target does not support read watchpoints, but does
     support access watchpoints, we'll have bp_read_watchpoint
     watchpoints with hw_access locations.  Those should be considered
     duplicates of hw_read locations.  The hw_read locations will
     become hw_access locations later.  */
  return (loc1->owner->type == loc2->owner->type
	  && loc1->pspace->aspace == loc2->pspace->aspace
	  && loc1->address == loc2->address
	  && loc1->length == loc2->length);
}

/* See breakpoint.h.  */

int
breakpoint_address_match (const address_space *aspace1, CORE_ADDR addr1,
			  const address_space *aspace2, CORE_ADDR addr2)
{
  return ((gdbarch_has_global_breakpoints (current_inferior ()->arch ())
	   || aspace1 == aspace2)
	  && addr1 == addr2);
}

/* Returns true if {ASPACE2,ADDR2} falls within the range determined by
   {ASPACE1,ADDR1,LEN1}.  In most targets, this can only be true if ASPACE1
   matches ASPACE2.  On targets that have global breakpoints, the address
   space doesn't really matter.  */

static bool
breakpoint_address_match_range (const address_space *aspace1,
				CORE_ADDR addr1,
				int len1, const address_space *aspace2,
				CORE_ADDR addr2)
{
  return ((gdbarch_has_global_breakpoints (current_inferior ()->arch ())
	   || aspace1 == aspace2)
	  && addr2 >= addr1 && addr2 < addr1 + len1);
}

/* Returns true if {ASPACE,ADDR} matches the breakpoint BL.  BL may be
   a ranged breakpoint.  In most targets, a match happens only if ASPACE
   matches the breakpoint's address space.  On targets that have global
   breakpoints, the address space doesn't really matter.  */

static bool
breakpoint_location_address_match (struct bp_location *bl,
				   const address_space *aspace,
				   CORE_ADDR addr)
{
  return (breakpoint_address_match (bl->pspace->aspace.get (), bl->address,
				    aspace, addr)
	  || (bl->length
	      && breakpoint_address_match_range (bl->pspace->aspace.get (),
						 bl->address, bl->length,
						 aspace, addr)));
}

/* Returns true if the [ADDR,ADDR+LEN) range in ASPACE overlaps
   breakpoint BL.  BL may be a ranged breakpoint.  In most targets, a
   match happens only if ASPACE matches the breakpoint's address
   space.  On targets that have global breakpoints, the address space
   doesn't really matter.  */

static bool
breakpoint_location_address_range_overlap (struct bp_location *bl,
					   const address_space *aspace,
					   CORE_ADDR addr, int len)
{
  if (gdbarch_has_global_breakpoints (current_inferior ()->arch ())
      || bl->pspace->aspace.get () == aspace)
    {
      int bl_len = bl->length != 0 ? bl->length : 1;

      if (mem_ranges_overlap (addr, len, bl->address, bl_len))
	return 1;
    }
  return 0;
}

/* If LOC1 and LOC2's owners are not tracepoints, returns false directly.
   Then, if LOC1 and LOC2 represent the same tracepoint location, returns
   true, otherwise returns false.  */

static bool
tracepoint_locations_match (const struct bp_location *loc1,
			    const struct bp_location *loc2)
{
  if (is_tracepoint (loc1->owner) && is_tracepoint (loc2->owner))
    /* Since tracepoint locations are never duplicated with others', tracepoint
       locations at the same address of different tracepoints are regarded as
       different locations.  */
    return (loc1->address == loc2->address && loc1->owner == loc2->owner);
  else
    return false;
}

/* Assuming LOC1 and LOC2's types' have meaningful target addresses
   (bl_address_is_meaningful), returns true if LOC1 and LOC2 represent
   the same location.  If SW_HW_BPS_MATCH is true, then software
   breakpoint locations and hardware breakpoint locations match,
   otherwise they don't.  */

static bool
breakpoint_locations_match (const struct bp_location *loc1,
			    const struct bp_location *loc2,
			    bool sw_hw_bps_match)
{
  int hw_point1, hw_point2;

  /* Both of them must not be in moribund_locations.  */
  gdb_assert (loc1->owner != NULL);
  gdb_assert (loc2->owner != NULL);

  hw_point1 = is_hardware_watchpoint (loc1->owner);
  hw_point2 = is_hardware_watchpoint (loc2->owner);

  if (hw_point1 != hw_point2)
    return false;
  else if (hw_point1)
    return watchpoint_locations_match (loc1, loc2);
  else if (is_tracepoint (loc1->owner) || is_tracepoint (loc2->owner))
    return tracepoint_locations_match (loc1, loc2);
  else
    /* We compare bp_location.length in order to cover ranged
       breakpoints.  Keep this in sync with
       bp_location_is_less_than.  */
    return (breakpoint_address_match (loc1->pspace->aspace.get (),
				      loc1->address,
				      loc2->pspace->aspace.get (),
				      loc2->address)
	    && (loc1->loc_type == loc2->loc_type || sw_hw_bps_match)
	    && loc1->length == loc2->length);
}

static void
breakpoint_adjustment_warning (CORE_ADDR from_addr, CORE_ADDR to_addr,
			       int bnum, bool have_bnum)
{
  /* The longest string possibly returned by hex_string_custom
     is 50 chars.  These must be at least that big for safety.  */
  char astr1[64];
  char astr2[64];

  strcpy (astr1, hex_string_custom ((unsigned long) from_addr, 8));
  strcpy (astr2, hex_string_custom ((unsigned long) to_addr, 8));
  if (have_bnum)
    warning (_("Breakpoint %d address previously adjusted from %s to %s."),
	     bnum, astr1, astr2);
  else
    warning (_("Breakpoint address adjusted from %s to %s."), astr1, astr2);
}

/* Adjust a breakpoint's address to account for architectural
   constraints on breakpoint placement.  Return the adjusted address.
   Note: Very few targets require this kind of adjustment.  For most
   targets, this function is simply the identity function.  */

static CORE_ADDR
adjust_breakpoint_address (struct gdbarch *gdbarch,
			   CORE_ADDR bpaddr, enum bptype bptype,
			   struct program_space *pspace)
{
  gdb_assert (pspace != nullptr);

  if (bptype == bp_watchpoint
      || bptype == bp_hardware_watchpoint
      || bptype == bp_read_watchpoint
      || bptype == bp_access_watchpoint
      || bptype == bp_catchpoint)
    {
      /* Watchpoints and the various bp_catch_* eventpoints should not
	 have their addresses modified.  */
      return bpaddr;
    }
  else if (bptype == bp_single_step)
    {
      /* Single-step breakpoints should not have their addresses
	 modified.  If there's any architectural constrain that
	 applies to this address, then it should have already been
	 taken into account when the breakpoint was created in the
	 first place.  If we didn't do this, stepping through e.g.,
	 Thumb-2 IT blocks would break.  */
      return bpaddr;
    }
  else
    {
      CORE_ADDR adjusted_bpaddr = bpaddr;

      /* Some targets have architectural constraints on the placement
	 of breakpoint instructions.  Obtain the adjusted address.  */
      if (gdbarch_adjust_breakpoint_address_p (gdbarch))
	{
	  /* Targets that implement this adjustment function will likely
	     inspect either the symbol table, target memory at BPADDR, or
	     even state registers, so ensure a suitable thread (and its
	     associated program space) are currently selected.  */
	  scoped_restore_current_pspace_and_thread restore_pspace_thread;
	  switch_to_program_space_and_thread (pspace);
	  adjusted_bpaddr
	    = gdbarch_adjust_breakpoint_address (gdbarch, bpaddr);
	}

      adjusted_bpaddr
	= gdbarch_remove_non_address_bits (gdbarch, adjusted_bpaddr);

      /* An adjusted breakpoint address can significantly alter
	 a user's expectations.  Print a warning if an adjustment
	 is required.  */
      if (adjusted_bpaddr != bpaddr)
	breakpoint_adjustment_warning (bpaddr, adjusted_bpaddr, 0, false);

      return adjusted_bpaddr;
    }
}

static bp_loc_type
bp_location_from_bp_type (bptype type)
{
  switch (type)
    {
    case bp_breakpoint:
    case bp_single_step:
    case bp_until:
    case bp_finish:
    case bp_longjmp:
    case bp_longjmp_resume:
    case bp_longjmp_call_dummy:
    case bp_exception:
    case bp_exception_resume:
    case bp_step_resume:
    case bp_hp_step_resume:
    case bp_watchpoint_scope:
    case bp_call_dummy:
    case bp_std_terminate:
    case bp_shlib_event:
    case bp_thread_event:
    case bp_overlay_event:
    case bp_jit_event:
    case bp_longjmp_master:
    case bp_std_terminate_master:
    case bp_exception_master:
    case bp_gnu_ifunc_resolver:
    case bp_gnu_ifunc_resolver_return:
    case bp_dprintf:
      return bp_loc_software_breakpoint;

    case bp_hardware_breakpoint:
      return bp_loc_hardware_breakpoint;

    case bp_hardware_watchpoint:
    case bp_read_watchpoint:
    case bp_access_watchpoint:
      return bp_loc_hardware_watchpoint;

    case bp_watchpoint:
      return bp_loc_software_watchpoint;

    case bp_tracepoint:
    case bp_fast_tracepoint:
    case bp_static_tracepoint:
    case bp_static_marker_tracepoint:
      return bp_loc_tracepoint;

    case bp_catchpoint:
      return bp_loc_other;

    default:
      internal_error (_("unknown breakpoint type"));
    }
}

bp_location::bp_location (breakpoint *owner, bp_loc_type type)
{
  this->owner = owner;
  this->cond_bytecode = NULL;
  this->shlib_disabled = 0;
  this->enabled = 1;
  this->disabled_by_cond = false;

  this->loc_type = type;

  if (this->loc_type == bp_loc_software_breakpoint
      || this->loc_type == bp_loc_hardware_breakpoint)
    mark_breakpoint_location_modified (this);

  incref ();
}

bp_location::bp_location (breakpoint *owner)
  : bp_location::bp_location (owner,
			      bp_location_from_bp_type (owner->type))
{
}

/* See breakpoint.h.  */

std::string
bp_location::to_string () const
{
  string_file stb;
  ui_out_redirect_pop redir (current_uiout, &stb);
  print_breakpoint_location (this->owner, this);
  return stb.release ();
}

/* Decrement reference count.  If the reference count reaches 0,
   destroy the bp_location.  Sets *BLP to NULL.  */

static void
decref_bp_location (struct bp_location **blp)
{
  bp_location_ref_policy::decref (*blp);
  *blp = NULL;
}

/* Add breakpoint B at the end of the global breakpoint chain.  */

static breakpoint *
add_to_breakpoint_chain (std::unique_ptr<breakpoint> &&b)
{
  /* Add this breakpoint to the end of the chain so that a list of
     breakpoints will come out in order of increasing numbers.  */

  breakpoint_chain.push_back (*b.release ());

  return &breakpoint_chain.back ();
}

/* Initialize loc->function_name.  */

static void
set_breakpoint_location_function (struct bp_location *loc)
{
  gdb_assert (loc->owner != NULL);

  if (loc->owner->type == bp_breakpoint
      || loc->owner->type == bp_hardware_breakpoint
      || is_tracepoint (loc->owner))
    {
      const char *function_name;

      if (loc->msymbol != NULL
	  && (loc->msymbol->type () == mst_text_gnu_ifunc
	      || loc->msymbol->type () == mst_data_gnu_ifunc))
	{
	  struct breakpoint *b = loc->owner;

	  function_name = loc->msymbol->linkage_name ();

	  if (b->type == bp_breakpoint
	      && b->has_single_location ()
	      && b->related_breakpoint == b)
	    {
	      /* Create only the whole new breakpoint of this type but do not
		 mess more complicated breakpoints with multiple locations.  */
	      b->type = bp_gnu_ifunc_resolver;
	      /* Remember the resolver's address for use by the return
		 breakpoint.  */
	      loc->related_address = loc->address;
	    }
	}
      else
	find_pc_partial_function (loc->address, &function_name, NULL, NULL);

      if (function_name)
	loc->function_name = make_unique_xstrdup (function_name);
    }
}

/* Attempt to determine architecture of location identified by SAL.  */
struct gdbarch *
get_sal_arch (struct symtab_and_line sal)
{
  if (sal.section)
    return sal.section->objfile->arch ();
  if (sal.symtab)
    return sal.symtab->compunit ()->objfile ()->arch ();

  return NULL;
}

/* Call this routine when stepping and nexting to enable a breakpoint
   if we do a longjmp() or 'throw' in TP.  FRAME is the frame which
   initiated the operation.  */

void
set_longjmp_breakpoint (struct thread_info *tp, struct frame_id frame)
{
  int thread = tp->global_num;

  /* To avoid having to rescan all objfile symbols at every step,
     we maintain a list of continually-inserted but always disabled
     longjmp "master" breakpoints.  Here, we simply create momentary
     clones of those and enable them for the requested thread.  */
  for (breakpoint &b : all_breakpoints_safe ())
    if (b.pspace == current_program_space
	&& (b.type == bp_longjmp_master
	    || b.type == bp_exception_master))
      {
	bptype type = b.type == bp_longjmp_master ? bp_longjmp : bp_exception;
	/* longjmp_breakpoint_ops ensures INITIATING_FRAME is cleared again
	   after their removal.  */
	momentary_breakpoint_from_master (&b, type, 1, thread);
      }

  tp->initiating_frame = frame;
}

/* Delete all longjmp breakpoints from THREAD.  */
void
delete_longjmp_breakpoint (int thread)
{
  for (breakpoint &b : all_breakpoints_safe ())
    if (b.type == bp_longjmp || b.type == bp_exception)
      {
	if (b.thread == thread)
	  {
	    gdb_assert (b.inferior == -1);
	    delete_breakpoint (&b);
	  }
      }
}

void
delete_longjmp_breakpoint_at_next_stop (int thread)
{
  for (breakpoint &b : all_breakpoints_safe ())
    if (b.type == bp_longjmp || b.type == bp_exception)
      {
	if (b.thread == thread)
	  {
	    gdb_assert (b.inferior == -1);
	    b.disposition = disp_del_at_next_stop;
	  }
      }
}

/* Place breakpoints of type bp_longjmp_call_dummy to catch longjmp for
   INFERIOR_PTID thread.  Chain them all by RELATED_BREAKPOINT and return
   pointer to any of them.  Return NULL if this system cannot place longjmp
   breakpoints.  */

struct breakpoint *
set_longjmp_breakpoint_for_call_dummy (void)
{
  breakpoint *retval = nullptr;

  for (breakpoint &b : all_breakpoints ())
    if (b.pspace == current_program_space && b.type == bp_longjmp_master)
      {
	int thread = inferior_thread ()->global_num;
	breakpoint *new_b
	  = momentary_breakpoint_from_master (&b, bp_longjmp_call_dummy,
					      1, thread);

	/* Link NEW_B into the chain of RETVAL breakpoints.  */

	gdb_assert (new_b->related_breakpoint == new_b);
	if (retval == NULL)
	  retval = new_b;
	new_b->related_breakpoint = retval;
	while (retval->related_breakpoint != new_b->related_breakpoint)
	  retval = retval->related_breakpoint;
	retval->related_breakpoint = new_b;
      }

  return retval;
}

/* Verify all existing dummy frames and their associated breakpoints for
   TP.  Remove those which can no longer be found in the current frame
   stack.

   If the unwind fails then there is not sufficient information to discard
   dummy frames.  In this case, elide the clean up and the dummy frames will
   be cleaned up next time this function is called from a location where
   unwinding is possible.  */

void
check_longjmp_breakpoint_for_call_dummy (struct thread_info *tp)
{
  /* We would need to delete breakpoints other than the current one while
     iterating, so all_breakpoints_safe is not sufficient to make that safe.
     Save all breakpoints to delete in that set and delete them at the end.  */
  std::unordered_set<breakpoint *> to_delete;

  for (struct breakpoint &b : all_breakpoints ())
    {
      if (b.type == bp_longjmp_call_dummy && b.thread == tp->global_num)
	{
	  gdb_assert (b.inferior == -1);
	  struct breakpoint *dummy_b = b.related_breakpoint;

	  /* Find the bp_call_dummy breakpoint in the list of breakpoints
	     chained off b->related_breakpoint.  */
	  while (dummy_b != &b && dummy_b->type != bp_call_dummy)
	    dummy_b = dummy_b->related_breakpoint;

	  /* If there was no bp_call_dummy breakpoint then there's nothing
	     more to do.  Or, if the dummy frame associated with the
	     bp_call_dummy is still on the stack then we need to leave this
	     bp_call_dummy in place.  */
	  if (dummy_b->type != bp_call_dummy
	      || frame_find_by_id (dummy_b->frame_id) != NULL)
	    continue;

	  /* We didn't find the dummy frame on the stack, this could be
	     because we have longjmp'd to a stack frame that is previous to
	     the dummy frame, or it could be because the stack unwind is
	     broken at some point between the longjmp frame and the dummy
	     frame.

	     Next we figure out why the stack unwind stopped.  If it looks
	     like the unwind is complete then we assume the dummy frame has
	     been jumped over, however, if the unwind stopped for an
	     unexpected reason then we assume the stack unwind is currently
	     broken, and that we will (eventually) return to the dummy
	     frame.

	     It might be tempting to consider using frame_id_inner here, but
	     that is not safe.   There is no guarantee that the stack frames
	     we are looking at here are even on the same stack as the
	     original dummy frame, hence frame_id_inner can't be used.  See
	     the comments on frame_id_inner for more details.  */
	  bool unwind_finished_unexpectedly = false;
	  for (frame_info_ptr fi = get_current_frame (); fi != nullptr; )
	    {
	      frame_info_ptr prev = get_prev_frame (fi);
	      if (prev == nullptr)
		{
		  /* FI is the last stack frame.  Why did this frame not
		     unwind further?  */
		  auto stop_reason = get_frame_unwind_stop_reason (fi);
		  if (stop_reason != UNWIND_NO_REASON
		      && stop_reason != UNWIND_OUTERMOST)
		    unwind_finished_unexpectedly = true;
		}
	      fi = prev;
	    }
	  if (unwind_finished_unexpectedly)
	    continue;

	  dummy_frame_discard (dummy_b->frame_id, tp);

	  for (breakpoint *related_breakpoint = b.related_breakpoint;
	       related_breakpoint != &b;
	       related_breakpoint = related_breakpoint->related_breakpoint)
	    to_delete.insert (b.related_breakpoint);

	  to_delete.insert (&b);
	}
    }

  for (breakpoint *b : to_delete)
    delete_breakpoint (b);
}

void
enable_overlay_breakpoints (void)
{
  for (breakpoint &b : all_breakpoints ())
    if (b.type == bp_overlay_event)
      {
	b.enable_state = bp_enabled;
	update_global_location_list (UGLL_MAY_INSERT);
	overlay_events_enabled = 1;
      }
}

void
disable_overlay_breakpoints (void)
{
  for (breakpoint &b : all_breakpoints ())
    if (b.type == bp_overlay_event)
      {
	b.enable_state = bp_disabled;
	update_global_location_list (UGLL_DONT_INSERT);
	overlay_events_enabled = 0;
      }
}

/* Set an active std::terminate breakpoint for each std::terminate
   master breakpoint.  */
void
set_std_terminate_breakpoint (void)
{
  for (breakpoint &b : all_breakpoints_safe ())
    if (b.pspace == current_program_space
	&& b.type == bp_std_terminate_master)
      {
	momentary_breakpoint_from_master (&b, bp_std_terminate, 1,
					  inferior_thread ()->global_num);
      }
}

/* Delete all the std::terminate breakpoints.  */
void
delete_std_terminate_breakpoint (void)
{
  for (breakpoint &b : all_breakpoints_safe ())
    if (b.type == bp_std_terminate)
      delete_breakpoint (&b);
}

struct breakpoint *
create_thread_event_breakpoint (struct gdbarch *gdbarch, CORE_ADDR address)
{
  struct breakpoint *b;

  b = create_internal_breakpoint (gdbarch, address, bp_thread_event);

  b->enable_state = bp_enabled;
  /* locspec has to be used or breakpoint_re_set will delete me.  */
  b->locspec = new_address_location_spec (b->first_loc ().address, NULL, 0);

  update_global_location_list_nothrow (UGLL_MAY_INSERT);

  return b;
}

struct lang_and_radix
  {
    enum language lang;
    int radix;
  };

/* Create a breakpoint for JIT code registration and unregistration.  */

struct breakpoint *
create_jit_event_breakpoint (struct gdbarch *gdbarch, CORE_ADDR address)
{
  return create_internal_breakpoint (gdbarch, address, bp_jit_event);
}

/* Remove JIT code registration and unregistration breakpoint(s).  */

void
remove_jit_event_breakpoints (void)
{
  for (breakpoint &b : all_breakpoints_safe ())
    if (b.type == bp_jit_event
	&& b.first_loc ().pspace == current_program_space)
      delete_breakpoint (&b);
}

void
remove_solib_event_breakpoints (void)
{
  for (breakpoint &b : all_breakpoints_safe ())
    if (b.type == bp_shlib_event
	&& b.first_loc ().pspace == current_program_space)
      delete_breakpoint (&b);
}

/* See breakpoint.h.  */

void
remove_solib_event_breakpoints_at_next_stop (void)
{
  for (breakpoint &b : all_breakpoints_safe ())
    if (b.type == bp_shlib_event
	&& b.first_loc ().pspace == current_program_space)
      b.disposition = disp_del_at_next_stop;
}

/* Helper for create_solib_event_breakpoint /
   create_and_insert_solib_event_breakpoint.  Allows specifying which
   INSERT_MODE to pass through to update_global_location_list.  */

static struct breakpoint *
create_solib_event_breakpoint_1 (struct gdbarch *gdbarch, CORE_ADDR address,
				 enum ugll_insert_mode insert_mode)
{
  struct breakpoint *b;

  b = create_internal_breakpoint (gdbarch, address, bp_shlib_event);
  update_global_location_list_nothrow (insert_mode);
  return b;
}

struct breakpoint *
create_solib_event_breakpoint (struct gdbarch *gdbarch, CORE_ADDR address)
{
  return create_solib_event_breakpoint_1 (gdbarch, address, UGLL_MAY_INSERT);
}

/* See breakpoint.h.  */

struct breakpoint *
create_and_insert_solib_event_breakpoint (struct gdbarch *gdbarch, CORE_ADDR address)
{
  struct breakpoint *b;

  /* Explicitly tell update_global_location_list to insert
     locations.  */
  b = create_solib_event_breakpoint_1 (gdbarch, address, UGLL_INSERT);
  if (!b->first_loc ().inserted)
    {
      delete_breakpoint (b);
      return NULL;
    }
  return b;
}

/* Disable any breakpoints that are on code in shared libraries.  Only
   apply to enabled breakpoints, disabled ones can just stay disabled.  */

void
disable_breakpoints_in_shlibs (void)
{
  for (bp_location *loc : all_bp_locations ())
    {
      /* ALL_BP_LOCATIONS bp_location has LOC->OWNER always non-NULL.  */
      struct breakpoint *b = loc->owner;

      /* We apply the check to all breakpoints, including disabled for
	 those with loc->duplicate set.  This is so that when breakpoint
	 becomes enabled, or the duplicate is removed, gdb will try to
	 insert all breakpoints.  If we don't set shlib_disabled here,
	 we'll try to insert those breakpoints and fail.  */
      if (((b->type == bp_breakpoint)
	   || (b->type == bp_jit_event)
	   || (b->type == bp_hardware_breakpoint)
	   || (is_tracepoint (b)))
	  && loc->pspace == current_program_space
	  && !loc->shlib_disabled
	  && solib_name_from_address (loc->pspace, loc->address)
	  )
	{
	  loc->shlib_disabled = 1;
	}
    }
}

/* Disable any breakpoints and tracepoints that are in SOLIB upon
   notification of unloaded_shlib.  Only apply to enabled breakpoints,
   disabled ones can just stay disabled.  */

static void
disable_breakpoints_in_unloaded_shlib (program_space *pspace, const shobj &solib)
{
  bool disabled_shlib_breaks = false;

  for (bp_location *loc : all_bp_locations ())
    {
      /* ALL_BP_LOCATIONS bp_location has LOC->OWNER always non-NULL.  */
      struct breakpoint *b = loc->owner;

      if (pspace == loc->pspace
	  && !loc->shlib_disabled
	  && (((b->type == bp_breakpoint
		|| b->type == bp_jit_event
		|| b->type == bp_hardware_breakpoint)
	       && (loc->loc_type == bp_loc_hardware_breakpoint
		   || loc->loc_type == bp_loc_software_breakpoint))
	      || is_tracepoint (b))
	  && solib_contains_address_p (solib, loc->address))
	{
	  loc->shlib_disabled = 1;
	  /* At this point, we cannot rely on remove_breakpoint
	     succeeding so we must mark the breakpoint as not inserted
	     to prevent future errors occurring in remove_breakpoints.  */
	  loc->inserted = 0;

	  /* This may cause duplicate notifications for the same breakpoint.  */
	  notify_breakpoint_modified (b);

	  if (!disabled_shlib_breaks)
	    {
	      target_terminal::ours_for_output ();
	      warning (_("Temporarily disabling breakpoints "
			 "for unloaded shared library \"%s\""),
		       solib.so_name.c_str ());
	    }
	  disabled_shlib_breaks = true;
	}
    }
}

/* Disable any breakpoints and tracepoints in OBJFILE upon
   notification of free_objfile.  Only apply to enabled breakpoints,
   disabled ones can just stay disabled.  */

static void
disable_breakpoints_in_freed_objfile (struct objfile *objfile)
{
  /* OBJF_SHARED|OBJF_USERLOADED objfiles are dynamic modules manually
     managed by the user with add-symbol-file/remove-symbol-file.
     Similarly to how breakpoints in shared libraries are handled in
     response to "nosharedlibrary", mark breakpoints in such modules
     shlib_disabled so they end up uninserted on the next global
     location list update.  Shared libraries not loaded by the user
     aren't handled here -- they're already handled in
     disable_breakpoints_in_unloaded_shlib, called by solib.c's
     solib_unloaded observer.  We skip objfiles that are not
     OBJF_SHARED as those aren't considered dynamic objects (e.g. the
     main objfile).  */
  if ((objfile->flags & OBJF_SHARED) == 0
      || (objfile->flags & OBJF_USERLOADED) == 0)
    return;

  for (breakpoint &b : all_breakpoints ())
    {
      bool bp_modified = false;

      if (!is_breakpoint (&b) && !is_tracepoint (&b))
	continue;

      for (bp_location &loc : b.locations ())
	{
	  CORE_ADDR loc_addr = loc.address;

	  if (loc.loc_type != bp_loc_hardware_breakpoint
	      && loc.loc_type != bp_loc_software_breakpoint)
	    continue;

	  if (loc.shlib_disabled != 0)
	    continue;

	  if (objfile->pspace != loc.pspace)
	    continue;

	  if (loc.loc_type != bp_loc_hardware_breakpoint
	      && loc.loc_type != bp_loc_software_breakpoint)
	    continue;

	  if (is_addr_in_objfile (loc_addr, objfile))
	    {
	      loc.shlib_disabled = 1;
	      /* At this point, we don't know whether the object was
		 unmapped from the inferior or not, so leave the
		 inserted flag alone.  We'll handle failure to
		 uninsert quietly, in case the object was indeed
		 unmapped.  */

	      mark_breakpoint_location_modified (&loc);

	      bp_modified = true;
	    }
	}

      if (bp_modified)
	notify_breakpoint_modified (&b);
    }
}

/* See breakpoint.h.  */

breakpoint::breakpoint (struct gdbarch *gdbarch_, enum bptype bptype,
			bool temp, const char *cond_string_)
  : type (bptype),
    disposition (temp ? disp_del : disp_donttouch),
    gdbarch (gdbarch_),
    language (current_language->la_language),
    input_radix (::input_radix),
    cond_string (cond_string_ != nullptr
		 ? make_unique_xstrdup (cond_string_)
		 : nullptr),
    related_breakpoint (this)
{
}

/* See breakpoint.h.  */

catchpoint::catchpoint (struct gdbarch *gdbarch, bool temp,
			const char *cond_string)
  : breakpoint (gdbarch, bp_catchpoint, temp, cond_string)
{
  add_dummy_location (this, current_program_space);

  pspace = current_program_space;
}

/* Notify interpreters and observers that breakpoint B was created.  */

static void
notify_breakpoint_created (breakpoint *b)
{
  interps_notify_breakpoint_created (b);
  gdb::observers::breakpoint_created.notify (b);
}

breakpoint *
install_breakpoint (int internal, std::unique_ptr<breakpoint> &&arg, int update_gll)
{
  breakpoint *b = add_to_breakpoint_chain (std::move (arg));
  set_breakpoint_number (internal, b);
  if (is_tracepoint (b))
    set_tracepoint_count (breakpoint_count);
  if (!internal)
    mention (b);

  notify_breakpoint_created (b);

  if (update_gll)
    update_global_location_list (UGLL_MAY_INSERT);

  return b;
}

static int
hw_breakpoint_used_count (void)
{
  int i = 0;

  for (breakpoint &b : all_breakpoints ())
    if (b.type == bp_hardware_breakpoint && breakpoint_enabled (&b))
      for (bp_location &bl : b.locations ())
	{
	  /* Special types of hardware breakpoints may use more than
	     one register.  */
	  i += b.resources_needed (&bl);
	}

  return i;
}

/* Returns the resources B would use if it were a hardware
   watchpoint.  */

static int
hw_watchpoint_use_count (struct breakpoint *b)
{
  int i = 0;

  if (!breakpoint_enabled (b))
    return 0;

  for (bp_location &bl : b->locations ())
    {
      /* Special types of hardware watchpoints may use more than
	 one register.  */
      i += b->resources_needed (&bl);
    }

  return i;
}

/* Returns the sum the used resources of all hardware watchpoints of
   type TYPE in the breakpoints list.  Also returns in OTHER_TYPE_USED
   the sum of the used resources of all hardware watchpoints of other
   types _not_ TYPE.  */

static int
hw_watchpoint_used_count_others (struct breakpoint *except,
				 enum bptype type, int *other_type_used)
{
  int i = 0;

  *other_type_used = 0;
  for (breakpoint &b : all_breakpoints ())
    {
      if (&b == except)
	continue;
      if (!breakpoint_enabled (&b))
	continue;

      if (b.type == type)
	i += hw_watchpoint_use_count (&b);
      else if (is_hardware_watchpoint (&b))
	*other_type_used = 1;
    }

  return i;
}

void
disable_watchpoints_before_interactive_call_start (void)
{
  for (breakpoint &b : all_breakpoints ())
    if (is_watchpoint (&b) && breakpoint_enabled (&b))
      {
	b.enable_state = bp_call_disabled;
	update_global_location_list (UGLL_DONT_INSERT);
      }
}

void
enable_watchpoints_after_interactive_call_stop (void)
{
  for (breakpoint &b : all_breakpoints ())
    if (is_watchpoint (&b) && b.enable_state == bp_call_disabled)
      {
	b.enable_state = bp_enabled;
	update_global_location_list (UGLL_MAY_INSERT);
      }
}

void
disable_breakpoints_before_startup (void)
{
  current_program_space->executing_startup = 1;
  update_global_location_list (UGLL_DONT_INSERT);
}

void
enable_breakpoints_after_startup (void)
{
  current_program_space->executing_startup = 0;
  breakpoint_re_set ();
}

/* Allocate a new momentary breakpoint.  */

template<typename... Arg>
static momentary_breakpoint *
new_momentary_breakpoint (struct gdbarch *gdbarch, enum bptype type,
			  Arg&&... args)
{
  if (type == bp_longjmp || type == bp_exception)
    return new longjmp_breakpoint (gdbarch, type,
				   std::forward<Arg> (args)...);
  else
    return new momentary_breakpoint (gdbarch, type,
				     std::forward<Arg> (args)...);
}

/* Set a momentary breakpoint of type TYPE at address specified by
   SAL.  If FRAME_ID is valid, the breakpoint is restricted to that
   frame.  */

breakpoint_up
set_momentary_breakpoint (struct gdbarch *gdbarch, struct symtab_and_line sal,
			  struct frame_id frame_id, enum bptype type)
{
  /* If FRAME_ID is valid, it should be a real frame, not an inlined or
     tail-called one.  */
  gdb_assert (!frame_id_artificial_p (frame_id));

  std::unique_ptr<momentary_breakpoint> b
    (new_momentary_breakpoint (gdbarch, type, sal.pspace, frame_id,
			       inferior_thread ()->global_num));

  b->add_location (sal);

  breakpoint_up bp (add_to_breakpoint_chain (std::move (b)));

  update_global_location_list_nothrow (UGLL_MAY_INSERT);

  return bp;
}

/* Make a momentary breakpoint based on the master breakpoint ORIG.
   The new breakpoint will have type TYPE, use OPS as its
   breakpoint_ops, and will set enabled to LOC_ENABLED.  */

static struct breakpoint *
momentary_breakpoint_from_master (struct breakpoint *orig,
				  enum bptype type,
				  int loc_enabled,
				  int thread)
{
  std::unique_ptr<breakpoint> copy
    (new_momentary_breakpoint (orig->gdbarch, type, orig->pspace,
			       orig->frame_id, thread));
  const bp_location &orig_loc = orig->first_loc ();
  bp_location *copy_loc = copy->allocate_location ();
  copy->add_location (*copy_loc);
  set_breakpoint_location_function (copy_loc);

  copy_loc->gdbarch = orig_loc.gdbarch;
  copy_loc->requested_address = orig_loc.requested_address;
  copy_loc->address = orig_loc.address;
  copy_loc->section = orig_loc.section;
  copy_loc->pspace = orig_loc.pspace;
  copy_loc->probe = orig_loc.probe;
  copy_loc->line_number = orig_loc.line_number;
  copy_loc->symtab = orig_loc.symtab;
  copy_loc->enabled = loc_enabled;

  breakpoint *b = add_to_breakpoint_chain (std::move (copy));
  update_global_location_list_nothrow (UGLL_DONT_INSERT);
  return b;
}

/* Make a deep copy of momentary breakpoint ORIG.  Returns NULL if
   ORIG is NULL.  */

struct breakpoint *
clone_momentary_breakpoint (struct breakpoint *orig)
{
  /* If there's nothing to clone, then return nothing.  */
  if (orig == NULL)
    return NULL;

  return momentary_breakpoint_from_master (orig, orig->type, 0,
					   orig->thread);
}

breakpoint_up
set_momentary_breakpoint_at_pc (struct gdbarch *gdbarch, CORE_ADDR pc,
				enum bptype type)
{
  struct symtab_and_line sal;

  sal = find_pc_line (pc, 0);
  sal.pc = pc;
  sal.section = find_pc_overlay (pc);
  sal.explicit_pc = 1;

  return set_momentary_breakpoint (gdbarch, sal, null_frame_id, type);
}


/* Tell the user we have just set a breakpoint B.  */

static void
mention (const breakpoint *b)
{
  b->print_mention ();
  current_uiout->text ("\n");
}


static bool bp_loc_is_permanent (struct bp_location *loc);

/* Handle "set breakpoint auto-hw on".

   If the explicitly specified breakpoint type is not hardware
   breakpoint, check the memory map to see whether the breakpoint
   address is in read-only memory.

   - location type is not hardware breakpoint, memory is read-only.
   We change the type of the location to hardware breakpoint.

   - location type is hardware breakpoint, memory is read-write.  This
   means we've previously made the location hardware one, but then the
   memory map changed, so we undo.
*/

static void
handle_automatic_hardware_breakpoints (bp_location *bl)
{
  if (automatic_hardware_breakpoints
      && bl->owner->type != bp_hardware_breakpoint
      && (bl->loc_type == bp_loc_software_breakpoint
	  || bl->loc_type == bp_loc_hardware_breakpoint))
    {
      /* When breakpoints are removed, remove_breakpoints will use
	 location types we've just set here, the only possible problem
	 is that memory map has changed during running program, but
	 it's not going to work anyway with current gdb.  */
      mem_region *mr = lookup_mem_region (bl->address);

      if (mr != nullptr)
	{
	  enum bp_loc_type new_type;

	  if (mr->attrib.mode != MEM_RW)
	    new_type = bp_loc_hardware_breakpoint;
	  else
	    new_type = bp_loc_software_breakpoint;

	  if (new_type != bl->loc_type)
	    {
	      static bool said = false;

	      bl->loc_type = new_type;
	      if (!said)
		{
		  gdb_printf (_("Note: automatically using "
				"hardware breakpoints for "
				"read-only addresses.\n"));
		  said = true;
		}
	    }
	}
    }
}

bp_location *
code_breakpoint::add_location (const symtab_and_line &sal)
{
  CORE_ADDR adjusted_address;
  struct gdbarch *loc_gdbarch = get_sal_arch (sal);

  if (loc_gdbarch == NULL)
    loc_gdbarch = gdbarch;

  /* Adjust the breakpoint's address prior to allocating a location.
     Once we call allocate_location(), that mostly uninitialized
     location will be placed on the location chain.  Adjustment of the
     breakpoint may cause target_read_memory() to be called and we do
     not want its scan of the location chain to find a breakpoint and
     location that's only been partially initialized.  */
  adjusted_address = adjust_breakpoint_address (loc_gdbarch,
						sal.pc, type,
						sal.pspace);

  /* Sort the locations by their ADDRESS.  */
  bp_location *new_loc = this->allocate_location ();

  new_loc->requested_address = sal.pc;
  new_loc->address = adjusted_address;
  new_loc->pspace = sal.pspace;
  new_loc->probe.prob = sal.prob;
  new_loc->probe.objfile = sal.objfile;
  gdb_assert (new_loc->pspace != NULL);
  new_loc->section = sal.section;
  new_loc->gdbarch = loc_gdbarch;
  new_loc->line_number = sal.line;
  new_loc->symtab = sal.symtab;
  new_loc->symbol = sal.symbol;
  new_loc->msymbol = sal.msymbol;
  new_loc->objfile = sal.objfile;

  breakpoint::add_location (*new_loc);

  set_breakpoint_location_function (new_loc);

  /* While by definition, permanent breakpoints are already present in the
     code, we don't mark the location as inserted.  Normally one would expect
     that GDB could rely on that breakpoint instruction to stop the program,
     thus removing the need to insert its own breakpoint, except that executing
     the breakpoint instruction can kill the target instead of reporting a
     SIGTRAP.  E.g., on SPARC, when interrupts are disabled, executing the
     instruction resets the CPU, so QEMU 2.0.0 for SPARC correspondingly dies
     with "Trap 0x02 while interrupts disabled, Error state".  Letting the
     breakpoint be inserted normally results in QEMU knowing about the GDB
     breakpoint, and thus trap before the breakpoint instruction is executed.
     (If GDB later needs to continue execution past the permanent breakpoint,
     it manually increments the PC, thus avoiding executing the breakpoint
     instruction.)  */
  if (bp_loc_is_permanent (new_loc))
    new_loc->permanent = 1;

  return new_loc;
}


/* Return true if LOC is pointing to a permanent breakpoint,
   return false otherwise.  */

static bool
bp_loc_is_permanent (struct bp_location *loc)
{
  gdb_assert (loc != NULL);

  /* If we have a non-breakpoint-backed catchpoint or a software
     watchpoint, just return 0.  We should not attempt to read from
     the addresses the locations of these breakpoint types point to.
     gdbarch_program_breakpoint_here_p, below, will attempt to read
     memory.  */
  if (!bl_address_is_meaningful (loc))
    return false;

  scoped_restore_current_pspace_and_thread restore_pspace_thread;
  switch_to_program_space_and_thread (loc->pspace);
  return gdbarch_program_breakpoint_here_p (loc->gdbarch, loc->address);
}

/* Build a command list for the dprintf corresponding to the current
   settings of the dprintf style options.  */

static void
update_dprintf_command_list (struct breakpoint *b)
{
  const char *dprintf_args = b->extra_string.get ();
  gdb::unique_xmalloc_ptr<char> printf_line = nullptr;

  if (!dprintf_args)
    return;

  dprintf_args = skip_spaces (dprintf_args);

  /* Allow a comma, as it may have terminated a location, but don't
     insist on it.  */
  if (*dprintf_args == ',')
    ++dprintf_args;
  dprintf_args = skip_spaces (dprintf_args);

  if (*dprintf_args != '"')
    error (_("Bad format string, missing '\"'."));

  if (strcmp (dprintf_style, dprintf_style_gdb) == 0)
    printf_line = xstrprintf ("printf %s", dprintf_args);
  else if (strcmp (dprintf_style, dprintf_style_call) == 0)
    {
      if (dprintf_function.empty ())
	error (_("No function supplied for dprintf call"));

      if (!dprintf_channel.empty ())
	printf_line = xstrprintf ("call (void) %s (%s,%s)",
				  dprintf_function.c_str (),
				  dprintf_channel.c_str (),
				  dprintf_args);
      else
	printf_line = xstrprintf ("call (void) %s (%s)",
				  dprintf_function.c_str (),
				  dprintf_args);
    }
  else if (strcmp (dprintf_style, dprintf_style_agent) == 0)
    {
      if (target_can_run_breakpoint_commands ())
	printf_line = xstrprintf ("agent-printf %s", dprintf_args);
      else
	{
	  warning (_("Target cannot run dprintf commands, falling back to GDB printf"));
	  printf_line = xstrprintf ("printf %s", dprintf_args);
	}
    }
  else
    internal_error (_("Invalid dprintf style."));

  gdb_assert (printf_line != NULL);

  /* Manufacture a printf sequence.  */
  struct command_line *printf_cmd_line
    = new struct command_line (simple_control, printf_line.release ());
  breakpoint_set_commands (b, counted_command_line (printf_cmd_line,
						    command_lines_deleter ()));
}

/* Update all dprintf commands, making their command lists reflect
   current style settings.  */

static void
update_dprintf_commands (const char *args, int from_tty,
			 struct cmd_list_element *c)
{
  for (breakpoint &b : all_breakpoints ())
    if (b.type == bp_dprintf)
	update_dprintf_command_list (&b);
}

code_breakpoint::code_breakpoint (struct gdbarch *gdbarch_,
				  enum bptype type_,
				  gdb::array_view<const symtab_and_line> sals,
				  location_spec_up &&locspec_,
				  gdb::unique_xmalloc_ptr<char> filter_,
				  gdb::unique_xmalloc_ptr<char> cond_string_,
				  gdb::unique_xmalloc_ptr<char> extra_string_,
				  enum bpdisp disposition_,
				  int thread_, int task_, int inferior_,
				  int ignore_count_,
				  int from_tty,
				  int enabled_, unsigned flags,
				  int display_canonical_)
  : breakpoint (gdbarch_, type_)
{
  int i;

  if (type == bp_hardware_breakpoint)
    {
      int target_resources_ok;

      i = hw_breakpoint_used_count ();
      target_resources_ok =
	target_can_use_hardware_watchpoint (bp_hardware_breakpoint,
					    i + 1, 0);
      if (target_resources_ok == 0)
	error (_("No hardware breakpoint support in the target."));
      else if (target_resources_ok < 0)
	error (_("Hardware breakpoints used exceeds limit."));
    }

  gdb_assert (!sals.empty ());

  /* At most one of thread, task, or inferior can be set on any breakpoint.  */
  gdb_assert (((thread == -1 ? 0 : 1)
	       + (task == -1 ? 0 : 1)
	       + (inferior == -1 ? 0 : 1)) <= 1);

  thread = thread_;
  task = task_;
  inferior = inferior_;

  cond_string = std::move (cond_string_);
  extra_string = std::move (extra_string_);
  ignore_count = ignore_count_;
  enable_state = enabled_ ? bp_enabled : bp_disabled;
  disposition = disposition_;

  if (type == bp_static_tracepoint
      || type == bp_static_marker_tracepoint)
    {
      auto *t = gdb::checked_static_cast<tracepoint *> (this);
      struct static_tracepoint_marker marker;

      if (strace_marker_p (this))
	{
	  /* We already know the marker exists, otherwise, we wouldn't
	     see a sal for it.  */
	  const char *p = &locspec_->to_string ()[3];
	  const char *endp;

	  p = skip_spaces (p);

	  endp = skip_to_space (p);

	  t->static_trace_marker_id.assign (p, endp - p);

	  gdb_printf (_("Probed static tracepoint marker \"%s\"\n"),
		      t->static_trace_marker_id.c_str ());
	}
      else if (target_static_tracepoint_marker_at (sals[0].pc, &marker))
	{
	  t->static_trace_marker_id = std::move (marker.str_id);

	  gdb_printf (_("Probed static tracepoint marker \"%s\"\n"),
		      t->static_trace_marker_id.c_str ());
	}
      else
	warning (_("Couldn't determine the static tracepoint marker to probe"));
    }

  for (const auto &sal : sals)
    {
      if (from_tty)
	{
	  struct gdbarch *loc_gdbarch = get_sal_arch (sal);
	  if (loc_gdbarch == nullptr)
	    loc_gdbarch = gdbarch;

	  describe_other_breakpoints (loc_gdbarch,
				      sal.pspace, sal.pc, sal.section, thread);
	}

      bp_location *new_loc = add_location (sal);
      if ((flags & CREATE_BREAKPOINT_FLAGS_INSERTED) != 0)
	new_loc->inserted = 1;

      /* Do not set breakpoint locations conditions yet.  As locations
	 are inserted, they get sorted based on their addresses.  Let
	 the list stabilize to have reliable location numbers.  */

      /* Dynamic printf requires and uses additional arguments on the
	 command line, otherwise it's an error.  */
      if (type == bp_dprintf)
	{
	  if (extra_string != nullptr)
	    update_dprintf_command_list (this);
	  else
	    error (_("Format string required"));
	}
      else if (extra_string != nullptr)
	error (_("Garbage '%s' at end of command"), extra_string.get ());
    }

  /* The order of the locations is now stable.  Set the location
     condition using the location's number.  */
  int loc_num = 1;
  for (bp_location &bl : locations ())
    {
      if (cond_string != nullptr)
	set_breakpoint_location_condition (cond_string.get (), &bl,
					   number, loc_num);

      ++loc_num;
    }

  display_canonical = display_canonical_;
  if (locspec_ != nullptr)
    locspec = std::move (locspec_);
  else
    locspec = new_address_location_spec (this->first_loc ().address, NULL, 0);
  filter = std::move (filter_);
}

static void
create_breakpoint_sal (struct gdbarch *gdbarch,
		       gdb::array_view<const symtab_and_line> sals,
		       location_spec_up &&locspec,
		       gdb::unique_xmalloc_ptr<char> filter,
		       gdb::unique_xmalloc_ptr<char> cond_string,
		       gdb::unique_xmalloc_ptr<char> extra_string,
		       enum bptype type, enum bpdisp disposition,
		       int thread, int task, int inferior, int ignore_count,
		       int from_tty,
		       int enabled, int internal, unsigned flags,
		       int display_canonical)
{
  std::unique_ptr<code_breakpoint> b
    = new_breakpoint_from_type (gdbarch,
				type,
				sals,
				std::move (locspec),
				std::move (filter),
				std::move (cond_string),
				std::move (extra_string),
				disposition,
				thread, task, inferior, ignore_count,
				from_tty,
				enabled, flags,
				display_canonical);

  install_breakpoint (internal, std::move (b), 0);
}

/* Add SALS.nelts breakpoints to the breakpoint table.  For each
   SALS.sal[i] breakpoint, include the corresponding ADDR_STRING[i]
   value.  COND_STRING, if not NULL, specified the condition to be
   used for all breakpoints.  Essentially the only case where
   SALS.nelts is not 1 is when we set a breakpoint on an overloaded
   function.  In that case, it's still not possible to specify
   separate conditions for different overloaded functions, so
   we take just a single condition string.
   
   NOTE: If the function succeeds, the caller is expected to cleanup
   the arrays ADDR_STRING, COND_STRING, and SALS (but not the
   array contents).  If the function fails (error() is called), the
   caller is expected to cleanups both the ADDR_STRING, COND_STRING,
   COND and SALS arrays and each of those arrays contents.  */

static void
create_breakpoints_sal (struct gdbarch *gdbarch,
			struct linespec_result *canonical,
			gdb::unique_xmalloc_ptr<char> cond_string,
			gdb::unique_xmalloc_ptr<char> extra_string,
			enum bptype type, enum bpdisp disposition,
			int thread, int task, int inferior,
			int ignore_count,
			int from_tty,
			int enabled, int internal, unsigned flags)
{
  if (canonical->pre_expanded)
    gdb_assert (canonical->lsals.size () == 1);

  for (const auto &lsal : canonical->lsals)
    {
      /* Note that 'location' can be NULL in the case of a plain
	 'break', without arguments.  */
      location_spec_up locspec
	= (canonical->locspec != nullptr
	   ? canonical->locspec->clone ()
	   : nullptr);
      gdb::unique_xmalloc_ptr<char> filter_string
	(lsal.canonical != NULL ? xstrdup (lsal.canonical) : NULL);

      create_breakpoint_sal (gdbarch, lsal.sals,
			     std::move (locspec),
			     std::move (filter_string),
			     std::move (cond_string),
			     std::move (extra_string),
			     type, disposition,
			     thread, task, inferior, ignore_count,
			     from_tty, enabled, internal, flags,
			     canonical->special_display);
    }
}

/* Parse LOCSPEC which is assumed to be a SAL specification possibly
   followed by conditionals.  On return, SALS contains an array of SAL
   addresses found.  LOCSPEC points to the end of the SAL (for
   linespec locspecs).

   The array and the line spec strings are allocated on the heap, it is
   the caller's responsibility to free them.  */

static void
parse_breakpoint_sals (location_spec *locspec,
		       struct linespec_result *canonical)
{
  struct symtab_and_line cursal;

  if (locspec->type () == LINESPEC_LOCATION_SPEC)
    {
      const char *spec
	= as_linespec_location_spec (locspec)->spec_string.get ();

      if (spec == NULL)
	{
	  /* The last displayed codepoint, if it's valid, is our default
	     breakpoint address.  */
	  if (last_displayed_sal_is_valid ())
	    {
	      /* Set sal's pspace, pc, symtab, and line to the values
		 corresponding to the last call to print_frame_info.
		 Be sure to reinitialize LINE with NOTCURRENT == 0
		 as the breakpoint line number is inappropriate otherwise.
		 find_pc_line would adjust PC, re-set it back.  */
	      symtab_and_line sal = get_last_displayed_sal ();
	      CORE_ADDR pc = sal.pc;

	      sal = find_pc_line (pc, 0);

	      /* "break" without arguments is equivalent to "break *PC"
		 where PC is the last displayed codepoint's address.  So
		 make sure to set sal.explicit_pc to prevent GDB from
		 trying to expand the list of sals to include all other
		 instances with the same symtab and line.  */
	      sal.pc = pc;
	      sal.explicit_pc = 1;

	      struct linespec_sals lsal;
	      lsal.sals = {sal};
	      lsal.canonical = NULL;

	      canonical->lsals.push_back (std::move (lsal));
	      return;
	    }
	  else
	    error (_("No default breakpoint address now."));
	}
    }

  /* Force almost all breakpoints to be in terms of the
     current_source_symtab (which is decode_line_1's default).
     This should produce the results we want almost all of the
     time while leaving default_breakpoint_* alone.

     ObjC: However, don't match an Objective-C method name which
     may have a '+' or '-' succeeded by a '['.  */
  cursal = get_current_source_symtab_and_line ();
  if (last_displayed_sal_is_valid ())
    {
      const char *spec = NULL;

      if (locspec->type () == LINESPEC_LOCATION_SPEC)
	spec = as_linespec_location_spec (locspec)->spec_string.get ();

      if (!cursal.symtab
	  || (spec != NULL
	      && strchr ("+-", spec[0]) != NULL
	      && spec[1] != '['))
	{
	  decode_line_full (locspec, DECODE_LINE_FUNFIRSTLINE, NULL,
			    get_last_displayed_symtab (),
			    get_last_displayed_line (),
			    canonical, NULL, NULL);
	  return;
	}
    }

  decode_line_full (locspec, DECODE_LINE_FUNFIRSTLINE, NULL,
		    cursal.symtab, cursal.line, canonical, NULL, NULL);
}


/* Convert each SAL into a real PC.  Verify that the PC can be
   inserted as a breakpoint.  If it can't throw an error.  */

static void
breakpoint_sals_to_pc (std::vector<symtab_and_line> &sals)
{    
  for (auto &sal : sals)
    resolve_sal_pc (&sal);
}

/* Fast tracepoints may have restrictions on valid locations.  For
   instance, a fast tracepoint using a jump instead of a trap will
   likely have to overwrite more bytes than a trap would, and so can
   only be placed where the instruction is longer than the jump, or a
   multi-instruction sequence does not have a jump into the middle of
   it, etc.  */

static void
check_fast_tracepoint_sals (struct gdbarch *gdbarch,
			    gdb::array_view<const symtab_and_line> sals)
{
  for (const auto &sal : sals)
    {
      struct gdbarch *sarch;

      sarch = get_sal_arch (sal);
      /* We fall back to GDBARCH if there is no architecture
	 associated with SAL.  */
      if (sarch == NULL)
	sarch = gdbarch;
      std::string msg;
      if (!gdbarch_fast_tracepoint_valid_at (sarch, sal.pc, &msg))
	error (_("May not have a fast tracepoint at %s%s"),
	       paddress (sarch, sal.pc), msg.c_str ());
    }
}

/* Given TOK, a string specification of condition and thread, as accepted
   by the 'break' command, extract the condition string into *COND_STRING.
   If no condition string is found then *COND_STRING is set to nullptr.

   If the breakpoint specification has an associated thread, task, or
   inferior, these are extracted into *THREAD, *TASK, and *INFERIOR
   respectively, otherwise these arguments are set to -1 (for THREAD and
   INFERIOR) or 0 (for TASK).

   PC identifies the context at which the condition should be parsed.  */

static void
find_condition_and_thread (const char *tok, CORE_ADDR pc,
			   gdb::unique_xmalloc_ptr<char> *cond_string,
			   int *thread, int *inferior, int *task,
			   gdb::unique_xmalloc_ptr<char> *rest)
{
  cond_string->reset ();
  *thread = -1;
  *inferior = -1;
  *task = -1;
  rest->reset ();
  bool force = false;

  while (tok && *tok)
    {
      const char *end_tok;
      int toklen;
      const char *cond_start = NULL;
      const char *cond_end = NULL;

      tok = skip_spaces (tok);

      if ((*tok == '"' || *tok == ',') && rest)
	{
	  rest->reset (savestring (tok, strlen (tok)));
	  break;
	}

      end_tok = skip_to_space (tok);

      toklen = end_tok - tok;

      if (toklen >= 1 && strncmp (tok, "if", toklen) == 0)
	{
	  tok = cond_start = end_tok + 1;
	  try
	    {
	      parse_exp_1 (&tok, pc, block_for_pc (pc), 0);
	    }
	  catch (const gdb_exception_error &)
	    {
	      if (!force)
		throw;
	      else
		tok = tok + strlen (tok);
	    }
	  cond_end = tok;
	  cond_string->reset (savestring (cond_start, cond_end - cond_start));
	}
      else if (toklen >= 1 && strncmp (tok, "-force-condition", toklen) == 0)
	{
	  tok = tok + toklen;
	  force = true;
	}
      else if (toklen >= 1 && strncmp (tok, "thread", toklen) == 0)
	{
	  const char *tmptok;
	  struct thread_info *thr;

	  if (*thread != -1)
	    error(_("You can specify only one thread."));

	  if (*task != -1)
	    error (_("You can specify only one of thread or task."));

	  if (*inferior != -1)
	    error (_("You can specify only one of inferior or thread."));

	  tok = end_tok + 1;
	  thr = parse_thread_id (tok, &tmptok);
	  if (tok == tmptok)
	    error (_("Junk after thread keyword."));
	  *thread = thr->global_num;
	  tok = tmptok;
	}
      else if (toklen >= 1 && strncmp (tok, "inferior", toklen) == 0)
	{
	  if (*inferior != -1)
	    error(_("You can specify only one inferior."));

	  if (*task != -1)
	    error (_("You can specify only one of inferior or task."));

	  if (*thread != -1)
	    error (_("You can specify only one of inferior or thread."));

	  char *tmptok;
	  tok = end_tok + 1;
	  *inferior = strtol (tok, &tmptok, 0);
	  if (tok == tmptok)
	    error (_("Junk after inferior keyword."));
	  if (!valid_global_inferior_id (*inferior))
	    error (_("Unknown inferior number %d."), *inferior);
	  tok = tmptok;
	}
      else if (toklen >= 1 && strncmp (tok, "task", toklen) == 0)
	{
	  char *tmptok;

	  if (*task != -1)
	    error(_("You can specify only one task."));

	  if (*thread != -1)
	    error (_("You can specify only one of thread or task."));

	  if (*inferior != -1)
	    error (_("You can specify only one of inferior or task."));

	  tok = end_tok + 1;
	  *task = strtol (tok, &tmptok, 0);
	  if (tok == tmptok)
	    error (_("Junk after task keyword."));
	  if (!valid_task_id (*task))
	    error (_("Unknown task %d."), *task);
	  tok = tmptok;
	}
      else if (rest)
	{
	  rest->reset (savestring (tok, strlen (tok)));
	  break;
	}
      else
	error (_("Junk at end of arguments."));
    }
}

/* Call 'find_condition_and_thread' for each sal in SALS until a parse
   succeeds.  The parsed values are written to COND_STRING, THREAD,
   TASK, and REST.  See the comment of 'find_condition_and_thread'
   for the description of these parameters and INPUT.  */

static void
find_condition_and_thread_for_sals (const std::vector<symtab_and_line> &sals,
				    const char *input,
				    gdb::unique_xmalloc_ptr<char> *cond_string,
				    int *thread, int *inferior, int *task,
				    gdb::unique_xmalloc_ptr<char> *rest)
{
  int num_failures = 0;
  for (auto &sal : sals)
    {
      gdb::unique_xmalloc_ptr<char> cond;
      int thread_id = -1;
      int inferior_id = -1;
      int task_id = -1;
      gdb::unique_xmalloc_ptr<char> remaining;

      /* Here we want to parse 'arg' to separate condition from thread
	 number.  But because parsing happens in a context and the
	 contexts of sals might be different, try each until there is
	 success.  Finding one successful parse is sufficient for our
	 goal.  When setting the breakpoint we'll re-parse the
	 condition in the context of each sal.  */
      try
	{
	  find_condition_and_thread (input, sal.pc, &cond, &thread_id,
				     &inferior_id, &task_id, &remaining);
	  *cond_string = std::move (cond);
	  /* A value of -1 indicates that these fields are unset.  At most
	     one of these fields should be set (to a value other than -1)
	     at this point.  */
	  gdb_assert (((thread_id == -1 ? 1 : 0)
		       + (task_id == -1 ? 1 : 0)
		       + (inferior_id == -1 ? 1 : 0)) >= 2);
	  *thread = thread_id;
	  *inferior = inferior_id;
	  *task = task_id;
	  *rest = std::move (remaining);
	  break;
	}
      catch (const gdb_exception_error &e)
	{
	  num_failures++;
	  /* If no sal remains, do not continue.  */
	  if (num_failures == sals.size ())
	    throw;
	}
    }
}

/* Decode a static tracepoint marker spec.  */

static std::vector<symtab_and_line>
decode_static_tracepoint_spec (const char **arg_p)
{
  const char *p = &(*arg_p)[3];
  const char *endp;

  p = skip_spaces (p);

  endp = skip_to_space (p);

  std::string marker_str (p, endp - p);

  std::vector<static_tracepoint_marker> markers
    = target_static_tracepoint_markers_by_strid (marker_str.c_str ());
  if (markers.empty ())
    error (_("No known static tracepoint marker named %s"),
	   marker_str.c_str ());

  std::vector<symtab_and_line> sals;
  sals.reserve (markers.size ());

  for (const static_tracepoint_marker &marker : markers)
    {
      symtab_and_line sal = find_pc_line (marker.address, 0);
      sal.pc = marker.address;
      sals.push_back (sal);
   }

  *arg_p = endp;
  return sals;
}

/* Returns the breakpoint ops appropriate for use with with LOCATION_TYPE and
   according to IS_TRACEPOINT.  */

static const struct breakpoint_ops *
breakpoint_ops_for_location_spec_type (enum location_spec_type locspec_type,
				       bool is_tracepoint)
{
  if (is_tracepoint)
    {
      if (locspec_type == PROBE_LOCATION_SPEC)
	return &tracepoint_probe_breakpoint_ops;
      else
	return &code_breakpoint_ops;
    }
  else
    {
      if (locspec_type == PROBE_LOCATION_SPEC)
	return &bkpt_probe_breakpoint_ops;
      else
	return &code_breakpoint_ops;
    }
}

/* See breakpoint.h.  */

const struct breakpoint_ops *
breakpoint_ops_for_location_spec (const location_spec *locspec,
				  bool is_tracepoint)
{
  if (locspec != nullptr)
    return (breakpoint_ops_for_location_spec_type
	    (locspec->type (), is_tracepoint));
  return &code_breakpoint_ops;
}

/* See breakpoint.h.  */

int
create_breakpoint (struct gdbarch *gdbarch,
		   location_spec *locspec,
		   const char *cond_string,
		   int thread, int inferior,
		   const char *extra_string,
		   bool force_condition, int parse_extra,
		   int tempflag, enum bptype type_wanted,
		   int ignore_count,
		   enum auto_boolean pending_break_support,
		   const struct breakpoint_ops *ops,
		   int from_tty, int enabled, int internal,
		   unsigned flags)
{
  struct linespec_result canonical;
  bool pending = false;
  int task = -1;
  int prev_bkpt_count = breakpoint_count;

  gdb_assert (thread == -1 || thread > 0);
  gdb_assert (inferior == -1 || inferior > 0);
  gdb_assert (thread == -1 || inferior == -1);

  gdb_assert (ops != NULL);

  /* If extra_string isn't useful, set it to NULL.  */
  if (extra_string != NULL && *extra_string == '\0')
    extra_string = NULL;

  try
    {
      ops->create_sals_from_location_spec (locspec, &canonical);
    }
  catch (const gdb_exception_error &e)
    {
      /* If caller is interested in rc value from parse, set
	 value.  */
      if (e.error == NOT_FOUND_ERROR)
	{
	  /* If pending breakpoint support is turned off, throw
	     error.  */

	  if (pending_break_support == AUTO_BOOLEAN_FALSE)
	    throw;

	  exception_print (gdb_stderr, e);

	  /* If pending breakpoint support is auto query and the user
	     selects no, then simply return the error code.  */
	  if (pending_break_support == AUTO_BOOLEAN_AUTO
	      && !nquery (_("Make %s pending on future shared library load? "),
			  bptype_string (type_wanted)))
	    return 0;

	  /* At this point, either the user was queried about setting
	     a pending breakpoint and selected yes, or pending
	     breakpoint behavior is on and thus a pending breakpoint
	     is defaulted on behalf of the user.  */
	  pending = true;
	}
      else
	throw;
    }

  if (!pending && canonical.lsals.empty ())
    return 0;

  /* Resolve all line numbers to PC's and verify that the addresses
     are ok for the target.  */
  if (!pending)
    {
      for (auto &lsal : canonical.lsals)
	breakpoint_sals_to_pc (lsal.sals);
    }

  /* Fast tracepoints may have additional restrictions on location.  */
  if (!pending && type_wanted == bp_fast_tracepoint)
    {
      for (const auto &lsal : canonical.lsals)
	check_fast_tracepoint_sals (gdbarch, lsal.sals);
    }

  /* Verify that condition can be parsed, before setting any
     breakpoints.  Allocate a separate condition expression for each
     breakpoint.  */
  if (!pending)
    {
      gdb::unique_xmalloc_ptr<char> cond_string_copy;
      gdb::unique_xmalloc_ptr<char> extra_string_copy;

      if (parse_extra)
	{
	  gdb::unique_xmalloc_ptr<char> rest;
	  gdb::unique_xmalloc_ptr<char> cond;

	  const linespec_sals &lsal = canonical.lsals[0];

	  find_condition_and_thread_for_sals (lsal.sals, extra_string,
					      &cond, &thread, &inferior,
					      &task, &rest);
	  cond_string_copy = std::move (cond);
	  extra_string_copy = std::move (rest);
	}
      else
	{
	  if (type_wanted != bp_dprintf
	      && extra_string != NULL && *extra_string != '\0')
		error (_("Garbage '%s' at end of location"), extra_string);

	  /* Check the validity of the condition.  We should error out
	     if the condition is invalid at all of the locations and
	     if it is not forced.  In the PARSE_EXTRA case above, this
	     check is done when parsing the EXTRA_STRING.  */
	  if (cond_string != nullptr && !force_condition)
	    {
	      int num_failures = 0;
	      const linespec_sals &lsal = canonical.lsals[0];
	      for (const auto &sal : lsal.sals)
		{
		  const char *cond = cond_string;
		  try
		    {
		      parse_exp_1 (&cond, sal.pc, block_for_pc (sal.pc), 0);
		      /* One success is sufficient to keep going.  */
		      break;
		    }
		  catch (const gdb_exception_error &)
		    {
		      num_failures++;
		      /* If this is the last sal, error out.  */
		      if (num_failures == lsal.sals.size ())
			throw;
		    }
		}
	    }

	  /* Create a private copy of condition string.  */
	  if (cond_string)
	    cond_string_copy.reset (xstrdup (cond_string));
	  /* Create a private copy of any extra string.  */
	  if (extra_string)
	    extra_string_copy.reset (xstrdup (extra_string));
	}

      ops->create_breakpoints_sal (gdbarch, &canonical,
				   std::move (cond_string_copy),
				   std::move (extra_string_copy),
				   type_wanted,
				   tempflag ? disp_del : disp_donttouch,
				   thread, task, inferior, ignore_count,
				   from_tty, enabled, internal, flags);
    }
  else
    {
      std::unique_ptr <breakpoint> b = new_breakpoint_from_type (gdbarch,
								 type_wanted);
      b->locspec = locspec->clone ();

      if (parse_extra)
	b->cond_string = NULL;
      else
	{
	  /* Create a private copy of condition string.  */
	  b->cond_string.reset (cond_string != NULL
				? xstrdup (cond_string)
				: NULL);
	  b->thread = thread;
	}

      /* Create a private copy of any extra string.  */
      b->extra_string.reset (extra_string != NULL
			     ? xstrdup (extra_string)
			     : NULL);
      b->ignore_count = ignore_count;
      b->disposition = tempflag ? disp_del : disp_donttouch;
      b->condition_not_parsed = 1;
      b->enable_state = enabled ? bp_enabled : bp_disabled;
      if ((type_wanted != bp_breakpoint
	   && type_wanted != bp_hardware_breakpoint) || thread != -1)
	b->pspace = current_program_space;

      install_breakpoint (internal, std::move (b), 0);
    }
  
  if (canonical.lsals.size () > 1)
    {
      warning (_("Multiple breakpoints were set.\nUse the "
		 "\"delete\" command to delete unwanted breakpoints."));
      prev_breakpoint_count = prev_bkpt_count;
    }

  update_global_location_list (UGLL_MAY_INSERT);

  return 1;
}

/* Set a breakpoint.
   ARG is a string describing breakpoint address,
   condition, and thread.
   FLAG specifies if a breakpoint is hardware on,
   and if breakpoint is temporary, using BP_HARDWARE_FLAG
   and BP_TEMPFLAG.  */

static void
break_command_1 (const char *arg, int flag, int from_tty)
{
  int tempflag = flag & BP_TEMPFLAG;
  enum bptype type_wanted = (flag & BP_HARDWAREFLAG
			     ? bp_hardware_breakpoint
			     : bp_breakpoint);

  location_spec_up locspec = string_to_location_spec (&arg, current_language);
  const struct breakpoint_ops *ops
    = breakpoint_ops_for_location_spec (locspec.get (),
					false /* is_tracepoint */);

  create_breakpoint (get_current_arch (),
		     locspec.get (),
		     NULL,
		     -1 /* thread */, -1 /* inferior */,
		     arg, false, 1 /* parse arg */,
		     tempflag, type_wanted,
		     0 /* Ignore count */,
		     pending_break_support,
		     ops,
		     from_tty,
		     1 /* enabled */,
		     0 /* internal */,
		     0);
}

/* Helper function for break_command_1 and disassemble_command.  */

void
resolve_sal_pc (struct symtab_and_line *sal)
{
  CORE_ADDR pc;

  if (sal->pc == 0 && sal->symtab != NULL)
    {
      if (!find_line_pc (sal->symtab, sal->line, &pc))
	error (_("No line %d in file \"%s\"."),
	       sal->line, symtab_to_filename_for_display (sal->symtab));
      sal->pc = pc;

      /* If this SAL corresponds to a breakpoint inserted using a line
	 number, then skip the function prologue if necessary.  */
      if (sal->explicit_line)
	skip_prologue_sal (sal);
    }

  if (sal->section == 0 && sal->symtab != NULL)
    {
      const struct blockvector *bv;
      const struct block *b;
      struct symbol *sym;

      bv = blockvector_for_pc_sect (sal->pc, 0, &b,
				    sal->symtab->compunit ());
      if (bv != NULL)
	{
	  sym = b->linkage_function ();
	  if (sym != NULL)
	    sal->section
	      = sym->obj_section (sal->symtab->compunit ()->objfile ());
	  else
	    {
	      /* It really is worthwhile to have the section, so we'll
		 just have to look harder. This case can be executed
		 if we have line numbers but no functions (as can
		 happen in assembly source).  */

	      scoped_restore_current_pspace_and_thread restore_pspace_thread;
	      switch_to_program_space_and_thread (sal->pspace);

	      bound_minimal_symbol msym = lookup_minimal_symbol_by_pc (sal->pc);
	      if (msym.minsym)
		sal->section = msym.obj_section ();
	    }
	}
    }
}

void
break_command (const char *arg, int from_tty)
{
  break_command_1 (arg, 0, from_tty);
}

void
tbreak_command (const char *arg, int from_tty)
{
  break_command_1 (arg, BP_TEMPFLAG, from_tty);
}

static void
hbreak_command (const char *arg, int from_tty)
{
  break_command_1 (arg, BP_HARDWAREFLAG, from_tty);
}

static void
thbreak_command (const char *arg, int from_tty)
{
  break_command_1 (arg, (BP_TEMPFLAG | BP_HARDWAREFLAG), from_tty);
}

/* The dynamic printf command is mostly like a regular breakpoint, but
   with a prewired command list consisting of a single output command,
   built from extra arguments supplied on the dprintf command
   line.  */

static void
dprintf_command (const char *arg, int from_tty)
{
  location_spec_up locspec = string_to_location_spec (&arg, current_language);

  /* If non-NULL, ARG should have been advanced past the location;
     the next character must be ','.  */
  if (arg != NULL)
    {
      if (arg[0] != ',' || arg[1] == '\0')
	error (_("Format string required"));
      else
	{
	  /* Skip the comma.  */
	  ++arg;
	}
    }

  create_breakpoint (get_current_arch (),
		     locspec.get (),
		     NULL, -1, -1,
		     arg, false, 1 /* parse arg */,
		     0, bp_dprintf,
		     0 /* Ignore count */,
		     pending_break_support,
		     &code_breakpoint_ops,
		     from_tty,
		     1 /* enabled */,
		     0 /* internal */,
		     0);
}

static void
agent_printf_command (const char *arg, int from_tty)
{
  error (_("May only run agent-printf on the target"));
}

/* Implement the "breakpoint_hit" method for ranged breakpoints.  */

int
ranged_breakpoint::breakpoint_hit (const struct bp_location *bl,
				   const address_space *aspace,
				   CORE_ADDR bp_addr,
				   const target_waitstatus &ws)
{
  if (ws.kind () != TARGET_WAITKIND_STOPPED
      || ws.sig () != GDB_SIGNAL_TRAP)
    return 0;

  return breakpoint_address_match_range (bl->pspace->aspace.get (),
					 bl->address, bl->length, aspace,
					 bp_addr);
}

/* Implement the "resources_needed" method for ranged breakpoints.  */

int
ranged_breakpoint::resources_needed (const struct bp_location *bl)
{
  return target_ranged_break_num_registers ();
}

/* Implement the "print_it" method for ranged breakpoints.  */

enum print_stop_action
ranged_breakpoint::print_it (const bpstat *bs) const
{
  struct ui_out *uiout = current_uiout;

  gdb_assert (type == bp_hardware_breakpoint);

  /* Ranged breakpoints have only one location.  */
  gdb_assert (this->has_single_location ());

  annotate_breakpoint (number);

  maybe_print_thread_hit_breakpoint (uiout);

  if (disposition == disp_del)
    uiout->text ("Temporary ranged breakpoint ");
  else
    uiout->text ("Ranged breakpoint ");
  if (uiout->is_mi_like_p ())
    {
      uiout->field_string ("reason",
		      async_reason_lookup (EXEC_ASYNC_BREAKPOINT_HIT));
      uiout->field_string ("disp", bpdisp_text (disposition));
    }
  print_num_locno (bs, uiout);
  uiout->text (", ");

  return PRINT_SRC_AND_LOC;
}

/* Implement the "print_one" method for ranged breakpoints.  */

bool
ranged_breakpoint::print_one (const bp_location **last_loc) const
{
  struct value_print_options opts;
  struct ui_out *uiout = current_uiout;

  /* Ranged breakpoints have only one location.  */
  gdb_assert (this->has_single_location ());

  get_user_print_options (&opts);

  if (opts.addressprint)
    /* We don't print the address range here, it will be printed later
       by ranged_breakpoint::print_one_detail.  */
    uiout->field_skip ("addr");
  annotate_field (5);
  print_breakpoint_location (this, &this->first_loc ());
  *last_loc = &this->first_loc ();

  return true;
}

/* Implement the "print_one_detail" method for ranged breakpoints.  */

void
ranged_breakpoint::print_one_detail (struct ui_out *uiout) const
{
  CORE_ADDR address_start, address_end;
  const bp_location &bl = this->first_loc ();
  string_file stb;

  address_start = bl.address;
  address_end = address_start + bl.length - 1;

  uiout->text ("\taddress range: ");
  stb.printf ("[%s, %s]",
	      print_core_address (bl.gdbarch, address_start),
	      print_core_address (bl.gdbarch, address_end));
  uiout->field_stream ("addr", stb);
  uiout->text ("\n");
}

/* Implement the "print_mention" method for ranged breakpoints.  */

void
ranged_breakpoint::print_mention () const
{
  const bp_location &bl = this->first_loc ();
  struct ui_out *uiout = current_uiout;

  gdb_assert (type == bp_hardware_breakpoint);

  uiout->message (_("Hardware assisted ranged breakpoint %d from %s to %s."),
		  number, paddress (bl.gdbarch, bl.address),
		  paddress (bl.gdbarch, bl.address + bl.length - 1));
}

/* Implement the "print_recreate" method for ranged breakpoints.  */

void
ranged_breakpoint::print_recreate (struct ui_file *fp) const
{
  gdb_printf (fp, "break-range %s, %s",
	      locspec->to_string (),
	      locspec_range_end->to_string ());
  print_recreate_thread (fp);
}

/* Find the address where the end of the breakpoint range should be
   placed, given the SAL of the end of the range.  This is so that if
   the user provides a line number, the end of the range is set to the
   last instruction of the given line.  */

static CORE_ADDR
find_breakpoint_range_end (struct symtab_and_line sal)
{
  CORE_ADDR end;

  /* If the user provided a PC value, use it.  Otherwise,
     find the address of the end of the given location.  */
  if (sal.explicit_pc)
    end = sal.pc;
  else
    {
      int ret;
      CORE_ADDR start;

      ret = find_line_pc_range (sal, &start, &end);
      if (!ret)
	error (_("Could not find location of the end of the range."));

      /* find_line_pc_range returns the start of the next line.  */
      end--;
    }

  return end;
}

/* Implement the "break-range" CLI command.  */

static void
break_range_command (const char *arg, int from_tty)
{
  const char *arg_start;
  struct linespec_result canonical_start, canonical_end;
  int bp_count, can_use_bp, length;
  CORE_ADDR end;

  /* We don't support software ranged breakpoints.  */
  if (target_ranged_break_num_registers () < 0)
    error (_("This target does not support hardware ranged breakpoints."));

  bp_count = hw_breakpoint_used_count ();
  bp_count += target_ranged_break_num_registers ();
  can_use_bp = target_can_use_hardware_watchpoint (bp_hardware_breakpoint,
						   bp_count, 0);
  if (can_use_bp < 0)
    error (_("Hardware breakpoints used exceeds limit."));

  arg = skip_spaces (arg);
  if (arg == NULL || arg[0] == '\0')
    error(_("No address range specified."));

  arg_start = arg;
  location_spec_up start_locspec
    = string_to_location_spec (&arg, current_language);
  parse_breakpoint_sals (start_locspec.get (), &canonical_start);

  if (arg[0] != ',')
    error (_("Too few arguments."));
  else if (canonical_start.lsals.empty ())
    error (_("Could not find location of the beginning of the range."));

  const linespec_sals &lsal_start = canonical_start.lsals[0];

  if (canonical_start.lsals.size () > 1
      || lsal_start.sals.size () != 1)
    error (_("Cannot create a ranged breakpoint with multiple locations."));

  const symtab_and_line &sal_start = lsal_start.sals[0];
  std::string addr_string_start (arg_start, arg - arg_start);

  arg++;	/* Skip the comma.  */
  arg = skip_spaces (arg);

  /* Parse the end location specification.  */

  arg_start = arg;

  /* We call decode_line_full directly here instead of using
     parse_breakpoint_sals because we need to specify the start
     location spec's symtab and line as the default symtab and line
     for the end of the range.  This makes it possible to have ranges
     like "foo.c:27, +14", where +14 means 14 lines from the start
     location spec.  */
  location_spec_up end_locspec
    = string_to_location_spec (&arg, current_language);
  decode_line_full (end_locspec.get (), DECODE_LINE_FUNFIRSTLINE, NULL,
		    sal_start.symtab, sal_start.line,
		    &canonical_end, NULL, NULL);

  if (canonical_end.lsals.empty ())
    error (_("Could not find location of the end of the range."));

  const linespec_sals &lsal_end = canonical_end.lsals[0];
  if (canonical_end.lsals.size () > 1
      || lsal_end.sals.size () != 1)
    error (_("Cannot create a ranged breakpoint with multiple locations."));

  const symtab_and_line &sal_end = lsal_end.sals[0];

  end = find_breakpoint_range_end (sal_end);
  if (sal_start.pc > end)
    error (_("Invalid address range, end precedes start."));

  length = end - sal_start.pc + 1;
  if (length < 0)
    /* Length overflowed.  */
    error (_("Address range too large."));
  else if (length == 1)
    {
      /* This range is simple enough to be handled by
	 the `hbreak' command.  */
      hbreak_command (&addr_string_start[0], 1);

      return;
    }

  /* Now set up the breakpoint and install it.  */

  std::unique_ptr<breakpoint> br
    (new ranged_breakpoint (get_current_arch (),
			    sal_start, length,
			    std::move (start_locspec),
			    std::move (end_locspec)));

  install_breakpoint (false, std::move (br), true);
}

/* See breakpoint.h.  */

watchpoint::~watchpoint ()
{
  /* Make sure to unlink the destroyed watchpoint from the related
     breakpoint ring.  */

  breakpoint *bpt = this;
  while (bpt->related_breakpoint != this)
    bpt = bpt->related_breakpoint;

  bpt->related_breakpoint = this->related_breakpoint;
}

/*  Return non-zero if EXP is verified as constant.  Returned zero
    means EXP is variable.  Also the constant detection may fail for
    some constant expressions and in such case still falsely return
    zero.  */

static bool
watchpoint_exp_is_const (const struct expression *exp)
{
  return exp->op->constant_p ();
}

/* Implement the "re_set" method for watchpoints.  */

void
watchpoint::re_set ()
{
  /* Watchpoint can be either on expression using entirely global
     variables, or it can be on local variables.

     Watchpoints of the first kind are never auto-deleted, and even
     persist across program restarts.  Since they can use variables
     from shared libraries, we need to reparse expression as libraries
     are loaded and unloaded.

     Watchpoints on local variables can also change meaning as result
     of solib event.  For example, if a watchpoint uses both a local
     and a global variables in expression, it's a local watchpoint,
     but unloading of a shared library will make the expression
     invalid.  This is not a very common use case, but we still
     re-evaluate expression, to avoid surprises to the user.

     Note that for local watchpoints, we re-evaluate it only if
     watchpoints frame id is still valid.  If it's not, it means the
     watchpoint is out of scope and will be deleted soon.  In fact,
     I'm not sure we'll ever be called in this case.

     If a local watchpoint's frame id is still valid, then
     exp_valid_block is likewise valid, and we can safely use it.

     Don't do anything about disabled watchpoints, since they will be
     reevaluated again when enabled.  */
  update_watchpoint (this, true /* reparse */);
}

/* Implement the "insert" method for hardware watchpoints.  */

int
watchpoint::insert_location (struct bp_location *bl)
{
  int length = exact ? 1 : bl->length;

  return target_insert_watchpoint (bl->address, length, bl->watchpoint_type,
				   cond_exp.get ());
}

/* Implement the "remove" method for hardware watchpoints.  */

int
watchpoint::remove_location (struct bp_location *bl,
			     enum remove_bp_reason reason)
{
  int length = exact ? 1 : bl->length;

  return target_remove_watchpoint (bl->address, length, bl->watchpoint_type,
				   cond_exp.get ());
}

int
watchpoint::breakpoint_hit (const struct bp_location *bl,
			    const address_space *aspace, CORE_ADDR bp_addr,
			    const target_waitstatus &ws)
{
  struct breakpoint *b = bl->owner;

  /* Continuable hardware watchpoints are treated as non-existent if the
     reason we stopped wasn't a hardware watchpoint (we didn't stop on
     some data address).  Otherwise gdb won't stop on a break instruction
     in the code (not from a breakpoint) when a hardware watchpoint has
     been defined.  Also skip watchpoints which we know did not trigger
     (did not match the data address).  */
  if (is_hardware_watchpoint (b)
      && watchpoint_triggered == watch_triggered_no)
    return 0;

  return 1;
}

void
watchpoint::check_status (bpstat *bs)
{
  bpstat_check_watchpoint (bs);
}

/* Implement the "resources_needed" method for hardware
   watchpoints.  */

int
watchpoint::resources_needed (const struct bp_location *bl)
{
  int length = exact? 1 : bl->length;

  return target_region_ok_for_hw_watchpoint (bl->address, length);
}

/* Implement the "works_in_software_mode" method for hardware
   watchpoints.  */

bool
watchpoint::works_in_software_mode () const
{
  /* Read and access watchpoints only work with hardware support.  */
  return type == bp_watchpoint || type == bp_hardware_watchpoint;
}

enum print_stop_action
watchpoint::print_it (const bpstat *bs) const
{
  enum print_stop_action result;
  struct ui_out *uiout = current_uiout;

  gdb_assert (bs->bp_location_at != NULL);

  annotate_watchpoint (this->number);
  maybe_print_thread_hit_breakpoint (uiout);

  string_file stb;

  std::optional<ui_out_emit_tuple> tuple_emitter;
  switch (this->type)
    {
    case bp_watchpoint:
    case bp_hardware_watchpoint:
      if (uiout->is_mi_like_p ())
	uiout->field_string
	  ("reason", async_reason_lookup (EXEC_ASYNC_WATCHPOINT_TRIGGER));
      mention (this);
      tuple_emitter.emplace (uiout, "value");
      uiout->text ("\nOld value = ");
      watchpoint_value_print (bs->old_val.get (), &stb);
      uiout->field_stream ("old", stb);
      uiout->text ("\nNew value = ");
      watchpoint_value_print (val.get (), &stb);
      uiout->field_stream ("new", stb);
      uiout->text ("\n");
      /* More than one watchpoint may have been triggered.  */
      result = PRINT_UNKNOWN;
      break;

    case bp_read_watchpoint:
      if (uiout->is_mi_like_p ())
	uiout->field_string
	  ("reason", async_reason_lookup (EXEC_ASYNC_READ_WATCHPOINT_TRIGGER));
      mention (this);
      tuple_emitter.emplace (uiout, "value");
      uiout->text ("\nValue = ");
      watchpoint_value_print (val.get (), &stb);
      uiout->field_stream ("value", stb);
      uiout->text ("\n");
      result = PRINT_UNKNOWN;
      break;

    case bp_access_watchpoint:
      if (bs->old_val != NULL)
	{
	  if (uiout->is_mi_like_p ())
	    uiout->field_string
	      ("reason",
	       async_reason_lookup (EXEC_ASYNC_ACCESS_WATCHPOINT_TRIGGER));
	  mention (this);
	  tuple_emitter.emplace (uiout, "value");
	  uiout->text ("\nOld value = ");
	  watchpoint_value_print (bs->old_val.get (), &stb);
	  uiout->field_stream ("old", stb);
	  uiout->text ("\nNew value = ");
	}
      else
	{
	  mention (this);
	  if (uiout->is_mi_like_p ())
	    uiout->field_string
	      ("reason",
	       async_reason_lookup (EXEC_ASYNC_ACCESS_WATCHPOINT_TRIGGER));
	  tuple_emitter.emplace (uiout, "value");
	  uiout->text ("\nValue = ");
	}
      watchpoint_value_print (val.get (), &stb);
      uiout->field_stream ("new", stb);
      uiout->text ("\n");
      result = PRINT_UNKNOWN;
      break;
    default:
      result = PRINT_UNKNOWN;
    }

  return result;
}

/* Implement the "print_mention" method for hardware watchpoints.  */

void
watchpoint::print_mention () const
{
  struct ui_out *uiout = current_uiout;
  const char *tuple_name;

  switch (type)
    {
    case bp_watchpoint:
      uiout->text ("Watchpoint ");
      tuple_name = "wpt";
      break;
    case bp_hardware_watchpoint:
      uiout->text ("Hardware watchpoint ");
      tuple_name = "wpt";
      break;
    case bp_read_watchpoint:
      uiout->text ("Hardware read watchpoint ");
      tuple_name = "hw-rwpt";
      break;
    case bp_access_watchpoint:
      uiout->text ("Hardware access (read/write) watchpoint ");
      tuple_name = "hw-awpt";
      break;
    default:
      internal_error (_("Invalid hardware watchpoint type."));
    }

  ui_out_emit_tuple tuple_emitter (uiout, tuple_name);
  uiout->field_signed ("number", number);
  uiout->text (": ");
  uiout->field_string ("exp", exp_string.get ());
}

/* Implement the "print_recreate" method for watchpoints.  */

void
watchpoint::print_recreate (struct ui_file *fp) const
{
  switch (type)
    {
    case bp_watchpoint:
    case bp_hardware_watchpoint:
      gdb_printf (fp, "watch");
      break;
    case bp_read_watchpoint:
      gdb_printf (fp, "rwatch");
      break;
    case bp_access_watchpoint:
      gdb_printf (fp, "awatch");
      break;
    default:
      internal_error (_("Invalid watchpoint type."));
    }

  gdb_printf (fp, " %s", exp_string.get ());
  print_recreate_thread (fp);
}

/* Implement the "explains_signal" method for watchpoints.  */

bool
watchpoint::explains_signal (enum gdb_signal sig)
{
  /* A software watchpoint cannot cause a signal other than
     GDB_SIGNAL_TRAP.  */
  if (type == bp_watchpoint && sig != GDB_SIGNAL_TRAP)
    return false;

  return true;
}

struct masked_watchpoint : public watchpoint
{
  using watchpoint::watchpoint;

  int insert_location (struct bp_location *) override;
  int remove_location (struct bp_location *,
		       enum remove_bp_reason reason) override;
  int resources_needed (const struct bp_location *) override;
  bool works_in_software_mode () const override;
  enum print_stop_action print_it (const bpstat *bs) const override;
  void print_one_detail (struct ui_out *) const override;
  void print_mention () const override;
  void print_recreate (struct ui_file *fp) const override;
};

/* Implement the "insert" method for masked hardware watchpoints.  */

int
masked_watchpoint::insert_location (struct bp_location *bl)
{
  return target_insert_mask_watchpoint (bl->address, hw_wp_mask,
					bl->watchpoint_type);
}

/* Implement the "remove" method for masked hardware watchpoints.  */

int
masked_watchpoint::remove_location (struct bp_location *bl,
				    enum remove_bp_reason reason)
{
  return target_remove_mask_watchpoint (bl->address, hw_wp_mask,
					bl->watchpoint_type);
}

/* Implement the "resources_needed" method for masked hardware
   watchpoints.  */

int
masked_watchpoint::resources_needed (const struct bp_location *bl)
{
  return target_masked_watch_num_registers (bl->address, hw_wp_mask);
}

/* Implement the "works_in_software_mode" method for masked hardware
   watchpoints.  */

bool
masked_watchpoint::works_in_software_mode () const
{
  return false;
}

/* Implement the "print_it" method for masked hardware
   watchpoints.  */

enum print_stop_action
masked_watchpoint::print_it (const bpstat *bs) const
{
  struct ui_out *uiout = current_uiout;

  /* Masked watchpoints have only one location.  */
  gdb_assert (this->has_single_location ());

  annotate_watchpoint (this->number);
  maybe_print_thread_hit_breakpoint (uiout);

  switch (this->type)
    {
    case bp_hardware_watchpoint:
      if (uiout->is_mi_like_p ())
	uiout->field_string
	  ("reason", async_reason_lookup (EXEC_ASYNC_WATCHPOINT_TRIGGER));
      break;

    case bp_read_watchpoint:
      if (uiout->is_mi_like_p ())
	uiout->field_string
	  ("reason", async_reason_lookup (EXEC_ASYNC_READ_WATCHPOINT_TRIGGER));
      break;

    case bp_access_watchpoint:
      if (uiout->is_mi_like_p ())
	uiout->field_string
	  ("reason",
	   async_reason_lookup (EXEC_ASYNC_ACCESS_WATCHPOINT_TRIGGER));
      break;
    default:
      internal_error (_("Invalid hardware watchpoint type."));
    }

  mention (this);
  uiout->text (_("\n\
Check the underlying instruction at PC for the memory\n\
address and value which triggered this watchpoint.\n"));
  uiout->text ("\n");

  /* More than one watchpoint may have been triggered.  */
  return PRINT_UNKNOWN;
}

/* Implement the "print_one_detail" method for masked hardware
   watchpoints.  */

void
masked_watchpoint::print_one_detail (struct ui_out *uiout) const
{
  /* Masked watchpoints have only one location.  */
  gdb_assert (this->has_single_location ());

  uiout->text ("\tmask ");
  uiout->field_core_addr ("mask", this->first_loc ().gdbarch, hw_wp_mask);
  uiout->text ("\n");
}

/* Implement the "print_mention" method for masked hardware
   watchpoints.  */

void
masked_watchpoint::print_mention () const
{
  struct ui_out *uiout = current_uiout;
  const char *tuple_name;

  switch (type)
    {
    case bp_hardware_watchpoint:
      uiout->text ("Masked hardware watchpoint ");
      tuple_name = "wpt";
      break;
    case bp_read_watchpoint:
      uiout->text ("Masked hardware read watchpoint ");
      tuple_name = "hw-rwpt";
      break;
    case bp_access_watchpoint:
      uiout->text ("Masked hardware access (read/write) watchpoint ");
      tuple_name = "hw-awpt";
      break;
    default:
      internal_error (_("Invalid hardware watchpoint type."));
    }

  ui_out_emit_tuple tuple_emitter (uiout, tuple_name);
  uiout->field_signed ("number", number);
  uiout->text (": ");
  uiout->field_string ("exp", exp_string.get ());
}

/* Implement the "print_recreate" method for masked hardware
   watchpoints.  */

void
masked_watchpoint::print_recreate (struct ui_file *fp) const
{
  switch (type)
    {
    case bp_hardware_watchpoint:
      gdb_printf (fp, "watch");
      break;
    case bp_read_watchpoint:
      gdb_printf (fp, "rwatch");
      break;
    case bp_access_watchpoint:
      gdb_printf (fp, "awatch");
      break;
    default:
      internal_error (_("Invalid hardware watchpoint type."));
    }

  gdb_printf (fp, " %s mask 0x%s", exp_string.get (),
	      phex (hw_wp_mask, sizeof (CORE_ADDR)));
  print_recreate_thread (fp);
}

/* Tell whether the given watchpoint is a masked hardware watchpoint.  */

static bool
is_masked_watchpoint (const struct breakpoint *b)
{
  return dynamic_cast<const masked_watchpoint *> (b) != nullptr;
}

/* accessflag:  hw_write:  watch write, 
		hw_read:   watch read, 
		hw_access: watch access (read or write) */
static void
watch_command_1 (const char *arg, int accessflag, int from_tty,
		 bool just_location, bool internal)
{
  struct breakpoint *scope_breakpoint = NULL;
  const struct block *exp_valid_block = NULL, *cond_exp_valid_block = NULL;
  struct value *result;
  int saved_bitpos = 0, saved_bitsize = 0;
  const char *exp_start = NULL;
  const char *exp_end = NULL;
  const char *tok, *end_tok;
  int toklen = -1;
  const char *cond_start = NULL;
  const char *cond_end = NULL;
  enum bptype bp_type;
  int thread = -1;
  int inferior = -1;
  /* Flag to indicate whether we are going to use masks for
     the hardware watchpoint.  */
  bool use_mask = false;
  CORE_ADDR mask = 0;
  int task = -1;

  /* Make sure that we actually have parameters to parse.  */
  if (arg != NULL && arg[0] != '\0')
    {
      const char *value_start;

      exp_end = arg + strlen (arg);

      /* Look for "parameter value" pairs at the end
	 of the arguments string.  */
      for (tok = exp_end - 1; tok > arg; tok--)
	{
	  /* Skip whitespace at the end of the argument list.  */
	  while (tok > arg && (*tok == ' ' || *tok == '\t'))
	    tok--;

	  /* Find the beginning of the last token.
	     This is the value of the parameter.  */
	  while (tok > arg && (*tok != ' ' && *tok != '\t'))
	    tok--;
	  value_start = tok + 1;

	  /* Skip whitespace.  */
	  while (tok > arg && (*tok == ' ' || *tok == '\t'))
	    tok--;

	  end_tok = tok;

	  /* Find the beginning of the second to last token.
	     This is the parameter itself.  */
	  while (tok > arg && (*tok != ' ' && *tok != '\t'))
	    tok--;
	  tok++;
	  toklen = end_tok - tok + 1;

	  if (toklen == 6 && startswith (tok, "thread"))
	    {
	      struct thread_info *thr;
	      /* At this point we've found a "thread" token, which means
		 the user is trying to set a watchpoint that triggers
		 only in a specific thread.  */
	      const char *endp;

	      if (thread != -1)
		error(_("You can specify only one thread."));

	      if (task != -1)
		error (_("You can specify only one of thread or task."));

	      if (inferior != -1)
		error (_("You can specify only one of inferior or thread."));

	      /* Extract the thread ID from the next token.  */
	      thr = parse_thread_id (value_start, &endp);
	      if (value_start == endp)
		error (_("Junk after thread keyword."));

	      thread = thr->global_num;
	    }
	  else if (toklen == 4 && startswith (tok, "task"))
	    {
	      char *tmp;

	      if (task != -1)
		error(_("You can specify only one task."));

	      if (thread != -1)
		error (_("You can specify only one of thread or task."));

	      if (inferior != -1)
		error (_("You can specify only one of inferior or task."));

	      task = strtol (value_start, &tmp, 0);
	      if (tmp == value_start)
		error (_("Junk after task keyword."));
	      if (!valid_task_id (task))
		error (_("Unknown task %d."), task);
	    }
	  else if (toklen == 8 && startswith (tok, "inferior"))
	    {
	      /* Support for watchpoints will be added in a later commit.  */
	      error (_("Cannot use 'inferior' keyword with watchpoints"));
	    }
	  else if (toklen == 4 && startswith (tok, "mask"))
	    {
	      /* We've found a "mask" token, which means the user wants to
		 create a hardware watchpoint that is going to have the mask
		 facility.  */
	      struct value *mask_value;

	      if (use_mask)
		error(_("You can specify only one mask."));

	      use_mask = just_location = true;

	      scoped_value_mark mark;
	      mask_value = parse_to_comma_and_eval (&value_start);
	      mask = value_as_address (mask_value);
	    }
	  else
	    /* We didn't recognize what we found.  We should stop here.  */
	    break;

	  /* Truncate the string and get rid of the "parameter value" pair before
	     the arguments string is parsed by the parse_exp_1 function.  */
	  exp_end = tok;
	}
    }
  else
    exp_end = arg;

  /* Parse the rest of the arguments.  From here on out, everything
     is in terms of a newly allocated string instead of the original
     ARG.  */
  std::string expression (arg, exp_end - arg);
  exp_start = arg = expression.c_str ();
  innermost_block_tracker tracker;
  expression_up exp = parse_exp_1 (&arg, 0, 0, 0, &tracker);
  exp_end = arg;
  /* Remove trailing whitespace from the expression before saving it.
     This makes the eventual display of the expression string a bit
     prettier.  */
  while (exp_end > exp_start && (exp_end[-1] == ' ' || exp_end[-1] == '\t'))
    --exp_end;

  /* Checking if the expression is not constant.  */
  if (watchpoint_exp_is_const (exp.get ()))
    {
      int len;

      len = exp_end - exp_start;
      while (len > 0 && isspace (exp_start[len - 1]))
	len--;
      error (_("Cannot watch constant value `%.*s'."), len, exp_start);
    }

  exp_valid_block = tracker.block ();
  struct value *mark = value_mark ();
  struct value *val_as_value = nullptr;
  fetch_subexp_value (exp.get (), exp->op.get (), &val_as_value, &result, NULL,
		      just_location);

  if (val_as_value != NULL && just_location)
    {
      saved_bitpos = val_as_value->bitpos ();
      saved_bitsize = val_as_value->bitsize ();
    }

  value_ref_ptr val;
  if (just_location)
    {
      int ret;

      exp_valid_block = NULL;
      val = release_value (value_addr (result));
      value_free_to_mark (mark);

      if (use_mask)
	{
	  ret = target_masked_watch_num_registers (value_as_address (val.get ()),
						   mask);
	  if (ret == -1)
	    error (_("This target does not support masked watchpoints."));
	  else if (ret == -2)
	    error (_("Invalid mask or memory region."));
	}
    }
  else if (val_as_value != NULL)
    val = release_value (val_as_value);

  tok = skip_spaces (arg);
  end_tok = skip_to_space (tok);

  toklen = end_tok - tok;
  if (toklen >= 1 && strncmp (tok, "if", toklen) == 0)
    {
      tok = cond_start = end_tok + 1;
      innermost_block_tracker if_tracker;
      parse_exp_1 (&tok, 0, 0, 0, &if_tracker);

      /* The watchpoint expression may not be local, but the condition
	 may still be.  E.g.: `watch global if local > 0'.  */
      cond_exp_valid_block = if_tracker.block ();

      cond_end = tok;
    }
  if (*tok)
    error (_("Junk at end of command."));

  frame_info_ptr wp_frame = block_innermost_frame (exp_valid_block);

  /* Save this because create_internal_breakpoint below invalidates
     'wp_frame'.  */
  frame_id watchpoint_frame = get_frame_id (wp_frame);

  /* If the expression is "local", then set up a "watchpoint scope"
     breakpoint at the point where we've left the scope of the watchpoint
     expression.  Create the scope breakpoint before the watchpoint, so
     that we will encounter it first in bpstat_stop_status.  */
  if (exp_valid_block != NULL && wp_frame != NULL)
    {
      frame_id caller_frame_id = frame_unwind_caller_id (wp_frame);

      if (frame_id_p (caller_frame_id))
	{
	  gdbarch *caller_arch = frame_unwind_caller_arch (wp_frame);
	  CORE_ADDR caller_pc = frame_unwind_caller_pc (wp_frame);

	  scope_breakpoint
	    = create_internal_breakpoint (caller_arch, caller_pc,
					  bp_watchpoint_scope);

	  /* create_internal_breakpoint could invalidate WP_FRAME.  */
	  wp_frame = NULL;

	  scope_breakpoint->enable_state = bp_enabled;

	  /* Automatically delete the breakpoint when it hits.  */
	  scope_breakpoint->disposition = disp_del;

	  /* Only break in the proper frame (help with recursion).  */
	  scope_breakpoint->frame_id = caller_frame_id;

	  /* Set the address at which we will stop.  */
	  bp_location &loc = scope_breakpoint->first_loc ();
	  loc.gdbarch = caller_arch;
	  loc.requested_address = caller_pc;
	  loc.address
	    = adjust_breakpoint_address (loc.gdbarch, loc.requested_address,
					 scope_breakpoint->type,
					 current_program_space);
	}
    }

  /* Now set up the breakpoint.  We create all watchpoints as hardware
     watchpoints here even if hardware watchpoints are turned off, a call
     to update_watchpoint later in this function will cause the type to
     drop back to bp_watchpoint (software watchpoint) if required.  */

  if (accessflag == hw_read)
    bp_type = bp_read_watchpoint;
  else if (accessflag == hw_access)
    bp_type = bp_access_watchpoint;
  else
    bp_type = bp_hardware_watchpoint;

  std::unique_ptr<watchpoint> w;
  if (use_mask)
    w.reset (new masked_watchpoint (nullptr, bp_type));
  else
    w.reset (new watchpoint (nullptr, bp_type));

  /* At most one of thread or task can be set on a watchpoint.  */
  gdb_assert (thread == -1 || task == -1);
  w->thread = thread;
  w->inferior = inferior;
  w->task = task;
  w->disposition = disp_donttouch;
  w->pspace = current_program_space;
  w->exp = std::move (exp);
  w->exp_valid_block = exp_valid_block;
  w->cond_exp_valid_block = cond_exp_valid_block;
  if (just_location)
    {
      struct type *t = val->type ();
      CORE_ADDR addr = value_as_address (val.get ());

      w->exp_string_reparse
	= current_language->watch_location_expression (t, addr);

      w->exp_string = xstrprintf ("-location %.*s",
				  (int) (exp_end - exp_start), exp_start);
    }
  else
    w->exp_string.reset (savestring (exp_start, exp_end - exp_start));

  if (use_mask)
    {
      w->hw_wp_mask = mask;
    }
  else
    {
      w->val = val;
      w->val_bitpos = saved_bitpos;
      w->val_bitsize = saved_bitsize;
      w->val_valid = true;
    }

  if (cond_start)
    w->cond_string.reset (savestring (cond_start, cond_end - cond_start));
  else
    w->cond_string = 0;

  if (frame_id_p (watchpoint_frame))
    {
      w->watchpoint_frame = watchpoint_frame;
      w->watchpoint_thread = inferior_ptid;
    }
  else
    {
      w->watchpoint_frame = null_frame_id;
      w->watchpoint_thread = null_ptid;
    }

  if (scope_breakpoint != NULL)
    {
      /* The scope breakpoint is related to the watchpoint.  We will
	 need to act on them together.  */
      w->related_breakpoint = scope_breakpoint;
      scope_breakpoint->related_breakpoint = w.get ();
    }

  if (!just_location)
    value_free_to_mark (mark);

  /* Finally update the new watchpoint.  This creates the locations
     that should be inserted.  */
  update_watchpoint (w.get (), true /* reparse */);

  install_breakpoint (internal, std::move (w), 1);
}

/* Return count of debug registers needed to watch the given expression.
   If the watchpoint cannot be handled in hardware return zero.  */

static int
can_use_hardware_watchpoint (const std::vector<value_ref_ptr> &vals)
{
  int found_memory_cnt = 0;

  /* Did the user specifically forbid us to use hardware watchpoints? */
  if (!can_use_hw_watchpoints)
    return 0;

  gdb_assert (!vals.empty ());
  struct value *head = vals[0].get ();

  /* Make sure that the value of the expression depends only upon
     memory contents, and values computed from them within GDB.  If we
     find any register references or function calls, we can't use a
     hardware watchpoint.

     The idea here is that evaluating an expression generates a series
     of values, one holding the value of every subexpression.  (The
     expression a*b+c has five subexpressions: a, b, a*b, c, and
     a*b+c.)  GDB's values hold almost enough information to establish
     the criteria given above --- they identify memory lvalues,
     register lvalues, computed values, etcetera.  So we can evaluate
     the expression, and then scan the chain of values that leaves
     behind to decide whether we can detect any possible change to the
     expression's final value using only hardware watchpoints.

     However, I don't think that the values returned by inferior
     function calls are special in any way.  So this function may not
     notice that an expression involving an inferior function call
     can't be watched with hardware watchpoints.  FIXME.  */
  for (const value_ref_ptr &iter : vals)
    {
      struct value *v = iter.get ();

      if (v->lval () == lval_memory)
	{
	  if (v != head && v->lazy ())
	    /* A lazy memory lvalue in the chain is one that GDB never
	       needed to fetch; we either just used its address (e.g.,
	       `a' in `a.b') or we never needed it at all (e.g., `a'
	       in `a,b').  This doesn't apply to HEAD; if that is
	       lazy then it was not readable, but watch it anyway.  */
	    ;
	  else
	    {
	      /* Ahh, memory we actually used!  Check if we can cover
		 it with hardware watchpoints.  */
	      struct type *vtype = check_typedef (v->type ());

	      /* We only watch structs and arrays if user asked for it
		 explicitly, never if they just happen to appear in a
		 middle of some value chain.  */
	      if (v == head
		  || (vtype->code () != TYPE_CODE_STRUCT
		      && vtype->code () != TYPE_CODE_ARRAY))
		{
		  CORE_ADDR vaddr = v->address ();
		  int len;
		  int num_regs;

		  len = (target_exact_watchpoints
			 && is_scalar_type_recursive (vtype))?
		    1 : v->type ()->length ();

		  num_regs = target_region_ok_for_hw_watchpoint (vaddr, len);
		  if (!num_regs)
		    return 0;
		  else
		    found_memory_cnt += num_regs;
		}
	    }
	}
      else if (v->lval () != not_lval && !v->deprecated_modifiable ())
	return 0;	/* These are values from the history (e.g., $1).  */
      else if (v->lval () == lval_register)
	return 0;	/* Cannot watch a register with a HW watchpoint.  */
    }

  /* The expression itself looks suitable for using a hardware
     watchpoint, but give the target machine a chance to reject it.  */
  return found_memory_cnt;
}

void
watch_command_wrapper (const char *arg, int from_tty, bool internal)
{
  watch_command_1 (arg, hw_write, from_tty, 0, internal);
}

/* Options for the watch, awatch, and rwatch commands.  */

struct watch_options
{
  /* For -location.  */
  bool location = false;
};

/* Definitions of options for the "watch", "awatch", and "rwatch" commands.

   Historically GDB always accepted both '-location' and '-l' flags for
   these commands (both flags being synonyms).  When converting to the
   newer option scheme only '-location' is added here.  That's fine (for
   backward compatibility) as any non-ambiguous prefix of a flag will be
   accepted, so '-l', '-loc', are now all accepted.

   What this means is that, if in the future, we add any new flag here
   that starts with '-l' then this will break backward compatibility, so
   please, don't do that!  */

static const gdb::option::option_def watch_option_defs[] = {
  gdb::option::flag_option_def<watch_options> {
    "location",
    [] (watch_options *opt) { return &opt->location; },
    N_("\
This evaluates EXPRESSION and watches the memory to which is refers.\n\
-l can be used as a short form of -location."),
  },
};

/* Returns the option group used by 'watch', 'awatch', and 'rwatch'
   commands.  */

static gdb::option::option_def_group
make_watch_options_def_group (watch_options *opts)
{
  return {{watch_option_defs}, opts};
}

/* A helper function that looks for the "-location" argument and then
   calls watch_command_1.  */

static void
watch_maybe_just_location (const char *arg, int accessflag, int from_tty)
{
  watch_options opts;
  auto grp = make_watch_options_def_group (&opts);
  gdb::option::process_options
    (&arg, gdb::option::PROCESS_OPTIONS_UNKNOWN_IS_OPERAND, grp);
  if (arg != nullptr && *arg == '\0')
    arg = nullptr;

  watch_command_1 (arg, accessflag, from_tty, opts.location, false);
}

/* Command completion for 'watch', 'awatch', and 'rwatch' commands.   */
static void
watch_command_completer (struct cmd_list_element *ignore,
			 completion_tracker &tracker,
			 const char *text, const char * /*word*/)
{
  const auto group = make_watch_options_def_group (nullptr);
  if (gdb::option::complete_options
      (tracker, &text, gdb::option::PROCESS_OPTIONS_UNKNOWN_IS_OPERAND, group))
    return;

  const char *word = advance_to_expression_complete_word_point (tracker, text);
  expression_completer (ignore, tracker, text, word);
}

static void
watch_command (const char *arg, int from_tty)
{
  watch_maybe_just_location (arg, hw_write, from_tty);
}

void
rwatch_command_wrapper (const char *arg, int from_tty, bool internal)
{
  watch_command_1 (arg, hw_read, from_tty, 0, internal);
}

static void
rwatch_command (const char *arg, int from_tty)
{
  watch_maybe_just_location (arg, hw_read, from_tty);
}

void
awatch_command_wrapper (const char *arg, int from_tty, bool internal)
{
  watch_command_1 (arg, hw_access, from_tty, 0, internal);
}

static void
awatch_command (const char *arg, int from_tty)
{
  watch_maybe_just_location (arg, hw_access, from_tty);
}


/* Data for the FSM that manages the until(location)/advance commands
   in infcmd.c.  Here because it uses the mechanisms of
   breakpoints.  */

struct until_break_fsm : public thread_fsm
{
  /* The thread that was current when the command was executed.  */
  int thread;

  /* The breakpoint set at the return address in the caller frame,
     plus breakpoints at all the destination locations.  */
  std::vector<breakpoint_up> breakpoints;

  until_break_fsm (struct interp *cmd_interp, int thread,
		   std::vector<breakpoint_up> &&breakpoints)
    : thread_fsm (cmd_interp),
      thread (thread),
      breakpoints (std::move (breakpoints))
  {
  }

  void clean_up (struct thread_info *thread) override;
  bool should_stop (struct thread_info *thread) override;
  enum async_reply_reason do_async_reply_reason () override;
};

/* Implementation of the 'should_stop' FSM method for the
   until(location)/advance commands.  */

bool
until_break_fsm::should_stop (struct thread_info *tp)
{
  for (const breakpoint_up &bp : breakpoints)
    if (bpstat_find_breakpoint (tp->control.stop_bpstat,
				bp.get ()) != NULL)
      {
	set_finished ();
	break;
      }

  return true;
}

/* Implementation of the 'clean_up' FSM method for the
   until(location)/advance commands.  */

void
until_break_fsm::clean_up (struct thread_info *)
{
  /* Clean up our temporary breakpoints.  */
  breakpoints.clear ();
  delete_longjmp_breakpoint (thread);
}

/* Implementation of the 'async_reply_reason' FSM method for the
   until(location)/advance commands.  */

enum async_reply_reason
until_break_fsm::do_async_reply_reason ()
{
  return EXEC_ASYNC_LOCATION_REACHED;
}

void
until_break_command (const char *arg, int from_tty, int anywhere)
{
  frame_info_ptr frame;
  struct gdbarch *frame_gdbarch;
  struct frame_id stack_frame_id;
  struct frame_id caller_frame_id;
  int thread;
  struct thread_info *tp;

  clear_proceed_status (0);

  /* Set a breakpoint where the user wants it and at return from
     this function.  */

  location_spec_up locspec = string_to_location_spec (&arg, current_language);

  std::vector<symtab_and_line> sals
    = (last_displayed_sal_is_valid ()
       ? decode_line_1 (locspec.get (), DECODE_LINE_FUNFIRSTLINE, NULL,
			get_last_displayed_symtab (),
			get_last_displayed_line ())
       : decode_line_1 (locspec.get (), DECODE_LINE_FUNFIRSTLINE,
			NULL, NULL, 0));

  if (sals.empty ())
    error (_("Couldn't get information on specified line."));

  if (*arg)
    error (_("Junk at end of arguments."));

  tp = inferior_thread ();
  thread = tp->global_num;

  /* Note linespec handling above invalidates the frame chain.
     Installing a breakpoint also invalidates the frame chain (as it
     may need to switch threads), so do any frame handling before
     that.  */

  frame = get_selected_frame (NULL);
  frame_gdbarch = get_frame_arch (frame);
  stack_frame_id = get_stack_frame_id (frame);
  caller_frame_id = frame_unwind_caller_id (frame);

  /* Keep within the current frame, or in frames called by the current
     one.  */

  std::vector<breakpoint_up> breakpoints;

  std::optional<delete_longjmp_breakpoint_cleanup> lj_deleter;

  if (frame_id_p (caller_frame_id))
    {
      struct symtab_and_line sal2;
      struct gdbarch *caller_gdbarch;

      sal2 = find_pc_line (frame_unwind_caller_pc (frame), 0);
      sal2.pc = frame_unwind_caller_pc (frame);
      caller_gdbarch = frame_unwind_caller_arch (frame);

      breakpoint_up caller_breakpoint
	= set_momentary_breakpoint (caller_gdbarch, sal2,
				    caller_frame_id, bp_until);
      breakpoints.emplace_back (std::move (caller_breakpoint));

      set_longjmp_breakpoint (tp, stack_frame_id);
      lj_deleter.emplace (thread);
    }

  /* If the user told us to continue until a specified location, we
     don't specify a frame at which we need to stop.  Otherwise,
     specify the selected frame, because we want to stop only at the
     very same frame.  */
  frame_id stop_frame_id = anywhere ? null_frame_id : stack_frame_id;

  for (symtab_and_line &sal : sals)
    {
      resolve_sal_pc (&sal);

      breakpoint_up location_breakpoint
	= set_momentary_breakpoint (frame_gdbarch, sal,
				    stop_frame_id, bp_until);
      breakpoints.emplace_back (std::move (location_breakpoint));
    }

  tp->set_thread_fsm
    (std::unique_ptr<thread_fsm>
     (new until_break_fsm (command_interp (), tp->global_num,
			   std::move (breakpoints))));

  if (lj_deleter)
    lj_deleter->release ();

  proceed (-1, GDB_SIGNAL_DEFAULT);
}



/* Compare two breakpoints and return a strcmp-like result.  */

static int
compare_breakpoints (const breakpoint *a, const breakpoint *b)
{
  uintptr_t ua = (uintptr_t) a;
  uintptr_t ub = (uintptr_t) b;

  if (a->number < b->number)
    return -1;
  else if (a->number > b->number)
    return 1;

  /* Now sort by address, in case we see, e..g, two breakpoints with
     the number 0.  */
  if (ua < ub)
    return -1;
  return ua > ub ? 1 : 0;
}

/* Delete breakpoints by address or line.  */

static void
clear_command (const char *arg, int from_tty)
{
  int default_match;

  std::vector<symtab_and_line> decoded_sals;
  symtab_and_line last_sal;
  gdb::array_view<symtab_and_line> sals;
  if (arg)
    {
      decoded_sals
	= decode_line_with_current_source (arg,
					   (DECODE_LINE_FUNFIRSTLINE
					    | DECODE_LINE_LIST_MODE));
      default_match = 0;
      sals = decoded_sals;
    }
  else
    {
      /* Set sal's line, symtab, pc, and pspace to the values
	 corresponding to the last call to print_frame_info.  If the
	 codepoint is not valid, this will set all the fields to 0.  */
      last_sal = get_last_displayed_sal ();
      if (last_sal.symtab == 0)
	error (_("No source file specified."));

      default_match = 1;
      sals = last_sal;
    }

  /* We don't call resolve_sal_pc here.  That's not as bad as it
     seems, because all existing breakpoints typically have both
     file/line and pc set.  So, if clear is given file/line, we can
     match this to existing breakpoint without obtaining pc at all.

     We only support clearing given the address explicitly 
     present in breakpoint table.  Say, we've set breakpoint 
     at file:line.  There were several PC values for that file:line,
     due to optimization, all in one block.

     We've picked one PC value.  If "clear" is issued with another
     PC corresponding to the same file:line, the breakpoint won't
     be cleared.  We probably can still clear the breakpoint, but 
     since the other PC value is never presented to user, user
     can only find it by guessing, and it does not seem important
     to support that.  */

  /* For each line spec given, delete bps which correspond to it.  Do
     it in two passes, solely to preserve the current behavior that
     from_tty is forced true if we delete more than one
     breakpoint.  */

  std::vector<struct breakpoint *> found;
  for (const auto &sal : sals)
    {
      const char *sal_fullname;

      /* If exact pc given, clear bpts at that pc.
	 If line given (pc == 0), clear all bpts on specified line.
	 If defaulting, clear all bpts on default line
	 or at default pc.

	 defaulting    sal.pc != 0    tests to do

	 0              1             pc
	 1              1             pc _and_ line
	 0              0             line
	 1              0             <can't happen> */

      sal_fullname = (sal.symtab == NULL
		      ? NULL : symtab_to_fullname (sal.symtab));

      /* Find all matching breakpoints and add them to 'found'.  */
      for (breakpoint &b : all_breakpoints ())
	{
	  int match = 0;
	  /* Are we going to delete b?  */
	  if (b.type != bp_none && !is_watchpoint (&b)
	      && user_breakpoint_p (&b))
	    {
	      for (bp_location &loc : b.locations ())
		{
		  /* If the user specified file:line, don't allow a PC
		     match.  This matches historical gdb behavior.  */
		  int pc_match = (!sal.explicit_line
				  && sal.pc
				  && (loc.pspace == sal.pspace)
				  && (loc.address == sal.pc)
				  && (!section_is_overlay (loc.section)
				      || loc.section == sal.section));
		  int line_match = 0;

		  if ((default_match || sal.explicit_line)
		      && loc.symtab != NULL
		      && sal_fullname != NULL
		      && sal.pspace == loc.pspace
		      && loc.line_number == sal.line
		      && filename_cmp (symtab_to_fullname (loc.symtab),
				       sal_fullname) == 0)
		    line_match = 1;

		  if (pc_match || line_match)
		    {
		      match = 1;
		      break;
		    }
		}
	    }

	  if (match)
	    found.push_back (&b);
	}
    }

  /* Now go thru the 'found' chain and delete them.  */
  if (found.empty ())
    {
      if (arg)
	error (_("No breakpoint at %s."), arg);
      else
	error (_("No breakpoint at this line."));
    }

  /* Remove duplicates from the vec.  */
  std::sort (found.begin (), found.end (),
	     [] (const breakpoint *bp_a, const breakpoint *bp_b)
	     {
	       return compare_breakpoints (bp_a, bp_b) < 0;
	     });
  found.erase (std::unique (found.begin (), found.end (),
			    [] (const breakpoint *bp_a, const breakpoint *bp_b)
			    {
			      return compare_breakpoints (bp_a, bp_b) == 0;
			    }),
	       found.end ());

  if (found.size () > 1)
    from_tty = 1;	/* Always report if deleted more than one.  */
  if (from_tty)
    {
      if (found.size () == 1)
	gdb_printf (_("Deleted breakpoint "));
      else
	gdb_printf (_("Deleted breakpoints "));
    }

  for (breakpoint *iter : found)
    {
      if (from_tty)
	gdb_printf ("%d ", iter->number);
      delete_breakpoint (iter);
    }
  if (from_tty)
    gdb_putc ('\n');
}

/* Delete breakpoint in BS if they are `delete' breakpoints and
   all breakpoints that are marked for deletion, whether hit or not.
   This is called after any breakpoint is hit, or after errors.  */

void
breakpoint_auto_delete (bpstat *bs)
{
  for (; bs; bs = bs->next)
    if (bs->breakpoint_at
	&& bs->breakpoint_at->disposition == disp_del
	&& bs->stop)
      delete_breakpoint (bs->breakpoint_at);

  for (breakpoint &b : all_breakpoints_safe ())
    if (b.disposition == disp_del_at_next_stop)
      delete_breakpoint (&b);
}

/* A comparison function for bp_location AP and BP being interfaced to
   std::sort.  Sort elements primarily by their ADDRESS (no matter what
   bl_address_is_meaningful says), secondarily by ordering first
   permanent elements and tertiarily just ensuring the array is sorted
   stable way despite std::sort being an unstable algorithm.  */

static int
bp_location_is_less_than (const bp_location *a, const bp_location *b)
{
  if (a->address != b->address)
    return a->address < b->address;

  /* Sort locations at the same address by their pspace number, keeping
     locations of the same inferior (in a multi-inferior environment)
     grouped.  */

  if (a->pspace->num != b->pspace->num)
    return a->pspace->num < b->pspace->num;

  /* Sort permanent breakpoints first.  */
  if (a->permanent != b->permanent)
    return a->permanent > b->permanent;

  /* Sort by type in order to make duplicate determination easier.
     See update_global_location_list.  This is kept in sync with
     breakpoint_locations_match.  */
  if (a->loc_type < b->loc_type)
    return true;

  /* Likewise, for range-breakpoints, sort by length.  */
  if (a->loc_type == bp_loc_hardware_breakpoint
      && b->loc_type == bp_loc_hardware_breakpoint
      && a->length < b->length)
    return true;

  /* Make the internal GDB representation stable across GDB runs
     where A and B memory inside GDB can differ.  Breakpoint locations of
     the same type at the same address can be sorted in arbitrary order.  */

  if (a->owner->number != b->owner->number)
    return a->owner->number < b->owner->number;

  return a < b;
}

/* Set bp_locations_placed_address_before_address_max and
   bp_locations_shadow_len_after_address_max according to the current
   content of the bp_locations array.  */

static void
bp_locations_target_extensions_update (void)
{
  bp_locations_placed_address_before_address_max = 0;
  bp_locations_shadow_len_after_address_max = 0;

  for (bp_location *bl : all_bp_locations ())
    {
      CORE_ADDR start, end, addr;

      if (!bp_location_has_shadow (bl))
	continue;

      start = bl->target_info.placed_address;
      end = start + bl->target_info.shadow_len;

      gdb_assert (bl->address >= start);
      addr = bl->address - start;
      if (addr > bp_locations_placed_address_before_address_max)
	bp_locations_placed_address_before_address_max = addr;

      /* Zero SHADOW_LEN would not pass bp_location_has_shadow.  */

      gdb_assert (bl->address < end);
      addr = end - bl->address;
      if (addr > bp_locations_shadow_len_after_address_max)
	bp_locations_shadow_len_after_address_max = addr;
    }
}

/* Download tracepoint locations if they haven't been.  */

static void
download_tracepoint_locations (void)
{
  enum tribool can_download_tracepoint = TRIBOOL_UNKNOWN;

  scoped_restore_current_pspace_and_thread restore_pspace_thread;

  for (breakpoint &b : all_tracepoints ())
    {
      bool bp_location_downloaded = false;

      if ((b.type == bp_fast_tracepoint
	   ? !may_insert_fast_tracepoints
	   : !may_insert_tracepoints))
	continue;

      if (can_download_tracepoint == TRIBOOL_UNKNOWN)
	{
	  if (target_can_download_tracepoint ())
	    can_download_tracepoint = TRIBOOL_TRUE;
	  else
	    can_download_tracepoint = TRIBOOL_FALSE;
	}

      if (can_download_tracepoint == TRIBOOL_FALSE)
	break;

      for (bp_location &bl : b.locations ())
	{
	  /* In tracepoint, locations are _never_ duplicated, so
	     should_be_inserted is equivalent to
	     unduplicated_should_be_inserted.  */
	  if (!should_be_inserted (&bl) || bl.inserted)
	    continue;

	  switch_to_program_space_and_thread (bl.pspace);

	  target_download_tracepoint (&bl);

	  bl.inserted = 1;
	  bp_location_downloaded = true;
	}

      tracepoint &t = gdb::checked_static_cast<tracepoint &> (b);
      t.number_on_target = b.number;
      if (bp_location_downloaded)
	notify_breakpoint_modified (&b);
    }
}

/* Swap the insertion/duplication state between two locations.  */

static void
swap_insertion (struct bp_location *left, struct bp_location *right)
{
  const int left_inserted = left->inserted;
  const int left_duplicate = left->duplicate;
  const int left_needs_update = left->needs_update;
  const struct bp_target_info left_target_info = left->target_info;

  /* Locations of tracepoints can never be duplicated.  */
  if (is_tracepoint (left->owner))
    gdb_assert (!left->duplicate);
  if (is_tracepoint (right->owner))
    gdb_assert (!right->duplicate);

  left->inserted = right->inserted;
  left->duplicate = right->duplicate;
  left->needs_update = right->needs_update;
  left->target_info = right->target_info;
  right->inserted = left_inserted;
  right->duplicate = left_duplicate;
  right->needs_update = left_needs_update;
  right->target_info = left_target_info;
}

/* Force the re-insertion of the locations at ADDRESS.  This is called
   once a new/deleted/modified duplicate location is found and we are evaluating
   conditions on the target's side.  Such conditions need to be updated on
   the target.  */

static void
force_breakpoint_reinsertion (struct bp_location *bl)
{
  CORE_ADDR address = 0;
  int pspace_num;

  address = bl->address;
  pspace_num = bl->pspace->num;

  /* This is only meaningful if the target is
     evaluating conditions and if the user has
     opted for condition evaluation on the target's
     side.  */
  if (gdb_evaluates_breakpoint_condition_p ()
      || !target_supports_evaluation_of_breakpoint_conditions ())
    return;

  /* Flag all breakpoint locations with this address and
     the same program space as the location
     as "its condition has changed".  We need to
     update the conditions on the target's side.  */
  for (bp_location *loc : all_bp_locations_at_addr (address))
    {
      if (!is_breakpoint (loc->owner)
	  || pspace_num != loc->pspace->num)
	continue;

      /* Flag the location appropriately.  We use a different state to
	 let everyone know that we already updated the set of locations
	 with addr bl->address and program space bl->pspace.  This is so
	 we don't have to keep calling these functions just to mark locations
	 that have already been marked.  */
      loc->condition_changed = condition_updated;

      /* Free the agent expression bytecode as well.  We will compute
	 it later on.  */
      loc->cond_bytecode.reset ();
    }
}

/* Called whether new breakpoints are created, or existing breakpoints
   deleted, to update the global location list and recompute which
   locations are duplicate of which.

   The INSERT_MODE flag determines whether locations may not, may, or
   shall be inserted now.  See 'enum ugll_insert_mode' for more
   info.  */

static void
update_global_location_list (enum ugll_insert_mode insert_mode)
{
  /* Last breakpoint location address that was marked for update.  */
  CORE_ADDR last_addr = 0;
  /* Last breakpoint location program space that was marked for update.  */
  int last_pspace_num = -1;

  breakpoint_debug_printf ("insert_mode = %s",
			   ugll_insert_mode_text (insert_mode));

  /* Used in the duplicates detection below.  When iterating over all
     bp_locations, points to the first bp_location of a given address.
     Breakpoints and watchpoints of different types are never
     duplicates of each other.  Keep one pointer for each type of
     breakpoint/watchpoint, so we only need to loop over all locations
     once.  */
  struct bp_location *bp_loc_first;  /* breakpoint */
  struct bp_location *wp_loc_first;  /* hardware watchpoint */
  struct bp_location *awp_loc_first; /* access watchpoint */
  struct bp_location *rwp_loc_first; /* read watchpoint */

  /* Saved former bp_locations array which we compare against the newly
     built bp_locations from the current state of ALL_BREAKPOINTS.  */
  std::vector<bp_location *> old_locations = std::move (bp_locations);
  bp_locations.clear ();

  for (breakpoint &b : all_breakpoints ())
    for (bp_location &loc : b.locations ())
      bp_locations.push_back (&loc);

  /* See if we need to "upgrade" a software breakpoint to a hardware
     breakpoint.  Do this before deciding whether locations are
     duplicates.  Also do this before sorting because sorting order
     depends on location type.  */
  for (bp_location *loc : bp_locations)
    if (!loc->inserted && should_be_inserted (loc))
	handle_automatic_hardware_breakpoints (loc);

  std::sort (bp_locations.begin (), bp_locations.end (),
	     bp_location_is_less_than);

  bp_locations_target_extensions_update ();

  /* Identify bp_location instances that are no longer present in the
     new list, and therefore should be freed.  Note that it's not
     necessary that those locations should be removed from inferior --
     if there's another location at the same address (previously
     marked as duplicate), we don't need to remove/insert the
     location.
     
     LOCP is kept in sync with OLD_LOCP, each pointing to the current
     and former bp_location array state respectively.  */

  size_t loc_i = 0;
  for (bp_location *old_loc : old_locations)
    {
      /* Tells if 'old_loc' is found among the new locations.  If
	 not, we have to free it.  */
      bool found_object = false;
      /* Tells if the location should remain inserted in the target.  */
      bool keep_in_target = false;
      bool removed = false;

      /* Skip LOCP entries which will definitely never be needed.
	 Stop either at or being the one matching OLD_LOC.  */
      while (loc_i < bp_locations.size ()
	     && bp_locations[loc_i]->address < old_loc->address)
	loc_i++;

      for (size_t loc2_i = loc_i;
	   (loc2_i < bp_locations.size ()
	    && bp_locations[loc2_i]->address == old_loc->address);
	   loc2_i++)
	{
	  /* Check if this is a new/duplicated location or a duplicated
	     location that had its condition modified.  If so, we want to send
	     its condition to the target if evaluation of conditions is taking
	     place there.  */
	  if (bp_locations[loc2_i]->condition_changed == condition_modified
	      && (last_addr != old_loc->address
		  || last_pspace_num != old_loc->pspace->num))
	    {
	      force_breakpoint_reinsertion (bp_locations[loc2_i]);
	      last_pspace_num = old_loc->pspace->num;
	    }

	  if (bp_locations[loc2_i] == old_loc)
	    found_object = true;
	}

      /* We have already handled this address, update it so that we don't
	 have to go through updates again.  */
      last_addr = old_loc->address;

      /* Target-side condition evaluation: Handle deleted locations.  */
      if (!found_object)
	force_breakpoint_reinsertion (old_loc);

      /* If this location is no longer present, and inserted, look if
	 there's maybe a new location at the same address.  If so,
	 mark that one inserted, and don't remove this one.  This is
	 needed so that we don't have a time window where a breakpoint
	 at certain location is not inserted.  */

      if (old_loc->inserted)
	{
	  /* If the location is inserted now, we might have to remove
	     it.  */

	  if (found_object && should_be_inserted (old_loc))
	    {
	      /* The location is still present in the location list,
		 and still should be inserted.  Don't do anything.  */
	      keep_in_target = true;
	    }
	  else
	    {
	      /* This location still exists, but it won't be kept in the
		 target since it may have been disabled.  We proceed to
		 remove its target-side condition.  */

	      /* The location is either no longer present, or got
		 disabled.  See if there's another location at the
		 same address, in which case we don't need to remove
		 this one from the target.  */

	      /* OLD_LOC comes from existing struct breakpoint.  */
	      if (bl_address_is_meaningful (old_loc))
		{
		  for (size_t loc2_i = loc_i;
		       (loc2_i < bp_locations.size ()
			&& bp_locations[loc2_i]->address == old_loc->address);
		       loc2_i++)
		    {
		      bp_location *loc2 = bp_locations[loc2_i];

		      if (loc2 == old_loc)
			continue;

		      if (breakpoint_locations_match (loc2, old_loc))
			{
			  /* Read watchpoint locations are switched to
			     access watchpoints, if the former are not
			     supported, but the latter are.  */
			  if (is_hardware_watchpoint (old_loc->owner))
			    {
			      gdb_assert (is_hardware_watchpoint (loc2->owner));
			      loc2->watchpoint_type = old_loc->watchpoint_type;
			    }

			  /* loc2 is a duplicated location. We need to check
			     if it should be inserted in case it will be
			     unduplicated.  */
			  if (unduplicated_should_be_inserted (loc2))
			    {
			      swap_insertion (old_loc, loc2);
			      keep_in_target = true;
			      break;
			    }
			}
		    }
		}
	    }

	  if (!keep_in_target)
	    {
	      if (remove_breakpoint (old_loc))
		{
		  /* This is just about all we can do.  We could keep
		     this location on the global list, and try to
		     remove it next time, but there's no particular
		     reason why we will succeed next time.
		     
		     Note that at this point, old_loc->owner is still
		     valid, as delete_breakpoint frees the breakpoint
		     only after calling us.  */
		  warning (_("error removing breakpoint %d at %s"),
			   old_loc->owner->number,
			   paddress (old_loc->gdbarch, old_loc->address));
		}
	      removed = true;
	    }
	}

      if (!found_object)
	{
	  if (removed && target_is_non_stop_p ()
	      && need_moribund_for_location_type (old_loc))
	    {
	      /* This location was removed from the target.  In
		 non-stop mode, a race condition is possible where
		 we've removed a breakpoint, but stop events for that
		 breakpoint are already queued and will arrive later.
		 We apply an heuristic to be able to distinguish such
		 SIGTRAPs from other random SIGTRAPs: we keep this
		 breakpoint location for a bit, and will retire it
		 after we see some number of events.  The theory here
		 is that reporting of events should, "on the average",
		 be fair, so after a while we'll see events from all
		 threads that have anything of interest, and no longer
		 need to keep this breakpoint location around.  We
		 don't hold locations forever so to reduce chances of
		 mistaking a non-breakpoint SIGTRAP for a breakpoint
		 SIGTRAP.

		 The heuristic failing can be disastrous on
		 decr_pc_after_break targets.

		 On decr_pc_after_break targets, like e.g., x86-linux,
		 if we fail to recognize a late breakpoint SIGTRAP,
		 because events_till_retirement has reached 0 too
		 soon, we'll fail to do the PC adjustment, and report
		 a random SIGTRAP to the user.  When the user resumes
		 the inferior, it will most likely immediately crash
		 with SIGILL/SIGBUS/SIGSEGV, or worse, get silently
		 corrupted, because of being resumed e.g., in the
		 middle of a multi-byte instruction, or skipped a
		 one-byte instruction.  This was actually seen happen
		 on native x86-linux, and should be less rare on
		 targets that do not support new thread events, like
		 remote, due to the heuristic depending on
		 thread_count.

		 Mistaking a random SIGTRAP for a breakpoint trap
		 causes similar symptoms (PC adjustment applied when
		 it shouldn't), but then again, playing with SIGTRAPs
		 behind the debugger's back is asking for trouble.

		 Since hardware watchpoint traps are always
		 distinguishable from other traps, so we don't need to
		 apply keep hardware watchpoint moribund locations
		 around.  We simply always ignore hardware watchpoint
		 traps we can no longer explain.  */

	      process_stratum_target *proc_target = nullptr;
	      for (inferior *inf : all_inferiors ())
		if (inf->pspace == old_loc->pspace)
		  {
		    proc_target = inf->process_target ();
		    break;
		  }
	      if (proc_target != nullptr)
		old_loc->events_till_retirement
		  = 3 * (thread_count (proc_target) + 1);
	      else
		old_loc->events_till_retirement = 1;
	      old_loc->owner = NULL;

	      moribund_locations.push_back (old_loc);
	    }
	  else
	    {
	      old_loc->owner = NULL;
	      decref_bp_location (&old_loc);
	    }
	}
    }

  /* Rescan breakpoints at the same address and section, marking the
     first one as "first" and any others as "duplicates".  This is so
     that the bpt instruction is only inserted once.  If we have a
     permanent breakpoint at the same place as BPT, make that one the
     official one, and the rest as duplicates.  Permanent breakpoints
     are sorted first for the same address.

     Do the same for hardware watchpoints, but also considering the
     watchpoint's type (regular/access/read) and length.  */

  bp_loc_first = NULL;
  wp_loc_first = NULL;
  awp_loc_first = NULL;
  rwp_loc_first = NULL;

  for (bp_location *loc : all_bp_locations ())
    {
      /* ALL_BP_LOCATIONS bp_location has LOC->OWNER always
	 non-NULL.  */
      struct bp_location **loc_first_p;
      breakpoint *b = loc->owner;

      if (!unduplicated_should_be_inserted (loc)
	  || !bl_address_is_meaningful (loc)
	  /* Don't detect duplicate for tracepoint locations because they are
	   never duplicated.  See the comments in field `duplicate' of
	   `struct bp_location'.  */
	  || is_tracepoint (b))
	{
	  /* Clear the condition modification flag.  */
	  loc->condition_changed = condition_unchanged;
	  continue;
	}

      if (b->type == bp_hardware_watchpoint)
	loc_first_p = &wp_loc_first;
      else if (b->type == bp_read_watchpoint)
	loc_first_p = &rwp_loc_first;
      else if (b->type == bp_access_watchpoint)
	loc_first_p = &awp_loc_first;
      else
	loc_first_p = &bp_loc_first;

      if (*loc_first_p == NULL
	  || (overlay_debugging && loc->section != (*loc_first_p)->section)
	  || !breakpoint_locations_match (loc, *loc_first_p))
	{
	  *loc_first_p = loc;
	  loc->duplicate = 0;

	  if (is_breakpoint (loc->owner) && loc->condition_changed)
	    {
	      loc->needs_update = 1;
	      /* Clear the condition modification flag.  */
	      loc->condition_changed = condition_unchanged;
	    }
	  continue;
	}


      /* This and the above ensure the invariant that the first location
	 is not duplicated, and is the inserted one.
	 All following are marked as duplicated, and are not inserted.  */
      if (loc->inserted)
	swap_insertion (loc, *loc_first_p);
      loc->duplicate = 1;

      /* Clear the condition modification flag.  */
      loc->condition_changed = condition_unchanged;
    }

  if (insert_mode == UGLL_INSERT || breakpoints_should_be_inserted_now ())
    {
      if (insert_mode != UGLL_DONT_INSERT)
	insert_breakpoint_locations ();
      else
	{
	  /* Even though the caller told us to not insert new
	     locations, we may still need to update conditions on the
	     target's side of breakpoints that were already inserted
	     if the target is evaluating breakpoint conditions.  We
	     only update conditions for locations that are marked
	     "needs_update".  */
	  update_inserted_breakpoint_locations ();
	}
    }

  if (insert_mode != UGLL_DONT_INSERT)
    download_tracepoint_locations ();
}

void
breakpoint_retire_moribund (void)
{
  for (int ix = 0; ix < moribund_locations.size (); ++ix)
    {
      struct bp_location *loc = moribund_locations[ix];
      if (--(loc->events_till_retirement) == 0)
	{
	  decref_bp_location (&loc);
	  unordered_remove (moribund_locations, ix);
	  --ix;
	}
    }
}

static void
update_global_location_list_nothrow (enum ugll_insert_mode insert_mode)
{

  try
    {
      update_global_location_list (insert_mode);
    }
  catch (const gdb_exception_error &e)
    {
    }
}

/* Clear BKP from a BPS.  */

static void
bpstat_remove_bp_location (bpstat *bps, struct breakpoint *bpt)
{
  bpstat *bs;

  for (bs = bps; bs; bs = bs->next)
    if (bs->breakpoint_at == bpt)
      {
	bs->breakpoint_at = NULL;
	bs->old_val = NULL;
	/* bs->commands will be freed later.  */
      }
}

/* Callback for iterate_over_threads.  */
static int
bpstat_remove_breakpoint_callback (struct thread_info *th, void *data)
{
  struct breakpoint *bpt = (struct breakpoint *) data;

  bpstat_remove_bp_location (th->control.stop_bpstat, bpt);
  return 0;
}

/* See breakpoint.h.  */

void
code_breakpoint::say_where () const
{
  struct value_print_options opts;

  get_user_print_options (&opts);

  /* i18n: cagney/2005-02-11: Below needs to be merged into a
     single string.  */
  if (!this->has_locations ())
    {
      /* For pending locations, the output differs slightly based
	 on extra_string.  If this is non-NULL, it contains either
	 a condition or dprintf arguments.  */
      if (extra_string == NULL)
	{
	  gdb_printf (_(" (%s) pending."), locspec->to_string ());
	}
      else if (type == bp_dprintf)
	{
	  gdb_printf (_(" (%s,%s) pending."),
		      locspec->to_string (),
		      extra_string.get ());
	}
      else
	{
	  gdb_printf (_(" (%s %s) pending."),
		      locspec->to_string (),
		      extra_string.get ());
	}
    }
  else
    {
      const bp_location &bl = this->first_loc ();
      if (opts.addressprint || bl.symtab == nullptr)
	gdb_printf (" at %ps",
		    styled_string (address_style.style (),
				   paddress (bl.gdbarch,
					     bl.address)));
      if (bl.symtab != NULL)
	{
	  /* If there is a single location, we can print the location
	     more nicely.  */
	  if (!this->has_multiple_locations ())
	    {
	      const char *filename
		= symtab_to_filename_for_display (bl.symtab);
	      gdb_printf (": file %ps, line %d.",
			  styled_string (file_name_style.style (),
					 filename),
			  bl.line_number);
	    }
	  else
	    /* This is not ideal, but each location may have a
	       different file name, and this at least reflects the
	       real situation somewhat.  */
	    gdb_printf (": %s.", locspec->to_string ());
	}

      if (this->has_multiple_locations ())
	{
	  int n = std::distance (m_locations.begin (), m_locations.end ());
	  gdb_printf (" (%d locations)", n);
	}
    }
}

/* See breakpoint.h.  */

bp_location_range breakpoint::locations () const
{
  return bp_location_range (m_locations.begin (), m_locations.end ());
}

struct bp_location *
breakpoint::allocate_location ()
{
  return new bp_location (this);
}

/* See breakpoint.h.  */

void
breakpoint::add_location (bp_location &loc)
{
  gdb_assert (loc.owner == this);
  gdb_assert (!loc.is_linked ());

  auto ub = std::upper_bound (m_locations.begin (), m_locations.end (),
			      loc,
			      [] (const bp_location &left,
				  const bp_location &right)
				{ return left.address < right.address; });
  m_locations.insert (ub, loc);
}

/* See breakpoint.h.  */

void
breakpoint::unadd_location (bp_location &loc)
{
  gdb_assert (loc.owner == this);
  gdb_assert (loc.is_linked ());

  m_locations.erase (m_locations.iterator_to (loc));
}

#define internal_error_pure_virtual_called() \
  gdb_assert_not_reached ("pure virtual function called")

int
breakpoint::insert_location (struct bp_location *bl)
{
  internal_error_pure_virtual_called ();
}

int
breakpoint::remove_location (struct bp_location *bl,
			     enum remove_bp_reason reason)
{
  internal_error_pure_virtual_called ();
}

int
breakpoint::breakpoint_hit (const struct bp_location *bl,
			    const address_space *aspace,
			    CORE_ADDR bp_addr,
			    const target_waitstatus &ws)
{
  internal_error_pure_virtual_called ();
}

int
breakpoint::resources_needed (const struct bp_location *bl)
{
  internal_error_pure_virtual_called ();
}

enum print_stop_action
breakpoint::print_it (const bpstat *bs) const
{
  internal_error_pure_virtual_called ();
}

void
breakpoint::print_mention () const
{
  internal_error_pure_virtual_called ();
}

void
breakpoint::print_recreate (struct ui_file *fp) const
{
  internal_error_pure_virtual_called ();
}

/* Default breakpoint_ops methods.  */

void
code_breakpoint::re_set ()
{
  /* FIXME: is this still reachable?  */
  if (breakpoint_location_spec_empty_p (this))
    {
      /* Anything without a location can't be re-set.  */
      delete_breakpoint (this);
      return;
    }

  re_set_default ();
}

int
code_breakpoint::insert_location (struct bp_location *bl)
{
  CORE_ADDR addr = bl->target_info.reqstd_address;

  bl->target_info.kind = breakpoint_kind (bl, &addr);
  bl->target_info.placed_address = addr;

  int result;
  if (bl->loc_type == bp_loc_hardware_breakpoint)
    result = target_insert_hw_breakpoint (bl->gdbarch, &bl->target_info);
  else
    result = target_insert_breakpoint (bl->gdbarch, &bl->target_info);

  if (result == 0 && bl->probe.prob != nullptr)
    {
      /* The insertion was successful, now let's set the probe's semaphore
	 if needed.  */
      bl->probe.prob->set_semaphore (bl->probe.objfile, bl->gdbarch);
    }

  return result;
}

int
code_breakpoint::remove_location (struct bp_location *bl,
				  enum remove_bp_reason reason)
{
  if (bl->probe.prob != nullptr)
    {
      /* Let's clear the semaphore before removing the location.  */
      bl->probe.prob->clear_semaphore (bl->probe.objfile, bl->gdbarch);
    }

  if (bl->loc_type == bp_loc_hardware_breakpoint)
    return target_remove_hw_breakpoint (bl->gdbarch, &bl->target_info);
  else
    return target_remove_breakpoint (bl->gdbarch, &bl->target_info, reason);
}

int
code_breakpoint::breakpoint_hit (const struct bp_location *bl,
				 const address_space *aspace,
				 CORE_ADDR bp_addr,
				 const target_waitstatus &ws)
{
  if (ws.kind () != TARGET_WAITKIND_STOPPED
      || ws.sig () != GDB_SIGNAL_TRAP)
    return 0;

  if (!breakpoint_address_match (bl->pspace->aspace.get (), bl->address,
				 aspace, bp_addr))
    return 0;

  if (overlay_debugging		/* unmapped overlay section */
      && section_is_overlay (bl->section)
      && !section_is_mapped (bl->section))
    return 0;

  return 1;
}

int
dprintf_breakpoint::breakpoint_hit (const struct bp_location *bl,
				    const address_space *aspace,
				    CORE_ADDR bp_addr,
				    const target_waitstatus &ws)
{
  if (dprintf_style == dprintf_style_agent
      && target_can_run_breakpoint_commands ())
    {
      /* An agent-style dprintf never causes a stop.  If we see a trap
	 for this address it must be for a breakpoint that happens to
	 be set at the same address.  */
      return 0;
    }

  return this->ordinary_breakpoint::breakpoint_hit (bl, aspace, bp_addr, ws);
}

int
ordinary_breakpoint::resources_needed (const struct bp_location *bl)
{
  gdb_assert (type == bp_hardware_breakpoint);

  return 1;
}

enum print_stop_action
ordinary_breakpoint::print_it (const bpstat *bs) const
{
  const struct bp_location *bl;
  int bp_temp;
  struct ui_out *uiout = current_uiout;

  bl = bs->bp_location_at.get ();

  bp_temp = disposition == disp_del;
  if (bl->address != bl->requested_address)
    breakpoint_adjustment_warning (bl->requested_address,
				   bl->address,
				   number, true);
  annotate_breakpoint (number);
  maybe_print_thread_hit_breakpoint (uiout);

  if (uiout->is_mi_like_p ())
    {
      uiout->field_string ("reason",
			   async_reason_lookup (EXEC_ASYNC_BREAKPOINT_HIT));
      uiout->field_string ("disp", bpdisp_text (disposition));
    }

  if (bp_temp)
    uiout->text ("Temporary breakpoint ");
  else
    uiout->text ("Breakpoint ");
  print_num_locno (bs, uiout);
  uiout->text (", ");

  return PRINT_SRC_AND_LOC;
}

void
ordinary_breakpoint::print_mention () const
{
  if (current_uiout->is_mi_like_p ())
    return;

  switch (type)
    {
    case bp_breakpoint:
    case bp_gnu_ifunc_resolver:
      if (disposition == disp_del)
	gdb_printf (_("Temporary breakpoint"));
      else
	gdb_printf (_("Breakpoint"));
      gdb_printf (_(" %d"), number);
      if (type == bp_gnu_ifunc_resolver)
	gdb_printf (_(" at gnu-indirect-function resolver"));
      break;
    case bp_hardware_breakpoint:
      gdb_printf (_("Hardware assisted breakpoint %d"), number);
      break;
    case bp_dprintf:
      gdb_printf (_("Dprintf %d"), number);
      break;
    }

  say_where ();
}

void
ordinary_breakpoint::print_recreate (struct ui_file *fp) const
{
  if (type == bp_breakpoint && disposition == disp_del)
    gdb_printf (fp, "tbreak");
  else if (type == bp_breakpoint)
    gdb_printf (fp, "break");
  else if (type == bp_hardware_breakpoint
	   && disposition == disp_del)
    gdb_printf (fp, "thbreak");
  else if (type == bp_hardware_breakpoint)
    gdb_printf (fp, "hbreak");
  else
    internal_error (_("unhandled breakpoint type %d"), (int) type);

  gdb_printf (fp, " %s", locspec->to_string ());

  /* Print out extra_string if this breakpoint is pending.  It might
     contain, for example, conditions that were set by the user.  */
  if (!this->has_locations () && extra_string != NULL)
    gdb_printf (fp, " %s", extra_string.get ());

  print_recreate_thread (fp);
}

std::vector<symtab_and_line>
code_breakpoint::decode_location_spec (location_spec *locspec,
				       program_space *search_pspace)
{
  if (locspec->type () == PROBE_LOCATION_SPEC)
    return bkpt_probe_decode_location_spec (this, locspec, search_pspace);

  struct linespec_result canonical;

  decode_line_full (locspec, DECODE_LINE_FUNFIRSTLINE, search_pspace,
		    NULL, 0, &canonical, multiple_symbols_all,
		    filter.get ());

  /* We should get 0 or 1 resulting SALs.  */
  gdb_assert (canonical.lsals.size () < 2);

  if (!canonical.lsals.empty ())
    {
      const linespec_sals &lsal = canonical.lsals[0];
      return std::move (lsal.sals);
    }
  return {};
}

/* Virtual table for internal breakpoints.  */

void
internal_breakpoint::re_set ()
{
  switch (type)
    {
      /* Delete overlay event and longjmp master breakpoints; they
	 will be reset later by breakpoint_re_set.  */
    case bp_overlay_event:
    case bp_longjmp_master:
    case bp_std_terminate_master:
    case bp_exception_master:
      delete_breakpoint (this);
      break;

      /* This breakpoint is special, it's set up when the inferior
	 starts and we really don't want to touch it.  */
    case bp_shlib_event:

      /* Like bp_shlib_event, this breakpoint type is special.  Once
	 it is set up, we do not want to touch it.  */
    case bp_thread_event:
      break;
    }
}

void
internal_breakpoint::check_status (bpstat *bs)
{
  if (type == bp_shlib_event)
    {
      /* If requested, stop when the dynamic linker notifies GDB of
	 events.  This allows the user to get control and place
	 breakpoints in initializer routines for dynamically loaded
	 objects (among other things).  */
      bs->stop = stop_on_solib_events != 0;
      bs->print = stop_on_solib_events != 0;
    }
  else
    bs->stop = false;
}

enum print_stop_action
internal_breakpoint::print_it (const bpstat *bs) const
{
  switch (type)
    {
    case bp_shlib_event:
      /* Did we stop because the user set the stop_on_solib_events
	 variable?  (If so, we report this as a generic, "Stopped due
	 to shlib event" message.) */
      print_solib_event (false);
      break;

    case bp_thread_event:
      /* Not sure how we will get here.
	 GDB should not stop for these breakpoints.  */
      gdb_printf (_("Thread Event Breakpoint: gdb should not stop!\n"));
      break;

    case bp_overlay_event:
      /* By analogy with the thread event, GDB should not stop for these.  */
      gdb_printf (_("Overlay Event Breakpoint: gdb should not stop!\n"));
      break;

    case bp_longjmp_master:
      /* These should never be enabled.  */
      gdb_printf (_("Longjmp Master Breakpoint: gdb should not stop!\n"));
      break;

    case bp_std_terminate_master:
      /* These should never be enabled.  */
      gdb_printf (_("std::terminate Master Breakpoint: "
		    "gdb should not stop!\n"));
      break;

    case bp_exception_master:
      /* These should never be enabled.  */
      gdb_printf (_("Exception Master Breakpoint: "
		    "gdb should not stop!\n"));
      break;
    }

  return PRINT_NOTHING;
}

void
internal_breakpoint::print_mention () const
{
  /* Nothing to mention.  These breakpoints are internal.  */
}

/* Virtual table for momentary breakpoints  */

void
momentary_breakpoint::re_set ()
{
  /* Keep temporary breakpoints, which can be encountered when we step
     over a dlopen call and solib_add is resetting the breakpoints.
     Otherwise these should have been blown away via the cleanup chain
     or by breakpoint_init_inferior when we rerun the executable.  */
}

void
momentary_breakpoint::check_status (bpstat *bs)
{
  /* Nothing.  The point of these breakpoints is causing a stop.  */
}

enum print_stop_action
momentary_breakpoint::print_it (const bpstat *bs) const
{
  return PRINT_UNKNOWN;
}

void
momentary_breakpoint::print_mention () const
{
  /* Nothing to mention.  These breakpoints are internal.  */
}

/* Ensure INITIATING_FRAME is cleared when no such breakpoint exists.

   It gets cleared already on the removal of the first one of such placed
   breakpoints.  This is OK as they get all removed altogether.  */

longjmp_breakpoint::~longjmp_breakpoint ()
{
  thread_info *tp = find_thread_global_id (this->thread);

  if (tp != NULL)
    tp->initiating_frame = null_frame_id;
}

static void
bkpt_probe_create_sals_from_location_spec (location_spec *locspec,
					   struct linespec_result *canonical)

{
  struct linespec_sals lsal;

  lsal.sals = parse_probes (locspec, NULL, canonical);
  lsal.canonical = xstrdup (canonical->locspec->to_string ());
  canonical->lsals.push_back (std::move (lsal));
}

static std::vector<symtab_and_line>
bkpt_probe_decode_location_spec (struct breakpoint *b,
				 location_spec *locspec,
				 program_space *search_pspace)
{
  std::vector<symtab_and_line> sals
    = parse_probes (locspec, search_pspace, NULL);
  if (sals.empty ())
    error (_("probe not found"));
  return sals;
}

int
tracepoint::breakpoint_hit (const struct bp_location *bl,
			    const address_space *aspace, CORE_ADDR bp_addr,
			    const target_waitstatus &ws)
{
  /* By definition, the inferior does not report stops at
     tracepoints.  */
  return 0;
}

void
tracepoint::print_one_detail (struct ui_out *uiout) const
{
  if (!static_trace_marker_id.empty ())
    {
      gdb_assert (type == bp_static_tracepoint
		  || type == bp_static_marker_tracepoint);

      uiout->message ("\tmarker id is %pF\n",
		      string_field ("static-tracepoint-marker-string-id",
				    static_trace_marker_id.c_str ()));
    }
}

void
tracepoint::print_mention () const
{
  if (current_uiout->is_mi_like_p ())
    return;

  switch (type)
    {
    case bp_tracepoint:
      gdb_printf (_("Tracepoint"));
      gdb_printf (_(" %d"), number);
      break;
    case bp_fast_tracepoint:
      gdb_printf (_("Fast tracepoint"));
      gdb_printf (_(" %d"), number);
      break;
    case bp_static_tracepoint:
    case bp_static_marker_tracepoint:
      gdb_printf (_("Static tracepoint"));
      gdb_printf (_(" %d"), number);
      break;
    default:
      internal_error (_("unhandled tracepoint type %d"), (int) type);
    }

  say_where ();
}

void
tracepoint::print_recreate (struct ui_file *fp) const
{
  if (type == bp_fast_tracepoint)
    gdb_printf (fp, "ftrace");
  else if (type == bp_static_tracepoint
	   || type == bp_static_marker_tracepoint)
    gdb_printf (fp, "strace");
  else if (type == bp_tracepoint)
    gdb_printf (fp, "trace");
  else
    internal_error (_("unhandled tracepoint type %d"), (int) type);

  gdb_printf (fp, " %s", locspec->to_string ());
  print_recreate_thread (fp);

  if (pass_count)
    gdb_printf (fp, "  passcount %d\n", pass_count);
}

/* Virtual table for tracepoints on static probes.  */

static void
tracepoint_probe_create_sals_from_location_spec
  (location_spec *locspec,
   struct linespec_result *canonical)
{
  /* We use the same method for breakpoint on probes.  */
  bkpt_probe_create_sals_from_location_spec (locspec, canonical);
}

void
dprintf_breakpoint::re_set ()
{
  re_set_default ();

  /* extra_string should never be non-NULL for dprintf.  */
  gdb_assert (extra_string != NULL);

  /* 1 - connect to target 1, that can run breakpoint commands.
     2 - create a dprintf, which resolves fine.
     3 - disconnect from target 1
     4 - connect to target 2, that can NOT run breakpoint commands.

     After steps #3/#4, you'll want the dprintf command list to
     be updated, because target 1 and 2 may well return different
     answers for target_can_run_breakpoint_commands().
     Given absence of finer grained resetting, we get to do
     it all the time.  */
  if (extra_string != NULL)
    update_dprintf_command_list (this);
}

/* Implement the "print_recreate" method for dprintf.  */

void
dprintf_breakpoint::print_recreate (struct ui_file *fp) const
{
  gdb_printf (fp, "dprintf %s,%s", locspec->to_string (), extra_string.get ());
  print_recreate_thread (fp);
}

/* Implement the "after_condition_true" method for dprintf.

   dprintf's are implemented with regular commands in their command
   list, but we run the commands here instead of before presenting the
   stop to the user, as dprintf's don't actually cause a stop.  This
   also makes it so that the commands of multiple dprintfs at the same
   address are all handled.  */

void
dprintf_breakpoint::after_condition_true (struct bpstat *bs)
{
  /* dprintf's never cause a stop.  This wasn't set in the
     check_status hook instead because that would make the dprintf's
     condition not be evaluated.  */
  bs->stop = false;

  /* Run the command list here.  Take ownership of it instead of
     copying.  We never want these commands to run later in
     bpstat_do_actions, if a breakpoint that causes a stop happens to
     be set at same address as this dprintf, or even if running the
     commands here throws.  */
  counted_command_line cmds = std::move (bs->commands);
  gdb_assert (cmds != nullptr);
  execute_control_commands (cmds.get (), 0);
}

/* The breakpoint_ops structure to be used on static tracepoints with
   markers (`-m').  */

static void
strace_marker_create_sals_from_location_spec (location_spec *locspec,
					      struct linespec_result *canonical)
{
  struct linespec_sals lsal;
  const char *arg_start, *arg;

  arg = arg_start = as_linespec_location_spec (locspec)->spec_string.get ();
  lsal.sals = decode_static_tracepoint_spec (&arg);

  std::string str (arg_start, arg - arg_start);
  const char *ptr = str.c_str ();
  canonical->locspec
    = new_linespec_location_spec (&ptr, symbol_name_match_type::FULL);

  lsal.canonical = xstrdup (canonical->locspec->to_string ());
  canonical->lsals.push_back (std::move (lsal));
}

static void
strace_marker_create_breakpoints_sal (struct gdbarch *gdbarch,
				      struct linespec_result *canonical,
				      gdb::unique_xmalloc_ptr<char> cond_string,
				      gdb::unique_xmalloc_ptr<char> extra_string,
				      enum bptype type_wanted,
				      enum bpdisp disposition,
				      int thread,
				      int task, int inferior,
				      int ignore_count,
				      int from_tty, int enabled,
				      int internal, unsigned flags)
{
  const linespec_sals &lsal = canonical->lsals[0];

  /* If the user is creating a static tracepoint by marker id
     (strace -m MARKER_ID), then store the sals index, so that
     breakpoint_re_set can try to match up which of the newly
     found markers corresponds to this one, and, don't try to
     expand multiple locations for each sal, given than SALS
     already should contain all sals for MARKER_ID.  */

  for (size_t i = 0; i < lsal.sals.size (); i++)
    {
      location_spec_up locspec = canonical->locspec->clone ();

      std::unique_ptr<tracepoint> tp
	(new tracepoint (gdbarch,
			 type_wanted,
			 lsal.sals[i],
			 std::move (locspec),
			 NULL,
			 std::move (cond_string),
			 std::move (extra_string),
			 disposition,
			 thread, task, inferior, ignore_count,
			 from_tty, enabled, flags,
			 canonical->special_display));

      /* Given that its possible to have multiple markers with
	 the same string id, if the user is creating a static
	 tracepoint by marker id ("strace -m MARKER_ID"), then
	 store the sals index, so that breakpoint_re_set can
	 try to match up which of the newly found markers
	 corresponds to this one  */
      tp->static_trace_marker_id_idx = i;

      install_breakpoint (internal, std::move (tp), 0);
    }
}

std::vector<symtab_and_line>
static_marker_tracepoint::decode_location_spec (location_spec *locspec,
						program_space *search_pspace)
{
  const char *s = as_linespec_location_spec (locspec)->spec_string.get ();

  std::vector<symtab_and_line> sals = decode_static_tracepoint_spec (&s);
  if (sals.size () > static_trace_marker_id_idx)
    {
      sals[0] = sals[static_trace_marker_id_idx];
      sals.resize (1);
      return sals;
    }
  else
    error (_("marker %s not found"), static_trace_marker_id.c_str ());
}

/* Static tracepoints with marker (`-m').  */
static struct breakpoint_ops strace_marker_breakpoint_ops =
{
  strace_marker_create_sals_from_location_spec,
  strace_marker_create_breakpoints_sal,
};

static bool
strace_marker_p (struct breakpoint *b)
{
  return b->type == bp_static_marker_tracepoint;
}

/* Notify interpreters and observers that breakpoint B was deleted.  */

static void
notify_breakpoint_deleted (breakpoint *b)
{
  interps_notify_breakpoint_deleted (b);
  gdb::observers::breakpoint_deleted.notify (b);
}

/* Delete a breakpoint and clean up all traces of it in the data
   structures.  */

void
delete_breakpoint (struct breakpoint *bpt)
{
  gdb_assert (bpt != NULL);

  /* Has this bp already been deleted?  This can happen because
     multiple lists can hold pointers to bp's.  bpstat lists are
     especial culprits.

     One example of this happening is a watchpoint's scope bp.  When
     the scope bp triggers, we notice that the watchpoint is out of
     scope, and delete it.  We also delete its scope bp.  But the
     scope bp is marked "auto-deleting", and is already on a bpstat.
     That bpstat is then checked for auto-deleting bp's, which are
     deleted.

     A real solution to this problem might involve reference counts in
     bp's, and/or giving them pointers back to their referencing
     bpstat's, and teaching delete_breakpoint to only free a bp's
     storage when no more references were extent.  A cheaper bandaid
     was chosen.  */
  if (bpt->type == bp_none)
    return;

  /* At least avoid this stale reference until the reference counting
     of breakpoints gets resolved.  */
  if (bpt->related_breakpoint != bpt)
    {
      struct breakpoint *related;
      struct watchpoint *w;

      if (bpt->type == bp_watchpoint_scope)
	w = gdb::checked_static_cast<watchpoint *> (bpt->related_breakpoint);
      else if (bpt->related_breakpoint->type == bp_watchpoint_scope)
	w = gdb::checked_static_cast<watchpoint *> (bpt);
      else
	w = NULL;
      if (w != NULL)
	watchpoint_del_at_next_stop (w);

      /* Unlink bpt from the bpt->related_breakpoint ring.  */
      for (related = bpt; related->related_breakpoint != bpt;
	   related = related->related_breakpoint);
      related->related_breakpoint = bpt->related_breakpoint;
      bpt->related_breakpoint = bpt;
    }

  /* watch_command_1 creates a watchpoint but only sets its number if
     update_watchpoint succeeds in creating its bp_locations.  If there's
     a problem in that process, we'll be asked to delete the half-created
     watchpoint.  In that case, don't announce the deletion.  */
  if (bpt->number)
    notify_breakpoint_deleted (bpt);

  breakpoint_chain.erase (breakpoint_chain.iterator_to (*bpt));

  /* Be sure no bpstat's are pointing at the breakpoint after it's
     been freed.  */
  /* FIXME, how can we find all bpstat's?  We just check stop_bpstat
     in all threads for now.  Note that we cannot just remove bpstats
     pointing at bpt from the stop_bpstat list entirely, as breakpoint
     commands are associated with the bpstat; if we remove it here,
     then the later call to bpstat_do_actions (&stop_bpstat); in
     event-top.c won't do anything, and temporary breakpoints with
     commands won't work.  */

  iterate_over_threads (bpstat_remove_breakpoint_callback, bpt);

  /* Now that breakpoint is removed from breakpoint list, update the
     global location list.  This will remove locations that used to
     belong to this breakpoint.  Do this before freeing the breakpoint
     itself, since remove_breakpoint looks at location's owner.  It
     might be better design to have location completely
     self-contained, but it's not the case now.

     Clear the location linked list first, otherwise, the intrusive_list
     destructor accesses the locations after they are freed.  */
  bpt->clear_locations ();
  update_global_location_list (UGLL_DONT_INSERT);

  /* On the chance that someone will soon try again to delete this
     same bp, we mark it as deleted before freeing its storage.  */
  bpt->type = bp_none;
  delete bpt;
}

/* Iterator function to call a user-provided callback function once
   for each of B and its related breakpoints.  */

static void
iterate_over_related_breakpoints (struct breakpoint *b,
				  gdb::function_view<void (breakpoint *)> function)
{
  struct breakpoint *related;

  related = b;
  do
    {
      struct breakpoint *next;

      /* FUNCTION may delete RELATED.  */
      next = related->related_breakpoint;

      if (next == related)
	{
	  /* RELATED is the last ring entry.  */
	  function (related);

	  /* FUNCTION may have deleted it, so we'd never reach back to
	     B.  There's nothing left to do anyway, so just break
	     out.  */
	  break;
	}
      else
	function (related);

      related = next;
    }
  while (related != b);
}

static void
delete_command (const char *arg, int from_tty)
{
  dont_repeat ();

  if (arg == 0)
    {
      int breaks_to_delete = 0;

      /* Delete all breakpoints if no argument.  Do not delete
	 internal breakpoints, these have to be deleted with an
	 explicit breakpoint number argument.  */
      for (breakpoint &b : all_breakpoints ())
	if (user_breakpoint_p (&b))
	  {
	    breaks_to_delete = 1;
	    break;
	  }

      /* Ask user only if there are some breakpoints to delete.  */
      if (!from_tty
	  || (breaks_to_delete && query (_("Delete all breakpoints? "))))
	for (breakpoint &b : all_breakpoints_safe ())
	  if (user_breakpoint_p (&b))
	    delete_breakpoint (&b);
    }
  else
    map_breakpoint_numbers
      (arg, [&] (breakpoint *br)
       {
	 iterate_over_related_breakpoints (br, delete_breakpoint);
       });
}

/* Return true if all locations of B bound to PSPACE are pending.  If
   PSPACE is NULL, all locations of all program spaces are
   considered.  */

static bool
all_locations_are_pending (struct breakpoint *b, struct program_space *pspace)
{
  for (bp_location &loc : b->locations ())
    if ((pspace == NULL
	 || loc.pspace == pspace)
	&& !loc.shlib_disabled
	&& !loc.pspace->executing_startup)
      return false;
  return true;
}

/* Subroutine of update_breakpoint_locations to simplify it.
   Return true if multiple fns in list LOCS have the same name.
   Null names are ignored.  */

static bool
ambiguous_names_p (const bp_location_range &locs)
{
  htab_up htab (htab_create_alloc (13, htab_hash_string, htab_eq_string, NULL,
				   xcalloc, xfree));

  for (const bp_location &l : locs)
    {
      const char **slot;
      const char *name = l.function_name.get ();

      /* Allow for some names to be NULL, ignore them.  */
      if (name == NULL)
	continue;

      slot = (const char **) htab_find_slot (htab.get (), (const void *) name,
					     INSERT);
      /* NOTE: We can assume slot != NULL here because xcalloc never
	 returns NULL.  */
      if (*slot != NULL)
	return true;
      *slot = name;
    }

  return false;
}

/* When symbols change, it probably means the sources changed as well,
   and it might mean the static tracepoint markers are no longer at
   the same address or line numbers they used to be at last we
   checked.  Losing your static tracepoints whenever you rebuild is
   undesirable.  This function tries to resync/rematch gdb static
   tracepoints with the markers on the target, for static tracepoints
   that have not been set by marker id.  Static tracepoint that have
   been set by marker id are reset by marker id in breakpoint_re_set.
   The heuristic is:

   1) For a tracepoint set at a specific address, look for a marker at
   the old PC.  If one is found there, assume to be the same marker.
   If the name / string id of the marker found is different from the
   previous known name, assume that means the user renamed the marker
   in the sources, and output a warning.

   2) For a tracepoint set at a given line number, look for a marker
   at the new address of the old line number.  If one is found there,
   assume to be the same marker.  If the name / string id of the
   marker found is different from the previous known name, assume that
   means the user renamed the marker in the sources, and output a
   warning.

   3) If a marker is no longer found at the same address or line, it
   may mean the marker no longer exists.  But it may also just mean
   the code changed a bit.  Maybe the user added a few lines of code
   that made the marker move up or down (in line number terms).  Ask
   the target for info about the marker with the string id as we knew
   it.  If found, update line number and address in the matching
   static tracepoint.  This will get confused if there's more than one
   marker with the same ID (possible in UST, although unadvised
   precisely because it confuses tools).  */

static struct symtab_and_line
update_static_tracepoint (tracepoint *tp, struct symtab_and_line sal)
{
  struct static_tracepoint_marker marker;
  CORE_ADDR pc;

  pc = sal.pc;
  if (sal.line)
    find_line_pc (sal.symtab, sal.line, &pc);

  if (target_static_tracepoint_marker_at (pc, &marker))
    {
      if (tp->static_trace_marker_id != marker.str_id)
	warning (_("static tracepoint %d changed probed marker from %s to %s"),
		 tp->number, tp->static_trace_marker_id.c_str (),
		 marker.str_id.c_str ());

      tp->static_trace_marker_id = std::move (marker.str_id);

      return sal;
    }

  /* Old marker wasn't found on target at lineno.  Try looking it up
     by string ID.  */
  if (!sal.explicit_pc
      && sal.line != 0
      && sal.symtab != NULL
      && !tp->static_trace_marker_id.empty ())
    {
      std::vector<static_tracepoint_marker> markers
	= target_static_tracepoint_markers_by_strid
	    (tp->static_trace_marker_id.c_str ());

      if (!markers.empty ())
	{
	  struct symbol *sym;
	  struct static_tracepoint_marker *tpmarker;
	  struct ui_out *uiout = current_uiout;

	  tpmarker = &markers[0];

	  tp->static_trace_marker_id = std::move (tpmarker->str_id);

	  warning (_("marker for static tracepoint %d (%s) not "
		     "found at previous line number"),
		   tp->number, tp->static_trace_marker_id.c_str ());

	  symtab_and_line sal2 = find_pc_line (tpmarker->address, 0);
	  sym = find_pc_sect_function (tpmarker->address, NULL);
	  uiout->text ("Now in ");
	  if (sym)
	    {
	      uiout->field_string ("func", sym->print_name (),
				   function_name_style.style ());
	      uiout->text (" at ");
	    }
	  uiout->field_string ("file",
			       symtab_to_filename_for_display (sal2.symtab),
			       file_name_style.style ());
	  uiout->text (":");

	  if (uiout->is_mi_like_p ())
	    {
	      const char *fullname = symtab_to_fullname (sal2.symtab);

	      uiout->field_string ("fullname", fullname);
	    }

	  uiout->field_signed ("line", sal2.line);
	  uiout->text ("\n");

	  tp->first_loc ().line_number = sal2.line;
	  tp->first_loc ().symtab = sym != NULL ? sal2.symtab : NULL;

	  std::unique_ptr<explicit_location_spec> els
	    (new explicit_location_spec
	     (symtab_to_filename_for_display (sal2.symtab)));
	  els->line_offset.offset = tp->first_loc ().line_number;
	  els->line_offset.sign = LINE_OFFSET_NONE;

	  tp->locspec = std::move (els);

	  /* Might be nice to check if function changed, and warn if
	     so.  */
	}
    }
  return sal;
}

/* Returns true iff location lists A and B are sufficiently same that
   we don't need to report breakpoint as changed.  */

static bool
locations_are_equal (const bp_location_list &a, const bp_location_range &b)
{
  auto a_iter = a.begin ();
  auto b_iter = b.begin ();

  for (; a_iter != a.end () && b_iter != b.end (); ++a_iter, ++b_iter)
    {
      if (a_iter->address != b_iter->address)
	return false;

      if (a_iter->shlib_disabled != b_iter->shlib_disabled)
	return false;

      if (a_iter->enabled != b_iter->enabled)
	return false;

      if (a_iter->disabled_by_cond != b_iter->disabled_by_cond)
	return false;
    }

  return (a_iter == a.end ()) == (b_iter == b.end ());
}

/* See breakpoint.h.  */

bp_location_list
breakpoint::steal_locations (program_space *pspace)
{
  if (pspace == NULL)
    return std::move (m_locations);

  bp_location_list ret;

  for (auto it = m_locations.begin (); it != m_locations.end (); )
    {
      if (it->pspace == pspace)
	{
	  bp_location &loc = *it;
	  it = m_locations.erase (it);
	  ret.push_back (loc);
	}
      else
	++it;
    }

  return ret;
}

/* Create new breakpoint locations for B (a hardware or software
   breakpoint) based on SALS and SALS_END.  If SALS_END.NELTS is not
   zero, then B is a ranged breakpoint.  Only recreates locations for
   FILTER_PSPACE.  Locations of other program spaces are left
   untouched.  */

void
update_breakpoint_locations (code_breakpoint *b,
			     struct program_space *filter_pspace,
			     gdb::array_view<const symtab_and_line> sals,
			     gdb::array_view<const symtab_and_line> sals_end)
{
  if (!sals_end.empty () && (sals.size () != 1 || sals_end.size () != 1))
    {
      /* Ranged breakpoints have only one start location and one end
	 location.  */
      b->enable_state = bp_disabled;
      gdb_printf (gdb_stderr,
		  _("Could not reset ranged breakpoint %d: "
		    "multiple locations found\n"),
		  b->number);
      return;
    }

  /* If there's no new locations, and all existing locations are
     pending, don't do anything.  This optimizes the common case where
     all locations are in the same shared library, that was unloaded.
     We'd like to retain the location, so that when the library is
     loaded again, we don't loose the enabled/disabled status of the
     individual locations.  */
  if (all_locations_are_pending (b, filter_pspace) && sals.empty ())
    return;

  bp_location_list existing_locations = b->steal_locations (filter_pspace);

  for (const auto &sal : sals)
    {
      struct bp_location *new_loc;

      switch_to_program_space_and_thread (sal.pspace);

      new_loc = b->add_location (sal);

      /* Reparse conditions, they might contain references to the
	 old symtab.  */
      if (b->cond_string != NULL)
	{
	  const char *s;

	  s = b->cond_string.get ();
	  try
	    {
	      new_loc->cond = parse_exp_1 (&s, sal.pc,
					   block_for_pc (sal.pc),
					   0);
	    }
	  catch (const gdb_exception_error &e)
	    {
	      new_loc->disabled_by_cond = true;
	    }
	}

      if (!sals_end.empty ())
	{
	  CORE_ADDR end = find_breakpoint_range_end (sals_end[0]);

	  new_loc->length = end - sals[0].pc + 1;
	}
    }

  /* If possible, carry over 'disable' status from existing
     breakpoints.  */
  {
    /* If there are multiple breakpoints with the same function name,
       e.g. for inline functions, comparing function names won't work.
       Instead compare pc addresses; this is just a heuristic as things
       may have moved, but in practice it gives the correct answer
       often enough until a better solution is found.  */
    int have_ambiguous_names = ambiguous_names_p (b->locations ());

    for (const bp_location &e : existing_locations)
      {
	if ((!e.enabled || e.disabled_by_cond) && e.function_name)
	  {
	    if (have_ambiguous_names)
	      {
		for (bp_location &l : b->locations ())
		  {
		    /* Ignore software vs hardware location type at
		       this point, because with "set breakpoint
		       auto-hw", after a re-set, locations that were
		       hardware can end up as software, or vice versa.
		       As mentioned above, this is an heuristic and in
		       practice should give the correct answer often
		       enough.  */
		    if (breakpoint_locations_match (&e, &l, true))
		      {
			l.enabled = e.enabled;
			l.disabled_by_cond = e.disabled_by_cond;
			break;
		      }
		  }
	      }
	    else
	      {
		for (bp_location &l : b->locations ())
		  if (l.function_name
		      && strcmp (e.function_name.get (),
				 l.function_name.get ()) == 0)
		    {
		      l.enabled = e.enabled;
		      l.disabled_by_cond = e.disabled_by_cond;
		      break;
		    }
	      }
	  }
      }
  }

  if (!locations_are_equal (existing_locations, b->locations ()))
    notify_breakpoint_modified (b);
}

/* Find the SaL locations corresponding to the given LOCSPEC.
   On return, FOUND will be 1 if any SaL was found, zero otherwise.  */

std::vector<symtab_and_line>
code_breakpoint::location_spec_to_sals (location_spec *locspec,
					struct program_space *search_pspace,
					int *found)
{
  struct gdb_exception exception;

  std::vector<symtab_and_line> sals;

  try
    {
      sals = decode_location_spec (locspec, search_pspace);
    }
  catch (gdb_exception_error &e)
    {
      int not_found_and_ok = false;

      /* For pending breakpoints, it's expected that parsing will
	 fail until the right shared library is loaded.  User has
	 already told to create pending breakpoints and don't need
	 extra messages.  If breakpoint is in bp_shlib_disabled
	 state, then user already saw the message about that
	 breakpoint being disabled, and don't want to see more
	 errors.  */
      if (e.error == NOT_FOUND_ERROR
	  && (condition_not_parsed
	      || (this->has_locations ()
		  && search_pspace != NULL
		  && this->first_loc ().pspace != search_pspace)
	      || (this->has_locations () && this->first_loc ().shlib_disabled)
	      || (this->has_locations ()
		  && this->first_loc ().pspace->executing_startup)
	      || enable_state == bp_disabled))
	not_found_and_ok = true;

      if (!not_found_and_ok)
	{
	  /* We surely don't want to warn about the same breakpoint
	     10 times.  One solution, implemented here, is disable
	     the breakpoint on error.  Another solution would be to
	     have separate 'warning emitted' flag.  Since this
	     happens only when a binary has changed, I don't know
	     which approach is better.  */
	  enable_state = bp_disabled;
	  throw;
	}

      exception = std::move (e);
    }

  if (exception.reason == 0 || exception.error != NOT_FOUND_ERROR)
    {
      for (auto &sal : sals)
	resolve_sal_pc (&sal);
      if (condition_not_parsed && extra_string != NULL)
	{
	  gdb::unique_xmalloc_ptr<char> local_cond, local_extra;
	  int local_thread, local_task, local_inferior;

	  find_condition_and_thread_for_sals (sals, extra_string.get (),
					      &local_cond, &local_thread,
					      &local_inferior,
					      &local_task, &local_extra);
	  gdb_assert (cond_string == nullptr);
	  if (local_cond != nullptr)
	    cond_string = std::move (local_cond);
	  thread = local_thread;
	  task = local_task;
	  if (local_extra != nullptr)
	    extra_string = std::move (local_extra);
	  condition_not_parsed = 0;
	}

      if (type == bp_static_tracepoint)
	{
	  tracepoint *t = gdb::checked_static_cast<tracepoint *> (this);
	  sals[0] = update_static_tracepoint (t, sals[0]);
	}

      *found = 1;
    }
  else
    *found = 0;

  return sals;
}

/* The default re_set method, for typical hardware or software
   breakpoints.  Reevaluate the breakpoint and recreate its
   locations.  */

void
code_breakpoint::re_set_default ()
{
  struct program_space *filter_pspace = current_program_space;
  std::vector<symtab_and_line> expanded, expanded_end;

  int found;
  std::vector<symtab_and_line> sals = location_spec_to_sals (locspec.get (),
							     filter_pspace,
							     &found);
  if (found)
    expanded = std::move (sals);

  if (locspec_range_end != nullptr)
    {
      std::vector<symtab_and_line> sals_end
	= location_spec_to_sals (locspec_range_end.get (),
				 filter_pspace, &found);
      if (found)
	expanded_end = std::move (sals_end);
    }

  update_breakpoint_locations (this, filter_pspace, expanded, expanded_end);
}

/* Default method for creating SALs from an address string.  It basically
   calls parse_breakpoint_sals.  Return 1 for success, zero for failure.  */

static void
create_sals_from_location_spec_default (location_spec *locspec,
					struct linespec_result *canonical)
{
  parse_breakpoint_sals (locspec, canonical);
}

/* Reset a breakpoint.  */

static void
breakpoint_re_set_one (breakpoint *b)
{
  input_radix = b->input_radix;
  set_language (b->language);

  b->re_set ();
}

/* Re-set breakpoint locations for the current program space.
   Locations bound to other program spaces are left untouched.  */

void
breakpoint_re_set (void)
{
  {
    scoped_restore_current_language save_language;
    scoped_restore save_input_radix = make_scoped_restore (&input_radix);
    scoped_restore_current_pspace_and_thread restore_pspace_thread;

    /* breakpoint_re_set_one sets the current_language to the language
       of the breakpoint it is resetting (see prepare_re_set_context)
       before re-evaluating the breakpoint's location.  This change can
       unfortunately get undone by accident if the language_mode is set
       to auto, and we either switch frames, or more likely in this context,
       we select the current frame.

       We prevent this by temporarily turning the language_mode to
       language_mode_manual.  We restore it once all breakpoints
       have been reset.  */
    scoped_restore save_language_mode = make_scoped_restore (&language_mode);
    language_mode = language_mode_manual;

    /* Note: we must not try to insert locations until after all
       breakpoints have been re-set.  Otherwise, e.g., when re-setting
       breakpoint 1, we'd insert the locations of breakpoint 2, which
       hadn't been re-set yet, and thus may have stale locations.  */

    for (breakpoint &b : all_breakpoints_safe ())
      {
	try
	  {
	    breakpoint_re_set_one (&b);
	  }
	catch (const gdb_exception &ex)
	  {
	    exception_fprintf (gdb_stderr, ex,
			       "Error in re-setting breakpoint %d: ",
			       b.number);
	  }
      }

    jit_breakpoint_re_set ();
  }

  create_overlay_event_breakpoint ();
  create_longjmp_master_breakpoint ();
  create_std_terminate_master_breakpoint ();
  create_exception_master_breakpoint ();

  /* Now we can insert.  */
  update_global_location_list (UGLL_MAY_INSERT);
}

/* Reset the thread number of this breakpoint:

   - If the breakpoint is for all threads, leave it as-is.
   - Else, reset it to the current thread for inferior_ptid.  */
void
breakpoint_re_set_thread (struct breakpoint *b)
{
  if (b->thread != -1)
    {
      b->thread = inferior_thread ()->global_num;

      /* We're being called after following a fork.  The new fork is
	 selected as current, and unless this was a vfork will have a
	 different program space from the original thread.  Reset that
	 as well.  */
      b->first_loc ().pspace = current_program_space;
    }
}

/* Set ignore-count of breakpoint number BPTNUM to COUNT.
   If from_tty is nonzero, it prints a message to that effect,
   which ends with a period (no newline).  */

void
set_ignore_count (int bptnum, int count, int from_tty)
{
  if (count < 0)
    count = 0;

  for (breakpoint &b : all_breakpoints ())
    if (b.number == bptnum)
      {
	if (is_tracepoint (&b))
	  {
	    if (from_tty && count != 0)
	      gdb_printf (_("Ignore count ignored for tracepoint %d."),
			  bptnum);
	    return;
	  }

	b.ignore_count = count;
	if (from_tty)
	  {
	    if (count == 0)
	      gdb_printf (_("Will stop next time "
			    "breakpoint %d is reached."),
			  bptnum);
	    else if (count == 1)
	      gdb_printf (_("Will ignore next crossing of breakpoint %d."),
			  bptnum);
	    else
	      gdb_printf (_("Will ignore next %d "
			    "crossings of breakpoint %d."),
			  count, bptnum);
	  }
	notify_breakpoint_modified (&b);
	return;
      }

  error (_("No breakpoint number %d."), bptnum);
}

/* Command to set ignore-count of breakpoint N to COUNT.  */

static void
ignore_command (const char *args, int from_tty)
{
  const char *p = args;
  int num;

  if (p == 0)
    error_no_arg (_("a breakpoint number"));

  num = get_number (&p);
  if (num == 0)
    error (_("bad breakpoint number: '%s'"), args);
  if (*p == 0)
    error (_("Second argument (specified ignore-count) is missing."));

  set_ignore_count (num,
		    longest_to_int (value_as_long (parse_and_eval (p))),
		    from_tty);
  if (from_tty)
    gdb_printf ("\n");
}


/* Call FUNCTION on each of the breakpoints with numbers in the range
   defined by BP_NUM_RANGE (an inclusive range).  */

static void
map_breakpoint_number_range (std::pair<int, int> bp_num_range,
			     gdb::function_view<void (breakpoint *)> function)
{
  if (bp_num_range.first == 0)
    {
      warning (_("bad breakpoint number at or near '%d'"),
	       bp_num_range.first);
    }
  else
    {
      for (int i = bp_num_range.first; i <= bp_num_range.second; i++)
	{
	  bool match = false;

	  for (breakpoint &b : all_breakpoints_safe ())
	    if (b.number == i)
	      {
		match = true;
		function (&b);
		break;
	      }
	  if (!match)
	    gdb_printf (_("No breakpoint number %d.\n"), i);
	}
    }
}

/* Call FUNCTION on each of the breakpoints whose numbers are given in
   ARGS.  */

static void
map_breakpoint_numbers (const char *args,
			gdb::function_view<void (breakpoint *)> function)
{
  if (args == NULL || *args == '\0')
    error_no_arg (_("one or more breakpoint numbers"));

  number_or_range_parser parser (args);

  while (!parser.finished ())
    {
      int num = parser.get_number ();
      map_breakpoint_number_range (std::make_pair (num, num), function);
    }
}

/* Return the breakpoint location structure corresponding to the
   BP_NUM and LOC_NUM values.  */

static struct bp_location *
find_location_by_number (int bp_num, int loc_num)
{
  breakpoint *b = get_breakpoint (bp_num);

  if (!b || b->number != bp_num)
    error (_("Bad breakpoint number '%d'"), bp_num);
  
  if (loc_num == 0)
    error (_("Bad breakpoint location number '%d'"), loc_num);

  int n = 0;
  for (bp_location &loc : b->locations ())
    if (++n == loc_num)
      return &loc;

  error (_("Bad breakpoint location number '%d'"), loc_num);
}

/* Modes of operation for extract_bp_num.  */
enum class extract_bp_kind
{
  /* Extracting a breakpoint number.  */
  bp,

  /* Extracting a location number.  */
  loc,
};

/* Extract a breakpoint or location number (as determined by KIND)
   from the string starting at START.  TRAILER is a character which
   can be found after the number.  If you don't want a trailer, use
   '\0'.  If END_OUT is not NULL, it is set to point after the parsed
   string.  This always returns a positive integer.  */

static int
extract_bp_num (extract_bp_kind kind, const char *start,
		int trailer, const char **end_out = NULL)
{
  const char *end = start;
  int num = get_number_trailer (&end, trailer);
  if (num < 0)
    error (kind == extract_bp_kind::bp
	   ? _("Negative breakpoint number '%.*s'")
	   : _("Negative breakpoint location number '%.*s'"),
	   int (end - start), start);
  if (num == 0)
    error (kind == extract_bp_kind::bp
	   ? _("Bad breakpoint number '%.*s'")
	   : _("Bad breakpoint location number '%.*s'"),
	   int (end - start), start);

  if (end_out != NULL)
    *end_out = end;
  return num;
}

/* Extract a breakpoint or location range (as determined by KIND) in
   the form NUM1-NUM2 stored at &ARG[arg_offset].  Returns a std::pair
   representing the (inclusive) range.  The returned pair's elements
   are always positive integers.  */

static std::pair<int, int>
extract_bp_or_bp_range (extract_bp_kind kind,
			const std::string &arg,
			std::string::size_type arg_offset)
{
  std::pair<int, int> range;
  const char *bp_loc = &arg[arg_offset];
  std::string::size_type dash = arg.find ('-', arg_offset);
  if (dash != std::string::npos)
    {
      /* bp_loc is a range (x-z).  */
      if (arg.length () == dash + 1)
	error (kind == extract_bp_kind::bp
	       ? _("Bad breakpoint number at or near: '%s'")
	       : _("Bad breakpoint location number at or near: '%s'"),
	       bp_loc);

      const char *end;
      const char *start_first = bp_loc;
      const char *start_second = &arg[dash + 1];
      range.first = extract_bp_num (kind, start_first, '-');
      range.second = extract_bp_num (kind, start_second, '\0', &end);

      if (range.first > range.second)
	error (kind == extract_bp_kind::bp
	       ? _("Inverted breakpoint range at '%.*s'")
	       : _("Inverted breakpoint location range at '%.*s'"),
	       int (end - start_first), start_first);
    }
  else
    {
      /* bp_loc is a single value.  */
      range.first = extract_bp_num (kind, bp_loc, '\0');
      range.second = range.first;
    }
  return range;
}

/* Extract the breakpoint/location range specified by ARG.  Returns
   the breakpoint range in BP_NUM_RANGE, and the location range in
   BP_LOC_RANGE.

   ARG may be in any of the following forms:

   x     where 'x' is a breakpoint number.
   x-y   where 'x' and 'y' specify a breakpoint numbers range.
   x.y   where 'x' is a breakpoint number and 'y' a location number.
   x.y-z where 'x' is a breakpoint number and 'y' and 'z' specify a
	 location number range.
*/

static void
extract_bp_number_and_location (const std::string &arg,
				std::pair<int, int> &bp_num_range,
				std::pair<int, int> &bp_loc_range)
{
  std::string::size_type dot = arg.find ('.');

  if (dot != std::string::npos)
    {
      /* Handle 'x.y' and 'x.y-z' cases.  */

      if (arg.length () == dot + 1 || dot == 0)
	error (_("Bad breakpoint number at or near: '%s'"), arg.c_str ());

      bp_num_range.first
	= extract_bp_num (extract_bp_kind::bp, arg.c_str (), '.');
      bp_num_range.second = bp_num_range.first;

      bp_loc_range = extract_bp_or_bp_range (extract_bp_kind::loc,
					     arg, dot + 1);
    }
  else
    {
      /* Handle x and x-y cases.  */

      bp_num_range = extract_bp_or_bp_range (extract_bp_kind::bp, arg, 0);
      bp_loc_range.first = 0;
      bp_loc_range.second = 0;
    }
}

/* Enable or disable a breakpoint location BP_NUM.LOC_NUM.  ENABLE
   specifies whether to enable or disable.  */

static void
enable_disable_bp_num_loc (int bp_num, int loc_num, bool enable)
{
  struct bp_location *loc = find_location_by_number (bp_num, loc_num);
  if (loc != NULL)
    {
      if (loc->disabled_by_cond && enable)
	error (_("Breakpoint %d's condition is invalid at location %d, "
		 "cannot enable."), bp_num, loc_num);

      if (loc->enabled != enable)
	{
	  loc->enabled = enable;
	  mark_breakpoint_location_modified (loc);
	}
      if (target_supports_enable_disable_tracepoint ()
	  && current_trace_status ()->running && loc->owner
	  && is_tracepoint (loc->owner))
	target_disable_tracepoint (loc);
    }
  update_global_location_list (UGLL_DONT_INSERT);

  notify_breakpoint_modified (loc->owner);
}

/* Calculates LOC_NUM for LOC by traversing the bp_location chain of LOC's
   owner.  1-based indexing.  -1 signals NOT FOUND.  */

static int
find_loc_num_by_location (const bp_location *loc)
{
  if (loc != nullptr && loc->owner != nullptr)
    {
      /* Locations use 1-based indexing.  */
      int loc_num = 1;
      for (bp_location &it : loc->owner->locations ())
	{
	  if (&it == loc)
	    return loc_num;
	  loc_num++;
	}
    }
  return -1;
}

/* Enable or disable a breakpoint location LOC.  ENABLE
   specifies whether to enable or disable.  */

void
enable_disable_bp_location (bp_location *loc, bool enable)
{
  if (loc == nullptr)
    error (_("Breakpoint location is invalid."));

  if (loc->owner == nullptr)
    error (_("Breakpoint location does not have an owner breakpoint."));

  if (loc->disabled_by_cond && enable)
    {
      int loc_num = find_loc_num_by_location (loc);
      if (loc_num == -1)
	error (_("Breakpoint location LOC_NUM could not be found."));
      else
	error (_("Breakpoint %d's condition is invalid at location %d, "
		 "cannot enable."), loc->owner->number, loc_num);
    }

  if (loc->enabled != enable)
    {
      loc->enabled = enable;
      mark_breakpoint_location_modified (loc);
    }

  if (target_supports_enable_disable_tracepoint ()
      && current_trace_status ()->running && loc->owner
      && is_tracepoint (loc->owner))
    target_disable_tracepoint (loc);

  update_global_location_list (UGLL_DONT_INSERT);
  notify_breakpoint_modified (loc->owner);
}

/* Enable or disable a range of breakpoint locations.  BP_NUM is the
   number of the breakpoint, and BP_LOC_RANGE specifies the
   (inclusive) range of location numbers of that breakpoint to
   enable/disable.  ENABLE specifies whether to enable or disable the
   location.  */

static void
enable_disable_breakpoint_location_range (int bp_num,
					  std::pair<int, int> &bp_loc_range,
					  bool enable)
{
  for (int i = bp_loc_range.first; i <= bp_loc_range.second; i++)
    enable_disable_bp_num_loc (bp_num, i, enable);
}

/* Set ignore-count of breakpoint number BPTNUM to COUNT.
   If from_tty is nonzero, it prints a message to that effect,
   which ends with a period (no newline).  */

void
disable_breakpoint (struct breakpoint *bpt)
{
  /* Never disable a watchpoint scope breakpoint; we want to
     hit them when we leave scope so we can delete both the
     watchpoint and its scope breakpoint at that time.  */
  if (bpt->type == bp_watchpoint_scope)
    return;

  bpt->enable_state = bp_disabled;

  /* Mark breakpoint locations modified.  */
  mark_breakpoint_modified (bpt);

  if (target_supports_enable_disable_tracepoint ()
      && current_trace_status ()->running && is_tracepoint (bpt))
    {
      for (bp_location &location : bpt->locations ())
	target_disable_tracepoint (&location);
    }

  update_global_location_list (UGLL_DONT_INSERT);

  notify_breakpoint_modified (bpt);
}

/* Enable or disable the breakpoint(s) or breakpoint location(s)
   specified in ARGS.  ARGS may be in any of the formats handled by
   extract_bp_number_and_location.  ENABLE specifies whether to enable
   or disable the breakpoints/locations.  */

static void
enable_disable_command (const char *args, int from_tty, bool enable)
{
  if (args == 0)
    {
      for (breakpoint &bpt : all_breakpoints ())
	if (user_breakpoint_p (&bpt))
	  {
	    if (enable)
	      enable_breakpoint (&bpt);
	    else
	      disable_breakpoint (&bpt);
	  }
    }
  else
    {
      std::string num = extract_arg (&args);

      while (!num.empty ())
	{
	  std::pair<int, int> bp_num_range, bp_loc_range;

	  extract_bp_number_and_location (num, bp_num_range, bp_loc_range);

	  if (bp_loc_range.first == bp_loc_range.second
	      && (bp_loc_range.first == 0
		  || (bp_loc_range.first == 1
		      && bp_num_range.first == bp_num_range.second
		      && !has_multiple_locations (bp_num_range.first))))
	    {
	      /* Handle breakpoint ids with formats 'x' or 'x-z'
		 or 'y.1' where y has only one code location.  */
	      map_breakpoint_number_range (bp_num_range,
					   enable
					   ? enable_breakpoint
					   : disable_breakpoint);
	    }
	  else
	    {
	      /* Handle breakpoint ids with formats 'x.y' or
		 'x.y-z'.  */
	      enable_disable_breakpoint_location_range
		(bp_num_range.first, bp_loc_range, enable);
	    }
	  num = extract_arg (&args);
	}
    }
}

/* The disable command disables the specified breakpoints/locations
   (or all defined breakpoints) so they're no longer effective in
   stopping the inferior.  ARGS may be in any of the forms defined in
   extract_bp_number_and_location.  */

static void
disable_command (const char *args, int from_tty)
{
  enable_disable_command (args, from_tty, false);
}

static void
enable_breakpoint_disp (struct breakpoint *bpt, enum bpdisp disposition,
			int count)
{
  int target_resources_ok;

  if (bpt->type == bp_hardware_breakpoint)
    {
      int i;
      i = hw_breakpoint_used_count ();
      target_resources_ok = 
	target_can_use_hardware_watchpoint (bp_hardware_breakpoint, 
					    i + 1, 0);
      if (target_resources_ok == 0)
	error (_("No hardware breakpoint support in the target."));
      else if (target_resources_ok < 0)
	error (_("Hardware breakpoints used exceeds limit."));
    }

  if (is_watchpoint (bpt))
    {
      /* Initialize it just to avoid a GCC false warning.  */
      enum enable_state orig_enable_state = bp_disabled;

      try
	{
	  watchpoint *w = gdb::checked_static_cast<watchpoint *> (bpt);

	  orig_enable_state = bpt->enable_state;
	  bpt->enable_state = bp_enabled;
	  update_watchpoint (w, true /* reparse */);
	}
      catch (const gdb_exception_error &e)
	{
	  bpt->enable_state = orig_enable_state;
	  exception_fprintf (gdb_stderr, e, _("Cannot enable watchpoint %d: "),
			     bpt->number);
	  return;
	}
    }

  bpt->enable_state = bp_enabled;

  /* Mark breakpoint locations modified.  */
  mark_breakpoint_modified (bpt);

  if (target_supports_enable_disable_tracepoint ()
      && current_trace_status ()->running && is_tracepoint (bpt))
    {
      for (bp_location &location : bpt->locations ())
	target_enable_tracepoint (&location);
    }

  bpt->disposition = disposition;
  bpt->enable_count = count;
  update_global_location_list (UGLL_MAY_INSERT);

  notify_breakpoint_modified (bpt);
}


void
enable_breakpoint (struct breakpoint *bpt)
{
  enable_breakpoint_disp (bpt, bpt->disposition, 0);
}

/* The enable command enables the specified breakpoints/locations (or
   all defined breakpoints) so they once again become (or continue to
   be) effective in stopping the inferior.  ARGS may be in any of the
   forms defined in extract_bp_number_and_location.  */

static void
enable_command (const char *args, int from_tty)
{
  enable_disable_command (args, from_tty, true);
}

static void
enable_once_command (const char *args, int from_tty)
{
  map_breakpoint_numbers
    (args, [&] (breakpoint *b)
     {
       iterate_over_related_breakpoints
	 (b, [&] (breakpoint *bpt)
	  {
	    enable_breakpoint_disp (bpt, disp_disable, 1);
	  });
     });
}

static void
enable_count_command (const char *args, int from_tty)
{
  int count;

  if (args == NULL)
    error_no_arg (_("hit count"));

  count = get_number (&args);

  map_breakpoint_numbers
    (args, [&] (breakpoint *b)
     {
       iterate_over_related_breakpoints
	 (b, [&] (breakpoint *bpt)
	  {
	    enable_breakpoint_disp (bpt, disp_disable, count);
	  });
     });
}

static void
enable_delete_command (const char *args, int from_tty)
{
  map_breakpoint_numbers
    (args, [&] (breakpoint *b)
     {
       iterate_over_related_breakpoints
	 (b, [&] (breakpoint *bpt)
	  {
	    enable_breakpoint_disp (bpt, disp_del, 1);
	  });
     });
}

/* Invalidate last known value of any hardware watchpoint if
   the memory which that value represents has been written to by
   GDB itself.  */

static void
invalidate_bp_value_on_memory_change (struct inferior *inferior,
				      CORE_ADDR addr, ssize_t len,
				      const bfd_byte *data)
{
  for (breakpoint &bp : all_breakpoints ())
    if (bp.enable_state == bp_enabled
	&& bp.type == bp_hardware_watchpoint)
      {
	watchpoint &wp = gdb::checked_static_cast<watchpoint &> (bp);

	if (wp.val_valid && wp.val != nullptr)
	  {
	    for (bp_location &loc : bp.locations ())
	      if (loc.loc_type == bp_loc_hardware_watchpoint
		  && loc.address + loc.length > addr
		  && addr + len > loc.address)
		{
		  wp.val = NULL;
		  wp.val_valid = false;
		}
	  }
      }
}

/* Create and insert a breakpoint for software single step.  */

void
insert_single_step_breakpoint (struct gdbarch *gdbarch,
			       const address_space *aspace,
			       CORE_ADDR next_pc)
{
  struct thread_info *tp = inferior_thread ();
  struct symtab_and_line sal;
  CORE_ADDR pc = next_pc;

  if (tp->control.single_step_breakpoints == NULL)
    {
      std::unique_ptr<breakpoint> b
	(new momentary_breakpoint (gdbarch, bp_single_step,
				   current_program_space,
				   null_frame_id,
				   tp->global_num));

      tp->control.single_step_breakpoints
	= add_to_breakpoint_chain (std::move (b));
    }

  sal = find_pc_line (pc, 0);
  sal.pc = pc;
  sal.section = find_pc_overlay (pc);
  sal.explicit_pc = 1;

  auto *ss_bp
    = (gdb::checked_static_cast<momentary_breakpoint *>
       (tp->control.single_step_breakpoints));
  ss_bp->add_location (sal);

  update_global_location_list (UGLL_INSERT);
}

/* Insert single step breakpoints according to the current state.  */

int
insert_single_step_breakpoints (struct gdbarch *gdbarch)
{
  regcache *regcache = get_thread_regcache (inferior_thread ());
  std::vector<CORE_ADDR> next_pcs;

  next_pcs = gdbarch_software_single_step (gdbarch, regcache);

  if (!next_pcs.empty ())
    {
      frame_info_ptr frame = get_current_frame ();
      const address_space *aspace = get_frame_address_space (frame);

      for (CORE_ADDR pc : next_pcs)
	insert_single_step_breakpoint (gdbarch, aspace, pc);

      return 1;
    }
  else
    return 0;
}

/* See breakpoint.h.  */

int
breakpoint_has_location_inserted_here (struct breakpoint *bp,
				       const address_space *aspace,
				       CORE_ADDR pc)
{
  for (bp_location &loc : bp->locations ())
    if (loc.inserted
	&& breakpoint_location_address_match (&loc, aspace, pc))
      return 1;

  return 0;
}

/* Check whether a software single-step breakpoint is inserted at
   PC.  */

int
single_step_breakpoint_inserted_here_p (const address_space *aspace,
					CORE_ADDR pc)
{
  for (breakpoint &bpt : all_breakpoints ())
    {
      if (bpt.type == bp_single_step
	  && breakpoint_has_location_inserted_here (&bpt, aspace, pc))
	return 1;
    }
  return 0;
}

/* Tracepoint-specific operations.  */

/* Set tracepoint count to NUM.  */
static void
set_tracepoint_count (int num)
{
  tracepoint_count = num;
  set_internalvar_integer (lookup_internalvar ("tpnum"), num);
}

static void
trace_command (const char *arg, int from_tty)
{
  location_spec_up locspec = string_to_location_spec (&arg,
						      current_language);
  const struct breakpoint_ops *ops = breakpoint_ops_for_location_spec
    (locspec.get (), true /* is_tracepoint */);

  create_breakpoint (get_current_arch (),
		     locspec.get (),
		     NULL, -1, -1, arg, false, 1 /* parse arg */,
		     0 /* tempflag */,
		     bp_tracepoint /* type_wanted */,
		     0 /* Ignore count */,
		     pending_break_support,
		     ops,
		     from_tty,
		     1 /* enabled */,
		     0 /* internal */, 0);
}

static void
ftrace_command (const char *arg, int from_tty)
{
  location_spec_up locspec = string_to_location_spec (&arg,
						      current_language);
  create_breakpoint (get_current_arch (),
		     locspec.get (),
		     NULL, -1, -1, arg, false, 1 /* parse arg */,
		     0 /* tempflag */,
		     bp_fast_tracepoint /* type_wanted */,
		     0 /* Ignore count */,
		     pending_break_support,
		     &code_breakpoint_ops,
		     from_tty,
		     1 /* enabled */,
		     0 /* internal */, 0);
}

/* strace command implementation.  Creates a static tracepoint.  */

static void
strace_command (const char *arg, int from_tty)
{
  const struct breakpoint_ops *ops;
  location_spec_up locspec;
  enum bptype type;

  /* Decide if we are dealing with a static tracepoint marker (`-m'),
     or with a normal static tracepoint.  */
  if (arg && startswith (arg, "-m") && isspace (arg[2]))
    {
      ops = &strace_marker_breakpoint_ops;
      locspec = new_linespec_location_spec (&arg,
					    symbol_name_match_type::FULL);
      type = bp_static_marker_tracepoint;
    }
  else
    {
      ops = &code_breakpoint_ops;
      locspec = string_to_location_spec (&arg, current_language);
      type = bp_static_tracepoint;
    }

  create_breakpoint (get_current_arch (),
		     locspec.get (),
		     NULL, -1, -1, arg, false, 1 /* parse arg */,
		     0 /* tempflag */,
		     type /* type_wanted */,
		     0 /* Ignore count */,
		     pending_break_support,
		     ops,
		     from_tty,
		     1 /* enabled */,
		     0 /* internal */, 0);
}

/* Set up a fake reader function that gets command lines from a linked
   list that was acquired during tracepoint uploading.  */

static struct uploaded_tp *this_utp;
static int next_cmd;

static const char *
read_uploaded_action (std::string &buffer)
{
  char *rslt = nullptr;

  if (next_cmd < this_utp->cmd_strings.size ())
    {
      rslt = this_utp->cmd_strings[next_cmd].get ();
      next_cmd++;
    }

  return rslt;
}

/* Given information about a tracepoint as recorded on a target (which
   can be either a live system or a trace file), attempt to create an
   equivalent GDB tracepoint.  This is not a reliable process, since
   the target does not necessarily have all the information used when
   the tracepoint was originally defined.  */
  
struct tracepoint *
create_tracepoint_from_upload (struct uploaded_tp *utp)
{
  const char *addr_str;
  char small_buf[100];
  struct tracepoint *tp;

  if (utp->at_string)
    addr_str = utp->at_string.get ();
  else
    {
      /* In the absence of a source location, fall back to raw
	 address.  Since there is no way to confirm that the address
	 means the same thing as when the trace was started, warn the
	 user.  */
      warning (_("Uploaded tracepoint %d has no "
		 "source location, using raw address"),
	       utp->number);
      xsnprintf (small_buf, sizeof (small_buf), "*%s", hex_string (utp->addr));
      addr_str = small_buf;
    }

  /* There's not much we can do with a sequence of bytecodes.  */
  if (utp->cond && !utp->cond_string)
    warning (_("Uploaded tracepoint %d condition "
	       "has no source form, ignoring it"),
	     utp->number);

  location_spec_up locspec = string_to_location_spec (&addr_str,
						      current_language);
  if (!create_breakpoint (get_current_arch (),
			  locspec.get (),
			  utp->cond_string.get (), -1, -1, addr_str,
			  false /* force_condition */,
			  0 /* parse cond/thread */,
			  0 /* tempflag */,
			  utp->type /* type_wanted */,
			  0 /* Ignore count */,
			  pending_break_support,
			  &code_breakpoint_ops,
			  0 /* from_tty */,
			  utp->enabled /* enabled */,
			  0 /* internal */,
			  CREATE_BREAKPOINT_FLAGS_INSERTED))
    return NULL;

  /* Get the tracepoint we just created.  */
  tp = get_tracepoint (tracepoint_count);
  gdb_assert (tp != NULL);

  if (utp->pass > 0)
    {
      xsnprintf (small_buf, sizeof (small_buf), "%d %d", utp->pass,
		 tp->number);

      trace_pass_command (small_buf, 0);
    }

  /* If we have uploaded versions of the original commands, set up a
     special-purpose "reader" function and call the usual command line
     reader, then pass the result to the breakpoint command-setting
     function.  */
  if (!utp->cmd_strings.empty ())
    {
      counted_command_line cmd_list;

      this_utp = utp;
      next_cmd = 0;

      cmd_list = read_command_lines_1 (read_uploaded_action, 1, NULL);

      breakpoint_set_commands (tp, std::move (cmd_list));
    }
  else if (!utp->actions.empty ()
	   || !utp->step_actions.empty ())
    warning (_("Uploaded tracepoint %d actions "
	       "have no source form, ignoring them"),
	     utp->number);

  /* Copy any status information that might be available.  */
  tp->hit_count = utp->hit_count;
  tp->traceframe_usage = utp->traceframe_usage;

  return tp;
}
  
/* Print information on tracepoint number TPNUM_EXP, or all if
   omitted.  */

static void
info_tracepoints_command (const char *args, int from_tty)
{
  struct ui_out *uiout = current_uiout;
  int num_printed;

  num_printed = breakpoint_1 (args, false, is_tracepoint);

  if (num_printed == 0)
    {
      if (args == NULL || *args == '\0')
	uiout->message ("No tracepoints.\n");
      else
	uiout->message ("No tracepoint matching '%s'.\n", args);
    }

  default_collect_info ();
}

/* The 'enable trace' command enables tracepoints.
   Not supported by all targets.  */
static void
enable_trace_command (const char *args, int from_tty)
{
  enable_command (args, from_tty);
}

/* The 'disable trace' command disables tracepoints.
   Not supported by all targets.  */
static void
disable_trace_command (const char *args, int from_tty)
{
  disable_command (args, from_tty);
}

/* Remove a tracepoint (or all if no argument).  */
static void
delete_trace_command (const char *arg, int from_tty)
{
  dont_repeat ();

  if (arg == 0)
    {
      int breaks_to_delete = 0;

      /* Delete all breakpoints if no argument.
	 Do not delete internal or call-dummy breakpoints, these
	 have to be deleted with an explicit breakpoint number 
	 argument.  */
      for (breakpoint &tp : all_tracepoints ())
	if (is_tracepoint (&tp) && user_breakpoint_p (&tp))
	  {
	    breaks_to_delete = 1;
	    break;
	  }

      /* Ask user only if there are some breakpoints to delete.  */
      if (!from_tty
	  || (breaks_to_delete && query (_("Delete all tracepoints? "))))
	{
	  for (breakpoint &b : all_breakpoints_safe ())
	    if (is_tracepoint (&b) && user_breakpoint_p (&b))
	      delete_breakpoint (&b);
	}
    }
  else
    map_breakpoint_numbers
      (arg, [&] (breakpoint *br)
       {
	 iterate_over_related_breakpoints (br, delete_breakpoint);
       });
}

/* Helper function for trace_pass_command.  */

static void
trace_pass_set_count (struct tracepoint *tp, int count, int from_tty)
{
  tp->pass_count = count;
  notify_breakpoint_modified (tp);
  if (from_tty)
    gdb_printf (_("Setting tracepoint %d's passcount to %d\n"),
		tp->number, count);
}

/* Set passcount for tracepoint.

   First command argument is passcount, second is tracepoint number.
   If tracepoint number omitted, apply to most recently defined.
   Also accepts special argument "all".  */

static void
trace_pass_command (const char *args, int from_tty)
{
  ULONGEST count;

  if (args == 0 || *args == 0)
    error (_("passcount command requires an "
	     "argument (count + optional TP num)"));

  count = strtoulst (args, &args, 10);	/* Count comes first, then TP num.  */

  args = skip_spaces (args);
  if (*args && strncasecmp (args, "all", 3) == 0)
    {
      args += 3;			/* Skip special argument "all".  */
      if (*args)
	error (_("Junk at end of arguments."));

      for (breakpoint &b : all_tracepoints ())
	{
	  tracepoint &t1 = gdb::checked_static_cast<tracepoint &> (b);
	  trace_pass_set_count (&t1, count, from_tty);
	}
    }
  else if (*args == '\0')
    {
      tracepoint *t1 = get_tracepoint_by_number (&args, NULL);
      if (t1)
	trace_pass_set_count (t1, count, from_tty);
    }
  else
    {
      number_or_range_parser parser (args);
      while (!parser.finished ())
	{
	  tracepoint *t1 = get_tracepoint_by_number (&args, &parser);
	  if (t1)
	    trace_pass_set_count (t1, count, from_tty);
	}
    }
}

struct tracepoint *
get_tracepoint (int num)
{
  for (breakpoint &t : all_tracepoints ())
    if (t.number == num)
      return gdb::checked_static_cast<tracepoint *> (&t);

  return NULL;
}

/* Find the tracepoint with the given target-side number (which may be
   different from the tracepoint number after disconnecting and
   reconnecting).  */

struct tracepoint *
get_tracepoint_by_number_on_target (int num)
{
  for (breakpoint &b : all_tracepoints ())
    {
      tracepoint &t = gdb::checked_static_cast<tracepoint &> (b);

      if (t.number_on_target == num)
	return &t;
    }

  return NULL;
}

/* Utility: parse a tracepoint number and look it up in the list.
   If STATE is not NULL, use, get_number_or_range_state and ignore ARG.
   If the argument is missing, the most recent tracepoint
   (tracepoint_count) is returned.  */

struct tracepoint *
get_tracepoint_by_number (const char **arg,
			  number_or_range_parser *parser)
{
  int tpnum;
  const char *instring = arg == NULL ? NULL : *arg;

  if (parser != NULL)
    {
      gdb_assert (!parser->finished ());
      tpnum = parser->get_number ();
    }
  else if (arg == NULL || *arg == NULL || ! **arg)
    tpnum = tracepoint_count;
  else
    tpnum = get_number (arg);

  if (tpnum <= 0)
    {
      if (instring && *instring)
	gdb_printf (_("bad tracepoint number at or near '%s'\n"), 
		    instring);
      else
	gdb_printf (_("No previous tracepoint\n"));
      return NULL;
    }

  for (breakpoint &t : all_tracepoints ())
    if (t.number == tpnum)
      return gdb::checked_static_cast<tracepoint *> (&t);

  gdb_printf ("No tracepoint number %d.\n", tpnum);
  return NULL;
}

void
breakpoint::print_recreate_thread (struct ui_file *fp) const
{
  if (thread != -1)
    {
      struct thread_info *thr = find_thread_global_id (thread);
      gdb_printf (fp, " thread %s", print_full_thread_id (thr));
    }

  if (task != -1)
    gdb_printf (fp, " task %d", task);

  gdb_printf (fp, "\n");
}

/* Save information on user settable breakpoints (watchpoints, etc) to
   a new script file named FILENAME.  If FILTER is non-NULL, call it
   on each breakpoint and only include the ones for which it returns
   true.  */

static void
save_breakpoints (const char *filename, int from_tty,
		  bool (*filter) (const struct breakpoint *))
{
  bool any = false;
  int extra_trace_bits = 0;

  if (filename == 0 || *filename == 0)
    error (_("Argument required (file name in which to save)"));

  /* See if we have anything to save.  */
  for (breakpoint &tp : all_breakpoints ())
    {
      /* Skip internal and momentary breakpoints.  */
      if (!user_breakpoint_p (&tp))
	continue;

      /* If we have a filter, only save the breakpoints it accepts.  */
      if (filter && !filter (&tp))
	continue;

      any = true;

      if (is_tracepoint (&tp))
	{
	  extra_trace_bits = 1;

	  /* We can stop searching.  */
	  break;
	}
    }

  if (!any)
    {
      warning (_("Nothing to save."));
      return;
    }

  gdb::unique_xmalloc_ptr<char> expanded_filename (tilde_expand (filename));

  stdio_file fp;

  if (!fp.open (expanded_filename.get (), "w"))
    error (_("Unable to open file '%s' for saving (%s)"),
	   expanded_filename.get (), safe_strerror (errno));

  if (extra_trace_bits)
    save_trace_state_variables (&fp);

  for (breakpoint &tp : all_breakpoints ())
    {
      /* Skip internal and momentary breakpoints.  */
      if (!user_breakpoint_p (&tp))
	continue;

      /* If we have a filter, only save the breakpoints it accepts.  */
      if (filter && !filter (&tp))
	continue;

      tp.print_recreate (&fp);

      /* Note, we can't rely on tp->number for anything, as we can't
	 assume the recreated breakpoint numbers will match.  Use $bpnum
	 instead.  */

      if (tp.cond_string)
	fp.printf ("  condition $bpnum %s\n", tp.cond_string.get ());

      if (tp.ignore_count)
	fp.printf ("  ignore $bpnum %d\n", tp.ignore_count);

      if (tp.type != bp_dprintf && tp.commands)
	{
	  fp.puts ("  commands\n");

	  ui_out_redirect_pop redir (current_uiout, &fp);
	  print_command_lines (current_uiout, tp.commands.get (), 2);

	  fp.puts ("  end\n");
	}

      if (tp.enable_state == bp_disabled)
	fp.puts ("disable $bpnum\n");

      /* If this is a multi-location breakpoint, check if the locations
	 should be individually disabled.  Watchpoint locations are
	 special, and not user visible.  */
      if (!is_watchpoint (&tp) && tp.has_multiple_locations ())
	{
	  int n = 1;

	  for (bp_location &loc : tp.locations ())
	    {
	      if (!loc.enabled)
		fp.printf ("disable $bpnum.%d\n", n);

	      n++;
	    }
	}
    }

  if (extra_trace_bits && !default_collect.empty ())
    fp.printf ("set default-collect %s\n", default_collect.c_str ());

  if (from_tty)
    gdb_printf (_("Saved to file '%s'.\n"), expanded_filename.get ());
}

/* The `save breakpoints' command.  */

static void
save_breakpoints_command (const char *args, int from_tty)
{
  save_breakpoints (args, from_tty, NULL);
}

/* The `save tracepoints' command.  */

static void
save_tracepoints_command (const char *args, int from_tty)
{
  save_breakpoints (args, from_tty, is_tracepoint);
}


/* This help string is used to consolidate all the help string for specifying
   locations used by several commands.  */

#define LOCATION_SPEC_HELP_STRING \
"Linespecs are colon-separated lists of location parameters, such as\n\
source filename, function name, label name, and line number.\n\
Example: To specify the start of a label named \"the_top\" in the\n\
function \"fact\" in the file \"factorial.c\", use\n\
\"factorial.c:fact:the_top\".\n\
\n\
Address locations begin with \"*\" and specify an exact address in the\n\
program.  Example: To specify the fourth byte past the start function\n\
\"main\", use \"*main + 4\".\n\
\n\
Explicit locations are similar to linespecs but use an option/argument\n\
syntax to specify location parameters.\n\
Example: To specify the start of the label named \"the_top\" in the\n\
function \"fact\" in the file \"factorial.c\", use \"-source factorial.c\n\
-function fact -label the_top\".\n\
\n\
By default, a specified function is matched against the program's\n\
functions in all scopes.  For C++, this means in all namespaces and\n\
classes.  For Ada, this means in all packages.  E.g., in C++,\n\
\"func()\" matches \"A::func()\", \"A::B::func()\", etc.  The\n\
\"-qualified\" flag overrides this behavior, making GDB interpret the\n\
specified name as a complete fully-qualified name instead."

/* This help string is used for the break, hbreak, tbreak and thbreak
   commands.  It is defined as a macro to prevent duplication.
   COMMAND should be a string constant containing the name of the
   command.  */

#define BREAK_ARGS_HELP(command) \
command" [PROBE_MODIFIER] [LOCATION] [thread THREADNUM]\n\
\t[-force-condition] [if CONDITION]\n\
PROBE_MODIFIER shall be present if the command is to be placed in a\n\
probe point.  Accepted values are `-probe' (for a generic, automatically\n\
guessed probe type), `-probe-stap' (for a SystemTap probe) or \n\
`-probe-dtrace' (for a DTrace probe).\n\
LOCATION may be a linespec, address, or explicit location as described\n\
below.\n\
\n\
With no LOCATION, uses current execution address of the selected\n\
stack frame.  This is useful for breaking on return to a stack frame.\n\
\n\
THREADNUM is the number from \"info threads\".\n\
CONDITION is a boolean expression.\n\
\n\
With the \"-force-condition\" flag, the condition is defined even when\n\
it is invalid for all current locations.\n\
\n" LOCATION_SPEC_HELP_STRING "\n\n\
Multiple breakpoints at one place are permitted, and useful if their\n\
conditions are different.\n\
\n\
Do \"help breakpoints\" for info on other commands dealing with breakpoints."

/* List of subcommands for "catch".  */
static struct cmd_list_element *catch_cmdlist;

/* List of subcommands for "tcatch".  */
static struct cmd_list_element *tcatch_cmdlist;

void
add_catch_command (const char *name, const char *docstring,
		   cmd_func_ftype *func,
		   completer_ftype *completer,
		   void *user_data_catch,
		   void *user_data_tcatch)
{
  struct cmd_list_element *command;

  command = add_cmd (name, class_breakpoint, docstring,
		     &catch_cmdlist);
  command->func = func;
  command->set_context (user_data_catch);
  set_cmd_completer (command, completer);

  command = add_cmd (name, class_breakpoint, docstring,
		     &tcatch_cmdlist);
  command->func = func;
  command->set_context (user_data_tcatch);
  set_cmd_completer (command, completer);
}

/* False if any of the breakpoint's locations could be a location where
   functions have been inlined, true otherwise.  */

static bool
is_non_inline_function (struct breakpoint *b)
{
  /* The shared library event breakpoint is set on the address of a
     non-inline function.  */
  return (b->type == bp_shlib_event);
}

/* Nonzero if the specified PC cannot be a location where functions
   have been inlined.  */

int
pc_at_non_inline_function (const address_space *aspace, CORE_ADDR pc,
			   const target_waitstatus &ws)
{
  for (breakpoint &b : all_breakpoints ())
    {
      if (!is_non_inline_function (&b))
	continue;

      for (bp_location &bl : b.locations ())
	{
	  if (!bl.shlib_disabled
	      && bpstat_check_location (&bl, aspace, pc, ws))
	    return 1;
	}
    }

  return 0;
}

/* Remove any references to OBJFILE which is going to be freed.  */

void
breakpoint_free_objfile (struct objfile *objfile)
{
  for (bp_location *loc : all_bp_locations ())
    if (loc->symtab != NULL && loc->symtab->compunit ()->objfile () == objfile)
      loc->symtab = NULL;
}

/* Chain containing all defined "enable breakpoint" subcommands.  */

static struct cmd_list_element *enablebreaklist = NULL;

/* See breakpoint.h.  */

cmd_list_element *commands_cmd_element = nullptr;

void _initialize_breakpoint ();
void
_initialize_breakpoint ()
{
  struct cmd_list_element *c;

  gdb::observers::solib_unloaded.attach (disable_breakpoints_in_unloaded_shlib,
					 "breakpoint");
  gdb::observers::free_objfile.attach (disable_breakpoints_in_freed_objfile,
				       "breakpoint");
  gdb::observers::memory_changed.attach (invalidate_bp_value_on_memory_change,
					 "breakpoint");

  /* Don't bother to call set_breakpoint_count.  $bpnum isn't useful
     before a breakpoint is set.  */
  breakpoint_count = 0;

  tracepoint_count = 0;

  add_com ("ignore", class_breakpoint, ignore_command, _("\
Set ignore-count of breakpoint number N to COUNT.\n\
Usage is `ignore N COUNT'."));

  commands_cmd_element = add_com ("commands", class_breakpoint,
				  commands_command, _("\
Set commands to be executed when the given breakpoints are hit.\n\
Give a space-separated breakpoint list as argument after \"commands\".\n\
A list element can be a breakpoint number (e.g. `5') or a range of numbers\n\
(e.g. `5-7').\n\
With no argument, the targeted breakpoint is the last one set.\n\
The commands themselves follow starting on the next line.\n\
Type a line containing \"end\" to indicate the end of them.\n\
Give \"silent\" as the first line to make the breakpoint silent;\n\
then no output is printed when it is hit, except what the commands print."));

  const auto cc_opts = make_condition_command_options_def_group (nullptr);
  static std::string condition_command_help
    = gdb::option::build_help (_("\
Specify breakpoint number N to break only if COND is true.\n\
Usage is `condition [OPTION] N COND', where N is an integer and COND\n\
is an expression to be evaluated whenever breakpoint N is reached.\n\
\n\
Options:\n\
%OPTIONS%"), cc_opts);

  c = add_com ("condition", class_breakpoint, condition_command,
	       condition_command_help.c_str ());
  set_cmd_completer_handle_brkchars (c, condition_completer);

  c = add_com ("tbreak", class_breakpoint, tbreak_command, _("\
Set a temporary breakpoint.\n\
Like \"break\" except the breakpoint is only temporary,\n\
so it will be deleted when hit.  Equivalent to \"break\" followed\n\
by using \"enable delete\" on the breakpoint number.\n\
\n"
BREAK_ARGS_HELP ("tbreak")));
  set_cmd_completer (c, location_completer);

  c = add_com ("hbreak", class_breakpoint, hbreak_command, _("\
Set a hardware assisted breakpoint.\n\
Like \"break\" except the breakpoint requires hardware support,\n\
some target hardware may not have this support.\n\
\n"
BREAK_ARGS_HELP ("hbreak")));
  set_cmd_completer (c, location_completer);

  c = add_com ("thbreak", class_breakpoint, thbreak_command, _("\
Set a temporary hardware assisted breakpoint.\n\
Like \"hbreak\" except the breakpoint is only temporary,\n\
so it will be deleted when hit.\n\
\n"
BREAK_ARGS_HELP ("thbreak")));
  set_cmd_completer (c, location_completer);

  cmd_list_element *enable_cmd
    = add_prefix_cmd ("enable", class_breakpoint, enable_command, _("\
Enable all or some breakpoints.\n\
Usage: enable [BREAKPOINTNUM]...\n\
Give breakpoint numbers (separated by spaces) as arguments.\n\
With no subcommand, breakpoints are enabled until you command otherwise.\n\
This is used to cancel the effect of the \"disable\" command.\n\
With a subcommand you can enable temporarily."),
		      &enablelist, 1, &cmdlist);

  add_com_alias ("en", enable_cmd, class_breakpoint, 1);

  add_prefix_cmd ("breakpoints", class_breakpoint, enable_command, _("\
Enable all or some breakpoints.\n\
Usage: enable breakpoints [BREAKPOINTNUM]...\n\
Give breakpoint numbers (separated by spaces) as arguments.\n\
This is used to cancel the effect of the \"disable\" command.\n\
May be abbreviated to simply \"enable\"."),
		   &enablebreaklist, 1, &enablelist);

  add_cmd ("once", no_class, enable_once_command, _("\
Enable some breakpoints for one hit.\n\
Usage: enable breakpoints once BREAKPOINTNUM...\n\
If a breakpoint is hit while enabled in this fashion, it becomes disabled."),
	   &enablebreaklist);

  add_cmd ("delete", no_class, enable_delete_command, _("\
Enable some breakpoints and delete when hit.\n\
Usage: enable breakpoints delete BREAKPOINTNUM...\n\
If a breakpoint is hit while enabled in this fashion, it is deleted."),
	   &enablebreaklist);

  add_cmd ("count", no_class, enable_count_command, _("\
Enable some breakpoints for COUNT hits.\n\
Usage: enable breakpoints count COUNT BREAKPOINTNUM...\n\
If a breakpoint is hit while enabled in this fashion,\n\
the count is decremented; when it reaches zero, the breakpoint is disabled."),
	   &enablebreaklist);

  add_cmd ("delete", no_class, enable_delete_command, _("\
Enable some breakpoints and delete when hit.\n\
Usage: enable delete BREAKPOINTNUM...\n\
If a breakpoint is hit while enabled in this fashion, it is deleted."),
	   &enablelist);

  add_cmd ("once", no_class, enable_once_command, _("\
Enable some breakpoints for one hit.\n\
Usage: enable once BREAKPOINTNUM...\n\
If a breakpoint is hit while enabled in this fashion, it becomes disabled."),
	   &enablelist);

  add_cmd ("count", no_class, enable_count_command, _("\
Enable some breakpoints for COUNT hits.\n\
Usage: enable count COUNT BREAKPOINTNUM...\n\
If a breakpoint is hit while enabled in this fashion,\n\
the count is decremented; when it reaches zero, the breakpoint is disabled."),
	   &enablelist);

  cmd_list_element *disable_cmd
    = add_prefix_cmd ("disable", class_breakpoint, disable_command, _("\
Disable all or some breakpoints.\n\
Usage: disable [BREAKPOINTNUM]...\n\
Arguments are breakpoint numbers with spaces in between.\n\
To disable all breakpoints, give no argument.\n\
A disabled breakpoint is not forgotten, but has no effect until re-enabled."),
		      &disablelist, 1, &cmdlist);
  add_com_alias ("dis", disable_cmd, class_breakpoint, 1);
  add_com_alias ("disa", disable_cmd, class_breakpoint, 1);

  add_cmd ("breakpoints", class_breakpoint, disable_command, _("\
Disable all or some breakpoints.\n\
Usage: disable breakpoints [BREAKPOINTNUM]...\n\
Arguments are breakpoint numbers with spaces in between.\n\
To disable all breakpoints, give no argument.\n\
A disabled breakpoint is not forgotten, but has no effect until re-enabled.\n\
This command may be abbreviated \"disable\"."),
	   &disablelist);

  cmd_list_element *delete_cmd
    = add_prefix_cmd ("delete", class_breakpoint, delete_command, _("\
Delete all or some breakpoints.\n\
Usage: delete [BREAKPOINTNUM]...\n\
Arguments are breakpoint numbers with spaces in between.\n\
To delete all breakpoints, give no argument.\n\
\n\
Also a prefix command for deletion of other GDB objects."),
		      &deletelist, 1, &cmdlist);
  add_com_alias ("d", delete_cmd, class_breakpoint, 1);
  add_com_alias ("del", delete_cmd, class_breakpoint, 1);

  add_cmd ("breakpoints", class_breakpoint, delete_command, _("\
Delete all or some breakpoints or auto-display expressions.\n\
Usage: delete breakpoints [BREAKPOINTNUM]...\n\
Arguments are breakpoint numbers with spaces in between.\n\
To delete all breakpoints, give no argument.\n\
This command may be abbreviated \"delete\"."),
	   &deletelist);

  cmd_list_element *clear_cmd
   = add_com ("clear", class_breakpoint, clear_command, _("\
Clear breakpoint at specified location.\n\
Argument may be a linespec, explicit, or address location as described below.\n\
\n\
With no argument, clears all breakpoints in the line that the selected frame\n\
is executing in.\n"
"\n" LOCATION_SPEC_HELP_STRING "\n\n\
See also the \"delete\" command which clears breakpoints by number."));
  add_com_alias ("cl", clear_cmd, class_breakpoint, 1);

  cmd_list_element *break_cmd
    = add_com ("break", class_breakpoint, break_command, _("\
Set breakpoint at specified location.\n"
BREAK_ARGS_HELP ("break")));
  set_cmd_completer (break_cmd, location_completer);

  add_com_alias ("b", break_cmd, class_run, 1);
  add_com_alias ("br", break_cmd, class_run, 1);
  add_com_alias ("bre", break_cmd, class_run, 1);
  add_com_alias ("brea", break_cmd, class_run, 1);

  cmd_list_element *info_breakpoints_cmd
    = add_info ("breakpoints", info_breakpoints_command, _("\
Status of specified breakpoints (all user-settable breakpoints if no argument).\n\
The \"Type\" column indicates one of:\n\
\tbreakpoint     - normal breakpoint\n\
\twatchpoint     - watchpoint\n\
The \"Disp\" column contains one of \"keep\", \"del\", or \"dis\" to indicate\n\
the disposition of the breakpoint after it gets hit.  \"dis\" means that the\n\
breakpoint will be disabled.  The \"Address\" and \"What\" columns indicate the\n\
address and file/line number respectively.\n\
\n\
Convenience variable \"$_\" and default examine address for \"x\"\n\
are set to the address of the last breakpoint listed unless the command\n\
is prefixed with \"server \".\n\n\
Convenience variable \"$bpnum\" contains the number of the last\n\
breakpoint set."));

  add_info_alias ("b", info_breakpoints_cmd, 1);

  add_cmd ("breakpoints", class_maintenance, maintenance_info_breakpoints, _("\
Status of all breakpoints, or breakpoint number NUMBER.\n\
The \"Type\" column indicates one of:\n\
\tbreakpoint     - normal breakpoint\n\
\twatchpoint     - watchpoint\n\
\tlongjmp        - internal breakpoint used to step through longjmp()\n\
\tlongjmp resume - internal breakpoint at the target of longjmp()\n\
\tuntil          - internal breakpoint used by the \"until\" command\n\
\tfinish         - internal breakpoint used by the \"finish\" command\n\
The \"Disp\" column contains one of \"keep\", \"del\", or \"dis\" to indicate\n\
the disposition of the breakpoint after it gets hit.  \"dis\" means that the\n\
breakpoint will be disabled.  The \"Address\" and \"What\" columns indicate the\n\
address and file/line number respectively.\n\
\n\
Convenience variable \"$_\" and default examine address for \"x\"\n\
are set to the address of the last breakpoint listed unless the command\n\
is prefixed with \"server \".\n\n\
Convenience variable \"$bpnum\" contains the number of the last\n\
breakpoint set."),
	   &maintenanceinfolist);

  add_basic_prefix_cmd ("catch", class_breakpoint, _("\
Set catchpoints to catch events."),
			&catch_cmdlist,
			0/*allow-unknown*/, &cmdlist);

  add_basic_prefix_cmd ("tcatch", class_breakpoint, _("\
Set temporary catchpoints to catch events."),
			&tcatch_cmdlist,
			0/*allow-unknown*/, &cmdlist);

  const auto opts = make_watch_options_def_group (nullptr);

  static const std::string watch_help = gdb::option::build_help (_("\
Set a watchpoint for EXPRESSION.\n\
Usage: watch [-location] EXPRESSION\n\
\n\
Options:\n\
%OPTIONS%\n\
\n\
A watchpoint stops execution of your program whenever the value of\n\
an expression changes."), opts);
  c = add_com ("watch", class_breakpoint, watch_command,
	       watch_help.c_str ());
  set_cmd_completer_handle_brkchars (c, watch_command_completer);

  static const std::string rwatch_help = gdb::option::build_help (_("\
Set a read watchpoint for EXPRESSION.\n\
Usage: rwatch [-location] EXPRESSION\n\
\n\
Options:\n\
%OPTIONS%\n\
\n\
A read watchpoint stops execution of your program whenever the value of\n\
an expression is read."), opts);
  c = add_com ("rwatch", class_breakpoint, rwatch_command,
	       rwatch_help.c_str ());
  set_cmd_completer_handle_brkchars (c, watch_command_completer);

  static const std::string awatch_help = gdb::option::build_help (_("\
Set an access watchpoint for EXPRESSION.\n\
Usage: awatch [-location] EXPRESSION\n\
\n\
Options:\n\
%OPTIONS%\n\
\n\
An access watchpoint stops execution of your program whenever the value\n\
of an expression is either read or written."), opts);
  c = add_com ("awatch", class_breakpoint, awatch_command,
	       awatch_help.c_str ());
  set_cmd_completer_handle_brkchars (c, watch_command_completer);

  add_info ("watchpoints", info_watchpoints_command, _("\
Status of specified watchpoints (all watchpoints if no argument)."));

  /* XXX: cagney/2005-02-23: This should be a boolean, and should
     respond to changes - contrary to the description.  */
  add_setshow_zinteger_cmd ("can-use-hw-watchpoints", class_support,
			    &can_use_hw_watchpoints, _("\
Set debugger's willingness to use watchpoint hardware."), _("\
Show debugger's willingness to use watchpoint hardware."), _("\
If zero, gdb will not use hardware for new watchpoints, even if\n\
such is available.  (However, any hardware watchpoints that were\n\
created before setting this to nonzero, will continue to use watchpoint\n\
hardware.)"),
			    NULL,
			    show_can_use_hw_watchpoints,
			    &setlist, &showlist);

  can_use_hw_watchpoints = 1;

  /* Tracepoint manipulation commands.  */

  cmd_list_element *trace_cmd
    = add_com ("trace", class_breakpoint, trace_command, _("\
Set a tracepoint at specified location.\n\
\n"
BREAK_ARGS_HELP ("trace") "\n\
Do \"help tracepoints\" for info on other tracepoint commands."));
  set_cmd_completer (trace_cmd, location_completer);

  add_com_alias ("tp", trace_cmd, class_breakpoint, 0);
  add_com_alias ("tr", trace_cmd, class_breakpoint, 1);
  add_com_alias ("tra", trace_cmd, class_breakpoint, 1);
  add_com_alias ("trac", trace_cmd, class_breakpoint, 1);

  c = add_com ("ftrace", class_breakpoint, ftrace_command, _("\
Set a fast tracepoint at specified location.\n\
\n"
BREAK_ARGS_HELP ("ftrace") "\n\
Do \"help tracepoints\" for info on other tracepoint commands."));
  set_cmd_completer (c, location_completer);

  c = add_com ("strace", class_breakpoint, strace_command, _("\
Set a static tracepoint at location or marker.\n\
\n\
strace [LOCATION] [if CONDITION]\n\
LOCATION may be a linespec, explicit, or address location (described below) \n\
or -m MARKER_ID.\n\n\
If a marker id is specified, probe the marker with that name.  With\n\
no LOCATION, uses current execution address of the selected stack frame.\n\
Static tracepoints accept an extra collect action -- ``collect $_sdata''.\n\
This collects arbitrary user data passed in the probe point call to the\n\
tracing library.  You can inspect it when analyzing the trace buffer,\n\
by printing the $_sdata variable like any other convenience variable.\n\
\n\
CONDITION is a boolean expression.\n\
\n" LOCATION_SPEC_HELP_STRING "\n\n\
Multiple tracepoints at one place are permitted, and useful if their\n\
conditions are different.\n\
\n\
Do \"help breakpoints\" for info on other commands dealing with breakpoints.\n\
Do \"help tracepoints\" for info on other tracepoint commands."));
  set_cmd_completer (c, location_completer);

  cmd_list_element *info_tracepoints_cmd
    = add_info ("tracepoints", info_tracepoints_command, _("\
Status of specified tracepoints (all tracepoints if no argument).\n\
Convenience variable \"$tpnum\" contains the number of the\n\
last tracepoint set."));

  add_info_alias ("tp", info_tracepoints_cmd, 1);

  cmd_list_element *delete_tracepoints_cmd
    = add_cmd ("tracepoints", class_trace, delete_trace_command, _("\
Delete specified tracepoints.\n\
Arguments are tracepoint numbers, separated by spaces.\n\
No argument means delete all tracepoints."),
	   &deletelist);
  add_alias_cmd ("tr", delete_tracepoints_cmd, class_trace, 1, &deletelist);

  c = add_cmd ("tracepoints", class_trace, disable_trace_command, _("\
Disable specified tracepoints.\n\
Arguments are tracepoint numbers, separated by spaces.\n\
No argument means disable all tracepoints."),
	   &disablelist);
  deprecate_cmd (c, "disable");

  c = add_cmd ("tracepoints", class_trace, enable_trace_command, _("\
Enable specified tracepoints.\n\
Arguments are tracepoint numbers, separated by spaces.\n\
No argument means enable all tracepoints."),
	   &enablelist);
  deprecate_cmd (c, "enable");

  add_com ("passcount", class_trace, trace_pass_command, _("\
Set the passcount for a tracepoint.\n\
The trace will end when the tracepoint has been passed 'count' times.\n\
Usage: passcount COUNT TPNUM, where TPNUM may also be \"all\";\n\
if TPNUM is omitted, passcount refers to the last tracepoint defined."));

  add_basic_prefix_cmd ("save", class_breakpoint,
			_("Save breakpoint definitions as a script."),
			&save_cmdlist,
			0/*allow-unknown*/, &cmdlist);

  c = add_cmd ("breakpoints", class_breakpoint, save_breakpoints_command, _("\
Save current breakpoint definitions as a script.\n\
This includes all types of breakpoints (breakpoints, watchpoints,\n\
catchpoints, tracepoints).  Use the 'source' command in another debug\n\
session to restore them."),
	       &save_cmdlist);
  set_cmd_completer (c, filename_completer);

  cmd_list_element *save_tracepoints_cmd
    = add_cmd ("tracepoints", class_trace, save_tracepoints_command, _("\
Save current tracepoint definitions as a script.\n\
Use the 'source' command in another debug session to restore them."),
	       &save_cmdlist);
  set_cmd_completer (save_tracepoints_cmd, filename_completer);

  c = add_com_alias ("save-tracepoints", save_tracepoints_cmd, class_trace, 0);
  deprecate_cmd (c, "save tracepoints");

  add_setshow_prefix_cmd ("breakpoint", class_maintenance,
			  _("\
Breakpoint specific settings.\n\
Configure various breakpoint-specific variables such as\n\
pending breakpoint behavior."),
			  _("\
Breakpoint specific settings.\n\
Configure various breakpoint-specific variables such as\n\
pending breakpoint behavior."),
			  &breakpoint_set_cmdlist, &breakpoint_show_cmdlist,
			  &setlist, &showlist);

  add_setshow_auto_boolean_cmd ("pending", no_class,
				&pending_break_support, _("\
Set debugger's behavior regarding pending breakpoints."), _("\
Show debugger's behavior regarding pending breakpoints."), _("\
If on, an unrecognized breakpoint location will cause gdb to create a\n\
pending breakpoint.  If off, an unrecognized breakpoint location results in\n\
an error.  If auto, an unrecognized breakpoint location results in a\n\
user-query to see if a pending breakpoint should be created."),
				NULL,
				show_pending_break_support,
				&breakpoint_set_cmdlist,
				&breakpoint_show_cmdlist);

  pending_break_support = AUTO_BOOLEAN_AUTO;

  add_setshow_boolean_cmd ("auto-hw", no_class,
			   &automatic_hardware_breakpoints, _("\
Set automatic usage of hardware breakpoints."), _("\
Show automatic usage of hardware breakpoints."), _("\
If set, the debugger will automatically use hardware breakpoints for\n\
breakpoints set with \"break\" but falling in read-only memory.  If not set,\n\
a warning will be emitted for such breakpoints."),
			   NULL,
			   show_automatic_hardware_breakpoints,
			   &breakpoint_set_cmdlist,
			   &breakpoint_show_cmdlist);

  add_setshow_boolean_cmd ("always-inserted", class_support,
			   &always_inserted_mode, _("\
Set mode for inserting breakpoints."), _("\
Show mode for inserting breakpoints."), _("\
When this mode is on, breakpoints are inserted immediately as soon as\n\
they're created, kept inserted even when execution stops, and removed\n\
only when the user deletes them.  When this mode is off (the default),\n\
breakpoints are inserted only when execution continues, and removed\n\
when execution stops."),
				NULL,
				&show_always_inserted_mode,
				&breakpoint_set_cmdlist,
				&breakpoint_show_cmdlist);

  add_setshow_boolean_cmd ("breakpoint", class_maintenance,
			   &debug_breakpoint, _("\
Set breakpoint location debugging."), _("\
Show breakpoint location debugging."), _("\
When on, breakpoint location specific debugging is enabled."),
			   NULL,
			   show_debug_breakpoint,
			   &setdebuglist, &showdebuglist);

  add_setshow_enum_cmd ("condition-evaluation", class_breakpoint,
			condition_evaluation_enums,
			&condition_evaluation_mode_1, _("\
Set mode of breakpoint condition evaluation."), _("\
Show mode of breakpoint condition evaluation."), _("\
When this is set to \"host\", breakpoint conditions will be\n\
evaluated on the host's side by GDB.  When it is set to \"target\",\n\
breakpoint conditions will be downloaded to the target (if the target\n\
supports such feature) and conditions will be evaluated on the target's side.\n\
If this is set to \"auto\" (default), this will be automatically set to\n\
\"target\" if it supports condition evaluation, otherwise it will\n\
be set to \"host\"."),
			   &set_condition_evaluation_mode,
			   &show_condition_evaluation_mode,
			   &breakpoint_set_cmdlist,
			   &breakpoint_show_cmdlist);

  add_com ("break-range", class_breakpoint, break_range_command, _("\
Set a breakpoint for an address range.\n\
break-range START-LOCATION, END-LOCATION\n\
where START-LOCATION and END-LOCATION can be one of the following:\n\
  LINENUM, for that line in the current file,\n\
  FILE:LINENUM, for that line in that file,\n\
  +OFFSET, for that number of lines after the current line\n\
	   or the start of the range\n\
  FUNCTION, for the first line in that function,\n\
  FILE:FUNCTION, to distinguish among like-named static functions.\n\
  *ADDRESS, for the instruction at that address.\n\
\n\
The breakpoint will stop execution of the inferior whenever it executes\n\
an instruction at any address within the [START-LOCATION, END-LOCATION]\n\
range (including START-LOCATION and END-LOCATION)."));

  c = add_com ("dprintf", class_breakpoint, dprintf_command, _("\
Set a dynamic printf at specified location.\n\
dprintf location,format string,arg1,arg2,...\n\
location may be a linespec, explicit, or address location.\n"
"\n" LOCATION_SPEC_HELP_STRING));
  set_cmd_completer (c, location_completer);

  add_setshow_enum_cmd ("dprintf-style", class_support,
			dprintf_style_enums, &dprintf_style, _("\
Set the style of usage for dynamic printf."), _("\
Show the style of usage for dynamic printf."), _("\
This setting chooses how GDB will do a dynamic printf.\n\
If the value is \"gdb\", then the printing is done by GDB to its own\n\
console, as with the \"printf\" command.\n\
If the value is \"call\", the print is done by calling a function in your\n\
program; by default printf(), but you can choose a different function or\n\
output stream by setting dprintf-function and dprintf-channel."),
			update_dprintf_commands, NULL,
			&setlist, &showlist);

  add_setshow_string_cmd ("dprintf-function", class_support,
			  &dprintf_function, _("\
Set the function to use for dynamic printf."), _("\
Show the function to use for dynamic printf."), NULL,
			  update_dprintf_commands, NULL,
			  &setlist, &showlist);

  add_setshow_string_cmd ("dprintf-channel", class_support,
			  &dprintf_channel, _("\
Set the channel to use for dynamic printf."), _("\
Show the channel to use for dynamic printf."), NULL,
			  update_dprintf_commands, NULL,
			  &setlist, &showlist);

  add_setshow_boolean_cmd ("disconnected-dprintf", no_class,
			   &disconnected_dprintf, _("\
Set whether dprintf continues after GDB disconnects."), _("\
Show whether dprintf continues after GDB disconnects."), _("\
Use this to let dprintf commands continue to hit and produce output\n\
even if GDB disconnects or detaches from the target."),
			   NULL,
			   NULL,
			   &setlist, &showlist);

  add_com ("agent-printf", class_vars, agent_printf_command, _("\
Target agent only formatted printing, like the C \"printf\" function.\n\
Usage: agent-printf \"format string\", ARG1, ARG2, ARG3, ..., ARGN\n\
This supports most C printf format specifications, like %s, %d, etc.\n\
This is useful for formatted output in user-defined commands."));

  automatic_hardware_breakpoints = true;

  gdb::observers::about_to_proceed.attach (breakpoint_about_to_proceed,
					   "breakpoint");
  gdb::observers::thread_exit.attach (remove_threaded_breakpoints,
				      "breakpoint");
  gdb::observers::inferior_removed.attach (remove_inferior_breakpoints,
					   "breakpoint");
}
