/* Annotation routines for GDB.
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
#include "annotate.h"
#include "value.h"
#include "target.h"
#include "gdbtypes.h"
#include "breakpoint.h"
#include "observable.h"
#include "inferior.h"
#include "infrun.h"
#include "source.h"
#include "objfiles.h"
#include "source-cache.h"
#include "ui.h"

/* Prototypes for local functions.  */

static void print_value_flags (struct type *);

static void breakpoint_changed (struct breakpoint *b);


void (*deprecated_annotate_signalled_hook) (void);
void (*deprecated_annotate_signal_hook) (void);

/* Booleans indicating whether we've emitted certain notifications.
   Used to suppress useless repeated notifications until the next time
   we're ready to accept more commands.  Reset whenever a prompt is
   displayed.  */
static int frames_invalid_emitted;
static int breakpoints_invalid_emitted;

static void
print_value_flags (struct type *t)
{
  if (can_dereference (t))
    printf_unfiltered (("*"));
  else
    printf_unfiltered (("-"));
}

static void
annotate_breakpoints_invalid (void)
{
  if (annotation_level == 2
      && (!breakpoints_invalid_emitted
	  || current_ui->prompt_state != PROMPT_BLOCKED))
    {
      target_terminal::scoped_restore_terminal_state term_state;
      target_terminal::ours_for_output ();

      printf_unfiltered (("\n\032\032breakpoints-invalid\n"));
      breakpoints_invalid_emitted = 1;
    }
}

void
annotate_breakpoint (int num)
{
  if (annotation_level > 1)
    printf_unfiltered (("\n\032\032breakpoint %d\n"), num);
}

void
annotate_catchpoint (int num)
{
  if (annotation_level > 1)
    printf_unfiltered (("\n\032\032catchpoint %d\n"), num);
}

void
annotate_watchpoint (int num)
{
  if (annotation_level > 1)
    printf_unfiltered (("\n\032\032watchpoint %d\n"), num);
}

void
annotate_starting (void)
{
  if (annotation_level > 1)
    printf_unfiltered (("\n\032\032starting\n"));
}

void
annotate_stopped (void)
{
  if (annotation_level > 1)
    printf_unfiltered (("\n\032\032stopped\n"));
}

void
annotate_exited (int exitstatus)
{
  if (annotation_level > 1)
    printf_unfiltered (("\n\032\032exited %d\n"), exitstatus);
}

void
annotate_signalled (void)
{
  if (deprecated_annotate_signalled_hook)
    deprecated_annotate_signalled_hook ();

  if (annotation_level > 1)
    printf_unfiltered (("\n\032\032signalled\n"));
}

void
annotate_signal_name (void)
{
  if (annotation_level == 2)
    printf_unfiltered (("\n\032\032signal-name\n"));
}

void
annotate_signal_name_end (void)
{
  if (annotation_level == 2)
    printf_unfiltered (("\n\032\032signal-name-end\n"));
}

void
annotate_signal_string (void)
{
  if (annotation_level == 2)
    printf_unfiltered (("\n\032\032signal-string\n"));
}

void
annotate_signal_string_end (void)
{
  if (annotation_level == 2)
    printf_unfiltered (("\n\032\032signal-string-end\n"));
}

void
annotate_signal (void)
{
  if (deprecated_annotate_signal_hook)
    deprecated_annotate_signal_hook ();

  if (annotation_level > 1)
    printf_unfiltered (("\n\032\032signal\n"));
}

void
annotate_breakpoints_headers (void)
{
  if (annotation_level == 2)
    printf_unfiltered (("\n\032\032breakpoints-headers\n"));
}

void
annotate_field (int num)
{
  if (annotation_level == 2)
    printf_unfiltered (("\n\032\032field %d\n"), num);
}

void
annotate_breakpoints_table (void)
{
  if (annotation_level == 2)
    printf_unfiltered (("\n\032\032breakpoints-table\n"));
}

void
annotate_record (void)
{
  if (annotation_level == 2)
    printf_unfiltered (("\n\032\032record\n"));
}

void
annotate_breakpoints_table_end (void)
{
  if (annotation_level == 2)
    printf_unfiltered (("\n\032\032breakpoints-table-end\n"));
}

void
annotate_frames_invalid (void)
{
  if (annotation_level == 2
      && (!frames_invalid_emitted
	  || current_ui->prompt_state != PROMPT_BLOCKED))
    {
      target_terminal::scoped_restore_terminal_state term_state;
      target_terminal::ours_for_output ();

      printf_unfiltered (("\n\032\032frames-invalid\n"));
      frames_invalid_emitted = 1;
    }
}

void
annotate_new_thread (void)
{
  if (annotation_level > 1)
    {
      printf_unfiltered (("\n\032\032new-thread\n"));
    }
}

