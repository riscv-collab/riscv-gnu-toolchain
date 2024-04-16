/* Target operations for the remote server for GDB.
   Copyright (C) 2002-2024 Free Software Foundation, Inc.

   Contributed by MontaVista Software.

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

#ifndef GDBSERVER_TARGET_H
#define GDBSERVER_TARGET_H

#include <sys/types.h>
#include "target/target.h"
#include "target/resume.h"
#include "target/wait.h"
#include "target/waitstatus.h"
#include "mem-break.h"
#include "gdbsupport/array-view.h"
#include "gdbsupport/btrace-common.h"
#include <vector>
#include "gdbsupport/byte-vector.h"

struct emit_ops;
struct process_info;

/* This structure describes how to resume a particular thread (or all
   threads) based on the client's request.  If thread is -1, then this
   entry applies to all threads.  These are passed around as an
   array.  */

struct thread_resume
{
  ptid_t thread;

  /* How to "resume".  */
  enum resume_kind kind;

  /* If non-zero, send this signal when we resume, or to stop the
     thread.  If stopping a thread, and this is 0, the target should
     stop the thread however it best decides to (e.g., SIGSTOP on
     linux; SuspendThread on win32).  This is a host signal value (not
     enum gdb_signal).  */
  int sig;

  /* Range to single step within.  Valid only iff KIND is resume_step.

     Single-step once, and then continuing stepping as long as the
     thread stops in this range.  (If the range is empty
     [STEP_RANGE_START == STEP_RANGE_END], then this is a single-step
     request.)  */
  CORE_ADDR step_range_start;	/* Inclusive */
  CORE_ADDR step_range_end;	/* Exclusive */
};

/* GDBserver doesn't have a concept of strata like GDB, but we call
   its target vector "process_stratum" anyway for the benefit of
   shared code.  */

class process_stratum_target
{
public:

  virtual ~process_stratum_target () = default;

  /* Start a new process.

     PROGRAM is a path to the program to execute.
     PROGRAM_ARGS is a standard NULL-terminated array of arguments,
     to be passed to the inferior as ``argv'' (along with PROGRAM).

     Returns the new PID on success, -1 on failure.  Registers the new
     process with the process list.  */
  virtual int create_inferior (const char *program,
			       const std::vector<char *> &program_args) = 0;

  /* Do additional setup after a new process is created, including
     exec-wrapper completion.  */
  virtual void post_create_inferior ();

  /* Attach to a running process.

     PID is the process ID to attach to, specified by the user
     or a higher layer.

     Returns -1 if attaching is unsupported, 0 on success, and calls
     error() otherwise.  */
  virtual int attach (unsigned long pid) = 0;

  /* Kill process PROC.  Return -1 on failure, and 0 on success.  */
  virtual int kill (process_info *proc) = 0;

  /* Detach from process PROC.  Return -1 on failure, and 0 on
     success.  */
  virtual int detach (process_info *proc) = 0;

  /* The inferior process has died.  Do what is right.  */
  virtual void mourn (process_info *proc) = 0;

  /* Wait for process PID to exit.  */
  virtual void join (int pid) = 0;

  /* Return true iff the thread with process ID PID is alive.  */
  virtual bool thread_alive (ptid_t pid) = 0;

  /* Resume the inferior process.  */
  virtual void resume (thread_resume *resume_info, size_t n) = 0;

  /* Wait for the inferior process or thread to change state.  Store
     status through argument pointer STATUS.

     PTID = -1 to wait for any pid to do something, PTID(pid,0,0) to
     wait for any thread of process pid to do something.  Return ptid
     of child, or -1 in case of error; store status through argument
     pointer STATUS.  OPTIONS is a bit set of options defined as
     TARGET_W* above.  If options contains TARGET_WNOHANG and there's
     no child stop to report, return is
     null_ptid/TARGET_WAITKIND_IGNORE.  */
  virtual ptid_t wait (ptid_t ptid, target_waitstatus *status,
		       target_wait_flags options) = 0;

  /* Fetch registers from the inferior process.

     If REGNO is -1, fetch all registers; otherwise, fetch at least REGNO.  */
  virtual void fetch_registers (regcache *regcache, int regno) = 0;

  /* Store registers to the inferior process.

     If REGNO is -1, store all registers; otherwise, store at least REGNO.  */
  virtual void store_registers (regcache *regcache, int regno) = 0;

