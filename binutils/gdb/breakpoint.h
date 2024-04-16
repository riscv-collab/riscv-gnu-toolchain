/* Data structures associated with breakpoints in GDB.
   Copyright (C) 1992-2024 Free Software Foundation, Inc.

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

#if !defined (BREAKPOINT_H)
#define BREAKPOINT_H 1

#include "frame.h"
#include "value.h"
#include "ax.h"
#include "command.h"
#include "gdbsupport/break-common.h"
#include "probe.h"
#include "location.h"
#include <vector>
#include "gdbsupport/array-view.h"
#include "gdbsupport/filtered-iterator.h"
#include "gdbsupport/function-view.h"
#include "gdbsupport/next-iterator.h"
#include "gdbsupport/iterator-range.h"
#include "gdbsupport/refcounted-object.h"
#include "gdbsupport/safe-iterator.h"
#include "cli/cli-script.h"
#include "target/waitstatus.h"

struct block;
struct gdbpy_breakpoint_object;
struct gdbscm_breakpoint_object;
struct number_or_range_parser;
struct thread_info;
struct bpstat;
struct bp_location;
struct linespec_result;
struct linespec_sals;
struct inferior;

/* Enum for exception-handling support in 'catch throw', 'catch rethrow',
   'catch catch' and the MI equivalent.  */

enum exception_event_kind
{
  EX_EVENT_THROW,
  EX_EVENT_RETHROW,
  EX_EVENT_CATCH
};

/* Why are we removing the breakpoint from the target?  */

enum remove_bp_reason
{
  /* A regular remove.  Remove the breakpoint and forget everything
     about it.  */
  REMOVE_BREAKPOINT,

  /* Detach the breakpoints from a fork child.  */
  DETACH_BREAKPOINT,
};

/* This is the maximum number of bytes a breakpoint instruction can
   take.  Feel free to increase it.  It's just used in a few places to
   size arrays that should be independent of the target
   architecture.  */

#define	BREAKPOINT_MAX	16


/* Type of breakpoint.  */

enum bptype
  {
    bp_none = 0,		/* Eventpoint has been deleted */
    bp_breakpoint,		/* Normal breakpoint */
    bp_hardware_breakpoint,	/* Hardware assisted breakpoint */
    bp_single_step,		/* Software single-step */
    bp_until,			/* used by until command */
    bp_finish,			/* used by finish command */
    bp_watchpoint,		/* Watchpoint */
    bp_hardware_watchpoint,	/* Hardware assisted watchpoint */
    bp_read_watchpoint,		/* read watchpoint, (hardware assisted) */
    bp_access_watchpoint,	/* access watchpoint, (hardware assisted) */
    bp_longjmp,			/* secret breakpoint to find longjmp() */
    bp_longjmp_resume,		/* secret breakpoint to escape longjmp() */

    /* Breakpoint placed to the same location(s) like bp_longjmp but used to
       protect against stale DUMMY_FRAME.  Multiple bp_longjmp_call_dummy and
       one bp_call_dummy are chained together by related_breakpoint for each
       DUMMY_FRAME.  */
    bp_longjmp_call_dummy,

    /* An internal breakpoint that is installed on the unwinder's
       debug hook.  */
    bp_exception,
    /* An internal breakpoint that is set at the point where an
       exception will land.  */
    bp_exception_resume,

    /* Used by wait_for_inferior for stepping over subroutine calls,
       and for skipping prologues.  */
    bp_step_resume,

    /* Used by wait_for_inferior for stepping over signal
       handlers.  */
    bp_hp_step_resume,

    /* Used to detect when a watchpoint expression has gone out of
       scope.  These breakpoints are usually not visible to the user.

       This breakpoint has some interesting properties:

       1) There's always a 1:1 mapping between watchpoints
       on local variables and watchpoint_scope breakpoints.

       2) It automatically deletes itself and the watchpoint it's
       associated with when hit.

       3) It can never be disabled.  */
    bp_watchpoint_scope,

    /* The breakpoint at the end of a call dummy.  See bp_longjmp_call_dummy it
       is chained with by related_breakpoint.  */
    bp_call_dummy,

    /* A breakpoint set on std::terminate, that is used to catch
       otherwise uncaught exceptions thrown during an inferior call.  */
    bp_std_terminate,

    /* Some dynamic linkers (HP, maybe Solaris) can arrange for special
       code in the inferior to run when significant events occur in the
       dynamic linker (for example a library is loaded or unloaded).

       By placing a breakpoint in this magic code GDB will get control
       when these significant events occur.  GDB can then re-examine
       the dynamic linker's data structures to discover any newly loaded
       dynamic libraries.  */
    bp_shlib_event,

    /* Some multi-threaded systems can arrange for a location in the 
       inferior to be executed when certain thread-related events occur
       (such as thread creation or thread death).

       By placing a breakpoint at one of these locations, GDB will get
       control when these events occur.  GDB can then update its thread
       lists etc.  */

    bp_thread_event,

    /* On the same principal, an overlay manager can arrange to call a
       magic location in the inferior whenever there is an interesting
       change in overlay status.  GDB can update its overlay tables
       and fiddle with breakpoints in overlays when this breakpoint 
       is hit.  */

    bp_overlay_event, 

    /* Master copies of longjmp breakpoints.  These are always installed
       as soon as an objfile containing longjmp is loaded, but they are
       always disabled.  While necessary, temporary clones of bp_longjmp
       type will be created and enabled.  */

    bp_longjmp_master,

    /* Master copies of std::terminate breakpoints.  */
    bp_std_terminate_master,

    /* Like bp_longjmp_master, but for exceptions.  */
    bp_exception_master,

    bp_catchpoint,

    bp_tracepoint,
    bp_fast_tracepoint,
    bp_static_tracepoint,
    /* Like bp_static_tracepoint but for static markers.  */
    bp_static_marker_tracepoint,

    /* A dynamic printf stops at the given location, does a formatted
       print, then automatically continues.  (Although this is sort of
       like a macro packaging up standard breakpoint functionality,
       GDB doesn't have a way to construct types of breakpoint from
       elements of behavior.)  */
    bp_dprintf,

    /* Event for JIT compiled code generation or deletion.  */
    bp_jit_event,

    /* Breakpoint is placed at the STT_GNU_IFUNC resolver.  When hit GDB
       inserts new bp_gnu_ifunc_resolver_return at the caller.
       bp_gnu_ifunc_resolver is still being kept here as a different thread
       may still hit it before bp_gnu_ifunc_resolver_return is hit by the
       original thread.  */
    bp_gnu_ifunc_resolver,

    /* On its hit GDB now know the resolved address of the target
       STT_GNU_IFUNC function.  Associated bp_gnu_ifunc_resolver can be
       deleted now and the breakpoint moved to the target function entry
       point.  */
    bp_gnu_ifunc_resolver_return,
  };

/* States of enablement of breakpoint.  */

enum enable_state
  {
    bp_disabled,	 /* The eventpoint is inactive, and cannot
			    trigger.  */
    bp_enabled,		 /* The eventpoint is active, and can
			    trigger.  */
    bp_call_disabled,	 /* The eventpoint has been disabled while a
			    call into the inferior is "in flight",
			    because some eventpoints interfere with
			    the implementation of a call on some
			    targets.  The eventpoint will be
			    automatically enabled and reset when the
			    call "lands" (either completes, or stops
			    at another eventpoint).  */
  };


/* Disposition of breakpoint.  Ie: what to do after hitting it.  */

enum bpdisp
  {
    disp_del,			/* Delete it */
    disp_del_at_next_stop,	/* Delete at next stop, 
				   whether hit or not */
    disp_disable,		/* Disable it */
    disp_donttouch		/* Leave it alone */
  };

/* Status of breakpoint conditions used when synchronizing
   conditions with the target.  */

enum condition_status
  {
    condition_unchanged = 0,
    condition_modified,
    condition_updated
  };

/* Information used by targets to insert and remove breakpoints.  */

struct bp_target_info
{
  /* Address space at which the breakpoint was placed.  */
  struct address_space *placed_address_space;

  /* Address at which the breakpoint was placed.  This is normally
     the same as REQUESTED_ADDRESS, except when adjustment happens in
     gdbarch_breakpoint_from_pc.  The most common form of adjustment
     is stripping an alternate ISA marker from the PC which is used
     to determine the type of breakpoint to insert.  */
  CORE_ADDR placed_address;

  /* Address at which the breakpoint was requested.  */
  CORE_ADDR reqstd_address;

  /* If this is a ranged breakpoint, then this field contains the
     length of the range that will be watched for execution.  */
  int length;

