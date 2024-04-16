/* Common definitions for remote server for GDB.
   Copyright (C) 1993-2024 Free Software Foundation, Inc.

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

#ifndef GDBSERVER_SERVER_H
#define GDBSERVER_SERVER_H

#include "gdbsupport/common-defs.h"

#undef PACKAGE
#undef PACKAGE_NAME
#undef PACKAGE_VERSION
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME

#include <config.h>

static_assert (sizeof (CORE_ADDR) >= sizeof (void *));

#include "gdbsupport/version.h"

#if !HAVE_DECL_PERROR
#ifndef perror
extern void perror (const char *);
#endif
#endif

#if !HAVE_DECL_VASPRINTF
extern int vasprintf(char **strp, const char *fmt, va_list ap);
#endif
#if !HAVE_DECL_VSNPRINTF
int vsnprintf(char *str, size_t size, const char *format, va_list ap);
#endif

#ifdef IN_PROCESS_AGENT
#  define PROG "ipa"
#else
#  define PROG "gdbserver"
#endif

#include "gdbsupport/xml-utils.h"
#include "regcache.h"
#include "gdbsupport/gdb_signals.h"
#include "target.h"
#include "mem-break.h"
#include "gdbsupport/environ.h"

/* Target-specific functions */

void initialize_low ();

/* Public variables in server.c */

extern bool server_waiting;

extern bool disable_packet_vCont;
extern bool disable_packet_Tthread;
extern bool disable_packet_qC;
extern bool disable_packet_qfThreadInfo;
extern bool disable_packet_T;

extern bool run_once;
extern bool non_stop;

#include "gdbsupport/event-loop.h"

/* Functions from server.c.  */
extern void handle_v_requests (char *own_buf, int packet_len,
			       int *new_packet_len);
extern void handle_serial_event (int err, gdb_client_data client_data);
extern void handle_target_event (int err, gdb_client_data client_data);

/* Get rid of the currently pending stop replies that match PTID.  */
extern void discard_queued_stop_replies (ptid_t ptid);

/* Returns true if there's a pending stop reply that matches PTID in
   the vStopped notifications queue.  */
extern int in_queued_stop_replies (ptid_t ptid);

#include "remote-utils.h"

#include "utils.h"
#include "debug.h"
#include "gdbsupport/gdb_vecs.h"

/* Maximum number of bytes to read/write at once.  The value here
   is chosen to fill up a packet (the headers account for the 32).  */
#define MAXBUFBYTES(N) (((N)-32)/2)

/* Buffer sizes for transferring memory, registers, etc.   Set to a constant
   value to accommodate multiple register formats.  This value must be at least
   as large as the largest register set supported by gdbserver.  */
#define PBUFSIZ 131104

/* Definition for an unknown syscall, used basically in error-cases.  */
#define UNKNOWN_SYSCALL (-1)

/* Definition for any syscall, used for unfiltered syscall reporting.  */
#define ANY_SYSCALL (-2)

/* After fork_inferior has been called, we need to adjust a few
   signals and call startup_inferior to start the inferior and consume
   its first events.  This is done here.  PID is the pid of the new
   inferior and PROGRAM is its name.  */
extern void post_fork_inferior (int pid, const char *program);

/* Get the gdb_environ being used in the current session.  */
extern gdb_environ *get_environ ();

extern unsigned long signal_pid;


/* Description of the client remote protocol state for the currently
   connected client.  */

struct client_state
{
  client_state ():
    own_buf ((char *) xmalloc (PBUFSIZ + 1)) 
  {}

  /* The thread set with an `Hc' packet.  `Hc' is deprecated in favor of
     `vCont'.  Note the multi-process extensions made `vCont' a
     requirement, so `Hc pPID.TID' is pretty much undefined.  So
     CONT_THREAD can be null_ptid for no `Hc' thread, minus_one_ptid for
     resuming all threads of the process (again, `Hc' isn't used for
     multi-process), or a specific thread ptid_t.  */
  ptid_t cont_thread;

  /* The thread set with an `Hg' packet.  */
  ptid_t general_thread;

  int multi_process = 0;
  int report_fork_events = 0;
  int report_vfork_events = 0;
  int report_exec_events = 0;
  int report_thread_events = 0;

  /* True if the "swbreak+" feature is active.  In that case, GDB wants
     us to report whether a trap is explained by a software breakpoint
     and for the server to handle PC adjustment if necessary on this
     target.  Only enabled if the target supports it.  */
  int swbreak_feature = 0;
  /* True if the "hwbreak+" feature is active.  In that case, GDB wants
     us to report whether a trap is explained by a hardware breakpoint.
     Only enabled if the target supports it.  */
  int hwbreak_feature = 0;

  /* True if the "vContSupported" feature is active.  In that case, GDB
     wants us to report whether single step is supported in the reply to
     "vCont?" packet.  */
  int vCont_supported = 0;

  /* Whether we should attempt to disable the operating system's address
     space randomization feature before starting an inferior.  */
  int disable_randomization = 1;

  int pass_signals[GDB_SIGNAL_LAST];
  int program_signals[GDB_SIGNAL_LAST];
  int program_signals_p = 0;

  /* Last status reported to GDB.  */
  struct target_waitstatus last_status;
  ptid_t last_ptid;

  char *own_buf;

  /* If true, then GDB has requested noack mode.  */
  int noack_mode = 0;
  /* If true, then we tell GDB to use noack mode by default.  */
  int transport_is_reliable = 0;

  /* The traceframe to be used as the source of data to send back to
     GDB.  A value of -1 means to get data from the live program.  */

  int current_traceframe = -1;

  /* If true, memory tagging features are supported.  */
  bool memory_tagging_feature = false;

};

client_state &get_client_state ();

#include "gdbthread.h"
#include "inferiors.h"

#endif /* GDBSERVER_SERVER_H */