  /* Read memory from the inferior process.  This should generally be
     called through read_inferior_memory, which handles breakpoint shadowing.

     Read LEN bytes at MEMADDR into a buffer at MYADDR.

     Returns 0 on success and errno on failure.  */
  virtual int read_memory (CORE_ADDR memaddr, unsigned char *myaddr,
			   int len) = 0;

  /* Write memory to the inferior process.  This should generally be
     called through target_write_memory, which handles breakpoint shadowing.

     Write LEN bytes from the buffer at MYADDR to MEMADDR.

     Returns 0 on success and errno on failure.  */
  virtual int write_memory (CORE_ADDR memaddr, const unsigned char *myaddr,
			    int len) = 0;

  /* Query GDB for the values of any symbols we're interested in.
     This function is called whenever we receive a "qSymbols::"
     query, which corresponds to every time more symbols (might)
     become available.  */
  virtual void look_up_symbols ();

  /* Send an interrupt request to the inferior process,
     however is appropriate.  */
  virtual void request_interrupt () = 0;

  /* Return true if the read_auxv target op is supported.  */
  virtual bool supports_read_auxv ();

  /* Read auxiliary vector data from the process with pid PID.

     Read LEN bytes at OFFSET into a buffer at MYADDR.  */
  virtual int read_auxv (int pid, CORE_ADDR offset, unsigned char *myaddr,
			 unsigned int len);

  /* Returns true if GDB Z breakpoint type TYPE is supported, false
     otherwise.  The type is coded as follows:
       '0' - software-breakpoint
       '1' - hardware-breakpoint
       '2' - write watchpoint
       '3' - read watchpoint
       '4' - access watchpoint
  */
  virtual bool supports_z_point_type (char z_type);

  /* Insert and remove a break or watchpoint.
     Returns 0 on success, -1 on failure and 1 on unsupported.  */
  virtual int insert_point (enum raw_bkpt_type type, CORE_ADDR addr,
			    int size, raw_breakpoint *bp);

  virtual int remove_point (enum raw_bkpt_type type, CORE_ADDR addr,
			    int size, raw_breakpoint *bp);

  /* Returns true if the target stopped because it executed a software
     breakpoint instruction, false otherwise.  */
  virtual bool stopped_by_sw_breakpoint ();

  /* Returns true if the target knows whether a trap was caused by a
     SW breakpoint triggering.  */
  virtual bool supports_stopped_by_sw_breakpoint ();

  /* Returns true if the target stopped for a hardware breakpoint.  */
  virtual bool stopped_by_hw_breakpoint ();

  /* Returns true if the target knows whether a trap was caused by a
     HW breakpoint triggering.  */
  virtual bool supports_stopped_by_hw_breakpoint ();

  /* Returns true if the target can do hardware single step.  */
  virtual bool supports_hardware_single_step ();

  /* Returns true if target was stopped due to a watchpoint hit, false
     otherwise.  */
  virtual bool stopped_by_watchpoint ();

  /* Returns the address associated with the watchpoint that hit, if any;
     returns 0 otherwise.  */
  virtual CORE_ADDR stopped_data_address ();

  /* Return true if the read_offsets target op is supported.  */
  virtual bool supports_read_offsets ();

  /* Reports the text, data offsets of the executable.  This is
     needed for uclinux where the executable is relocated during load
     time.  */
  virtual int read_offsets (CORE_ADDR *text, CORE_ADDR *data);

  /* Return true if the get_tls_address target op is supported.  */
  virtual bool supports_get_tls_address ();

  /* Fetch the address associated with a specific thread local storage
     area, determined by the specified THREAD, OFFSET, and LOAD_MODULE.
     Stores it in *ADDRESS and returns zero on success; otherwise returns
     an error code.  A return value of -1 means this system does not
     support the operation.  */
  virtual int get_tls_address (thread_info *thread, CORE_ADDR offset,
			       CORE_ADDR load_module, CORE_ADDR *address);

  /* Return true if the qxfer_osdata target op is supported.  */
  virtual bool supports_qxfer_osdata ();

  /* Read/Write OS data using qXfer packets.  */
  virtual int qxfer_osdata (const char *annex, unsigned char *readbuf,
			    unsigned const char *writebuf,
			    CORE_ADDR offset, int len);

  /* Return true if the qxfer_siginfo target op is supported.  */
  virtual bool supports_qxfer_siginfo ();

