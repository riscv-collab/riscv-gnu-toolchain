/* Main code for remote server for GDB.
   Copyright (C) 1989-2024 Free Software Foundation, Inc.

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

#include "server.h"
#include "gdbthread.h"
#include "gdbsupport/agent.h"
#include "notif.h"
#include "tdesc.h"
#include "gdbsupport/rsp-low.h"
#include "gdbsupport/signals-state-save-restore.h"
#include <ctype.h>
#include <unistd.h>
#if HAVE_SIGNAL_H
#include <signal.h>
#endif
#include "gdbsupport/gdb_vecs.h"
#include "gdbsupport/gdb_wait.h"
#include "gdbsupport/btrace-common.h"
#include "gdbsupport/filestuff.h"
#include "tracepoint.h"
#include "dll.h"
#include "hostio.h"
#include <vector>
#include <unordered_map>
#include "gdbsupport/common-inferior.h"
#include "gdbsupport/job-control.h"
#include "gdbsupport/environ.h"
#include "filenames.h"
#include "gdbsupport/pathstuff.h"
#ifdef USE_XML
#include "xml-builtin.h"
#endif

#include "gdbsupport/selftest.h"
#include "gdbsupport/scope-exit.h"
#include "gdbsupport/gdb_select.h"
#include "gdbsupport/scoped_restore.h"
#include "gdbsupport/search.h"

/* PBUFSIZ must also be at least as big as IPA_CMD_BUF_SIZE, because
   the client state data is passed directly to some agent
   functions.  */
static_assert (PBUFSIZ >= IPA_CMD_BUF_SIZE);

#define require_running_or_return(BUF)		\
  if (!target_running ())			\
    {						\
      write_enn (BUF);				\
      return;					\
    }

#define require_running_or_break(BUF)		\
  if (!target_running ())			\
    {						\
      write_enn (BUF);				\
      break;					\
    }

/* The environment to pass to the inferior when creating it.  */

static gdb_environ our_environ;

bool server_waiting;

static bool extended_protocol;
static bool response_needed;
static bool exit_requested;

/* --once: Exit after the first connection has closed.  */
bool run_once;

/* Whether to report TARGET_WAITKIND_NO_RESUMED events.  */
static bool report_no_resumed;

/* The event loop checks this to decide whether to continue accepting
   events.  */
static bool keep_processing_events = true;

bool non_stop;

static struct {
  /* Set the PROGRAM_PATH.  Here we adjust the path of the provided
     binary if needed.  */
  void set (const char *path)
  {
    m_path = path;

    /* Make sure we're using the absolute path of the inferior when
       creating it.  */
    if (!contains_dir_separator (m_path.c_str ()))
      {
	int reg_file_errno;

	/* Check if the file is in our CWD.  If it is, then we prefix
	   its name with CURRENT_DIRECTORY.  Otherwise, we leave the
	   name as-is because we'll try searching for it in $PATH.  */
	if (is_regular_file (m_path.c_str (), &reg_file_errno))
	  m_path = gdb_abspath (m_path.c_str ());
      }
  }

  /* Return the PROGRAM_PATH.  */
  const char *get ()
  { return m_path.empty () ? nullptr : m_path.c_str (); }

private:
  /* The program name, adjusted if needed.  */
  std::string m_path;
} program_path;
static std::vector<char *> program_args;
static std::string wrapper_argv;

/* The PID of the originally created or attached inferior.  Used to
   send signals to the process when GDB sends us an asynchronous interrupt
   (user hitting Control-C in the client), and to wait for the child to exit
   when no longer debugging it.  */

unsigned long signal_pid;

/* Set if you want to disable optional thread related packets support
   in gdbserver, for the sake of testing GDB against stubs that don't
   support them.  */
bool disable_packet_vCont;
bool disable_packet_Tthread;
bool disable_packet_qC;
bool disable_packet_qfThreadInfo;
bool disable_packet_T;

static unsigned char *mem_buf;

/* A sub-class of 'struct notif_event' for stop, holding information
   relative to a single stop reply.  We keep a queue of these to
   push to GDB in non-stop mode.  */

struct vstop_notif : public notif_event
{
  /* Thread or process that got the event.  */
  ptid_t ptid;

  /* Event info.  */
  struct target_waitstatus status;
};

/* The current btrace configuration.  This is gdbserver's mirror of GDB's
   btrace configuration.  */
static struct btrace_config current_btrace_conf;

/* The client remote protocol state. */

static client_state g_client_state;

client_state &
get_client_state ()
{
  client_state &cs = g_client_state;
  return cs;
}


/* Put a stop reply to the stop reply queue.  */

static void
queue_stop_reply (ptid_t ptid, const target_waitstatus &status)
{
  struct vstop_notif *new_notif = new struct vstop_notif;

  new_notif->ptid = ptid;
  new_notif->status = status;

  notif_event_enque (&notif_stop, new_notif);
}

static bool
remove_all_on_match_ptid (struct notif_event *event, ptid_t filter_ptid)
{
  struct vstop_notif *vstop_event = (struct vstop_notif *) event;

  return vstop_event->ptid.matches (filter_ptid);
}

/* See server.h.  */

void
discard_queued_stop_replies (ptid_t ptid)
{
  std::list<notif_event *>::iterator iter, next, end;
  end = notif_stop.queue.end ();
  for (iter = notif_stop.queue.begin (); iter != end; iter = next)
    {
      next = iter;
      ++next;

      if (iter == notif_stop.queue.begin ())
	{
	  /* The head of the list contains the notification that was
	     already sent to GDB.  So we can't remove it, otherwise
	     when GDB sends the vStopped, it would ack the _next_
	     notification, which hadn't been sent yet!  */
	  continue;
	}

      if (remove_all_on_match_ptid (*iter, ptid))
	{
	  delete *iter;
	  notif_stop.queue.erase (iter);
	}
    }
}

static void
vstop_notif_reply (struct notif_event *event, char *own_buf)
{
  struct vstop_notif *vstop = (struct vstop_notif *) event;

  prepare_resume_reply (own_buf, vstop->ptid, vstop->status);
}

/* Helper for in_queued_stop_replies.  */

static bool
in_queued_stop_replies_ptid (struct notif_event *event, ptid_t filter_ptid)
{
  struct vstop_notif *vstop_event = (struct vstop_notif *) event;

  if (vstop_event->ptid.matches (filter_ptid))
    return true;

  /* Don't resume fork children that GDB does not know about yet.  */
  if ((vstop_event->status.kind () == TARGET_WAITKIND_FORKED
       || vstop_event->status.kind () == TARGET_WAITKIND_VFORKED
       || vstop_event->status.kind () == TARGET_WAITKIND_THREAD_CLONED)
      && vstop_event->status.child_ptid ().matches (filter_ptid))
    return true;

  return false;
}

/* See server.h.  */

int
in_queued_stop_replies (ptid_t ptid)
{
  for (notif_event *event : notif_stop.queue)
    {
      if (in_queued_stop_replies_ptid (event, ptid))
	return true;
    }

  return false;
}

struct notif_server notif_stop =
{
  "vStopped", "Stop", {}, vstop_notif_reply,
};

static int
target_running (void)
{
  return get_first_thread () != NULL;
}

/* See gdbsupport/common-inferior.h.  */

const char *
get_exec_wrapper ()
{
  return !wrapper_argv.empty () ? wrapper_argv.c_str () : NULL;
}

/* See gdbsupport/common-inferior.h.  */

const char *
get_exec_file (int err)
{
  if (err && program_path.get () == NULL)
    error (_("No executable file specified."));

  return program_path.get ();
}

/* See server.h.  */

gdb_environ *
get_environ ()
{
  return &our_environ;
}

static int
attach_inferior (int pid)
{
  client_state &cs = get_client_state ();
  /* myattach should return -1 if attaching is unsupported,
     0 if it succeeded, and call error() otherwise.  */

  if (find_process_pid (pid) != nullptr)
    error ("Already attached to process %d\n", pid);

  if (myattach (pid) != 0)
    return -1;

  fprintf (stderr, "Attached; pid = %d\n", pid);
  fflush (stderr);

  /* FIXME - It may be that we should get the SIGNAL_PID from the
     attach function, so that it can be the main thread instead of
     whichever we were told to attach to.  */
  signal_pid = pid;

  if (!non_stop)
    {
      cs.last_ptid = mywait (ptid_t (pid), &cs.last_status, 0, 0);

      /* GDB knows to ignore the first SIGSTOP after attaching to a running
	 process using the "attach" command, but this is different; it's
	 just using "target remote".  Pretend it's just starting up.  */
      if (cs.last_status.kind () == TARGET_WAITKIND_STOPPED
	  && cs.last_status.sig () == GDB_SIGNAL_STOP)
	cs.last_status.set_stopped (GDB_SIGNAL_TRAP);

      current_thread->last_resume_kind = resume_stop;
      current_thread->last_status = cs.last_status;
    }

  return 0;
}

/* Decode a qXfer read request.  Return 0 if everything looks OK,
   or -1 otherwise.  */

static int
decode_xfer_read (char *buf, CORE_ADDR *ofs, unsigned int *len)
{
  /* After the read marker and annex, qXfer looks like a
     traditional 'm' packet.  */
  decode_m_packet (buf, ofs, len);

  return 0;
}

static int
decode_xfer (char *buf, char **object, char **rw, char **annex, char **offset)
{
  /* Extract and NUL-terminate the object.  */
  *object = buf;
  while (*buf && *buf != ':')
    buf++;
  if (*buf == '\0')
    return -1;
  *buf++ = 0;

  /* Extract and NUL-terminate the read/write action.  */
  *rw = buf;
  while (*buf && *buf != ':')
    buf++;
  if (*buf == '\0')
    return -1;
  *buf++ = 0;

  /* Extract and NUL-terminate the annex.  */
  *annex = buf;
  while (*buf && *buf != ':')
    buf++;
  if (*buf == '\0')
    return -1;
  *buf++ = 0;

  *offset = buf;
  return 0;
}

/* Write the response to a successful qXfer read.  Returns the
   length of the (binary) data stored in BUF, corresponding
   to as much of DATA/LEN as we could fit.  IS_MORE controls
   the first character of the response.  */
static int
write_qxfer_response (char *buf, const gdb_byte *data, int len, int is_more)
{
  int out_len;

  if (is_more)
    buf[0] = 'm';
  else
    buf[0] = 'l';

  return remote_escape_output (data, len, 1, (unsigned char *) buf + 1,
			       &out_len, PBUFSIZ - 2) + 1;
}

/* Handle btrace enabling in BTS format.  */

static void
handle_btrace_enable_bts (struct thread_info *thread)
{
  if (thread->btrace != NULL)
    error (_("Btrace already enabled."));

  current_btrace_conf.format = BTRACE_FORMAT_BTS;
  thread->btrace = target_enable_btrace (thread, &current_btrace_conf);
}

/* Handle btrace enabling in Intel Processor Trace format.  */

static void
handle_btrace_enable_pt (struct thread_info *thread)
{
  if (thread->btrace != NULL)
    error (_("Btrace already enabled."));

  current_btrace_conf.format = BTRACE_FORMAT_PT;
  thread->btrace = target_enable_btrace (thread, &current_btrace_conf);
}

/* Handle btrace disabling.  */

static void
handle_btrace_disable (struct thread_info *thread)
{

  if (thread->btrace == NULL)
    error (_("Branch tracing not enabled."));

  if (target_disable_btrace (thread->btrace) != 0)
    error (_("Could not disable branch tracing."));

  thread->btrace = NULL;
}

/* Handle the "Qbtrace" packet.  */

static int
handle_btrace_general_set (char *own_buf)
{
  client_state &cs = get_client_state ();
  struct thread_info *thread;
  char *op;

  if (!startswith (own_buf, "Qbtrace:"))
    return 0;

  op = own_buf + strlen ("Qbtrace:");

  if (cs.general_thread == null_ptid
      || cs.general_thread == minus_one_ptid)
    {
      strcpy (own_buf, "E.Must select a single thread.");
      return -1;
    }

  thread = find_thread_ptid (cs.general_thread);
  if (thread == NULL)
    {
      strcpy (own_buf, "E.No such thread.");
      return -1;
    }

  try
    {
      if (strcmp (op, "bts") == 0)
	handle_btrace_enable_bts (thread);
      else if (strcmp (op, "pt") == 0)
	handle_btrace_enable_pt (thread);
      else if (strcmp (op, "off") == 0)
	handle_btrace_disable (thread);
      else
	error (_("Bad Qbtrace operation.  Use bts, pt, or off."));

      write_ok (own_buf);
    }
  catch (const gdb_exception_error &exception)
    {
      sprintf (own_buf, "E.%s", exception.what ());
    }

  return 1;
}

/* Handle the "Qbtrace-conf" packet.  */

static int
handle_btrace_conf_general_set (char *own_buf)
{
  client_state &cs = get_client_state ();
  struct thread_info *thread;
  char *op;

  if (!startswith (own_buf, "Qbtrace-conf:"))
    return 0;

  op = own_buf + strlen ("Qbtrace-conf:");

  if (cs.general_thread == null_ptid
      || cs.general_thread == minus_one_ptid)
    {
      strcpy (own_buf, "E.Must select a single thread.");
      return -1;
    }

  thread = find_thread_ptid (cs.general_thread);
  if (thread == NULL)
    {
      strcpy (own_buf, "E.No such thread.");
      return -1;
    }

  if (startswith (op, "bts:size="))
    {
      unsigned long size;
      char *endp = NULL;

      errno = 0;
      size = strtoul (op + strlen ("bts:size="), &endp, 16);
      if (endp == NULL || *endp != 0 || errno != 0 || size > UINT_MAX)
	{
	  strcpy (own_buf, "E.Bad size value.");
	  return -1;
	}

      current_btrace_conf.bts.size = (unsigned int) size;
    }
  else if (strncmp (op, "pt:size=", strlen ("pt:size=")) == 0)
    {
      unsigned long size;
      char *endp = NULL;

      errno = 0;
      size = strtoul (op + strlen ("pt:size="), &endp, 16);
      if (endp == NULL || *endp != 0 || errno != 0 || size > UINT_MAX)
	{
	  strcpy (own_buf, "E.Bad size value.");
	  return -1;
	}

      current_btrace_conf.pt.size = (unsigned int) size;
    }
  else
    {
      strcpy (own_buf, "E.Bad Qbtrace configuration option.");
      return -1;
    }

  write_ok (own_buf);
  return 1;
}

/* Create the qMemTags packet reply given TAGS.

   Returns true if parsing succeeded and false otherwise.  */

static bool
create_fetch_memtags_reply (char *reply, const gdb::byte_vector &tags)
{
  /* It is an error to pass a zero-sized tag vector.  */
  gdb_assert (tags.size () != 0);

  std::string packet ("m");

  /* Write the tag data.  */
  packet += bin2hex (tags.data (), tags.size ());

  /* Check if the reply is too big for the packet to handle.  */
  if (PBUFSIZ < packet.size ())
    return false;

  strcpy (reply, packet.c_str ());
  return true;
}

/* Parse the QMemTags request into ADDR, LEN and TAGS.

   Returns true if parsing succeeded and false otherwise.  */

static bool
parse_store_memtags_request (char *request, CORE_ADDR *addr, size_t *len,
			     gdb::byte_vector &tags, int *type)
{
  gdb_assert (startswith (request, "QMemTags:"));

  const char *p = request + strlen ("QMemTags:");

  /* Read address and length.  */
  unsigned int length = 0;
  p = decode_m_packet_params (p, addr, &length, ':');
  *len = length;

  /* Read the tag type.  */
  ULONGEST tag_type = 0;
  p = unpack_varlen_hex (p, &tag_type);
  *type = (int) tag_type;

  /* Make sure there is a colon after the type.  */
  if (*p != ':')
    return false;

  /* Skip the colon.  */
  p++;

  /* Read the tag data.  */
  tags = hex2bin (p);

  return true;
}

/* Parse thread options starting at *P and return them.  On exit,
   advance *P past the options.  */

static gdb_thread_options
parse_gdb_thread_options (const char **p)
{
  ULONGEST options = 0;
  *p = unpack_varlen_hex (*p, &options);
  return (gdb_thread_option) options;
}

/* Handle all of the extended 'Q' packets.  */

