/* Common things used by the various darwin files
   Copyright (C) 1995-2024 Free Software Foundation, Inc.

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

#ifndef DARWIN_NAT_H
#define DARWIN_NAT_H

#include "inf-child.h"
#include <mach/mach.h>
#include "gdbthread.h"

struct darwin_exception_msg
{
  mach_msg_header_t header;

  /* Thread and task taking the exception.  */
  mach_port_t thread_port;
  mach_port_t task_port;

  /* Type of the exception.  */
  exception_type_t ex_type;

  /* Machine dependent details.  */
  mach_msg_type_number_t data_count;
  integer_t ex_data[2];
};

enum darwin_msg_state
{
  /* The thread is running.  */
  DARWIN_RUNNING,

  /* The thread is stopped.  */
  DARWIN_STOPPED,

  /* The thread has sent a message and waits for a reply.  */
  DARWIN_MESSAGE
};

struct darwin_thread_info : public private_thread_info
{
  /* The thread port from a GDB point of view.  */
  thread_t gdb_port = 0;

  /* The thread port from the inferior point of view.  Not to be used inside
     gdb except for get_ada_task_ptid.  */
  thread_t inf_port = 0;

  /* Current message state.
     If the kernel has sent a message it expects a reply and the inferior
     can't be killed before.  */
  enum darwin_msg_state msg_state = DARWIN_RUNNING;

  /* True if this thread is single-stepped.  */
  bool single_step = false;

  /* True if a signal was manually sent to the thread.  */
  bool signaled = false;

  /* The last exception received.  */
  struct darwin_exception_msg event {};
};
typedef struct darwin_thread_info darwin_thread_t;

/* This needs to be overridden by the platform specific nat code.  */

class darwin_nat_target : public inf_child_target
{
  void create_inferior (const char *exec_file,
			const std::string &allargs,
			char **env, int from_tty) override;

  void attach (const char *, int) override;

  void detach (inferior *, int) override;

  ptid_t wait (ptid_t, struct target_waitstatus *, target_wait_flags) override;

  void mourn_inferior () override;

  void kill () override;

  void interrupt () override;

  void resume (ptid_t, int , enum gdb_signal) override;

  bool thread_alive (ptid_t ptid) override;

  std::string pid_to_str (ptid_t) override;

  const char *pid_to_exec_file (int pid) override;

  enum target_xfer_status xfer_partial (enum target_object object,
					const char *annex,
					gdb_byte *readbuf,
					const gdb_byte *writebuf,
					ULONGEST offset, ULONGEST len,
					ULONGEST *xfered_len) override;

  bool supports_multi_process () override;

  ptid_t get_ada_task_ptid (long lwp, ULONGEST thread) override;

private:
  ptid_t wait_1 (ptid_t, struct target_waitstatus *);
  void check_new_threads (inferior *inf);
  int decode_exception_message (mach_msg_header_t *hdr,
				inferior **pinf,
				darwin_thread_t **pthread);
  ptid_t decode_message (mach_msg_header_t *hdr,
			 darwin_thread_t **pthread,
			 inferior **pinf,
			 target_waitstatus *status);
  void stop_inferior (inferior *inf);
  void init_thread_list (inferior *inf);
  void ptrace_him (int pid);
  int cancel_breakpoint (inferior *inf, ptid_t ptid);
};

/* Describe the mach exception handling state for a task.  This state is saved
   before being changed and restored when a process is detached.
   For more information on these fields see task_get_exception_ports manual
   page.  */
struct darwin_exception_info
{
  /* Exceptions handled by the port.  */
  exception_mask_t masks[EXC_TYPES_COUNT] {};

  /* Ports receiving exception messages.  */
  mach_port_t ports[EXC_TYPES_COUNT] {};

  /* Type of messages sent.  */
  exception_behavior_t behaviors[EXC_TYPES_COUNT] {};

  /* Type of state to be sent.  */
  thread_state_flavor_t flavors[EXC_TYPES_COUNT] {};

  /* Number of elements set.  */
  mach_msg_type_number_t count = 0;
};

static inline darwin_thread_info *
get_darwin_thread_info (class thread_info *thread)
{
  return gdb::checked_static_cast<darwin_thread_info *> (thread->priv.get ());
}

/* Describe an inferior.  */
struct darwin_inferior : public private_inferior
{
  /* Corresponding task port.  */
  task_t task = 0;

  /* Port which will receive the dead-name notification for the task port.
     This is used to detect the death of the task.  */
  mach_port_t notify_port = 0;

  /* Initial exception handling.  */
  darwin_exception_info exception_info;

  /* Number of messages that have been received but not yet replied.  */
  unsigned int pending_messages = 0;

  /* Set if inferior is not controlled by ptrace(2) but through Mach.  */
  bool no_ptrace = false;

  /* True if this task is suspended.  */
  bool suspended = false;

  /* Sorted vector of known threads.  */
  std::vector<darwin_thread_t *> threads;
};

/* Return the darwin_inferior attached to INF.  */

static inline darwin_inferior *
get_darwin_inferior (inferior *inf)
{
  return gdb::checked_static_cast<darwin_inferior *> (inf->priv.get ());
}

/* Exception port.  */
extern mach_port_t darwin_ex_port;

/* Port set.  */
extern mach_port_t darwin_port_set;

/* A copy of mach_host_self ().  */
extern mach_port_t darwin_host_self;

#define MACH_CHECK_ERROR(ret) \
  mach_check_error (ret, __FILE__, __LINE__, __func__)

extern void mach_check_error (kern_return_t ret, const char *file,
			      unsigned int line, const char *func);

void darwin_set_sstep (thread_t thread, int enable);

void darwin_check_osabi (darwin_inferior *inf, thread_t thread);

#endif /* DARWIN_NAT_H */