  /* Read/Write extra signal info.  */
  virtual int qxfer_siginfo (const char *annex, unsigned char *readbuf,
			     unsigned const char *writebuf,
			     CORE_ADDR offset, int len);

  /* Return true if non-stop mode is supported.  */
  virtual bool supports_non_stop ();

  /* Enables async target events.  Returns the previous enable
     state.  */
  virtual bool async (bool enable);

  /* Switch to non-stop (ENABLE == true) or all-stop (ENABLE == false)
     mode.  Return 0 on success, -1 otherwise.  */
  virtual int start_non_stop (bool enable);

  /* Returns true if the target supports multi-process debugging.  */
  virtual bool supports_multi_process ();

  /* Returns true if fork events are supported.  */
  virtual bool supports_fork_events ();

  /* Returns true if vfork events are supported.  */
  virtual bool supports_vfork_events ();

  /* Returns the set of supported thread options.  */
  virtual gdb_thread_options supported_thread_options ();

  /* Returns true if exec events are supported.  */
  virtual bool supports_exec_events ();

  /* Allows target to re-initialize connection-specific settings.  */
  virtual void handle_new_gdb_connection ();

  /* The target-specific routine to process monitor command.
     Returns 1 if handled, or 0 to perform default processing.  */
  virtual int handle_monitor_command (char *mon);

  /* Returns the core given a thread, or -1 if not known.  */
  virtual int core_of_thread (ptid_t ptid);

  /* Returns true if the read_loadmap target op is supported.  */
  virtual bool supports_read_loadmap ();

  /* Read loadmaps.  Read LEN bytes at OFFSET into a buffer at MYADDR.  */
  virtual int read_loadmap (const char *annex, CORE_ADDR offset,
			    unsigned char *myaddr, unsigned int len);

  /* Target specific qSupported support.  FEATURES is an array of
     features unsupported by the core of GDBserver.  */
  virtual void process_qsupported
    (gdb::array_view<const char * const> features);

  /* Return true if the target supports tracepoints, false otherwise.  */
  virtual bool supports_tracepoints ();

  /* Read PC from REGCACHE.  */
  virtual CORE_ADDR read_pc (regcache *regcache);

  /* Write PC to REGCACHE.  */
  virtual void write_pc (regcache *regcache, CORE_ADDR pc);

  /* Return true if the thread_stopped op is supported.  */
  virtual bool supports_thread_stopped ();

  /* Return true if THREAD is known to be stopped now.  */
  virtual bool thread_stopped (thread_info *thread);

  /* Return true if any thread is known to be resumed.  */
  virtual bool any_resumed ();

  /* Return true if the get_tib_address op is supported.  */
  virtual bool supports_get_tib_address ();

  /* Read Thread Information Block address.  */
  virtual int get_tib_address (ptid_t ptid, CORE_ADDR *address);

  /* Pause all threads.  If FREEZE, arrange for any resume attempt to
     be ignored until an unpause_all call unfreezes threads again.
     There can be nested calls to pause_all, so a freeze counter
     should be maintained.  */
  virtual void pause_all (bool freeze);

  /* Unpause all threads.  Threads that hadn't been resumed by the
     client should be left stopped.  Basically a pause/unpause call
     pair should not end up resuming threads that were stopped before
     the pause call.  */
  virtual void unpause_all (bool unfreeze);

  /* Stabilize all threads.  That is, force them out of jump pads.  */
  virtual void stabilize_threads ();

  /* Return true if the install_fast_tracepoint_jump_pad op is
     supported.  */
  virtual bool supports_fast_tracepoints ();

  /* Install a fast tracepoint jump pad.  TPOINT is the address of the
     tracepoint internal object as used by the IPA agent.  TPADDR is
     the address of tracepoint.  COLLECTOR is address of the function
     the jump pad redirects to.  LOCKADDR is the address of the jump
     pad lock object.  ORIG_SIZE is the size in bytes of the
     instruction at TPADDR.  JUMP_ENTRY points to the address of the
     jump pad entry, and on return holds the address past the end of
     the created jump pad.  If a trampoline is created by the function,
     then TRAMPOLINE and TRAMPOLINE_SIZE return the address and size of
     the trampoline, else they remain unchanged.  JJUMP_PAD_INSN is a
     buffer containing a copy of the instruction at TPADDR.
     ADJUST_INSN_ADDR and ADJUST_INSN_ADDR_END are output parameters that
     return the address range where the instruction at TPADDR was relocated
     to.  If an error occurs, the ERR may be used to pass on an error
     message.  */
  virtual int install_fast_tracepoint_jump_pad
    (CORE_ADDR tpoint, CORE_ADDR tpaddr, CORE_ADDR collector,
     CORE_ADDR lockaddr, ULONGEST orig_size, CORE_ADDR *jump_entry,
     CORE_ADDR *trampoline, ULONGEST *trampoline_size,
     unsigned char *jjump_pad_insn, ULONGEST *jjump_pad_insn_size,
     CORE_ADDR *adjusted_insn_addr, CORE_ADDR *adjusted_insn_addr_end,
     char *err);

