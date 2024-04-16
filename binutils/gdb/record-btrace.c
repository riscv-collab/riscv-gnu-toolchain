/* Branch trace support for GDB, the GNU debugger.

   Copyright (C) 2013-2024 Free Software Foundation, Inc.

   Contributed by Intel Corp. <markus.t.metzger@intel.com>

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
#include "record.h"
#include "record-btrace.h"
#include "gdbthread.h"
#include "target.h"
#include "gdbcmd.h"
#include "disasm.h"
#include "observable.h"
#include "cli/cli-utils.h"
#include "source.h"
#include "ui-out.h"
#include "symtab.h"
#include "filenames.h"
#include "regcache.h"
#include "frame-unwind.h"
#include "hashtab.h"
#include "infrun.h"
#include "gdbsupport/event-loop.h"
#include "inf-loop.h"
#include "inferior.h"
#include <algorithm>
#include "gdbarch.h"
#include "cli/cli-style.h"
#include "async-event.h"
#include <forward_list>
#include "objfiles.h"
#include "interps.h"

static const target_info record_btrace_target_info = {
  "record-btrace",
  N_("Branch tracing target"),
  N_("Collect control-flow trace and provide the execution history.")
};

/* The target_ops of record-btrace.  */

class record_btrace_target final : public target_ops
{
public:
  const target_info &info () const override
  { return record_btrace_target_info; }

  strata stratum () const override { return record_stratum; }

  void close () override;
  void async (bool) override;

  void detach (inferior *inf, int from_tty) override
  { record_detach (this, inf, from_tty); }

  void disconnect (const char *, int) override;

  void mourn_inferior () override
  { record_mourn_inferior (this); }

  void kill () override
  { record_kill (this); }

  enum record_method record_method (ptid_t ptid) override;

  void stop_recording () override;
  void info_record () override;

  void insn_history (int size, gdb_disassembly_flags flags) override;
  void insn_history_from (ULONGEST from, int size,
			  gdb_disassembly_flags flags) override;
  void insn_history_range (ULONGEST begin, ULONGEST end,
			   gdb_disassembly_flags flags) override;
  void call_history (int size, record_print_flags flags) override;
  void call_history_from (ULONGEST begin, int size, record_print_flags flags)
    override;
  void call_history_range (ULONGEST begin, ULONGEST end, record_print_flags flags)
    override;

  bool record_is_replaying (ptid_t ptid) override;
  bool record_will_replay (ptid_t ptid, int dir) override;
  void record_stop_replaying () override;

  enum target_xfer_status xfer_partial (enum target_object object,
					const char *annex,
					gdb_byte *readbuf,
					const gdb_byte *writebuf,
					ULONGEST offset, ULONGEST len,
					ULONGEST *xfered_len) override;

  int insert_breakpoint (struct gdbarch *,
			 struct bp_target_info *) override;
  int remove_breakpoint (struct gdbarch *, struct bp_target_info *,
			 enum remove_bp_reason) override;

  void fetch_registers (struct regcache *, int) override;

  void store_registers (struct regcache *, int) override;
  void prepare_to_store (struct regcache *) override;

  const struct frame_unwind *get_unwinder () override;

  const struct frame_unwind *get_tailcall_unwinder () override;

  void resume (ptid_t, int, enum gdb_signal) override;
  ptid_t wait (ptid_t, struct target_waitstatus *, target_wait_flags) override;

  void stop (ptid_t) override;
  void update_thread_list () override;
  bool thread_alive (ptid_t ptid) override;
  void goto_record_begin () override;
  void goto_record_end () override;
  void goto_record (ULONGEST insn) override;

  bool can_execute_reverse () override;

  bool stopped_by_sw_breakpoint () override;
  bool supports_stopped_by_sw_breakpoint () override;

  bool stopped_by_hw_breakpoint () override;
  bool supports_stopped_by_hw_breakpoint () override;

  enum exec_direction_kind execution_direction () override;
  void prepare_to_generate_core () override;
  void done_generating_core () override;
};

static record_btrace_target record_btrace_ops;

/* Initialize the record-btrace target ops.  */

/* Token associated with a new-thread observer enabling branch tracing
   for the new thread.  */
static const gdb::observers::token record_btrace_thread_observer_token {};

/* Memory access types used in set/show record btrace replay-memory-access.  */
static const char replay_memory_access_read_only[] = "read-only";
static const char replay_memory_access_read_write[] = "read-write";
static const char *const replay_memory_access_types[] =
{
  replay_memory_access_read_only,
  replay_memory_access_read_write,
  NULL
};

/* The currently allowed replay memory access type.  */
static const char *replay_memory_access = replay_memory_access_read_only;

/* The cpu state kinds.  */
enum record_btrace_cpu_state_kind
{
  CS_AUTO,
  CS_NONE,
  CS_CPU
};

/* The current cpu state.  */
static enum record_btrace_cpu_state_kind record_btrace_cpu_state = CS_AUTO;

/* The current cpu for trace decode.  */
static struct btrace_cpu record_btrace_cpu;

/* Command lists for "set/show record btrace".  */
static struct cmd_list_element *set_record_btrace_cmdlist;
static struct cmd_list_element *show_record_btrace_cmdlist;

/* The execution direction of the last resume we got.  See record-full.c.  */
static enum exec_direction_kind record_btrace_resume_exec_dir = EXEC_FORWARD;

/* The async event handler for reverse/replay execution.  */
static struct async_event_handler *record_btrace_async_inferior_event_handler;

/* A flag indicating that we are currently generating a core file.  */
static int record_btrace_generating_corefile;

/* The current branch trace configuration.  */
static struct btrace_config record_btrace_conf;

/* Command list for "record btrace".  */
static struct cmd_list_element *record_btrace_cmdlist;

/* Command lists for "set/show record btrace bts".  */
static struct cmd_list_element *set_record_btrace_bts_cmdlist;
static struct cmd_list_element *show_record_btrace_bts_cmdlist;

/* Command lists for "set/show record btrace pt".  */
static struct cmd_list_element *set_record_btrace_pt_cmdlist;
static struct cmd_list_element *show_record_btrace_pt_cmdlist;

/* Command list for "set record btrace cpu".  */
static struct cmd_list_element *set_record_btrace_cpu_cmdlist;

/* Print a record-btrace debug message.  Use do ... while (0) to avoid
   ambiguities when used in if statements.  */

