/* Darwin support for GDB, the GNU debugger.
   Copyright (C) 2008-2024 Free Software Foundation, Inc.

   Contributed by AdaCore.

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
#include "top.h"
#include "inferior.h"
#include "target.h"
#include "symfile.h"
#include "symtab.h"
#include "objfiles.h"
#include "gdbcmd.h"
#include "gdbcore.h"
#include "gdbthread.h"
#include "regcache.h"
#include "event-top.h"
#include "inf-loop.h"
#include <sys/stat.h>
#include "inf-child.h"
#include "value.h"
#include "arch-utils.h"
#include "bfd.h"
#include "bfd/mach-o.h"
#include "gdbarch.h"

#include <copyfile.h>
#include <sys/ptrace.h>
#include <sys/signal.h>
#include <setjmp.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>
#include <sys/sysctl.h>
#include <sys/proc.h>
#include <libproc.h>
#include <sys/syscall.h>
#include <spawn.h>

#include <mach/mach_error.h>
#include <mach/mach_vm.h>
#include <mach/mach_init.h>
#include <mach/vm_map.h>
#include <mach/task.h>
#include <mach/mach_port.h>
#include <mach/thread_act.h>
#include <mach/port.h>

#include "darwin-nat.h"
#include "filenames.h"
#include "gdbsupport/filestuff.h"
#include "gdbsupport/gdb_unlinker.h"
#include "gdbsupport/pathstuff.h"
#include "gdbsupport/scoped_fd.h"
#include "nat/fork-inferior.h"

/* Quick overview.
   Darwin kernel is Mach + BSD derived kernel.  Note that they share the
   same memory space and are linked together (ie there is no micro-kernel).

   Although ptrace(2) is available on Darwin, it is not complete.  We have
   to use Mach calls to read and write memory and to modify registers.  We
   also use Mach to get inferior faults.  As we cannot use select(2) or
   signals with Mach port (the Mach communication channel), signals are
   reported to gdb as an exception.  Furthermore we detect death of the
   inferior through a Mach notification message.  This way we only wait
   on Mach ports.

   Some Mach documentation is available for Apple xnu source package or
   from the web.  */