  /* If the breakpoint lives in memory and reading that memory would
     give back the breakpoint, instead of the original contents, then
     the original contents are cached here.  Only SHADOW_LEN bytes of
     this buffer are valid, and only when the breakpoint is inserted.  */
  gdb_byte shadow_contents[BREAKPOINT_MAX];

  /* The length of the data cached in SHADOW_CONTENTS.  */
  int shadow_len;

  /* The breakpoint's kind.  It is used in 'kind' parameter in Z
     packets.  */
  int kind;

  /* Conditions the target should evaluate if it supports target-side
     breakpoint conditions.  These are non-owning pointers.  */
  std::vector<agent_expr *> conditions;

  /* Commands the target should evaluate if it supports target-side
     breakpoint commands.  These are non-owning pointers.  */
  std::vector<agent_expr *> tcommands;

  /* Flag that is true if the breakpoint should be left in place even
     when GDB is not connected.  */
  int persist;
};

/* GDB maintains two types of information about each breakpoint (or
   watchpoint, or other related event).  The first type corresponds
   to struct breakpoint; this is a relatively high-level structure
   which contains the source location(s), stopping conditions, user
   commands to execute when the breakpoint is hit, and so forth.

   The second type of information corresponds to struct bp_location.
   Each breakpoint has one or (eventually) more locations associated
   with it, which represent target-specific and machine-specific
   mechanisms for stopping the program.  For instance, a watchpoint
   expression may require multiple hardware watchpoints in order to
   catch all changes in the value of the expression being watched.  */

enum bp_loc_type
{
  bp_loc_software_breakpoint,
  bp_loc_hardware_breakpoint,
  bp_loc_software_watchpoint,
  bp_loc_hardware_watchpoint,
  bp_loc_tracepoint,
  bp_loc_other			/* Miscellaneous...  */
};

class bp_location : public refcounted_object, public intrusive_list_node<bp_location>
{
public:
  /* Construct a bp_location with the type inferred from OWNER's
     type.  */
  explicit bp_location (breakpoint *owner);

  /* Construct a bp_location with type TYPE.  */
  bp_location (breakpoint *owner, bp_loc_type type);

  virtual ~bp_location () = default;

  /* Type of this breakpoint location.  */
  bp_loc_type loc_type {};

  /* Each breakpoint location must belong to exactly one higher-level
     breakpoint.  This pointer is NULL iff this bp_location is no
     longer attached to a breakpoint.  For example, when a breakpoint
     is deleted, its locations may still be found in the
     moribund_locations list, or if we had stopped for it, in
     bpstats.  */
  breakpoint *owner = NULL;

  /* Conditional.  Break only if this expression's value is nonzero.
     Unlike string form of condition, which is associated with
     breakpoint, this is associated with location, since if breakpoint
     has several locations, the evaluation of expression can be
     different for different locations.  Only valid for real
     breakpoints; a watchpoint's conditional expression is stored in
     the owner breakpoint object.  */
  expression_up cond;

  /* Conditional expression in agent expression
     bytecode form.  This is used for stub-side breakpoint
     condition evaluation.  */
  agent_expr_up cond_bytecode;

  /* Signals that the condition has changed since the last time
     we updated the global location list.  This means the condition
     needs to be sent to the target again.  This is used together
     with target-side breakpoint conditions.

     condition_unchanged: It means there has been no condition changes.

     condition_modified: It means this location had its condition modified.

     condition_updated: It means we already marked all the locations that are
     duplicates of this location and thus we don't need to call
     force_breakpoint_reinsertion (...) for this location.  */

  condition_status condition_changed {};

  agent_expr_up cmd_bytecode;

  /* Signals that breakpoint conditions and/or commands need to be
     re-synced with the target.  This has no use other than
     target-side breakpoints.  */
  bool needs_update = false;

  /* This location's address is in an unloaded solib, and so this
     location should not be inserted.  It will be automatically
     enabled when that solib is loaded.  */
  bool shlib_disabled = false;

  /* Is this particular location enabled.  */
  bool enabled = false;
  
  /* Is this particular location disabled because the condition
     expression is invalid at this location.  For a location to be
     reported as enabled, the ENABLED field above has to be true *and*
     the DISABLED_BY_COND field has to be false.  */
  bool disabled_by_cond = false;

  /* True if this breakpoint is now inserted.  */
  bool inserted = false;

  /* True if this is a permanent breakpoint.  There is a breakpoint
     instruction hard-wired into the target's code.  Don't try to
     write another breakpoint instruction on top of it, or restore its
     value.  Step over it using the architecture's
     gdbarch_skip_permanent_breakpoint method.  */
  bool permanent = false;

  /* True if this is not the first breakpoint in the list
     for the given address.  location of tracepoint can _never_
     be duplicated with other locations of tracepoints and other
     kinds of breakpoints, because two locations at the same
     address may have different actions, so both of these locations
     should be downloaded and so that `tfind N' always works.  */
  bool duplicate = false;

  /* If we someday support real thread-specific breakpoints, then
     the breakpoint location will need a thread identifier.  */

  /* Data for specific breakpoint types.  These could be a union, but
     simplicity is more important than memory usage for breakpoints.  */

  /* Architecture associated with this location's address.  May be
     different from the breakpoint architecture.  */
  struct gdbarch *gdbarch = NULL;

  /* The program space associated with this breakpoint location
     address.  Note that an address space may be represented in more
     than one program space (e.g. each uClinux program will be given
     its own program space, but there will only be one address space
     for all of them), but we must not insert more than one location
     at the same address in the same address space.  */
  program_space *pspace = NULL;

  /* Note that zero is a perfectly valid code address on some platforms
     (for example, the mn10200 (OBSOLETE) and mn10300 simulators).  NULL
     is not a special value for this field.  Valid for all types except
     bp_loc_other.  */
  CORE_ADDR address = 0;

  /* For hardware watchpoints, the size of the memory region being
     watched.  For hardware ranged breakpoints, the size of the
     breakpoint range.  */
  int length = 0;

  /* Type of hardware watchpoint.  */
  target_hw_bp_type watchpoint_type {};

  /* For any breakpoint type with an address, this is the section
     associated with the address.  Used primarily for overlay
     debugging.  */
  obj_section *section = NULL;

  /* Address at which breakpoint was requested, either by the user or
     by GDB for internal breakpoints.  This will usually be the same
     as ``address'' (above) except for cases in which
     ADJUST_BREAKPOINT_ADDRESS has computed a different address at
     which to place the breakpoint in order to comply with a
     processor's architectual constraints.  */
  CORE_ADDR requested_address = 0;

  /* An additional address assigned with this location.  This is currently
     only used by STT_GNU_IFUNC resolver breakpoints to hold the address
     of the resolver function.  */
  CORE_ADDR related_address = 0;

  /* If the location comes from a probe point, this is the probe associated
     with it.  */
  bound_probe probe {};

  gdb::unique_xmalloc_ptr<char> function_name;

  /* Details of the placed breakpoint, when inserted.  */
  bp_target_info target_info {};

  /* Similarly, for the breakpoint at an overlay's LMA, if necessary.  */
  bp_target_info overlay_target_info {};

  /* In a non-stop mode, it's possible that we delete a breakpoint,
     but as we do that, some still running thread hits that breakpoint.
     For that reason, we need to keep locations belonging to deleted
     breakpoints for a bit, so that don't report unexpected SIGTRAP.
     We can't keep such locations forever, so we use a heuristic --
     after we process certain number of inferior events since
     breakpoint was deleted, we retire all locations of that breakpoint.
     This variable keeps a number of events still to go, when
     it becomes 0 this location is retired.  */
  int events_till_retirement = 0;

  /* Line number which was used to place this location.

     Breakpoint placed into a comment keeps it's user specified line number
     despite ADDRESS resolves into a different line number.  */

  int line_number = 0;

  /* Symtab which was used to place this location.  This is used
     to find the corresponding source file name.  */

  struct symtab *symtab = NULL;

  /* The symbol found by the location parser, if any.  This may be used to
     ascertain when a location spec was set at a different location than
     the one originally selected by parsing, e.g., inlined symbols.  */
  const struct symbol *symbol = NULL;

  /* Similarly, the minimal symbol found by the location parser, if
     any.  This may be used to ascertain if the location was
     originally set on a GNU ifunc symbol.  */
  const minimal_symbol *msymbol = NULL;

  /* The objfile the symbol or minimal symbol were found in.  */
  const struct objfile *objfile = NULL;

  /* Return a string representation of the bp_location.
     This is only meant to be used in debug messages.  */
  std::string to_string () const;
};

/* A policy class for bp_location reference counting.  */
struct bp_location_ref_policy
{
  static void incref (bp_location *loc)
  {
    loc->incref ();
  }

  static void decref (bp_location *loc)
  {
    gdb_assert (loc->refcount () > 0);
    loc->decref ();
    if (loc->refcount () == 0)
      delete loc;
  }
};