#define DEBUG(msg, args...)						\
  do									\
    {									\
      if (record_debug != 0)						\
	gdb_printf (gdb_stdlog,						\
		    "[record-btrace] " msg "\n", ##args);		\
    }									\
  while (0)


/* Return the cpu configured by the user.  Returns NULL if the cpu was
   configured as auto.  */
const struct btrace_cpu *
record_btrace_get_cpu (void)
{
  switch (record_btrace_cpu_state)
    {
    case CS_AUTO:
      return nullptr;

    case CS_NONE:
      record_btrace_cpu.vendor = CV_UNKNOWN;
      [[fallthrough]];
    case CS_CPU:
      return &record_btrace_cpu;
    }

  error (_("Internal error: bad record btrace cpu state."));
}

/* Update the branch trace for the current thread and return a pointer to its
   thread_info.

   Throws an error if there is no thread or no trace.  This function never
   returns NULL.  */

static struct thread_info *
require_btrace_thread (void)
{
  DEBUG ("require");

  if (inferior_ptid == null_ptid)
    error (_("No thread."));

  thread_info *tp = inferior_thread ();

  validate_registers_access ();

  btrace_fetch (tp, record_btrace_get_cpu ());

  if (btrace_is_empty (tp))
    error (_("No trace."));

  return tp;
}

/* Update the branch trace for the current thread and return a pointer to its
   branch trace information struct.

   Throws an error if there is no thread or no trace.  This function never
   returns NULL.  */

static struct btrace_thread_info *
require_btrace (void)
{
  struct thread_info *tp;

  tp = require_btrace_thread ();

  return &tp->btrace;
}

/* The new thread observer.  */

static void
record_btrace_on_new_thread (struct thread_info *tp)
{
  /* Ignore this thread if its inferior is not recorded by us.  */
  target_ops *rec = tp->inf->target_at (record_stratum);
  if (rec != &record_btrace_ops)
    return;

  try
    {
      btrace_enable (tp, &record_btrace_conf);
    }
  catch (const gdb_exception_error &error)
    {
      warning ("%s", error.what ());
    }
}

/* Enable automatic tracing of new threads.  */

static void
record_btrace_auto_enable (void)
{
  DEBUG ("attach thread observer");

  gdb::observers::new_thread.attach (record_btrace_on_new_thread,
				     record_btrace_thread_observer_token,
				     "record-btrace");
}

/* Disable automatic tracing of new threads.  */

static void
record_btrace_auto_disable (void)
{
  DEBUG ("detach thread observer");

  gdb::observers::new_thread.detach (record_btrace_thread_observer_token);
}

/* The record-btrace async event handler function.  */

static void
record_btrace_handle_async_inferior_event (gdb_client_data data)
{
  inferior_event_handler (INF_REG_EVENT);
}

/* See record-btrace.h.  */

void
record_btrace_push_target (void)
{
  const char *format;

  record_btrace_auto_enable ();

  current_inferior ()->push_target (&record_btrace_ops);

  record_btrace_async_inferior_event_handler
    = create_async_event_handler (record_btrace_handle_async_inferior_event,
				  NULL, "record-btrace");
  record_btrace_generating_corefile = 0;

  format = btrace_format_short_string (record_btrace_conf.format);
  interps_notify_record_changed (current_inferior (), 1, "btrace", format);
}

/* Disable btrace on a set of threads on scope exit.  */

struct scoped_btrace_disable
{
  scoped_btrace_disable () = default;

  DISABLE_COPY_AND_ASSIGN (scoped_btrace_disable);

  ~scoped_btrace_disable ()
  {
    for (thread_info *tp : m_threads)
      btrace_disable (tp);
  }

  void add_thread (thread_info *thread)
  {
    m_threads.push_front (thread);
  }

  void discard ()
  {
    m_threads.clear ();
  }

private:
  std::forward_list<thread_info *> m_threads;
};

/* Open target record-btrace.  */

static void
record_btrace_target_open (const char *args, int from_tty)
{
  /* If we fail to enable btrace for one thread, disable it for the threads for
     which it was successfully enabled.  */
  scoped_btrace_disable btrace_disable;

  DEBUG ("open");

  record_preopen ();

  if (!target_has_execution ())
    error (_("The program is not being run."));

  for (thread_info *tp : current_inferior ()->non_exited_threads ())
    if (args == NULL || *args == 0 || number_is_in_list (args, tp->global_num))
      {
	btrace_enable (tp, &record_btrace_conf);

	btrace_disable.add_thread (tp);
      }

  record_btrace_push_target ();

  btrace_disable.discard ();
}

/* The stop_recording method of target record-btrace.  */

void
record_btrace_target::stop_recording ()
{
  DEBUG ("stop recording");

  record_btrace_auto_disable ();

  for (thread_info *tp : current_inferior ()->non_exited_threads ())
    if (tp->btrace.target != NULL)
      btrace_disable (tp);
}

/* The disconnect method of target record-btrace.  */

void
record_btrace_target::disconnect (const char *args,
				  int from_tty)
{
  struct target_ops *beneath = this->beneath ();

  /* Do not stop recording, just clean up GDB side.  */
  current_inferior ()->unpush_target (this);

  /* Forward disconnect.  */
  beneath->disconnect (args, from_tty);
}

/* The close method of target record-btrace.  */

void
record_btrace_target::close ()
{
  if (record_btrace_async_inferior_event_handler != NULL)
    delete_async_event_handler (&record_btrace_async_inferior_event_handler);

  /* Make sure automatic recording gets disabled even if we did not stop
     recording before closing the record-btrace target.  */
  record_btrace_auto_disable ();

  /* We should have already stopped recording.
     Tear down btrace in case we have not.  */
  for (thread_info *tp : current_inferior ()->non_exited_threads ())
    btrace_teardown (tp);
}

/* The async method of target record-btrace.  */

void
record_btrace_target::async (bool enable)
{
  if (enable)
    mark_async_event_handler (record_btrace_async_inferior_event_handler);
  else
    clear_async_event_handler (record_btrace_async_inferior_event_handler);

  this->beneath ()->async (enable);
}

/* Adjusts the size and returns a human readable size suffix.  */

static const char *
record_btrace_adjust_size (unsigned int *size)
{
  unsigned int sz;

  sz = *size;

  if ((sz & ((1u << 30) - 1)) == 0)
    {
      *size = sz >> 30;
      return "GB";
    }
  else if ((sz & ((1u << 20) - 1)) == 0)
    {
      *size = sz >> 20;
      return "MB";
    }
  else if ((sz & ((1u << 10) - 1)) == 0)
    {
      *size = sz >> 10;
      return "kB";
    }
  else
    return "";
}

/* Print a BTS configuration.  */

static void
record_btrace_print_bts_conf (const struct btrace_config_bts *conf)
{
  const char *suffix;
  unsigned int size;

  size = conf->size;
  if (size > 0)
    {
      suffix = record_btrace_adjust_size (&size);
      gdb_printf (_("Buffer size: %u%s.\n"), size, suffix);
    }
}

/* Print an Intel Processor Trace configuration.  */

static void
record_btrace_print_pt_conf (const struct btrace_config_pt *conf)
{
  const char *suffix;
  unsigned int size;

  size = conf->size;
  if (size > 0)
    {
      suffix = record_btrace_adjust_size (&size);
      gdb_printf (_("Buffer size: %u%s.\n"), size, suffix);
    }
}

/* Print a branch tracing configuration.  */

static void
record_btrace_print_conf (const struct btrace_config *conf)
{
  gdb_printf (_("Recording format: %s.\n"),
	      btrace_format_string (conf->format));

  switch (conf->format)
    {
    case BTRACE_FORMAT_NONE:
      return;

    case BTRACE_FORMAT_BTS:
      record_btrace_print_bts_conf (&conf->bts);
      return;

    case BTRACE_FORMAT_PT:
      record_btrace_print_pt_conf (&conf->pt);
      return;
    }

  internal_error (_("Unknown branch trace format."));
}

/* The info_record method of target record-btrace.  */

void
record_btrace_target::info_record ()
{
  struct btrace_thread_info *btinfo;
  const struct btrace_config *conf;
  struct thread_info *tp;
  unsigned int insns, calls, gaps;

  DEBUG ("info");

  if (inferior_ptid == null_ptid)
    error (_("No thread."));

  tp = inferior_thread ();

  validate_registers_access ();

  btinfo = &tp->btrace;

  conf = ::btrace_conf (btinfo);
  if (conf != NULL)
    record_btrace_print_conf (conf);

  btrace_fetch (tp, record_btrace_get_cpu ());

  insns = 0;
  calls = 0;
  gaps = 0;

  if (!btrace_is_empty (tp))
    {
      struct btrace_call_iterator call;
      struct btrace_insn_iterator insn;

      btrace_call_end (&call, btinfo);
      btrace_call_prev (&call, 1);
      calls = btrace_call_number (&call);

      btrace_insn_end (&insn, btinfo);
      insns = btrace_insn_number (&insn);

      /* If the last instruction is not a gap, it is the current instruction
	 that is not actually part of the record.  */
      if (btrace_insn_get (&insn) != NULL)
	insns -= 1;

      gaps = btinfo->ngaps;
    }

  gdb_printf (_("Recorded %u instructions in %u functions (%u gaps) "
		"for thread %s (%s).\n"), insns, calls, gaps,
	      print_thread_id (tp),
	      target_pid_to_str (tp->ptid).c_str ());

  if (btrace_is_replaying (tp))
    gdb_printf (_("Replay in progress.  At instruction %u.\n"),
		btrace_insn_number (btinfo->replay));
}

/* Print a decode error.  */

static void
btrace_ui_out_decode_error (struct ui_out *uiout, int errcode,
			    enum btrace_format format)
{
  const char *errstr = btrace_decode_error (format, errcode);

  uiout->text (_("["));
  /* ERRCODE > 0 indicates notifications on BTRACE_FORMAT_PT.  */
  if (!(format == BTRACE_FORMAT_PT && errcode > 0))
    {
      uiout->text (_("decode error ("));
      uiout->field_signed ("errcode", errcode);
      uiout->text (_("): "));
    }
  uiout->text (errstr);
  uiout->text (_("]\n"));
}

/* A range of source lines.  */

struct btrace_line_range
{
  /* The symtab this line is from.  */
  struct symtab *symtab;

  /* The first line (inclusive).  */
  int begin;

  /* The last line (exclusive).  */
  int end;
};

/* Construct a line range.  */

static struct btrace_line_range
btrace_mk_line_range (struct symtab *symtab, int begin, int end)
{
  struct btrace_line_range range;

  range.symtab = symtab;
  range.begin = begin;
  range.end = end;

  return range;
}

/* Add a line to a line range.  */

static struct btrace_line_range
btrace_line_range_add (struct btrace_line_range range, int line)
{
  if (range.end <= range.begin)
    {
      /* This is the first entry.  */
      range.begin = line;
      range.end = line + 1;
    }
  else if (line < range.begin)
    range.begin = line;
  else if (range.end < line)
    range.end = line;

  return range;
}

/* Return non-zero if RANGE is empty, zero otherwise.  */

static int
btrace_line_range_is_empty (struct btrace_line_range range)
{
  return range.end <= range.begin;
}

/* Return non-zero if LHS contains RHS, zero otherwise.  */

static int
btrace_line_range_contains_range (struct btrace_line_range lhs,
				  struct btrace_line_range rhs)
{
  return ((lhs.symtab == rhs.symtab)
	  && (lhs.begin <= rhs.begin)
	  && (rhs.end <= lhs.end));
}

/* Find the line range associated with PC.  */

static struct btrace_line_range
btrace_find_line_range (CORE_ADDR pc)
{
  struct btrace_line_range range;
  const linetable_entry *lines;
  const linetable *ltable;
  struct symtab *symtab;
  int nlines, i;

  symtab = find_pc_line_symtab (pc);
  if (symtab == NULL)
    return btrace_mk_line_range (NULL, 0, 0);

  ltable = symtab->linetable ();
  if (ltable == NULL)
    return btrace_mk_line_range (symtab, 0, 0);

  nlines = ltable->nitems;
  lines = ltable->item;
  if (nlines <= 0)
    return btrace_mk_line_range (symtab, 0, 0);

  struct objfile *objfile = symtab->compunit ()->objfile ();
  unrelocated_addr unrel_pc
    = unrelocated_addr (pc - objfile->text_section_offset ());

  range = btrace_mk_line_range (symtab, 0, 0);
  for (i = 0; i < nlines - 1; i++)
    {
      /* The test of is_stmt here was added when the is_stmt field was
	 introduced to the 'struct linetable_entry' structure.  This
	 ensured that this loop maintained the same behaviour as before we
	 introduced is_stmt.  That said, it might be that we would be
	 better off not checking is_stmt here, this would lead to us
	 possibly adding more line numbers to the range.  At the time this
	 change was made I was unsure how to test this so chose to go with
	 maintaining the existing experience.  */
      if (lines[i].unrelocated_pc () == unrel_pc && lines[i].line != 0
	  && lines[i].is_stmt)
	range = btrace_line_range_add (range, lines[i].line);
    }

  return range;
}

/* Print source lines in LINES to UIOUT.

   UI_ITEM_CHAIN is a cleanup chain for the last source line and the
   instructions corresponding to that source line.  When printing a new source
   line, we do the cleanups for the open chain and open a new cleanup chain for
   the new source line.  If the source line range in LINES is not empty, this
   function will leave the cleanup chain for the last printed source line open
   so instructions can be added to it.  */

static void
btrace_print_lines (struct btrace_line_range lines, struct ui_out *uiout,
		    std::optional<ui_out_emit_tuple> *src_and_asm_tuple,
		    std::optional<ui_out_emit_list> *asm_list,
		    gdb_disassembly_flags flags)
{
  print_source_lines_flags psl_flags;

  if (flags & DISASSEMBLY_FILENAME)
    psl_flags |= PRINT_SOURCE_LINES_FILENAME;

  for (int line = lines.begin; line < lines.end; ++line)
    {
      asm_list->reset ();

      src_and_asm_tuple->emplace (uiout, "src_and_asm_line");

      print_source_lines (lines.symtab, line, line + 1, psl_flags);

      asm_list->emplace (uiout, "line_asm_insn");
    }
}

/* Disassemble a section of the recorded instruction trace.  */

static void
btrace_insn_history (struct ui_out *uiout,
		     const struct btrace_thread_info *btinfo,
		     const struct btrace_insn_iterator *begin,
		     const struct btrace_insn_iterator *end,
		     gdb_disassembly_flags flags)
{
  DEBUG ("itrace (0x%x): [%u; %u)", (unsigned) flags,
	 btrace_insn_number (begin), btrace_insn_number (end));

  flags |= DISASSEMBLY_SPECULATIVE;

  gdbarch *gdbarch = current_inferior ()->arch ();
  btrace_line_range last_lines = btrace_mk_line_range (NULL, 0, 0);

  ui_out_emit_list list_emitter (uiout, "asm_insns");

  std::optional<ui_out_emit_tuple> src_and_asm_tuple;
  std::optional<ui_out_emit_list> asm_list;

  gdb_pretty_print_disassembler disasm (gdbarch, uiout);

  for (btrace_insn_iterator it = *begin; btrace_insn_cmp (&it, end) != 0;
	 btrace_insn_next (&it, 1))
    {
      const struct btrace_insn *insn;

      insn = btrace_insn_get (&it);

      /* A NULL instruction indicates a gap in the trace.  */
      if (insn == NULL)
	{
	  const struct btrace_config *conf;

	  conf = btrace_conf (btinfo);

	  /* We have trace so we must have a configuration.  */
	  gdb_assert (conf != NULL);

	  uiout->field_fmt ("insn-number", "%u",
			    btrace_insn_number (&it));
	  uiout->text ("\t");

	  btrace_ui_out_decode_error (uiout, btrace_insn_get_error (&it),
				      conf->format);
	}
      else
	{
	  struct disasm_insn dinsn;

	  if ((flags & DISASSEMBLY_SOURCE) != 0)
	    {
	      struct btrace_line_range lines;

	      lines = btrace_find_line_range (insn->pc);
	      if (!btrace_line_range_is_empty (lines)
		  && !btrace_line_range_contains_range (last_lines, lines))
		{
		  btrace_print_lines (lines, uiout, &src_and_asm_tuple, &asm_list,
				      flags);
		  last_lines = lines;
		}
	      else if (!src_and_asm_tuple.has_value ())
		{
		  gdb_assert (!asm_list.has_value ());

		  src_and_asm_tuple.emplace (uiout, "src_and_asm_line");

		  /* No source information.  */
		  asm_list.emplace (uiout, "line_asm_insn");
		}

	      gdb_assert (src_and_asm_tuple.has_value ());
	      gdb_assert (asm_list.has_value ());
	    }

	  memset (&dinsn, 0, sizeof (dinsn));
	  dinsn.number = btrace_insn_number (&it);
	  dinsn.addr = insn->pc;

	  if ((insn->flags & BTRACE_INSN_FLAG_SPECULATIVE) != 0)
	    dinsn.is_speculative = 1;

	  disasm.pretty_print_insn (&dinsn, flags);
	}
    }
}

/* The insn_history method of target record-btrace.  */

void
record_btrace_target::insn_history (int size, gdb_disassembly_flags flags)
{
  struct btrace_thread_info *btinfo;
  struct btrace_insn_history *history;
  struct btrace_insn_iterator begin, end;
  struct ui_out *uiout;
  unsigned int context, covered;

  uiout = current_uiout;
  ui_out_emit_tuple tuple_emitter (uiout, "insn history");
  context = abs (size);
  if (context == 0)
    error (_("Bad record instruction-history-size."));

  btinfo = require_btrace ();
  history = btinfo->insn_history;
  if (history == NULL)
    {
      struct btrace_insn_iterator *replay;

      DEBUG ("insn-history (0x%x): %d", (unsigned) flags, size);

      /* If we're replaying, we start at the replay position.  Otherwise, we
	 start at the tail of the trace.  */
      replay = btinfo->replay;
      if (replay != NULL)
	begin = *replay;
      else
	btrace_insn_end (&begin, btinfo);

      /* We start from here and expand in the requested direction.  Then we
	 expand in the other direction, as well, to fill up any remaining
	 context.  */
      end = begin;
      if (size < 0)
	{
	  /* We want the current position covered, as well.  */
	  covered = btrace_insn_next (&end, 1);
	  covered += btrace_insn_prev (&begin, context - covered);
	  covered += btrace_insn_next (&end, context - covered);
	}
      else
	{
	  covered = btrace_insn_next (&end, context);
	  covered += btrace_insn_prev (&begin, context - covered);
	}
    }
  else
    {
      begin = history->begin;
      end = history->end;

      DEBUG ("insn-history (0x%x): %d, prev: [%u; %u)", (unsigned) flags, size,
	     btrace_insn_number (&begin), btrace_insn_number (&end));

      if (size < 0)
	{
	  end = begin;
	  covered = btrace_insn_prev (&begin, context);
	}
      else
	{
	  begin = end;
	  covered = btrace_insn_next (&end, context);
	}
    }

  if (covered > 0)
    btrace_insn_history (uiout, btinfo, &begin, &end, flags);
  else
    {
      if (size < 0)
	gdb_printf (_("At the start of the branch trace record.\n"));
      else
	gdb_printf (_("At the end of the branch trace record.\n"));
    }

  btrace_set_insn_history (btinfo, &begin, &end);
}

/* The insn_history_range method of target record-btrace.  */

void
record_btrace_target::insn_history_range (ULONGEST from, ULONGEST to,
					  gdb_disassembly_flags flags)
{
  struct btrace_thread_info *btinfo;
  struct btrace_insn_iterator begin, end;
  struct ui_out *uiout;
  unsigned int low, high;
  int found;

  uiout = current_uiout;
  ui_out_emit_tuple tuple_emitter (uiout, "insn history");
  low = from;
  high = to;

  DEBUG ("insn-history (0x%x): [%u; %u)", (unsigned) flags, low, high);

  /* Check for wrap-arounds.  */
  if (low != from || high != to)
    error (_("Bad range."));

  if (high < low)
    error (_("Bad range."));

  btinfo = require_btrace ();

  found = btrace_find_insn_by_number (&begin, btinfo, low);
  if (found == 0)
    error (_("Range out of bounds."));

  found = btrace_find_insn_by_number (&end, btinfo, high);
  if (found == 0)
    {
      /* Silently truncate the range.  */
      btrace_insn_end (&end, btinfo);
    }
  else
    {
      /* We want both begin and end to be inclusive.  */
      btrace_insn_next (&end, 1);
    }

  btrace_insn_history (uiout, btinfo, &begin, &end, flags);
  btrace_set_insn_history (btinfo, &begin, &end);
}

/* The insn_history_from method of target record-btrace.  */

void
record_btrace_target::insn_history_from (ULONGEST from, int size,
					 gdb_disassembly_flags flags)
{
  ULONGEST begin, end, context;

  context = abs (size);
  if (context == 0)
    error (_("Bad record instruction-history-size."));

  if (size < 0)
    {
      end = from;

      if (from < context)
	begin = 0;
      else
	begin = from - context + 1;
    }
  else
    {
      begin = from;
      end = from + context - 1;

      /* Check for wrap-around.  */
      if (end < begin)
	end = ULONGEST_MAX;
    }

  insn_history_range (begin, end, flags);
}

/* Print the instruction number range for a function call history line.  */

static void
btrace_call_history_insn_range (struct ui_out *uiout,
				const struct btrace_function *bfun)
{
  unsigned int begin, end, size;

  size = bfun->insn.size ();
  gdb_assert (size > 0);

  begin = bfun->insn_offset;
  end = begin + size - 1;

  uiout->field_unsigned ("insn begin", begin);
  uiout->text (",");
  uiout->field_unsigned ("insn end", end);
}

/* Compute the lowest and highest source line for the instructions in BFUN
   and return them in PBEGIN and PEND.
   Ignore instructions that can't be mapped to BFUN, e.g. instructions that
   result from inlining or macro expansion.  */

static void
btrace_compute_src_line_range (const struct btrace_function *bfun,
			       int *pbegin, int *pend)
{
  struct symtab *symtab;
  struct symbol *sym;
  int begin, end;

  begin = INT_MAX;
  end = INT_MIN;

  sym = bfun->sym;
  if (sym == NULL)
    goto out;

  symtab = sym->symtab ();

  for (const btrace_insn &insn : bfun->insn)
    {
      struct symtab_and_line sal;

      sal = find_pc_line (insn.pc, 0);
      if (sal.symtab != symtab || sal.line == 0)
	continue;

      begin = std::min (begin, sal.line);
      end = std::max (end, sal.line);
    }

 out:
  *pbegin = begin;
  *pend = end;
}

/* Print the source line information for a function call history line.  */

static void
btrace_call_history_src_line (struct ui_out *uiout,
			      const struct btrace_function *bfun)
{
  struct symbol *sym;
  int begin, end;

  sym = bfun->sym;
  if (sym == NULL)
    return;

  uiout->field_string ("file",
		       symtab_to_filename_for_display (sym->symtab ()),
		       file_name_style.style ());

  btrace_compute_src_line_range (bfun, &begin, &end);
  if (end < begin)
    return;

  uiout->text (":");
  uiout->field_signed ("min line", begin);

  if (end == begin)
    return;

  uiout->text (",");
  uiout->field_signed ("max line", end);
}

/* Get the name of a branch trace function.  */

static const char *
btrace_get_bfun_name (const struct btrace_function *bfun)
{
  struct minimal_symbol *msym;
  struct symbol *sym;

  if (bfun == NULL)
    return "??";

  msym = bfun->msym;
  sym = bfun->sym;

  if (sym != NULL)
    return sym->print_name ();
  else if (msym != NULL)
    return msym->print_name ();
  else
    return "??";
}

/* Disassemble a section of the recorded function trace.  */

static void
btrace_call_history (struct ui_out *uiout,
		     const struct btrace_thread_info *btinfo,
		     const struct btrace_call_iterator *begin,
		     const struct btrace_call_iterator *end,
		     int int_flags)
{
  struct btrace_call_iterator it;
  record_print_flags flags = (enum record_print_flag) int_flags;

  DEBUG ("ftrace (0x%x): [%u; %u)", int_flags, btrace_call_number (begin),
	 btrace_call_number (end));

  for (it = *begin; btrace_call_cmp (&it, end) < 0; btrace_call_next (&it, 1))
    {
      const struct btrace_function *bfun;
      struct minimal_symbol *msym;
      struct symbol *sym;

      bfun = btrace_call_get (&it);
      sym = bfun->sym;
      msym = bfun->msym;

      /* Print the function index.  */
      uiout->field_unsigned ("index", bfun->number);
      uiout->text ("\t");

      /* Indicate gaps in the trace.  */
      if (bfun->errcode != 0)
	{
	  const struct btrace_config *conf;

	  conf = btrace_conf (btinfo);

	  /* We have trace so we must have a configuration.  */
	  gdb_assert (conf != NULL);

	  btrace_ui_out_decode_error (uiout, bfun->errcode, conf->format);

	  continue;
	}

      if ((flags & RECORD_PRINT_INDENT_CALLS) != 0)
	{
	  int level = bfun->level + btinfo->level, i;

	  for (i = 0; i < level; ++i)
	    uiout->text ("  ");
	}

      if (sym != NULL)
	uiout->field_string ("function", sym->print_name (),
			     function_name_style.style ());
      else if (msym != NULL)
	uiout->field_string ("function", msym->print_name (),
			     function_name_style.style ());
      else if (!uiout->is_mi_like_p ())
	uiout->field_string ("function", "??",
			     function_name_style.style ());

      if ((flags & RECORD_PRINT_INSN_RANGE) != 0)
	{
	  uiout->text (_("\tinst "));
	  btrace_call_history_insn_range (uiout, bfun);
	}

      if ((flags & RECORD_PRINT_SRC_LINE) != 0)
	{
	  uiout->text (_("\tat "));
	  btrace_call_history_src_line (uiout, bfun);
	}

      uiout->text ("\n");
    }
}

/* The call_history method of target record-btrace.  */

void
record_btrace_target::call_history (int size, record_print_flags flags)
{
  struct btrace_thread_info *btinfo;
  struct btrace_call_history *history;
  struct btrace_call_iterator begin, end;
  struct ui_out *uiout;
  unsigned int context, covered;

  uiout = current_uiout;
  ui_out_emit_tuple tuple_emitter (uiout, "insn history");
  context = abs (size);
  if (context == 0)
    error (_("Bad record function-call-history-size."));

  btinfo = require_btrace ();
  history = btinfo->call_history;
  if (history == NULL)
    {
      struct btrace_insn_iterator *replay;

      DEBUG ("call-history (0x%x): %d", (int) flags, size);

      /* If we're replaying, we start at the replay position.  Otherwise, we
	 start at the tail of the trace.  */
      replay = btinfo->replay;
      if (replay != NULL)
	{
	  begin.btinfo = btinfo;
	  begin.index = replay->call_index;
	}
      else
	btrace_call_end (&begin, btinfo);

      /* We start from here and expand in the requested direction.  Then we
	 expand in the other direction, as well, to fill up any remaining
	 context.  */
      end = begin;
      if (size < 0)
	{
	  /* We want the current position covered, as well.  */
	  covered = btrace_call_next (&end, 1);
	  covered += btrace_call_prev (&begin, context - covered);
	  covered += btrace_call_next (&end, context - covered);
	}
      else
	{
	  covered = btrace_call_next (&end, context);
	  covered += btrace_call_prev (&begin, context- covered);
	}
    }
  else
    {
      begin = history->begin;
      end = history->end;

      DEBUG ("call-history (0x%x): %d, prev: [%u; %u)", (int) flags, size,
	     btrace_call_number (&begin), btrace_call_number (&end));

      if (size < 0)
	{
	  end = begin;
	  covered = btrace_call_prev (&begin, context);
	}
      else
	{
	  begin = end;
	  covered = btrace_call_next (&end, context);
	}
    }

  if (covered > 0)
    btrace_call_history (uiout, btinfo, &begin, &end, flags);
  else
    {
      if (size < 0)
	gdb_printf (_("At the start of the branch trace record.\n"));
      else
	gdb_printf (_("At the end of the branch trace record.\n"));
    }

  btrace_set_call_history (btinfo, &begin, &end);
}

/* The call_history_range method of target record-btrace.  */

void
record_btrace_target::call_history_range (ULONGEST from, ULONGEST to,
					  record_print_flags flags)
{
  struct btrace_thread_info *btinfo;
  struct btrace_call_iterator begin, end;
  struct ui_out *uiout;
  unsigned int low, high;
  int found;

  uiout = current_uiout;
  ui_out_emit_tuple tuple_emitter (uiout, "func history");
  low = from;
  high = to;

  DEBUG ("call-history (0x%x): [%u; %u)", (int) flags, low, high);

  /* Check for wrap-arounds.  */
  if (low != from || high != to)
    error (_("Bad range."));

  if (high < low)
    error (_("Bad range."));

  btinfo = require_btrace ();

  found = btrace_find_call_by_number (&begin, btinfo, low);
  if (found == 0)
    error (_("Range out of bounds."));

  found = btrace_find_call_by_number (&end, btinfo, high);
  if (found == 0)
    {
      /* Silently truncate the range.  */
      btrace_call_end (&end, btinfo);
    }
  else
    {
      /* We want both begin and end to be inclusive.  */
      btrace_call_next (&end, 1);
    }

  btrace_call_history (uiout, btinfo, &begin, &end, flags);
  btrace_set_call_history (btinfo, &begin, &end);
}

/* The call_history_from method of target record-btrace.  */

void
record_btrace_target::call_history_from (ULONGEST from, int size,
					 record_print_flags flags)
{
  ULONGEST begin, end, context;

  context = abs (size);
  if (context == 0)
    error (_("Bad record function-call-history-size."));

  if (size < 0)
    {
      end = from;

      if (from < context)
	begin = 0;
      else
	begin = from - context + 1;
    }
  else
    {
      begin = from;
      end = from + context - 1;

      /* Check for wrap-around.  */
      if (end < begin)
	end = ULONGEST_MAX;
    }

  call_history_range ( begin, end, flags);
}

/* The record_method method of target record-btrace.  */

enum record_method
record_btrace_target::record_method (ptid_t ptid)
{
  process_stratum_target *proc_target = current_inferior ()->process_target ();
  thread_info *const tp = proc_target->find_thread (ptid);

  if (tp == NULL)
    error (_("No thread."));

  if (tp->btrace.target == NULL)
    return RECORD_METHOD_NONE;

  return RECORD_METHOD_BTRACE;
}

/* The record_is_replaying method of target record-btrace.  */

bool
record_btrace_target::record_is_replaying (ptid_t ptid)
{
  process_stratum_target *proc_target = current_inferior ()->process_target ();
  for (thread_info *tp : all_non_exited_threads (proc_target, ptid))
    if (btrace_is_replaying (tp))
      return true;

  return false;
}

/* The record_will_replay method of target record-btrace.  */

bool
record_btrace_target::record_will_replay (ptid_t ptid, int dir)
{
  return dir == EXEC_REVERSE || record_is_replaying (ptid);
}

/* The xfer_partial method of target record-btrace.  */

enum target_xfer_status
record_btrace_target::xfer_partial (enum target_object object,
				    const char *annex, gdb_byte *readbuf,
				    const gdb_byte *writebuf, ULONGEST offset,
				    ULONGEST len, ULONGEST *xfered_len)
{
  /* Filter out requests that don't make sense during replay.  */
  if (replay_memory_access == replay_memory_access_read_only
      && !record_btrace_generating_corefile
      && record_is_replaying (inferior_ptid))
    {
      switch (object)
	{
	case TARGET_OBJECT_MEMORY:
	  {
	    const struct target_section *section;

	    /* We do not allow writing memory in general.  */
	    if (writebuf != NULL)
	      {
		*xfered_len = len;
		return TARGET_XFER_UNAVAILABLE;
	      }

	    /* We allow reading readonly memory.  */
	    section = target_section_by_addr (this, offset);
	    if (section != NULL)
	      {
		/* Check if the section we found is readonly.  */
		if ((bfd_section_flags (section->the_bfd_section)
		     & SEC_READONLY) != 0)
		  {
		    /* Truncate the request to fit into this section.  */
		    len = std::min (len, section->endaddr - offset);
		    break;
		  }
	      }

	    *xfered_len = len;
	    return TARGET_XFER_UNAVAILABLE;
	  }
	}
    }

  /* Forward the request.  */
  return this->beneath ()->xfer_partial (object, annex, readbuf, writebuf,
					 offset, len, xfered_len);
}

/* The insert_breakpoint method of target record-btrace.  */

int
record_btrace_target::insert_breakpoint (struct gdbarch *gdbarch,
					 struct bp_target_info *bp_tgt)
{
  const char *old;
  int ret;

  /* Inserting breakpoints requires accessing memory.  Allow it for the
     duration of this function.  */
  old = replay_memory_access;
  replay_memory_access = replay_memory_access_read_write;

  ret = 0;
  try
    {
      ret = this->beneath ()->insert_breakpoint (gdbarch, bp_tgt);
    }
  catch (const gdb_exception &except)
    {
      replay_memory_access = old;
      throw;
    }
  replay_memory_access = old;

  return ret;
}

/* The remove_breakpoint method of target record-btrace.  */

int
record_btrace_target::remove_breakpoint (struct gdbarch *gdbarch,
					 struct bp_target_info *bp_tgt,
					 enum remove_bp_reason reason)
{
  const char *old;
  int ret;

  /* Removing breakpoints requires accessing memory.  Allow it for the
     duration of this function.  */
  old = replay_memory_access;
  replay_memory_access = replay_memory_access_read_write;

  ret = 0;
  try
    {
      ret = this->beneath ()->remove_breakpoint (gdbarch, bp_tgt, reason);
    }
  catch (const gdb_exception &except)
    {
      replay_memory_access = old;
      throw;
    }
  replay_memory_access = old;

  return ret;
}

/* The fetch_registers method of target record-btrace.  */

void
record_btrace_target::fetch_registers (struct regcache *regcache, int regno)
{
  btrace_insn_iterator *replay = nullptr;

  /* Thread-db may ask for a thread's registers before GDB knows about the
     thread.  We forward the request to the target beneath in this
     case.  */
  thread_info *tp
    = current_inferior ()->process_target ()->find_thread (regcache->ptid ());
  if (tp != nullptr)
    replay =  tp->btrace.replay;

  if (replay != nullptr && !record_btrace_generating_corefile)
    {
      const struct btrace_insn *insn;
      struct gdbarch *gdbarch;
      int pcreg;

      gdbarch = regcache->arch ();
      pcreg = gdbarch_pc_regnum (gdbarch);
      if (pcreg < 0)
	return;

      /* We can only provide the PC register.  */
      if (regno >= 0 && regno != pcreg)
	return;

      insn = btrace_insn_get (replay);
      gdb_assert (insn != NULL);

      regcache->raw_supply (regno, &insn->pc);
    }
  else
    this->beneath ()->fetch_registers (regcache, regno);
}

/* The store_registers method of target record-btrace.  */

void
record_btrace_target::store_registers (struct regcache *regcache, int regno)
{
  if (!record_btrace_generating_corefile
      && record_is_replaying (regcache->ptid ()))
    error (_("Cannot write registers while replaying."));

  gdb_assert (may_write_registers);

  this->beneath ()->store_registers (regcache, regno);
}

/* The prepare_to_store method of target record-btrace.  */

void
record_btrace_target::prepare_to_store (struct regcache *regcache)
{
  if (!record_btrace_generating_corefile
      && record_is_replaying (regcache->ptid ()))
    return;

  this->beneath ()->prepare_to_store (regcache);
}

/* The branch trace frame cache.  */

struct btrace_frame_cache
{
  /* The thread.  */
  struct thread_info *tp;

  /* The frame info.  */
  frame_info *frame;

  /* The branch trace function segment.  */
  const struct btrace_function *bfun;
};

/* A struct btrace_frame_cache hash table indexed by NEXT.  */

static htab_t bfcache;

/* hash_f for htab_create_alloc of bfcache.  */

static hashval_t
bfcache_hash (const void *arg)
{
  const struct btrace_frame_cache *cache
    = (const struct btrace_frame_cache *) arg;

  return htab_hash_pointer (cache->frame);
}

/* eq_f for htab_create_alloc of bfcache.  */

static int
bfcache_eq (const void *arg1, const void *arg2)
{
  const struct btrace_frame_cache *cache1
    = (const struct btrace_frame_cache *) arg1;
  const struct btrace_frame_cache *cache2
    = (const struct btrace_frame_cache *) arg2;

  return cache1->frame == cache2->frame;
}

/* Create a new btrace frame cache.  */

static struct btrace_frame_cache *
bfcache_new (frame_info_ptr frame)
{
  struct btrace_frame_cache *cache;
  void **slot;

  cache = FRAME_OBSTACK_ZALLOC (struct btrace_frame_cache);
  cache->frame = frame.get ();

  slot = htab_find_slot (bfcache, cache, INSERT);
  gdb_assert (*slot == NULL);
  *slot = cache;

  return cache;
}

/* Extract the branch trace function from a branch trace frame.  */

static const struct btrace_function *
btrace_get_frame_function (frame_info_ptr frame)
{
  const struct btrace_frame_cache *cache;
  struct btrace_frame_cache pattern;
  void **slot;

  pattern.frame = frame.get ();

  slot = htab_find_slot (bfcache, &pattern, NO_INSERT);
  if (slot == NULL)
    return NULL;

  cache = (const struct btrace_frame_cache *) *slot;
  return cache->bfun;
}

/* Implement stop_reason method for record_btrace_frame_unwind.  */

static enum unwind_stop_reason
record_btrace_frame_unwind_stop_reason (frame_info_ptr this_frame,
					void **this_cache)
{
  const struct btrace_frame_cache *cache;
  const struct btrace_function *bfun;

  cache = (const struct btrace_frame_cache *) *this_cache;
  bfun = cache->bfun;
  gdb_assert (bfun != NULL);

  if (bfun->up == 0)
    return UNWIND_UNAVAILABLE;

  return UNWIND_NO_REASON;
}

/* Implement this_id method for record_btrace_frame_unwind.  */

static void
record_btrace_frame_this_id (frame_info_ptr this_frame, void **this_cache,
			     struct frame_id *this_id)
{
  const struct btrace_frame_cache *cache;
  const struct btrace_function *bfun;
  struct btrace_call_iterator it;
  CORE_ADDR code, special;

  cache = (const struct btrace_frame_cache *) *this_cache;

  bfun = cache->bfun;
  gdb_assert (bfun != NULL);

  while (btrace_find_call_by_number (&it, &cache->tp->btrace, bfun->prev) != 0)
    bfun = btrace_call_get (&it);

  code = get_frame_func (this_frame);
  special = bfun->number;

  *this_id = frame_id_build_unavailable_stack_special (code, special);

  DEBUG ("[frame] %s id: (!stack, pc=%s, special=%s)",
	 btrace_get_bfun_name (cache->bfun),
	 core_addr_to_string_nz (this_id->code_addr),
	 core_addr_to_string_nz (this_id->special_addr));
}

/* Implement prev_register method for record_btrace_frame_unwind.  */

static struct value *
record_btrace_frame_prev_register (frame_info_ptr this_frame,
				   void **this_cache,
				   int regnum)
{
  const struct btrace_frame_cache *cache;
  const struct btrace_function *bfun, *caller;
  struct btrace_call_iterator it;
  struct gdbarch *gdbarch;
  CORE_ADDR pc;
  int pcreg;

  gdbarch = get_frame_arch (this_frame);
  pcreg = gdbarch_pc_regnum (gdbarch);
  if (pcreg < 0 || regnum != pcreg)
    throw_error (NOT_AVAILABLE_ERROR,
		 _("Registers are not available in btrace record history"));

  cache = (const struct btrace_frame_cache *) *this_cache;
  bfun = cache->bfun;
  gdb_assert (bfun != NULL);

  if (btrace_find_call_by_number (&it, &cache->tp->btrace, bfun->up) == 0)
    throw_error (NOT_AVAILABLE_ERROR,
		 _("No caller in btrace record history"));

  caller = btrace_call_get (&it);

  if ((bfun->flags & BFUN_UP_LINKS_TO_RET) != 0)
    pc = caller->insn.front ().pc;
  else
    {
      pc = caller->insn.back ().pc;
      pc += gdb_insn_length (gdbarch, pc);
    }

  DEBUG ("[frame] unwound PC in %s on level %d: %s",
	 btrace_get_bfun_name (bfun), bfun->level,
	 core_addr_to_string_nz (pc));

  return frame_unwind_got_address (this_frame, regnum, pc);
}

/* Implement sniffer method for record_btrace_frame_unwind.  */

static int
record_btrace_frame_sniffer (const struct frame_unwind *self,
			     frame_info_ptr this_frame,
			     void **this_cache)
{
  const struct btrace_function *bfun;
  struct btrace_frame_cache *cache;
  struct thread_info *tp;
  frame_info_ptr next;

  /* THIS_FRAME does not contain a reference to its thread.  */
  tp = inferior_thread ();

  bfun = NULL;
  next = get_next_frame (this_frame);
  if (next == NULL)
    {
      const struct btrace_insn_iterator *replay;

      replay = tp->btrace.replay;
      if (replay != NULL)
	bfun = &replay->btinfo->functions[replay->call_index];
    }
  else
    {
      const struct btrace_function *callee;
      struct btrace_call_iterator it;

      callee = btrace_get_frame_function (next);
      if (callee == NULL || (callee->flags & BFUN_UP_LINKS_TO_TAILCALL) != 0)
	return 0;

      if (btrace_find_call_by_number (&it, &tp->btrace, callee->up) == 0)
	return 0;

      bfun = btrace_call_get (&it);
    }

  if (bfun == NULL)
    return 0;

  DEBUG ("[frame] sniffed frame for %s on level %d",
	 btrace_get_bfun_name (bfun), bfun->level);

  /* This is our frame.  Initialize the frame cache.  */
  cache = bfcache_new (this_frame);
  cache->tp = tp;
  cache->bfun = bfun;

  *this_cache = cache;
  return 1;
}

/* Implement sniffer method for record_btrace_tailcall_frame_unwind.  */

static int
record_btrace_tailcall_frame_sniffer (const struct frame_unwind *self,
				      frame_info_ptr this_frame,
				      void **this_cache)
{
  const struct btrace_function *bfun, *callee;
  struct btrace_frame_cache *cache;
  struct btrace_call_iterator it;
  frame_info_ptr next;
  struct thread_info *tinfo;

  next = get_next_frame (this_frame);
  if (next == NULL)
    return 0;

  callee = btrace_get_frame_function (next);
  if (callee == NULL)
    return 0;

  if ((callee->flags & BFUN_UP_LINKS_TO_TAILCALL) == 0)
    return 0;

  tinfo = inferior_thread ();
  if (btrace_find_call_by_number (&it, &tinfo->btrace, callee->up) == 0)
    return 0;

  bfun = btrace_call_get (&it);

  DEBUG ("[frame] sniffed tailcall frame for %s on level %d",
	 btrace_get_bfun_name (bfun), bfun->level);

  /* This is our frame.  Initialize the frame cache.  */
  cache = bfcache_new (this_frame);
  cache->tp = tinfo;
  cache->bfun = bfun;

  *this_cache = cache;
  return 1;
}

static void
record_btrace_frame_dealloc_cache (frame_info *self, void *this_cache)
{
  struct btrace_frame_cache *cache;
  void **slot;

  cache = (struct btrace_frame_cache *) this_cache;

  slot = htab_find_slot (bfcache, cache, NO_INSERT);
  gdb_assert (slot != NULL);

  htab_remove_elt (bfcache, cache);
}

/* btrace recording does not store previous memory content, neither the stack
   frames content.  Any unwinding would return erroneous results as the stack
   contents no longer matches the changed PC value restored from history.
   Therefore this unwinder reports any possibly unwound registers as
   <unavailable>.  */

const struct frame_unwind record_btrace_frame_unwind =
{
  "record-btrace",
  NORMAL_FRAME,
  record_btrace_frame_unwind_stop_reason,
  record_btrace_frame_this_id,
  record_btrace_frame_prev_register,
  NULL,
  record_btrace_frame_sniffer,
  record_btrace_frame_dealloc_cache
};

const struct frame_unwind record_btrace_tailcall_frame_unwind =
{
  "record-btrace tailcall",
  TAILCALL_FRAME,
  record_btrace_frame_unwind_stop_reason,
  record_btrace_frame_this_id,
  record_btrace_frame_prev_register,
  NULL,
  record_btrace_tailcall_frame_sniffer,
  record_btrace_frame_dealloc_cache
};

/* Implement the get_unwinder method.  */

const struct frame_unwind *
record_btrace_target::get_unwinder ()
{
  return &record_btrace_frame_unwind;
}

/* Implement the get_tailcall_unwinder method.  */

const struct frame_unwind *
record_btrace_target::get_tailcall_unwinder ()
{
  return &record_btrace_tailcall_frame_unwind;
}

/* Return a human-readable string for FLAG.  */

static const char *
btrace_thread_flag_to_str (btrace_thread_flags flag)
{
  switch (flag)
    {
    case BTHR_STEP:
      return "step";

    case BTHR_RSTEP:
      return "reverse-step";

    case BTHR_CONT:
      return "cont";

    case BTHR_RCONT:
      return "reverse-cont";

    case BTHR_STOP:
      return "stop";
    }

  return "<invalid>";
}

/* Indicate that TP should be resumed according to FLAG.  */

static void
record_btrace_resume_thread (struct thread_info *tp,
			     enum btrace_thread_flag flag)
{
  struct btrace_thread_info *btinfo;

  DEBUG ("resuming thread %s (%s): %x (%s)", print_thread_id (tp),
	 tp->ptid.to_string ().c_str (), flag,
	 btrace_thread_flag_to_str (flag));

  btinfo = &tp->btrace;

  /* Fetch the latest branch trace.  */
  btrace_fetch (tp, record_btrace_get_cpu ());

  /* A resume request overwrites a preceding resume or stop request.  */
  btinfo->flags &= ~(BTHR_MOVE | BTHR_STOP);
  btinfo->flags |= flag;
}

/* Get the current frame for TP.  */

static struct frame_id
get_thread_current_frame_id (struct thread_info *tp)
{
  /* Set current thread, which is implicitly used by
     get_current_frame.  */
  scoped_restore_current_thread restore_thread;

  switch_to_thread (tp);

  process_stratum_target *proc_target = tp->inf->process_target ();

  /* Clear the executing flag to allow changes to the current frame.
     We are not actually running, yet.  We just started a reverse execution
     command or a record goto command.
     For the latter, EXECUTING is false and this has no effect.
     For the former, EXECUTING is true and we're in wait, about to
     move the thread.  Since we need to recompute the stack, we temporarily
     set EXECUTING to false.  */
  bool executing = tp->executing ();
  set_executing (proc_target, inferior_ptid, false);
  SCOPE_EXIT
    {
      set_executing (proc_target, inferior_ptid, executing);
    };
  return get_frame_id (get_current_frame ());
}

/* Start replaying a thread.  */

static struct btrace_insn_iterator *
record_btrace_start_replaying (struct thread_info *tp)
{
  struct btrace_insn_iterator *replay;
  struct btrace_thread_info *btinfo;

  btinfo = &tp->btrace;
  replay = NULL;

  /* We can't start replaying without trace.  */
  if (btinfo->functions.empty ())
    error (_("No trace."));

  /* GDB stores the current frame_id when stepping in order to detects steps
     into subroutines.
     Since frames are computed differently when we're replaying, we need to
     recompute those stored frames and fix them up so we can still detect
     subroutines after we started replaying.  */
  try
    {
      struct frame_id frame_id;
      int upd_step_frame_id, upd_step_stack_frame_id;

      /* The current frame without replaying - computed via normal unwind.  */
      frame_id = get_thread_current_frame_id (tp);

      /* Check if we need to update any stepping-related frame id's.  */
      upd_step_frame_id = (frame_id == tp->control.step_frame_id);
      upd_step_stack_frame_id = (frame_id == tp->control.step_stack_frame_id);

      /* We start replaying at the end of the branch trace.  This corresponds
	 to the current instruction.  */
      replay = XNEW (struct btrace_insn_iterator);
      btrace_insn_end (replay, btinfo);

      /* Skip gaps at the end of the trace.  */
      while (btrace_insn_get (replay) == NULL)
	{
	  unsigned int steps;

	  steps = btrace_insn_prev (replay, 1);
	  if (steps == 0)
	    error (_("No trace."));
	}

      /* We're not replaying, yet.  */
      gdb_assert (btinfo->replay == NULL);
      btinfo->replay = replay;

      /* Make sure we're not using any stale registers.  */
      registers_changed_thread (tp);

      /* The current frame with replaying - computed via btrace unwind.  */
      frame_id = get_thread_current_frame_id (tp);

      /* Replace stepping related frames where necessary.  */
      if (upd_step_frame_id)
	tp->control.step_frame_id = frame_id;
      if (upd_step_stack_frame_id)
	tp->control.step_stack_frame_id = frame_id;
    }
  catch (const gdb_exception &except)
    {
      xfree (btinfo->replay);
      btinfo->replay = NULL;

      registers_changed_thread (tp);

      throw;
    }

  return replay;
}

/* Stop replaying a thread.  */

static void
record_btrace_stop_replaying (struct thread_info *tp)
{
  struct btrace_thread_info *btinfo;

  btinfo = &tp->btrace;

  xfree (btinfo->replay);
  btinfo->replay = NULL;

  /* Make sure we're not leaving any stale registers.  */
  registers_changed_thread (tp);
}

/* Stop replaying TP if it is at the end of its execution history.  */

static void
record_btrace_stop_replaying_at_end (struct thread_info *tp)
{
  struct btrace_insn_iterator *replay, end;
  struct btrace_thread_info *btinfo;

  btinfo = &tp->btrace;
  replay = btinfo->replay;

  if (replay == NULL)
    return;

  btrace_insn_end (&end, btinfo);

  if (btrace_insn_cmp (replay, &end) == 0)
    record_btrace_stop_replaying (tp);
}

/* The resume method of target record-btrace.  */

void
record_btrace_target::resume (ptid_t ptid, int step, enum gdb_signal signal)
{
  enum btrace_thread_flag flag, cflag;

  DEBUG ("resume %s: %s%s", ptid.to_string ().c_str (),
	 ::execution_direction == EXEC_REVERSE ? "reverse-" : "",
	 step ? "step" : "cont");

  /* Store the execution direction of the last resume.

     If there is more than one resume call, we have to rely on infrun
     to not change the execution direction in-between.  */
  record_btrace_resume_exec_dir = ::execution_direction;

  /* As long as we're not replaying, just forward the request.

     For non-stop targets this means that no thread is replaying.  In order to
     make progress, we may need to explicitly move replaying threads to the end
     of their execution history.  */
  if ((::execution_direction != EXEC_REVERSE)
      && !record_is_replaying (minus_one_ptid))
    {
      this->beneath ()->resume (ptid, step, signal);
      return;
    }

  /* Compute the btrace thread flag for the requested move.  */
  if (::execution_direction == EXEC_REVERSE)
    {
      flag = step == 0 ? BTHR_RCONT : BTHR_RSTEP;
      cflag = BTHR_RCONT;
    }
  else
    {
      flag = step == 0 ? BTHR_CONT : BTHR_STEP;
      cflag = BTHR_CONT;
    }

  /* We just indicate the resume intent here.  The actual stepping happens in
     record_btrace_wait below.

     For all-stop targets, we only step INFERIOR_PTID and continue others.  */

  process_stratum_target *proc_target = current_inferior ()->process_target ();

  if (!target_is_non_stop_p ())
    {
      gdb_assert (inferior_ptid.matches (ptid));

      for (thread_info *tp : all_non_exited_threads (proc_target, ptid))
	{
	  if (tp->ptid.matches (inferior_ptid))
	    record_btrace_resume_thread (tp, flag);
	  else
	    record_btrace_resume_thread (tp, cflag);
	}
    }
  else
    {
      for (thread_info *tp : all_non_exited_threads (proc_target, ptid))
	record_btrace_resume_thread (tp, flag);
    }

  /* Async support.  */
  if (target_can_async_p ())
    {
      target_async (true);
      mark_async_event_handler (record_btrace_async_inferior_event_handler);
    }
}

/* Cancel resuming TP.  */

static void
record_btrace_cancel_resume (struct thread_info *tp)
{
  btrace_thread_flags flags;

  flags = tp->btrace.flags & (BTHR_MOVE | BTHR_STOP);
  if (flags == 0)
    return;

  DEBUG ("cancel resume thread %s (%s): %x (%s)",
	 print_thread_id (tp),
	 tp->ptid.to_string ().c_str (), flags.raw (),
	 btrace_thread_flag_to_str (flags));

  tp->btrace.flags &= ~(BTHR_MOVE | BTHR_STOP);
  record_btrace_stop_replaying_at_end (tp);
}

/* Return a target_waitstatus indicating that we ran out of history.  */

static struct target_waitstatus
btrace_step_no_history (void)
{
  struct target_waitstatus status;

  status.set_no_history ();

  return status;
}

/* Return a target_waitstatus indicating that a step finished.  */

static struct target_waitstatus
btrace_step_stopped (void)
{
  struct target_waitstatus status;

  status.set_stopped (GDB_SIGNAL_TRAP);

  return status;
}

/* Return a target_waitstatus indicating that a thread was stopped as
   requested.  */

static struct target_waitstatus
btrace_step_stopped_on_request (void)
{
  struct target_waitstatus status;

  status.set_stopped (GDB_SIGNAL_0);

  return status;
}

/* Return a target_waitstatus indicating a spurious stop.  */

static struct target_waitstatus
btrace_step_spurious (void)
{
  struct target_waitstatus status;

  status.set_spurious ();

  return status;
}

/* Return a target_waitstatus indicating that the thread was not resumed.  */

static struct target_waitstatus
btrace_step_no_resumed (void)
{
  struct target_waitstatus status;

  status.set_no_resumed ();

  return status;
}

/* Return a target_waitstatus indicating that we should wait again.  */

static struct target_waitstatus
btrace_step_again (void)
{
  struct target_waitstatus status;

  status.set_ignore ();

  return status;
}

/* Clear the record histories.  */

static void
record_btrace_clear_histories (struct btrace_thread_info *btinfo)
{
  xfree (btinfo->insn_history);
  xfree (btinfo->call_history);

  btinfo->insn_history = NULL;
  btinfo->call_history = NULL;
}

/* Check whether TP's current replay position is at a breakpoint.  */

static int
record_btrace_replay_at_breakpoint (struct thread_info *tp)
{
  struct btrace_insn_iterator *replay;
  struct btrace_thread_info *btinfo;
  const struct btrace_insn *insn;

  btinfo = &tp->btrace;
  replay = btinfo->replay;

  if (replay == NULL)
    return 0;

  insn = btrace_insn_get (replay);
  if (insn == NULL)
    return 0;

  return record_check_stopped_by_breakpoint (tp->inf->aspace.get (), insn->pc,
					     &btinfo->stop_reason);
}

/* Step one instruction in forward direction.  */

static struct target_waitstatus
record_btrace_single_step_forward (struct thread_info *tp)
{
  struct btrace_insn_iterator *replay, end, start;
  struct btrace_thread_info *btinfo;

  btinfo = &tp->btrace;
  replay = btinfo->replay;

  /* We're done if we're not replaying.  */
  if (replay == NULL)
    return btrace_step_no_history ();

  /* Check if we're stepping a breakpoint.  */
  if (record_btrace_replay_at_breakpoint (tp))
    return btrace_step_stopped ();

  /* Skip gaps during replay.  If we end up at a gap (at the end of the trace),
     jump back to the instruction at which we started.  */
  start = *replay;
  do
    {
      unsigned int steps;

      /* We will bail out here if we continue stepping after reaching the end
	 of the execution history.  */
      steps = btrace_insn_next (replay, 1);
      if (steps == 0)
	{
	  *replay = start;
	  return btrace_step_no_history ();
	}
    }
  while (btrace_insn_get (replay) == NULL);

  /* Determine the end of the instruction trace.  */
  btrace_insn_end (&end, btinfo);

  /* The execution trace contains (and ends with) the current instruction.
     This instruction has not been executed, yet, so the trace really ends
     one instruction earlier.  */
  if (btrace_insn_cmp (replay, &end) == 0)
    return btrace_step_no_history ();

  return btrace_step_spurious ();
}

/* Step one instruction in backward direction.  */

static struct target_waitstatus
record_btrace_single_step_backward (struct thread_info *tp)
{
  struct btrace_insn_iterator *replay, start;
  struct btrace_thread_info *btinfo;

  btinfo = &tp->btrace;
  replay = btinfo->replay;

  /* Start replaying if we're not already doing so.  */
  if (replay == NULL)
    replay = record_btrace_start_replaying (tp);

  /* If we can't step any further, we reached the end of the history.
     Skip gaps during replay.  If we end up at a gap (at the beginning of
     the trace), jump back to the instruction at which we started.  */
  start = *replay;
  do
    {
      unsigned int steps;

      steps = btrace_insn_prev (replay, 1);
      if (steps == 0)
	{
	  *replay = start;
	  return btrace_step_no_history ();
	}
    }
  while (btrace_insn_get (replay) == NULL);

  /* Check if we're stepping a breakpoint.

     For reverse-stepping, this check is after the step.  There is logic in
     infrun.c that handles reverse-stepping separately.  See, for example,
     proceed and adjust_pc_after_break.

     This code assumes that for reverse-stepping, PC points to the last
     de-executed instruction, whereas for forward-stepping PC points to the
     next to-be-executed instruction.  */
  if (record_btrace_replay_at_breakpoint (tp))
    return btrace_step_stopped ();

  return btrace_step_spurious ();
}

/* Step a single thread.  */

static struct target_waitstatus
record_btrace_step_thread (struct thread_info *tp)
{
  struct btrace_thread_info *btinfo;
  struct target_waitstatus status;
  btrace_thread_flags flags;

  btinfo = &tp->btrace;

  flags = btinfo->flags & (BTHR_MOVE | BTHR_STOP);
  btinfo->flags &= ~(BTHR_MOVE | BTHR_STOP);

  DEBUG ("stepping thread %s (%s): %x (%s)", print_thread_id (tp),
	 tp->ptid.to_string ().c_str (), flags.raw (),
	 btrace_thread_flag_to_str (flags));

  /* We can't step without an execution history.  */
  if ((flags & BTHR_MOVE) != 0 && btrace_is_empty (tp))
    return btrace_step_no_history ();

  switch (flags)
    {
    default:
      internal_error (_("invalid stepping type."));

    case BTHR_STOP:
      return btrace_step_stopped_on_request ();

    case BTHR_STEP:
      status = record_btrace_single_step_forward (tp);
      if (status.kind () != TARGET_WAITKIND_SPURIOUS)
	break;

      return btrace_step_stopped ();

    case BTHR_RSTEP:
      status = record_btrace_single_step_backward (tp);
      if (status.kind () != TARGET_WAITKIND_SPURIOUS)
	break;

      return btrace_step_stopped ();

    case BTHR_CONT:
      status = record_btrace_single_step_forward (tp);
      if (status.kind () != TARGET_WAITKIND_SPURIOUS)
	break;

      btinfo->flags |= flags;
      return btrace_step_again ();

    case BTHR_RCONT:
      status = record_btrace_single_step_backward (tp);
      if (status.kind () != TARGET_WAITKIND_SPURIOUS)
	break;

      btinfo->flags |= flags;
      return btrace_step_again ();
    }

  /* We keep threads moving at the end of their execution history.  The wait
     method will stop the thread for whom the event is reported.  */
  if (status.kind () == TARGET_WAITKIND_NO_HISTORY)
    btinfo->flags |= flags;

  return status;
}

/* Announce further events if necessary.  */

static void
record_btrace_maybe_mark_async_event
  (const std::vector<thread_info *> &moving,
   const std::vector<thread_info *> &no_history)
{
  bool more_moving = !moving.empty ();
  bool more_no_history = !no_history.empty ();;

  if (!more_moving && !more_no_history)
    return;

  if (more_moving)
    DEBUG ("movers pending");

  if (more_no_history)
    DEBUG ("no-history pending");

  mark_async_event_handler (record_btrace_async_inferior_event_handler);
}

/* The wait method of target record-btrace.  */

ptid_t
record_btrace_target::wait (ptid_t ptid, struct target_waitstatus *status,
			    target_wait_flags options)
{
  std::vector<thread_info *> moving;
  std::vector<thread_info *> no_history;

  /* Clear this, if needed we'll re-mark it below.  */
  clear_async_event_handler (record_btrace_async_inferior_event_handler);

  DEBUG ("wait %s (0x%x)", ptid.to_string ().c_str (),
	 (unsigned) options);

  /* As long as we're not replaying, just forward the request.  */
  if ((::execution_direction != EXEC_REVERSE)
      && !record_is_replaying (minus_one_ptid))
    {
      return this->beneath ()->wait (ptid, status, options);
    }

  /* Keep a work list of moving threads.  */
  process_stratum_target *proc_target = current_inferior ()->process_target ();
  for (thread_info *tp : all_non_exited_threads (proc_target, ptid))
    if ((tp->btrace.flags & (BTHR_MOVE | BTHR_STOP)) != 0)
      moving.push_back (tp);

  if (moving.empty ())
    {
      *status = btrace_step_no_resumed ();

      DEBUG ("wait ended by %s: %s", null_ptid.to_string ().c_str (),
	     status->to_string ().c_str ());

      return null_ptid;
    }

  /* Step moving threads one by one, one step each, until either one thread
     reports an event or we run out of threads to step.

     When stepping more than one thread, chances are that some threads reach
     the end of their execution history earlier than others.  If we reported
     this immediately, all-stop on top of non-stop would stop all threads and
     resume the same threads next time.  And we would report the same thread
     having reached the end of its execution history again.

     In the worst case, this would starve the other threads.  But even if other
     threads would be allowed to make progress, this would result in far too
     many intermediate stops.

     We therefore delay the reporting of "no execution history" until we have
     nothing else to report.  By this time, all threads should have moved to
     either the beginning or the end of their execution history.  There will
     be a single user-visible stop.  */
  struct thread_info *eventing = NULL;
  while ((eventing == NULL) && !moving.empty ())
    {
      for (unsigned int ix = 0; eventing == NULL && ix < moving.size ();)
	{
	  thread_info *tp = moving[ix];

	  *status = record_btrace_step_thread (tp);

	  switch (status->kind ())
	    {
	    case TARGET_WAITKIND_IGNORE:
	      ix++;
	      break;

	    case TARGET_WAITKIND_NO_HISTORY:
	      no_history.push_back (ordered_remove (moving, ix));
	      break;

	    default:
	      eventing = unordered_remove (moving, ix);
	      break;
	    }
	}
    }

  if (eventing == NULL)
    {
      /* We started with at least one moving thread.  This thread must have
	 either stopped or reached the end of its execution history.

	 In the former case, EVENTING must not be NULL.
	 In the latter case, NO_HISTORY must not be empty.  */
      gdb_assert (!no_history.empty ());

      /* We kept threads moving at the end of their execution history.  Stop
	 EVENTING now that we are going to report its stop.  */
      eventing = unordered_remove (no_history, 0);
      eventing->btrace.flags &= ~BTHR_MOVE;

      *status = btrace_step_no_history ();
    }

  gdb_assert (eventing != NULL);

  /* We kept threads replaying at the end of their execution history.  Stop
     replaying EVENTING now that we are going to report its stop.  */
  record_btrace_stop_replaying_at_end (eventing);

  /* Stop all other threads. */
  if (!target_is_non_stop_p ())
    {
      for (thread_info *tp : current_inferior ()->non_exited_threads ())
	record_btrace_cancel_resume (tp);
    }

  /* In async mode, we need to announce further events.  */
  if (target_is_async_p ())
    record_btrace_maybe_mark_async_event (moving, no_history);

  /* Start record histories anew from the current position.  */
  record_btrace_clear_histories (&eventing->btrace);

  /* We moved the replay position but did not update registers.  */
  registers_changed_thread (eventing);

  DEBUG ("wait ended by thread %s (%s): %s",
	 print_thread_id (eventing),
	 eventing->ptid.to_string ().c_str (),
	 status->to_string ().c_str ());

  return eventing->ptid;
}

/* The stop method of target record-btrace.  */

void
record_btrace_target::stop (ptid_t ptid)
{
  DEBUG ("stop %s", ptid.to_string ().c_str ());

  /* As long as we're not replaying, just forward the request.  */
  if ((::execution_direction != EXEC_REVERSE)
      && !record_is_replaying (minus_one_ptid))
    {
      this->beneath ()->stop (ptid);
    }
  else
    {
      process_stratum_target *proc_target
	= current_inferior ()->process_target ();

      for (thread_info *tp : all_non_exited_threads (proc_target, ptid))
	{
	  tp->btrace.flags &= ~BTHR_MOVE;
	  tp->btrace.flags |= BTHR_STOP;
	}
    }
 }

/* The can_execute_reverse method of target record-btrace.  */

bool
record_btrace_target::can_execute_reverse ()
{
  return true;
}

/* The stopped_by_sw_breakpoint method of target record-btrace.  */

bool
record_btrace_target::stopped_by_sw_breakpoint ()
{
  if (record_is_replaying (minus_one_ptid))
    {
      struct thread_info *tp = inferior_thread ();

      return tp->btrace.stop_reason == TARGET_STOPPED_BY_SW_BREAKPOINT;
    }

  return this->beneath ()->stopped_by_sw_breakpoint ();
}

/* The supports_stopped_by_sw_breakpoint method of target
   record-btrace.  */

bool
record_btrace_target::supports_stopped_by_sw_breakpoint ()
{
  if (record_is_replaying (minus_one_ptid))
    return true;

  return this->beneath ()->supports_stopped_by_sw_breakpoint ();
}

/* The stopped_by_sw_breakpoint method of target record-btrace.  */

bool
record_btrace_target::stopped_by_hw_breakpoint ()
{
  if (record_is_replaying (minus_one_ptid))
    {
      struct thread_info *tp = inferior_thread ();

      return tp->btrace.stop_reason == TARGET_STOPPED_BY_HW_BREAKPOINT;
    }

  return this->beneath ()->stopped_by_hw_breakpoint ();
}

/* The supports_stopped_by_hw_breakpoint method of target
   record-btrace.  */

bool
record_btrace_target::supports_stopped_by_hw_breakpoint ()
{
  if (record_is_replaying (minus_one_ptid))
    return true;

  return this->beneath ()->supports_stopped_by_hw_breakpoint ();
}

/* The update_thread_list method of target record-btrace.  */

void
record_btrace_target::update_thread_list ()
{
  /* We don't add or remove threads during replay.  */
  if (record_is_replaying (minus_one_ptid))
    return;

  /* Forward the request.  */
  this->beneath ()->update_thread_list ();
}

/* The thread_alive method of target record-btrace.  */

bool
record_btrace_target::thread_alive (ptid_t ptid)
{
  /* We don't add or remove threads during replay.  */
  if (record_is_replaying (minus_one_ptid))
    return true;

  /* Forward the request.  */
  return this->beneath ()->thread_alive (ptid);
}

/* Set the replay branch trace instruction iterator.  If IT is NULL, replay
   is stopped.  */

static void
record_btrace_set_replay (struct thread_info *tp,
			  const struct btrace_insn_iterator *it)
{
  struct btrace_thread_info *btinfo;

  btinfo = &tp->btrace;

  if (it == NULL)
    record_btrace_stop_replaying (tp);
  else
    {
      if (btinfo->replay == NULL)
	record_btrace_start_replaying (tp);
      else if (btrace_insn_cmp (btinfo->replay, it) == 0)
	return;

      *btinfo->replay = *it;
      registers_changed_thread (tp);
    }

  /* Start anew from the new replay position.  */
  record_btrace_clear_histories (btinfo);

  tp->set_stop_pc (regcache_read_pc (get_thread_regcache (tp)));
  print_stack_frame (get_selected_frame (NULL), 1, SRC_AND_LOC, 1);
}

/* The goto_record_begin method of target record-btrace.  */

void
record_btrace_target::goto_record_begin ()
{
  struct thread_info *tp;
  struct btrace_insn_iterator begin;

  tp = require_btrace_thread ();

  btrace_insn_begin (&begin, &tp->btrace);

  /* Skip gaps at the beginning of the trace.  */
  while (btrace_insn_get (&begin) == NULL)
    {
      unsigned int steps;

      steps = btrace_insn_next (&begin, 1);
      if (steps == 0)
	error (_("No trace."));
    }

  record_btrace_set_replay (tp, &begin);
}

/* The goto_record_end method of target record-btrace.  */

void
record_btrace_target::goto_record_end ()
{
  struct thread_info *tp;

  tp = require_btrace_thread ();

  record_btrace_set_replay (tp, NULL);
}

/* The goto_record method of target record-btrace.  */

void
record_btrace_target::goto_record (ULONGEST insn)
{
  struct thread_info *tp;
  struct btrace_insn_iterator it;
  unsigned int number;
  int found;

  number = insn;

  /* Check for wrap-arounds.  */
  if (number != insn)
    error (_("Instruction number out of range."));

  tp = require_btrace_thread ();

  found = btrace_find_insn_by_number (&it, &tp->btrace, number);

  /* Check if the instruction could not be found or is a gap.  */
  if (found == 0 || btrace_insn_get (&it) == NULL)
    error (_("No such instruction."));

  record_btrace_set_replay (tp, &it);
}

/* The record_stop_replaying method of target record-btrace.  */

void
record_btrace_target::record_stop_replaying ()
{
  for (thread_info *tp : current_inferior ()->non_exited_threads ())
    record_btrace_stop_replaying (tp);
}

/* The execution_direction target method.  */

enum exec_direction_kind
record_btrace_target::execution_direction ()
{
  return record_btrace_resume_exec_dir;
}

/* The prepare_to_generate_core target method.  */

void
record_btrace_target::prepare_to_generate_core ()
{
  record_btrace_generating_corefile = 1;
}

/* The done_generating_core target method.  */

void
record_btrace_target::done_generating_core ()
{
  record_btrace_generating_corefile = 0;
}

/* Start recording in BTS format.  */

static void
cmd_record_btrace_bts_start (const char *args, int from_tty)
{
  if (args != NULL && *args != 0)
    error (_("Invalid argument."));

  record_btrace_conf.format = BTRACE_FORMAT_BTS;

  try
    {
      execute_command ("target record-btrace", from_tty);
    }
  catch (const gdb_exception &exception)
    {
      record_btrace_conf.format = BTRACE_FORMAT_NONE;
      throw;
    }
}

/* Start recording in Intel Processor Trace format.  */

static void
cmd_record_btrace_pt_start (const char *args, int from_tty)
{
  if (args != NULL && *args != 0)
    error (_("Invalid argument."));

  record_btrace_conf.format = BTRACE_FORMAT_PT;

  try
    {
      execute_command ("target record-btrace", from_tty);
    }
  catch (const gdb_exception &exception)
    {
      record_btrace_conf.format = BTRACE_FORMAT_NONE;
      throw;
    }
}

/* Alias for "target record".  */

static void
cmd_record_btrace_start (const char *args, int from_tty)
{
  if (args != NULL && *args != 0)
    error (_("Invalid argument."));

  record_btrace_conf.format = BTRACE_FORMAT_PT;

  try
    {
      execute_command ("target record-btrace", from_tty);
    }
  catch (const gdb_exception_error &exception)
    {
      record_btrace_conf.format = BTRACE_FORMAT_BTS;

      try
	{
	  execute_command ("target record-btrace", from_tty);
	}
      catch (const gdb_exception &ex)
	{
	  record_btrace_conf.format = BTRACE_FORMAT_NONE;
	  throw;
	}
    }
}

/* The "show record btrace replay-memory-access" command.  */

static void
cmd_show_replay_memory_access (struct ui_file *file, int from_tty,
			       struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Replay memory access is %s.\n"),
	      replay_memory_access);
}

