/* Declarations for common target functions.

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

#ifndef TARGET_TARGET_H
#define TARGET_TARGET_H

#include "target/waitstatus.h"
#include "target/wait.h"
#include "gdbsupport/enum-flags.h"

/* This header is a stopgap until more code is shared.  */

/* Available thread options.  Keep this in sync with to_string, in
   target.c.  */

enum gdb_thread_option : unsigned
{
  /* Tell the target to report TARGET_WAITKIND_THREAD_CLONED events
     for the thread.  */
  GDB_THREAD_OPTION_CLONE = 1 << 0,

  /* Tell the target to report TARGET_WAITKIND_THREAD_EXIT events for
     the thread.  */
  GDB_THREAD_OPTION_EXIT = 1 << 1,
};

DEF_ENUM_FLAGS_TYPE (enum gdb_thread_option, gdb_thread_options);

/* Convert gdb_thread_option to a string.  */
extern std::string to_string (gdb_thread_options options);

/* Read LEN bytes of target memory at address MEMADDR, placing the
   results in GDB's memory at MYADDR.  Return zero for success,
   nonzero if any error occurs.  This function must be provided by
   the client.  Implementations of this function may define and use
   their own error codes, but functions in the common, nat and target
   directories must treat the return code as opaque.  No guarantee is
   made about the contents of the data at MYADDR if any error
   occurs.  */

extern int target_read_memory (CORE_ADDR memaddr, gdb_byte *myaddr,
			       ssize_t len);

/* Read an unsigned 32-bit integer in the target's format from target
   memory at address MEMADDR, storing the result in GDB's format in
   GDB's memory at RESULT.  Return zero for success, nonzero if any
   error occurs.  This function must be provided by the client.
   Implementations of this function may define and use their own error
   codes, but functions in the common, nat and target directories must
   treat the return code as opaque.  No guarantee is made about the
   contents of the data at RESULT if any error occurs.  */

extern int target_read_uint32 (CORE_ADDR memaddr, uint32_t *result);

/* Read a string from target memory at address MEMADDR.  The string
   will be at most LEN bytes long (note that excess bytes may be read
   in some cases -- but these will not be returned).  Returns nullptr
   on error.  */

extern gdb::unique_xmalloc_ptr<char> target_read_string
  (CORE_ADDR memaddr, int len, int *bytes_read = nullptr);

/* Read a string from the inferior, at ADDR, with LEN characters of
   WIDTH bytes each.  Fetch at most FETCHLIMIT characters.  BUFFER
   will be set to a newly allocated buffer containing the string, and
   BYTES_READ will be set to the number of bytes read.  Returns 0 on
   success, or a target_xfer_status on failure.

   If LEN > 0, reads the lesser of LEN or FETCHLIMIT characters
   (including eventual NULs in the middle or end of the string).

   If LEN is -1, stops at the first null character (not necessarily
   the first null byte) up to a maximum of FETCHLIMIT characters.  Set
   FETCHLIMIT to UINT_MAX to read as many characters as possible from
   the string.

   Unless an exception is thrown, BUFFER will always be allocated, even on
   failure.  In this case, some characters might have been read before the
   failure happened.  Check BYTES_READ to recognize this situation.  */

extern int target_read_string (CORE_ADDR addr, int len, int width,
			       unsigned int fetchlimit,
			       gdb::unique_xmalloc_ptr<gdb_byte> *buffer,
			       int *bytes_read);

/* Write LEN bytes from MYADDR to target memory at address MEMADDR.
   Return zero for success, nonzero if any error occurs.  This
   function must be provided by the client.  Implementations of this
   function may define and use their own error codes, but functions
   in the common, nat and target directories must treat the return
   code as opaque.  No guarantee is made about the contents of the
   data at MEMADDR if any error occurs.  */

extern int target_write_memory (CORE_ADDR memaddr, const gdb_byte *myaddr,
				ssize_t len);

/* Cause the target to stop in a continuable fashion--for instance,
   under Unix, this should act like SIGSTOP--and wait for the target
   to be stopped before returning.  This function must be provided by
   the client.  */

extern void target_stop_and_wait (ptid_t ptid);