static void
handle_general_set (char *own_buf)
{
  client_state &cs = get_client_state ();
  if (startswith (own_buf, "QPassSignals:"))
    {
      int numsigs = (int) GDB_SIGNAL_LAST, i;
      const char *p = own_buf + strlen ("QPassSignals:");
      CORE_ADDR cursig;

      p = decode_address_to_semicolon (&cursig, p);
      for (i = 0; i < numsigs; i++)
	{
	  if (i == cursig)
	    {
	      cs.pass_signals[i] = 1;
	      if (*p == '\0')
		/* Keep looping, to clear the remaining signals.  */
		cursig = -1;
	      else
		p = decode_address_to_semicolon (&cursig, p);
	    }
	  else
	    cs.pass_signals[i] = 0;
	}
      strcpy (own_buf, "OK");
      return;
    }

  if (startswith (own_buf, "QProgramSignals:"))
    {
      int numsigs = (int) GDB_SIGNAL_LAST, i;
      const char *p = own_buf + strlen ("QProgramSignals:");
      CORE_ADDR cursig;

      cs.program_signals_p = 1;

      p = decode_address_to_semicolon (&cursig, p);
      for (i = 0; i < numsigs; i++)
	{
	  if (i == cursig)
	    {
	      cs.program_signals[i] = 1;
	      if (*p == '\0')
		/* Keep looping, to clear the remaining signals.  */
		cursig = -1;
	      else
		p = decode_address_to_semicolon (&cursig, p);
	    }
	  else
	    cs.program_signals[i] = 0;
	}
      strcpy (own_buf, "OK");
      return;
    }

  if (startswith (own_buf, "QCatchSyscalls:"))
    {
      const char *p = own_buf + sizeof ("QCatchSyscalls:") - 1;
      int enabled = -1;
      CORE_ADDR sysno;
      struct process_info *process;

      if (!target_running () || !target_supports_catch_syscall ())
	{
	  write_enn (own_buf);
	  return;
	}

      if (strcmp (p, "0") == 0)
	enabled = 0;
      else if (p[0] == '1' && (p[1] == ';' || p[1] == '\0'))
	enabled = 1;
      else
	{
	  fprintf (stderr, "Unknown catch-syscalls mode requested: %s\n",
		   own_buf);
	  write_enn (own_buf);
	  return;
	}

      process = current_process ();
      process->syscalls_to_catch.clear ();

      if (enabled)
	{
	  p += 1;
	  if (*p == ';')
	    {
	      p += 1;
	      while (*p != '\0')
		{
		  p = decode_address_to_semicolon (&sysno, p);
		  process->syscalls_to_catch.push_back (sysno);
		}
	    }
	  else
	    process->syscalls_to_catch.push_back (ANY_SYSCALL);
	}

      write_ok (own_buf);
      return;
    }

  if (strcmp (own_buf, "QEnvironmentReset") == 0)
    {
      our_environ = gdb_environ::from_host_environ ();

      write_ok (own_buf);
      return;
    }

  if (startswith (own_buf, "QEnvironmentHexEncoded:"))
    {
      const char *p = own_buf + sizeof ("QEnvironmentHexEncoded:") - 1;
      /* The final form of the environment variable.  FINAL_VAR will
	 hold the 'VAR=VALUE' format.  */
      std::string final_var = hex2str (p);
      std::string var_name, var_value;

      remote_debug_printf ("[QEnvironmentHexEncoded received '%s']", p);
      remote_debug_printf ("[Environment variable to be set: '%s']",
			   final_var.c_str ());

      size_t pos = final_var.find ('=');
      if (pos == std::string::npos)
	{
	  warning (_("Unexpected format for environment variable: '%s'"),
		   final_var.c_str ());
	  write_enn (own_buf);
	  return;
	}

      var_name = final_var.substr (0, pos);
      var_value = final_var.substr (pos + 1, std::string::npos);

      our_environ.set (var_name.c_str (), var_value.c_str ());

      write_ok (own_buf);
      return;
    }

  if (startswith (own_buf, "QEnvironmentUnset:"))
    {
      const char *p = own_buf + sizeof ("QEnvironmentUnset:") - 1;
      std::string varname = hex2str (p);

      remote_debug_printf ("[QEnvironmentUnset received '%s']", p);
      remote_debug_printf ("[Environment variable to be unset: '%s']",
			   varname.c_str ());

      our_environ.unset (varname.c_str ());

      write_ok (own_buf);
      return;
    }

  if (strcmp (own_buf, "QStartNoAckMode") == 0)
    {
      remote_debug_printf ("[noack mode enabled]");

      cs.noack_mode = 1;
      write_ok (own_buf);
      return;
    }

  if (startswith (own_buf, "QNonStop:"))
    {
      char *mode = own_buf + 9;
      int req = -1;
      const char *req_str;

      if (strcmp (mode, "0") == 0)
	req = 0;
      else if (strcmp (mode, "1") == 0)
	req = 1;
      else
	{
	  /* We don't know what this mode is, so complain to
	     GDB.  */
	  fprintf (stderr, "Unknown non-stop mode requested: %s\n",
		   own_buf);
	  write_enn (own_buf);
	  return;
	}

      req_str = req ? "non-stop" : "all-stop";
      if (the_target->start_non_stop (req == 1) != 0)
	{
	  fprintf (stderr, "Setting %s mode failed\n", req_str);
	  write_enn (own_buf);
	  return;
	}

      non_stop = (req != 0);

      remote_debug_printf ("[%s mode enabled]", req_str);

      write_ok (own_buf);
      return;
    }

  if (startswith (own_buf, "QDisableRandomization:"))
    {
      char *packet = own_buf + strlen ("QDisableRandomization:");
      ULONGEST setting;

      unpack_varlen_hex (packet, &setting);
      cs.disable_randomization = setting;

      remote_debug_printf (cs.disable_randomization
			   ? "[address space randomization disabled]"
			       : "[address space randomization enabled]");

      write_ok (own_buf);
      return;
    }

  if (target_supports_tracepoints ()
      && handle_tracepoint_general_set (own_buf))
    return;

  if (startswith (own_buf, "QAgent:"))
    {
      char *mode = own_buf + strlen ("QAgent:");
      int req = 0;

      if (strcmp (mode, "0") == 0)
	req = 0;
      else if (strcmp (mode, "1") == 0)
	req = 1;
      else
	{
	  /* We don't know what this value is, so complain to GDB.  */
	  sprintf (own_buf, "E.Unknown QAgent value");
	  return;
	}

      /* Update the flag.  */
      use_agent = req;
      remote_debug_printf ("[%s agent]", req ? "Enable" : "Disable");
      write_ok (own_buf);
      return;
    }

  if (handle_btrace_general_set (own_buf))
    return;

  if (handle_btrace_conf_general_set (own_buf))
    return;

  if (startswith (own_buf, "QThreadEvents:"))
    {
      char *mode = own_buf + strlen ("QThreadEvents:");
      enum tribool req = TRIBOOL_UNKNOWN;

      if (strcmp (mode, "0") == 0)
	req = TRIBOOL_FALSE;
      else if (strcmp (mode, "1") == 0)
	req = TRIBOOL_TRUE;
      else
	{
	  /* We don't know what this mode is, so complain to GDB.  */
	  std::string err
	    = string_printf ("E.Unknown thread-events mode requested: %s\n",
			     mode);
	  strcpy (own_buf, err.c_str ());
	  return;
	}

      cs.report_thread_events = (req == TRIBOOL_TRUE);

      remote_debug_printf ("[thread events are now %s]\n",
			   cs.report_thread_events ? "enabled" : "disabled");

      write_ok (own_buf);
      return;
    }

  if (startswith (own_buf, "QThreadOptions;"))
    {
      const char *p = own_buf + strlen ("QThreadOptions");

      gdb_thread_options supported_options = target_supported_thread_options ();
      if (supported_options == 0)
	{
	  /* Something went wrong -- we don't support any option, but
	     GDB sent the packet anyway.  */
	  write_enn (own_buf);
	  return;
	}

      /* We could store the options directly in thread->thread_options
	 without this map, but that would mean that a QThreadOptions
	 packet with a wildcard like "QThreadOptions;0;3:TID" would
	 result in the debug logs showing:

	   [options for TID are now 0x0]
	   [options for TID are now 0x3]

	 It's nicer if we only print the final options for each TID,
	 and if we only print about it if the options changed compared
	 to the options that were previously set on the thread.  */
      std::unordered_map<thread_info *, gdb_thread_options> set_options;

      while (*p != '\0')
	{
	  if (p[0] != ';')
	    {
	      write_enn (own_buf);
	      return;
	    }
	  p++;

	  /* Read the options.  */

	  gdb_thread_options options = parse_gdb_thread_options (&p);

	  if ((options & ~supported_options) != 0)
	    {
	      /* GDB asked for an unknown or unsupported option, so
		 error out.  */
	      std::string err
		= string_printf ("E.Unknown thread options requested: %s\n",
				 to_string (options).c_str ());
	      strcpy (own_buf, err.c_str ());
	      return;
	    }

	  ptid_t ptid;

	  if (p[0] == ';' || p[0] == '\0')
	    ptid = minus_one_ptid;
	  else if (p[0] == ':')
	    {
	      const char *q;

	      ptid = read_ptid (p + 1, &q);

	      if (p == q)
		{
		  write_enn (own_buf);
		  return;
		}
	      p = q;
	      if (p[0] != ';' && p[0] != '\0')
		{
		  write_enn (own_buf);
		  return;
		}
	    }
	  else
	    {
	      write_enn (own_buf);
	      return;
	    }

	  /* Convert PID.-1 => PID.0 for ptid.matches.  */
	  if (ptid.lwp () == -1)
	    ptid = ptid_t (ptid.pid ());

	  for_each_thread ([&] (thread_info *thread)
	    {
	      if (ptid_of (thread).matches (ptid))
		set_options[thread] = options;
	    });
	}

      for (const auto &iter : set_options)
	{
	  thread_info *thread = iter.first;
	  gdb_thread_options options = iter.second;

	  if (thread->thread_options != options)
	    {
	      threads_debug_printf ("[options for %s are now %s]\n",
				    target_pid_to_str (ptid_of (thread)).c_str (),
				    to_string (options).c_str ());

	      thread->thread_options = options;
	    }
	}

      write_ok (own_buf);
      return;
    }

  if (startswith (own_buf, "QStartupWithShell:"))
    {
      const char *value = own_buf + strlen ("QStartupWithShell:");

      if (strcmp (value, "1") == 0)
	startup_with_shell = true;
      else if (strcmp (value, "0") == 0)
	startup_with_shell = false;
      else
	{
	  /* Unknown value.  */
	  fprintf (stderr, "Unknown value to startup-with-shell: %s\n",
		   own_buf);
	  write_enn (own_buf);
	  return;
	}

      remote_debug_printf ("[Inferior will %s started with shell]",
			   startup_with_shell ? "be" : "not be");

      write_ok (own_buf);
      return;
    }

  if (startswith (own_buf, "QSetWorkingDir:"))
    {
      const char *p = own_buf + strlen ("QSetWorkingDir:");

      if (*p != '\0')
	{
	  std::string path = hex2str (p);

	  remote_debug_printf ("[Set the inferior's current directory to %s]",
			       path.c_str ());

	  set_inferior_cwd (std::move (path));
	}
      else
	{
	  /* An empty argument means that we should clear out any
	     previously set cwd for the inferior.  */
	  set_inferior_cwd ("");

	  remote_debug_printf ("[Unset the inferior's current directory; will "
			       "use gdbserver's cwd]");
	}
      write_ok (own_buf);

      return;
    }


  /* Handle store memory tags packets.  */
  if (startswith (own_buf, "QMemTags:")
      && target_supports_memory_tagging ())
    {
      gdb::byte_vector tags;
      CORE_ADDR addr = 0;
      size_t len = 0;
      int type = 0;

      require_running_or_return (own_buf);

      bool ret = parse_store_memtags_request (own_buf, &addr, &len, tags,
					     &type);

      if (ret)
	ret = the_target->store_memtags (addr, len, tags, type);

      if (!ret)
	write_enn (own_buf);
      else
	write_ok (own_buf);

      return;
    }

  /* Otherwise we didn't know what packet it was.  Say we didn't
     understand it.  */
  own_buf[0] = 0;
}

static const char *
get_features_xml (const char *annex)
{
  const struct target_desc *desc = current_target_desc ();

  /* `desc->xmltarget' defines what to return when looking for the
     "target.xml" file.  Its contents can either be verbatim XML code
     (prefixed with a '@') or else the name of the actual XML file to
     be used in place of "target.xml".

     This variable is set up from the auto-generated
     init_registers_... routine for the current target.  */

  if (strcmp (annex, "target.xml") == 0)
    {
      const char *ret = tdesc_get_features_xml (desc);

      if (*ret == '@')
	return ret + 1;
      else
	annex = ret;
    }

#ifdef USE_XML
  {
    int i;

    /* Look for the annex.  */
    for (i = 0; xml_builtin[i][0] != NULL; i++)
      if (strcmp (annex, xml_builtin[i][0]) == 0)
	break;

    if (xml_builtin[i][0] != NULL)
      return xml_builtin[i][1];
  }
#endif

  return NULL;
}

static void
monitor_show_help (void)
{
  monitor_output ("The following monitor commands are supported:\n");
  monitor_output ("  set debug on\n");
  monitor_output ("    Enable general debugging messages\n");
  monitor_output ("  set debug off\n");
  monitor_output ("    Disable all debugging messages\n");
  monitor_output ("  set debug COMPONENT <off|on>\n");
  monitor_output ("    Enable debugging messages for COMPONENT, which is\n");
  monitor_output ("    one of: all, threads, remote, event-loop.\n");
  monitor_output ("  set debug-hw-points <0|1>\n");
  monitor_output ("    Enable h/w breakpoint/watchpoint debugging messages\n");
  monitor_output ("  set debug-format option1[,option2,...]\n");
  monitor_output ("    Add additional information to debugging messages\n");
  monitor_output ("    Options: all, none, timestamp\n");
  monitor_output ("  exit\n");
  monitor_output ("    Quit GDBserver\n");
}

/* Read trace frame or inferior memory.  Returns the number of bytes
   actually read, zero when no further transfer is possible, and -1 on
   error.  Return of a positive value smaller than LEN does not
   indicate there's no more to be read, only the end of the transfer.
   E.g., when GDB reads memory from a traceframe, a first request may
   be served from a memory block that does not cover the whole request
   length.  A following request gets the rest served from either
   another block (of the same traceframe) or from the read-only
   regions.  */

static int
gdb_read_memory (CORE_ADDR memaddr, unsigned char *myaddr, int len)
{
  client_state &cs = get_client_state ();
  int res;

  if (cs.current_traceframe >= 0)
    {
      ULONGEST nbytes;
      ULONGEST length = len;

      if (traceframe_read_mem (cs.current_traceframe,
			       memaddr, myaddr, len, &nbytes))
	return -1;
      /* Data read from trace buffer, we're done.  */
      if (nbytes > 0)
	return nbytes;
      if (!in_readonly_region (memaddr, length))
	return -1;
      /* Otherwise we have a valid readonly case, fall through.  */
      /* (assume no half-trace half-real blocks for now) */
    }

  if (set_desired_process ())
    res = read_inferior_memory (memaddr, myaddr, len);
  else
    res = 1;

  return res == 0 ? len : -1;
}

/* Write trace frame or inferior memory.  Actually, writing to trace
   frames is forbidden.  */

static int
gdb_write_memory (CORE_ADDR memaddr, const unsigned char *myaddr, int len)
{
  client_state &cs = get_client_state ();
  if (cs.current_traceframe >= 0)
    return EIO;
  else
    {
      int ret;

      if (set_desired_process ())
	ret = target_write_memory (memaddr, myaddr, len);
      else
	ret = EIO;
      return ret;
    }
}

/* Handle qSearch:memory packets.  */

static void
handle_search_memory (char *own_buf, int packet_len)
{
  CORE_ADDR start_addr;
  CORE_ADDR search_space_len;
  gdb_byte *pattern;
  unsigned int pattern_len;
  int found;
  CORE_ADDR found_addr;
  int cmd_name_len = sizeof ("qSearch:memory:") - 1;

  pattern = (gdb_byte *) malloc (packet_len);
  if (pattern == NULL)
    error ("Unable to allocate memory to perform the search");

  if (decode_search_memory_packet (own_buf + cmd_name_len,
				   packet_len - cmd_name_len,
				   &start_addr, &search_space_len,
				   pattern, &pattern_len) < 0)
    {
      free (pattern);
      error ("Error in parsing qSearch:memory packet");
    }

  auto read_memory = [] (CORE_ADDR addr, gdb_byte *result, size_t len)
    {
      return gdb_read_memory (addr, result, len) == len;
    };

  found = simple_search_memory (read_memory, start_addr, search_space_len,
				pattern, pattern_len, &found_addr);

  if (found > 0)
    sprintf (own_buf, "1,%lx", (long) found_addr);
  else if (found == 0)
    strcpy (own_buf, "0");
  else
    strcpy (own_buf, "E00");

  free (pattern);
}

/* Handle the "D" packet.  */

