/* Support for complaint handling during symbol reading in GDB.

   Copyright (C) 1990-2024 Free Software Foundation, Inc.

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
#include "complaints.h"
#include "command.h"
#include "gdbcmd.h"
#include "run-on-main-thread.h"
#include "gdbsupport/selftest.h"
#include <unordered_map>
#include <mutex>

/* Map format strings to counters.  */

static std::unordered_map<const char *, int> counters;

/* How many complaints about a particular thing should be printed
   before we stop whining about it?  Default is no whining at all,
   since so many systems have ill-constructed symbol files.  */

int stop_whining = 0;

#if CXX_STD_THREAD
static std::mutex complaint_mutex;
#endif /* CXX_STD_THREAD */

/* See complaints.h.  */

void
complaint_internal (const char *fmt, ...)
{
  va_list args;

  {
#if CXX_STD_THREAD
    std::lock_guard<std::mutex> guard (complaint_mutex);
#endif
    if (++counters[fmt] > stop_whining)
      return;
  }

  va_start (args, fmt);

  if (deprecated_warning_hook)
    (*deprecated_warning_hook) (fmt, args);
  else
    {
      gdb_puts (_("During symbol reading: "), gdb_stderr);
      gdb_vprintf (gdb_stderr, fmt, args);
      gdb_puts ("\n", gdb_stderr);
    }

  va_end (args);
}

/* See complaints.h.  */

void
clear_complaints ()
{
  counters.clear ();
}

/* See complaints.h.  */

thread_local complaint_interceptor *complaint_interceptor::g_complaint_interceptor;

/* See complaints.h.  */

complaint_interceptor::complaint_interceptor ()
  : m_saved_warning_hook (&deprecated_warning_hook, issue_complaint),
    m_saved_complaint_interceptor (&g_complaint_interceptor, this)
{
}

/* A helper that wraps a warning hook.  */

static void
wrap_warning_hook (void (*hook) (const char *, va_list), ...)
{
  va_list args;
  va_start (args, hook);
  hook ("%s", args);
  va_end (args);
}

/* See complaints.h.  */

void
re_emit_complaints (const complaint_collection &complaints)
{
  gdb_assert (is_main_thread ());

  for (const std::string &str : complaints)
    {
      if (deprecated_warning_hook)
	wrap_warning_hook (deprecated_warning_hook, str.c_str ());
      else
	gdb_printf (gdb_stderr, _("During symbol reading: %s\n"),
		    str.c_str ());
    }
}

/* See complaints.h.  */

void
complaint_interceptor::issue_complaint (const char *fmt, va_list args)
{
#if CXX_STD_THREAD
  std::lock_guard<std::mutex> guard (complaint_mutex);
#endif
  g_complaint_interceptor->m_complaints.insert (string_vprintf (fmt, args));
}

static void
complaints_show_value (struct ui_file *file, int from_tty,
		       struct cmd_list_element *cmd, const char *value)
{
  gdb_printf (file, _("Max number of complaints about incorrect"
		      " symbols is %s.\n"),
	      value);
}

#if GDB_SELF_TEST
namespace selftests {

/* Entry point for complaints unit tests.  */

static void
test_complaints ()
{
  std::unordered_map<const char *, int> tmp;
  scoped_restore reset_counters = make_scoped_restore (&counters, tmp);
  scoped_restore reset_stop_whining = make_scoped_restore (&stop_whining, 2);

#define CHECK_COMPLAINT(STR, CNT)					\
  do									\
    {									\
      std::string output;						\
      execute_fn_to_string (output, []() { complaint (STR); }, false);	\
      std::string expected						\
	= _("During symbol reading: ") + std::string (STR "\n");	\
      SELF_CHECK (output == expected);					\
      SELF_CHECK (counters[STR] == CNT);				\
    } while (0)

#define CHECK_COMPLAINT_SILENT(STR, CNT)				\
  do									\
    {									\
      std::string output;						\
      execute_fn_to_string (output, []() { complaint (STR); }, false);	\
      SELF_CHECK (output.empty ());					\
      SELF_CHECK (counters[STR] == CNT);				\
    } while (0)

  CHECK_COMPLAINT ("maintenance complaint 0", 1);
  CHECK_COMPLAINT ("maintenance complaint 0", 2);
  CHECK_COMPLAINT_SILENT ("maintenance complaint 0", 3);
  CHECK_COMPLAINT ("maintenance complaint 1", 1);
  clear_complaints ();
  CHECK_COMPLAINT ("maintenance complaint 0", 1);

#undef CHECK_COMPLAINT
#undef CHECK_COMPLAINT_SILENT
}


} // namespace selftests
#endif /* GDB_SELF_TEST */

void _initialize_complaints ();
void
_initialize_complaints ()
{
  add_setshow_zinteger_cmd ("complaints", class_support, 
			    &stop_whining, _("\
Set max number of complaints about incorrect symbols."), _("\
Show max number of complaints about incorrect symbols."), NULL,
			    NULL, complaints_show_value,
			    &setlist, &showlist);

#if GDB_SELF_TEST
  selftests::register_test ("complaints", selftests::test_complaints);
#endif /* GDB_SELF_TEST */
}