  /* Return the minimum length of an instruction that can be safely
     overwritten for use as a fast tracepoint.  */
  virtual int get_min_fast_tracepoint_insn_len ();

  /* Return the bytecode operations vector for the current inferior.
     Returns nullptr if bytecode compilation is not supported.  */
  virtual struct emit_ops *emit_ops ();

  /* Returns true if the target supports disabling randomization.  */
  virtual bool supports_disable_randomization ();

  /* Return true if the qxfer_libraries_svr4 op is supported.  */
  virtual bool supports_qxfer_libraries_svr4 ();

  /* Read solib info on SVR4 platforms.  */
  virtual int qxfer_libraries_svr4 (const char *annex,
				    unsigned char *readbuf,
				    unsigned const char *writebuf,
				    CORE_ADDR offset, int len);

  /* Return true if target supports debugging agent.  */
  virtual bool supports_agent ();

  /* Return true if target supports btrace.  */
  virtual bool supports_btrace ();

  /* Enable branch tracing for TP based on CONF and allocate a branch trace
     target information struct for reading and for disabling branch trace.  */
  virtual btrace_target_info *enable_btrace (thread_info *tp,
					     const btrace_config *conf);

  /* Disable branch tracing.
     Returns zero on success, non-zero otherwise.  */
  virtual int disable_btrace (btrace_target_info *tinfo);

  /* Read branch trace data into buffer.
     Return 0 on success; print an error message into BUFFER and return -1,
     otherwise.  */
  virtual int read_btrace (btrace_target_info *tinfo, std::string *buf,
			   enum btrace_read_type type);

  /* Read the branch trace configuration into BUFFER.
     Return 0 on success; print an error message into BUFFER and return -1
     otherwise.  */
  virtual int read_btrace_conf (const btrace_target_info *tinfo,
				std::string *buf);

  /* Return true if target supports range stepping.  */
  virtual bool supports_range_stepping ();

  /* Return true if the pid_to_exec_file op is supported.  */
  virtual bool supports_pid_to_exec_file ();

  /* Return the full absolute name of the executable file that was
     run to create the process PID.  If the executable file cannot
     be determined, NULL is returned.  Otherwise, a pointer to a
     character string containing the pathname is returned.  This
     string should be copied into a buffer by the client if the string
     will not be immediately used, or if it must persist.  */
  virtual const char *pid_to_exec_file (int pid);

  /* Return true if any of the multifs ops is supported.  */
  virtual bool supports_multifs ();

  /* Multiple-filesystem-aware open.  Like open(2), but operating in
     the filesystem as it appears to process PID.  Systems where all
     processes share a common filesystem should not override this.
     The default behavior is to use open(2).  */
  virtual int multifs_open (int pid, const char *filename,
			    int flags, mode_t mode);

  /* Multiple-filesystem-aware unlink.  Like unlink(2), but operates
     in the filesystem as it appears to process PID.  Systems where
     all processes share a common filesystem should not override this.
     The default behavior is to use unlink(2).  */
  virtual int multifs_unlink (int pid, const char *filename);

  /* Multiple-filesystem-aware readlink.  Like readlink(2), but
     operating in the filesystem as it appears to process PID.
     Systems where all processes share a common filesystem should
     not override this.  The default behavior is to use readlink(2).  */
  virtual ssize_t multifs_readlink (int pid, const char *filename,
				    char *buf, size_t bufsiz);

  /* Return the breakpoint kind for this target based on PC.  The
     PCPTR is adjusted to the real memory location in case a flag
     (e.g., the Thumb bit on ARM) was present in the PC.  */
  virtual int breakpoint_kind_from_pc (CORE_ADDR *pcptr);

  /* Return the software breakpoint from KIND.  KIND can have target
     specific meaning like the Z0 kind parameter.
     SIZE is set to the software breakpoint's length in memory.  */
  virtual const gdb_byte *sw_breakpoint_from_kind (int kind, int *size) = 0;