/* The "set record btrace cpu none" command.  */

static void
cmd_set_record_btrace_cpu_none (const char *args, int from_tty)
{
  if (args != nullptr && *args != 0)
    error (_("Trailing junk: '%s'."), args);

  record_btrace_cpu_state = CS_NONE;
}

/* The "set record btrace cpu auto" command.  */

static void
cmd_set_record_btrace_cpu_auto (const char *args, int from_tty)
{
  if (args != nullptr && *args != 0)
    error (_("Trailing junk: '%s'."), args);

  record_btrace_cpu_state = CS_AUTO;
}

/* The "set record btrace cpu" command.  */

static void
cmd_set_record_btrace_cpu (const char *args, int from_tty)
{
  if (args == nullptr)
    args = "";

  /* We use a hard-coded vendor string for now.  */
  unsigned int family, model, stepping;
  int l1, l2, matches = sscanf (args, "intel: %u/%u%n/%u%n", &family,
				&model, &l1, &stepping, &l2);
  if (matches == 3)
    {
      if (strlen (args) != l2)
	error (_("Trailing junk: '%s'."), args + l2);
    }
  else if (matches == 2)
    {
      if (strlen (args) != l1)
	error (_("Trailing junk: '%s'."), args + l1);

      stepping = 0;
    }
  else
    error (_("Bad format.  See \"help set record btrace cpu\"."));

  if (USHRT_MAX < family)
    error (_("Cpu family too big."));

  if (UCHAR_MAX < model)
    error (_("Cpu model too big."));

  if (UCHAR_MAX < stepping)
    error (_("Cpu stepping too big."));

  record_btrace_cpu.vendor = CV_INTEL;
  record_btrace_cpu.family = family;
  record_btrace_cpu.model = model;
  record_btrace_cpu.stepping = stepping;

  record_btrace_cpu_state = CS_CPU;
}