static void
handle_detach (char *own_buf)
{
  client_state &cs = get_client_state ();

  process_info *process;

  if (cs.multi_process)
    {
      /* skip 'D;' */
      int pid = strtol (&own_buf[2], NULL, 16);

      process = find_process_pid (pid);
    }
  else
    {
      process = (current_thread != nullptr
		 ? get_thread_process (current_thread)
		 : nullptr);
    }

  if (process == NULL)
    {
      write_enn (own_buf);
      return;
    }

  if ((tracing && disconnected_tracing) || any_persistent_commands (process))
    {
      if (tracing && disconnected_tracing)
	fprintf (stderr,
		 "Disconnected tracing in effect, "
		 "leaving gdbserver attached to the process\n");

      if (any_persistent_commands (process))
	fprintf (stderr,
		 "Persistent commands are present, "
		 "leaving gdbserver attached to the process\n");

      /* Make sure we're in non-stop/async mode, so we we can both
	 wait for an async socket accept, and handle async target
	 events simultaneously.  There's also no point either in
	 having the target stop all threads, when we're going to
	 pass signals down without informing GDB.  */
      if (!non_stop)
	{
	  threads_debug_printf ("Forcing non-stop mode");

	  non_stop = true;
	  the_target->start_non_stop (true);
	}

      process->gdb_detached = 1;

      /* Detaching implicitly resumes all threads.  */
      target_continue_no_signal (minus_one_ptid);

      write_ok (own_buf);
      return;
    }

  fprintf (stderr, "Detaching from process %d\n", process->pid);
  stop_tracing ();

  /* We'll need this after PROCESS has been destroyed.  */
  int pid = process->pid;

  /* If this process has an unreported fork child, that child is not known to
     GDB, so GDB won't take care of detaching it.  We must do it here.

     Here, we specifically don't want to use "safe iteration", as detaching
     another process might delete the next thread in the iteration, which is
     the one saved by the safe iterator.  We will never delete the currently
     iterated on thread, so standard iteration should be safe.  */
  for (thread_info *thread : all_threads)
    {
      /* Only threads that are of the process we are detaching.  */
      if (thread->id.pid () != pid)
	continue;

      /* Only threads that have a pending fork event.  */
      target_waitkind kind;
      thread_info *child = target_thread_pending_child (thread, &kind);
      if (child == nullptr || kind == TARGET_WAITKIND_THREAD_CLONED)
	continue;

      process_info *fork_child_process = get_thread_process (child);
      gdb_assert (fork_child_process != nullptr);

      int fork_child_pid = fork_child_process->pid;

      if (detach_inferior (fork_child_process) != 0)
	warning (_("Failed to detach fork child %s, child of %s"),
		 target_pid_to_str (ptid_t (fork_child_pid)).c_str (),
		 target_pid_to_str (thread->id).c_str ());
    }

  if (detach_inferior (process) != 0)
    write_enn (own_buf);
  else
    {
      discard_queued_stop_replies (ptid_t (pid));
      write_ok (own_buf);

      if (extended_protocol || target_running ())
	{
	  /* There is still at least one inferior remaining or
	     we are in extended mode, so don't terminate gdbserver,
	     and instead treat this like a normal program exit.  */
	  cs.last_status.set_exited (0);
	  cs.last_ptid = ptid_t (pid);

	  switch_to_thread (nullptr);
	}
      else
	{
	  putpkt (own_buf);
	  remote_close ();

	  /* If we are attached, then we can exit.  Otherwise, we
	     need to hang around doing nothing, until the child is
	     gone.  */
	  join_inferior (pid);
	  exit (0);
	}
    }
}

/* Parse options to --debug-format= and "monitor set debug-format".
   ARG is the text after "--debug-format=" or "monitor set debug-format".
   IS_MONITOR is non-zero if we're invoked via "monitor set debug-format".
   This triggers calls to monitor_output.
   The result is an empty string if all options were parsed ok, otherwise an
   error message which the caller must free.

   N.B. These commands affect all debug format settings, they are not
   cumulative.  If a format is not specified, it is turned off.
   However, we don't go to extra trouble with things like
   "monitor set debug-format all,none,timestamp".
   Instead we just parse them one at a time, in order.

   The syntax for "monitor set debug" we support here is not identical
   to gdb's "set debug foo on|off" because we also use this function to
   parse "--debug-format=foo,bar".  */

static std::string
parse_debug_format_options (const char *arg, int is_monitor)
{
  /* First turn all debug format options off.  */
  debug_timestamp = 0;

  /* First remove leading spaces, for "monitor set debug-format".  */
  while (isspace (*arg))
    ++arg;

  std::vector<gdb::unique_xmalloc_ptr<char>> options
    = delim_string_to_char_ptr_vec (arg, ',');

  for (const gdb::unique_xmalloc_ptr<char> &option : options)
    {
      if (strcmp (option.get (), "all") == 0)
	{
	  debug_timestamp = 1;
	  if (is_monitor)
	    monitor_output ("All extra debug format options enabled.\n");
	}
      else if (strcmp (option.get (), "none") == 0)
	{
	  debug_timestamp = 0;
	  if (is_monitor)
	    monitor_output ("All extra debug format options disabled.\n");
	}
      else if (strcmp (option.get (), "timestamp") == 0)
	{
	  debug_timestamp = 1;
	  if (is_monitor)
	    monitor_output ("Timestamps will be added to debug output.\n");
	}
      else if (*option == '\0')
	{
	  /* An empty option, e.g., "--debug-format=foo,,bar", is ignored.  */
	  continue;
	}
      else
	return string_printf ("Unknown debug-format argument: \"%s\"\n",
			      option.get ());
    }

  return std::string ();
}

/* A wrapper to enable, or disable a debug flag.  These are debug flags
   that control the debug output from gdbserver, that developers might
   want, this is not something most end users will need.  */

struct debug_opt
{
  /* NAME is the name of this debug option, this should be a simple string
     containing no whitespace, starting with a letter from isalpha(), and
     contain only isalnum() characters and '_' underscore and '-' hyphen.

     SETTER is a callback function used to set the debug variable.  This
     callback will be passed true to enable the debug setting, or false to
     disable the debug setting.  */
  debug_opt (const char *name, std::function<void (bool)> setter)
    : m_name (name),
      m_setter (setter)
  {
    gdb_assert (isalpha (*name));
  }

  /* Called to enable or disable the debug setting.  */
  void set (bool enable) const
  {
    m_setter (enable);
  }

  /* Return the name of this debug option.  */
  const char *name () const
  { return m_name; }

private:
  /* The name of this debug option.  */
  const char *m_name;

  /* The callback to update the debug setting.  */
  std::function<void (bool)> m_setter;
};

/* The set of all debug options that gdbserver supports.  These are the
   options that can be passed to the command line '--debug=...' flag, or to
   the monitor command 'monitor set debug ...'.  */

static std::vector<debug_opt> all_debug_opt {
  {"threads", [] (bool enable)
  {
    debug_threads = enable;
  }},
  {"remote", [] (bool enable)
  {
    remote_debug = enable;
  }},
  {"event-loop", [] (bool enable)
  {
    debug_event_loop = (enable ? debug_event_loop_kind::ALL
			: debug_event_loop_kind::OFF);
  }}
};

/* Parse the options to --debug=...

   OPTIONS is the string of debug components which should be enabled (or
   disabled), and must not be nullptr.  An empty OPTIONS string is valid,
   in which case a default set of debug components will be enabled.

   An unknown, or otherwise invalid debug component will result in an
   exception being thrown.

   OPTIONS can consist of multiple debug component names separated by a
   comma.  Debugging for each component will be turned on.  The special
   component 'all' can be used to enable debugging for all components.

   A component can also be prefixed with '-' to disable debugging of that
   component, so a user might use: '--debug=all,-remote', to enable all
   debugging, except for the remote (protocol) component.  Components are
   processed left to write in the OPTIONS list.  */

static void
parse_debug_options (const char *options)
{
  gdb_assert (options != nullptr);

  /* Empty options means the "default" set.  This exists mostly for
     backwards compatibility with gdbserver's legacy behaviour.  */
  if (*options == '\0')
    options = "+threads";

  while (*options != '\0')
    {
      const char *end = strchrnul (options, ',');

      bool enable = *options != '-';
      if (*options == '-' || *options == '+')
	++options;

      std::string opt (options, end - options);

      if (opt.size () == 0)
	error ("invalid empty debug option");

      bool is_opt_all = opt == "all";

      bool found = false;
      for (const auto &debug_opt : all_debug_opt)
	if (is_opt_all || opt == debug_opt.name ())
	  {
	    debug_opt.set (enable);
	    found = true;
	    if (!is_opt_all)
	      break;
	  }

      if (!found)
	error ("unknown debug option '%s'", opt.c_str ());

      options = (*end == ',') ? end + 1 : end;
    }
}

/* Called from the 'monitor' command handler, to handle general 'set debug'
   monitor commands with one of the formats:

     set debug COMPONENT VALUE
     set debug VALUE

   In both of these command formats VALUE can be 'on', 'off', '1', or '0'
   with 1/0 being equivalent to on/off respectively.

   In the no-COMPONENT version of the command, if VALUE is 'on' (or '1')
   then the component 'threads' is assumed, this is for backward
   compatibility, but maybe in the future we might find a better "default"
   set of debug flags to enable.

   In the no-COMPONENT version of the command, if VALUE is 'off' (or '0')
   then all debugging is turned off.

   Otherwise, COMPONENT must be one of the known debug components, and that
   component is either enabled or disabled as appropriate.

   The string MON contains either 'COMPONENT VALUE' or just the 'VALUE' for
   the second command format, the 'set debug ' has been stripped off
   already.

   Return a string containing an error message if something goes wrong,
   this error can be returned as part of the monitor command output.  If
   everything goes correctly then the debug global will have been updated,
   and an empty string is returned.  */

static std::string
handle_general_monitor_debug (const char *mon)
{
  mon = skip_spaces (mon);

  if (*mon == '\0')
    return "No debug component name found.\n";

  /* Find the first word within MON.  This is either the component name,
     or the value if no component has been given.  */
  const char *end = skip_to_space (mon);
  std::string component (mon, end - mon);
  if (component.find (',') != component.npos || component[0] == '-'
      || component[0] == '+')
    return "Invalid character found in debug component name.\n";

  /* In ACTION_STR we create a string that will be passed to the
     parse_debug_options string.  This will be either '+COMPONENT' or
     '-COMPONENT' depending on whether we want to enable or disable
     COMPONENT.  */
  std::string action_str;

  /* If parse_debug_options succeeds, then MSG will be returned to the user
     as the output of the monitor command.  */
  std::string msg;

  /* Check for 'set debug off', this disables all debug output.  */
  if (component == "0" || component == "off")
    {
      if (*skip_spaces (end) != '\0')
	return string_printf
	  ("Junk '%s' found at end of 'set debug %s' command.\n",
	   skip_spaces (end), std::string (mon, end - mon).c_str ());

      action_str = "-all";
      msg = "All debug output disabled.\n";
    }
  /* Check for 'set debug on', this disables a general set of debug.  */
  else if (component == "1" || component == "on")
    {
      if (*skip_spaces (end) != '\0')
	return string_printf
	  ("Junk '%s' found at end of 'set debug %s' command.\n",
	   skip_spaces (end), std::string (mon, end - mon).c_str ());

      action_str = "+threads";
      msg = "General debug output enabled.\n";
    }
  /* Otherwise we should have 'set debug COMPONENT VALUE'.  Extract the two
     parts and validate.  */
  else
    {
      /* Figure out the value the user passed.  */
      const char *value_start = skip_spaces (end);
      if (*value_start == '\0')
	return string_printf ("Missing value for 'set debug %s' command.\n",
			      mon);

      const char *after_value = skip_to_space (value_start);
      if (*skip_spaces (after_value) != '\0')
	return string_printf
	  ("Junk '%s' found at end of 'set debug %s' command.\n",
	   skip_spaces (after_value),
	   std::string (mon, after_value - mon).c_str ());

      std::string value (value_start, after_value - value_start);

      /* Check VALUE to see if we are enabling, or disabling.  */
      bool enable;
      if (value == "0" || value == "off")
	enable = false;
      else if (value == "1" || value == "on")
	enable = true;
      else
	return string_printf ("Invalid value '%s' for 'set debug %s'.\n",
			      value.c_str (),
			      std::string (mon, end - mon).c_str ());

      action_str = std::string (enable ? "+" : "-") + component;
      msg = string_printf ("Debug output for '%s' %s.\n", component.c_str (),
			   enable ? "enabled" : "disabled");
    }

  gdb_assert (!msg.empty ());
  gdb_assert (!action_str.empty ());

  try
    {
      parse_debug_options (action_str.c_str ());
      monitor_output (msg.c_str ());
    }
  catch (const gdb_exception_error &exception)
    {
      return string_printf ("Error: %s\n", exception.what ());
    }

  return {};
}

/* Handle monitor commands not handled by target-specific handlers.  */

static void
handle_monitor_command (char *mon, char *own_buf)
{
  if (startswith (mon, "set debug "))
    {
      std::string error_msg
	= handle_general_monitor_debug (mon + sizeof ("set debug ") - 1);

      if (!error_msg.empty ())
	{
	  monitor_output (error_msg.c_str ());
	  monitor_show_help ();
	  write_enn (own_buf);
	}
    }
  else if (strcmp (mon, "set debug-hw-points 1") == 0)
    {
      show_debug_regs = 1;
      monitor_output ("H/W point debugging output enabled.\n");
    }
  else if (strcmp (mon, "set debug-hw-points 0") == 0)
    {
      show_debug_regs = 0;
      monitor_output ("H/W point debugging output disabled.\n");
    }
  else if (startswith (mon, "set debug-format "))
    {
      std::string error_msg
	= parse_debug_format_options (mon + sizeof ("set debug-format ") - 1,
				      1);

      if (!error_msg.empty ())
	{
	  monitor_output (error_msg.c_str ());
	  monitor_show_help ();
	  write_enn (own_buf);
	}
    }
  else if (strcmp (mon, "set debug-file") == 0)
    debug_set_output (nullptr);
  else if (startswith (mon, "set debug-file "))
    debug_set_output (mon + sizeof ("set debug-file ") - 1);
  else if (strcmp (mon, "help") == 0)
    monitor_show_help ();
  else if (strcmp (mon, "exit") == 0)
    exit_requested = true;
  else
    {
      monitor_output ("Unknown monitor command.\n\n");
      monitor_show_help ();
      write_enn (own_buf);
    }
}

/* Associates a callback with each supported qXfer'able object.  */

struct qxfer
{
  /* The object this handler handles.  */
  const char *object;

  /* Request that the target transfer up to LEN 8-bit bytes of the
     target's OBJECT.  The OFFSET, for a seekable object, specifies
     the starting point.  The ANNEX can be used to provide additional
     data-specific information to the target.

     Return the number of bytes actually transfered, zero when no
     further transfer is possible, -1 on error, -2 when the transfer
     is not supported, and -3 on a verbose error message that should
     be preserved.  Return of a positive value smaller than LEN does
     not indicate the end of the object, only the end of the transfer.

     One, and only one, of readbuf or writebuf must be non-NULL.  */
  int (*xfer) (const char *annex,
	       gdb_byte *readbuf, const gdb_byte *writebuf,
	       ULONGEST offset, LONGEST len);
};

/* Handle qXfer:auxv:read.  */

static int
handle_qxfer_auxv (const char *annex,
		   gdb_byte *readbuf, const gdb_byte *writebuf,
		   ULONGEST offset, LONGEST len)
{
  if (!the_target->supports_read_auxv () || writebuf != NULL)
    return -2;

  if (annex[0] != '\0' || current_thread == NULL)
    return -1;

  return the_target->read_auxv (current_thread->id.pid (), offset, readbuf,
				len);
}

/* Handle qXfer:exec-file:read.  */

static int
handle_qxfer_exec_file (const char *annex,
			gdb_byte *readbuf, const gdb_byte *writebuf,
			ULONGEST offset, LONGEST len)
{
  ULONGEST pid;
  int total_len;

  if (!the_target->supports_pid_to_exec_file () || writebuf != NULL)
    return -2;

  if (annex[0] == '\0')
    {
      if (current_thread == NULL)
	return -1;

      pid = pid_of (current_thread);
    }
  else
    {
      annex = unpack_varlen_hex (annex, &pid);
      if (annex[0] != '\0')
	return -1;
    }

  if (pid <= 0)
    return -1;

  const char *file = the_target->pid_to_exec_file (pid);
  if (file == NULL)
    return -1;

  total_len = strlen (file);

  if (offset > total_len)
    return -1;

  if (offset + len > total_len)
    len = total_len - offset;

  memcpy (readbuf, file + offset, len);
  return len;
}

/* Handle qXfer:features:read.  */

static int
handle_qxfer_features (const char *annex,
		       gdb_byte *readbuf, const gdb_byte *writebuf,
		       ULONGEST offset, LONGEST len)
{
  const char *document;
  size_t total_len;

  if (writebuf != NULL)
    return -2;

  if (!target_running ())
    return -1;

  /* Grab the correct annex.  */
  document = get_features_xml (annex);
  if (document == NULL)
    return -1;

  total_len = strlen (document);

  if (offset > total_len)
    return -1;

  if (offset + len > total_len)
    len = total_len - offset;

  memcpy (readbuf, document + offset, len);
  return len;
}

/* Handle qXfer:libraries:read.  */

static int
handle_qxfer_libraries (const char *annex,
			gdb_byte *readbuf, const gdb_byte *writebuf,
			ULONGEST offset, LONGEST len)
{
  if (writebuf != NULL)
    return -2;

  if (annex[0] != '\0' || current_thread == NULL)
    return -1;

  std::string document = "<library-list version=\"1.0\">\n";

  process_info *proc = current_process ();
  for (const dll_info &dll : proc->all_dlls)
    document += string_printf
      ("  <library name=\"%s\"><segment address=\"0x%s\"/></library>\n",
       dll.name.c_str (), paddress (dll.base_addr));

  document += "</library-list>\n";

  if (offset > document.length ())
    return -1;

  if (offset + len > document.length ())
    len = document.length () - offset;

  memcpy (readbuf, &document[offset], len);

  return len;
}

/* Handle qXfer:libraries-svr4:read.  */

static int
handle_qxfer_libraries_svr4 (const char *annex,
			     gdb_byte *readbuf, const gdb_byte *writebuf,
			     ULONGEST offset, LONGEST len)
{
  if (writebuf != NULL)
    return -2;

  if (current_thread == NULL
      || !the_target->supports_qxfer_libraries_svr4 ())
    return -1;

  return the_target->qxfer_libraries_svr4 (annex, readbuf, writebuf,
					   offset, len);
}

/* Handle qXfer:osadata:read.  */