/* A gdb::ref_ptr that has been specialized for bp_location.  */
typedef gdb::ref_ptr<bp_location, bp_location_ref_policy>
     bp_location_ref_ptr;

/* The possible return values for print_bpstat, print_it_normal,
   print_it_done, print_it_noop.  */
enum print_stop_action
{
  /* We printed nothing or we need to do some more analysis.  */
  PRINT_UNKNOWN = -1,

  /* We printed something, and we *do* desire that something to be
     followed by a location.  */
  PRINT_SRC_AND_LOC,

  /* We printed something, and we do *not* desire that something to be
     followed by a location.  */
  PRINT_SRC_ONLY,

  /* We already printed all we needed to print, don't print anything
     else.  */
  PRINT_NOTHING
};

/* This structure is a collection of function pointers that, if available,
   will be called instead of the performing the default action for this
   bptype.  */

struct breakpoint_ops
{
  /* Create SALs from location spec, storing the result in
     linespec_result.

     For an explanation about the arguments, see the function
     `create_sals_from_location_spec_default'.

     This function is called inside `create_breakpoint'.  */
  void (*create_sals_from_location_spec) (location_spec *locspec,
					  struct linespec_result *canonical);

  /* This method will be responsible for creating a breakpoint given its SALs.
     Usually, it just calls `create_breakpoints_sal' (for ordinary
     breakpoints).  However, there may be some special cases where we might
     need to do some tweaks, e.g., see
     `strace_marker_create_breakpoints_sal'.

     This function is called inside `create_breakpoint'.  */
  void (*create_breakpoints_sal) (struct gdbarch *,
				  struct linespec_result *,
				  gdb::unique_xmalloc_ptr<char>,
				  gdb::unique_xmalloc_ptr<char>,
				  enum bptype, enum bpdisp, int, int, int,
				  int, int, int, int, unsigned);
};

enum watchpoint_triggered
{
  /* This watchpoint definitely did not trigger.  */
  watch_triggered_no = 0,

  /* Some hardware watchpoint triggered, and it might have been this
     one, but we do not know which it was.  */
  watch_triggered_unknown,

  /* This hardware watchpoint definitely did trigger.  */
  watch_triggered_yes  
};

/* Some targets (e.g., embedded PowerPC) need two debug registers to set
   a watchpoint over a memory region.  If this flag is true, GDB will use
   only one register per watchpoint, thus assuming that all accesses that
   modify a memory location happen at its starting address. */

extern bool target_exact_watchpoints;

using bp_location_list = intrusive_list<bp_location>;
using bp_location_iterator = bp_location_list::iterator;
using bp_location_range = iterator_range<bp_location_iterator>;

/* Note that the ->silent field is not currently used by any commands
   (though the code is in there if it was to be, and set_raw_breakpoint
   does set it to 0).  I implemented it because I thought it would be
   useful for a hack I had to put in; I'm going to leave it in because
   I can see how there might be times when it would indeed be useful */

/* Abstract base class representing all kinds of breakpoints.  */

struct breakpoint : public intrusive_list_node<breakpoint>
{
  breakpoint (struct gdbarch *gdbarch_, enum bptype bptype,
	      bool temp = true, const char *cond_string = nullptr);

  DISABLE_COPY_AND_ASSIGN (breakpoint);

  virtual ~breakpoint () = 0;

  /* Allocate a location for this breakpoint.  */
  virtual struct bp_location *allocate_location ();

  /* Return a range of this breakpoint's locations.  */
  bp_location_range locations () const;

  /* Add LOC to the location list of this breakpoint, sorted by address
     (using LOC.ADDRESS).

     LOC must have this breakpoint as its owner.  LOC must not already be linked
     in a location list.  */
  void add_location (bp_location &loc);

  /* Remove LOC from this breakpoint's location list.  The name is a bit funny
     because remove_location is already taken, and means something else.

     LOC must be have this breakpoints as its owner.  LOC must be linked in this
     breakpoint's location list.  */
  void unadd_location (bp_location &loc);

  /* Clear the location list of this breakpoint.  */
  void clear_locations ()
  { m_locations.clear (); }

  /* Split all locations of this breakpoint that are bound to PSPACE out of its
     location list to a separate list and return that list.  If
     PSPACE is nullptr, hoist out all locations.  */
  bp_location_list steal_locations (program_space *pspace);

  /* Return true if this breakpoint has a least one location.  */
  bool has_locations () const
  { return !m_locations.empty (); }

  /* Return true if this breakpoint has a single location.  */
  bool has_single_location () const
  {
    if (!this->has_locations ())
      return false;

    return std::next (m_locations.begin ()) == m_locations.end ();
  }

  /* Return true if this breakpoint has multiple locations.  */
  bool has_multiple_locations () const
  {
    if (!this->has_locations ())
      return false;

    return std::next (m_locations.begin ()) != m_locations.end ();
  }

  /* Return a reference to the first location of this breakpoint.  */
  bp_location &first_loc ()
  {
    gdb_assert (this->has_locations ());
    return m_locations.front ();
  }

  /* Return a reference to the first location of this breakpoint.  */
  const bp_location &first_loc () const
  {
    gdb_assert (this->has_locations ());
    return m_locations.front ();
  }

  /* Return a reference to the last location of this breakpoint.  */
  const bp_location &last_loc () const
  {
    gdb_assert (this->has_locations ());
    return m_locations.back ();
  }

  /* Reevaluate a breakpoint.  This is necessary after symbols change
     (e.g., an executable or DSO was loaded, or the inferior just
     started).  */
  virtual void re_set ()
  {
    /* Nothing to re-set.  */
  }

  /* Insert the breakpoint or watchpoint or activate the catchpoint.
     Return 0 for success, 1 if the breakpoint, watchpoint or
     catchpoint type is not supported, -1 for failure.  */
  virtual int insert_location (struct bp_location *);

  /* Remove the breakpoint/catchpoint that was previously inserted
     with the "insert" method above.  Return 0 for success, 1 if the
     breakpoint, watchpoint or catchpoint type is not supported,
     -1 for failure.  */
  virtual int remove_location (struct bp_location *,
			       enum remove_bp_reason reason);

  /* Return true if it the target has stopped due to hitting
     breakpoint location BL.  This function does not check if we
     should stop, only if BL explains the stop.  ASPACE is the address
     space in which the event occurred, BP_ADDR is the address at
     which the inferior stopped, and WS is the target_waitstatus
     describing the event.  */
  virtual int breakpoint_hit (const struct bp_location *bl,
			      const address_space *aspace,
			      CORE_ADDR bp_addr,
			      const target_waitstatus &ws);

  /* Check internal conditions of the breakpoint referred to by BS.
     If we should not stop for this breakpoint, set BS->stop to
     false.  */
  virtual void check_status (struct bpstat *bs)
  {
    /* Always stop.  */
  }

  /* Tell how many hardware resources (debug registers) are needed
     for this breakpoint.  If this function is not provided, then
     the breakpoint or watchpoint needs one debug register.  */
  virtual int resources_needed (const struct bp_location *);

  /* The normal print routine for this breakpoint, called when we
     hit it.  */
  virtual enum print_stop_action print_it (const bpstat *bs) const;

  /* Display information about this breakpoint, for "info
     breakpoints".  Returns false if this method should use the
     default behavior.  */
  virtual bool print_one (const bp_location **) const
  {
    return false;
  }

  /* Display extra information about this breakpoint, below the normal
     breakpoint description in "info breakpoints".

     In the example below, the "address range" line was printed
     by ranged_breakpoint::print_one_detail.

     (gdb) info breakpoints
     Num     Type           Disp Enb Address    What
     2       hw breakpoint  keep y              in main at test-watch.c:70
	     address range: [0x10000458, 0x100004c7]

   */
  virtual void print_one_detail (struct ui_out *) const
  {
    /* Nothing.  */
  }

  /* Display information about this breakpoint after setting it
     (roughly speaking; this is called from "mention").  */
  virtual void print_mention () const;

  /* Print to FP the CLI command that recreates this breakpoint.  */
  virtual void print_recreate (struct ui_file *fp) const;

  /* Return true if this breakpoint explains a signal.  See
     bpstat_explains_signal.  */
  virtual bool explains_signal (enum gdb_signal)
  {
    return true;
  }

  /* Called after evaluating the breakpoint's condition,
     and only if it evaluated true.  */
  virtual void after_condition_true (struct bpstat *bs)
  {
    /* Nothing to do.  */
  }

  /* Type of breakpoint.  */
  bptype type = bp_none;
  /* Zero means disabled; remember the info but don't break here.  */
  enum enable_state enable_state = bp_enabled;
  /* What to do with this breakpoint after we hit it.  */
  bpdisp disposition = disp_del;
  /* Number assigned to distinguish breakpoints.  */
  int number = 0;