/* The "show record btrace cpu" command.  */

static void
cmd_show_record_btrace_cpu (const char *args, int from_tty)
{
  if (args != nullptr && *args != 0)
    error (_("Trailing junk: '%s'."), args);

  switch (record_btrace_cpu_state)
    {
    case CS_AUTO:
      gdb_printf (_("btrace cpu is 'auto'.\n"));
      return;

    case CS_NONE:
      gdb_printf (_("btrace cpu is 'none'.\n"));
      return;

    case CS_CPU:
      switch (record_btrace_cpu.vendor)
	{
	case CV_INTEL:
	  if (record_btrace_cpu.stepping == 0)
	    gdb_printf (_("btrace cpu is 'intel: %u/%u'.\n"),
			record_btrace_cpu.family,
			record_btrace_cpu.model);
	  else
	    gdb_printf (_("btrace cpu is 'intel: %u/%u/%u'.\n"),
			record_btrace_cpu.family,
			record_btrace_cpu.model,
			record_btrace_cpu.stepping);
	  return;
	}
    }

  error (_("Internal error: bad cpu state."));
}

/* The "record bts buffer-size" show value function.  */

static void
show_record_bts_buffer_size_value (struct ui_file *file, int from_tty,
				   struct cmd_list_element *c,
				   const char *value)
{
  gdb_printf (file, _("The record/replay bts buffer size is %s.\n"),
	      value);
}