static int
handle_qxfer_osdata (const char *annex,
		     gdb_byte *readbuf, const gdb_byte *writebuf,
		     ULONGEST offset, LONGEST len)
{
  if (!the_target->supports_qxfer_osdata () || writebuf != NULL)
    return -2;

  return the_target->qxfer_osdata (annex, readbuf, NULL, offset, len);
}

/* Handle qXfer:siginfo:read and qXfer:siginfo:write.  */

static int
handle_qxfer_siginfo (const char *annex,
		      gdb_byte *readbuf, const gdb_byte *writebuf,
		      ULONGEST offset, LONGEST len)
{
  if (!the_target->supports_qxfer_siginfo ())
    return -2;

  if (annex[0] != '\0' || current_thread == NULL)
    return -1;

  return the_target->qxfer_siginfo (annex, readbuf, writebuf, offset, len);
}

/* Handle qXfer:statictrace:read.  */

static int
handle_qxfer_statictrace (const char *annex,
			  gdb_byte *readbuf, const gdb_byte *writebuf,
			  ULONGEST offset, LONGEST len)
{
  client_state &cs = get_client_state ();
  ULONGEST nbytes;

  if (writebuf != NULL)
    return -2;

  if (annex[0] != '\0' || current_thread == NULL 
      || cs.current_traceframe == -1)
    return -1;

  if (traceframe_read_sdata (cs.current_traceframe, offset,
			     readbuf, len, &nbytes))
    return -1;
  return nbytes;
}

/* Helper for handle_qxfer_threads_proper.
   Emit the XML to describe the thread of INF.  */

static void
handle_qxfer_threads_worker (thread_info *thread, std::string *buffer)
{
  ptid_t ptid = ptid_of (thread);
  char ptid_s[100];
  int core = target_core_of_thread (ptid);
  char core_s[21];
  const char *name = target_thread_name (ptid);
  int handle_len;
  gdb_byte *handle;
  bool handle_status = target_thread_handle (ptid, &handle, &handle_len);

  /* If this is a (v)fork/clone child (has a (v)fork/clone parent),
     GDB does not yet know about this thread, and must not know about
     it until it gets the corresponding (v)fork/clone event.  Exclude
     this thread from the list.  */
  if (target_thread_pending_parent (thread) != nullptr)
    return;

  write_ptid (ptid_s, ptid);

  string_xml_appendf (*buffer, "<thread id=\"%s\"", ptid_s);

  if (core != -1)
    {
      sprintf (core_s, "%d", core);
      string_xml_appendf (*buffer, " core=\"%s\"", core_s);
    }

  if (name != NULL)
    string_xml_appendf (*buffer, " name=\"%s\"", name);

  if (handle_status)
    {
      char *handle_s = (char *) alloca (handle_len * 2 + 1);
      bin2hex (handle, handle_s, handle_len);
      string_xml_appendf (*buffer, " handle=\"%s\"", handle_s);
    }

  string_xml_appendf (*buffer, "/>\n");
}

/* Helper for handle_qxfer_threads.  Return true on success, false
   otherwise.  */

static bool
handle_qxfer_threads_proper (std::string *buffer)
{
  *buffer += "<threads>\n";

  /* The target may need to access memory and registers (e.g. via
     libthread_db) to fetch thread properties.  Even if don't need to
     stop threads to access memory, we still will need to be able to
     access registers, and other ptrace accesses like
     PTRACE_GET_THREAD_AREA that require a paused thread.  Pause all
     threads here, so that we pause each thread at most once for all
     accesses.  */
  if (non_stop)
    target_pause_all (true);

  for_each_thread ([&] (thread_info *thread)
    {
      handle_qxfer_threads_worker (thread, buffer);
    });

  if (non_stop)
    target_unpause_all (true);

  *buffer += "</threads>\n";
  return true;
}

/* Handle qXfer:threads:read.  */

static int
handle_qxfer_threads (const char *annex,
		      gdb_byte *readbuf, const gdb_byte *writebuf,
		      ULONGEST offset, LONGEST len)
{
  static std::string result;

  if (writebuf != NULL)
    return -2;

  if (annex[0] != '\0')
    return -1;

  if (offset == 0)
    {
      /* When asked for data at offset 0, generate everything and store into
	 'result'.  Successive reads will be served off 'result'.  */
      result.clear ();

      bool res = handle_qxfer_threads_proper (&result);

      if (!res)
	return -1;
    }

  if (offset >= result.length ())
    {
      /* We're out of data.  */
      result.clear ();
      return 0;
    }

  if (len > result.length () - offset)
    len = result.length () - offset;

  memcpy (readbuf, result.c_str () + offset, len);

  return len;
}

/* Handle qXfer:traceframe-info:read.  */

static int
handle_qxfer_traceframe_info (const char *annex,
			      gdb_byte *readbuf, const gdb_byte *writebuf,
			      ULONGEST offset, LONGEST len)
{
  client_state &cs = get_client_state ();
  static std::string result;

  if (writebuf != NULL)
    return -2;

  if (!target_running () || annex[0] != '\0' || cs.current_traceframe == -1)
    return -1;

  if (offset == 0)
    {
      /* When asked for data at offset 0, generate everything and
	 store into 'result'.  Successive reads will be served off
	 'result'.  */
      result.clear ();

      traceframe_read_info (cs.current_traceframe, &result);
    }

  if (offset >= result.length ())
    {
      /* We're out of data.  */
      result.clear ();
      return 0;
    }

  if (len > result.length () - offset)
    len = result.length () - offset;

  memcpy (readbuf, result.c_str () + offset, len);
  return len;
}

/* Handle qXfer:fdpic:read.  */

static int
handle_qxfer_fdpic (const char *annex, gdb_byte *readbuf,
		    const gdb_byte *writebuf, ULONGEST offset, LONGEST len)
{
  if (!the_target->supports_read_loadmap ())
    return -2;

  if (current_thread == NULL)
    return -1;

  return the_target->read_loadmap (annex, offset, readbuf, len);
}

/* Handle qXfer:btrace:read.  */

static int
handle_qxfer_btrace (const char *annex,
		     gdb_byte *readbuf, const gdb_byte *writebuf,
		     ULONGEST offset, LONGEST len)
{
  client_state &cs = get_client_state ();
  static std::string cache;
  struct thread_info *thread;
  enum btrace_read_type type;
  int result;

  if (writebuf != NULL)
    return -2;

  if (cs.general_thread == null_ptid
      || cs.general_thread == minus_one_ptid)
    {
      strcpy (cs.own_buf, "E.Must select a single thread.");
      return -3;
    }

  thread = find_thread_ptid (cs.general_thread);
  if (thread == NULL)
    {
      strcpy (cs.own_buf, "E.No such thread.");
      return -3;
    }

  if (thread->btrace == NULL)
    {
      strcpy (cs.own_buf, "E.Btrace not enabled.");
      return -3;
    }

  if (strcmp (annex, "all") == 0)
    type = BTRACE_READ_ALL;
  else if (strcmp (annex, "new") == 0)
    type = BTRACE_READ_NEW;
  else if (strcmp (annex, "delta") == 0)
    type = BTRACE_READ_DELTA;
  else
    {
      strcpy (cs.own_buf, "E.Bad annex.");
      return -3;
    }

  if (offset == 0)
    {
      cache.clear ();

      try
	{
	  result = target_read_btrace (thread->btrace, &cache, type);
	  if (result != 0)
	    memcpy (cs.own_buf, cache.c_str (), cache.length ());
	}
      catch (const gdb_exception_error &exception)
	{
	  sprintf (cs.own_buf, "E.%s", exception.what ());
	  result = -1;
	}

      if (result != 0)
	return -3;
    }
  else if (offset > cache.length ())
    {
      cache.clear ();
      return -3;
    }

  if (len > cache.length () - offset)
    len = cache.length () - offset;

  memcpy (readbuf, cache.c_str () + offset, len);

  return len;
}

/* Handle qXfer:btrace-conf:read.  */

static int
handle_qxfer_btrace_conf (const char *annex,
			  gdb_byte *readbuf, const gdb_byte *writebuf,
			  ULONGEST offset, LONGEST len)
{
  client_state &cs = get_client_state ();
  static std::string cache;
  struct thread_info *thread;
  int result;

  if (writebuf != NULL)
    return -2;

  if (annex[0] != '\0')
    return -1;

  if (cs.general_thread == null_ptid
      || cs.general_thread == minus_one_ptid)
    {
      strcpy (cs.own_buf, "E.Must select a single thread.");
      return -3;
    }

  thread = find_thread_ptid (cs.general_thread);
  if (thread == NULL)
    {
      strcpy (cs.own_buf, "E.No such thread.");
      return -3;
    }

  if (thread->btrace == NULL)
    {
      strcpy (cs.own_buf, "E.Btrace not enabled.");
      return -3;
    }

  if (offset == 0)
    {
      cache.clear ();

      try
	{
	  result = target_read_btrace_conf (thread->btrace, &cache);
	  if (result != 0)
	    memcpy (cs.own_buf, cache.c_str (), cache.length ());
	}
      catch (const gdb_exception_error &exception)
	{
	  sprintf (cs.own_buf, "E.%s", exception.what ());
	  result = -1;
	}

      if (result != 0)
	return -3;
    }
  else if (offset > cache.length ())
    {
      cache.clear ();
      return -3;
    }

  if (len > cache.length () - offset)
    len = cache.length () - offset;

  memcpy (readbuf, cache.c_str () + offset, len);

  return len;
}

static const struct qxfer qxfer_packets[] =
  {
    { "auxv", handle_qxfer_auxv },
    { "btrace", handle_qxfer_btrace },
    { "btrace-conf", handle_qxfer_btrace_conf },
    { "exec-file", handle_qxfer_exec_file},
    { "fdpic", handle_qxfer_fdpic},
    { "features", handle_qxfer_features },
    { "libraries", handle_qxfer_libraries },
    { "libraries-svr4", handle_qxfer_libraries_svr4 },
    { "osdata", handle_qxfer_osdata },
    { "siginfo", handle_qxfer_siginfo },
    { "statictrace", handle_qxfer_statictrace },
    { "threads", handle_qxfer_threads },
    { "traceframe-info", handle_qxfer_traceframe_info },
  };

static int
handle_qxfer (char *own_buf, int packet_len, int *new_packet_len_p)
{
  int i;
  char *object;
  char *rw;
  char *annex;
  char *offset;

  if (!startswith (own_buf, "qXfer:"))
    return 0;

  /* Grab the object, r/w and annex.  */
  if (decode_xfer (own_buf + 6, &object, &rw, &annex, &offset) < 0)
    {
      write_enn (own_buf);
      return 1;
    }

  for (i = 0;
       i < sizeof (qxfer_packets) / sizeof (qxfer_packets[0]);
       i++)
    {
      const struct qxfer *q = &qxfer_packets[i];

      if (strcmp (object, q->object) == 0)
	{
	  if (strcmp (rw, "read") == 0)
	    {
	      unsigned char *data;
	      int n;
	      CORE_ADDR ofs;
	      unsigned int len;

	      /* Grab the offset and length.  */
	      if (decode_xfer_read (offset, &ofs, &len) < 0)
		{
		  write_enn (own_buf);
		  return 1;
		}

	      /* Read one extra byte, as an indicator of whether there is
		 more.  */
	      if (len > PBUFSIZ - 2)
		len = PBUFSIZ - 2;
	      data = (unsigned char *) malloc (len + 1);
	      if (data == NULL)
		{
		  write_enn (own_buf);
		  return 1;
		}
	      n = (*q->xfer) (annex, data, NULL, ofs, len + 1);
	      if (n == -2)
		{
		  free (data);
		  return 0;
		}
	      else if (n == -3)
		{
		  /* Preserve error message.  */
		}
	      else if (n < 0)
		write_enn (own_buf);
	      else if (n > len)
		*new_packet_len_p = write_qxfer_response (own_buf, data, len, 1);
	      else
		*new_packet_len_p = write_qxfer_response (own_buf, data, n, 0);

	      free (data);
	      return 1;
	    }
	  else if (strcmp (rw, "write") == 0)
	    {
	      int n;
	      unsigned int len;
	      CORE_ADDR ofs;
	      unsigned char *data;

	      strcpy (own_buf, "E00");
	      data = (unsigned char *) malloc (packet_len - (offset - own_buf));
	      if (data == NULL)
		{
		  write_enn (own_buf);
		  return 1;
		}
	      if (decode_xfer_write (offset, packet_len - (offset - own_buf),
				     &ofs, &len, data) < 0)
		{
		  free (data);
		  write_enn (own_buf);
		  return 1;
		}

	      n = (*q->xfer) (annex, NULL, data, ofs, len);
	      if (n == -2)
		{
		  free (data);
		  return 0;
		}
	      else if (n == -3)
		{
		  /* Preserve error message.  */
		}
	      else if (n < 0)
		write_enn (own_buf);
	      else
		sprintf (own_buf, "%x", n);

	      free (data);
	      return 1;
	    }

	  return 0;
	}
    }

  return 0;
}

/* Compute 32 bit CRC from inferior memory.

   On success, return 32 bit CRC.
   On failure, return (unsigned long long) -1.  */

static unsigned long long
crc32 (CORE_ADDR base, int len, unsigned int crc)
{
  while (len--)
    {
      unsigned char byte = 0;

      /* Return failure if memory read fails.  */
      if (read_inferior_memory (base, &byte, 1) != 0)
	return (unsigned long long) -1;

      crc = xcrc32 (&byte, 1, crc);
      base++;
    }
  return (unsigned long long) crc;
}

/* Parse the qMemTags packet request into ADDR and LEN.  */

static void
parse_fetch_memtags_request (char *request, CORE_ADDR *addr, size_t *len,
			     int *type)
{
  gdb_assert (startswith (request, "qMemTags:"));

  const char *p = request + strlen ("qMemTags:");

  /* Read address and length.  */
  unsigned int length = 0;
  p = decode_m_packet_params (p, addr, &length, ':');
  *len = length;

  /* Read the tag type.  */
  ULONGEST tag_type = 0;
  p = unpack_varlen_hex (p, &tag_type);
  *type = (int) tag_type;
}

/* Add supported btrace packets to BUF.  */

static void
supported_btrace_packets (char *buf)
{
  strcat (buf, ";Qbtrace:bts+");
  strcat (buf, ";Qbtrace-conf:bts:size+");
  strcat (buf, ";Qbtrace:pt+");
  strcat (buf, ";Qbtrace-conf:pt:size+");
  strcat (buf, ";Qbtrace:off+");
  strcat (buf, ";qXfer:btrace:read+");
  strcat (buf, ";qXfer:btrace-conf:read+");
}

/* Handle all of the extended 'q' packets.  */