/* Restart a target previously stopped.  No signal is delivered to the
   target.  This function must be provided by the client.  */

extern void target_continue_no_signal (ptid_t ptid);

/* Restart a target previously stopped.  SIGNAL is delivered to the
   target.  This function must be provided by the client.  */

extern void target_continue (ptid_t ptid, enum gdb_signal signal);

/* Wait for process pid to do something.  PTID = -1 to wait for any
   pid to do something.  Return pid of child, or -1 in case of error;
   store status through argument pointer STATUS.  Note that it is
   _NOT_ OK to throw_exception() out of target_wait() without popping
   the debugging target from the stack; GDB isn't prepared to get back
   to the prompt with a debugging target but without the frame cache,
   stop_pc, etc., set up.  OPTIONS is a bitwise OR of TARGET_W*
   options.  */

extern ptid_t target_wait (ptid_t ptid, struct target_waitstatus *status,
			   target_wait_flags options);

/* The inferior process has died.  Do what is right.  */

extern void target_mourn_inferior (ptid_t ptid);

/* Return 1 if this target can debug multiple processes
   simultaneously, zero otherwise.  */

extern int target_supports_multi_process (void);

/* Possible terminal states.  */

enum class target_terminal_state
  {
    /* The inferior's terminal settings are in effect.  */
    is_inferior = 0,

    /* Some of our terminal settings are in effect, enough to get
       proper output.  */
    is_ours_for_output = 1,

    /* Our terminal settings are in effect, for output and input.  */
    is_ours = 2
  };

/* Represents the state of the target terminal.  */
class target_terminal
{
public:

  target_terminal () = delete;
  ~target_terminal () = delete;
  DISABLE_COPY_AND_ASSIGN (target_terminal);

  /* Initialize the terminal settings we record for the inferior,
     before we actually run the inferior.  */
  static void init ();

  /* Put the current inferior's terminal settings into effect.  This
     is preparation for starting or resuming the inferior.  This is a
     no-op unless called with the main UI as current UI.  */
  static void inferior ();

  /* Put our terminal settings into effect.  First record the inferior's
     terminal settings so they can be restored properly later.  This is
     a no-op unless called with the main UI as current UI.  */
  static void ours ();

  /* Put some of our terminal settings into effect, enough to get proper
     results from our output, but do not change into or out of RAW mode
     so that no input is discarded.  This is a no-op if terminal_ours
     was most recently called.  This is a no-op unless called with the main
     UI as current UI.  */
  static void ours_for_output ();

  /* Restore terminal settings of inferiors that are in
     is_ours_for_output state back to "inferior".  Used when we need
     to temporarily switch to is_ours_for_output state.  */
  static void restore_inferior ();

  /* Returns true if the terminal settings of the inferior are in
     effect.  */
  static bool is_inferior ()
  {
    return m_terminal_state == target_terminal_state::is_inferior;
  }

  /* Returns true if our terminal settings are in effect.  */
  static bool is_ours ()
  {
    return m_terminal_state == target_terminal_state::is_ours;
  }

  /* Returns true if our terminal settings are in effect.  */
  static bool is_ours_for_output ()
  {
    return m_terminal_state == target_terminal_state::is_ours_for_output;
  }

  /* Print useful information about our terminal status, if such a thing
     exists.  */
  static void info (const char *arg, int from_tty);

public:

  /* A class that restores the state of the terminal to the current
     state.  */
  class scoped_restore_terminal_state
  {
  public:

    scoped_restore_terminal_state ()
      : m_state (m_terminal_state)
    {
    }

    ~scoped_restore_terminal_state ()
    {
      switch (m_state)
	{
	case target_terminal_state::is_ours:
	  ours ();
	  break;
	case target_terminal_state::is_ours_for_output:
	  ours_for_output ();
	  break;
	case target_terminal_state::is_inferior:
	  restore_inferior ();
	  break;
	}
    }

    DISABLE_COPY_AND_ASSIGN (scoped_restore_terminal_state);

  private:

    target_terminal_state m_state;
  };

private:

  static target_terminal_state m_terminal_state;
};

#endif /* TARGET_TARGET_H */