void
annotate_thread_changed (void)
{
  if (annotation_level > 1)
    {
      printf_unfiltered (("\n\032\032thread-changed\n"));
    }
}

/* Emit notification on thread exit.  */

static void
annotate_thread_exited (thread_info *t,
			std::optional<ULONGEST> exit_code,
			bool /* silent */)
{
  if (annotation_level > 1)
    {
      printf_unfiltered (("\n\032\032thread-exited,"
			  "id=\"%d\",group-id=\"i%d\"\n"),
			 t->global_num, t->inf->num);
    }
}

void
annotate_field_begin (struct type *type)
{
  if (annotation_level == 2)
    {
      printf_unfiltered (("\n\032\032field-begin "));
      print_value_flags (type);
      printf_unfiltered (("\n"));
    }
}

void
annotate_field_name_end (void)
{
  if (annotation_level == 2)
    printf_unfiltered (("\n\032\032field-name-end\n"));
}

void
annotate_field_value (void)
{
  if (annotation_level == 2)
    printf_unfiltered (("\n\032\032field-value\n"));
}

void
annotate_field_end (void)
{
  if (annotation_level == 2)
    printf_unfiltered (("\n\032\032field-end\n"));
}

void
annotate_quit (void)
{
  if (annotation_level > 1)
    printf_unfiltered (("\n\032\032quit\n"));
}

void
annotate_error (void)
{
  if (annotation_level > 1)
    printf_unfiltered (("\n\032\032error\n"));
}

void
annotate_error_begin (void)
{
  if (annotation_level > 1)
    gdb_printf (gdb_stderr, "\n\032\032error-begin\n");
}

void
annotate_value_history_begin (int histindex, struct type *type)
{
  if (annotation_level == 2)
    {
      printf_unfiltered (("\n\032\032value-history-begin %d "), histindex);
      print_value_flags (type);
      printf_unfiltered (("\n"));
    }
}

void
annotate_value_begin (struct type *type)
{
  if (annotation_level == 2)
    {
      printf_unfiltered (("\n\032\032value-begin "));
      print_value_flags (type);
      printf_unfiltered (("\n"));
    }
}

void
annotate_value_history_value (void)
{
  if (annotation_level == 2)
    printf_unfiltered (("\n\032\032value-history-value\n"));
}

void
annotate_value_history_end (void)
{
  if (annotation_level == 2)
    printf_unfiltered (("\n\032\032value-history-end\n"));
}

void
annotate_value_end (void)
{
  if (annotation_level == 2)
    printf_unfiltered (("\n\032\032value-end\n"));
}

void
annotate_display_begin (void)
{
  if (annotation_level == 2)
    printf_unfiltered (("\n\032\032display-begin\n"));
}

void
annotate_display_number_end (void)
{
  if (annotation_level == 2)
    printf_unfiltered (("\n\032\032display-number-end\n"));
}

void
annotate_display_format (void)
{
  if (annotation_level == 2)
    printf_unfiltered (("\n\032\032display-format\n"));
}

void
annotate_display_expression (void)
{
  if (annotation_level == 2)
    printf_unfiltered (("\n\032\032display-expression\n"));
}

void
annotate_display_expression_end (void)
{
  if (annotation_level == 2)
    printf_unfiltered (("\n\032\032display-expression-end\n"));
}

void
annotate_display_value (void)
{
  if (annotation_level == 2)
    printf_unfiltered (("\n\032\032display-value\n"));
}

void
annotate_display_end (void)
{
  if (annotation_level == 2)
    printf_unfiltered (("\n\032\032display-end\n"));
}

void
annotate_arg_begin (void)
{
  if (annotation_level == 2)
    printf_unfiltered (("\n\032\032arg-begin\n"));
}

void
annotate_arg_name_end (void)
{
  if (annotation_level == 2)
    printf_unfiltered (("\n\032\032arg-name-end\n"));
}

void
annotate_arg_value (struct type *type)
{
  if (annotation_level == 2)
    {
      printf_unfiltered (("\n\032\032arg-value "));
      print_value_flags (type);
      printf_unfiltered (("\n"));
    }
}

void
annotate_arg_end (void)
{
  if (annotation_level == 2)
    printf_unfiltered (("\n\032\032arg-end\n"));
}

static void
annotate_source (const char *filename, int line, int character, int mid,
		 struct gdbarch *gdbarch, CORE_ADDR pc)
{
  if (annotation_level > 1)
    printf_unfiltered (("\n\032\032source "));
  else
    printf_unfiltered (("\032\032"));

  printf_unfiltered (("%s:%d:%d:%s:%s\n"), filename, line, character,
		     mid ? "middle" : "beg", paddress (gdbarch, pc));
}

/* See annotate.h.  */