static void
handle_query (char *own_buf, int packet_len, int *new_packet_len_p)
{
  client_state &cs = get_client_state ();
  static std::list<thread_info *>::const_iterator thread_iter;

  /* Reply the current thread id.  */
  if (strcmp ("qC", own_buf) == 0 && !disable_packet_qC)
    {
      ptid_t ptid;
      require_running_or_return (own_buf);

      if (cs.general_thread != null_ptid && cs.general_thread != minus_one_ptid)
	ptid = cs.general_thread;
      else
	{
	  thread_iter = all_threads.begin ();
	  ptid = (*thread_iter)->id;
	}

      sprintf (own_buf, "QC");
      own_buf += 2;
      write_ptid (own_buf, ptid);
      return;
    }

  if (strcmp ("qSymbol::", own_buf) == 0)
    {
      scoped_restore_current_thread restore_thread;

      /* For qSymbol, GDB only changes the current thread if the
	 previous current thread was of a different process.  So if
	 the previous thread is gone, we need to pick another one of
	 the same process.  This can happen e.g., if we followed an
	 exec in a non-leader thread.  */
      if (current_thread == NULL)
	{
	  thread_info *any_thread
	    = find_any_thread_of_pid (cs.general_thread.pid ());
	  switch_to_thread (any_thread);

	  /* Just in case, if we didn't find a thread, then bail out
	     instead of crashing.  */
	  if (current_thread == NULL)
	    {
	      write_enn (own_buf);
	      return;
	    }
	}

      /* GDB is suggesting new symbols have been loaded.  This may
	 mean a new shared library has been detected as loaded, so
	 take the opportunity to check if breakpoints we think are
	 inserted, still are.  Note that it isn't guaranteed that
	 we'll see this when a shared library is loaded, and nor will
	 we see this for unloads (although breakpoints in unloaded
	 libraries shouldn't trigger), as GDB may not find symbols for
	 the library at all.  We also re-validate breakpoints when we
	 see a second GDB breakpoint for the same address, and or when
	 we access breakpoint shadows.  */
      validate_breakpoints ();

      if (target_supports_tracepoints ())
	tracepoint_look_up_symbols ();

      if (current_thread != NULL)
	the_target->look_up_symbols ();

      strcpy (own_buf, "OK");
      return;
    }

  if (!disable_packet_qfThreadInfo)
    {
      if (strcmp ("qfThreadInfo", own_buf) == 0)
	{
	  require_running_or_return (own_buf);
	  thread_iter = all_threads.begin ();

	  *own_buf++ = 'm';
	  ptid_t ptid = (*thread_iter)->id;
	  write_ptid (own_buf, ptid);
	  thread_iter++;
	  return;
	}

      if (strcmp ("qsThreadInfo", own_buf) == 0)
	{
	  require_running_or_return (own_buf);
	  if (thread_iter != all_threads.end ())
	    {
	      *own_buf++ = 'm';
	      ptid_t ptid = (*thread_iter)->id;
	      write_ptid (own_buf, ptid);
	      thread_iter++;
	      return;
	    }
	  else
	    {
	      sprintf (own_buf, "l");
	      return;
	    }
	}
    }

  if (the_target->supports_read_offsets ()
      && strcmp ("qOffsets", own_buf) == 0)
    {
      CORE_ADDR text, data;

      require_running_or_return (own_buf);
      if (the_target->read_offsets (&text, &data))
	sprintf (own_buf, "Text=%lX;Data=%lX;Bss=%lX",
		 (long)text, (long)data, (long)data);
      else
	write_enn (own_buf);

      return;
    }

  /* Protocol features query.  */
  if (startswith (own_buf, "qSupported")
      && (own_buf[10] == ':' || own_buf[10] == '\0'))
    {
      char *p = &own_buf[10];
      int gdb_supports_qRelocInsn = 0;

      /* Process each feature being provided by GDB.  The first
	 feature will follow a ':', and latter features will follow
	 ';'.  */
      if (*p == ':')
	{
	  std::vector<std::string> qsupported;
	  std::vector<const char *> unknowns;

	  /* Two passes, to avoid nested strtok calls in
	     target_process_qsupported.  */
	  char *saveptr;
	  for (p = strtok_r (p + 1, ";", &saveptr);
	       p != NULL;
	       p = strtok_r (NULL, ";", &saveptr))
	    qsupported.emplace_back (p);

	  for (const std::string &feature : qsupported)
	    {
	      if (feature == "multiprocess+")
		{
		  /* GDB supports and wants multi-process support if
		     possible.  */
		  if (target_supports_multi_process ())
		    cs.multi_process = 1;
		}
	      else if (feature == "qRelocInsn+")
		{
		  /* GDB supports relocate instruction requests.  */
		  gdb_supports_qRelocInsn = 1;
		}
	      else if (feature == "swbreak+")
		{
		  /* GDB wants us to report whether a trap is caused
		     by a software breakpoint and for us to handle PC
		     adjustment if necessary on this target.  */
		  if (target_supports_stopped_by_sw_breakpoint ())
		    cs.swbreak_feature = 1;
		}
	      else if (feature == "hwbreak+")
		{
		  /* GDB wants us to report whether a trap is caused
		     by a hardware breakpoint.  */
		  if (target_supports_stopped_by_hw_breakpoint ())
		    cs.hwbreak_feature = 1;
		}
	      else if (feature == "fork-events+")
		{
		  /* GDB supports and wants fork events if possible.  */
		  if (target_supports_fork_events ())
		    cs.report_fork_events = 1;
		}
	      else if (feature == "vfork-events+")
		{
		  /* GDB supports and wants vfork events if possible.  */
		  if (target_supports_vfork_events ())
		    cs.report_vfork_events = 1;
		}
	      else if (feature == "exec-events+")
		{
		  /* GDB supports and wants exec events if possible.  */
		  if (target_supports_exec_events ())
		    cs.report_exec_events = 1;
		}
	      else if (feature == "vContSupported+")
		cs.vCont_supported = 1;
	      else if (feature == "QThreadEvents+")
		;
	      else if (feature == "QThreadOptions+")
		;
	      else if (feature == "no-resumed+")
		{
		  /* GDB supports and wants TARGET_WAITKIND_NO_RESUMED
		     events.  */
		  report_no_resumed = true;
		}
	      else if (feature == "memory-tagging+")
		{
		  /* GDB supports memory tagging features.  */
		  if (target_supports_memory_tagging ())
		    cs.memory_tagging_feature = true;
		}
	      else
		{
		  /* Move the unknown features all together.  */
		  unknowns.push_back (feature.c_str ());
		}
	    }

	  /* Give the target backend a chance to process the unknown
	     features.  */
	  target_process_qsupported (unknowns);
	}

      sprintf (own_buf,
	       "PacketSize=%x;QPassSignals+;QProgramSignals+;"
	       "QStartupWithShell+;QEnvironmentHexEncoded+;"
	       "QEnvironmentReset+;QEnvironmentUnset+;"
	       "QSetWorkingDir+",
	       PBUFSIZ - 1);

      if (target_supports_catch_syscall ())
	strcat (own_buf, ";QCatchSyscalls+");

      if (the_target->supports_qxfer_libraries_svr4 ())
	strcat (own_buf, ";qXfer:libraries-svr4:read+"
		";augmented-libraries-svr4-read+");
      else
	{
	  /* We do not have any hook to indicate whether the non-SVR4 target
	     backend supports qXfer:libraries:read, so always report it.  */
	  strcat (own_buf, ";qXfer:libraries:read+");
	}

      if (the_target->supports_read_auxv ())
	strcat (own_buf, ";qXfer:auxv:read+");

      if (the_target->supports_qxfer_siginfo ())
	strcat (own_buf, ";qXfer:siginfo:read+;qXfer:siginfo:write+");

      if (the_target->supports_read_loadmap ())
	strcat (own_buf, ";qXfer:fdpic:read+");

      /* We always report qXfer:features:read, as targets may
	 install XML files on a subsequent call to arch_setup.
	 If we reported to GDB on startup that we don't support
	 qXfer:feature:read at all, we will never be re-queried.  */
      strcat (own_buf, ";qXfer:features:read+");

      if (cs.transport_is_reliable)
	strcat (own_buf, ";QStartNoAckMode+");

      if (the_target->supports_qxfer_osdata ())
	strcat (own_buf, ";qXfer:osdata:read+");

      if (target_supports_multi_process ())
	strcat (own_buf, ";multiprocess+");

      if (target_supports_fork_events ())
	strcat (own_buf, ";fork-events+");

      if (target_supports_vfork_events ())
	strcat (own_buf, ";vfork-events+");

      if (target_supports_exec_events ())
	strcat (own_buf, ";exec-events+");

      if (target_supports_non_stop ())
	strcat (own_buf, ";QNonStop+");

      if (target_supports_disable_randomization ())
	strcat (own_buf, ";QDisableRandomization+");

      strcat (own_buf, ";qXfer:threads:read+");

      if (target_supports_tracepoints ())
	{
	  strcat (own_buf, ";ConditionalTracepoints+");
	  strcat (own_buf, ";TraceStateVariables+");
	  strcat (own_buf, ";TracepointSource+");
	  strcat (own_buf, ";DisconnectedTracing+");
	  if (gdb_supports_qRelocInsn && target_supports_fast_tracepoints ())
	    strcat (own_buf, ";FastTracepoints+");
	  strcat (own_buf, ";StaticTracepoints+");
	  strcat (own_buf, ";InstallInTrace+");
	  strcat (own_buf, ";qXfer:statictrace:read+");
	  strcat (own_buf, ";qXfer:traceframe-info:read+");
	  strcat (own_buf, ";EnableDisableTracepoints+");
	  strcat (own_buf, ";QTBuffer:size+");
	  strcat (own_buf, ";tracenz+");
	}

      if (target_supports_hardware_single_step ()
	  || target_supports_software_single_step () )
	{
	  strcat (own_buf, ";ConditionalBreakpoints+");
	}
      strcat (own_buf, ";BreakpointCommands+");

      if (target_supports_agent ())
	strcat (own_buf, ";QAgent+");

      if (the_target->supports_btrace ())
	supported_btrace_packets (own_buf);

      if (target_supports_stopped_by_sw_breakpoint ())
	strcat (own_buf, ";swbreak+");

      if (target_supports_stopped_by_hw_breakpoint ())
	strcat (own_buf, ";hwbreak+");

      if (the_target->supports_pid_to_exec_file ())
	strcat (own_buf, ";qXfer:exec-file:read+");

      strcat (own_buf, ";vContSupported+");

      gdb_thread_options supported_options = target_supported_thread_options ();
      if (supported_options != 0)
	{
	  char *end_buf = own_buf + strlen (own_buf);
	  sprintf (end_buf, ";QThreadOptions=%s",
		   phex_nz (supported_options, sizeof (supported_options)));
	}

      strcat (own_buf, ";QThreadEvents+");

      strcat (own_buf, ";no-resumed+");

      if (target_supports_memory_tagging ())
	strcat (own_buf, ";memory-tagging+");

      /* Reinitialize components as needed for the new connection.  */
      hostio_handle_new_gdb_connection ();
      target_handle_new_gdb_connection ();

      return;
    }

  /* Thread-local storage support.  */
  if (the_target->supports_get_tls_address ()
      && startswith (own_buf, "qGetTLSAddr:"))
    {
      char *p = own_buf + 12;
      CORE_ADDR parts[2], address = 0;
      int i, err;
      ptid_t ptid = null_ptid;

      require_running_or_return (own_buf);

      for (i = 0; i < 3; i++)
	{
	  char *p2;
	  int len;

	  if (p == NULL)
	    break;

	  p2 = strchr (p, ',');
	  if (p2)
	    {
	      len = p2 - p;
	      p2++;
	    }
	  else
	    {
	      len = strlen (p);
	      p2 = NULL;
	    }

	  if (i == 0)
	    ptid = read_ptid (p, NULL);
	  else
	    decode_address (&parts[i - 1], p, len);
	  p = p2;
	}

      if (p != NULL || i < 3)
	err = 1;
      else
	{
	  struct thread_info *thread = find_thread_ptid (ptid);

	  if (thread == NULL)
	    err = 2;
	  else
	    err = the_target->get_tls_address (thread, parts[0], parts[1],
					       &address);
	}

      if (err == 0)
	{
	  strcpy (own_buf, paddress(address));
	  return;
	}
      else if (err > 0)
	{
	  write_enn (own_buf);
	  return;
	}

      /* Otherwise, pretend we do not understand this packet.  */
    }

  /* Windows OS Thread Information Block address support.  */
  if (the_target->supports_get_tib_address ()
      && startswith (own_buf, "qGetTIBAddr:"))
    {
      const char *annex;
      int n;
      CORE_ADDR tlb;
      ptid_t ptid = read_ptid (own_buf + 12, &annex);

      n = the_target->get_tib_address (ptid, &tlb);
      if (n == 1)
	{
	  strcpy (own_buf, paddress(tlb));
	  return;
	}
      else if (n == 0)
	{
	  write_enn (own_buf);
	  return;
	}
      return;
    }

  /* Handle "monitor" commands.  */
  if (startswith (own_buf, "qRcmd,"))
    {
      char *mon = (char *) malloc (PBUFSIZ);
      int len = strlen (own_buf + 6);

      if (mon == NULL)
	{
	  write_enn (own_buf);
	  return;
	}

      if ((len % 2) != 0
	  || hex2bin (own_buf + 6, (gdb_byte *) mon, len / 2) != len / 2)
	{
	  write_enn (own_buf);
	  free (mon);
	  return;
	}
      mon[len / 2] = '\0';

      write_ok (own_buf);

      if (the_target->handle_monitor_command (mon) == 0)
	/* Default processing.  */
	handle_monitor_command (mon, own_buf);

      free (mon);
      return;
    }

  if (startswith (own_buf, "qSearch:memory:"))
    {
      require_running_or_return (own_buf);
      handle_search_memory (own_buf, packet_len);
      return;
    }

  if (strcmp (own_buf, "qAttached") == 0
      || startswith (own_buf, "qAttached:"))
    {
      struct process_info *process;

      if (own_buf[sizeof ("qAttached") - 1])
	{
	  int pid = strtoul (own_buf + sizeof ("qAttached:") - 1, NULL, 16);
	  process = find_process_pid (pid);
	}
      else
	{
	  require_running_or_return (own_buf);
	  process = current_process ();
	}

      if (process == NULL)
	{
	  write_enn (own_buf);
	  return;
	}

      strcpy (own_buf, process->attached ? "1" : "0");
      return;
    }

  if (startswith (own_buf, "qCRC:"))
    {
      /* CRC check (compare-section).  */
      const char *comma;
      ULONGEST base;
      int len;
      unsigned long long crc;

      require_running_or_return (own_buf);
      comma = unpack_varlen_hex (own_buf + 5, &base);
      if (*comma++ != ',')
	{
	  write_enn (own_buf);
	  return;
	}
      len = strtoul (comma, NULL, 16);
      crc = crc32 (base, len, 0xffffffff);
      /* Check for memory failure.  */
      if (crc == (unsigned long long) -1)
	{
	  write_enn (own_buf);
	  return;
	}
      sprintf (own_buf, "C%lx", (unsigned long) crc);
      return;
    }

  if (handle_qxfer (own_buf, packet_len, new_packet_len_p))
    return;

  if (target_supports_tracepoints () && handle_tracepoint_query (own_buf))
    return;

  /* Handle fetch memory tags packets.  */
  if (startswith (own_buf, "qMemTags:")
      && target_supports_memory_tagging ())
    {
      gdb::byte_vector tags;
      CORE_ADDR addr = 0;
      size_t len = 0;
      int type = 0;

      require_running_or_return (own_buf);

      parse_fetch_memtags_request (own_buf, &addr, &len, &type);

      bool ret = the_target->fetch_memtags (addr, len, tags, type);

      if (ret)
	ret = create_fetch_memtags_reply (own_buf, tags);

      if (!ret)
	write_enn (own_buf);

      *new_packet_len_p = strlen (own_buf);
      return;
    }

  /* Otherwise we didn't know what packet it was.  Say we didn't
     understand it.  */
  own_buf[0] = 0;
}

static void gdb_wants_all_threads_stopped (void);
static void resume (struct thread_resume *actions, size_t n);

/* The callback that is passed to visit_actioned_threads.  */
typedef int (visit_actioned_threads_callback_ftype)
  (const struct thread_resume *, struct thread_info *);

/* Call CALLBACK for any thread to which ACTIONS applies to.  Returns
   true if CALLBACK returns true.  Returns false if no matching thread
   is found or CALLBACK results false.
   Note: This function is itself a callback for find_thread.  */

static bool
visit_actioned_threads (thread_info *thread,
			const struct thread_resume *actions,
			size_t num_actions,
			visit_actioned_threads_callback_ftype *callback)
{
  for (size_t i = 0; i < num_actions; i++)
    {
      const struct thread_resume *action = &actions[i];

      if (action->thread == minus_one_ptid
	  || action->thread == thread->id
	  || ((action->thread.pid ()
	       == thread->id.pid ())
	      && action->thread.lwp () == -1))
	{
	  if ((*callback) (action, thread))
	    return true;
	}
    }

  return false;
}

/* Callback for visit_actioned_threads.  If the thread has a pending
   status to report, report it now.  */

static int
handle_pending_status (const struct thread_resume *resumption,
		       struct thread_info *thread)
{
  client_state &cs = get_client_state ();
  if (thread->status_pending_p)
    {
      thread->status_pending_p = 0;

      cs.last_status = thread->last_status;
      cs.last_ptid = thread->id;
      prepare_resume_reply (cs.own_buf, cs.last_ptid, cs.last_status);
      return 1;
    }
  return 0;
}

/* Parse vCont packets.  */
static void
handle_v_cont (char *own_buf)
{
  const char *p;
  int n = 0, i = 0;
  struct thread_resume *resume_info;
  struct thread_resume default_action { null_ptid };

  /* Count the number of semicolons in the packet.  There should be one
     for every action.  */
  p = &own_buf[5];
  while (p)
    {
      n++;
      p++;
      p = strchr (p, ';');
    }

  resume_info = (struct thread_resume *) malloc (n * sizeof (resume_info[0]));
  if (resume_info == NULL)
    goto err;

  p = &own_buf[5];
  while (*p)
    {
      p++;

      memset (&resume_info[i], 0, sizeof resume_info[i]);

      if (p[0] == 's' || p[0] == 'S')
	resume_info[i].kind = resume_step;
      else if (p[0] == 'r')
	resume_info[i].kind = resume_step;
      else if (p[0] == 'c' || p[0] == 'C')
	resume_info[i].kind = resume_continue;
      else if (p[0] == 't')
	resume_info[i].kind = resume_stop;
      else
	goto err;

      if (p[0] == 'S' || p[0] == 'C')
	{
	  char *q;
	  int sig = strtol (p + 1, &q, 16);
	  if (p == q)
	    goto err;
	  p = q;

	  if (!gdb_signal_to_host_p ((enum gdb_signal) sig))
	    goto err;
	  resume_info[i].sig = gdb_signal_to_host ((enum gdb_signal) sig);
	}
      else if (p[0] == 'r')
	{
	  ULONGEST addr;

	  p = unpack_varlen_hex (p + 1, &addr);
	  resume_info[i].step_range_start = addr;

	  if (*p != ',')
	    goto err;

	  p = unpack_varlen_hex (p + 1, &addr);
	  resume_info[i].step_range_end = addr;
	}
      else
	{
	  p = p + 1;
	}

      if (p[0] == 0)
	{
	  resume_info[i].thread = minus_one_ptid;
	  default_action = resume_info[i];

	  /* Note: we don't increment i here, we'll overwrite this entry
	     the next time through.  */
	}
      else if (p[0] == ':')
	{
	  const char *q;
	  ptid_t ptid = read_ptid (p + 1, &q);

	  if (p == q)
	    goto err;
	  p = q;
	  if (p[0] != ';' && p[0] != 0)
	    goto err;

	  resume_info[i].thread = ptid;

	  i++;
	}
    }

  if (i < n)
    resume_info[i] = default_action;

  resume (resume_info, n);
  free (resume_info);
  return;

err:
  write_enn (own_buf);
  free (resume_info);
  return;
}