  /* True means a silent breakpoint (don't print frame info if we stop
     here).  */
  bool silent = false;
  /* True means display ADDR_STRING to the user verbatim.  */
  bool display_canonical = false;
  /* Number of stops at this breakpoint that should be continued
     automatically before really stopping.  */
  int ignore_count = 0;

  /* Number of stops at this breakpoint before it will be
     disabled.  */
  int enable_count = 0;

  /* Chain of command lines to execute when this breakpoint is
     hit.  */
  counted_command_line commands;
  /* Stack depth (address of frame).  If nonzero, break only if fp
     equals this.  */
  struct frame_id frame_id = null_frame_id;

  /* The program space used to set the breakpoint.  This is only set
     for breakpoints which are specific to a program space; for
     non-thread-specific ordinary breakpoints this is NULL.  */
  program_space *pspace = NULL;

  /* The location specification we used to set the breakpoint.  */
  location_spec_up locspec;

  /* The filter that should be passed to decode_line_full when
     re-setting this breakpoint.  This may be NULL.  */
  gdb::unique_xmalloc_ptr<char> filter;

  /* For a ranged breakpoint, the location specification we used to
     find the end of the range.  */
  location_spec_up locspec_range_end;

  /* Architecture we used to set the breakpoint.  */
  struct gdbarch *gdbarch;
  /* Language we used to set the breakpoint.  */
  enum language language;
  /* Input radix we used to set the breakpoint.  */
  int input_radix;
  /* String form of the breakpoint condition (malloc'd), or NULL if
     there is no condition.  */
  gdb::unique_xmalloc_ptr<char> cond_string;

  /* String form of extra parameters, or NULL if there are none.
     Malloc'd.  */
  gdb::unique_xmalloc_ptr<char> extra_string;

  /* Holds the address of the related watchpoint_scope breakpoint when
     using watchpoints on local variables (might the concept of a
     related breakpoint be useful elsewhere, if not just call it the
     watchpoint_scope breakpoint or something like that.  FIXME).  */
  breakpoint *related_breakpoint;

  /* Thread number for thread-specific breakpoint, or -1 if don't
     care.  */
  int thread = -1;

  /* Inferior number for inferior-specific breakpoint, or -1 if this
     breakpoint is for all inferiors.  */
  int inferior = -1;

  /* Ada task number for task-specific breakpoint, or -1 if don't
     care.  */
  int task = -1;

  /* Count of the number of times this breakpoint was taken, dumped
     with the info, but not used for anything else.  Useful for seeing
     how many times you hit a break prior to the program aborting, so
     you can back up to just before the abort.  */
  int hit_count = 0;

  /* Is breakpoint's condition not yet parsed because we found no
     location initially so had no context to parse the condition
     in.  */
  int condition_not_parsed = 0;

  /* With a Python scripting enabled GDB, store a reference to the
     Python object that has been associated with this breakpoint.
     This is always NULL for a GDB that is not script enabled.  It can
     sometimes be NULL for enabled GDBs as not all breakpoint types
     are tracked by the scripting language API.  */
  gdbpy_breakpoint_object *py_bp_object = NULL;

  /* Same as py_bp_object, but for Scheme.  */
  gdbscm_breakpoint_object *scm_bp_object = NULL;

protected:

  /* Helper for breakpoint_ops->print_recreate implementations.  Prints
     the "thread" or "task" condition of B, and then a newline.

     Necessary because most breakpoint implementations accept
     thread/task conditions at the end of the spec line, like "break foo
     thread 1", which needs outputting before any breakpoint-type
     specific extra command necessary for B's recreation.  */
  void print_recreate_thread (struct ui_file *fp) const;

  /* Location(s) associated with this high-level breakpoint.  */
  bp_location_list m_locations;
};

/* Abstract base class representing code breakpoints.  User "break"
   breakpoints, internal and momentary breakpoints, etc.  IOW, any
   kind of breakpoint whose locations are created from SALs.  */
struct code_breakpoint : public breakpoint
{
  using breakpoint::breakpoint;

  /* Create a breakpoint with SALS as locations.  Use LOCATION as a
     description of the location, and COND_STRING as condition
     expression.  If LOCATION is NULL then create an "address
     location" from the address in the SAL.  */
  code_breakpoint (struct gdbarch *gdbarch, bptype type,
		   gdb::array_view<const symtab_and_line> sals,
		   location_spec_up &&locspec,
		   gdb::unique_xmalloc_ptr<char> filter,
		   gdb::unique_xmalloc_ptr<char> cond_string,
		   gdb::unique_xmalloc_ptr<char> extra_string,
		   enum bpdisp disposition,
		   int thread, int task, int inferior, int ignore_count,
		   int from_tty,
		   int enabled, unsigned flags,
		   int display_canonical);

  ~code_breakpoint () override = 0;

  /* Add a location for SAL to this breakpoint.  */
  bp_location *add_location (const symtab_and_line &sal);

  void re_set () override;
  int insert_location (struct bp_location *) override;
  int remove_location (struct bp_location *,
		       enum remove_bp_reason reason) override;
  int breakpoint_hit (const struct bp_location *bl,
		      const address_space *aspace,
		      CORE_ADDR bp_addr,
		      const target_waitstatus &ws) override;

protected:

  /* Given the location spec, this method decodes it and returns the
     SAL locations related to it.  For ordinary breakpoints, it calls
     `decode_line_full'.  If SEARCH_PSPACE is not NULL, symbol search
     is restricted to just that program space.

     This function is called inside `location_spec_to_sals'.  */
  virtual std::vector<symtab_and_line> decode_location_spec
    (location_spec *locspec,
     struct program_space *search_pspace);

  /* Helper method that does the basic work of re_set.  */
  void re_set_default ();

  /* Find the SaL locations corresponding to the given LOCATION.
     On return, FOUND will be 1 if any SaL was found, zero otherwise.  */

  std::vector<symtab_and_line> location_spec_to_sals
       (location_spec *locspec,
	struct program_space *search_pspace,
	int *found);

  /* Helper for breakpoint and tracepoint breakpoint->mention
     callbacks.  */
  void say_where () const;
};

/* An instance of this type is used to represent a watchpoint,
   a.k.a. a data breakpoint.  */

struct watchpoint : public breakpoint
{
  using breakpoint::breakpoint;

  void re_set () override;
  int insert_location (struct bp_location *) override;
  int remove_location (struct bp_location *,
		       enum remove_bp_reason reason) override;
  int breakpoint_hit (const struct bp_location *bl,
		      const address_space *aspace,
		      CORE_ADDR bp_addr,
		      const target_waitstatus &ws) override;
  void check_status (struct bpstat *bs) override;
  int resources_needed (const struct bp_location *) override;

  /* Tell whether we can downgrade from a hardware watchpoint to a software
     one.  If not, the user will not be able to enable the watchpoint when
     there are not enough hardware resources available.  */
  virtual bool works_in_software_mode () const;

  enum print_stop_action print_it (const bpstat *bs) const override;
  void print_mention () const override;
  void print_recreate (struct ui_file *fp) const override;
  bool explains_signal (enum gdb_signal) override;

  /* Destructor for WATCHPOINT.  */
  ~watchpoint ();

  /* String form of exp to use for displaying to the user (malloc'd),
     or NULL if none.  */
  gdb::unique_xmalloc_ptr<char> exp_string;
  /* String form to use for reparsing of EXP (malloc'd) or NULL.  */
  gdb::unique_xmalloc_ptr<char> exp_string_reparse;

  /* The expression we are watching, or NULL if not a watchpoint.  */
  expression_up exp;
  /* The largest block within which it is valid, or NULL if it is
     valid anywhere (e.g. consists just of global symbols).  */
  const struct block *exp_valid_block;
  /* The conditional expression if any.  */
  expression_up cond_exp;
  /* The largest block within which it is valid, or NULL if it is
     valid anywhere (e.g. consists just of global symbols).  */
  const struct block *cond_exp_valid_block;
  /* Value of the watchpoint the last time we checked it, or NULL when
     we do not know the value yet or the value was not readable.  VAL
     is never lazy.  */
  value_ref_ptr val;

  /* True if VAL is valid.  If VAL_VALID is set but VAL is NULL,
     then an error occurred reading the value.  */
  bool val_valid;

  /* When watching the location of a bitfield, contains the offset and size of
     the bitfield.  Otherwise contains 0.  */
  int val_bitpos;
  int val_bitsize;

  /* Holds the frame address which identifies the frame this
     watchpoint should be evaluated in, or `null' if the watchpoint
     should be evaluated on the outermost frame.  */
  struct frame_id watchpoint_frame;