/* The "record pt buffer-size" show value function.  */

static void
show_record_pt_buffer_size_value (struct ui_file *file, int from_tty,
				  struct cmd_list_element *c,
				  const char *value)
{
  gdb_printf (file, _("The record/replay pt buffer size is %s.\n"),
	      value);
}

/* Initialize btrace commands.  */

void _initialize_record_btrace ();
void
_initialize_record_btrace ()
{
  cmd_list_element *record_btrace_cmd
    = add_prefix_cmd ("btrace", class_obscure, cmd_record_btrace_start,
		      _("Start branch trace recording."),
		      &record_btrace_cmdlist, 0, &record_cmdlist);
  add_alias_cmd ("b", record_btrace_cmd, class_obscure, 1, &record_cmdlist);

  cmd_list_element *record_btrace_bts_cmd
    = add_cmd ("bts", class_obscure, cmd_record_btrace_bts_start,
	       _("\
Start branch trace recording in Branch Trace Store (BTS) format.\n\n\
The processor stores a from/to record for each branch into a cyclic buffer.\n\
This format may not be available on all processors."),
	     &record_btrace_cmdlist);
  add_alias_cmd ("bts", record_btrace_bts_cmd, class_obscure, 1,
		 &record_cmdlist);

  cmd_list_element *record_btrace_pt_cmd
    = add_cmd ("pt", class_obscure, cmd_record_btrace_pt_start,
	       _("\
Start branch trace recording in Intel Processor Trace format.\n\n\
This format may not be available on all processors."),
	     &record_btrace_cmdlist);
  add_alias_cmd ("pt", record_btrace_pt_cmd, class_obscure, 1, &record_cmdlist);

  add_setshow_prefix_cmd ("btrace", class_support,
			  _("Set record options."),
			  _("Show record options."),
			  &set_record_btrace_cmdlist,
			  &show_record_btrace_cmdlist,
			  &set_record_cmdlist, &show_record_cmdlist);

  add_setshow_enum_cmd ("replay-memory-access", no_class,
			replay_memory_access_types, &replay_memory_access, _("\
Set what memory accesses are allowed during replay."), _("\
Show what memory accesses are allowed during replay."),
			   _("Default is READ-ONLY.\n\n\
The btrace record target does not trace data.\n\
The memory therefore corresponds to the live target and not \
to the current replay position.\n\n\
When READ-ONLY, allow accesses to read-only memory during replay.\n\
When READ-WRITE, allow accesses to read-only and read-write memory during \
replay."),
			   NULL, cmd_show_replay_memory_access,
			   &set_record_btrace_cmdlist,
			   &show_record_btrace_cmdlist);

  add_prefix_cmd ("cpu", class_support, cmd_set_record_btrace_cpu,
		  _("\
Set the cpu to be used for trace decode.\n\n\
The format is \"VENDOR:IDENTIFIER\" or \"none\" or \"auto\" (default).\n\
For vendor \"intel\" the format is \"FAMILY/MODEL[/STEPPING]\".\n\n\
When decoding branch trace, enable errata workarounds for the specified cpu.\n\
The default is \"auto\", which uses the cpu on which the trace was recorded.\n\
When GDB does not support that cpu, this option can be used to enable\n\
workarounds for a similar cpu that GDB supports.\n\n\
When set to \"none\", errata workarounds are disabled."),
		  &set_record_btrace_cpu_cmdlist,
		  1,
		  &set_record_btrace_cmdlist);

  add_cmd ("auto", class_support, cmd_set_record_btrace_cpu_auto, _("\
Automatically determine the cpu to be used for trace decode."),
	   &set_record_btrace_cpu_cmdlist);

  add_cmd ("none", class_support, cmd_set_record_btrace_cpu_none, _("\
Do not enable errata workarounds for trace decode."),
	   &set_record_btrace_cpu_cmdlist);

  add_cmd ("cpu", class_support, cmd_show_record_btrace_cpu, _("\
Show the cpu to be used for trace decode."),
	   &show_record_btrace_cmdlist);

  add_setshow_prefix_cmd ("bts", class_support,
			  _("Set record btrace bts options."),
			  _("Show record btrace bts options."),
			  &set_record_btrace_bts_cmdlist,
			  &show_record_btrace_bts_cmdlist,
			  &set_record_btrace_cmdlist,
			  &show_record_btrace_cmdlist);

  add_setshow_uinteger_cmd ("buffer-size", no_class,
			    &record_btrace_conf.bts.size,
			    _("Set the record/replay bts buffer size."),
			    _("Show the record/replay bts buffer size."), _("\
When starting recording request a trace buffer of this size.  \
The actual buffer size may differ from the requested size.  \
Use \"info record\" to see the actual buffer size.\n\n\
Bigger buffers allow longer recording but also take more time to process \
the recorded execution trace.\n\n\
The trace buffer size may not be changed while recording."), NULL,
			    show_record_bts_buffer_size_value,
			    &set_record_btrace_bts_cmdlist,
			    &show_record_btrace_bts_cmdlist);

  add_setshow_prefix_cmd ("pt", class_support,
			  _("Set record btrace pt options."),
			  _("Show record btrace pt options."),
			  &set_record_btrace_pt_cmdlist,
			  &show_record_btrace_pt_cmdlist,
			  &set_record_btrace_cmdlist,
			  &show_record_btrace_cmdlist);

  add_setshow_uinteger_cmd ("buffer-size", no_class,
			    &record_btrace_conf.pt.size,
			    _("Set the record/replay pt buffer size."),
			    _("Show the record/replay pt buffer size."), _("\
Bigger buffers allow longer recording but also take more time to process \
the recorded execution.\n\
The actual buffer size may differ from the requested size.  Use \"info record\" \
to see the actual buffer size."), NULL, show_record_pt_buffer_size_value,
			    &set_record_btrace_pt_cmdlist,
			    &show_record_btrace_pt_cmdlist);

  add_target (record_btrace_target_info, record_btrace_target_open);

  bfcache = htab_create_alloc (50, bfcache_hash, bfcache_eq, NULL,
			       xcalloc, xfree);

  record_btrace_conf.bts.size = 64 * 1024;
  record_btrace_conf.pt.size = 16 * 1024;
}