/* Resume target with ACTIONS, an array of NUM_ACTIONS elements.  */

static void
resume (struct thread_resume *actions, size_t num_actions)
{
  client_state &cs = get_client_state ();
  if (!non_stop)
    {
      /* Check if among the threads that GDB wants actioned, there's
	 one with a pending status to report.  If so, skip actually
	 resuming/stopping and report the pending event
	 immediately.  */

      thread_info *thread_with_status = find_thread ([&] (thread_info *thread)
	{
	  return visit_actioned_threads (thread, actions, num_actions,
					 handle_pending_status);
	});

      if (thread_with_status != NULL)
	return;

      enable_async_io ();
    }

  the_target->resume (actions, num_actions);

  if (non_stop)
    write_ok (cs.own_buf);
  else
    {
      cs.last_ptid = mywait (minus_one_ptid, &cs.last_status, 0, 1);

      if (cs.last_status.kind () == TARGET_WAITKIND_NO_RESUMED
	  && !report_no_resumed)
	{
	  /* The client does not support this stop reply.  At least
	     return error.  */
	  sprintf (cs.own_buf, "E.No unwaited-for children left.");
	  disable_async_io ();
	  return;
	}

      if (cs.last_status.kind () != TARGET_WAITKIND_EXITED
	  && cs.last_status.kind () != TARGET_WAITKIND_SIGNALLED
	  && cs.last_status.kind () != TARGET_WAITKIND_THREAD_EXITED
	  && cs.last_status.kind () != TARGET_WAITKIND_NO_RESUMED)
	current_thread->last_status = cs.last_status;

      /* From the client's perspective, all-stop mode always stops all
	 threads implicitly (and the target backend has already done
	 so by now).  Tag all threads as "want-stopped", so we don't
	 resume them implicitly without the client telling us to.  */
      gdb_wants_all_threads_stopped ();
      prepare_resume_reply (cs.own_buf, cs.last_ptid, cs.last_status);
      disable_async_io ();

      if (cs.last_status.kind () == TARGET_WAITKIND_EXITED
	  || cs.last_status.kind () == TARGET_WAITKIND_SIGNALLED)
	target_mourn_inferior (cs.last_ptid);
    }
}

/* Attach to a new program.  */
static void
handle_v_attach (char *own_buf)
{
  client_state &cs = get_client_state ();
  int pid;

  pid = strtol (own_buf + 8, NULL, 16);
  if (pid != 0 && attach_inferior (pid) == 0)
    {
      /* Don't report shared library events after attaching, even if
	 some libraries are preloaded.  GDB will always poll the
	 library list.  Avoids the "stopped by shared library event"
	 notice on the GDB side.  */
      current_process ()->dlls_changed = false;

      if (non_stop)
	{
	  /* In non-stop, we don't send a resume reply.  Stop events
	     will follow up using the normal notification
	     mechanism.  */
	  write_ok (own_buf);
	}
      else
	prepare_resume_reply (own_buf, cs.last_ptid, cs.last_status);
    }
  else
    write_enn (own_buf);
}

/* Decode an argument from the vRun packet buffer.  PTR points to the
   first hex-encoded character in the buffer, and LEN is the number of
   characters to read from the packet buffer.

   If the argument decoding is successful, return a buffer containing the
   decoded argument, including a null terminator at the end.

   If the argument decoding fails for any reason, return nullptr.  */

static gdb::unique_xmalloc_ptr<char>
decode_v_run_arg (const char *ptr, size_t len)
{
  /* Two hex characters are required for each decoded byte.  */
  if (len % 2 != 0)
    return nullptr;

  /* The length in bytes needed for the decoded argument.  */
  len /= 2;

  /* Buffer to decode the argument into.  The '+ 1' is for the null
     terminator we will add.  */
  char *arg = (char *) xmalloc (len + 1);

  /* Decode the argument from the packet and add a null terminator.  We do
     this within a try block as invalid characters within the PTR buffer
     will cause hex2bin to throw an exception.  Our caller relies on us
     returning nullptr in order to clean up some memory allocations.  */
  try
    {
      hex2bin (ptr, (gdb_byte *) arg, len);
      arg[len] = '\0';
    }
  catch (const gdb_exception_error &exception)
    {
      return nullptr;
    }

  return gdb::unique_xmalloc_ptr<char> (arg);
}

/* Run a new program.  */
static void
handle_v_run (char *own_buf)
{
  client_state &cs = get_client_state ();
  char *p, *next_p;
  std::vector<char *> new_argv;
  gdb::unique_xmalloc_ptr<char> new_program_name;
  int i;

  for (i = 0, p = own_buf + strlen ("vRun;");
       /* Exit condition is at the end of the loop.  */;
       p = next_p + 1, ++i)
    {
      next_p = strchr (p, ';');
      if (next_p == NULL)
	next_p = p + strlen (p);

      if (i == 0 && p == next_p)
	{
	  /* No program specified.  */
	  gdb_assert (new_program_name == nullptr);
	}
      else if (p == next_p)
	{
	  /* Empty argument.  */
	  new_argv.push_back (xstrdup (""));
	}
      else
	{
	  /* The length of the argument string in the packet.  */
	  size_t len = next_p - p;

	  gdb::unique_xmalloc_ptr<char> arg = decode_v_run_arg (p, len);
	  if (arg == nullptr)
	    {
	      write_enn (own_buf);
	      free_vector_argv (new_argv);
	      return;
	    }

	  if (i == 0)
	    new_program_name = std::move (arg);
	  else
	    new_argv.push_back (arg.release ());
	}
      if (*next_p == '\0')
	break;
    }

  if (new_program_name == nullptr)
    {
      /* GDB didn't specify a program to run.  Use the program from the
	 last run with the new argument list.  */
      if (program_path.get () == nullptr)
	{
	  write_enn (own_buf);
	  free_vector_argv (new_argv);
	  return;
	}
    }
  else
    program_path.set (new_program_name.get ());

  /* Free the old argv and install the new one.  */
  free_vector_argv (program_args);
  program_args = new_argv;

  target_create_inferior (program_path.get (), program_args);

  if (cs.last_status.kind () == TARGET_WAITKIND_STOPPED)
    {
      prepare_resume_reply (own_buf, cs.last_ptid, cs.last_status);

      /* In non-stop, sending a resume reply doesn't set the general
	 thread, but GDB assumes a vRun sets it (this is so GDB can
	 query which is the main thread of the new inferior.  */
      if (non_stop)
	cs.general_thread = cs.last_ptid;
    }
  else
    write_enn (own_buf);
}

/* Kill process.  */
static void
handle_v_kill (char *own_buf)
{
  client_state &cs = get_client_state ();
  int pid;
  char *p = &own_buf[6];
  if (cs.multi_process)
    pid = strtol (p, NULL, 16);
  else
    pid = signal_pid;

  process_info *proc = find_process_pid (pid);

  if (proc != nullptr && kill_inferior (proc) == 0)
    {
      cs.last_status.set_signalled (GDB_SIGNAL_KILL);
      cs.last_ptid = ptid_t (pid);
      discard_queued_stop_replies (cs.last_ptid);
      write_ok (own_buf);
    }
  else
    write_enn (own_buf);
}

/* Handle all of the extended 'v' packets.  */
void
handle_v_requests (char *own_buf, int packet_len, int *new_packet_len)
{
  client_state &cs = get_client_state ();
  if (!disable_packet_vCont)
    {
      if (strcmp (own_buf, "vCtrlC") == 0)
	{
	  the_target->request_interrupt ();
	  write_ok (own_buf);
	  return;
	}

      if (startswith (own_buf, "vCont;"))
	{
	  handle_v_cont (own_buf);
	  return;
	}

      if (startswith (own_buf, "vCont?"))
	{
	  strcpy (own_buf, "vCont;c;C;t");

	  if (target_supports_hardware_single_step ()
	      || target_supports_software_single_step ()
	      || !cs.vCont_supported)
	    {
	      /* If target supports single step either by hardware or by
		 software, add actions s and S to the list of supported
		 actions.  On the other hand, if GDB doesn't request the
		 supported vCont actions in qSupported packet, add s and
		 S to the list too.  */
	      own_buf = own_buf + strlen (own_buf);
	      strcpy (own_buf, ";s;S");
	    }

	  if (target_supports_range_stepping ())
	    {
	      own_buf = own_buf + strlen (own_buf);
	      strcpy (own_buf, ";r");
	    }
	  return;
	}
    }

  if (startswith (own_buf, "vFile:")
      && handle_vFile (own_buf, packet_len, new_packet_len))
    return;

  if (startswith (own_buf, "vAttach;"))
    {
      if ((!extended_protocol || !cs.multi_process) && target_running ())
	{
	  fprintf (stderr, "Already debugging a process\n");
	  write_enn (own_buf);
	  return;
	}
      handle_v_attach (own_buf);
      return;
    }

  if (startswith (own_buf, "vRun;"))
    {
      if ((!extended_protocol || !cs.multi_process) && target_running ())
	{
	  fprintf (stderr, "Already debugging a process\n");
	  write_enn (own_buf);
	  return;
	}
      handle_v_run (own_buf);
      return;
    }

  if (startswith (own_buf, "vKill;"))
    {
      if (!target_running ())
	{
	  fprintf (stderr, "No process to kill\n");
	  write_enn (own_buf);
	  return;
	}
      handle_v_kill (own_buf);
      return;
    }

  if (handle_notif_ack (own_buf, packet_len))
    return;

  /* Otherwise we didn't know what packet it was.  Say we didn't
     understand it.  */
  own_buf[0] = 0;
  return;
}

/* Resume thread and wait for another event.  In non-stop mode,
   don't really wait here, but return immediately to the event
   loop.  */
static void
myresume (char *own_buf, int step, int sig)
{
  client_state &cs = get_client_state ();
  struct thread_resume resume_info[2];
  int n = 0;
  int valid_cont_thread;

  valid_cont_thread = (cs.cont_thread != null_ptid
			 && cs.cont_thread != minus_one_ptid);

  if (step || sig || valid_cont_thread)
    {
      resume_info[0].thread = current_ptid;
      if (step)
	resume_info[0].kind = resume_step;
      else
	resume_info[0].kind = resume_continue;
      resume_info[0].sig = sig;
      n++;
    }

  if (!valid_cont_thread)
    {
      resume_info[n].thread = minus_one_ptid;
      resume_info[n].kind = resume_continue;
      resume_info[n].sig = 0;
      n++;
    }

  resume (resume_info, n);
}

/* Callback for for_each_thread.  Make a new stop reply for each
   stopped thread.  */

static void
queue_stop_reply_callback (thread_info *thread)
{
  /* For now, assume targets that don't have this callback also don't
     manage the thread's last_status field.  */
  if (!the_target->supports_thread_stopped ())
    {
      struct vstop_notif *new_notif = new struct vstop_notif;

      new_notif->ptid = thread->id;
      new_notif->status = thread->last_status;
      /* Pass the last stop reply back to GDB, but don't notify
	 yet.  */
      notif_event_enque (&notif_stop, new_notif);
    }
  else
    {
      if (target_thread_stopped (thread))
	{
	  threads_debug_printf
	    ("Reporting thread %s as already stopped with %s",
	     target_pid_to_str (thread->id).c_str (),
	     thread->last_status.to_string ().c_str ());

	  gdb_assert (thread->last_status.kind () != TARGET_WAITKIND_IGNORE);

	  /* Pass the last stop reply back to GDB, but don't notify
	     yet.  */
	  queue_stop_reply (thread->id, thread->last_status);
	}
    }
}

/* Set this inferior threads's state as "want-stopped".  We won't
   resume this thread until the client gives us another action for
   it.  */

static void
gdb_wants_thread_stopped (thread_info *thread)
{
  thread->last_resume_kind = resume_stop;

  if (thread->last_status.kind () == TARGET_WAITKIND_IGNORE)
    {
      /* Most threads are stopped implicitly (all-stop); tag that with
	 signal 0.  */
      thread->last_status.set_stopped (GDB_SIGNAL_0);
    }
}

/* Set all threads' states as "want-stopped".  */

static void
gdb_wants_all_threads_stopped (void)
{
  for_each_thread (gdb_wants_thread_stopped);
}

/* Callback for for_each_thread.  If the thread is stopped with an
   interesting event, mark it as having a pending event.  */

static void
set_pending_status_callback (thread_info *thread)
{
  if (thread->last_status.kind () != TARGET_WAITKIND_STOPPED
      || (thread->last_status.sig () != GDB_SIGNAL_0
	  /* A breakpoint, watchpoint or finished step from a previous
	     GDB run isn't considered interesting for a new GDB run.
	     If we left those pending, the new GDB could consider them
	     random SIGTRAPs.  This leaves out real async traps.  We'd
	     have to peek into the (target-specific) siginfo to
	     distinguish those.  */
	  && thread->last_status.sig () != GDB_SIGNAL_TRAP))
    thread->status_pending_p = 1;
}

/* Status handler for the '?' packet.  */

static void
handle_status (char *own_buf)
{
  client_state &cs = get_client_state ();

  /* GDB is connected, don't forward events to the target anymore.  */
  for_each_process ([] (process_info *process) {
    process->gdb_detached = 0;
  });

  /* In non-stop mode, we must send a stop reply for each stopped
     thread.  In all-stop mode, just send one for the first stopped
     thread we find.  */

  if (non_stop)
    {
      for_each_thread (queue_stop_reply_callback);

      /* The first is sent immediatly.  OK is sent if there is no
	 stopped thread, which is the same handling of the vStopped
	 packet (by design).  */
      notif_write_event (&notif_stop, cs.own_buf);
    }
  else
    {
      thread_info *thread = NULL;

      target_pause_all (false);
      target_stabilize_threads ();
      gdb_wants_all_threads_stopped ();

      /* We can only report one status, but we might be coming out of
	 non-stop -- if more than one thread is stopped with
	 interesting events, leave events for the threads we're not
	 reporting now pending.  They'll be reported the next time the
	 threads are resumed.  Start by marking all interesting events
	 as pending.  */
      for_each_thread (set_pending_status_callback);

      /* Prefer the last thread that reported an event to GDB (even if
	 that was a GDB_SIGNAL_TRAP).  */
      if (cs.last_status.kind () != TARGET_WAITKIND_IGNORE
	  && cs.last_status.kind () != TARGET_WAITKIND_EXITED
	  && cs.last_status.kind () != TARGET_WAITKIND_SIGNALLED)
	thread = find_thread_ptid (cs.last_ptid);

      /* If the last event thread is not found for some reason, look
	 for some other thread that might have an event to report.  */
      if (thread == NULL)
	thread = find_thread ([] (thread_info *thr_arg)
	  {
	    return thr_arg->status_pending_p;
	  });

      /* If we're still out of luck, simply pick the first thread in
	 the thread list.  */
      if (thread == NULL)
	thread = get_first_thread ();

      if (thread != NULL)
	{
	  struct thread_info *tp = (struct thread_info *) thread;

	  /* We're reporting this event, so it's no longer
	     pending.  */
	  tp->status_pending_p = 0;

	  /* GDB assumes the current thread is the thread we're
	     reporting the status for.  */
	  cs.general_thread = thread->id;
	  set_desired_thread ();

	  gdb_assert (tp->last_status.kind () != TARGET_WAITKIND_IGNORE);
	  prepare_resume_reply (own_buf, tp->id, tp->last_status);
	}
      else
	strcpy (own_buf, "W00");
    }
}

static void
gdbserver_version (void)
{
  printf ("GNU gdbserver %s%s\n"
	  "Copyright (C) 2024 Free Software Foundation, Inc.\n"
	  "gdbserver is free software, covered by the "
	  "GNU General Public License.\n"
	  "This gdbserver was configured as \"%s\"\n",
	  PKGVERSION, version, host_name);
}

static void
gdbserver_usage (FILE *stream)
{
  fprintf (stream, "Usage:\tgdbserver [OPTIONS] COMM PROG [ARGS ...]\n"
	   "\tgdbserver [OPTIONS] --attach COMM PID\n"
	   "\tgdbserver [OPTIONS] --multi COMM\n"
	   "\n"
	   "COMM may either be a tty device (for serial debugging),\n"
	   "HOST:PORT to listen for a TCP connection, or '-' or 'stdio' to use \n"
	   "stdin/stdout of gdbserver.\n"
	   "PROG is the executable program.  ARGS are arguments passed to inferior.\n"
	   "PID is the process ID to attach to, when --attach is specified.\n"
	   "\n"
	   "Operating modes:\n"
	   "\n"
	   "  --attach              Attach to running process PID.\n"
	   "  --multi               Start server without a specific program, and\n"
	   "                        only quit when explicitly commanded.\n"
	   "  --once                Exit after the first connection has closed.\n"
	   "  --help                Print this message and then exit.\n"
	   "  --version             Display version information and exit.\n"
	   "\n"
	   "Other options:\n"
	   "\n"
	   "  --wrapper WRAPPER --  Run WRAPPER to start new programs.\n"
	   "  --disable-randomization\n"
	   "                        Run PROG with address space randomization disabled.\n"
	   "  --no-disable-randomization\n"
	   "                        Don't disable address space randomization when\n"
	   "                        starting PROG.\n"
	   "  --startup-with-shell\n"
	   "                        Start PROG using a shell.  I.e., execs a shell that\n"
	   "                        then execs PROG.  (default)\n"
	   "  --no-startup-with-shell\n"
	   "                        Exec PROG directly instead of using a shell.\n"
	   "                        Disables argument globbing and variable substitution\n"
	   "                        on UNIX-like systems.\n"
	   "\n"
	   "Debug options:\n"
	   "\n"
	   "  --debug[=OPT1,OPT2,...]\n"
	   "                        Enable debugging output.\n"
	   "                          Options:\n"
	   "                            all, threads, event-loop, remote\n"
	   "                          With no options, 'threads' is assumed.\n"
	   "                          Prefix an option with '-' to disable\n"
	   "                          debugging of that component.\n"
	   "  --debug-format=OPT1[,OPT2,...]\n"
	   "                        Specify extra content in debugging output.\n"
	   "                          Options:\n"
	   "                            all\n"
	   "                            none\n"
	   "                            timestamp\n"
	   "  --disable-packet=OPT1[,OPT2,...]\n"
	   "                        Disable support for RSP packets or features.\n"
	   "                          Options:\n"
	   "                            vCont, T, Tthread, qC, qfThreadInfo and \n"
	   "                            threads (disable all threading packets).\n"
	   "\n"
	   "For more information, consult the GDB manual (available as on-line \n"
	   "info or a printed manual).\n");
  if (REPORT_BUGS_TO[0] && stream == stdout)
    fprintf (stream, "Report bugs to \"%s\".\n", REPORT_BUGS_TO);
}