#define PTRACE(CMD, PID, ADDR, SIG) \
 darwin_ptrace(#CMD, CMD, (PID), (ADDR), (SIG))

static void darwin_ptrace_me (void);

static void darwin_encode_reply (mig_reply_error_t *reply,
				 mach_msg_header_t *hdr, integer_t code);

static void darwin_setup_request_notification (struct inferior *inf);
static void darwin_deallocate_exception_ports (darwin_inferior *inf);
static void darwin_setup_exceptions (struct inferior *inf);
static void darwin_deallocate_threads (struct inferior *inf);

/* Task identifier of gdb.  */
static task_t gdb_task;

/* A copy of mach_host_self ().  */
mach_port_t darwin_host_self;

/* Exception port.  */
mach_port_t darwin_ex_port;

/* Port set, to wait for answer on all ports.  */
mach_port_t darwin_port_set;

/* Page size.  */
static vm_size_t mach_page_size;

/* If Set, catch all mach exceptions (before they are converted to signals
   by the kernel).  */
static bool enable_mach_exceptions;

/* Inferior that should report a fake stop event.  */
static struct inferior *darwin_inf_fake_stop;

/* If non-NULL, the shell we actually invoke.  See maybe_cache_shell
   for details.  */
static const char *copied_shell;

#define PAGE_TRUNC(x) ((x) & ~(mach_page_size - 1))
#define PAGE_ROUND(x) PAGE_TRUNC((x) + mach_page_size - 1)

/* This controls output of inferior debugging.  */
static unsigned int darwin_debug_flag = 0;

/* Create a __TEXT __info_plist section in the executable so that gdb could
   be signed.  This is required to get an authorization for task_for_pid.

   Once gdb is built, you must codesign it with any system-trusted signing
   authority.  See taskgated(8) for details.  */
static const unsigned char info_plist[]
__attribute__ ((section ("__TEXT,__info_plist"),used)) =
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\""
  " \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
  "<plist version=\"1.0\">\n"
  "<dict>\n"
  "  <key>CFBundleIdentifier</key>\n"
  "  <string>org.gnu.gdb</string>\n"
  "  <key>CFBundleName</key>\n"
  "  <string>gdb</string>\n"
  "  <key>CFBundleVersion</key>\n"
  "  <string>1.0</string>\n"
  "  <key>SecTaskAccess</key>\n"
  "  <array>\n"
  "    <string>allowed</string>\n"
  "    <string>debug</string>\n"
  "  </array>\n"
  "</dict>\n"
  "</plist>\n";

static void inferior_debug (int level, const char *fmt, ...)
  ATTRIBUTE_PRINTF (2, 3);

static void
inferior_debug (int level, const char *fmt, ...)
{
  va_list ap;

  if (darwin_debug_flag < level)
    return;

  va_start (ap, fmt);
  gdb_printf (gdb_stdlog, _("[%d inferior]: "), getpid ());
  gdb_vprintf (gdb_stdlog, fmt, ap);
  va_end (ap);
}

void
mach_check_error (kern_return_t ret, const char *file,
		  unsigned int line, const char *func)
{
  if (ret == KERN_SUCCESS)
    return;
  if (func == NULL)
    func = _("[UNKNOWN]");

  warning (_("Mach error at \"%s:%u\" in function \"%s\": %s (0x%lx)"),
	   file, line, func, mach_error_string (ret), (unsigned long) ret);
}

static const char *
unparse_exception_type (unsigned int i)
{
  static char unknown_exception_buf[32];

  switch (i)
    {
    case EXC_BAD_ACCESS:
      return "EXC_BAD_ACCESS";
    case EXC_BAD_INSTRUCTION:
      return "EXC_BAD_INSTRUCTION";
    case EXC_ARITHMETIC:
      return "EXC_ARITHMETIC";
    case EXC_EMULATION:
      return "EXC_EMULATION";
    case EXC_SOFTWARE:
      return "EXC_SOFTWARE";
    case EXC_BREAKPOINT:
      return "EXC_BREAKPOINT";
    case EXC_SYSCALL:
      return "EXC_SYSCALL";
    case EXC_MACH_SYSCALL:
      return "EXC_MACH_SYSCALL";
    case EXC_RPC_ALERT:
      return "EXC_RPC_ALERT";
    case EXC_CRASH:
      return "EXC_CRASH";
    default:
      snprintf (unknown_exception_buf, 32, _("unknown (%d)"), i);
      return unknown_exception_buf;
    }
}

/* Set errno to zero, and then call ptrace with the given arguments.
   If inferior debugging traces are on, then also print a debug
   trace.

   The returned value is the same as the value returned by ptrace,
   except in the case where that value is -1 but errno is zero.
   This case is documented to be a non-error situation, so we
   return zero in that case. */

static int
darwin_ptrace (const char *name,
	       int request, int pid, caddr_t arg3, int arg4)
{
  int ret;

  errno = 0;
  ret = ptrace (request, pid, arg3, arg4);
  if (ret == -1 && errno == 0)
    ret = 0;

  inferior_debug (4, _("ptrace (%s, %d, 0x%lx, %d): %d (%s)\n"),
		  name, pid, (unsigned long) arg3, arg4, ret,
		  (ret != 0) ? safe_strerror (errno) : _("no error"));
  return ret;
}

static int
cmp_thread_t (const void *l, const void *r)
{
  thread_t tl = *(const thread_t *)l;
  thread_t tr = *(const thread_t *)r;
  return (int)(tl - tr);
}

void
darwin_nat_target::check_new_threads (inferior *inf)
{
  kern_return_t kret;
  thread_array_t thread_list;
  unsigned int new_nbr;
  unsigned int old_nbr;
  unsigned int new_ix, old_ix;
  darwin_inferior *darwin_inf = get_darwin_inferior (inf);
  std::vector<darwin_thread_t *> new_thread_vec;

  if (darwin_inf == nullptr)
    return;

  /* Get list of threads.  */
  kret = task_threads (darwin_inf->task, &thread_list, &new_nbr);
  MACH_CHECK_ERROR (kret);
  if (kret != KERN_SUCCESS)
    return;

  /* Sort the list.  */
  if (new_nbr > 1)
    qsort (thread_list, new_nbr, sizeof (thread_t), cmp_thread_t);

  old_nbr = darwin_inf->threads.size ();

  /* Quick check for no changes.  */
  if (old_nbr == new_nbr)
    {
      size_t i;

      for (i = 0; i < new_nbr; i++)
	if (thread_list[i] != darwin_inf->threads[i]->gdb_port)
	  break;
      if (i == new_nbr)
	{
	  /* Deallocate ports.  */
	  for (i = 0; i < new_nbr; i++)
	    {
	      kret = mach_port_deallocate (mach_task_self (), thread_list[i]);
	      MACH_CHECK_ERROR (kret);
	    }

	  /* Deallocate the buffer.  */
	  kret = vm_deallocate (gdb_task, (vm_address_t) thread_list,
				new_nbr * sizeof (int));
	  MACH_CHECK_ERROR (kret);

	  return;
	}
    }

  /* Full handling: detect new threads, remove dead threads.  */

  new_thread_vec.reserve (new_nbr);

  for (new_ix = 0, old_ix = 0; new_ix < new_nbr || old_ix < old_nbr;)
    {
      thread_t new_id = (new_ix < new_nbr) ? thread_list[new_ix] : THREAD_NULL;
      darwin_thread_t *old
	= (old_ix < old_nbr) ? darwin_inf->threads[old_ix] : NULL;
      thread_t old_id = old != NULL ? old->gdb_port : THREAD_NULL;

      inferior_debug
	(12, _(" new_ix:%d/%d, old_ix:%d/%d, new_id:0x%x old_id:0x%x\n"),
	 new_ix, new_nbr, old_ix, old_nbr, new_id, old_id);

      if (old_id == new_id)
	{
	  /* Thread still exist.  */
	  new_thread_vec.push_back (old);
	  new_ix++;
	  old_ix++;

	  /* Deallocate the port.  */
	  kret = mach_port_deallocate (gdb_task, new_id);
	  MACH_CHECK_ERROR (kret);

	  continue;
	}
      if (new_ix < new_nbr && new_id == MACH_PORT_DEAD)
	{
	  /* Ignore dead ports.
	     In some weird cases, we might get dead ports.  They should
	     correspond to dead thread so they could safely be ignored.  */
	  new_ix++;
	  continue;
	}
      if (new_ix < new_nbr && (old_ix == old_nbr || new_id < old_id))
	{
	  /* A thread was created.  */
	  darwin_thread_info *pti = new darwin_thread_info;

	  pti->gdb_port = new_id;
	  pti->msg_state = DARWIN_RUNNING;

	  /* Add the new thread.  */
	  add_thread_with_info (this, ptid_t (inf->pid, 0, new_id),
				private_thread_info_up (pti));
	  new_thread_vec.push_back (pti);
	  new_ix++;
	  continue;
	}
      if (old_ix < old_nbr && (new_ix == new_nbr || new_id > old_id))
	{
	  /* A thread was removed.  */
	  struct thread_info *thr
	    = this->find_thread (ptid_t (inf->pid, 0, old_id));
	  delete_thread (thr);
	  kret = mach_port_deallocate (gdb_task, old_id);
	  MACH_CHECK_ERROR (kret);
	  old_ix++;
	  continue;
	}
      gdb_assert_not_reached ("unexpected thread case");
    }

  darwin_inf->threads = std::move (new_thread_vec);

  /* Deallocate the buffer.  */
  kret = vm_deallocate (gdb_task, (vm_address_t) thread_list,
			new_nbr * sizeof (int));
  MACH_CHECK_ERROR (kret);
}

/* Return an inferior by task port.  */
static struct inferior *
darwin_find_inferior_by_task (task_t port)
{
  for (inferior *inf : all_inferiors ())
    {
      darwin_inferior *priv = get_darwin_inferior (inf);

      if (priv != nullptr && priv->task == port)
	return inf;
    }
  return nullptr;
}

/* Return an inferior by pid port.  */
static struct inferior *
darwin_find_inferior_by_pid (int pid)
{
  for (inferior *inf : all_inferiors ())
    {
      if (inf->pid == pid)
	return inf;
    }
  return nullptr;
}

/* Return a thread by port.  */
static darwin_thread_t *
darwin_find_thread (struct inferior *inf, thread_t thread)
{
  darwin_inferior *priv = get_darwin_inferior (inf);

  if (priv != nullptr)
    for (darwin_thread_t *t : priv->threads)
      {
	if (t->gdb_port == thread)
	  return t;
      }

  return NULL;
}

/* Suspend (ie stop) an inferior at Mach level.  */

static void
darwin_suspend_inferior (struct inferior *inf)
{
  darwin_inferior *priv = get_darwin_inferior (inf);

  if (priv != nullptr && !priv->suspended)
    {
      kern_return_t kret;

      kret = task_suspend (priv->task);
      MACH_CHECK_ERROR (kret);

      priv->suspended = 1;
    }
}

/* Resume an inferior at Mach level.  */

static void
darwin_resume_inferior (struct inferior *inf)
{
  darwin_inferior *priv = get_darwin_inferior (inf);

  if (priv != nullptr && priv->suspended)
    {
      kern_return_t kret;

      kret = task_resume (priv->task);
      MACH_CHECK_ERROR (kret);

      priv->suspended = 0;
    }
}

static void
darwin_dump_message (mach_msg_header_t *hdr, int disp_body)
{
  gdb_printf (gdb_stdlog,
	      _("message header:\n"));
  gdb_printf (gdb_stdlog,
	      _(" bits: 0x%x\n"), hdr->msgh_bits);
  gdb_printf (gdb_stdlog,
	      _(" size: 0x%x\n"), hdr->msgh_size);
  gdb_printf (gdb_stdlog,
	      _(" remote-port: 0x%x\n"), hdr->msgh_remote_port);
  gdb_printf (gdb_stdlog,
	      _(" local-port: 0x%x\n"), hdr->msgh_local_port);
  gdb_printf (gdb_stdlog,
	      _(" reserved: 0x%x\n"), hdr->msgh_reserved);
  gdb_printf (gdb_stdlog,
	      _(" id: 0x%x\n"), hdr->msgh_id);

  if (disp_body)
    {
      const unsigned char *data;
      const unsigned int *ldata;
      int size;
      int i;

      data = (unsigned char *)(hdr + 1);
      size = hdr->msgh_size - sizeof (mach_msg_header_t);

      if (hdr->msgh_bits & MACH_MSGH_BITS_COMPLEX)
	{
	  mach_msg_body_t *bod = (mach_msg_body_t*)data;
	  mach_msg_port_descriptor_t *desc =
	    (mach_msg_port_descriptor_t *)(bod + 1);
	  int k;
	  NDR_record_t *ndr;
	  gdb_printf (gdb_stdlog,
		      _("body: descriptor_count=%u\n"),
		      bod->msgh_descriptor_count);
	  data += sizeof (mach_msg_body_t);
	  size -= sizeof (mach_msg_body_t);
	  for (k = 0; k < bod->msgh_descriptor_count; k++)
	    switch (desc[k].type)
	      {
	      case MACH_MSG_PORT_DESCRIPTOR:
		gdb_printf
		  (gdb_stdlog,
		   _(" descr %d: type=%u (port) name=0x%x, dispo=%d\n"),
		   k, desc[k].type, desc[k].name, desc[k].disposition);
		break;
	      default:
		gdb_printf (gdb_stdlog,
			    _(" descr %d: type=%u\n"),
			    k, desc[k].type);
		break;
	      }
	  data += bod->msgh_descriptor_count
	    * sizeof (mach_msg_port_descriptor_t);
	  size -= bod->msgh_descriptor_count
	    * sizeof (mach_msg_port_descriptor_t);
	  ndr = (NDR_record_t *)(desc + bod->msgh_descriptor_count);
	  gdb_printf
	    (gdb_stdlog,
	     _("NDR: mig=%02x if=%02x encod=%02x "
	       "int=%02x char=%02x float=%02x\n"),
	     ndr->mig_vers, ndr->if_vers, ndr->mig_encoding,
	     ndr->int_rep, ndr->char_rep, ndr->float_rep);
	  data += sizeof (NDR_record_t);
	  size -= sizeof (NDR_record_t);
	}

      gdb_printf (gdb_stdlog, _("  data:"));
      ldata = (const unsigned int *)data;
      for (i = 0; i < size / sizeof (unsigned int); i++)
	gdb_printf (gdb_stdlog, " %08x", ldata[i]);
      gdb_printf (gdb_stdlog, _("\n"));
    }
}

/* Adjust inferior data when a new task was created.  */

static struct inferior *
darwin_find_new_inferior (task_t task_port, thread_t thread_port)
{
  int task_pid;
  struct inferior *inf;
  kern_return_t kret;
  mach_port_t prev;

  /* Find the corresponding pid.  */
  kret = pid_for_task (task_port, &task_pid);
  if (kret != KERN_SUCCESS)
    {
      MACH_CHECK_ERROR (kret);
      return NULL;
    }

  /* Find the inferior for this pid.  */
  inf = darwin_find_inferior_by_pid (task_pid);
  if (inf == NULL)
    return NULL;

  darwin_inferior *priv = get_darwin_inferior (inf);

  /* Deallocate saved exception ports.  */
  darwin_deallocate_exception_ports (priv);

  /* No need to remove dead_name notification, but still...  */
  kret = mach_port_request_notification (gdb_task, priv->task,
					 MACH_NOTIFY_DEAD_NAME, 0,
					 MACH_PORT_NULL,
					 MACH_MSG_TYPE_MAKE_SEND_ONCE,
					 &prev);
  if (kret != KERN_INVALID_ARGUMENT)
    MACH_CHECK_ERROR (kret);

  /* Replace old task port.  */
  kret = mach_port_deallocate (gdb_task, priv->task);
  MACH_CHECK_ERROR (kret);
  priv->task = task_port;

  darwin_setup_request_notification (inf);
  darwin_setup_exceptions (inf);

  return inf;
}

/* Check data representation.  */

static int
darwin_check_message_ndr (NDR_record_t *ndr)
{
  if (ndr->mig_vers != NDR_PROTOCOL_2_0
      || ndr->if_vers != NDR_PROTOCOL_2_0
      || ndr->mig_encoding != NDR_record.mig_encoding
      || ndr->int_rep != NDR_record.int_rep
      || ndr->char_rep != NDR_record.char_rep
      || ndr->float_rep != NDR_record.float_rep)
    return -1;
  return 0;
}

/* Decode an exception message.  */

int
darwin_nat_target::decode_exception_message (mach_msg_header_t *hdr,
					     inferior **pinf,
					     darwin_thread_t **pthread)
{
  mach_msg_body_t *bod = (mach_msg_body_t*)(hdr + 1);
  mach_msg_port_descriptor_t *desc = (mach_msg_port_descriptor_t *)(bod + 1);
  NDR_record_t *ndr;
  integer_t *data;
  struct inferior *inf;
  darwin_thread_t *thread;
  task_t task_port;
  thread_t thread_port;
  kern_return_t kret;
  int i;

  /* Check message destination.  */
  if (hdr->msgh_local_port != darwin_ex_port)
    return -1;

  /* Check message header.  */
  if (!(hdr->msgh_bits & MACH_MSGH_BITS_COMPLEX))
    return -1;

  /* Check descriptors.  */
  if (hdr->msgh_size < (sizeof (*hdr) + sizeof (*bod) + 2 * sizeof (*desc)
			+ sizeof (*ndr) + 2 * sizeof (integer_t))
      || bod->msgh_descriptor_count != 2
      || desc[0].type != MACH_MSG_PORT_DESCRIPTOR
      || desc[0].disposition != MACH_MSG_TYPE_MOVE_SEND
      || desc[1].type != MACH_MSG_PORT_DESCRIPTOR
      || desc[1].disposition != MACH_MSG_TYPE_MOVE_SEND)
    return -1;

  /* Check data representation.  */
  ndr = (NDR_record_t *)(desc + 2);
  if (darwin_check_message_ndr (ndr) != 0)
    return -1;

  /* Ok, the hard work.  */
  data = (integer_t *)(ndr + 1);

  task_port = desc[1].name;
  thread_port = desc[0].name;

  /* Find process by port.  */
  inf = darwin_find_inferior_by_task (task_port);
  *pinf = inf;

  if (inf == NULL && data[0] == EXC_SOFTWARE && data[1] == 2
      && data[2] == EXC_SOFT_SIGNAL && data[3] == SIGTRAP)
    {
      /* Not a known inferior, but a sigtrap.  This happens on darwin 16.1.0,
	 as a new Mach task is created when a process exec.  */
      inf = darwin_find_new_inferior (task_port, thread_port);
      *pinf = inf;

      if (inf == NULL)
	{
	  /* Deallocate task_port, unless it was saved.  */
	  kret = mach_port_deallocate (mach_task_self (), task_port);
	  MACH_CHECK_ERROR (kret);
	}
    }
  else
    {
      /* We got new rights to the task, get rid of it.  Do not get rid of
	 thread right, as we will need it to find the thread.  */
      kret = mach_port_deallocate (mach_task_self (), task_port);
      MACH_CHECK_ERROR (kret);
    }

  if (inf == NULL)
    {
      /* Not a known inferior.  This could happen if the child fork, as
	 the created process will inherit its exception port.
	 FIXME: should the exception port be restored ?  */
      mig_reply_error_t reply;

      inferior_debug
	(4, _("darwin_decode_exception_message: unknown task 0x%x\n"),
	 task_port);

      /* Free thread port (we don't know it).  */
      kret = mach_port_deallocate (mach_task_self (), thread_port);
      MACH_CHECK_ERROR (kret);

      darwin_encode_reply (&reply, hdr, KERN_SUCCESS);

      kret = mach_msg (&reply.Head, MACH_SEND_MSG | MACH_SEND_INTERRUPT,
		       reply.Head.msgh_size, 0,
		       MACH_PORT_NULL, MACH_MSG_TIMEOUT_NONE,
		       MACH_PORT_NULL);
      MACH_CHECK_ERROR (kret);

      return 0;
    }

  /* Find thread by port.  */
  /* Check for new threads.  Do it early so that the port in the exception
     message can be deallocated.  */
  check_new_threads (inf);

  /* Free the thread port (as gdb knows the thread, it has already has a right
     for it, so this just decrement a reference counter).  */
  kret = mach_port_deallocate (mach_task_self (), thread_port);
  MACH_CHECK_ERROR (kret);

  thread = darwin_find_thread (inf, thread_port);
  if (thread == NULL)
    return -1;
  *pthread = thread;

  /* The thread should be running.  However we have observed cases where a
     thread got a SIGTTIN message after being stopped.  */
  gdb_assert (thread->msg_state != DARWIN_MESSAGE);

  /* Finish decoding.  */
  thread->event.header = *hdr;
  thread->event.thread_port = thread_port;
  thread->event.task_port = task_port;
  thread->event.ex_type = data[0];
  thread->event.data_count = data[1];

  if (hdr->msgh_size < (sizeof (*hdr) + sizeof (*bod) + 2 * sizeof (*desc)
			+ sizeof (*ndr) + 2 * sizeof (integer_t)
			+ data[1] * sizeof (integer_t)))
      return -1;
  for (i = 0; i < data[1]; i++)
    thread->event.ex_data[i] = data[2 + i];

  thread->msg_state = DARWIN_MESSAGE;

  return 0;
}

/* Decode dead_name notify message.  */

static int
darwin_decode_notify_message (mach_msg_header_t *hdr, struct inferior **pinf)
{
  NDR_record_t *ndr = (NDR_record_t *)(hdr + 1);
  integer_t *data = (integer_t *)(ndr + 1);
  struct inferior *inf;
  task_t task_port;

  /* Check message header.  */
  if (hdr->msgh_bits & MACH_MSGH_BITS_COMPLEX)
    return -1;

  /* Check descriptors.  */
  if (hdr->msgh_size < (sizeof (*hdr) + sizeof (*ndr) + sizeof (integer_t)))
    return -2;

  /* Check data representation.  */
  if (darwin_check_message_ndr (ndr) != 0)
    return -3;

  task_port = data[0];

  /* Find process by port.  */
  inf = darwin_find_inferior_by_task (task_port);
  *pinf = inf;

  /* Check message destination.  */
  if (inf != NULL)
    {
      darwin_inferior *priv = get_darwin_inferior (inf);
      if (hdr->msgh_local_port != priv->notify_port)
	return -4;
    }

  return 0;
}

static void
darwin_encode_reply (mig_reply_error_t *reply, mach_msg_header_t *hdr,
		     integer_t code)
{
  mach_msg_header_t *rh = &reply->Head;

  rh->msgh_bits = MACH_MSGH_BITS (MACH_MSGH_BITS_REMOTE (hdr->msgh_bits), 0);
  rh->msgh_remote_port = hdr->msgh_remote_port;
  rh->msgh_size = (mach_msg_size_t) sizeof (mig_reply_error_t);
  rh->msgh_local_port = MACH_PORT_NULL;
  rh->msgh_id = hdr->msgh_id + 100;

  reply->NDR = NDR_record;
  reply->RetCode = code;
}

static void
darwin_send_reply (struct inferior *inf, darwin_thread_t *thread)
{
  kern_return_t kret;
  mig_reply_error_t reply;
  darwin_inferior *priv = get_darwin_inferior (inf);

  darwin_encode_reply (&reply, &thread->event.header, KERN_SUCCESS);

  kret = mach_msg (&reply.Head, MACH_SEND_MSG | MACH_SEND_INTERRUPT,
		   reply.Head.msgh_size, 0,
		   MACH_PORT_NULL, MACH_MSG_TIMEOUT_NONE,
		   MACH_PORT_NULL);
  MACH_CHECK_ERROR (kret);

  priv->pending_messages--;
}

/* Wrapper around the __pthread_kill syscall.  We use this instead of the
   pthread_kill function to be able to send a signal to any kind of thread,
   including GCD threads.  */

static int
darwin_pthread_kill (darwin_thread_t *thread, int nsignal)
{
  DIAGNOSTIC_PUSH;
  DIAGNOSTIC_IGNORE_DEPRECATED_DECLARATIONS;
  int res = syscall (SYS___pthread_kill, thread->gdb_port, nsignal);
  DIAGNOSTIC_POP;
  return res;
}

static void
darwin_resume_thread (struct inferior *inf, darwin_thread_t *thread,
		      int step, int nsignal)
{
  inferior_debug
    (3, _("darwin_resume_thread: state=%d, thread=0x%x, step=%d nsignal=%d\n"),
     thread->msg_state, thread->gdb_port, step, nsignal);

  switch (thread->msg_state)
    {
    case DARWIN_MESSAGE:
      if (thread->event.ex_type == EXC_SOFTWARE
	  && thread->event.ex_data[0] == EXC_SOFT_SIGNAL)
	{
	  /* Either deliver a new signal or cancel the signal received.  */
	  int res = PTRACE (PT_THUPDATE, inf->pid,
			    (caddr_t) (uintptr_t) thread->gdb_port, nsignal);
	  if (res < 0)
	    inferior_debug (1, _("ptrace THUP: res=%d\n"), res);
	}
      else if (nsignal)
	{
	  /* Note: ptrace is allowed only if the process is stopped.
	     Directly send the signal to the thread.  */
	  int res = darwin_pthread_kill (thread, nsignal);
	  inferior_debug (4, _("darwin_resume_thread: kill 0x%x %d: %d\n"),
			  thread->gdb_port, nsignal, res);
	  thread->signaled = 1;
	}

      /* Set or reset single step.  */
      inferior_debug (4, _("darwin_set_sstep (thread=0x%x, enable=%d)\n"),
		      thread->gdb_port, step);
      darwin_set_sstep (thread->gdb_port, step);
      thread->single_step = step;

      darwin_send_reply (inf, thread);
      thread->msg_state = DARWIN_RUNNING;
      break;

    case DARWIN_RUNNING:
      break;

    case DARWIN_STOPPED:
      kern_return_t kret = thread_resume (thread->gdb_port);
      MACH_CHECK_ERROR (kret);

      thread->msg_state = DARWIN_RUNNING;
      break;
    }
}

/* Resume all threads of the inferior.  */

static void
darwin_resume_inferior_threads (struct inferior *inf, int step, int nsignal)
{
  darwin_inferior *priv = get_darwin_inferior (inf);

  if (priv != nullptr)
    for (darwin_thread_t *thread : priv->threads)
      darwin_resume_thread (inf, thread, step, nsignal);
}

/* Suspend all threads of INF.  */

static void
darwin_suspend_inferior_threads (struct inferior *inf)
{
  darwin_inferior *priv = get_darwin_inferior (inf);

  for (darwin_thread_t *thread : priv->threads)
    {
      switch (thread->msg_state)
	{
	case DARWIN_STOPPED:
	case DARWIN_MESSAGE:
	  break;
	case DARWIN_RUNNING:
	  {
	    kern_return_t kret = thread_suspend (thread->gdb_port);
	    MACH_CHECK_ERROR (kret);
	    thread->msg_state = DARWIN_STOPPED;
	    break;
	  }
	}
    }
}

void
darwin_nat_target::resume (ptid_t ptid, int step, enum gdb_signal signal)
{
  int nsignal;

  inferior_debug
    (2, _("darwin_resume: ptid=%s, step=%d, signal=%d\n"),
     ptid.to_string ().c_str (), step, signal);

  if (signal == GDB_SIGNAL_0)
    nsignal = 0;
  else
    nsignal = gdb_signal_to_host (signal);

  /* Don't try to single step all threads.  */
  if (step)
    ptid = inferior_ptid;

  /* minus_one_ptid is RESUME_ALL.  */
  if (ptid == minus_one_ptid)
    {
      /* Resume threads.  */
      for (inferior *inf : all_inferiors ())
	darwin_resume_inferior_threads (inf, step, nsignal);

      /* Resume tasks.  */
      for (inferior *inf : all_inferiors ())
	darwin_resume_inferior (inf);
    }
  else
    {
      inferior *inf = find_inferior_ptid (this, ptid);
      long tid = ptid.tid ();

      /* Stop the inferior (should be useless).  */
      darwin_suspend_inferior (inf);

      if (tid == 0)
	darwin_resume_inferior_threads (inf, step, nsignal);
      else
	{
	  darwin_thread_t *thread;

	  /* Suspend threads of the task.  */
	  darwin_suspend_inferior_threads (inf);

	  /* Resume the selected thread.  */
	  thread = darwin_find_thread (inf, tid);
	  gdb_assert (thread);
	  darwin_resume_thread (inf, thread, step, nsignal);
	}

      /* Resume the task.  */
      darwin_resume_inferior (inf);
    }
}

ptid_t
darwin_nat_target::decode_message (mach_msg_header_t *hdr,
				   darwin_thread_t **pthread,
				   inferior **pinf,
				   target_waitstatus *status)
{
  darwin_thread_t *thread;
  struct inferior *inf;

  /* Exception message.  2401 == 0x961 is exc.  */
  if (hdr->msgh_id == 2401)
    {
      int res;

      /* Decode message.  */
      res = decode_exception_message (hdr, &inf, &thread);

      if (res < 0)
	{
	  /* Should not happen...  */
	  warning (_("darwin_wait: ill-formatted message (id=0x%x)\n"),
		   hdr->msgh_id);
	  /* FIXME: send a failure reply?  */
	  status->set_ignore ();
	  return minus_one_ptid;
	}
      if (inf == NULL)
	{
	  status->set_ignore ();
	  return minus_one_ptid;
	}
      *pinf = inf;
      *pthread = thread;

      darwin_inferior *priv = get_darwin_inferior (inf);

      priv->pending_messages++;

      thread->msg_state = DARWIN_MESSAGE;

      inferior_debug (4, _("darwin_wait: thread=0x%x, got %s\n"),
		      thread->gdb_port,
		      unparse_exception_type (thread->event.ex_type));

      switch (thread->event.ex_type)
	{
	case EXC_BAD_ACCESS:
	  status->set_stopped (GDB_EXC_BAD_ACCESS);
	  break;
	case EXC_BAD_INSTRUCTION:
	  status->set_stopped (GDB_EXC_BAD_INSTRUCTION);
	  break;
	case EXC_ARITHMETIC:
	  status->set_stopped (GDB_EXC_ARITHMETIC);
	  break;
	case EXC_EMULATION:
	  status->set_stopped (GDB_EXC_EMULATION);
	  break;
	case EXC_SOFTWARE:
	  if (thread->event.ex_data[0] == EXC_SOFT_SIGNAL)
	    {
	      status->set_stopped
		(gdb_signal_from_host (thread->event.ex_data[1]));
	      inferior_debug (5, _("  (signal %d: %s)\n"),
			      thread->event.ex_data[1],
			      gdb_signal_to_name (status->sig ()));

	      /* If the thread is stopped because it has received a signal
		 that gdb has just sent, continue.  */
	      if (thread->signaled)
		{
		  thread->signaled = 0;
		  darwin_send_reply (inf, thread);
		  thread->msg_state = DARWIN_RUNNING;
		  status->set_ignore ();
		}
	    }
	  else
	    status->set_stopped (GDB_EXC_SOFTWARE);
	  break;
	case EXC_BREAKPOINT:
	  /* Many internal GDB routines expect breakpoints to be reported
	     as GDB_SIGNAL_TRAP, and will report GDB_EXC_BREAKPOINT
	     as a spurious signal.  */
	  status->set_stopped (GDB_SIGNAL_TRAP);
	  break;
	default:
	  status->set_stopped (GDB_SIGNAL_UNKNOWN);
	  break;
	}

      return ptid_t (inf->pid, 0, thread->gdb_port);
    }
  else if (hdr->msgh_id == 0x48)
    {
      /* MACH_NOTIFY_DEAD_NAME: notification for exit *or* WIFSTOPPED.  */
      int res;

      res = darwin_decode_notify_message (hdr, &inf);

      if (res < 0)
	{
	  /* Should not happen...  */
	  warning
	    (_("darwin_wait: ill-formatted message (id=0x%x, res=%d)\n"),
	     hdr->msgh_id, res);
	}

      *pinf = NULL;
      *pthread = NULL;

      if (res < 0 || inf == NULL)
	{
	  status->set_ignore ();
	  return minus_one_ptid;
	}

      if (inf != NULL)
	{
	  darwin_inferior *priv = get_darwin_inferior (inf);

	  if (!priv->no_ptrace)
	    {
	      pid_t res_pid;
	      int wstatus;

	      res_pid = wait4 (inf->pid, &wstatus, 0, NULL);
	      if (res_pid < 0 || res_pid != inf->pid)
		{
		  warning (_("wait4: res=%d: %s\n"),
			   res_pid, safe_strerror (errno));
		  status->set_ignore ();
		  return minus_one_ptid;
		}
	      if (WIFEXITED (wstatus))
		{
		  status->set_exited (WEXITSTATUS (wstatus));
		  inferior_debug (4, _("darwin_wait: pid=%d exit, status=0x%x\n"),
				  res_pid, wstatus);
		}
	      else if (WIFSTOPPED (wstatus))
		{
		  /* Ignore stopped state, it will be handled by the next
		     exception.  */
		  status->set_ignore ();
		  inferior_debug (4, _("darwin_wait: pid %d received WIFSTOPPED\n"),
				  res_pid);
		  return minus_one_ptid;
		}
	      else if (WIFSIGNALED (wstatus))
		{
		  status->set_signalled
		    (gdb_signal_from_host (WTERMSIG (wstatus)));
		  inferior_debug (4, _("darwin_wait: pid=%d received signal %d\n"),
				  res_pid, status->sig());
		}
	      else
		{
		  status->set_ignore ();
		  warning (_("Unexpected wait status after MACH_NOTIFY_DEAD_NAME "
			     "notification: 0x%x"), wstatus);
		  return minus_one_ptid;
		}

	      return ptid_t (inf->pid);
	    }
	  else
	    {
	      inferior_debug (4, _("darwin_wait: pid=%d\n"), inf->pid);
	      status->set_exited (0 /* Don't know.  */);
	      return ptid_t (inf->pid, 0, 0);
	    }
	}
    }

  /* Unknown message.  */
  warning (_("darwin: got unknown message, id: 0x%x"), hdr->msgh_id);
  status->set_ignore ();
  return minus_one_ptid;
}

int
darwin_nat_target::cancel_breakpoint (inferior *inf, ptid_t ptid)
{
  /* Arrange for a breakpoint to be hit again later.  We will handle
     the current event, eventually we will resume this thread, and this
     breakpoint will trap again.

     If we do not do this, then we run the risk that the user will
     delete or disable the breakpoint, but the thread will have already
     tripped on it.  */

  struct regcache *regcache = get_thread_regcache (this, ptid);
  struct gdbarch *gdbarch = regcache->arch ();
  CORE_ADDR pc;

  pc = regcache_read_pc (regcache) - gdbarch_decr_pc_after_break (gdbarch);
  if (breakpoint_inserted_here_p (inf->aspace.get (), pc))
    {
      inferior_debug (4, "cancel_breakpoint for thread 0x%lx\n",
		      (unsigned long) ptid.tid ());

      /* Back up the PC if necessary.  */
      if (gdbarch_decr_pc_after_break (gdbarch))
	regcache_write_pc (regcache, pc);

      return 1;
    }
  return 0;
}

ptid_t
darwin_nat_target::wait_1 (ptid_t ptid, struct target_waitstatus *status)
{
  kern_return_t kret;
  union
  {
    mach_msg_header_t hdr;
    char data[0x100];
  } msgin;
  mach_msg_header_t *hdr = &msgin.hdr;
  ptid_t res;
  darwin_thread_t *thread;

  inferior_debug
    (2, _("darwin_wait: waiting for a message ptid=%s\n"),
     ptid.to_string ().c_str ());

  /* Handle fake stop events at first.  */
  if (darwin_inf_fake_stop != NULL)
    {
      inferior *inf = darwin_inf_fake_stop;
      darwin_inf_fake_stop = NULL;

      darwin_inferior *priv = get_darwin_inferior (inf);

      status->set_stopped (GDB_SIGNAL_TRAP);
      thread = priv->threads[0];
      thread->msg_state = DARWIN_STOPPED;
      return ptid_t (inf->pid, 0, thread->gdb_port);
    }

  do
    {
      /* set_sigint_trap (); */

      /* Wait for a message.  */
      kret = mach_msg (&msgin.hdr, MACH_RCV_MSG | MACH_RCV_INTERRUPT, 0,
		       sizeof (msgin.data), darwin_port_set, 0, MACH_PORT_NULL);

      /* clear_sigint_trap (); */

      if (kret == MACH_RCV_INTERRUPTED)
	{
	  status->set_ignore ();
	  return minus_one_ptid;
	}

      if (kret != MACH_MSG_SUCCESS)
	{
	  inferior_debug (5, _("mach_msg: ret=0x%x\n"), kret);
	  status->set_spurious ();
	  return minus_one_ptid;
	}

      /* Debug: display message.  */
      if (darwin_debug_flag > 10)
	darwin_dump_message (hdr, darwin_debug_flag > 11);

      inferior *inf;
      res = decode_message (hdr, &thread, &inf, status);
      if (res == minus_one_ptid)
	continue;

      /* Early return in case an inferior has exited.  */
      if (inf == NULL)
	return res;
    }
  while (status->kind () == TARGET_WAITKIND_IGNORE);

  /* Stop all tasks.  */
  for (inferior *inf : all_inferiors (this))
    {
      darwin_suspend_inferior (inf);
      check_new_threads (inf);
    }

  /* Read pending messages.  */
  while (1)
    {
      struct target_waitstatus status2;
      ptid_t ptid2;

      kret = mach_msg (&msgin.hdr,
		       MACH_RCV_MSG | MACH_RCV_TIMEOUT, 0,
		       sizeof (msgin.data), darwin_port_set, 1, MACH_PORT_NULL);

      if (kret == MACH_RCV_TIMED_OUT)
	break;
      if (kret != MACH_MSG_SUCCESS)
	{
	  inferior_debug
	    (5, _("darwin_wait: mach_msg(pending) ret=0x%x\n"), kret);
	  break;
	}

      /* Debug: display message.  */
      if (darwin_debug_flag > 10)
	darwin_dump_message (hdr, darwin_debug_flag > 11);

      inferior *inf;
      ptid2 = decode_message (hdr, &thread, &inf, &status2);

      if (inf != NULL && thread != NULL
	  && thread->event.ex_type == EXC_BREAKPOINT)
	{
	  if (thread->single_step
	      || cancel_breakpoint (inf,
				    ptid_t (inf->pid, 0, thread->gdb_port)))
	    {
	      gdb_assert (thread->msg_state == DARWIN_MESSAGE);
	      darwin_send_reply (inf, thread);
	      thread->msg_state = DARWIN_RUNNING;
	    }
	  else
	    inferior_debug
	      (3, _("darwin_wait: thread 0x%x hit a non-gdb breakpoint\n"),
	       thread->gdb_port);
	}
      else
	inferior_debug (3, _("darwin_wait: unhandled pending message\n"));
    }
  return res;
}

ptid_t
darwin_nat_target::wait (ptid_t ptid, struct target_waitstatus *status,
			 target_wait_flags options)
{
  return wait_1 (ptid, status);
}

void
darwin_nat_target::interrupt ()
{
  struct inferior *inf = current_inferior ();
  darwin_inferior *priv = get_darwin_inferior (inf);

  /* FIXME: handle in no_ptrace mode.  */
  gdb_assert (!priv->no_ptrace);
  ::kill (inf->pid, SIGINT);
}

/* Deallocate threads port and vector.  */

static void
darwin_deallocate_threads (struct inferior *inf)
{
  darwin_inferior *priv = get_darwin_inferior (inf);

  for (darwin_thread_t *t : priv->threads)
    {
      kern_return_t kret = mach_port_deallocate (gdb_task, t->gdb_port);
      MACH_CHECK_ERROR (kret);
    }

  priv->threads.clear ();
}

void
darwin_nat_target::mourn_inferior ()
{
  struct inferior *inf = current_inferior ();
  darwin_inferior *priv = get_darwin_inferior (inf);
  kern_return_t kret;
  mach_port_t prev;

  /* Deallocate threads.  */
  darwin_deallocate_threads (inf);

  /* Remove notify_port from darwin_port_set.  */
  kret = mach_port_move_member (gdb_task,
				priv->notify_port, MACH_PORT_NULL);
  MACH_CHECK_ERROR (kret);

  /* Remove task port dead_name notification.  */
  kret = mach_port_request_notification (gdb_task, priv->task,
					 MACH_NOTIFY_DEAD_NAME, 0,
					 MACH_PORT_NULL,
					 MACH_MSG_TYPE_MAKE_SEND_ONCE,
					 &prev);
  /* This can fail if the task is dead.  */
  inferior_debug (4, "task=0x%x, prev=0x%x, notify_port=0x%x\n",
		  priv->task, prev, priv->notify_port);

  if (kret == KERN_SUCCESS)
    {
      kret = mach_port_deallocate (gdb_task, prev);
      MACH_CHECK_ERROR (kret);
    }

  /* Destroy notify_port.  */
  kret = mach_port_destroy (gdb_task, priv->notify_port);
  MACH_CHECK_ERROR (kret);

  /* Deallocate saved exception ports.  */
  darwin_deallocate_exception_ports (priv);

  /* Deallocate task port.  */
  kret = mach_port_deallocate (gdb_task, priv->task);
  MACH_CHECK_ERROR (kret);

  inf->priv = NULL;

  inf_child_target::mourn_inferior ();
}

static void
darwin_reply_to_all_pending_messages (struct inferior *inf)
{
  darwin_inferior *priv = get_darwin_inferior (inf);

  for (darwin_thread_t *t : priv->threads)
    {
      if (t->msg_state == DARWIN_MESSAGE)
	darwin_resume_thread (inf, t, 0, 0);
    }
}

void
darwin_nat_target::stop_inferior (inferior *inf)
{
  struct target_waitstatus wstatus;
  ptid_t ptid;
  int res;
  darwin_inferior *priv = get_darwin_inferior (inf);

  gdb_assert (inf != NULL);

  darwin_suspend_inferior (inf);

  darwin_reply_to_all_pending_messages (inf);

  if (priv->no_ptrace)
    return;

  res = ::kill (inf->pid, SIGSTOP);
  if (res != 0)
    warning (_("cannot kill: %s"), safe_strerror (errno));

  /* Wait until the process is really stopped.  */
  while (1)
    {
      ptid = wait_1 (ptid_t (inf->pid), &wstatus);
      if (wstatus.kind () == TARGET_WAITKIND_STOPPED
	  && wstatus.sig () == GDB_SIGNAL_STOP)
	break;
    }
}

static kern_return_t
darwin_save_exception_ports (darwin_inferior *inf)
{
  kern_return_t kret;

  inf->exception_info.count =
    sizeof (inf->exception_info.ports) / sizeof (inf->exception_info.ports[0]);

  kret = task_get_exception_ports
    (inf->task, EXC_MASK_ALL, inf->exception_info.masks,
     &inf->exception_info.count, inf->exception_info.ports,
     inf->exception_info.behaviors, inf->exception_info.flavors);
  return kret;
}

static kern_return_t
darwin_restore_exception_ports (darwin_inferior *inf)
{
  int i;
  kern_return_t kret;

  for (i = 0; i < inf->exception_info.count; i++)
    {
      kret = task_set_exception_ports
	(inf->task, inf->exception_info.masks[i], inf->exception_info.ports[i],
	 inf->exception_info.behaviors[i], inf->exception_info.flavors[i]);
      if (kret != KERN_SUCCESS)
	return kret;
    }

  return KERN_SUCCESS;
}

/* Deallocate saved exception ports.  */

static void
darwin_deallocate_exception_ports (darwin_inferior *inf)
{
  int i;
  kern_return_t kret;

  for (i = 0; i < inf->exception_info.count; i++)
    {
      kret = mach_port_deallocate (gdb_task, inf->exception_info.ports[i]);
      MACH_CHECK_ERROR (kret);
    }
  inf->exception_info.count = 0;
}

static void
darwin_setup_exceptions (struct inferior *inf)
{
  darwin_inferior *priv = get_darwin_inferior (inf);
  kern_return_t kret;
  exception_mask_t mask;

  kret = darwin_save_exception_ports (priv);
  if (kret != KERN_SUCCESS)
    error (_("Unable to save exception ports, task_get_exception_ports"
	     "returned: %d"),
	   kret);

  /* Set exception port.  */
  if (enable_mach_exceptions)
    mask = EXC_MASK_ALL;
  else
    mask = EXC_MASK_SOFTWARE | EXC_MASK_BREAKPOINT;
  kret = task_set_exception_ports (priv->task, mask, darwin_ex_port,
				   EXCEPTION_DEFAULT, THREAD_STATE_NONE);
  if (kret != KERN_SUCCESS)
    error (_("Unable to set exception ports, task_set_exception_ports"
	     "returned: %d"),
	   kret);
}

void
darwin_nat_target::kill ()
{
  struct inferior *inf = current_inferior ();
  darwin_inferior *priv = get_darwin_inferior (inf);
  struct target_waitstatus wstatus;
  ptid_t ptid;
  kern_return_t kret;
  int res;

  if (inferior_ptid == null_ptid)
    return;

  gdb_assert (inf != NULL);

  kret = darwin_restore_exception_ports (priv);
  MACH_CHECK_ERROR (kret);

  darwin_reply_to_all_pending_messages (inf);

  res = ::kill (inf->pid, 9);

  if (res == 0)
    {
      /* On MacOS version Sierra, the darwin_restore_exception_ports call
	 does not work as expected.
	 When the kill function is called, the SIGKILL signal is received
	 by gdb whereas it should have been received by the kernel since
	 the exception ports have been restored.
	 This behavior is not the expected one thus gdb does not reply to
	 the received SIGKILL message. This situation leads to a "busy"
	 resource from the kernel point of view and the inferior is never
	 released, causing it to remain as a zombie process, even after
	 GDB exits.
	 To work around this, we mark all the threads of the inferior as
	 signaled thus darwin_decode_message function knows that the kill
	 signal was sent by gdb and will take the appropriate action
	 (cancel signal and reply to the signal message).  */
      for (darwin_thread_t *thread : priv->threads)
	thread->signaled = 1;

      darwin_resume_inferior (inf);

      ptid = wait_1 (ptid_t (inf->pid), &wstatus);
    }
  else if (errno != ESRCH)
    warning (_("Failed to kill inferior: kill (%d, 9) returned [%s]"),
	     inf->pid, safe_strerror (errno));

  target_mourn_inferior (ptid_t (inf->pid));
}

static void
darwin_setup_request_notification (struct inferior *inf)
{
  darwin_inferior *priv = get_darwin_inferior (inf);
  kern_return_t kret;
  mach_port_t prev_not;

  kret = mach_port_request_notification (gdb_task, priv->task,
					 MACH_NOTIFY_DEAD_NAME, 0,
					 priv->notify_port,
					 MACH_MSG_TYPE_MAKE_SEND_ONCE,
					 &prev_not);
  if (kret != KERN_SUCCESS)
    error (_("Termination notification request failed, "
	     "mach_port_request_notification\n"
	     "returned: %d"),
	   kret);
  if (prev_not != MACH_PORT_NULL)
    {
      /* This is unexpected, as there should not be any previously
	 registered notification request.  But this is not a fatal
	 issue, so just emit a warning.  */
      warning (_("\
A task termination request was registered before the debugger registered\n\
its own.  This is unexpected, but should otherwise not have any actual\n\
impact on the debugging session."));
    }
}

static void
darwin_attach_pid (struct inferior *inf)
{
  kern_return_t kret;

  darwin_inferior *priv = new darwin_inferior;
  inf->priv.reset (priv);

  try
    {
      kret = task_for_pid (gdb_task, inf->pid, &priv->task);
      if (kret != KERN_SUCCESS)
	{
	  int status;

	  if (!inf->attach_flag)
	    {
	      kill (inf->pid, 9);
	      waitpid (inf->pid, &status, 0);
	    }

	  error
	    (_("Unable to find Mach task port for process-id %d: %s (0x%lx).\n"
	       " (please check gdb is codesigned - see taskgated(8))"),
	     inf->pid, mach_error_string (kret), (unsigned long) kret);
	}

      inferior_debug (2, _("inferior task: 0x%x, pid: %d\n"),
		      priv->task, inf->pid);

      if (darwin_ex_port == MACH_PORT_NULL)
	{
	  /* Create a port to get exceptions.  */
	  kret = mach_port_allocate (gdb_task, MACH_PORT_RIGHT_RECEIVE,
				     &darwin_ex_port);
	  if (kret != KERN_SUCCESS)
	    error (_("Unable to create exception port, mach_port_allocate "
		     "returned: %d"),
		   kret);

	  kret = mach_port_insert_right (gdb_task, darwin_ex_port,
					 darwin_ex_port,
					 MACH_MSG_TYPE_MAKE_SEND);
	  if (kret != KERN_SUCCESS)
	    error (_("Unable to create exception port, mach_port_insert_right "
		     "returned: %d"),
		   kret);

	  /* Create a port set and put ex_port in it.  */
	  kret = mach_port_allocate (gdb_task, MACH_PORT_RIGHT_PORT_SET,
				     &darwin_port_set);
	  if (kret != KERN_SUCCESS)
	    error (_("Unable to create port set, mach_port_allocate "
		     "returned: %d"),
		   kret);

	  kret = mach_port_move_member (gdb_task, darwin_ex_port,
					darwin_port_set);
	  if (kret != KERN_SUCCESS)
	    error (_("Unable to move exception port into new port set, "
		     "mach_port_move_member\n"
		     "returned: %d"),
		   kret);
	}

      /* Create a port to be notified when the child task terminates.  */
      kret = mach_port_allocate (gdb_task, MACH_PORT_RIGHT_RECEIVE,
				 &priv->notify_port);
      if (kret != KERN_SUCCESS)
	error (_("Unable to create notification port, mach_port_allocate "
		 "returned: %d"),
	       kret);

      kret = mach_port_move_member (gdb_task,
				    priv->notify_port, darwin_port_set);
      if (kret != KERN_SUCCESS)
	error (_("Unable to move notification port into new port set, "
		 "mach_port_move_member\n"
		 "returned: %d"),
	       kret);

      darwin_setup_request_notification (inf);

      darwin_setup_exceptions (inf);
    }
  catch (const gdb_exception &ex)
    {
      exit_inferior (inf);
      switch_to_no_thread ();

      throw;
    }

  target_ops *darwin_ops = get_native_target ();
  if (!inf->target_is_pushed (darwin_ops))
    inf->push_target (darwin_ops);
}

/* Get the thread_info object corresponding to this darwin_thread_info.  */

static struct thread_info *
thread_info_from_private_thread_info (darwin_thread_info *pti)
{
  for (struct thread_info *it : all_threads ())
    {
      darwin_thread_info *iter_pti = get_darwin_thread_info (it);

      if (iter_pti->gdb_port == pti->gdb_port)
	return it;
    }

  gdb_assert_not_reached ("did not find gdb thread for darwin thread");
}

void
darwin_nat_target::init_thread_list (inferior *inf)
{
  check_new_threads (inf);

  darwin_inferior *priv = get_darwin_inferior (inf);

  gdb_assert (!priv->threads.empty ());

  darwin_thread_info *first_pti = priv->threads.front ();
  struct thread_info *first_thread
    = thread_info_from_private_thread_info (first_pti);

  switch_to_thread (first_thread);
}

/* The child must synchronize with gdb: gdb must set the exception port
   before the child call PTRACE_SIGEXC.  We use a pipe to achieve this.
   FIXME: is there a lighter way ?  */
static int ptrace_fds[2];

static void
darwin_ptrace_me (void)
{
  int res;
  char c;

  /* Close write end point.  */
  if (close (ptrace_fds[1]) < 0)
    trace_start_error_with_name ("close");

  /* Wait until gdb is ready.  */
  res = read (ptrace_fds[0], &c, 1);
  if (res != 0)
    trace_start_error (_("unable to read from pipe, read returned: %d"), res);

  if (close (ptrace_fds[0]) < 0)
    trace_start_error_with_name ("close");

  /* Get rid of privileges.  */
  if (setegid (getgid ()) < 0)
    trace_start_error_with_name ("setegid");

  /* Set TRACEME.  */
  if (PTRACE (PT_TRACE_ME, 0, 0, 0) < 0)
    trace_start_error_with_name ("PTRACE");

  /* Redirect signals to exception port.  */
  if (PTRACE (PT_SIGEXC, 0, 0, 0) < 0)
    trace_start_error_with_name ("PTRACE");
}

/* Dummy function to be sure fork_inferior uses fork(2) and not vfork(2).  */
static void
darwin_pre_ptrace (void)
{
  if (pipe (ptrace_fds) != 0)
    {
      ptrace_fds[0] = -1;
      ptrace_fds[1] = -1;
      error (_("unable to create a pipe: %s"), safe_strerror (errno));
    }

  mark_fd_no_cloexec (ptrace_fds[0]);
  mark_fd_no_cloexec (ptrace_fds[1]);
}

void
darwin_nat_target::ptrace_him (int pid)
{
  struct inferior *inf = current_inferior ();

  darwin_attach_pid (inf);

  /* Let's the child run.  */
  ::close (ptrace_fds[0]);
  ::close (ptrace_fds[1]);

  unmark_fd_no_cloexec (ptrace_fds[0]);
  unmark_fd_no_cloexec (ptrace_fds[1]);

  init_thread_list (inf);

  gdb_startup_inferior (pid, START_INFERIOR_TRAPS_EXPECTED);
}

static void
darwin_execvp (const char *file, char * const argv[], char * const env[])
{
  posix_spawnattr_t attr;
  short ps_flags = 0;
  int res;

  res = posix_spawnattr_init (&attr);
  if (res != 0)
    {
      gdb_printf
	(gdb_stderr, "Cannot initialize attribute for posix_spawn\n");
      return;
    }

  /* Do like execve: replace the image.  */
  ps_flags = POSIX_SPAWN_SETEXEC;

  /* Disable ASLR.  The constant doesn't look to be available outside the
     kernel include files.  */
#ifndef _POSIX_SPAWN_DISABLE_ASLR
#define _POSIX_SPAWN_DISABLE_ASLR 0x0100
#endif
  ps_flags |= _POSIX_SPAWN_DISABLE_ASLR;
  res = posix_spawnattr_setflags (&attr, ps_flags);
  if (res != 0)
    {
      gdb_printf (gdb_stderr, "Cannot set posix_spawn flags\n");
      return;
    }

  posix_spawnp (NULL, argv[0], NULL, &attr, argv, env);
}

/* Read kernel version, and return TRUE if this host may have System
   Integrity Protection (Sierra or later).  */

static bool
may_have_sip ()
{
  char str[16];
  size_t sz = sizeof (str);
  int ret;

  ret = sysctlbyname ("kern.osrelease", str, &sz, NULL, 0);
  if (ret == 0 && sz < sizeof (str))
    {
      unsigned long ver = strtoul (str, NULL, 10);
      if (ver >= 16)
	return true;
    }
  return false;
}

/* A helper for maybe_cache_shell.  This copies the shell to the
   cache.  It will throw an exception on any failure.  */

static void
copy_shell_to_cache (const char *shell, const std::string &new_name)
{
  scoped_fd from_fd = gdb_open_cloexec (shell, O_RDONLY, 0);
  if (from_fd.get () < 0)
    error (_("Could not open shell (%s) for reading: %s"),
	   shell, safe_strerror (errno));

  std::string new_dir = ldirname (new_name.c_str ());
  if (!mkdir_recursive (new_dir.c_str ()))
    error (_("Could not make cache directory \"%s\": %s"),
	   new_dir.c_str (), safe_strerror (errno));

  gdb::char_vector temp_name = make_temp_filename (new_name);
  scoped_fd to_fd = gdb_mkostemp_cloexec (&temp_name[0]);
  gdb::unlinker unlink_file_on_error (temp_name.data ());

  if (to_fd.get () < 0)
    error (_("Could not open temporary file \"%s\" for writing: %s"),
	   temp_name.data (), safe_strerror (errno));

  if (fcopyfile (from_fd.get (), to_fd.get (), nullptr,
		 COPYFILE_STAT | COPYFILE_DATA) != 0)
    error (_("Could not copy shell to cache as \"%s\": %s"),
	   temp_name.data (), safe_strerror (errno));

  /* Be sure that the caching is atomic so that we don't get bad
     results from multiple copies of gdb running at the same time.  */
  if (rename (temp_name.data (), new_name.c_str ()) != 0)
    error (_("Could not rename shell cache file to \"%s\": %s"),
	   new_name.c_str (), safe_strerror (errno));

  unlink_file_on_error.keep ();
}

/* If $SHELL is restricted, try to cache a copy.  Starting with El
   Capitan, macOS introduced System Integrity Protection.  Among other
   things, this prevents certain executables from being ptrace'd.  In
   particular, executables in /bin, like most shells, are affected.
   To work around this, while preserving command-line glob expansion
   and redirections, gdb will cache a copy of the shell.  Return true
   if all is well -- either the shell is not subject to SIP or it has
   been successfully cached.  Returns false if something failed.  */

static bool
maybe_cache_shell ()
{
  /* SF_RESTRICTED is defined in sys/stat.h and lets us determine if a
     given file is subject to SIP.  */
#ifdef SF_RESTRICTED

  /* If a check fails we want to revert -- maybe the user deleted the
     cache while gdb was running, or something like that.  */
  copied_shell = nullptr;

  const char *shell = get_shell ();
  if (!IS_ABSOLUTE_PATH (shell))
    {
      warning (_("This version of macOS has System Integrity Protection.\n\
Normally gdb would try to work around this by caching a copy of your shell,\n\
but because your shell (%s) is not an absolute path, this is being skipped."),
	       shell);
      return false;
    }

  struct stat sb;
  if (stat (shell, &sb) < 0)
    {
      warning (_("This version of macOS has System Integrity Protection.\n\
Normally gdb would try to work around this by caching a copy of your shell,\n\
but because gdb could not stat your shell (%s), this is being skipped.\n\
The error was: %s"),
	       shell, safe_strerror (errno));
      return false;
    }

  if ((sb.st_flags & SF_RESTRICTED) == 0)
    return true;

  /* Put the copy somewhere like ~/Library/Caches/gdb/bin/sh.  */
  std::string new_name = get_standard_cache_dir ();
  /* There's no need to insert a directory separator here, because
     SHELL is known to be absolute.  */
  new_name.append (shell);

  /* Maybe it was cached by some earlier gdb.  */
  if (stat (new_name.c_str (), &sb) != 0 || !S_ISREG (sb.st_mode))
    {
      try
	{
	  copy_shell_to_cache (shell, new_name);
	}
      catch (const gdb_exception_error &ex)
	{
	  warning (_("This version of macOS has System Integrity Protection.\n\
Because `startup-with-shell' is enabled, gdb tried to work around SIP by\n\
caching a copy of your shell.  However, this failed:\n\
%s\n\
If you correct the problem, gdb will automatically try again the next time\n\
you \"run\".  To prevent these attempts, you can use:\n\
    set startup-with-shell off"),
		   ex.what ());
	  return false;
	}

      gdb_printf (_("Note: this version of macOS has System Integrity Protection.\n\
Because `startup-with-shell' is enabled, gdb has worked around this by\n\
caching a copy of your shell.  The shell used by \"run\" is now:\n\
    %s\n"),
		  new_name.c_str ());
    }

  /* We need to make sure that the new name has the correct lifetime.  */
  static std::string saved_shell = std::move (new_name);
  copied_shell = saved_shell.c_str ();

#endif /* SF_RESTRICTED */

  return true;
}

void
darwin_nat_target::create_inferior (const char *exec_file,
				    const std::string &allargs,
				    char **env, int from_tty)
{
  std::optional<scoped_restore_tmpl<bool>> restore_startup_with_shell;
  darwin_nat_target *the_target = this;

  if (startup_with_shell && may_have_sip ())
    {
      if (!maybe_cache_shell ())
	{
	  warning (_("startup-with-shell is now temporarily disabled"));
	  restore_startup_with_shell.emplace (&startup_with_shell, 0);
	}
    }

  /* Do the hard work.  */
  fork_inferior (exec_file, allargs, env, darwin_ptrace_me,
		 [the_target] (int pid)
		   {
		     the_target->ptrace_him (pid);
		   },
		 darwin_pre_ptrace, copied_shell,
		 darwin_execvp);
}


/* Set things up such that the next call to darwin_wait will immediately
   return a fake stop event for inferior INF.

   This assumes that the inferior's thread list has been initialized,
   as it will suspend the inferior's first thread.  */

static void
darwin_setup_fake_stop_event (struct inferior *inf)
{
  darwin_inferior *priv = get_darwin_inferior (inf);
  darwin_thread_t *thread;
  kern_return_t kret;

  gdb_assert (darwin_inf_fake_stop == NULL);
  darwin_inf_fake_stop = inf;

  /* When detecting a fake pending stop event, darwin_wait returns
     an event saying that the first thread is in a DARWIN_STOPPED
     state.  To make that accurate, we need to suspend that thread
     as well.  Otherwise, we'll try resuming it when resuming the
     inferior, and get a warning because the thread's suspend count
     is already zero, making the resume request useless.  */
  thread = priv->threads[0];
  kret = thread_suspend (thread->gdb_port);
  MACH_CHECK_ERROR (kret);
}

/* Attach to process PID, then initialize for debugging it
   and wait for the trace-trap that results from attaching.  */
void
darwin_nat_target::attach (const char *args, int from_tty)
{
  pid_t pid;
  struct inferior *inf;

  pid = parse_pid_to_attach (args);

  if (pid == getpid ())		/* Trying to masturbate?  */
    error (_("I refuse to debug myself!"));

  target_announce_attach (from_tty, pid);

  if (pid == 0 || ::kill (pid, 0) < 0)
    error (_("Can't attach to process %d: %s (%d)"),
	   pid, safe_strerror (errno), errno);

  inf = current_inferior ();
  inferior_appeared (inf, pid);
  inf->attach_flag = true;

  darwin_attach_pid (inf);

  darwin_suspend_inferior (inf);

  init_thread_list (inf);

  darwin_inferior *priv = get_darwin_inferior (inf);

  darwin_check_osabi (priv, inferior_ptid.tid ());

  darwin_setup_fake_stop_event (inf);

  priv->no_ptrace = 1;
}

/* Take a program previously attached to and detaches it.
   The program resumes execution and will no longer stop
   on signals, etc.  We'd better not have left any breakpoints
   in the program or it'll die when it hits one.  For this
   to work, it may be necessary for the process to have been
   previously attached.  It *might* work if the program was
   started via fork.  */

void
darwin_nat_target::detach (inferior *inf, int from_tty)
{
  darwin_inferior *priv = get_darwin_inferior (inf);
  kern_return_t kret;
  int res;

  /* Display message.  */
  target_announce_detach (from_tty);

  /* If ptrace() is in use, stop the process.  */
  if (!priv->no_ptrace)
    stop_inferior (inf);

  kret = darwin_restore_exception_ports (priv);
  MACH_CHECK_ERROR (kret);

  if (!priv->no_ptrace)
    {
      res = PTRACE (PT_DETACH, inf->pid, 0, 0);
      if (res != 0)
	warning (_("Unable to detach from process-id %d: %s (%d)"),
		 inf->pid, safe_strerror (errno), errno);
    }

  darwin_reply_to_all_pending_messages (inf);

  /* When using ptrace, we have just performed a PT_DETACH, which
     resumes the inferior.  On the other hand, when we are not using
     ptrace, we need to resume its execution ourselves.  */
  if (priv->no_ptrace)
    darwin_resume_inferior (inf);

  mourn_inferior ();
}

std::string
darwin_nat_target::pid_to_str (ptid_t ptid)
{
  long tid = ptid.tid ();

  if (tid != 0)
    return string_printf (_("Thread 0x%lx of process %u"),
			  tid, ptid.pid ());

  return normal_pid_to_str (ptid);
}

bool
darwin_nat_target::thread_alive (ptid_t ptid)
{
  return true;
}

/* If RDADDR is not NULL, read inferior task's LEN bytes from ADDR and
   copy it to RDADDR in gdb's address space.
   If WRADDR is not NULL, write gdb's LEN bytes from WRADDR and copy it
   to ADDR in inferior task's address space.
   Return 0 on failure; number of bytes read / written otherwise.  */

static int
darwin_read_write_inferior (task_t task, CORE_ADDR addr,
			    gdb_byte *rdaddr, const gdb_byte *wraddr,
			    ULONGEST length)
{
  kern_return_t kret;
  mach_vm_size_t res_length = 0;

  inferior_debug (8, _("darwin_read_write_inferior(task=0x%x, %s, len=%s)\n"),
		  task, core_addr_to_string (addr), pulongest (length));

  /* First read.  */
  if (rdaddr != NULL)
    {
      mach_vm_size_t count;

      /* According to target.h(to_xfer_partial), one and only one may be
	 non-null.  */
      gdb_assert (wraddr == NULL);

      kret = mach_vm_read_overwrite (task, addr, length,
				     (mach_vm_address_t) rdaddr, &count);
      if (kret != KERN_SUCCESS)
	{
	  inferior_debug
	    (1, _("darwin_read_write_inferior: mach_vm_read failed at %s: %s"),
	     core_addr_to_string (addr), mach_error_string (kret));
	  return 0;
	}
      return count;
    }

  /* See above.  */
  gdb_assert (wraddr != NULL);

  while (length != 0)
    {
      mach_vm_address_t offset = addr & (mach_page_size - 1);
      mach_vm_address_t region_address = (mach_vm_address_t) (addr - offset);
      mach_vm_size_t aligned_length =
	(mach_vm_size_t) PAGE_ROUND (offset + length);
      vm_region_submap_short_info_data_64_t info;
      mach_msg_type_number_t count = VM_REGION_SUBMAP_SHORT_INFO_COUNT_64;
      natural_t region_depth = 1000;
      mach_vm_address_t region_start = region_address;
      mach_vm_size_t region_length;
      mach_vm_size_t write_length;

      /* Read page protection.  */
      kret = mach_vm_region_recurse
	(task, &region_start, &region_length, &region_depth,
	 (vm_region_recurse_info_t) &info, &count);

      if (kret != KERN_SUCCESS)
	{
	  inferior_debug (1, _("darwin_read_write_inferior: "
			       "mach_vm_region_recurse failed at %s: %s\n"),
			  core_addr_to_string (region_address),
			  mach_error_string (kret));
	  return res_length;
	}

      inferior_debug
	(9, _("darwin_read_write_inferior: "
	      "mach_vm_region_recurse addr=%s, start=%s, len=%s\n"),
	 core_addr_to_string (region_address),
	 core_addr_to_string (region_start),
	 core_addr_to_string (region_length));

      /* Check for holes in memory.  */
      if (region_start > region_address)
	{
	  warning (_("No memory at %s (vs %s+0x%x).  Nothing written"),
		   core_addr_to_string (region_address),
		   core_addr_to_string (region_start),
		   (unsigned)region_length);
	  return res_length;
	}

      /* Adjust the length.  */
      region_length -= (region_address - region_start);
      if (region_length > aligned_length)
	region_length = aligned_length;

      /* Make the pages RW.  */
      if (!(info.protection & VM_PROT_WRITE))
	{
	  vm_prot_t prot = VM_PROT_READ | VM_PROT_WRITE;

	  kret = mach_vm_protect (task, region_address, region_length,
				  FALSE, prot);
	  if (kret != KERN_SUCCESS)
	    {
	      prot |= VM_PROT_COPY;
	      kret = mach_vm_protect (task, region_address, region_length,
				      FALSE, prot);
	    }
	  if (kret != KERN_SUCCESS)
	    {
	      warning (_("darwin_read_write_inferior: "
			 "mach_vm_protect failed at %s "
			 "(len=0x%lx, prot=0x%x): %s"),
		       core_addr_to_string (region_address),
		       (unsigned long) region_length, (unsigned) prot,
		       mach_error_string (kret));
	      return res_length;
	    }
	}

      if (offset + length > region_length)
	write_length = region_length - offset;
      else
	write_length = length;

      /* Write.  */
      kret = mach_vm_write (task, addr, (vm_offset_t) wraddr, write_length);
      if (kret != KERN_SUCCESS)
	{
	  warning (_("darwin_read_write_inferior: mach_vm_write failed: %s"),
		   mach_error_string (kret));
	  return res_length;
	}

      /* Restore page rights.  */
      if (!(info.protection & VM_PROT_WRITE))
	{
	  kret = mach_vm_protect (task, region_address, region_length,
				  FALSE, info.protection);
	  if (kret != KERN_SUCCESS)
	    {
	      warning (_("darwin_read_write_inferior: "
			 "mach_vm_protect restore failed at %s "
			 "(len=0x%lx): %s"),
		       core_addr_to_string (region_address),
		       (unsigned long) region_length,
		       mach_error_string (kret));
	    }
	}

      addr += write_length;
      wraddr += write_length;
      res_length += write_length;
      length -= write_length;
    }

  return res_length;
}

/* Read LENGTH bytes at offset ADDR of task_dyld_info for TASK, and copy them
   to RDADDR (in big endian).
   Return 0 on failure; number of bytes read / written otherwise.  */

#ifdef TASK_DYLD_INFO_COUNT
/* This is not available in Darwin 9.  */
static enum target_xfer_status
darwin_read_dyld_info (task_t task, CORE_ADDR addr, gdb_byte *rdaddr,
		       ULONGEST length, ULONGEST *xfered_len)
{
  struct task_dyld_info task_dyld_info;
  mach_msg_type_number_t count = TASK_DYLD_INFO_COUNT;
  kern_return_t kret;

  if (addr != 0 || length > sizeof (mach_vm_address_t))
    return TARGET_XFER_EOF;

  kret = task_info (task, TASK_DYLD_INFO,
		    (task_info_t) &task_dyld_info, &count);
  MACH_CHECK_ERROR (kret);
  if (kret != KERN_SUCCESS)
    return TARGET_XFER_E_IO;

  store_unsigned_integer (rdaddr, length, BFD_ENDIAN_BIG,
			  task_dyld_info.all_image_info_addr);
  *xfered_len = (ULONGEST) length;
  return TARGET_XFER_OK;
}
#endif



enum target_xfer_status
darwin_nat_target::xfer_partial (enum target_object object, const char *annex,
				 gdb_byte *readbuf, const gdb_byte *writebuf,
				 ULONGEST offset, ULONGEST len,
				 ULONGEST *xfered_len)
{
  struct inferior *inf = current_inferior ();
  darwin_inferior *priv = get_darwin_inferior (inf);

  inferior_debug
    (8, _("darwin_xfer_partial(%s, %s, rbuf=%s, wbuf=%s) pid=%u\n"),
     core_addr_to_string (offset), pulongest (len),
     host_address_to_string (readbuf), host_address_to_string (writebuf),
     inf->pid);

  switch (object)
    {
    case TARGET_OBJECT_MEMORY:
      {
	int l = darwin_read_write_inferior (priv->task, offset,
					    readbuf, writebuf, len);

	if (l == 0)
	  return TARGET_XFER_EOF;
	else
	  {
	    gdb_assert (l > 0);
	    *xfered_len = (ULONGEST) l;
	    return TARGET_XFER_OK;
	  }
      }
#ifdef TASK_DYLD_INFO_COUNT
    case TARGET_OBJECT_DARWIN_DYLD_INFO:
      if (writebuf != NULL || readbuf == NULL)
	{
	  /* Support only read.  */
	  return TARGET_XFER_E_IO;
	}
      return darwin_read_dyld_info (priv->task, offset, readbuf, len,
				    xfered_len);
#endif
    default:
      return TARGET_XFER_E_IO;
    }

}

static void
set_enable_mach_exceptions (const char *args, int from_tty,
			    struct cmd_list_element *c)
{
  if (inferior_ptid != null_ptid)
    {
      struct inferior *inf = current_inferior ();
      darwin_inferior *priv = get_darwin_inferior (inf);
      exception_mask_t mask;
      kern_return_t kret;

      if (enable_mach_exceptions)
	mask = EXC_MASK_ALL;
      else
	{
	  darwin_restore_exception_ports (priv);
	  mask = EXC_MASK_SOFTWARE | EXC_MASK_BREAKPOINT;
	}
      kret = task_set_exception_ports (priv->task, mask, darwin_ex_port,
				       EXCEPTION_DEFAULT, THREAD_STATE_NONE);
      MACH_CHECK_ERROR (kret);
    }
}

const char *
darwin_nat_target::pid_to_exec_file (int pid)
{
  static char path[PATH_MAX];
  int res;

  res = proc_pidinfo (pid, PROC_PIDPATHINFO, 0, path, PATH_MAX);
  if (res >= 0)
    return path;
  else
    return NULL;
}

ptid_t
darwin_nat_target::get_ada_task_ptid (long lwp, ULONGEST thread)
{
  struct inferior *inf = current_inferior ();
  darwin_inferior *priv = get_darwin_inferior (inf);
  kern_return_t kret;
  mach_port_name_array_t names;
  mach_msg_type_number_t names_count;
  mach_port_type_array_t types;
  mach_msg_type_number_t types_count;
  long res = 0;

  /* First linear search.  */
  for (darwin_thread_t *t : priv->threads)
    {
      if (t->inf_port == lwp)
	return ptid_t (inferior_ptid.pid (), 0, t->gdb_port);
    }

  /* Maybe the port was never extract.  Do it now.  */

  /* First get inferior port names.  */
  kret = mach_port_names (priv->task, &names, &names_count, &types,
			  &types_count);
  MACH_CHECK_ERROR (kret);
  if (kret != KERN_SUCCESS)
    return null_ptid;

  /* For each name, copy the right in the gdb space and then compare with
     our view of the inferior threads.  We don't forget to deallocate the
     right.  */
  for (int i = 0; i < names_count; i++)
    {
      mach_port_t local_name;
      mach_msg_type_name_t local_type;

      /* We just need to know the corresponding name in gdb name space.
	 So extract and deallocate the right.  */
      kret = mach_port_extract_right (priv->task, names[i],
				      MACH_MSG_TYPE_COPY_SEND,
				      &local_name, &local_type);
      if (kret != KERN_SUCCESS)
	continue;
      mach_port_deallocate (gdb_task, local_name);

      for (darwin_thread_t *t : priv->threads)
	{
	  if (t->gdb_port == local_name)
	    {
	      t->inf_port = names[i];
	      if (names[i] == lwp)
		res = t->gdb_port;
	    }
	}
    }

  vm_deallocate (gdb_task, (vm_address_t) names,
		 names_count * sizeof (mach_port_t));

  if (res)
    return ptid_t (current_inferior ()->pid, 0, res);
  else
    return null_ptid;
}

bool
darwin_nat_target::supports_multi_process ()
{
  return true;
}

void _initialize_darwin_nat ();
void
_initialize_darwin_nat ()
{
  kern_return_t kret;

  gdb_task = mach_task_self ();
  darwin_host_self = mach_host_self ();

  /* Read page size.  */
  kret = host_page_size (darwin_host_self, &mach_page_size);
  if (kret != KERN_SUCCESS)
    {
      mach_page_size = 0x1000;
      MACH_CHECK_ERROR (kret);
    }

  inferior_debug (2, _("GDB task: 0x%lx, pid: %d\n"),
		  (unsigned long) mach_task_self (), getpid ());

  add_setshow_zuinteger_cmd ("darwin", class_obscure,
			     &darwin_debug_flag, _("\
Set if printing inferior communication debugging statements."), _("\
Show if printing inferior communication debugging statements."), NULL,
			     NULL, NULL,
			     &setdebuglist, &showdebuglist);

  add_setshow_boolean_cmd ("mach-exceptions", class_support,
			   &enable_mach_exceptions, _("\
Set if mach exceptions are caught."), _("\
Show if mach exceptions are caught."), _("\
When this mode is on, all low level exceptions are reported before being\n\
reported by the kernel."),
			   &set_enable_mach_exceptions, NULL,
			   &setlist, &showlist);
}