  /* Holds the thread which identifies the frame this watchpoint
     should be considered in scope for, or `null_ptid' if the
     watchpoint should be evaluated in all threads.  */
  ptid_t watchpoint_thread;

  /* For hardware watchpoints, the triggered status according to the
     hardware.  */
  enum watchpoint_triggered watchpoint_triggered;

  /* Whether this watchpoint is exact (see
     target_exact_watchpoints).  */
  int exact;

  /* The mask address for a masked hardware watchpoint.  */
  CORE_ADDR hw_wp_mask;
};

/* Return true if BPT is either a software breakpoint or a hardware
   breakpoint.  */

extern bool is_breakpoint (const struct breakpoint *bpt);

/* Return true if BPT is of any watchpoint kind, hardware or
   software.  */

extern bool is_watchpoint (const struct breakpoint *bpt);

/* Return true if BPT is a C++ exception catchpoint (catch
   catch/throw/rethrow).  */

extern bool is_exception_catchpoint (breakpoint *bp);

/* An instance of this type is used to represent all kinds of
   tracepoints.  */

struct tracepoint : public code_breakpoint
{
  using code_breakpoint::code_breakpoint;

  int breakpoint_hit (const struct bp_location *bl,
		      const address_space *aspace, CORE_ADDR bp_addr,
		      const target_waitstatus &ws) override;
  void print_one_detail (struct ui_out *uiout) const override;
  void print_mention () const override;
  void print_recreate (struct ui_file *fp) const override;

  /* Number of times this tracepoint should single-step and collect
     additional data.  */
  long step_count = 0;

  /* Number of times this tracepoint should be hit before
     disabling/ending.  */
  int pass_count = 0;

  /* The number of the tracepoint on the target.  */
  int number_on_target = 0;

  /* The total space taken by all the trace frames for this
     tracepoint.  */
  ULONGEST traceframe_usage = 0;

  /* The static tracepoint marker id, if known.  */
  std::string static_trace_marker_id;

  /* LTTng/UST allow more than one marker with the same ID string,
     although it unadvised because it confuses tools.  When setting
     static tracepoints by marker ID, this will record the index in
     the array of markers we found for the given marker ID for which
     this static tracepoint corresponds.  When resetting breakpoints,
     we will use this index to try to find the same marker again.  */
  int static_trace_marker_id_idx = 0;
};

/* The abstract base class for catchpoints.  */

struct catchpoint : public breakpoint
{
  /* If TEMP is true, then make the breakpoint temporary.  If
     COND_STRING is not NULL, then store it in the breakpoint.  */
  catchpoint (struct gdbarch *gdbarch, bool temp, const char *cond_string);

  ~catchpoint () override = 0;
};


/* The following stuff is an abstract data type "bpstat" ("breakpoint
   status").  This provides the ability to determine whether we have
   stopped at a breakpoint, and what we should do about it.  */

/* Clears a chain of bpstat, freeing storage
   of each.  */
extern void bpstat_clear (bpstat **);

/* Return a copy of a bpstat.  Like "bs1 = bs2" but all storage that
   is part of the bpstat is copied as well.  */
extern bpstat *bpstat_copy (bpstat *);

/* Build the (raw) bpstat chain for the stop information given by ASPACE,
   BP_ADDR, and WS.  Returns the head of the bpstat chain.  */

extern bpstat *build_bpstat_chain (const address_space *aspace,
				  CORE_ADDR bp_addr,
				  const target_waitstatus &ws);

/* Get a bpstat associated with having just stopped at address
   BP_ADDR in thread PTID.  STOP_CHAIN may be supplied as a previously
   computed stop chain or NULL, in which case the stop chain will be
   computed using build_bpstat_chain.

   Determine whether we stopped at a breakpoint, etc, or whether we
   don't understand this stop.  Result is a chain of bpstat's such
   that:

   if we don't understand the stop, the result is a null pointer.

   if we understand why we stopped, the result is not null.

   Each element of the chain refers to a particular breakpoint or
   watchpoint at which we have stopped.  (We may have stopped for
   several reasons concurrently.)

   Each element of the chain has valid next, breakpoint_at,
   commands, FIXME??? fields.

   watchpoints_triggered must be called beforehand to set up each
   watchpoint's watchpoint_triggered value.

*/

extern bpstat *bpstat_stop_status (const address_space *aspace,
				  CORE_ADDR pc, thread_info *thread,
				  const target_waitstatus &ws,
				  bpstat *stop_chain = nullptr);

/* Like bpstat_stop_status, but clears all watchpoints'
   watchpoint_triggered flag.  Unlike with bpstat_stop_status, there's
   no need to call watchpoint_triggered beforehand.  You'll typically
   use this variant when handling a known-non-watchpoint event, like a
   fork or exec event.  */

extern bpstat *bpstat_stop_status_nowatch (const address_space *aspace,
					   CORE_ADDR bp_addr,
					   thread_info *thread,
					   const target_waitstatus &ws);



/* This bpstat_what stuff tells wait_for_inferior what to do with a
   breakpoint (a challenging task).

   The enum values order defines priority-like order of the actions.
   Once you've decided that some action is appropriate, you'll never
   go back and decide something of a lower priority is better.  Each
   of these actions is mutually exclusive with the others.  That
   means, that if you find yourself adding a new action class here and
   wanting to tell GDB that you have two simultaneous actions to
   handle, something is wrong, and you probably don't actually need a
   new action type.

   Note that a step resume breakpoint overrides another breakpoint of
   signal handling (see comment in wait_for_inferior at where we set
   the step_resume breakpoint).  */

enum bpstat_what_main_action
  {
    /* Perform various other tests; that is, this bpstat does not
       say to perform any action (e.g. failed watchpoint and nothing
       else).  */
    BPSTAT_WHAT_KEEP_CHECKING,

    /* Remove breakpoints, single step once, then put them back in and
       go back to what we were doing.  It's possible that this should
       be removed from the main_action and put into a separate field,
       to more cleanly handle
       BPSTAT_WHAT_CLEAR_LONGJMP_RESUME_SINGLE.  */
    BPSTAT_WHAT_SINGLE,

    /* Set longjmp_resume breakpoint, remove all other breakpoints,
       and continue.  The "remove all other breakpoints" part is
       required if we are also stepping over another breakpoint as
       well as doing the longjmp handling.  */
    BPSTAT_WHAT_SET_LONGJMP_RESUME,

    /* Clear longjmp_resume breakpoint, then handle as
       BPSTAT_WHAT_KEEP_CHECKING.  */
    BPSTAT_WHAT_CLEAR_LONGJMP_RESUME,

    /* Clear step resume breakpoint, and keep checking.  */
    BPSTAT_WHAT_STEP_RESUME,

    /* Rather than distinguish between noisy and silent stops here, it
       might be cleaner to have bpstat_print make that decision (also
       taking into account stop_print_frame and source_only).  But the
       implications are a bit scary (interaction with auto-displays,
       etc.), so I won't try it.  */

    /* Stop silently.  */
    BPSTAT_WHAT_STOP_SILENT,

    /* Stop and print.  */
    BPSTAT_WHAT_STOP_NOISY,

    /* Clear step resume breakpoint, and keep checking.  High-priority
       step-resume breakpoints are used when even if there's a user
       breakpoint at the current PC when we set the step-resume
       breakpoint, we don't want to re-handle any breakpoint other
       than the step-resume when it's hit; instead we want to move
       past the breakpoint.  This is used in the case of skipping
       signal handlers.  */
    BPSTAT_WHAT_HP_STEP_RESUME,
  };

/* An enum indicating the kind of "stack dummy" stop.  This is a bit
   of a misnomer because only one kind of truly a stack dummy.  */
enum stop_stack_kind
  {
    /* We didn't stop at a stack dummy breakpoint.  */
    STOP_NONE = 0,

    /* Stopped at a stack dummy.  */
    STOP_STACK_DUMMY,

    /* Stopped at std::terminate.  */
    STOP_STD_TERMINATE
  };

struct bpstat_what
  {
    enum bpstat_what_main_action main_action;

    /* Did we hit a call dummy breakpoint?  This only goes with a
       main_action of BPSTAT_WHAT_STOP_SILENT or
       BPSTAT_WHAT_STOP_NOISY (the concept of continuing from a call
       dummy without popping the frame is not a useful one).  */
    enum stop_stack_kind call_dummy;

    /* Used for BPSTAT_WHAT_SET_LONGJMP_RESUME and
       BPSTAT_WHAT_CLEAR_LONGJMP_RESUME.  True if we are handling a
       longjmp, false if we are handling an exception.  */
    bool is_longjmp;
  };

/* Tell what to do about this bpstat.  */
struct bpstat_what bpstat_what (bpstat *);

/* Run breakpoint event callbacks associated with the breakpoints that
   triggered.  */