static void
gdbserver_show_disableable (FILE *stream)
{
  fprintf (stream, "Disableable packets:\n"
	   "  vCont       \tAll vCont packets\n"
	   "  qC          \tQuerying the current thread\n"
	   "  qfThreadInfo\tThread listing\n"
	   "  Tthread     \tPassing the thread specifier in the "
	   "T stop reply packet\n"
	   "  threads     \tAll of the above\n"
	   "  T           \tAll 'T' packets\n");
}

/* Start up the event loop.  This is the entry point to the event
   loop.  */

static void
start_event_loop ()
{
  /* Loop until there is nothing to do.  This is the entry point to
     the event loop engine.  If nothing is ready at this time, wait
     for something to happen (via wait_for_event), then process it.
     Return when there are no longer event sources to wait for.  */

  keep_processing_events = true;
  while (keep_processing_events)
    {
      /* Any events already waiting in the queue?  */
      int res = gdb_do_one_event ();

      /* Was there an error?  */
      if (res == -1)
	break;
    }

  /* We are done with the event loop.  There are no more event sources
     to listen to.  So we exit gdbserver.  */
}

static void
kill_inferior_callback (process_info *process)
{
  kill_inferior (process);
  discard_queued_stop_replies (ptid_t (process->pid));
}

/* Call this when exiting gdbserver with possible inferiors that need
   to be killed or detached from.  */

static void
detach_or_kill_for_exit (void)
{
  /* First print a list of the inferiors we will be killing/detaching.
     This is to assist the user, for example, in case the inferior unexpectedly
     dies after we exit: did we screw up or did the inferior exit on its own?
     Having this info will save some head-scratching.  */

  if (have_started_inferiors_p ())
    {
      fprintf (stderr, "Killing process(es):");

      for_each_process ([] (process_info *process) {
	if (!process->attached)
	  fprintf (stderr, " %d", process->pid);
      });

      fprintf (stderr, "\n");
    }
  if (have_attached_inferiors_p ())
    {
      fprintf (stderr, "Detaching process(es):");

      for_each_process ([] (process_info *process) {
	if (process->attached)
	  fprintf (stderr, " %d", process->pid);
      });

      fprintf (stderr, "\n");
    }

  /* Now we can kill or detach the inferiors.  */
  for_each_process ([] (process_info *process) {
    int pid = process->pid;

    if (process->attached)
      detach_inferior (process);
    else
      kill_inferior (process);

    discard_queued_stop_replies (ptid_t (pid));
  });
}

/* Value that will be passed to exit(3) when gdbserver exits.  */
static int exit_code;

/* Wrapper for detach_or_kill_for_exit that catches and prints
   errors.  */

static void
detach_or_kill_for_exit_cleanup ()
{
  try
    {
      detach_or_kill_for_exit ();
    }
  catch (const gdb_exception &exception)
    {
      fflush (stdout);
      fprintf (stderr, "Detach or kill failed: %s\n",
	       exception.what ());
      exit_code = 1;
    }
}

#if GDB_SELF_TEST

namespace selftests {

static void
test_memory_tagging_functions (void)
{
  /* Setup testing.  */
  gdb::char_vector packet;
  gdb::byte_vector tags, bv;
  std::string expected;
  packet.resize (32000);
  CORE_ADDR addr;
  size_t len;
  int type;

  /* Test parsing a qMemTags request.  */

  /* Valid request, addr, len and type updated.  */
  addr = 0xff;
  len = 255;
  type = 255;
  strcpy (packet.data (), "qMemTags:0,0:0");
  parse_fetch_memtags_request (packet.data (), &addr, &len, &type);
  SELF_CHECK (addr == 0 && len == 0 && type == 0);

  /* Valid request, addr, len and type updated.  */
  addr = 0;
  len = 0;
  type = 0;
  strcpy (packet.data (), "qMemTags:deadbeef,ff:5");
  parse_fetch_memtags_request (packet.data (), &addr, &len, &type);
  SELF_CHECK (addr == 0xdeadbeef && len == 255 && type == 5);

  /* Test creating a qMemTags reply.  */

  /* Non-empty tag data.  */
  bv.resize (0);

  for (int i = 0; i < 5; i++)
    bv.push_back (i);

  expected = "m0001020304";
  SELF_CHECK (create_fetch_memtags_reply (packet.data (), bv) == true);
  SELF_CHECK (strcmp (packet.data (), expected.c_str ()) == 0);

  /* Test parsing a QMemTags request.  */

  /* Valid request and empty tag data: addr, len, type and tags updated.  */
  addr = 0xff;
  len = 255;
  type = 255;
  tags.resize (5);
  strcpy (packet.data (), "QMemTags:0,0:0:");
  SELF_CHECK (parse_store_memtags_request (packet.data (),
					   &addr, &len, tags, &type) == true);
  SELF_CHECK (addr == 0 && len == 0 && type == 0 && tags.size () == 0);

  /* Valid request and non-empty tag data: addr, len, type
     and tags updated.  */
  addr = 0;
  len = 0;
  type = 0;
  tags.resize (0);
  strcpy (packet.data (),
	  "QMemTags:deadbeef,ff:5:0001020304");
  SELF_CHECK (parse_store_memtags_request (packet.data (), &addr, &len, tags,
					   &type) == true);
  SELF_CHECK (addr == 0xdeadbeef && len == 255 && type == 5
	      && tags.size () == 5);
}

} // namespace selftests
#endif /* GDB_SELF_TEST */

/* Main function.  This is called by the real "main" function,
   wrapped in a TRY_CATCH that handles any uncaught exceptions.  */

static void ATTRIBUTE_NORETURN
captured_main (int argc, char *argv[])
{
  int bad_attach;
  int pid;
  char *arg_end;
  const char *port = NULL;
  char **next_arg = &argv[1];
  volatile int multi_mode = 0;
  volatile int attach = 0;
  int was_running;
  bool selftest = false;
#if GDB_SELF_TEST
  std::vector<const char *> selftest_filters;

  selftests::register_test ("remote_memory_tagging",
			    selftests::test_memory_tagging_functions);
#endif

  current_directory = getcwd (NULL, 0);
  client_state &cs = get_client_state ();

  if (current_directory == NULL)
    {
      error (_("Could not find current working directory: %s"),
	     safe_strerror (errno));
    }

  while (*next_arg != NULL && **next_arg == '-')
    {
      if (strcmp (*next_arg, "--version") == 0)
	{
	  gdbserver_version ();
	  exit (0);
	}
      else if (strcmp (*next_arg, "--help") == 0)
	{
	  gdbserver_usage (stdout);
	  exit (0);
	}
      else if (strcmp (*next_arg, "--attach") == 0)
	attach = 1;
      else if (strcmp (*next_arg, "--multi") == 0)
	multi_mode = 1;
      else if (strcmp (*next_arg, "--wrapper") == 0)
	{
	  char **tmp;

	  next_arg++;

	  tmp = next_arg;
	  while (*next_arg != NULL && strcmp (*next_arg, "--") != 0)
	    {
	      wrapper_argv += *next_arg;
	      wrapper_argv += ' ';
	      next_arg++;
	    }

	  if (!wrapper_argv.empty ())
	    {
	      /* Erase the last whitespace.  */
	      wrapper_argv.erase (wrapper_argv.end () - 1);
	    }

	  if (next_arg == tmp || *next_arg == NULL)
	    {
	      gdbserver_usage (stderr);
	      exit (1);
	    }

	  /* Consume the "--".  */
	  *next_arg = NULL;
	}
      else if (startswith (*next_arg, "--debug="))
	{
	  try
	    {
	      parse_debug_options ((*next_arg) + sizeof ("--debug=") - 1);
	    }
	  catch (const gdb_exception_error &exception)
	    {
	      fflush (stdout);
	      fprintf (stderr, "gdbserver: %s\n", exception.what ());
	      exit (1);
	    }
	}
      else if (strcmp (*next_arg, "--debug") == 0)
	{
	  try
	    {
	      parse_debug_options ("");
	    }
	  catch (const gdb_exception_error &exception)
	    {
	      fflush (stdout);
	      fprintf (stderr, "gdbserver: %s\n", exception.what ());
	      exit (1);
	    }
	}
      else if (startswith (*next_arg, "--debug-format="))
	{
	  std::string error_msg
	    = parse_debug_format_options ((*next_arg)
					  + sizeof ("--debug-format=") - 1, 0);

	  if (!error_msg.empty ())
	    {
	      fprintf (stderr, "%s", error_msg.c_str ());
	      exit (1);
	    }
	}
      else if (startswith (*next_arg, "--debug-file="))
	debug_set_output ((*next_arg) + sizeof ("--debug-file=") -1);
      else if (strcmp (*next_arg, "--disable-packet") == 0)
	{
	  gdbserver_show_disableable (stdout);
	  exit (0);
	}
      else if (startswith (*next_arg, "--disable-packet="))
	{
	  char *packets = *next_arg += sizeof ("--disable-packet=") - 1;
	  char *saveptr;
	  for (char *tok = strtok_r (packets, ",", &saveptr);
	       tok != NULL;
	       tok = strtok_r (NULL, ",", &saveptr))
	    {
	      if (strcmp ("vCont", tok) == 0)
		disable_packet_vCont = true;
	      else if (strcmp ("Tthread", tok) == 0)
		disable_packet_Tthread = true;
	      else if (strcmp ("qC", tok) == 0)
		disable_packet_qC = true;
	      else if (strcmp ("qfThreadInfo", tok) == 0)
		disable_packet_qfThreadInfo = true;
	      else if (strcmp ("T", tok) == 0)
		disable_packet_T = true;
	      else if (strcmp ("threads", tok) == 0)
		{
		  disable_packet_vCont = true;
		  disable_packet_Tthread = true;
		  disable_packet_qC = true;
		  disable_packet_qfThreadInfo = true;
		}
	      else
		{
		  fprintf (stderr, "Don't know how to disable \"%s\".\n\n",
			   tok);
		  gdbserver_show_disableable (stderr);
		  exit (1);
		}
	    }
	}
      else if (strcmp (*next_arg, "-") == 0)
	{
	  /* "-" specifies a stdio connection and is a form of port
	     specification.  */
	  port = STDIO_CONNECTION_NAME;
	  next_arg++;
	  break;
	}
      else if (strcmp (*next_arg, "--disable-randomization") == 0)
	cs.disable_randomization = 1;
      else if (strcmp (*next_arg, "--no-disable-randomization") == 0)
	cs.disable_randomization = 0;
      else if (strcmp (*next_arg, "--startup-with-shell") == 0)
	startup_with_shell = true;
      else if (strcmp (*next_arg, "--no-startup-with-shell") == 0)
	startup_with_shell = false;
      else if (strcmp (*next_arg, "--once") == 0)
	run_once = true;
      else if (strcmp (*next_arg, "--selftest") == 0)
	selftest = true;
      else if (startswith (*next_arg, "--selftest="))
	{
	  selftest = true;

#if GDB_SELF_TEST
	  const char *filter = *next_arg + strlen ("--selftest=");
	  if (*filter == '\0')
	    {
	      fprintf (stderr, _("Error: selftest filter is empty.\n"));
	      exit (1);
	    }

	  selftest_filters.push_back (filter);
#endif
	}
      else
	{
	  fprintf (stderr, "Unknown argument: %s\n", *next_arg);
	  exit (1);
	}

      next_arg++;
      continue;
    }

  if (port == NULL)
    {
      port = *next_arg;
      next_arg++;
    }
  if ((port == NULL || (!attach && !multi_mode && *next_arg == NULL))
       && !selftest)
    {
      gdbserver_usage (stderr);
      exit (1);
    }

  /* Remember stdio descriptors.  LISTEN_DESC must not be listed, it will be
     opened by remote_prepare.  */
  notice_open_fds ();

  save_original_signals_state (false);

  /* We need to know whether the remote connection is stdio before
     starting the inferior.  Inferiors created in this scenario have
     stdin,stdout redirected.  So do this here before we call
     start_inferior.  */
  if (port != NULL)
    remote_prepare (port);

  bad_attach = 0;
  pid = 0;

  /* --attach used to come after PORT, so allow it there for
       compatibility.  */
  if (*next_arg != NULL && strcmp (*next_arg, "--attach") == 0)
    {
      attach = 1;
      next_arg++;
    }

  if (attach
      && (*next_arg == NULL
	  || (*next_arg)[0] == '\0'
	  || (pid = strtoul (*next_arg, &arg_end, 0)) == 0
	  || *arg_end != '\0'
	  || next_arg[1] != NULL))
    bad_attach = 1;

  if (bad_attach)
    {
      gdbserver_usage (stderr);
      exit (1);
    }

  /* Gather information about the environment.  */
  our_environ = gdb_environ::from_host_environ ();

  initialize_async_io ();
  initialize_low ();
  have_job_control ();
  if (target_supports_tracepoints ())
    initialize_tracepoint ();

  mem_buf = (unsigned char *) xmalloc (PBUFSIZ);

  if (selftest)
    {
#if GDB_SELF_TEST
      selftests::run_tests (selftest_filters);
#else
      printf (_("Selftests have been disabled for this build.\n"));
#endif
      throw_quit ("Quit");
    }

  if (pid == 0 && *next_arg != NULL)
    {
      int i, n;

      n = argc - (next_arg - argv);
      program_path.set (next_arg[0]);
      for (i = 1; i < n; i++)
	program_args.push_back (xstrdup (next_arg[i]));

      /* Wait till we are at first instruction in program.  */
      target_create_inferior (program_path.get (), program_args);

      /* We are now (hopefully) stopped at the first instruction of
	 the target process.  This assumes that the target process was
	 successfully created.  */
    }
  else if (pid != 0)
    {
      if (attach_inferior (pid) == -1)
	error ("Attaching not supported on this target");

      /* Otherwise succeeded.  */
    }
  else
    {
      cs.last_status.set_exited (0);
      cs.last_ptid = minus_one_ptid;
    }

  SCOPE_EXIT { detach_or_kill_for_exit_cleanup (); };

  /* Don't report shared library events on the initial connection,
     even if some libraries are preloaded.  Avoids the "stopped by
     shared library event" notice on gdb side.  */
  if (current_thread != nullptr)
    current_process ()->dlls_changed = false;

  if (cs.last_status.kind () == TARGET_WAITKIND_EXITED
      || cs.last_status.kind () == TARGET_WAITKIND_SIGNALLED)
    was_running = 0;
  else
    was_running = 1;

  if (!was_running && !multi_mode)
    error ("No program to debug");

  while (1)
    {
      cs.noack_mode = 0;
      cs.multi_process = 0;
      cs.report_fork_events = 0;
      cs.report_vfork_events = 0;
      cs.report_exec_events = 0;
      /* Be sure we're out of tfind mode.  */
      cs.current_traceframe = -1;
      cs.cont_thread = null_ptid;
      cs.swbreak_feature = 0;
      cs.hwbreak_feature = 0;
      cs.vCont_supported = 0;
      cs.memory_tagging_feature = false;

      remote_open (port);

      try
	{
	  /* Wait for events.  This will return when all event sources
	     are removed from the event loop.  */
	  start_event_loop ();

	  /* If an exit was requested (using the "monitor exit"
	     command), terminate now.  */
	  if (exit_requested)
	    throw_quit ("Quit");

	  /* The only other way to get here is for getpkt to fail:

	      - If --once was specified, we're done.

	      - If not in extended-remote mode, and we're no longer
		debugging anything, simply exit: GDB has disconnected
		after processing the last process exit.

	      - Otherwise, close the connection and reopen it at the
		top of the loop.  */
	  if (run_once || (!extended_protocol && !target_running ()))
	    throw_quit ("Quit");

	  fprintf (stderr,
		   "Remote side has terminated connection.  "
		   "GDBserver will reopen the connection.\n");

	  /* Get rid of any pending statuses.  An eventual reconnection
	     (by the same GDB instance or another) will refresh all its
	     state from scratch.  */
	  discard_queued_stop_replies (minus_one_ptid);
	  for_each_thread ([] (thread_info *thread)
	    {
	      thread->status_pending_p = 0;
	    });

	  if (tracing)
	    {
	      if (disconnected_tracing)
		{
		  /* Try to enable non-stop/async mode, so we we can
		     both wait for an async socket accept, and handle
		     async target events simultaneously.  There's also
		     no point either in having the target always stop
		     all threads, when we're going to pass signals
		     down without informing GDB.  */
		  if (!non_stop)
		    {
		      if (the_target->start_non_stop (true))
			non_stop = 1;

		      /* Detaching implicitly resumes all threads;
			 simply disconnecting does not.  */
		    }
		}
	      else
		{
		  fprintf (stderr,
			   "Disconnected tracing disabled; "
			   "stopping trace run.\n");
		  stop_tracing ();
		}
	    }
	}
      catch (const gdb_exception_error &exception)
	{
	  fflush (stdout);
	  fprintf (stderr, "gdbserver: %s\n", exception.what ());

	  if (response_needed)
	    {
	      write_enn (cs.own_buf);
	      putpkt (cs.own_buf);
	    }

	  if (run_once)
	    throw_quit ("Quit");
	}
    }
}