  /* Return the breakpoint kind for this target based on the current
     processor state (e.g. the current instruction mode on ARM) and the
     PC.  The PCPTR is adjusted to the real memory location in case a
     flag (e.g., the Thumb bit on ARM) is present in the  PC.  */
  virtual int breakpoint_kind_from_current_state (CORE_ADDR *pcptr);

  /* Return the thread's name, or NULL if the target is unable to
     determine it.  The returned value must not be freed by the
     caller.  */
  virtual const char *thread_name (ptid_t thread);

  /* Thread ID to (numeric) thread handle: Return true on success and
     false for failure.  Return pointer to thread handle via HANDLE
     and the handle's length via HANDLE_LEN.  */
  virtual bool thread_handle (ptid_t ptid, gdb_byte **handle,
			      int *handle_len);

  /* If THREAD is a fork/vfork/clone child that was not reported to
     GDB, return its parent else nullptr.  */
  virtual thread_info *thread_pending_parent (thread_info *thread);

  /* If THREAD is the parent of a fork/vfork/clone child that was not
     reported to GDB, return this child and fill in KIND with the
     matching waitkind, otherwise nullptr.  */
  virtual thread_info *thread_pending_child (thread_info *thread,
					     target_waitkind *kind);

  /* Returns true if the target can software single step.  */
  virtual bool supports_software_single_step ();

  /* Return true if the target supports catch syscall.  */
  virtual bool supports_catch_syscall ();

  /* Return tdesc index for IPA.  */
  virtual int get_ipa_tdesc_idx ();

  /* Returns true if the target supports memory tagging facilities.  */
  virtual bool supports_memory_tagging ();

  /* Return the allocated memory tags of type TYPE associated with
     [ADDRESS, ADDRESS + LEN) in TAGS.

     Returns true if successful and false otherwise.  */
  virtual bool fetch_memtags (CORE_ADDR address, size_t len,
			      gdb::byte_vector &tags, int type);

  /* Write the allocation tags of type TYPE contained in TAGS to the
     memory range [ADDRESS, ADDRESS + LEN).

     Returns true if successful and false otherwise.  */
  virtual bool store_memtags (CORE_ADDR address, size_t len,
			      const gdb::byte_vector &tags, int type);
};

extern process_stratum_target *the_target;

void set_target_ops (process_stratum_target *);

#define target_create_inferior(program, program_args)	\
  the_target->create_inferior (program, program_args)

#define target_post_create_inferior()			 \
  the_target->post_create_inferior ()

#define myattach(pid) \
  the_target->attach (pid)

int kill_inferior (process_info *proc);

#define target_supports_fork_events() \
  the_target->supports_fork_events ()

#define target_supports_vfork_events() \
  the_target->supports_vfork_events ()

#define target_supported_thread_options(options) \
  the_target->supported_thread_options (options)

#define target_supports_exec_events() \
  the_target->supports_exec_events ()

#define target_supports_memory_tagging() \
  the_target->supports_memory_tagging ()

#define target_handle_new_gdb_connection()		 \
  the_target->handle_new_gdb_connection ()

#define detach_inferior(proc) \
  the_target->detach (proc)

#define mythread_alive(pid) \
  the_target->thread_alive (pid)

#define fetch_inferior_registers(regcache, regno)	\
  the_target->fetch_registers (regcache, regno)

#define store_inferior_registers(regcache, regno) \
  the_target->store_registers (regcache, regno)

#define join_inferior(pid) \
  the_target->join (pid)

#define target_supports_non_stop() \
  the_target->supports_non_stop ()

#define target_async(enable) \
  the_target->async (enable)

#define target_process_qsupported(features) \
  the_target->process_qsupported (features)

#define target_supports_catch_syscall()              	\
  the_target->supports_catch_syscall ()

#define target_get_ipa_tdesc_idx()			\
  the_target->get_ipa_tdesc_idx ()

#define target_supports_tracepoints()			\
  the_target->supports_tracepoints ()

#define target_supports_fast_tracepoints()		\
  the_target->supports_fast_tracepoints ()

#define target_get_min_fast_tracepoint_insn_len()	\
  the_target->get_min_fast_tracepoint_insn_len ()

#define target_thread_stopped(thread) \
  the_target->thread_stopped (thread)

#define target_pause_all(freeze)		\
  the_target->pause_all (freeze)