extern void bpstat_run_callbacks (bpstat *bs_head);

/* Find the bpstat associated with a breakpoint.  NULL otherwise.  */
bpstat *bpstat_find_breakpoint (bpstat *, struct breakpoint *);

/* True if a signal that we got in target_wait() was due to
   circumstances explained by the bpstat; the signal is therefore not
   random.  */
extern bool bpstat_explains_signal (bpstat *, enum gdb_signal);

/* True if this bpstat causes a stop.  */
extern bool bpstat_causes_stop (bpstat *);

/* True if we should step constantly (e.g. watchpoints on machines
   without hardware support).  This isn't related to a specific bpstat,
   just to things like whether watchpoints are set.  */
extern bool bpstat_should_step ();

/* Print a message indicating what happened.  */
extern enum print_stop_action bpstat_print (bpstat *bs, target_waitkind kind);

/* Put in *NUM the breakpoint number of the first breakpoint we are
   stopped at.  *BSP upon return is a bpstat which points to the
   remaining breakpoints stopped at (but which is not guaranteed to be
   good for anything but further calls to bpstat_num).

   Return 0 if passed a bpstat which does not indicate any breakpoints.
   Return -1 if stopped at a breakpoint that has been deleted since
   we set it.
   Return 1 otherwise.  */
extern int bpstat_num (bpstat **, int *);

/* If BS indicates a breakpoint and this breakpoint has several code locations,
   return the location number of BS, otherwise return 0.  */

extern int bpstat_locno (const bpstat *bs);

/* Print BS breakpoint number optionally followed by a . and breakpoint locno.

   For a breakpoint with only one code location, outputs the signed field
   "bkptno" breakpoint number of BS (as returned by bpstat_num).
   If BS has several code locations, outputs a '.' character followed by
   the signed field "locno" (as returned by bpstat_locno).  */

extern void print_num_locno (const bpstat *bs, struct ui_out *);

/* Perform actions associated with the stopped inferior.  Actually, we
   just use this for breakpoint commands.  Perhaps other actions will
   go here later, but this is executed at a late time (from the
   command loop).  */
extern void bpstat_do_actions (void);

/* Modify all entries of STOP_BPSTAT of INFERIOR_PTID so that the actions will
   not be performed.  */
extern void bpstat_clear_actions (void);

/* Implementation:  */

/* Values used to tell the printing routine how to behave for this
   bpstat.  */
enum bp_print_how
  {
    /* This is used when we want to do a normal printing of the reason
       for stopping.  The output will depend on the type of eventpoint
       we are dealing with.  This is the default value, most commonly
       used.  */
    print_it_normal,
    /* This is used when nothing should be printed for this bpstat
       entry.  */
    print_it_noop,
    /* This is used when everything which needs to be printed has
       already been printed.  But we still want to print the frame.  */
    print_it_done
  };

struct bpstat
  {
    bpstat ();
    bpstat (struct bp_location *bl, bpstat ***bs_link_pointer);

    bpstat (const bpstat &);
    bpstat &operator= (const bpstat &) = delete;

    /* Linked list because there can be more than one breakpoint at
       the same place, and a bpstat reflects the fact that all have
       been hit.  */
    bpstat *next;

    /* Location that caused the stop.  Locations are refcounted, so
       this will never be NULL.  Note that this location may end up
       detached from a breakpoint, but that does not necessary mean
       that the struct breakpoint is gone.  E.g., consider a
       watchpoint with a condition that involves an inferior function
       call.  Watchpoint locations are recreated often (on resumes,
       hence on infcalls too).  Between creating the bpstat and after
       evaluating the watchpoint condition, this location may hence
       end up detached from its original owner watchpoint, even though
       the watchpoint is still listed.  If it's condition evaluates as
       true, we still want this location to cause a stop, and we will
       still need to know which watchpoint it was originally attached.
       What this means is that we should not (in most cases) follow
       the `bpstat->bp_location->owner' link, but instead use the
       `breakpoint_at' field below.  */
    bp_location_ref_ptr bp_location_at;

    /* Breakpoint that caused the stop.  This is nullified if the
       breakpoint ends up being deleted.  See comments on
       `bp_location_at' above for why do we need this field instead of
       following the location's owner.  */
    struct breakpoint *breakpoint_at;

    /* The associated command list.  */
    counted_command_line commands;

    /* Old value associated with a watchpoint.  */
    value_ref_ptr old_val;

    /* True if this breakpoint tells us to print the frame.  */
    bool print;

    /* True if this breakpoint tells us to stop.  */
    bool stop;

    /* Tell bpstat_print and print_bp_stop_message how to print stuff
       associated with this element of the bpstat chain.  */
    enum bp_print_how print_it;
  };

enum inf_context
  {
    inf_starting,
    inf_running,
    inf_exited,
    inf_execd
  };

/* The possible return values for breakpoint_here_p.
   We guarantee that zero always means "no breakpoint here".  */
enum breakpoint_here
  {
    no_breakpoint_here = 0,
    ordinary_breakpoint_here,
    permanent_breakpoint_here
  };


/* Prototypes for breakpoint-related functions.  */

extern enum breakpoint_here breakpoint_here_p (const address_space *,
					       CORE_ADDR);

/* Return true if an enabled breakpoint exists in the range defined by
   ADDR and LEN, in ASPACE.  */
extern int breakpoint_in_range_p (const address_space *aspace,
				  CORE_ADDR addr, ULONGEST len);

extern int moribund_breakpoint_here_p (const address_space *, CORE_ADDR);

extern int breakpoint_inserted_here_p (const address_space *,
				       CORE_ADDR);

extern int software_breakpoint_inserted_here_p (const address_space *,
						CORE_ADDR);

/* Return non-zero iff there is a hardware breakpoint inserted at
   PC.  */
extern int hardware_breakpoint_inserted_here_p (const address_space *,
						CORE_ADDR);

/* Check whether any location of BP is inserted at PC.  */

extern int breakpoint_has_location_inserted_here (struct breakpoint *bp,
						  const address_space *aspace,
						  CORE_ADDR pc);

extern int single_step_breakpoint_inserted_here_p (const address_space *,
						   CORE_ADDR);

/* Returns true if there's a hardware watchpoint or access watchpoint
   inserted in the range defined by ADDR and LEN.  */
extern int hardware_watchpoint_inserted_in_range (const address_space *,
						  CORE_ADDR addr,
						  ULONGEST len);

/* Returns true if {ASPACE1,ADDR1} and {ASPACE2,ADDR2} represent the
   same breakpoint location.  In most targets, this can only be true
   if ASPACE1 matches ASPACE2.  On targets that have global
   breakpoints, the address space doesn't really matter.  */

extern int breakpoint_address_match (const address_space *aspace1,
				     CORE_ADDR addr1,
				     const address_space *aspace2,
				     CORE_ADDR addr2);

extern void until_break_command (const char *, int, int);

/* Initialize a struct bp_location.  */

extern void update_breakpoint_locations
  (code_breakpoint *b,
   struct program_space *filter_pspace,
   gdb::array_view<const symtab_and_line> sals,
   gdb::array_view<const symtab_and_line> sals_end);

extern void breakpoint_re_set (void);

extern void breakpoint_re_set_thread (struct breakpoint *);

extern void delete_breakpoint (struct breakpoint *);

struct breakpoint_deleter
{
  void operator() (struct breakpoint *b) const
  {
    delete_breakpoint (b);
  }
};

typedef std::unique_ptr<struct breakpoint, breakpoint_deleter> breakpoint_up;

extern breakpoint_up set_momentary_breakpoint
  (struct gdbarch *, struct symtab_and_line, struct frame_id, enum bptype);

extern breakpoint_up set_momentary_breakpoint_at_pc
  (struct gdbarch *, CORE_ADDR pc, enum bptype type);

extern struct breakpoint *clone_momentary_breakpoint (struct breakpoint *bpkt);

extern void set_ignore_count (int, int, int);

extern void breakpoint_init_inferior (enum inf_context);

extern void breakpoint_auto_delete (bpstat *);

/* Return the chain of command lines to execute when this breakpoint
   is hit.  */
extern struct command_line *breakpoint_commands (struct breakpoint *b);

/* Return a string image of DISP.  The string is static, and thus should
   NOT be deallocated after use.  */
const char *bpdisp_text (enum bpdisp disp);

extern void break_command (const char *, int);

extern void watch_command_wrapper (const char *, int, bool);
extern void awatch_command_wrapper (const char *, int, bool);
extern void rwatch_command_wrapper (const char *, int, bool);
extern void tbreak_command (const char *, int);

extern const struct breakpoint_ops code_breakpoint_ops;

/* Arguments to pass as context to some catch command handlers.  */
#define CATCH_PERMANENT ((void *) (uintptr_t) 0)
#define CATCH_TEMPORARY ((void *) (uintptr_t) 1)