/* Main function.  */

int
main (int argc, char *argv[])
{
  setlocale (LC_CTYPE, "");

  try
    {
      captured_main (argc, argv);
    }
  catch (const gdb_exception &exception)
    {
      if (exception.reason == RETURN_ERROR)
	{
	  fflush (stdout);
	  fprintf (stderr, "%s\n", exception.what ());
	  fprintf (stderr, "Exiting\n");
	  exit_code = 1;
	}

      exit (exit_code);
    }

  gdb_assert_not_reached ("captured_main should never return");
}

/* Process options coming from Z packets for a breakpoint.  PACKET is
   the packet buffer.  *PACKET is updated to point to the first char
   after the last processed option.  */

static void
process_point_options (struct gdb_breakpoint *bp, const char **packet)
{
  const char *dataptr = *packet;
  int persist;

  /* Check if data has the correct format.  */
  if (*dataptr != ';')
    return;

  dataptr++;

  while (*dataptr)
    {
      if (*dataptr == ';')
	++dataptr;

      if (*dataptr == 'X')
	{
	  /* Conditional expression.  */
	  threads_debug_printf ("Found breakpoint condition.");
	  if (!add_breakpoint_condition (bp, &dataptr))
	    dataptr = strchrnul (dataptr, ';');
	}
      else if (startswith (dataptr, "cmds:"))
	{
	  dataptr += strlen ("cmds:");
	  threads_debug_printf ("Found breakpoint commands %s.", dataptr);
	  persist = (*dataptr == '1');
	  dataptr += 2;
	  if (add_breakpoint_commands (bp, &dataptr, persist))
	    dataptr = strchrnul (dataptr, ';');
	}
      else
	{
	  fprintf (stderr, "Unknown token %c, ignoring.\n",
		   *dataptr);
	  /* Skip tokens until we find one that we recognize.  */
	  dataptr = strchrnul (dataptr, ';');
	}
    }
  *packet = dataptr;
}

/* Event loop callback that handles a serial event.  The first byte in
   the serial buffer gets us here.  We expect characters to arrive at
   a brisk pace, so we read the rest of the packet with a blocking
   getpkt call.  */

static int
process_serial_event (void)
{
  client_state &cs = get_client_state ();
  int signal;
  unsigned int len;
  CORE_ADDR mem_addr;
  unsigned char sig;
  int packet_len;
  int new_packet_len = -1;

  disable_async_io ();

  response_needed = false;
  packet_len = getpkt (cs.own_buf);
  if (packet_len <= 0)
    {
      remote_close ();
      /* Force an event loop break.  */
      return -1;
    }
  response_needed = true;

  char ch = cs.own_buf[0];
  switch (ch)
    {
    case 'q':
      handle_query (cs.own_buf, packet_len, &new_packet_len);
      break;
    case 'Q':
      handle_general_set (cs.own_buf);
      break;
    case 'D':
      handle_detach (cs.own_buf);
      break;
    case '!':
      extended_protocol = true;
      write_ok (cs.own_buf);
      break;
    case '?':
      handle_status (cs.own_buf);
      break;
    case 'H':
      if (cs.own_buf[1] == 'c' || cs.own_buf[1] == 'g' || cs.own_buf[1] == 's')
	{
	  require_running_or_break (cs.own_buf);

	  ptid_t thread_id = read_ptid (&cs.own_buf[2], NULL);

	  if (thread_id == null_ptid || thread_id == minus_one_ptid)
	    thread_id = null_ptid;
	  else if (thread_id.is_pid ())
	    {
	      /* The ptid represents a pid.  */
	      thread_info *thread = find_any_thread_of_pid (thread_id.pid ());

	      if (thread == NULL)
		{
		  write_enn (cs.own_buf);
		  break;
		}

	      thread_id = thread->id;
	    }
	  else
	    {
	      /* The ptid represents a lwp/tid.  */
	      if (find_thread_ptid (thread_id) == NULL)
		{
		  write_enn (cs.own_buf);
		  break;
		}
	    }

	  if (cs.own_buf[1] == 'g')
	    {
	      if (thread_id == null_ptid)
		{
		  /* GDB is telling us to choose any thread.  Check if
		     the currently selected thread is still valid. If
		     it is not, select the first available.  */
		  thread_info *thread = find_thread_ptid (cs.general_thread);
		  if (thread == NULL)
		    thread = get_first_thread ();
		  thread_id = thread->id;
		}

	      cs.general_thread = thread_id;
	      set_desired_thread ();
	      gdb_assert (current_thread != NULL);
	    }
	  else if (cs.own_buf[1] == 'c')
	    cs.cont_thread = thread_id;

	  write_ok (cs.own_buf);
	}
      else
	{
	  /* Silently ignore it so that gdb can extend the protocol
	     without compatibility headaches.  */
	  cs.own_buf[0] = '\0';
	}
      break;
    case 'g':
      require_running_or_break (cs.own_buf);
      if (cs.current_traceframe >= 0)
	{
	  struct regcache *regcache
	    = new_register_cache (current_target_desc ());

	  if (fetch_traceframe_registers (cs.current_traceframe,
					  regcache, -1) == 0)
	    registers_to_string (regcache, cs.own_buf);
	  else
	    write_enn (cs.own_buf);
	  free_register_cache (regcache);
	}
      else
	{
	  struct regcache *regcache;

	  if (!set_desired_thread ())
	    write_enn (cs.own_buf);
	  else
	    {
	      regcache = get_thread_regcache (current_thread, 1);
	      registers_to_string (regcache, cs.own_buf);
	    }
	}
      break;
    case 'G':
      require_running_or_break (cs.own_buf);
      if (cs.current_traceframe >= 0)
	write_enn (cs.own_buf);
      else
	{
	  struct regcache *regcache;

	  if (!set_desired_thread ())
	    write_enn (cs.own_buf);
	  else
	    {
	      regcache = get_thread_regcache (current_thread, 1);
	      registers_from_string (regcache, &cs.own_buf[1]);
	      write_ok (cs.own_buf);
	    }
	}
      break;
    case 'm':
      {
	require_running_or_break (cs.own_buf);
	decode_m_packet (&cs.own_buf[1], &mem_addr, &len);
	int res = gdb_read_memory (mem_addr, mem_buf, len);
	if (res < 0)
	  write_enn (cs.own_buf);
	else
	  bin2hex (mem_buf, cs.own_buf, res);
      }
      break;
    case 'M':
      require_running_or_break (cs.own_buf);
      decode_M_packet (&cs.own_buf[1], &mem_addr, &len, &mem_buf);
      if (gdb_write_memory (mem_addr, mem_buf, len) == 0)
	write_ok (cs.own_buf);
      else
	write_enn (cs.own_buf);
      break;
    case 'X':
      require_running_or_break (cs.own_buf);
      if (decode_X_packet (&cs.own_buf[1], packet_len - 1,
			   &mem_addr, &len, &mem_buf) < 0
	  || gdb_write_memory (mem_addr, mem_buf, len) != 0)
	write_enn (cs.own_buf);
      else
	write_ok (cs.own_buf);
      break;
    case 'C':
      require_running_or_break (cs.own_buf);
      hex2bin (cs.own_buf + 1, &sig, 1);
      if (gdb_signal_to_host_p ((enum gdb_signal) sig))
	signal = gdb_signal_to_host ((enum gdb_signal) sig);
      else
	signal = 0;
      myresume (cs.own_buf, 0, signal);
      break;
    case 'S':
      require_running_or_break (cs.own_buf);
      hex2bin (cs.own_buf + 1, &sig, 1);
      if (gdb_signal_to_host_p ((enum gdb_signal) sig))
	signal = gdb_signal_to_host ((enum gdb_signal) sig);
      else
	signal = 0;
      myresume (cs.own_buf, 1, signal);
      break;
    case 'c':
      require_running_or_break (cs.own_buf);
      signal = 0;
      myresume (cs.own_buf, 0, signal);
      break;
    case 's':
      require_running_or_break (cs.own_buf);
      signal = 0;
      myresume (cs.own_buf, 1, signal);
      break;
    case 'Z':  /* insert_ ... */
      /* Fallthrough.  */
    case 'z':  /* remove_ ... */
      {
	char *dataptr;
	ULONGEST addr;
	int kind;
	char type = cs.own_buf[1];
	int res;
	const int insert = ch == 'Z';
	const char *p = &cs.own_buf[3];

	p = unpack_varlen_hex (p, &addr);
	kind = strtol (p + 1, &dataptr, 16);

	if (insert)
	  {
	    struct gdb_breakpoint *bp;

	    bp = set_gdb_breakpoint (type, addr, kind, &res);
	    if (bp != NULL)
	      {
		res = 0;

		/* GDB may have sent us a list of *point parameters to
		   be evaluated on the target's side.  Read such list
		   here.  If we already have a list of parameters, GDB
		   is telling us to drop that list and use this one
		   instead.  */
		clear_breakpoint_conditions_and_commands (bp);
		const char *options = dataptr;
		process_point_options (bp, &options);
	      }
	  }
	else
	  res = delete_gdb_breakpoint (type, addr, kind);

	if (res == 0)
	  write_ok (cs.own_buf);
	else if (res == 1)
	  /* Unsupported.  */
	  cs.own_buf[0] = '\0';
	else
	  write_enn (cs.own_buf);
	break;
      }
    case 'k':
      response_needed = false;
      if (!target_running ())
	/* The packet we received doesn't make sense - but we can't
	   reply to it, either.  */
	return 0;

      fprintf (stderr, "Killing all inferiors\n");

      for_each_process (kill_inferior_callback);

      /* When using the extended protocol, we wait with no program
	 running.  The traditional protocol will exit instead.  */
      if (extended_protocol)
	{
	  cs.last_status.set_exited (GDB_SIGNAL_KILL);
	  return 0;
	}
      else
	exit (0);

    case 'T':
      {
	require_running_or_break (cs.own_buf);

	ptid_t thread_id = read_ptid (&cs.own_buf[1], NULL);
	if (find_thread_ptid (thread_id) == NULL)
	  {
	    write_enn (cs.own_buf);
	    break;
	  }

	if (mythread_alive (thread_id))
	  write_ok (cs.own_buf);
	else
	  write_enn (cs.own_buf);
      }
      break;
    case 'R':
      response_needed = false;

      /* Restarting the inferior is only supported in the extended
	 protocol.  */
      if (extended_protocol)
	{
	  if (target_running ())
	    for_each_process (kill_inferior_callback);

	  fprintf (stderr, "GDBserver restarting\n");

	  /* Wait till we are at 1st instruction in prog.  */
	  if (program_path.get () != NULL)
	    {
	      target_create_inferior (program_path.get (), program_args);

	      if (cs.last_status.kind () == TARGET_WAITKIND_STOPPED)
		{
		  /* Stopped at the first instruction of the target
		     process.  */
		  cs.general_thread = cs.last_ptid;
		}
	      else
		{
		  /* Something went wrong.  */
		  cs.general_thread = null_ptid;
		}
	    }
	  else
	    {
	      cs.last_status.set_exited (GDB_SIGNAL_KILL);
	    }
	  return 0;
	}
      else
	{
	  /* It is a request we don't understand.  Respond with an
	     empty packet so that gdb knows that we don't support this
	     request.  */
	  cs.own_buf[0] = '\0';
	  break;
	}
    case 'v':
      /* Extended (long) request.  */
      handle_v_requests (cs.own_buf, packet_len, &new_packet_len);
      break;

    default:
      /* It is a request we don't understand.  Respond with an empty
	 packet so that gdb knows that we don't support this
	 request.  */
      cs.own_buf[0] = '\0';
      break;
    }

  if (new_packet_len != -1)
    putpkt_binary (cs.own_buf, new_packet_len);
  else
    putpkt (cs.own_buf);

  response_needed = false;

  if (exit_requested)
    return -1;

  return 0;
}

/* Event-loop callback for serial events.  */

void
handle_serial_event (int err, gdb_client_data client_data)
{
  threads_debug_printf ("handling possible serial event");

  /* Really handle it.  */
  if (process_serial_event () < 0)
    {
      keep_processing_events = false;
      return;
    }

  /* Be sure to not change the selected thread behind GDB's back.
     Important in the non-stop mode asynchronous protocol.  */
  set_desired_thread ();
}

/* Push a stop notification on the notification queue.  */

static void
push_stop_notification (ptid_t ptid, const target_waitstatus &status)
{
  struct vstop_notif *vstop_notif = new struct vstop_notif;

  vstop_notif->status = status;
  vstop_notif->ptid = ptid;
  /* Push Stop notification.  */
  notif_push (&notif_stop, vstop_notif);
}

/* Event-loop callback for target events.  */

void
handle_target_event (int err, gdb_client_data client_data)
{
  client_state &cs = get_client_state ();
  threads_debug_printf ("handling possible target event");

  cs.last_ptid = mywait (minus_one_ptid, &cs.last_status,
		      TARGET_WNOHANG, 1);

  if (cs.last_status.kind () == TARGET_WAITKIND_NO_RESUMED)
    {
      if (gdb_connected () && report_no_resumed)
	push_stop_notification (null_ptid, cs.last_status);
    }
  else if (cs.last_status.kind () != TARGET_WAITKIND_IGNORE)
    {
      int pid = cs.last_ptid.pid ();
      struct process_info *process = find_process_pid (pid);
      int forward_event = !gdb_connected () || process->gdb_detached;

      if (cs.last_status.kind () == TARGET_WAITKIND_EXITED
	  || cs.last_status.kind () == TARGET_WAITKIND_SIGNALLED)
	{
	  mark_breakpoints_out (process);
	  target_mourn_inferior (cs.last_ptid);
	}
      else if (cs.last_status.kind () == TARGET_WAITKIND_THREAD_EXITED)
	;
      else
	{
	  /* We're reporting this thread as stopped.  Update its
	     "want-stopped" state to what the client wants, until it
	     gets a new resume action.  */
	  current_thread->last_resume_kind = resume_stop;
	  current_thread->last_status = cs.last_status;
	}

      if (forward_event)
	{
	  if (!target_running ())
	    {
	      /* The last process exited.  We're done.  */
	      exit (0);
	    }

	  if (cs.last_status.kind () == TARGET_WAITKIND_EXITED
	      || cs.last_status.kind () == TARGET_WAITKIND_SIGNALLED
	      || cs.last_status.kind () == TARGET_WAITKIND_THREAD_EXITED)
	    ;
	  else
	    {
	      /* A thread stopped with a signal, but gdb isn't
		 connected to handle it.  Pass it down to the
		 inferior, as if it wasn't being traced.  */
	      enum gdb_signal signal;

	      threads_debug_printf ("GDB not connected; forwarding event %d for"
				    " [%s]",
				    (int) cs.last_status.kind (),
				    target_pid_to_str (cs.last_ptid).c_str ());

	      if (cs.last_status.kind () == TARGET_WAITKIND_STOPPED)
		signal = cs.last_status.sig ();
	      else
		signal = GDB_SIGNAL_0;
	      target_continue (cs.last_ptid, signal);
	    }
	}
      else
	{
	  push_stop_notification (cs.last_ptid, cs.last_status);

	  if (cs.last_status.kind () == TARGET_WAITKIND_THREAD_EXITED
	      && !target_any_resumed ())
	    {
	      target_waitstatus ws;
	      ws.set_no_resumed ();
	      push_stop_notification (null_ptid, ws);
	    }
	}
    }

  /* Be sure to not change the selected thread behind GDB's back.
     Important in the non-stop mode asynchronous protocol.  */
  set_desired_thread ();
}

/* See gdbsupport/event-loop.h.  */

int
invoke_async_signal_handlers ()
{
  return 0;
}

/* See gdbsupport/event-loop.h.  */

int
check_async_event_handlers ()
{
  return 0;
}

/* See gdbsupport/errors.h  */

void
flush_streams ()
{
  fflush (stdout);
  fflush (stderr);
}

/* See gdbsupport/gdb_select.h.  */

int
gdb_select (int n, fd_set *readfds, fd_set *writefds,
	    fd_set *exceptfds, struct timeval *timeout)
{
  return select (n, readfds, writefds, exceptfds, timeout);
}

#if GDB_SELF_TEST
namespace selftests
{

void
reset ()
{}

} // namespace selftests
#endif /* GDB_SELF_TEST */