#define target_unpause_all(unfreeze)		\
  the_target->unpause_all (unfreeze)

#define target_stabilize_threads()		\
  the_target->stabilize_threads ()

#define target_install_fast_tracepoint_jump_pad(tpoint, tpaddr,		\
						collector, lockaddr,	\
						orig_size,		\
						jump_entry,		\
						trampoline, trampoline_size, \
						jjump_pad_insn,		\
						jjump_pad_insn_size,	\
						adjusted_insn_addr,	\
						adjusted_insn_addr_end,	\
						err)			\
  the_target->install_fast_tracepoint_jump_pad (tpoint, tpaddr,	\
						collector,lockaddr,	\
						orig_size, jump_entry,	\
						trampoline,		\
						trampoline_size,	\
						jjump_pad_insn,		\
						jjump_pad_insn_size,	\
						adjusted_insn_addr,	\
						adjusted_insn_addr_end, \
						err)

#define target_emit_ops() \
  the_target->emit_ops ()

#define target_supports_disable_randomization() \
  the_target->supports_disable_randomization ()

#define target_supports_agent() \
  the_target->supports_agent ()

static inline struct btrace_target_info *
target_enable_btrace (thread_info *tp, const struct btrace_config *conf)
{
  return the_target->enable_btrace (tp, conf);
}

static inline int
target_disable_btrace (struct btrace_target_info *tinfo)
{
  return the_target->disable_btrace (tinfo);
}

static inline int
target_read_btrace (struct btrace_target_info *tinfo,
		    std::string *buffer,
		    enum btrace_read_type type)
{
  return the_target->read_btrace (tinfo, buffer, type);
}

static inline int
target_read_btrace_conf (struct btrace_target_info *tinfo,
			 std::string *buffer)
{
  return the_target->read_btrace_conf (tinfo, buffer);
}

#define target_supports_range_stepping() \
  the_target->supports_range_stepping ()

#define target_supports_stopped_by_sw_breakpoint() \
  the_target->supports_stopped_by_sw_breakpoint ()

#define target_stopped_by_sw_breakpoint() \
  the_target->stopped_by_sw_breakpoint ()

#define target_supports_stopped_by_hw_breakpoint() \
  the_target->supports_stopped_by_hw_breakpoint ()

#define target_supports_hardware_single_step() \
  the_target->supports_hardware_single_step ()

#define target_stopped_by_hw_breakpoint() \
  the_target->stopped_by_hw_breakpoint ()

#define target_breakpoint_kind_from_pc(pcptr) \
  the_target->breakpoint_kind_from_pc (pcptr)

#define target_breakpoint_kind_from_current_state(pcptr) \
  the_target->breakpoint_kind_from_current_state (pcptr)

#define target_supports_software_single_step() \
  the_target->supports_software_single_step ()

#define target_any_resumed() \
  the_target->any_resumed ()

ptid_t mywait (ptid_t ptid, struct target_waitstatus *ourstatus,
	       target_wait_flags options, int connected_wait);

#define target_core_of_thread(ptid)		\
  the_target->core_of_thread (ptid)

#define target_thread_name(ptid)                                \
  the_target->thread_name (ptid)

#define target_thread_handle(ptid, handle, handle_len) \
  the_target->thread_handle (ptid, handle, handle_len)

static inline thread_info *
target_thread_pending_parent (thread_info *thread)
{
  return the_target->thread_pending_parent (thread);
}

static inline thread_info *
target_thread_pending_child (thread_info *thread, target_waitkind *kind)
{
  return the_target->thread_pending_child (thread, kind);
}

/* Read LEN bytes from MEMADDR in the buffer MYADDR.  Return 0 if the read
   is successful, otherwise, return a non-zero error code.  */

int read_inferior_memory (CORE_ADDR memaddr, unsigned char *myaddr, int len);

/* Set GDBserver's current thread to the thread the client requested
   via Hg.  Also switches the current process to the requested
   process.  If the requested thread is not found in the thread list,
   then the current thread is set to NULL.  Likewise, if the requested
   process is not found in the process list, then the current process
   is set to NULL.  Returns true if the requested thread was found,
   false otherwise.  */

bool set_desired_thread ();

/* Set GDBserver's current process to the process the client requested
   via Hg.  The current thread is set to NULL.  */

bool set_desired_process ();

std::string target_pid_to_str (ptid_t);

#endif /* GDBSERVER_TARGET_H */