/* Like add_cmd, but add the command to both the "catch" and "tcatch"
   lists, and pass some additional user data to the command
   function.  */

extern void
  add_catch_command (const char *name, const char *docstring,
		     cmd_func_ftype *func,
		     completer_ftype *completer,
		     void *user_data_catch,
		     void *user_data_tcatch);

/* Add breakpoint B on the breakpoint list, and notify the user, the
   target and breakpoint_created observers of its existence.  If
   INTERNAL is non-zero, the breakpoint number will be allocated from
   the internal breakpoint count.  If UPDATE_GLL is non-zero,
   update_global_location_list will be called.

   Takes ownership of B, and returns a non-owning reference to it.  */

extern breakpoint *install_breakpoint
  (int internal, std::unique_ptr<breakpoint> &&b, int update_gll);

/* Returns the breakpoint ops appropriate for use with with LOCSPEC
   and according to IS_TRACEPOINT.  Use this to ensure, for example,
   that you pass the correct ops to create_breakpoint for probe
   location specs.  If LOCSPEC is NULL, returns
   code_breakpoint_ops.  */

extern const struct breakpoint_ops *breakpoint_ops_for_location_spec
  (const location_spec *locspec, bool is_tracepoint);

/* Flags that can be passed down to create_breakpoint, etc., to affect
   breakpoint creation in several ways.  */

enum breakpoint_create_flags
  {
    /* We're adding a breakpoint to our tables that is already
       inserted in the target.  */
    CREATE_BREAKPOINT_FLAGS_INSERTED = 1 << 0
  };

/* Set a breakpoint.  This function is shared between CLI and MI
   functions for setting a breakpoint at LOCSPEC.

   This function has two major modes of operations, selected by the
   PARSE_EXTRA parameter.

   If PARSE_EXTRA is zero, LOCSPEC is just the breakpoint's location
   spec, with condition, thread, and extra string specified by the
   COND_STRING, THREAD, and EXTRA_STRING parameters.

   If PARSE_EXTRA is non-zero, this function will attempt to extract
   the condition, thread, and extra string from EXTRA_STRING, ignoring
   the similarly named parameters.

   If FORCE_CONDITION is true, the condition is accepted even when it is
   invalid at all of the locations.  However, if PARSE_EXTRA is non-zero,
   the FORCE_CONDITION parameter is ignored and the corresponding argument
   is parsed from EXTRA_STRING.

   If INTERNAL is non-zero, the breakpoint number will be allocated
   from the internal breakpoint count.

   Returns true if any breakpoint was created; false otherwise.  */

extern int create_breakpoint (struct gdbarch *gdbarch,
			      struct location_spec *locspec,
			      const char *cond_string, int thread,
			      int inferior,
			      const char *extra_string,
			      bool force_condition,
			      int parse_extra,
			      int tempflag, enum bptype wanted_type,
			      int ignore_count,
			      enum auto_boolean pending_break_support,
			      const struct breakpoint_ops *ops,
			      int from_tty,
			      int enabled,
			      int internal, unsigned flags);

extern void insert_breakpoints (void);

extern int remove_breakpoints (void);

/* Remove breakpoints of inferior INF.  */

extern void remove_breakpoints_inf (inferior *inf);

/* This function can be used to update the breakpoint package's state
   after an exec() system call has been executed.

   This function causes the following:

   - All eventpoints are marked "not inserted".
   - All eventpoints with a symbolic address are reset such that
   the symbolic address must be reevaluated before the eventpoints
   can be reinserted.
   - The solib breakpoints are explicitly removed from the breakpoint
   list.
   - A step-resume breakpoint, if any, is explicitly removed from the
   breakpoint list.
   - All eventpoints without a symbolic address are removed from the
   breakpoint list.  */
extern void update_breakpoints_after_exec (void);

/* This function can be used to physically remove hardware breakpoints
   and watchpoints from the specified traced inferior process, without
   modifying the breakpoint package's state.  This can be useful for
   those targets which support following the processes of a fork() or
   vfork() system call, when one of the resulting two processes is to
   be detached and allowed to run free.

   It is an error to use this function on the process whose id is
   inferior_ptid.  */
extern int detach_breakpoints (ptid_t ptid);

/* This function is called when program space PSPACE is about to be
   deleted.  It takes care of updating breakpoints to not reference
   this PSPACE anymore.  */
extern void breakpoint_program_space_exit (struct program_space *pspace);

extern void set_longjmp_breakpoint (struct thread_info *tp,
				    struct frame_id frame);
extern void delete_longjmp_breakpoint (int thread);

/* Mark all longjmp breakpoints from THREAD for later deletion.  */
extern void delete_longjmp_breakpoint_at_next_stop (int thread);

extern struct breakpoint *set_longjmp_breakpoint_for_call_dummy (void);
extern void check_longjmp_breakpoint_for_call_dummy (struct thread_info *tp);

extern void enable_overlay_breakpoints (void);
extern void disable_overlay_breakpoints (void);

extern void set_std_terminate_breakpoint (void);
extern void delete_std_terminate_breakpoint (void);

/* These functions respectively disable or reenable all currently
   enabled watchpoints.  When disabled, the watchpoints are marked
   call_disabled.  When re-enabled, they are marked enabled.

   The intended client of these functions is call_function_by_hand.

   The inferior must be stopped, and all breakpoints removed, when
   these functions are used.

   The need for these functions is that on some targets (e.g., HP-UX),
   gdb is unable to unwind through the dummy frame that is pushed as
   part of the implementation of a call command.  Watchpoints can
   cause the inferior to stop in places where this frame is visible,
   and that can cause execution control to become very confused.

   Note that if a user sets breakpoints in an interactively called
   function, the call_disabled watchpoints will have been re-enabled
   when the first such breakpoint is reached.  However, on targets
   that are unable to unwind through the call dummy frame, watches
   of stack-based storage may then be deleted, because gdb will
   believe that their watched storage is out of scope.  (Sigh.) */
extern void disable_watchpoints_before_interactive_call_start (void);

extern void enable_watchpoints_after_interactive_call_stop (void);

/* These functions disable and re-enable all breakpoints during
   inferior startup.  They are intended to be called from solib
   code where necessary.  This is needed on platforms where the
   main executable is relocated at some point during startup
   processing, making breakpoint addresses invalid.

   If additional breakpoints are created after the routine
   disable_breakpoints_before_startup but before the routine
   enable_breakpoints_after_startup was called, they will also
   be marked as disabled.  */
extern void disable_breakpoints_before_startup (void);
extern void enable_breakpoints_after_startup (void);

/* For script interpreters that need to define breakpoint commands
   after they've already read the commands into a struct
   command_line.  */
extern enum command_control_type commands_from_control_command
  (const char *arg, struct command_line *cmd);

extern void clear_breakpoint_hit_counts (void);

extern struct breakpoint *get_breakpoint (int num);

/* The following are for displays, which aren't really breakpoints,
   but here is as good a place as any for them.  */

extern void disable_current_display (void);

extern void do_displays (void);

extern void disable_display (int);

extern void clear_displays (void);

extern void disable_breakpoint (struct breakpoint *);

extern void enable_breakpoint (struct breakpoint *);

extern void breakpoint_set_commands (struct breakpoint *b, 
				     counted_command_line &&commands);

extern void breakpoint_set_silent (struct breakpoint *b, int silent);

/* Set the thread for this breakpoint.  If THREAD is -1, make the
   breakpoint work for any thread.  Passing a value other than -1 for
   THREAD should only be done if b->task is 0; it is not valid to try and
   set both a thread and task restriction on a breakpoint.  */

extern void breakpoint_set_thread (struct breakpoint *b, int thread);

/* Set the inferior for breakpoint B to INFERIOR.  If INFERIOR is -1, make
   the breakpoint work for any inferior.  */

extern void breakpoint_set_inferior (struct breakpoint *b, int inferior);

/* Set the task for this breakpoint.  If TASK is -1, make the breakpoint
   work for any task.  Passing a value other than -1 for TASK should only
   be done if b->thread is -1; it is not valid to try and set both a thread
   and task restriction on a breakpoint.  */

extern void breakpoint_set_task (struct breakpoint *b, int task);

/* Clear the "inserted" flag in all breakpoints.  */
extern void mark_breakpoints_out (void);

extern struct breakpoint *create_jit_event_breakpoint (struct gdbarch *,
						       CORE_ADDR);

extern struct breakpoint *create_solib_event_breakpoint (struct gdbarch *,
							 CORE_ADDR);

/* Create an solib event breakpoint at ADDRESS in the current program
   space, and immediately try to insert it.  Returns a pointer to the
   breakpoint on success.  Deletes the new breakpoint and returns NULL
   if inserting the breakpoint fails.  */