bool
annotate_source_line (struct symtab *s, int line, int mid_statement,
		      CORE_ADDR pc)
{
  if (annotation_level > 0)
    {
      const std::vector<off_t> *offsets;
      if (!g_source_cache.get_line_charpos (s, &offsets))
	return false;
      if (line > offsets->size ())
	return false;

      annotate_source (s->fullname, line, (int) (*offsets)[line - 1],
		       mid_statement, s->compunit ()->objfile ()->arch (),
		       pc);

      /* Update the current symtab and line.  */
      symtab_and_line sal;
      sal.pspace = s->compunit ()->objfile ()->pspace;
      sal.symtab = s;
      sal.line = line;
      set_current_source_symtab_and_line (sal);

      return true;
    }

  return false;
}


void
annotate_frame_begin (int level, struct gdbarch *gdbarch, CORE_ADDR pc)
{
  if (annotation_level > 1)
    printf_unfiltered (("\n\032\032frame-begin %d %s\n"),
		       level, paddress (gdbarch, pc));
}

void
annotate_function_call (void)
{
  if (annotation_level == 2)
    printf_unfiltered (("\n\032\032function-call\n"));
}

void
annotate_signal_handler_caller (void)
{
  if (annotation_level == 2)
    printf_unfiltered (("\n\032\032signal-handler-caller\n"));
}

void
annotate_frame_address (void)
{
  if (annotation_level == 2)
    printf_unfiltered (("\n\032\032frame-address\n"));
}

void
annotate_frame_address_end (void)
{
  if (annotation_level == 2)
    printf_unfiltered (("\n\032\032frame-address-end\n"));
}

void
annotate_frame_function_name (void)
{
  if (annotation_level == 2)
    printf_unfiltered (("\n\032\032frame-function-name\n"));
}

void
annotate_frame_args (void)
{
  if (annotation_level == 2)
    printf_unfiltered (("\n\032\032frame-args\n"));
}

void
annotate_frame_source_begin (void)
{
  if (annotation_level == 2)
    printf_unfiltered (("\n\032\032frame-source-begin\n"));
}

void
annotate_frame_source_file (void)
{
  if (annotation_level == 2)
    printf_unfiltered (("\n\032\032frame-source-file\n"));
}

void
annotate_frame_source_file_end (void)
{
  if (annotation_level == 2)
    printf_unfiltered (("\n\032\032frame-source-file-end\n"));
}

void
annotate_frame_source_line (void)
{
  if (annotation_level == 2)
    printf_unfiltered (("\n\032\032frame-source-line\n"));
}

void
annotate_frame_source_end (void)
{
  if (annotation_level == 2)
    printf_unfiltered (("\n\032\032frame-source-end\n"));
}

void
annotate_frame_where (void)
{
  if (annotation_level == 2)
    printf_unfiltered (("\n\032\032frame-where\n"));
}

void
annotate_frame_end (void)
{
  if (annotation_level == 2)
    printf_unfiltered (("\n\032\032frame-end\n"));
}

void
annotate_array_section_begin (int idx, struct type *elttype)
{
  if (annotation_level == 2)
    {
      printf_unfiltered (("\n\032\032array-section-begin %d "), idx);
      print_value_flags (elttype);
      printf_unfiltered (("\n"));
    }
}

void
annotate_elt_rep (unsigned int repcount)
{
  if (annotation_level == 2)
    printf_unfiltered (("\n\032\032elt-rep %u\n"), repcount);
}

void
annotate_elt_rep_end (void)
{
  if (annotation_level == 2)
    printf_unfiltered (("\n\032\032elt-rep-end\n"));
}

void
annotate_elt (void)
{
  if (annotation_level == 2)
    printf_unfiltered (("\n\032\032elt\n"));
}

void
annotate_array_section_end (void)
{
  if (annotation_level == 2)
    printf_unfiltered (("\n\032\032array-section-end\n"));
}

/* Called when GDB is about to display the prompt.  Used to reset
   annotation suppression whenever we're ready to accept new
   frontend/user commands.  */

void
annotate_display_prompt (void)
{
  frames_invalid_emitted = 0;
  breakpoints_invalid_emitted = 0;
}

static void
breakpoint_changed (struct breakpoint *b)
{
  if (b->number <= 0)
    return;

  annotate_breakpoints_invalid ();
}

void _initialize_annotate ();
void
_initialize_annotate ()
{
  gdb::observers::breakpoint_created.attach (breakpoint_changed, "annotate");
  gdb::observers::breakpoint_deleted.attach (breakpoint_changed, "annotate");
  gdb::observers::breakpoint_modified.attach (breakpoint_changed, "annotate");
  gdb::observers::thread_exit.attach (annotate_thread_exited, "annotate");
}
