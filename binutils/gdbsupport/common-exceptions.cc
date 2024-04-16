/* Exception (throw catch) mechanism, for GDB, the GNU debugger.

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

#include "common-defs.h"
#include "common-exceptions.h"
#include <forward_list>

/* Possible catcher states.  */
enum catcher_state {
  /* Initial state, a new catcher has just been created.  */
  CATCHER_CREATED,
  /* The catch code is running.  */
  CATCHER_RUNNING,
  CATCHER_RUNNING_1,
  /* The catch code threw an exception.  */
  CATCHER_ABORTING
};

/* Possible catcher actions.  */
enum catcher_action {
  CATCH_ITER,
  CATCH_ITER_1,
  CATCH_THROWING
};

struct catcher
{
  enum catcher_state state = CATCHER_CREATED;
  /* Jump buffer pointing back at the exception handler.  */
  jmp_buf buf;
  /* Status buffer belonging to the exception handler.  */
  struct gdb_exception exception;
};

/* Where to go for throw_exception().  */
static std::forward_list<struct catcher> catchers;

jmp_buf *
exceptions_state_mc_init ()
{
  catchers.emplace_front ();
  return &catchers.front ().buf;
}

/* Catcher state machine.  Returns non-zero if the m/c should be run
   again, zero if it should abort.  */

static int
exceptions_state_mc (enum catcher_action action)
{
  switch (catchers.front ().state)
    {
    case CATCHER_CREATED:
      switch (action)
	{
	case CATCH_ITER:
	  /* Allow the code to run the catcher.  */
	  catchers.front ().state = CATCHER_RUNNING;
	  return 1;
	default:
	  internal_error (_("bad state"));
	}
    case CATCHER_RUNNING:
      switch (action)
	{
	case CATCH_ITER:
	  /* No error/quit has occurred.  */
	  return 0;
	case CATCH_ITER_1:
	  catchers.front ().state = CATCHER_RUNNING_1;
	  return 1;
	case CATCH_THROWING:
	  catchers.front ().state = CATCHER_ABORTING;
	  /* See also throw_exception.  */
	  return 1;
	default:
	  internal_error (_("bad switch"));
	}
    case CATCHER_RUNNING_1:
      switch (action)
	{
	case CATCH_ITER:
	  /* The did a "break" from the inner while loop.  */
	  return 0;
	case CATCH_ITER_1:
	  catchers.front ().state = CATCHER_RUNNING;
	  return 0;
	case CATCH_THROWING:
	  catchers.front ().state = CATCHER_ABORTING;
	  /* See also throw_exception.  */
	  return 1;
	default:
	  internal_error (_("bad switch"));
	}
    case CATCHER_ABORTING:
      switch (action)
	{
	case CATCH_ITER:
	  {
	    /* Exit normally if this catcher can handle this
	       exception.  The caller analyses the func return
	       values.  */
	    return 0;
	  }
	default:
	  internal_error (_("bad state"));
	}
    default:
      internal_error (_("bad switch"));
    }
}

int
exceptions_state_mc_catch (struct gdb_exception *exception,
			   int mask)
{
  *exception = std::move (catchers.front ().exception);
  catchers.pop_front ();

  if (exception->reason < 0)
    {
      if (mask & RETURN_MASK (exception->reason))
	{
	  /* Exit normally and let the caller handle the
	     exception.  */
	  return 1;
	}

      /* The caller didn't request that the event be caught, relay the
	 event to the next exception_catch/CATCH_SJLJ.  */
      throw_exception_sjlj (*exception);
    }

  /* No exception was thrown.  */
  return 0;
}

int
exceptions_state_mc_action_iter (void)
{
  return exceptions_state_mc (CATCH_ITER);
}

int
exceptions_state_mc_action_iter_1 (void)
{
  return exceptions_state_mc (CATCH_ITER_1);
}

/* Return EXCEPTION to the nearest containing CATCH_SJLJ block.  */

void
throw_exception_sjlj (const struct gdb_exception &exception)
{
  /* Jump to the nearest CATCH_SJLJ block, communicating REASON to
     that call via setjmp's return value.  Note that REASON can't be
     zero, by definition in common-exceptions.h.  */
  exceptions_state_mc (CATCH_THROWING);
  enum return_reason reason = exception.reason;
  catchers.front ().exception = exception;
  longjmp (catchers.front ().buf, reason);
}

/* Implementation of throw_exception that uses C++ try/catch.  */

void
throw_exception (gdb_exception &&exception)
{
  if (exception.reason == RETURN_QUIT)
    throw gdb_exception_quit (std::move (exception));
  else if (exception.reason == RETURN_FORCED_QUIT)
    throw gdb_exception_forced_quit (std::move (exception));
  else if (exception.reason == RETURN_ERROR)
    throw gdb_exception_error (std::move (exception));
  else
    gdb_assert_not_reached ("invalid return reason");
}

static void ATTRIBUTE_NORETURN ATTRIBUTE_PRINTF (3, 0)
throw_it (enum return_reason reason, enum errors error, const char *fmt,
	  va_list ap)
{
  if (reason == RETURN_QUIT)
    throw gdb_exception_quit (fmt, ap);
  else if (reason == RETURN_FORCED_QUIT)
    throw gdb_exception_forced_quit (fmt, ap);
  else if (reason == RETURN_ERROR)
    throw gdb_exception_error (error, fmt, ap);
  else
    gdb_assert_not_reached ("invalid return reason");
}

void
throw_verror (enum errors error, const char *fmt, va_list ap)
{
  throw_it (RETURN_ERROR, error, fmt, ap);
}

void
throw_vquit (const char *fmt, va_list ap)
{
  throw_it (RETURN_QUIT, GDB_NO_ERROR, fmt, ap);
}

void
throw_error (enum errors error, const char *fmt, ...)
{
  va_list args;

  va_start (args, fmt);
  throw_verror (error, fmt, args);
  va_end (args);
}

void
throw_quit (const char *fmt, ...)
{
  va_list args;

  va_start (args, fmt);
  throw_vquit (fmt, args);
  va_end (args);
}

void
throw_forced_quit (const char *fmt, ...)
{
  va_list args;

  va_start (args, fmt);
  throw_it (RETURN_FORCED_QUIT, GDB_NO_ERROR, fmt, args);
  va_end (args);
}