extern struct breakpoint *create_and_insert_solib_event_breakpoint
  (struct gdbarch *gdbarch, CORE_ADDR address);

extern struct breakpoint *create_thread_event_breakpoint (struct gdbarch *,
							  CORE_ADDR);

extern void remove_jit_event_breakpoints (void);

extern void remove_solib_event_breakpoints (void);

/* Mark solib event breakpoints of the current program space with
   delete at next stop disposition.  */
extern void remove_solib_event_breakpoints_at_next_stop (void);

extern void disable_breakpoints_in_shlibs (void);

/* This function returns true if B is a catchpoint.  */

extern bool is_catchpoint (struct breakpoint *b);

/* Shared helper function (MI and CLI) for creating and installing
   a shared object event catchpoint.  If IS_LOAD is true then
   the events to be caught are load events, otherwise they are
   unload events.  If IS_TEMP is true the catchpoint is a
   temporary one.  If ENABLED is true the catchpoint is
   created in an enabled state.  */

extern void add_solib_catchpoint (const char *arg, bool is_load, bool is_temp,
				  bool enabled);

/* Create and insert a new software single step breakpoint for the
   current thread.  May be called multiple times; each time will add a
   new location to the set of potential addresses the next instruction
   is at.  */
extern void insert_single_step_breakpoint (struct gdbarch *,
					   const address_space *,
					   CORE_ADDR);

/* Insert all software single step breakpoints for the current frame.
   Return true if any software single step breakpoints are inserted,
   otherwise, return false.  */
extern int insert_single_step_breakpoints (struct gdbarch *);

/* Check whether any hardware watchpoints have triggered or not,
   according to the target, and record it in each watchpoint's
   'watchpoint_triggered' field.  */
int watchpoints_triggered (const target_waitstatus &);

/* Helper for transparent breakpoint hiding for memory read and write
   routines.

   Update one of READBUF or WRITEBUF with either the shadows
   (READBUF), or the breakpoint instructions (WRITEBUF) of inserted
   breakpoints at the memory range defined by MEMADDR and extending
   for LEN bytes.  If writing, then WRITEBUF is a copy of WRITEBUF_ORG
   on entry.*/
extern void breakpoint_xfer_memory (gdb_byte *readbuf, gdb_byte *writebuf,
				    const gdb_byte *writebuf_org,
				    ULONGEST memaddr, LONGEST len);

/* Return true if breakpoints should be inserted now.  That'll be the
   case if either:

    - the target has global breakpoints.

    - "breakpoint always-inserted" is on, and the target has
      execution.

    - threads are executing.
*/
extern int breakpoints_should_be_inserted_now (void);

/* Called each time new event from target is processed.
   Retires previously deleted breakpoint locations that
   in our opinion won't ever trigger.  */
extern void breakpoint_retire_moribund (void);

/* Set break condition of breakpoint B to EXP.
   If FORCE, define the condition even if it is invalid in
   all of the breakpoint locations.  */
extern void set_breakpoint_condition (struct breakpoint *b, const char *exp,
				      int from_tty, bool force);

/* Set break condition for the breakpoint with number BPNUM to EXP.
   Raise an error if no breakpoint with the given number is found.
   Also raise an error if the breakpoint already has stop conditions.
   If FORCE, define the condition even if it is invalid in
   all of the breakpoint locations.  */
extern void set_breakpoint_condition (int bpnum, const char *exp,
				      int from_tty, bool force);

/* Checks if we are catching syscalls or not.  */
extern bool catch_syscall_enabled ();

/* Checks if we are catching syscalls with the specific
   syscall_number.  Used for "filtering" the catchpoints.
   Returns false if not, true if we are.  */
extern bool catching_syscall_number (int syscall_number);

/* Return a tracepoint with the given number if found.  */
extern struct tracepoint *get_tracepoint (int num);

extern struct tracepoint *get_tracepoint_by_number_on_target (int num);

/* Find a tracepoint by parsing a number in the supplied string.  */
extern struct tracepoint *
  get_tracepoint_by_number (const char **arg,
			    number_or_range_parser *parser);

/* Return true if B is of tracepoint kind.  */

extern bool is_tracepoint (const struct breakpoint *b);

/* Return a vector of all static tracepoints defined at ADDR.  */
extern std::vector<breakpoint *> static_tracepoints_here (CORE_ADDR addr);

/* Create an instance of this to start registering breakpoint numbers
   for a later "commands" command.  */

class scoped_rbreak_breakpoints
{
public:

  scoped_rbreak_breakpoints ();
  ~scoped_rbreak_breakpoints ();

  DISABLE_COPY_AND_ASSIGN (scoped_rbreak_breakpoints);
};

/* Breakpoint linked list iterator.  */

using breakpoint_list = intrusive_list<breakpoint>;

using breakpoint_iterator = breakpoint_list::iterator;

/* Breakpoint linked list range.  */

using breakpoint_range = iterator_range<breakpoint_iterator>;

/* Return a range to iterate over all breakpoints.  */

breakpoint_range all_breakpoints ();

/* Breakpoint linked list range, safe against deletion of the current
   breakpoint while iterating.  */

using breakpoint_safe_range = basic_safe_range<breakpoint_range>;

/* Return a range to iterate over all breakpoints.  This range is safe against
   deletion of the current breakpoint while iterating.  */

breakpoint_safe_range all_breakpoints_safe ();

/* Breakpoint filter to only keep tracepoints.  */

struct tracepoint_filter
{
  bool operator() (breakpoint &b)
  { return is_tracepoint (&b); }
};

/* Breakpoint linked list iterator, filtering to only keep tracepoints.  */

using tracepoint_iterator
  = filtered_iterator<breakpoint_iterator, tracepoint_filter>;

/* Breakpoint linked list range, filtering to only keep tracepoints.  */

using tracepoint_range = iterator_range<tracepoint_iterator>;

/* Return a range to iterate over all tracepoints.  */

tracepoint_range all_tracepoints ();

/* Return a range to iterate over all breakpoint locations.  */

const std::vector<bp_location *> &all_bp_locations ();

/* Nonzero if the specified PC cannot be a location where functions
   have been inlined.  */

extern int pc_at_non_inline_function (const address_space *aspace,
				      CORE_ADDR pc,
				      const target_waitstatus &ws);

extern int user_breakpoint_p (struct breakpoint *);

/* Return true if this breakpoint is pending, false if not.  */
extern int pending_breakpoint_p (struct breakpoint *);

/* Attempt to determine architecture of location identified by SAL.  */
extern struct gdbarch *get_sal_arch (struct symtab_and_line sal);

extern void breakpoint_free_objfile (struct objfile *objfile);

extern const char *ep_parse_optional_if_clause (const char **arg);

/* Print the "Thread ID hit" part of "Thread ID hit Breakpoint N" to
   UIOUT iff debugging multiple threads.  */
extern void maybe_print_thread_hit_breakpoint (struct ui_out *uiout);

/* Print the specified breakpoint.  */
extern void print_breakpoint (breakpoint *bp);

/* Command element for the 'commands' command.  */
extern cmd_list_element *commands_cmd_element;

/* Whether to use the fixed output when printing information about a
   multi-location breakpoint (see PR 9659).  */

extern bool fix_multi_location_breakpoint_output_globally;

/* Whether to use the fixed output when printing information about
   commands attached to a breakpoint.  */

extern bool fix_breakpoint_script_output_globally;

/* Deal with "catch catch", "catch throw", and "catch rethrow" commands and
   the MI equivalents.  Sets up to catch events of type EX_EVENT.  When
   TEMPFLAG is true only the next matching event is caught after which the
   catch-point is deleted.  If REGEX is not NULL then only exceptions whose
   type name matches REGEX will trigger the event.  */

extern void catch_exception_event (enum exception_event_kind ex_event,
				   const char *regex, bool tempflag,
				   int from_tty);

/* A helper function that prints a shared library stopped event.
   IS_CATCHPOINT is true if the event is due to a "catch load"
   catchpoint, false otherwise.  */

extern void print_solib_event (bool is_catchpoint);

/* Print a message describing any user-breakpoints set at PC.  This
   concerns with logical breakpoints, so we match program spaces, not
   address spaces.  */

extern void describe_other_breakpoints (struct gdbarch *,
					struct program_space *, CORE_ADDR,
					struct obj_section *, int);

/* Enable or disable a breakpoint location LOC.  ENABLE
   specifies whether to enable or disable.  */

extern void enable_disable_bp_location (bp_location *loc, bool enable);


/* Notify interpreters and observers that breakpoint B was modified.  */

extern void notify_breakpoint_modified (breakpoint *b);

#endif /* !defined (BREAKPOINT_H) */
