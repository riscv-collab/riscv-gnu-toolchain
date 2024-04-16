/* Interface between GDB and target environments, including files and processes

   Copyright (C) 1990-2024 Free Software Foundation, Inc.

   Contributed by Cygnus Support.  Written by John Gilmore.

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

#if !defined (TARGET_H)
#define TARGET_H

struct objfile;
struct ui_file;
struct mem_attrib;
struct target_ops;
struct bp_location;
struct bp_target_info;
struct regcache;
struct trace_state_variable;
struct trace_status;
struct uploaded_tsv;
struct uploaded_tp;
struct static_tracepoint_marker;
struct traceframe_info;
struct expression;
struct dcache_struct;
struct inferior;

/* Define const gdb_byte using one identifier, to make it easy for
   make-target-delegates.py to parse.  */
typedef const gdb_byte const_gdb_byte;

#include "infrun.h"
#include "breakpoint.h"
#include "gdbsupport/scoped_restore.h"
#include "gdbsupport/refcounted-object.h"
#include "target-section.h"

/* This include file defines the interface between the main part
   of the debugger, and the part which is target-specific, or
   specific to the communications interface between us and the
   target.

   A TARGET is an interface between the debugger and a particular
   kind of file or process.  Targets can be STACKED in STRATA,
   so that more than one target can potentially respond to a request.
   In particular, memory accesses will walk down the stack of targets
   until they find a target that is interested in handling that particular
   address.  STRATA are artificial boundaries on the stack, within
   which particular kinds of targets live.  Strata exist so that
   people don't get confused by pushing e.g. a process target and then
   a file target, and wondering why they can't see the current values
   of variables any more (the file target is handling them and they
   never get to the process target).  So when you push a file target,
   it goes into the file stratum, which is always below the process
   stratum.

   Note that rather than allow an empty stack, we always have the
   dummy target at the bottom stratum, so we can call the target
   methods without checking them.  */

#include "target/target.h"
#include "target/resume.h"
#include "target/wait.h"
#include "target/waitstatus.h"
#include "bfd.h"
#include "symtab.h"
#include "memattr.h"
#include "gdbsupport/gdb_signals.h"
#include "btrace.h"
#include "record.h"
#include "command.h"
#include "disasm-flags.h"
#include "tracepoint.h"
#include "gdbsupport/fileio.h"
#include "gdbsupport/x86-xstate.h"

#include "gdbsupport/break-common.h"

enum strata
  {
    dummy_stratum,		/* The lowest of the low */
    file_stratum,		/* Executable files, etc */
    process_stratum,		/* Executing processes or core dump files */
    thread_stratum,		/* Executing threads */
    record_stratum,		/* Support record debugging */
    arch_stratum,		/* Architecture overrides */
    debug_stratum		/* Target debug.  Must be last.  */
  };

enum thread_control_capabilities
  {
    tc_none = 0,		/* Default: can't control thread execution.  */
    tc_schedlock = 1,		/* Can lock the thread scheduler.  */
  };

/* The structure below stores information about a system call.
   It is basically used in the "catch syscall" command, and in
   every function that gives information about a system call.
   
   It's also good to mention that its fields represent everything
   that we currently know about a syscall in GDB.  */
struct syscall
  {
    /* The syscall number.  */
    int number;

    /* The syscall name.  */
    const char *name;
  };

/* Return a pretty printed form of TARGET_OPTIONS.  */
extern std::string target_options_to_string (target_wait_flags target_options);

/* Possible types of events that the inferior handler will have to
   deal with.  */
enum inferior_event_type
  {
    /* Process a normal inferior event which will result in target_wait
       being called.  */
    INF_REG_EVENT,
    /* We are called to do stuff after the inferior stops.  */
    INF_EXEC_COMPLETE,
  };

/* Target objects which can be transfered using target_read,
   target_write, et cetera.  */

enum target_object
{
  /* AVR target specific transfer.  See "avr-tdep.c" and "remote.c".  */
  TARGET_OBJECT_AVR,
  /* Transfer up-to LEN bytes of memory starting at OFFSET.  */
  TARGET_OBJECT_MEMORY,
  /* Memory, avoiding GDB's data cache and trusting the executable.
     Target implementations of to_xfer_partial never need to handle
     this object, and most callers should not use it.  */
  TARGET_OBJECT_RAW_MEMORY,
  /* Memory known to be part of the target's stack.  This is cached even
     if it is not in a region marked as such, since it is known to be
     "normal" RAM.  */
  TARGET_OBJECT_STACK_MEMORY,
  /* Memory known to be part of the target code.   This is cached even
     if it is not in a region marked as such.  */
  TARGET_OBJECT_CODE_MEMORY,
  /* Kernel Unwind Table.  See "ia64-tdep.c".  */
  TARGET_OBJECT_UNWIND_TABLE,
  /* Transfer auxilliary vector.  */
  TARGET_OBJECT_AUXV,
  /* StackGhost cookie.  See "sparc-tdep.c".  */
  TARGET_OBJECT_WCOOKIE,
  /* Target memory map in XML format.  */
  TARGET_OBJECT_MEMORY_MAP,
  /* Flash memory.  This object can be used to write contents to
     a previously erased flash memory.  Using it without erasing
     flash can have unexpected results.  Addresses are physical
     address on target, and not relative to flash start.  */
  TARGET_OBJECT_FLASH,
  /* Available target-specific features, e.g. registers and coprocessors.
     See "target-descriptions.c".  ANNEX should never be empty.  */
  TARGET_OBJECT_AVAILABLE_FEATURES,
  /* Currently loaded libraries, in XML format.  */
  TARGET_OBJECT_LIBRARIES,
  /* Currently loaded libraries specific for SVR4 systems, in XML format.  */
  TARGET_OBJECT_LIBRARIES_SVR4,
  /* Currently loaded libraries specific to AIX systems, in XML format.  */
  TARGET_OBJECT_LIBRARIES_AIX,
  /* Get OS specific data.  The ANNEX specifies the type (running
     processes, etc.).  The data being transfered is expected to follow
     the DTD specified in features/osdata.dtd.  */
  TARGET_OBJECT_OSDATA,
  /* Extra signal info.  Usually the contents of `siginfo_t' on unix
     platforms.  */
  TARGET_OBJECT_SIGNAL_INFO,
  /* The list of threads that are being debugged.  */
  TARGET_OBJECT_THREADS,
  /* Collected static trace data.  */
  TARGET_OBJECT_STATIC_TRACE_DATA,
  /* Traceframe info, in XML format.  */
  TARGET_OBJECT_TRACEFRAME_INFO,
  /* Load maps for FDPIC systems.  */
  TARGET_OBJECT_FDPIC,
  /* Darwin dynamic linker info data.  */
  TARGET_OBJECT_DARWIN_DYLD_INFO,
  /* OpenVMS Unwind Information Block.  */
  TARGET_OBJECT_OPENVMS_UIB,
  /* Branch trace data, in XML format.  */
  TARGET_OBJECT_BTRACE,
  /* Branch trace configuration, in XML format.  */
  TARGET_OBJECT_BTRACE_CONF,
  /* The pathname of the executable file that was run to create
     a specified process.  ANNEX should be a string representation
     of the process ID of the process in question, in hexadecimal
     format.  */
  TARGET_OBJECT_EXEC_FILE,
  /* FreeBSD virtual memory mappings.  */
  TARGET_OBJECT_FREEBSD_VMMAP,
  /* FreeBSD process strings.  */
  TARGET_OBJECT_FREEBSD_PS_STRINGS,
  /* Possible future objects: TARGET_OBJECT_FILE, ...  */
};

/* Possible values returned by target_xfer_partial, etc.  */

enum target_xfer_status
{
  /* Some bytes are transferred.  */
  TARGET_XFER_OK = 1,

  /* No further transfer is possible.  */
  TARGET_XFER_EOF = 0,

  /* The piece of the object requested is unavailable.  */
  TARGET_XFER_UNAVAILABLE = 2,

  /* Generic I/O error.  Note that it's important that this is '-1',
     as we still have target_xfer-related code returning hardcoded
     '-1' on error.  */
  TARGET_XFER_E_IO = -1,

  /* Keep list in sync with target_xfer_status_to_string.  */
};

/* Return the string form of STATUS.  */

extern const char *
  target_xfer_status_to_string (enum target_xfer_status status);

typedef enum target_xfer_status
  target_xfer_partial_ftype (struct target_ops *ops,
			     enum target_object object,
			     const char *annex,
			     gdb_byte *readbuf,
			     const gdb_byte *writebuf,
			     ULONGEST offset,
			     ULONGEST len,
			     ULONGEST *xfered_len);

enum target_xfer_status
  raw_memory_xfer_partial (struct target_ops *ops, gdb_byte *readbuf,
			   const gdb_byte *writebuf, ULONGEST memaddr,
			   LONGEST len, ULONGEST *xfered_len);

/* Request that OPS transfer up to LEN addressable units of the target's
   OBJECT.  When reading from a memory object, the size of an addressable unit
   is architecture dependent and can be found using
   gdbarch_addressable_memory_unit_size.  Otherwise, an addressable unit is 1
   byte long.  BUF should point to a buffer large enough to hold the read data,
   taking into account the addressable unit size.  The OFFSET, for a seekable
   object, specifies the starting point.  The ANNEX can be used to provide
   additional data-specific information to the target.

   Return the number of addressable units actually transferred, or a negative
   error code (an 'enum target_xfer_error' value) if the transfer is not
   supported or otherwise fails.  Return of a positive value less than
   LEN indicates that no further transfer is possible.  Unlike the raw
   to_xfer_partial interface, callers of these functions do not need
   to retry partial transfers.  */

extern LONGEST target_read (struct target_ops *ops,
			    enum target_object object,
			    const char *annex, gdb_byte *buf,
			    ULONGEST offset, LONGEST len);

struct memory_read_result
{
  memory_read_result (ULONGEST begin_, ULONGEST end_,
		      gdb::unique_xmalloc_ptr<gdb_byte> &&data_)
    : begin (begin_),
      end (end_),
      data (std::move (data_))
  {
  }

  ~memory_read_result () = default;

  memory_read_result (memory_read_result &&other) = default;

  DISABLE_COPY_AND_ASSIGN (memory_read_result);

  /* First address that was read.  */
  ULONGEST begin;
  /* Past-the-end address.  */
  ULONGEST end;
  /* The data.  */
  gdb::unique_xmalloc_ptr<gdb_byte> data;
};

extern std::vector<memory_read_result> read_memory_robust
    (struct target_ops *ops, const ULONGEST offset, const LONGEST len);

/* Request that OPS transfer up to LEN addressable units from BUF to the
   target's OBJECT.  When writing to a memory object, the addressable unit
   size is architecture dependent and can be found using
   gdbarch_addressable_memory_unit_size.  Otherwise, an addressable unit is 1
   byte long.  The OFFSET, for a seekable object, specifies the starting point.
   The ANNEX can be used to provide additional data-specific information to
   the target.

   Return the number of addressable units actually transferred, or a negative
   error code (an 'enum target_xfer_status' value) if the transfer is not
   supported or otherwise fails.  Return of a positive value less than
   LEN indicates that no further transfer is possible.  Unlike the raw
   to_xfer_partial interface, callers of these functions do not need to
   retry partial transfers.  */

extern LONGEST target_write (struct target_ops *ops,
			     enum target_object object,
			     const char *annex, const gdb_byte *buf,
			     ULONGEST offset, LONGEST len);

/* Similar to target_write, except that it also calls PROGRESS with
   the number of bytes written and the opaque BATON after every
   successful partial write (and before the first write).  This is
   useful for progress reporting and user interaction while writing
   data.  To abort the transfer, the progress callback can throw an
   exception.  */

LONGEST target_write_with_progress (struct target_ops *ops,
				    enum target_object object,
				    const char *annex, const gdb_byte *buf,
				    ULONGEST offset, LONGEST len,
				    void (*progress) (ULONGEST, void *),
				    void *baton);

/* Wrapper to perform a full read of unknown size.  OBJECT/ANNEX will be read
   using OPS.  The return value will be uninstantiated if the transfer fails or
   is not supported.

   This method should be used for objects sufficiently small to store
   in a single xmalloc'd buffer, when no fixed bound on the object's
   size is known in advance.  Don't try to read TARGET_OBJECT_MEMORY
   through this function.  */

extern std::optional<gdb::byte_vector> target_read_alloc
    (struct target_ops *ops, enum target_object object, const char *annex);

/* Read OBJECT/ANNEX using OPS.  The result is a NUL-terminated character vector
   (therefore usable as a NUL-terminated string).  If an error occurs or the
   transfer is unsupported, the return value will be uninstantiated.  Empty
   objects are returned as allocated but empty strings.  Therefore, on success,
   the returned vector is guaranteed to have at least one element.  A warning is
   issued if the result contains any embedded NUL bytes.  */

extern std::optional<gdb::char_vector> target_read_stralloc
    (struct target_ops *ops, enum target_object object, const char *annex);

/* See target_ops->to_xfer_partial.  */
extern target_xfer_partial_ftype target_xfer_partial;

/* Wrappers to target read/write that perform memory transfers.  They
   throw an error if the memory transfer fails.

   NOTE: cagney/2003-10-23: The naming schema is lifted from
   "frame.h".  The parameter order is lifted from get_frame_memory,
   which in turn lifted it from read_memory.  */

extern void get_target_memory (struct target_ops *ops, CORE_ADDR addr,
			       gdb_byte *buf, LONGEST len);
extern ULONGEST get_target_memory_unsigned (struct target_ops *ops,
					    CORE_ADDR addr, int len,
					    enum bfd_endian byte_order);

struct thread_info;		/* fwd decl for parameter list below: */

/* The type of the callback to the to_async method.  */

typedef void async_callback_ftype (enum inferior_event_type event_type,
				   void *context);

/* Normally target debug printing is purely type-based.  However,
   sometimes it is necessary to override the debug printing on a
   per-argument basis.  This macro can be used, attribute-style, to
   name the target debug printing function for a particular method
   argument.  FUNC is the name of the function.  The macro's
   definition is empty because it is only used by the
   make-target-delegates script.  */

#define TARGET_DEBUG_PRINTER(FUNC)

/* These defines are used to mark target_ops methods.  The script
   make-target-delegates scans these and auto-generates the base
   method implementations.  There are four macros that can be used:
   
   1. TARGET_DEFAULT_IGNORE.  There is no argument.  The base method
   does nothing.  This is only valid if the method return type is
   'void'.
   
   2. TARGET_DEFAULT_NORETURN.  The argument is a function call, like
   'tcomplain ()'.  The base method simply makes this call, which is
   assumed not to return.
   
   3. TARGET_DEFAULT_RETURN.  The argument is a C expression.  The
   base method returns this expression's value.
   
   4. TARGET_DEFAULT_FUNC.  The argument is the name of a function.
   make-target-delegates does not generate a base method in this case,
   but instead uses the argument function as the base method.  */

#define TARGET_DEFAULT_IGNORE()
#define TARGET_DEFAULT_NORETURN(ARG)
#define TARGET_DEFAULT_RETURN(ARG)
#define TARGET_DEFAULT_FUNC(ARG)

/* Each target that can be activated with "target TARGET_NAME" passes
   the address of one of these objects to add_target, which uses the
   object's address as unique identifier, and registers the "target
   TARGET_NAME" command using SHORTNAME as target name.  */

struct target_info
{
  /* Name of this target.  */
  const char *shortname;

  /* Name for printing.  */
  const char *longname;

  /* Documentation.  Does not include trailing newline, and starts
     with a one-line description (probably similar to longname).  */
  const char *doc;
};

struct target_ops
  : public refcounted_object
  {
    /* Return this target's stratum.  */
    virtual strata stratum () const = 0;

    /* To the target under this one.  */
    target_ops *beneath () const;

    /* Free resources associated with the target.  Note that singleton
       targets, like e.g., native targets, are global objects, not
       heap allocated, and are thus only deleted on GDB exit.  The
       main teardown entry point is the "close" method, below.  */
    virtual ~target_ops () {}

    /* Return a reference to this target's unique target_info
       object.  */
    virtual const target_info &info () const = 0;

    /* Name this target type.  */
    const char *shortname () const
    { return info ().shortname; }

    const char *longname () const
    { return info ().longname; }

    /* Close the target.  This is where the target can handle
       teardown.  Heap-allocated targets should delete themselves
       before returning.  */
    virtual void close ();

    /* Attaches to a process on the target side.  Arguments are as
       passed to the `attach' command by the user.  This routine can
       be called when the target is not on the target-stack, if the
       target_ops::can_run method returns 1; in that case, it must push
       itself onto the stack.  Upon exit, the target should be ready
       for normal operations, and should be ready to deliver the
       status of the process immediately (without waiting) to an
       upcoming target_wait call.  */
    virtual bool can_attach ();
    virtual void attach (const char *, int);
    virtual void post_attach (int)
      TARGET_DEFAULT_IGNORE ();

    /* Detaches from the inferior.  Note that on targets that support
       async execution (i.e., targets where it is possible to detach
       from programs with threads running), the target is responsible
       for removing breakpoints from the program before the actual
       detach, otherwise the program dies when it hits one.  */
    virtual void detach (inferior *, int)
      TARGET_DEFAULT_IGNORE ();

    virtual void disconnect (const char *, int)
      TARGET_DEFAULT_NORETURN (tcomplain ());
    virtual void resume (ptid_t,
			 int TARGET_DEBUG_PRINTER (target_debug_print_step),
			 enum gdb_signal)
      TARGET_DEFAULT_NORETURN (noprocess ());

    /* Ensure that all resumed threads are committed to the target.

       See the description of
       process_stratum_target::commit_resumed_state for more
       details.  */
    virtual void commit_resumed ()
      TARGET_DEFAULT_IGNORE ();

    /* See target_wait's description.  Note that implementations of
       this method must not assume that inferior_ptid on entry is
       pointing at the thread or inferior that ends up reporting an
       event.  The reported event could be for some other thread in
       the current inferior or even for a different process of the
       current target.  inferior_ptid may also be null_ptid on
       entry.  */
    virtual ptid_t wait (ptid_t, struct target_waitstatus *,
			 target_wait_flags options)
      TARGET_DEFAULT_FUNC (default_target_wait);
    virtual void fetch_registers (struct regcache *, int)
      TARGET_DEFAULT_IGNORE ();
    virtual void store_registers (struct regcache *, int)
      TARGET_DEFAULT_NORETURN (noprocess ());
    virtual void prepare_to_store (struct regcache *)
      TARGET_DEFAULT_NORETURN (noprocess ());

    virtual void files_info ()
      TARGET_DEFAULT_IGNORE ();
    virtual int insert_breakpoint (struct gdbarch *,
				 struct bp_target_info *)
      TARGET_DEFAULT_NORETURN (noprocess ());
    virtual int remove_breakpoint (struct gdbarch *,
				 struct bp_target_info *,
				 enum remove_bp_reason)
      TARGET_DEFAULT_NORETURN (noprocess ());

    /* Returns true if the target stopped because it executed a
       software breakpoint.  This is necessary for correct background
       execution / non-stop mode operation, and for correct PC
       adjustment on targets where the PC needs to be adjusted when a
       software breakpoint triggers.  In these modes, by the time GDB
       processes a breakpoint event, the breakpoint may already be
       done from the target, so GDB needs to be able to tell whether
       it should ignore the event and whether it should adjust the PC.
       See adjust_pc_after_break.  */
    virtual bool stopped_by_sw_breakpoint ()
      TARGET_DEFAULT_RETURN (false);
    /* Returns true if the above method is supported.  */
    virtual bool supports_stopped_by_sw_breakpoint ()
      TARGET_DEFAULT_RETURN (false);

    /* Returns true if the target stopped for a hardware breakpoint.
       Likewise, if the target supports hardware breakpoints, this
       method is necessary for correct background execution / non-stop
       mode operation.  Even though hardware breakpoints do not
       require PC adjustment, GDB needs to be able to tell whether the
       hardware breakpoint event is a delayed event for a breakpoint
       that is already gone and should thus be ignored.  */
    virtual bool stopped_by_hw_breakpoint ()
      TARGET_DEFAULT_RETURN (false);
    /* Returns true if the above method is supported.  */
    virtual bool supports_stopped_by_hw_breakpoint ()
      TARGET_DEFAULT_RETURN (false);

    virtual int can_use_hw_breakpoint (enum bptype, int, int)
      TARGET_DEFAULT_RETURN (0);
    virtual int ranged_break_num_registers ()
      TARGET_DEFAULT_RETURN (-1);
    virtual int insert_hw_breakpoint (struct gdbarch *,
				      struct bp_target_info *)
      TARGET_DEFAULT_RETURN (-1);
    virtual int remove_hw_breakpoint (struct gdbarch *,
				      struct bp_target_info *)
      TARGET_DEFAULT_RETURN (-1);

    /* Documentation of what the two routines below are expected to do is
       provided with the corresponding target_* macros.  */
    virtual int remove_watchpoint (CORE_ADDR, int,
				 enum target_hw_bp_type, struct expression *)
      TARGET_DEFAULT_RETURN (-1);
    virtual int insert_watchpoint (CORE_ADDR, int,
				 enum target_hw_bp_type, struct expression *)
      TARGET_DEFAULT_RETURN (-1);

    virtual int insert_mask_watchpoint (CORE_ADDR, CORE_ADDR,
					enum target_hw_bp_type)
      TARGET_DEFAULT_RETURN (1);
    virtual int remove_mask_watchpoint (CORE_ADDR, CORE_ADDR,
					enum target_hw_bp_type)
      TARGET_DEFAULT_RETURN (1);
    virtual bool stopped_by_watchpoint ()
      TARGET_DEFAULT_RETURN (false);
    virtual bool have_steppable_watchpoint ()
      TARGET_DEFAULT_RETURN (false);
    virtual bool stopped_data_address (CORE_ADDR *)
      TARGET_DEFAULT_RETURN (false);
    virtual bool watchpoint_addr_within_range (CORE_ADDR, CORE_ADDR, int)
      TARGET_DEFAULT_FUNC (default_watchpoint_addr_within_range);

    /* Documentation of this routine is provided with the corresponding
       target_* macro.  */
    virtual int region_ok_for_hw_watchpoint (CORE_ADDR, int)
      TARGET_DEFAULT_FUNC (default_region_ok_for_hw_watchpoint);

    virtual bool can_accel_watchpoint_condition (CORE_ADDR, int, int,
						 struct expression *)
      TARGET_DEFAULT_RETURN (false);
    virtual int masked_watch_num_registers (CORE_ADDR, CORE_ADDR)
      TARGET_DEFAULT_RETURN (-1);

    /* Return 1 for sure target can do single step.  Return -1 for
       unknown.  Return 0 for target can't do.  */
    virtual int can_do_single_step ()
      TARGET_DEFAULT_RETURN (-1);

    virtual bool supports_terminal_ours ()
      TARGET_DEFAULT_RETURN (false);
    virtual void terminal_init ()
      TARGET_DEFAULT_IGNORE ();
    virtual void terminal_inferior ()
      TARGET_DEFAULT_IGNORE ();
    virtual void terminal_save_inferior ()
      TARGET_DEFAULT_IGNORE ();
    virtual void terminal_ours_for_output ()
      TARGET_DEFAULT_IGNORE ();
    virtual void terminal_ours ()
      TARGET_DEFAULT_IGNORE ();
    virtual void terminal_info (const char *, int)
      TARGET_DEFAULT_FUNC (default_terminal_info);
    virtual void kill ()
      TARGET_DEFAULT_NORETURN (noprocess ());
    virtual void load (const char *, int)
      TARGET_DEFAULT_NORETURN (tcomplain ());
    /* Start an inferior process and set inferior_ptid to its pid.
       EXEC_FILE is the file to run.
       ALLARGS is a string containing the arguments to the program.
       ENV is the environment vector to pass.  Errors reported with error().
       On VxWorks and various standalone systems, we ignore exec_file.  */
    virtual bool can_create_inferior ();
    virtual void create_inferior (const char *, const std::string &,
				  char **, int);
    virtual int insert_fork_catchpoint (int)
      TARGET_DEFAULT_RETURN (1);
    virtual int remove_fork_catchpoint (int)
      TARGET_DEFAULT_RETURN (1);
    virtual int insert_vfork_catchpoint (int)
      TARGET_DEFAULT_RETURN (1);
    virtual int remove_vfork_catchpoint (int)
      TARGET_DEFAULT_RETURN (1);
    virtual void follow_fork (inferior *, ptid_t, target_waitkind, bool, bool)
      TARGET_DEFAULT_FUNC (default_follow_fork);

    /* Add CHILD_PTID to the thread list, after handling a
       TARGET_WAITKIND_THREAD_CLONE event for the clone parent.  The
       parent is inferior_ptid.  */
    virtual void follow_clone (ptid_t child_ptid)
      TARGET_DEFAULT_FUNC (default_follow_clone);

    virtual int insert_exec_catchpoint (int)
      TARGET_DEFAULT_RETURN (1);
    virtual int remove_exec_catchpoint (int)
      TARGET_DEFAULT_RETURN (1);
    virtual void follow_exec (inferior *, ptid_t, const char *)
      TARGET_DEFAULT_IGNORE ();
    virtual int set_syscall_catchpoint (int, bool, int,
					gdb::array_view<const int>)
      TARGET_DEFAULT_RETURN (1);
    virtual void mourn_inferior ()
      TARGET_DEFAULT_FUNC (default_mourn_inferior);

    /* Note that can_run is special and can be invoked on an unpushed
       target.  Targets defining this method must also define
       to_can_async_p and to_supports_non_stop.  */
    virtual bool can_run ();

    /* Documentation of this routine is provided with the corresponding
       target_* macro.  */
    virtual void pass_signals (gdb::array_view<const unsigned char> TARGET_DEBUG_PRINTER (target_debug_print_signals))
      TARGET_DEFAULT_IGNORE ();

    /* Documentation of this routine is provided with the
       corresponding target_* function.  */
    virtual void program_signals (gdb::array_view<const unsigned char> TARGET_DEBUG_PRINTER (target_debug_print_signals))
      TARGET_DEFAULT_IGNORE ();

    virtual bool thread_alive (ptid_t ptid)
      TARGET_DEFAULT_RETURN (false);
    virtual void update_thread_list ()
      TARGET_DEFAULT_IGNORE ();
    virtual std::string pid_to_str (ptid_t)
      TARGET_DEFAULT_FUNC (default_pid_to_str);
    virtual const char *extra_thread_info (thread_info *)
      TARGET_DEFAULT_RETURN (NULL);
    virtual const char *thread_name (thread_info *)
      TARGET_DEFAULT_RETURN (NULL);
    virtual thread_info *thread_handle_to_thread_info (const gdb_byte *,
						       int,
						       inferior *inf)
      TARGET_DEFAULT_RETURN (NULL);
    /* See target_thread_info_to_thread_handle.  */
    virtual gdb::array_view<const_gdb_byte> thread_info_to_thread_handle (struct thread_info *)
      TARGET_DEFAULT_RETURN (gdb::array_view<const gdb_byte> ());
    virtual void stop (ptid_t)
      TARGET_DEFAULT_IGNORE ();
    virtual void interrupt ()
      TARGET_DEFAULT_IGNORE ();
    virtual void pass_ctrlc ()
      TARGET_DEFAULT_FUNC (default_target_pass_ctrlc);
    virtual void rcmd (const char *command, struct ui_file *output)
      TARGET_DEFAULT_FUNC (default_rcmd);
    virtual const char *pid_to_exec_file (int pid)
      TARGET_DEFAULT_RETURN (NULL);
    virtual void log_command (const char *)
      TARGET_DEFAULT_IGNORE ();
    virtual const std::vector<target_section> *get_section_table ()
      TARGET_DEFAULT_RETURN (default_get_section_table ());

    /* Provide default values for all "must have" methods.  */
    virtual bool has_all_memory () { return false; }
    virtual bool has_memory () { return false; }
    virtual bool has_stack () { return false; }
    virtual bool has_registers () { return false; }
    virtual bool has_execution (inferior *inf) { return false; }

    /* Control thread execution.  */
    virtual thread_control_capabilities get_thread_control_capabilities ()
      TARGET_DEFAULT_RETURN (tc_none);
    virtual bool attach_no_wait ()
      TARGET_DEFAULT_RETURN (0);
    /* This method must be implemented in some situations.  See the
       comment on 'can_run'.  */
    virtual bool can_async_p ()
      TARGET_DEFAULT_RETURN (false);
    virtual bool is_async_p ()
      TARGET_DEFAULT_RETURN (false);
    virtual void async (bool)
      TARGET_DEFAULT_NORETURN (tcomplain ());
    virtual int async_wait_fd ()
      TARGET_DEFAULT_NORETURN (noprocess ());
    /* Return true if the target has pending events to report to the
       core.  If true, then GDB avoids resuming the target until all
       pending events are consumed, so that multiple resumptions can
       be coalesced as an optimization.  Most targets can't tell
       whether they have pending events without calling target_wait,
       so we default to returning false.  The only downside is that a
       potential optimization is missed.  */
    virtual bool has_pending_events ()
      TARGET_DEFAULT_RETURN (false);
    virtual void thread_events (int)
      TARGET_DEFAULT_IGNORE ();
    /* Returns true if the target supports setting thread options
       OPTIONS, false otherwise.  */
    virtual bool supports_set_thread_options (gdb_thread_options options)
      TARGET_DEFAULT_RETURN (false);
    /* This method must be implemented in some situations.  See the
       comment on 'can_run'.  */
    virtual bool supports_non_stop ()
      TARGET_DEFAULT_RETURN (false);
    /* Return true if the target operates in non-stop mode even with
       "set non-stop off".  */
    virtual bool always_non_stop_p ()
      TARGET_DEFAULT_RETURN (false);
    /* find_memory_regions support method for gcore */
    virtual int find_memory_regions (find_memory_region_ftype func, void *data)
      TARGET_DEFAULT_FUNC (dummy_find_memory_regions);
    /* make_corefile_notes support method for gcore */
    virtual gdb::unique_xmalloc_ptr<char> make_corefile_notes (bfd *, int *)
      TARGET_DEFAULT_FUNC (dummy_make_corefile_notes);
    /* get_bookmark support method for bookmarks */
    virtual gdb_byte *get_bookmark (const char *, int)
      TARGET_DEFAULT_NORETURN (tcomplain ());
    /* goto_bookmark support method for bookmarks */
    virtual void goto_bookmark (const gdb_byte *, int)
      TARGET_DEFAULT_NORETURN (tcomplain ());
    /* Return the thread-local address at OFFSET in the
       thread-local storage for the thread PTID and the shared library
       or executable file given by LOAD_MODULE_ADDR.  If that block of
       thread-local storage hasn't been allocated yet, this function
       may throw an error.  LOAD_MODULE_ADDR may be zero for statically
       linked multithreaded inferiors.  */
    virtual CORE_ADDR get_thread_local_address (ptid_t ptid,
						CORE_ADDR load_module_addr,
						CORE_ADDR offset)
      TARGET_DEFAULT_NORETURN (generic_tls_error ());

    /* Request that OPS transfer up to LEN addressable units of the target's
       OBJECT.  When reading from a memory object, the size of an addressable
       unit is architecture dependent and can be found using
       gdbarch_addressable_memory_unit_size.  Otherwise, an addressable unit is
       1 byte long.  The OFFSET, for a seekable object, specifies the
       starting point.  The ANNEX can be used to provide additional
       data-specific information to the target.

       Return the transferred status, error or OK (an
       'enum target_xfer_status' value).  Save the number of addressable units
       actually transferred in *XFERED_LEN if transfer is successful
       (TARGET_XFER_OK) or the number unavailable units if the requested
       data is unavailable (TARGET_XFER_UNAVAILABLE).  *XFERED_LEN
       smaller than LEN does not indicate the end of the object, only
       the end of the transfer; higher level code should continue
       transferring if desired.  This is handled in target.c.

       The interface does not support a "retry" mechanism.  Instead it
       assumes that at least one addressable unit will be transfered on each
       successful call.

       NOTE: cagney/2003-10-17: The current interface can lead to
       fragmented transfers.  Lower target levels should not implement
       hacks, such as enlarging the transfer, in an attempt to
       compensate for this.  Instead, the target stack should be
       extended so that it implements supply/collect methods and a
       look-aside object cache.  With that available, the lowest
       target can safely and freely "push" data up the stack.

       See target_read and target_write for more information.  One,
       and only one, of readbuf or writebuf must be non-NULL.  */

    virtual enum target_xfer_status xfer_partial (enum target_object object,
						  const char *annex,
						  gdb_byte *readbuf,
						  const gdb_byte *writebuf,
						  ULONGEST offset, ULONGEST len,
						  ULONGEST *xfered_len)
      TARGET_DEFAULT_RETURN (TARGET_XFER_E_IO);

    /* Return the limit on the size of any single memory transfer
       for the target.  */

    virtual ULONGEST get_memory_xfer_limit ()
      TARGET_DEFAULT_RETURN (ULONGEST_MAX);

    /* Returns the memory map for the target.  A return value of NULL
       means that no memory map is available.  If a memory address
       does not fall within any returned regions, it's assumed to be
       RAM.  The returned memory regions should not overlap.

       The order of regions does not matter; target_memory_map will
       sort regions by starting address.  For that reason, this
       function should not be called directly except via
       target_memory_map.

       This method should not cache data; if the memory map could
       change unexpectedly, it should be invalidated, and higher
       layers will re-fetch it.  */
    virtual std::vector<mem_region> memory_map ()
      TARGET_DEFAULT_RETURN (std::vector<mem_region> ());

    /* Erases the region of flash memory starting at ADDRESS, of
       length LENGTH.

       Precondition: both ADDRESS and ADDRESS+LENGTH should be aligned
       on flash block boundaries, as reported by 'to_memory_map'.  */
    virtual void flash_erase (ULONGEST address, LONGEST length)
      TARGET_DEFAULT_NORETURN (tcomplain ());

    /* Finishes a flash memory write sequence.  After this operation
       all flash memory should be available for writing and the result
       of reading from areas written by 'to_flash_write' should be
       equal to what was written.  */
    virtual void flash_done ()
      TARGET_DEFAULT_NORETURN (tcomplain ());

    /* Describe the architecture-specific features of the current
       inferior.

       Returns the description found, or nullptr if no description was
       available.

       If some target features differ between threads, the description
       returned by read_description (and the resulting gdbarch) won't
       accurately describe all threads.  In this case, the
       thread_architecture method can be used to obtain gdbarches that
       accurately describe each thread.  */
    virtual const struct target_desc *read_description ()
	 TARGET_DEFAULT_RETURN (NULL);

    /* Build the PTID of the thread on which a given task is running,
       based on LWP and THREAD.  These values are extracted from the
       task Private_Data section of the Ada Task Control Block, and
       their interpretation depends on the target.  */
    virtual ptid_t get_ada_task_ptid (long lwp, ULONGEST thread)
      TARGET_DEFAULT_FUNC (default_get_ada_task_ptid);

    /* Read one auxv entry from *READPTR, not reading locations >= ENDPTR.
       Return 0 if *READPTR is already at the end of the buffer.
       Return -1 if there is insufficient buffer for a whole entry.
       Return 1 if an entry was read into *TYPEP and *VALP.  */
    virtual int auxv_parse (const gdb_byte **readptr,
			    const gdb_byte *endptr, CORE_ADDR *typep, CORE_ADDR *valp)
      TARGET_DEFAULT_FUNC (default_auxv_parse);

    /* Search SEARCH_SPACE_LEN bytes beginning at START_ADDR for the
       sequence of bytes in PATTERN with length PATTERN_LEN.

       The result is 1 if found, 0 if not found, and -1 if there was an error
       requiring halting of the search (e.g. memory read error).
       If the pattern is found the address is recorded in FOUND_ADDRP.  */
    virtual int search_memory (CORE_ADDR start_addr, ULONGEST search_space_len,
			       const gdb_byte *pattern, ULONGEST pattern_len,
			       CORE_ADDR *found_addrp)
      TARGET_DEFAULT_FUNC (default_search_memory);

    /* Can target execute in reverse?  */
    virtual bool can_execute_reverse ()
      TARGET_DEFAULT_RETURN (false);

    /* The direction the target is currently executing.  Must be
       implemented on targets that support reverse execution and async
       mode.  The default simply returns forward execution.  */
    virtual enum exec_direction_kind execution_direction ()
      TARGET_DEFAULT_FUNC (default_execution_direction);

    /* Does this target support debugging multiple processes
       simultaneously?  */
    virtual bool supports_multi_process ()
      TARGET_DEFAULT_RETURN (false);

    /* Does this target support enabling and disabling tracepoints while a trace
       experiment is running?  */
    virtual bool supports_enable_disable_tracepoint ()
      TARGET_DEFAULT_RETURN (false);

    /* Does this target support disabling address space randomization?  */
    virtual bool supports_disable_randomization ()
      TARGET_DEFAULT_FUNC (find_default_supports_disable_randomization);

    /* Does this target support the tracenz bytecode for string collection?  */
    virtual bool supports_string_tracing ()
      TARGET_DEFAULT_RETURN (false);

    /* Does this target support evaluation of breakpoint conditions on its
       end?  */
    virtual bool supports_evaluation_of_breakpoint_conditions ()
      TARGET_DEFAULT_RETURN (false);

    /* Does this target support native dumpcore API?  */
    virtual bool supports_dumpcore ()
      TARGET_DEFAULT_RETURN (false);

    /* Generate the core file with native target API.  */
    virtual void dumpcore (const char *filename)
      TARGET_DEFAULT_IGNORE ();

    /* Does this target support evaluation of breakpoint commands on its
       end?  */
    virtual bool can_run_breakpoint_commands ()
      TARGET_DEFAULT_RETURN (false);

    /* Determine current architecture of thread PTID.

       The target is supposed to determine the architecture of the code where
       the target is currently stopped at.  The architecture information is
       used to perform decr_pc_after_break adjustment, and also to determine
       the frame architecture of the innermost frame.  ptrace operations need to
       operate according to the current inferior's gdbarch.  */
    virtual struct gdbarch *thread_architecture (ptid_t)
      TARGET_DEFAULT_RETURN (NULL);

    /* Target file operations.  */

    /* Return true if the filesystem seen by the current inferior
       is the local filesystem, false otherwise.  */
    virtual bool filesystem_is_local ()
      TARGET_DEFAULT_RETURN (true);

    /* Open FILENAME on the target, in the filesystem as seen by INF,
       using FLAGS and MODE.  If INF is NULL, use the filesystem seen
       by the debugger (GDB or, for remote targets, the remote stub).
       If WARN_IF_SLOW is nonzero, print a warning message if the file
       is being accessed over a link that may be slow.  Return a
       target file descriptor, or -1 if an error occurs (and set
       *TARGET_ERRNO).  */
    virtual int fileio_open (struct inferior *inf, const char *filename,
			     int flags, int mode, int warn_if_slow,
			     fileio_error *target_errno);

    /* Write up to LEN bytes from WRITE_BUF to FD on the target.
       Return the number of bytes written, or -1 if an error occurs
       (and set *TARGET_ERRNO).  */
    virtual int fileio_pwrite (int fd, const gdb_byte *write_buf, int len,
			       ULONGEST offset, fileio_error *target_errno);

    /* Read up to LEN bytes FD on the target into READ_BUF.
       Return the number of bytes read, or -1 if an error occurs
       (and set *TARGET_ERRNO).  */
    virtual int fileio_pread (int fd, gdb_byte *read_buf, int len,
			      ULONGEST offset, fileio_error *target_errno);

    /* Get information about the file opened as FD and put it in
       SB.  Return 0 on success, or -1 if an error occurs (and set
       *TARGET_ERRNO).  */
    virtual int fileio_fstat (int fd, struct stat *sb, fileio_error *target_errno);

    /* Close FD on the target.  Return 0, or -1 if an error occurs
       (and set *TARGET_ERRNO).  */
    virtual int fileio_close (int fd, fileio_error *target_errno);

    /* Unlink FILENAME on the target, in the filesystem as seen by
       INF.  If INF is NULL, use the filesystem seen by the debugger
       (GDB or, for remote targets, the remote stub).  Return 0, or
       -1 if an error occurs (and set *TARGET_ERRNO).  */
    virtual int fileio_unlink (struct inferior *inf,
			       const char *filename,
			       fileio_error *target_errno);

    /* Read value of symbolic link FILENAME on the target, in the
       filesystem as seen by INF.  If INF is NULL, use the filesystem
       seen by the debugger (GDB or, for remote targets, the remote
       stub).  Return a string, or an empty optional if an error
       occurs (and set *TARGET_ERRNO).  */
    virtual std::optional<std::string> fileio_readlink (struct inferior *inf,
							const char *filename,
							fileio_error *target_errno);

    /* Implement the "info proc" command.  Returns true if the target
       actually implemented the command, false otherwise.  */
    virtual bool info_proc (const char *, enum info_proc_what);

    /* Tracepoint-related operations.  */

    /* Prepare the target for a tracing run.  */
    virtual void trace_init ()
      TARGET_DEFAULT_NORETURN (tcomplain ());

    /* Send full details of a tracepoint location to the target.  */
    virtual void download_tracepoint (struct bp_location *location)
      TARGET_DEFAULT_NORETURN (tcomplain ());

    /* Is the target able to download tracepoint locations in current
       state?  */
    virtual bool can_download_tracepoint ()
      TARGET_DEFAULT_RETURN (false);

    /* Send full details of a trace state variable to the target.  */
    virtual void download_trace_state_variable (const trace_state_variable &tsv)
      TARGET_DEFAULT_NORETURN (tcomplain ());

    /* Enable a tracepoint on the target.  */
    virtual void enable_tracepoint (struct bp_location *location)
      TARGET_DEFAULT_NORETURN (tcomplain ());

    /* Disable a tracepoint on the target.  */
    virtual void disable_tracepoint (struct bp_location *location)
      TARGET_DEFAULT_NORETURN (tcomplain ());

    /* Inform the target info of memory regions that are readonly
       (such as text sections), and so it should return data from
       those rather than look in the trace buffer.  */
    virtual void trace_set_readonly_regions ()
      TARGET_DEFAULT_NORETURN (tcomplain ());

    /* Start a trace run.  */
    virtual void trace_start ()
      TARGET_DEFAULT_NORETURN (tcomplain ());

    /* Get the current status of a tracing run.  */
    virtual int get_trace_status (struct trace_status *ts)
      TARGET_DEFAULT_RETURN (-1);

    virtual void get_tracepoint_status (tracepoint *tp,
					struct uploaded_tp *utp)
      TARGET_DEFAULT_NORETURN (tcomplain ());

    /* Stop a trace run.  */
    virtual void trace_stop ()
      TARGET_DEFAULT_NORETURN (tcomplain ());

   /* Ask the target to find a trace frame of the given type TYPE,
      using NUM, ADDR1, and ADDR2 as search parameters.  Returns the
      number of the trace frame, and also the tracepoint number at
      TPP.  If no trace frame matches, return -1.  May throw if the
      operation fails.  */
    virtual int trace_find (enum trace_find_type type, int num,
			    CORE_ADDR addr1, CORE_ADDR addr2, int *tpp)
      TARGET_DEFAULT_RETURN (-1);

    /* Get the value of the trace state variable number TSV, returning
       1 if the value is known and writing the value itself into the
       location pointed to by VAL, else returning 0.  */
    virtual bool get_trace_state_variable_value (int tsv, LONGEST *val)
      TARGET_DEFAULT_RETURN (false);

    virtual int save_trace_data (const char *filename)
      TARGET_DEFAULT_NORETURN (tcomplain ());

    virtual int upload_tracepoints (struct uploaded_tp **utpp)
      TARGET_DEFAULT_RETURN (0);

    virtual int upload_trace_state_variables (struct uploaded_tsv **utsvp)
      TARGET_DEFAULT_RETURN (0);

    virtual LONGEST get_raw_trace_data (gdb_byte *buf,
					ULONGEST offset, LONGEST len)
      TARGET_DEFAULT_NORETURN (tcomplain ());

    /* Get the minimum length of instruction on which a fast tracepoint
       may be set on the target.  If this operation is unsupported,
       return -1.  If for some reason the minimum length cannot be
       determined, return 0.  */
    virtual int get_min_fast_tracepoint_insn_len ()
      TARGET_DEFAULT_RETURN (-1);

    /* Set the target's tracing behavior in response to unexpected
       disconnection - set VAL to 1 to keep tracing, 0 to stop.  */
    virtual void set_disconnected_tracing (int val)
      TARGET_DEFAULT_IGNORE ();
    virtual void set_circular_trace_buffer (int val)
      TARGET_DEFAULT_IGNORE ();
    /* Set the size of trace buffer in the target.  */
    virtual void set_trace_buffer_size (LONGEST val)
      TARGET_DEFAULT_IGNORE ();

    /* Add/change textual notes about the trace run, returning true if
       successful, false otherwise.  */
    virtual bool set_trace_notes (const char *user, const char *notes,
				  const char *stopnotes)
      TARGET_DEFAULT_RETURN (false);

    /* Return the processor core that thread PTID was last seen on.
       This information is updated only when:
       - update_thread_list is called
       - thread stops
       If the core cannot be determined -- either for the specified
       thread, or right now, or in this debug session, or for this
       target -- return -1.  */
    virtual int core_of_thread (ptid_t ptid)
      TARGET_DEFAULT_RETURN (-1);

    /* Verify that the memory in the [MEMADDR, MEMADDR+SIZE) range
       matches the contents of [DATA,DATA+SIZE).  Returns 1 if there's
       a match, 0 if there's a mismatch, and -1 if an error is
       encountered while reading memory.  */
    virtual int verify_memory (const gdb_byte *data,
			       CORE_ADDR memaddr, ULONGEST size)
      TARGET_DEFAULT_FUNC (default_verify_memory);

    /* Return the address of the start of the Thread Information Block
       a Windows OS specific feature.  */
    virtual bool get_tib_address (ptid_t ptid, CORE_ADDR *addr)
      TARGET_DEFAULT_NORETURN (tcomplain ());

    /* Send the new settings of write permission variables.  */
    virtual void set_permissions ()
      TARGET_DEFAULT_IGNORE ();

    /* Look for a static tracepoint marker at ADDR, and fill in MARKER
       with its details.  Return true on success, false on failure.  */
    virtual bool static_tracepoint_marker_at (CORE_ADDR,
					      static_tracepoint_marker *marker)
      TARGET_DEFAULT_RETURN (false);

    /* Return a vector of all tracepoints markers string id ID, or all
       markers if ID is NULL.  */
    virtual std::vector<static_tracepoint_marker>
      static_tracepoint_markers_by_strid (const char *id)
      TARGET_DEFAULT_NORETURN (tcomplain ());

    /* Return a traceframe info object describing the current
       traceframe's contents.  This method should not cache data;
       higher layers take care of caching, invalidating, and
       re-fetching when necessary.  */
    virtual traceframe_info_up traceframe_info ()
      TARGET_DEFAULT_NORETURN (tcomplain ());

    /* Ask the target to use or not to use agent according to USE.
       Return true if successful, false otherwise.  */
    virtual bool use_agent (bool use)
      TARGET_DEFAULT_NORETURN (tcomplain ());

    /* Is the target able to use agent in current state?  */
    virtual bool can_use_agent ()
      TARGET_DEFAULT_RETURN (false);

    /* Enable branch tracing for TP using CONF configuration.
       Return a branch trace target information struct for reading and for
       disabling branch trace.  */
    virtual struct btrace_target_info *enable_btrace (thread_info *tp,
						      const struct btrace_config *conf)
      TARGET_DEFAULT_NORETURN (tcomplain ());

    /* Disable branch tracing and deallocate TINFO.  */
    virtual void disable_btrace (struct btrace_target_info *tinfo)
      TARGET_DEFAULT_NORETURN (tcomplain ());

    /* Disable branch tracing and deallocate TINFO.  This function is similar
       to to_disable_btrace, except that it is called during teardown and is
       only allowed to perform actions that are safe.  A counter-example would
       be attempting to talk to a remote target.  */
    virtual void teardown_btrace (struct btrace_target_info *tinfo)
      TARGET_DEFAULT_NORETURN (tcomplain ());

    /* Read branch trace data for the thread indicated by BTINFO into DATA.
       DATA is cleared before new trace is added.  */
    virtual enum btrace_error read_btrace (struct btrace_data *data,
					   struct btrace_target_info *btinfo,
					   enum btrace_read_type type)
      TARGET_DEFAULT_NORETURN (tcomplain ());

    /* Get the branch trace configuration.  */
    virtual const struct btrace_config *btrace_conf (const struct btrace_target_info *)
      TARGET_DEFAULT_RETURN (NULL);

    /* Current recording method.  */
    virtual enum record_method record_method (ptid_t ptid)
      TARGET_DEFAULT_RETURN (RECORD_METHOD_NONE);

    /* Stop trace recording.  */
    virtual void stop_recording ()
      TARGET_DEFAULT_IGNORE ();

    /* Print information about the recording.  */
    virtual void info_record ()
      TARGET_DEFAULT_IGNORE ();

    /* Save the recorded execution trace into a file.  */
    virtual void save_record (const char *filename)
      TARGET_DEFAULT_NORETURN (tcomplain ());

    /* Delete the recorded execution trace from the current position
       onwards.  */
    virtual bool supports_delete_record ()
      TARGET_DEFAULT_RETURN (false);
    virtual void delete_record ()
      TARGET_DEFAULT_NORETURN (tcomplain ());

    /* Query if the record target is currently replaying PTID.  */
    virtual bool record_is_replaying (ptid_t ptid)
      TARGET_DEFAULT_RETURN (false);

    /* Query if the record target will replay PTID if it were resumed in
       execution direction DIR.  */
    virtual bool record_will_replay (ptid_t ptid, int dir)
      TARGET_DEFAULT_RETURN (false);

    /* Stop replaying.  */
    virtual void record_stop_replaying ()
      TARGET_DEFAULT_IGNORE ();

    /* Go to the begin of the execution trace.  */
    virtual void goto_record_begin ()
      TARGET_DEFAULT_NORETURN (tcomplain ());

    /* Go to the end of the execution trace.  */
    virtual void goto_record_end ()
      TARGET_DEFAULT_NORETURN (tcomplain ());

    /* Go to a specific location in the recorded execution trace.  */
    virtual void goto_record (ULONGEST insn)
      TARGET_DEFAULT_NORETURN (tcomplain ());

    /* Disassemble SIZE instructions in the recorded execution trace from
       the current position.
       If SIZE < 0, disassemble abs (SIZE) preceding instructions; otherwise,
       disassemble SIZE succeeding instructions.  */
    virtual void insn_history (int size, gdb_disassembly_flags flags)
      TARGET_DEFAULT_NORETURN (tcomplain ());

    /* Disassemble SIZE instructions in the recorded execution trace around
       FROM.
       If SIZE < 0, disassemble abs (SIZE) instructions before FROM; otherwise,
       disassemble SIZE instructions after FROM.  */
    virtual void insn_history_from (ULONGEST from, int size,
				    gdb_disassembly_flags flags)
      TARGET_DEFAULT_NORETURN (tcomplain ());

    /* Disassemble a section of the recorded execution trace from instruction
       BEGIN (inclusive) to instruction END (inclusive).  */
    virtual void insn_history_range (ULONGEST begin, ULONGEST end,
				     gdb_disassembly_flags flags)
      TARGET_DEFAULT_NORETURN (tcomplain ());

    /* Print a function trace of the recorded execution trace.
       If SIZE < 0, print abs (SIZE) preceding functions; otherwise, print SIZE
       succeeding functions.  */
    virtual void call_history (int size, record_print_flags flags)
      TARGET_DEFAULT_NORETURN (tcomplain ());

    /* Print a function trace of the recorded execution trace starting
       at function FROM.
       If SIZE < 0, print abs (SIZE) functions before FROM; otherwise, print
       SIZE functions after FROM.  */
    virtual void call_history_from (ULONGEST begin, int size, record_print_flags flags)
      TARGET_DEFAULT_NORETURN (tcomplain ());

    /* Print a function trace of an execution trace section from function BEGIN
       (inclusive) to function END (inclusive).  */
    virtual void call_history_range (ULONGEST begin, ULONGEST end, record_print_flags flags)
      TARGET_DEFAULT_NORETURN (tcomplain ());

    /* True if TARGET_OBJECT_LIBRARIES_SVR4 may be read with a
       non-empty annex.  */
    virtual bool augmented_libraries_svr4_read ()
      TARGET_DEFAULT_RETURN (false);

    /* Those unwinders are tried before any other arch unwinders.  If
       SELF doesn't have unwinders, it should delegate to the
       "beneath" target.  */
    virtual const struct frame_unwind *get_unwinder ()
      TARGET_DEFAULT_RETURN (NULL);

    virtual const struct frame_unwind *get_tailcall_unwinder ()
      TARGET_DEFAULT_RETURN (NULL);

    /* Prepare to generate a core file.  */
    virtual void prepare_to_generate_core ()
      TARGET_DEFAULT_IGNORE ();

    /* Cleanup after generating a core file.  */
    virtual void done_generating_core ()
      TARGET_DEFAULT_IGNORE ();

    /* Returns true if the target supports memory tagging, false otherwise.  */
    virtual bool supports_memory_tagging ()
      TARGET_DEFAULT_RETURN (false);

    /* Return the allocated memory tags of type TYPE associated with
       [ADDRESS, ADDRESS + LEN) in TAGS.

       LEN is the number of bytes in the memory range.  TAGS is a vector of
       bytes containing the tags found in the above memory range.

       It is up to the architecture/target to interpret the bytes in the TAGS
       vector and read the tags appropriately.

       Returns true if fetching the tags succeeded and false otherwise.  */
    virtual bool fetch_memtags (CORE_ADDR address, size_t len,
				gdb::byte_vector &tags, int type)
      TARGET_DEFAULT_NORETURN (tcomplain ());

    /* Write the allocation tags of type TYPE contained in TAGS to the memory
       range [ADDRESS, ADDRESS + LEN).

       LEN is the number of bytes in the memory range.  TAGS is a vector of
       bytes containing the tags to be stored to the memory range.

       It is up to the architecture/target to interpret the bytes in the TAGS
       vector and store them appropriately.

       Returns true if storing the tags succeeded and false otherwise.  */
    virtual bool store_memtags (CORE_ADDR address, size_t len,
				const gdb::byte_vector &tags, int type)
      TARGET_DEFAULT_NORETURN (tcomplain ());

    /* Return the x86 XSAVE extended state area layout.  */
    virtual x86_xsave_layout fetch_x86_xsave_layout ()
      TARGET_DEFAULT_RETURN (x86_xsave_layout ());
  };

/* Deleter for std::unique_ptr.  See comments in
   target_ops::~target_ops and target_ops::close about heap-allocated
   targets.  */
struct target_ops_deleter
{
  void operator() (target_ops *target)
  {
    target->close ();
  }
};

/* A unique pointer for target_ops.  */
typedef std::unique_ptr<target_ops, target_ops_deleter> target_ops_up;

/* A policy class to interface gdb::ref_ptr with target_ops.  */

struct target_ops_ref_policy
{
  static void incref (target_ops *t)
  {
    t->incref ();
  }

  /* Decrement the reference count on T, and, if the reference count
     reaches zero, close the target.  */
  static void decref (target_ops *t);
};

/* A gdb::ref_ptr pointer to a target_ops.  */
typedef gdb::ref_ptr<target_ops, target_ops_ref_policy> target_ops_ref;

/* Native target backends call this once at initialization time to
   inform the core about which is the target that can respond to "run"
   or "attach".  Note: native targets are always singletons.  */
extern void set_native_target (target_ops *target);

/* Get the registered native target, if there's one.  Otherwise return
   NULL.  */
extern target_ops *get_native_target ();

/* Type that manages a target stack.  See description of target stacks
   and strata at the top of the file.  */

class target_stack
{
public:
  target_stack () = default;
  DISABLE_COPY_AND_ASSIGN (target_stack);

  /* Push a new target into the stack of the existing target
     accessors, possibly superseding some existing accessor.  */
  void push (target_ops *t);

  /* Remove a target from the stack, wherever it may be.  Return true
     if it was removed, false otherwise.  */
  bool unpush (target_ops *t);

  /* Returns true if T is pushed on the target stack.  */
  bool is_pushed (const target_ops *t) const
  { return at (t->stratum ()) == t; }

  /* Return the target at STRATUM.  */
  target_ops *at (strata stratum) const { return m_stack[stratum].get (); }

  /* Return the target at the top of the stack.  */
  target_ops *top () const { return at (m_top); }

  /* Find the next target down the stack from the specified target.  */
  target_ops *find_beneath (const target_ops *t) const;

private:
  /* The stratum of the top target.  */
  enum strata m_top {};

  /* The stack, represented as an array, with one slot per stratum.
     If no target is pushed at some stratum, the corresponding slot is
     null.  */
  std::array<target_ops_ref, (int) debug_stratum + 1> m_stack;
};

/* Return the dummy target.  */
extern target_ops *get_dummy_target ();

/* Define easy words for doing these operations on our current target.  */

extern const char *target_shortname ();

/* Find the correct target to use for "attach".  If a target on the
   current stack supports attaching, then it is returned.  Otherwise,
   the default run target is returned.  */

extern struct target_ops *find_attach_target (void);

/* Find the correct target to use for "run".  If a target on the
   current stack supports creating a new inferior, then it is
   returned.  Otherwise, the default run target is returned.  */

extern struct target_ops *find_run_target (void);

/* Some targets don't generate traps when attaching to the inferior,
   or their target_attach implementation takes care of the waiting.
   These targets must set to_attach_no_wait.  */

extern bool target_attach_no_wait ();

/* The target_attach operation places a process under debugger control,
   and stops the process.

   This operation provides a target-specific hook that allows the
   necessary bookkeeping to be performed after an attach completes.  */

extern void target_post_attach (int pid);

/* Display a message indicating we're about to attach to a given
   process.  */

extern void target_announce_attach (int from_tty, int pid);

/* Display a message indicating we're about to detach from the current
   inferior process.  */

extern void target_announce_detach (int from_tty);

/* Takes a program previously attached to and detaches it.
   The program may resume execution (some targets do, some don't) and will
   no longer stop on signals, etc.  We better not have left any breakpoints
   in the program or it'll die when it hits one.  FROM_TTY says whether to be
   verbose or not.  */

extern void target_detach (inferior *inf, int from_tty);

/* Disconnect from the current target without resuming it (leaving it
   waiting for a debugger).  */

extern void target_disconnect (const char *, int);

/* Resume execution (or prepare for execution) of the current thread
   (INFERIOR_PTID), while optionally letting other threads of the
   current process or all processes run free.

   STEP says whether to hardware single-step the current thread or to
   let it run free; SIGNAL is the signal to be given to the current
   thread, or GDB_SIGNAL_0 for no signal.  The caller may not pass
   GDB_SIGNAL_DEFAULT.

   SCOPE_PTID indicates the resumption scope.  I.e., which threads
   (other than the current) run free.  If resuming a single thread,
   SCOPE_PTID is the same thread as the current thread.  A wildcard
   SCOPE_PTID (all threads, or all threads of process) lets threads
   other than the current (for which the wildcard SCOPE_PTID matches)
   resume with their 'thread->suspend.stop_signal' signal (usually
   GDB_SIGNAL_0) if it is in "pass" state, or with no signal if in "no
   pass" state.  Note neither STEP nor SIGNAL apply to any thread
   other than the current.

   In order to efficiently handle batches of resumption requests,
   targets may implement this method such that it records the
   resumption request, but defers the actual resumption to the
   target_commit_resume method implementation.  See
   target_commit_resume below.  */
extern void target_resume (ptid_t scope_ptid,
			   int step, enum gdb_signal signal);

/* Ensure that all resumed threads are committed to the target.

   See the description of process_stratum_target::commit_resumed_state
   for more details.  */
extern void target_commit_resumed ();

/* For target_read_memory see target/target.h.  */

/* The default target_ops::to_wait implementation.  */

extern ptid_t default_target_wait (struct target_ops *ops,
				   ptid_t ptid,
				   struct target_waitstatus *status,
				   target_wait_flags options);

/* Return true if the target has pending events to report to the core.
   See target_ops::has_pending_events().  */

extern bool target_has_pending_events ();

/* Fetch at least register REGNO, or all regs if regno == -1.  No result.  */

extern void target_fetch_registers (struct regcache *regcache, int regno);

/* Store at least register REGNO, or all regs if REGNO == -1.
   It can store as many registers as it wants to, so target_prepare_to_store
   must have been previously called.  Calls error() if there are problems.  */

extern void target_store_registers (struct regcache *regcache, int regs);

/* Get ready to modify the registers array.  On machines which store
   individual registers, this doesn't need to do anything.  On machines
   which store all the registers in one fell swoop, this makes sure
   that REGISTERS contains all the registers from the program being
   debugged.  */

extern void target_prepare_to_store (regcache *regcache);

/* Implement the "info proc" command.  This returns one if the request
   was handled, and zero otherwise.  It can also throw an exception if
   an error was encountered while attempting to handle the
   request.  */

int target_info_proc (const char *, enum info_proc_what);

/* Returns true if this target can disable address space randomization.  */

int target_supports_disable_randomization (void);

/* Returns true if this target can enable and disable tracepoints
   while a trace experiment is running.  */

extern bool target_supports_enable_disable_tracepoint ();

extern bool target_supports_string_tracing ();

/* Returns true if this target can handle breakpoint conditions
   on its end.  */

extern bool target_supports_evaluation_of_breakpoint_conditions ();

/* Does this target support dumpcore API?  */

extern bool target_supports_dumpcore ();

/* Generate the core file with target API.  */

extern void target_dumpcore (const char *filename);

/* Returns true if this target can handle breakpoint commands
   on its end.  */

extern bool target_can_run_breakpoint_commands ();

/* For target_read_memory see target/target.h.  */

extern int target_read_raw_memory (CORE_ADDR memaddr, gdb_byte *myaddr,
				   ssize_t len);

extern int target_read_stack (CORE_ADDR memaddr, gdb_byte *myaddr, ssize_t len);

extern int target_read_code (CORE_ADDR memaddr, gdb_byte *myaddr, ssize_t len);

/* For target_write_memory see target/target.h.  */

extern int target_write_raw_memory (CORE_ADDR memaddr, const gdb_byte *myaddr,
				    ssize_t len);

/* Fetches the target's memory map.  If one is found it is sorted
   and returned, after some consistency checking.  Otherwise, NULL
   is returned.  */
std::vector<mem_region> target_memory_map (void);

/* Erases all flash memory regions on the target.  */
void flash_erase_command (const char *cmd, int from_tty);

/* Erase the specified flash region.  */
void target_flash_erase (ULONGEST address, LONGEST length);

/* Finish a sequence of flash operations.  */
void target_flash_done (void);

/* Describes a request for a memory write operation.  */
struct memory_write_request
{
  memory_write_request (ULONGEST begin_, ULONGEST end_,
			gdb_byte *data_ = nullptr, void *baton_ = nullptr)
    : begin (begin_), end (end_), data (data_), baton (baton_)
  {}

  /* Begining address that must be written.  */
  ULONGEST begin;
  /* Past-the-end address.  */
  ULONGEST end;
  /* The data to write.  */
  gdb_byte *data;
  /* A callback baton for progress reporting for this request.  */
  void *baton;
};

/* Enumeration specifying different flash preservation behaviour.  */
enum flash_preserve_mode
  {
    flash_preserve,
    flash_discard
  };

/* Write several memory blocks at once.  This version can be more
   efficient than making several calls to target_write_memory, in
   particular because it can optimize accesses to flash memory.

   Moreover, this is currently the only memory access function in gdb
   that supports writing to flash memory, and it should be used for
   all cases where access to flash memory is desirable.

   REQUESTS is the vector of memory_write_request.
   PRESERVE_FLASH_P indicates what to do with blocks which must be
     erased, but not completely rewritten.
   PROGRESS_CB is a function that will be periodically called to provide
     feedback to user.  It will be called with the baton corresponding
     to the request currently being written.  It may also be called
     with a NULL baton, when preserved flash sectors are being rewritten.

   The function returns 0 on success, and error otherwise.  */
int target_write_memory_blocks
    (const std::vector<memory_write_request> &requests,
     enum flash_preserve_mode preserve_flash_p,
     void (*progress_cb) (ULONGEST, void *));

/* Print a line about the current target.  */

extern void target_files_info ();

/* Insert a breakpoint at address BP_TGT->placed_address in
   the target machine.  Returns 0 for success, and returns non-zero or
   throws an error (with a detailed failure reason error code and
   message) otherwise.  */

extern int target_insert_breakpoint (struct gdbarch *gdbarch,
				     struct bp_target_info *bp_tgt);

/* Remove a breakpoint at address BP_TGT->placed_address in the target
   machine.  Result is 0 for success, non-zero for error.  */

extern int target_remove_breakpoint (struct gdbarch *gdbarch,
				     struct bp_target_info *bp_tgt,
				     enum remove_bp_reason reason);

/* Return true if the target stack has a non-default
  "terminal_ours" method.  */

extern bool target_supports_terminal_ours (void);

/* Kill the inferior process.   Make it go away.  */

extern void target_kill (void);

/* Load an executable file into the target process.  This is expected
   to not only bring new code into the target process, but also to
   update GDB's symbol tables to match.

   ARG contains command-line arguments, to be broken down with
   buildargv ().  The first non-switch argument is the filename to
   load, FILE; the second is a number (as parsed by strtoul (..., ...,
   0)), which is an offset to apply to the load addresses of FILE's
   sections.  The target may define switches, or other non-switch
   arguments, as it pleases.  */

extern void target_load (const char *arg, int from_tty);

/* On some targets, we can catch an inferior fork or vfork event when
   it occurs.  These functions insert/remove an already-created
   catchpoint for such events.  They return  0 for success, 1 if the
   catchpoint type is not supported and -1 for failure.  */

extern int target_insert_fork_catchpoint (int pid);

extern int target_remove_fork_catchpoint (int pid);

extern int target_insert_vfork_catchpoint (int pid);

extern int target_remove_vfork_catchpoint (int pid);

/* Call the follow_fork method on the current target stack.

   This function is called when the inferior forks or vforks, to perform any
   bookkeeping and fiddling necessary to continue debugging either the parent,
   the child or both.  */

void target_follow_fork (inferior *inf, ptid_t child_ptid,
			 target_waitkind fork_kind, bool follow_child,
			 bool detach_fork);

/* Handle the target-specific bookkeeping required when the inferior makes an
   exec call.

   The current inferior at the time of the call is the inferior that did the
   exec.  FOLLOW_INF is the inferior in which execution continues post-exec.
   If "follow-exec-mode" is "same", FOLLOW_INF is the same as the current
   inferior, meaning that execution continues with the same inferior.  If
   "follow-exec-mode" is "new", FOLLOW_INF is a different inferior, meaning
   that execution continues in a new inferior.

   On exit, the target must leave FOLLOW_INF as the current inferior.  */

void target_follow_exec (inferior *follow_inf, ptid_t ptid,
			 const char *execd_pathname);

/* On some targets, we can catch an inferior exec event when it
   occurs.  These functions insert/remove an already-created
   catchpoint for such events.  They return  0 for success, 1 if the
   catchpoint type is not supported and -1 for failure.  */

extern int target_insert_exec_catchpoint (int pid);

extern int target_remove_exec_catchpoint (int pid);

/* Syscall catch.

   NEEDED is true if any syscall catch (of any kind) is requested.
   If NEEDED is false, it means the target can disable the mechanism to
   catch system calls because there are no more catchpoints of this type.

   ANY_COUNT is nonzero if a generic (filter-less) syscall catch is
   being requested.  In this case, SYSCALL_COUNTS should be ignored.

   SYSCALL_COUNTS is an array of ints, indexed by syscall number.  An
   element in this array is nonzero if that syscall should be caught.
   This argument only matters if ANY_COUNT is zero.

   Return 0 for success, 1 if syscall catchpoints are not supported or -1
   for failure.  */

extern int target_set_syscall_catchpoint
  (int pid, bool needed, int any_count,
   gdb::array_view<const int> syscall_counts);

/* The debugger has completed a blocking wait() call.  There is now
   some process event that must be processed.  This function should
   be defined by those targets that require the debugger to perform
   cleanup or internal state changes in response to the process event.  */

/* For target_mourn_inferior see target/target.h.  */

/* Does target have enough data to do a run or attach command?  */

extern int target_can_run ();

/* Set list of signals to be handled in the target.

   PASS_SIGNALS is an array indexed by target signal number
   (enum gdb_signal).  For every signal whose entry in this array is
   non-zero, the target is allowed -but not required- to skip reporting
   arrival of the signal to the GDB core by returning from target_wait,
   and to pass the signal directly to the inferior instead.

   However, if the target is hardware single-stepping a thread that is
   about to receive a signal, it needs to be reported in any case, even
   if mentioned in a previous target_pass_signals call.   */

extern void target_pass_signals
  (gdb::array_view<const unsigned char> pass_signals);

/* Set list of signals the target may pass to the inferior.  This
   directly maps to the "handle SIGNAL pass/nopass" setting.

   PROGRAM_SIGNALS is an array indexed by target signal
   number (enum gdb_signal).  For every signal whose entry in this
   array is non-zero, the target is allowed to pass the signal to the
   inferior.  Signals not present in the array shall be silently
   discarded.  This does not influence whether to pass signals to the
   inferior as a result of a target_resume call.  This is useful in
   scenarios where the target needs to decide whether to pass or not a
   signal to the inferior without GDB core involvement, such as for
   example, when detaching (as threads may have been suspended with
   pending signals not reported to GDB).  */

extern void target_program_signals
  (gdb::array_view<const unsigned char> program_signals);

/* Check to see if a thread is still alive.  */

extern int target_thread_alive (ptid_t ptid);

/* Sync the target's threads with GDB's thread list.  */

extern void target_update_thread_list (void);

/* Make target stop in a continuable fashion.  (For instance, under
   Unix, this should act like SIGSTOP).  Note that this function is
   asynchronous: it does not wait for the target to become stopped
   before returning.  If this is the behavior you want please use
   target_stop_and_wait.  */

extern void target_stop (ptid_t ptid);

/* Interrupt the target.  Unlike target_stop, this does not specify
   which thread/process reports the stop.  For most target this acts
   like raising a SIGINT, though that's not absolutely required.  This
   function is asynchronous.  */

extern void target_interrupt ();

/* Pass a ^C, as determined to have been pressed by checking the quit
   flag, to the target, as if the user had typed the ^C on the
   inferior's controlling terminal while the inferior was in the
   foreground.  Remote targets may take the opportunity to detect the
   remote side is not responding and offer to disconnect.  */

extern void target_pass_ctrlc (void);

/* The default target_ops::to_pass_ctrlc implementation.  Simply calls
   target_interrupt.  */
extern void default_target_pass_ctrlc (struct target_ops *ops);

/* Send the specified COMMAND to the target's monitor
   (shell,interpreter) for execution.  The result of the query is
   placed in OUTBUF.  */

extern void target_rcmd (const char *command, struct ui_file *outbuf);

/* Does the target include memory?  (Dummy targets don't.)  */

extern int target_has_memory ();

/* Does the target have a stack?  (Exec files don't, VxWorks doesn't, until
   we start a process.)  */

extern int target_has_stack ();

/* Does the target have registers?  (Exec files don't.)  */

extern int target_has_registers ();

/* Does the target have execution?  Can we make it jump (through
   hoops), or pop its stack a few times?  This means that the current
   target is currently executing; for some targets, that's the same as
   whether or not the target is capable of execution, but there are
   also targets which can be current while not executing.  In that
   case this will become true after to_create_inferior or
   to_attach.  INF is the inferior to use; nullptr means to use the
   current inferior.  */

extern bool target_has_execution (inferior *inf = nullptr);

/* Can the target support the debugger control of thread execution?
   Can it lock the thread scheduler?  */

extern bool target_can_lock_scheduler ();

/* Controls whether async mode is permitted.  */
extern bool target_async_permitted;

/* Can the target support asynchronous execution?  */
extern bool target_can_async_p ();

/* An overload of the above that can be called when the target is not yet
   pushed, this calls TARGET::can_async_p directly.  */
extern bool target_can_async_p (struct target_ops *target);

/* Is the target in asynchronous execution mode?  */
extern bool target_is_async_p ();

/* Enables/disabled async target events.  */
extern void target_async (bool enable);

/* Enables/disables thread create and exit events.  */
extern void target_thread_events (int enable);

/* Returns true if the target supports setting thread options
   OPTIONS.  */
extern bool target_supports_set_thread_options (gdb_thread_options options);

/* Whether support for controlling the target backends always in
   non-stop mode is enabled.  */
extern enum auto_boolean target_non_stop_enabled;

/* Is the target in non-stop mode?  Some targets control the inferior
   in non-stop mode even with "set non-stop off".  Always true if "set
   non-stop" is on.  */
extern bool target_is_non_stop_p ();

/* Return true if at least one inferior has a non-stop target.  */
extern bool exists_non_stop_target ();

extern exec_direction_kind target_execution_direction ();

/* Converts a process id to a string.  Usually, the string just contains
   `process xyz', but on some systems it may contain
   `process xyz thread abc'.  */

extern std::string target_pid_to_str (ptid_t ptid);

extern std::string normal_pid_to_str (ptid_t ptid);

/* Return a short string describing extra information about PID,
   e.g. "sleeping", "runnable", "running on LWP 3".  Null return value
   is okay.  */

extern const char *target_extra_thread_info (thread_info *tp);

/* Return the thread's name, or NULL if the target is unable to determine it.
   The returned value must not be freed by the caller.

   You likely don't want to call this function, but use the thread_name
   function instead, which prefers the user-given thread name, if set.  */

extern const char *target_thread_name (struct thread_info *);

/* Given a pointer to a thread library specific thread handle and
   its length, return a pointer to the corresponding thread_info struct.  */

extern struct thread_info *target_thread_handle_to_thread_info
  (const gdb_byte *thread_handle, int handle_len, struct inferior *inf);

/* Given a thread, return the thread handle, a target-specific sequence of
   bytes which serves as a thread identifier within the program being
   debugged.  */
extern gdb::array_view<const gdb_byte> target_thread_info_to_thread_handle
  (struct thread_info *);

/* Attempts to find the pathname of the executable file
   that was run to create a specified process.

   The process PID must be stopped when this operation is used.

   If the executable file cannot be determined, NULL is returned.

   Else, a pointer to a character string containing the pathname
   is returned.  This string should be copied into a buffer by
   the client if the string will not be immediately used, or if
   it must persist.  */

extern const char *target_pid_to_exec_file (int pid);

/* See the to_thread_architecture description in struct target_ops.  */

extern gdbarch *target_thread_architecture (ptid_t ptid);

/*
 * Iterator function for target memory regions.
 * Calls a callback function once for each memory region 'mapped'
 * in the child process.  Defined as a simple macro rather than
 * as a function macro so that it can be tested for nullity.
 */

extern int target_find_memory_regions (find_memory_region_ftype func,
				       void *data);

/*
 * Compose corefile .note section.
 */

extern gdb::unique_xmalloc_ptr<char> target_make_corefile_notes (bfd *bfd,
								 int *size_p);

/* Bookmark interfaces.  */
extern gdb_byte *target_get_bookmark (const char *args, int from_tty);

extern void target_goto_bookmark (const gdb_byte *arg, int from_tty);

/* Hardware watchpoint interfaces.  */

/* GDB's current model is that there are three "kinds" of watchpoints,
   with respect to when they trigger and how you can move past them.

   Those are: continuable, steppable, and non-steppable.

   Continuable watchpoints are like x86's -- those trigger after the
   memory access's side effects are fully committed to memory.  I.e.,
   they trap with the PC pointing at the next instruction already.
   Continuing past such a watchpoint is doable by just normally
   continuing, hence the name.

   Both steppable and non-steppable watchpoints trap before the memory
   access.  I.e, the PC points at the instruction that is accessing
   the memory.  So GDB needs to single-step once past the current
   instruction in order to make the access effective and check whether
   the instruction's side effects change the watched expression.

   Now, in order to step past that instruction, depending on
   architecture and target, you can have two situations:

   - steppable watchpoints: you can single-step with the watchpoint
     still armed, and the watchpoint won't trigger again.

   - non-steppable watchpoints: if you try to single-step with the
     watchpoint still armed, you'd trap the watchpoint again and the
     thread wouldn't make any progress.  So GDB needs to temporarily
     remove the watchpoint in order to step past it.

   If your target/architecture does not signal that it has either
   steppable or non-steppable watchpoints via either
   target_have_steppable_watchpoint or
   gdbarch_have_nonsteppable_watchpoint, GDB assumes continuable
   watchpoints.  */

/* Returns true if we were stopped by a hardware watchpoint (memory read or
   write).  Only the INFERIOR_PTID task is being queried.  */

extern bool target_stopped_by_watchpoint ();

/* Returns true if the target stopped because it executed a
   software breakpoint instruction.  */

extern bool target_stopped_by_sw_breakpoint ();

extern bool target_supports_stopped_by_sw_breakpoint ();

extern bool target_stopped_by_hw_breakpoint ();

extern bool target_supports_stopped_by_hw_breakpoint ();

/* True if we have steppable watchpoints  */

extern bool target_have_steppable_watchpoint ();

/* Provide defaults for hardware watchpoint functions.  */

/* If the *_hw_breakpoint functions have not been defined
   elsewhere use the definitions in the target vector.  */

/* Returns positive if we can set a hardware watchpoint of type TYPE.
   Returns negative if the target doesn't have enough hardware debug
   registers available.  Return zero if hardware watchpoint of type
   TYPE isn't supported.  TYPE is one of bp_hardware_watchpoint,
   bp_read_watchpoint, bp_write_watchpoint, or bp_hardware_breakpoint.
   CNT is the number of such watchpoints used so far, including this
   one.  OTHERTYPE is the number of watchpoints of other types than
   this one used so far.  */

extern int target_can_use_hardware_watchpoint (bptype type, int cnt,
					       int othertype);

/* Returns the number of debug registers needed to watch the given
   memory region, or zero if not supported.  */

extern int target_region_ok_for_hw_watchpoint (CORE_ADDR addr, int len);

extern int target_can_do_single_step ();

/* Set/clear a hardware watchpoint starting at ADDR, for LEN bytes.
   TYPE is 0 for write, 1 for read, and 2 for read/write accesses.
   COND is the expression for its condition, or NULL if there's none.
   Returns 0 for success, 1 if the watchpoint type is not supported,
   -1 for failure.  */

extern int target_insert_watchpoint (CORE_ADDR addr, int len,
				     target_hw_bp_type type, expression *cond);

extern int target_remove_watchpoint (CORE_ADDR addr, int len,
				     target_hw_bp_type type, expression *cond);

/* Insert a new masked watchpoint at ADDR using the mask MASK.
   RW may be hw_read for a read watchpoint, hw_write for a write watchpoint
   or hw_access for an access watchpoint.  Returns 0 for success, 1 if
   masked watchpoints are not supported, -1 for failure.  */

extern int target_insert_mask_watchpoint (CORE_ADDR, CORE_ADDR,
					  enum target_hw_bp_type);

/* Remove a masked watchpoint at ADDR with the mask MASK.
   RW may be hw_read for a read watchpoint, hw_write for a write watchpoint
   or hw_access for an access watchpoint.  Returns 0 for success, non-zero
   for failure.  */

extern int target_remove_mask_watchpoint (CORE_ADDR, CORE_ADDR,
					  enum target_hw_bp_type);

/* Insert a hardware breakpoint at address BP_TGT->placed_address in
   the target machine.  Returns 0 for success, and returns non-zero or
   throws an error (with a detailed failure reason error code and
   message) otherwise.  */

extern int target_insert_hw_breakpoint (gdbarch *gdbarch,
					bp_target_info *bp_tgt);

extern int target_remove_hw_breakpoint (gdbarch *gdbarch,
					bp_target_info *bp_tgt);

/* Return number of debug registers needed for a ranged breakpoint,
   or -1 if ranged breakpoints are not supported.  */

extern int target_ranged_break_num_registers (void);

/* Return non-zero if target knows the data address which triggered this
   target_stopped_by_watchpoint, in such case place it to *ADDR_P.  Only the
   INFERIOR_PTID task is being queried.  */
#define target_stopped_data_address(target, addr_p) \
  (target)->stopped_data_address (addr_p)

/* Return non-zero if ADDR is within the range of a watchpoint spanning
   LENGTH bytes beginning at START.  */
#define target_watchpoint_addr_within_range(target, addr, start, length) \
  (target)->watchpoint_addr_within_range (addr, start, length)

/* Return non-zero if the target is capable of using hardware to evaluate
   the condition expression.  In this case, if the condition is false when
   the watched memory location changes, execution may continue without the
   debugger being notified.

   Due to limitations in the hardware implementation, it may be capable of
   avoiding triggering the watchpoint in some cases where the condition
   expression is false, but may report some false positives as well.
   For this reason, GDB will still evaluate the condition expression when
   the watchpoint triggers.  */

extern bool target_can_accel_watchpoint_condition (CORE_ADDR addr, int len,
						   int type, expression *cond);

/* Return number of debug registers needed for a masked watchpoint,
   -1 if masked watchpoints are not supported or -2 if the given address
   and mask combination cannot be used.  */

extern int target_masked_watch_num_registers (CORE_ADDR addr, CORE_ADDR mask);

/* Target can execute in reverse?  */

extern bool target_can_execute_reverse ();

extern const struct target_desc *target_read_description (struct target_ops *);

extern ptid_t target_get_ada_task_ptid (long lwp, ULONGEST tid);

/* Main entry point for searching memory.  */
extern int target_search_memory (CORE_ADDR start_addr,
				 ULONGEST search_space_len,
				 const gdb_byte *pattern,
				 ULONGEST pattern_len,
				 CORE_ADDR *found_addrp);

/* Target file operations.  */

/* Return true if the filesystem seen by the current inferior
   is the local filesystem, zero otherwise.  */

extern bool target_filesystem_is_local ();

/* Open FILENAME on the target, in the filesystem as seen by INF,
   using FLAGS and MODE.  If INF is NULL, use the filesystem seen by
   the debugger (GDB or, for remote targets, the remote stub).  Return
   a target file descriptor, or -1 if an error occurs (and set
   *TARGET_ERRNO).  If WARN_IF_SLOW is true, print a warning message
   if the file is being accessed over a link that may be slow.  */
extern int target_fileio_open (struct inferior *inf,
			       const char *filename, int flags,
			       int mode, bool warn_if_slow,
			       fileio_error *target_errno);

/* Write up to LEN bytes from WRITE_BUF to FD on the target.
   Return the number of bytes written, or -1 if an error occurs
   (and set *TARGET_ERRNO).  */
extern int target_fileio_pwrite (int fd, const gdb_byte *write_buf, int len,
				 ULONGEST offset, fileio_error *target_errno);

/* Read up to LEN bytes FD on the target into READ_BUF.
   Return the number of bytes read, or -1 if an error occurs
   (and set *TARGET_ERRNO).  */
extern int target_fileio_pread (int fd, gdb_byte *read_buf, int len,
				ULONGEST offset, fileio_error *target_errno);

/* Get information about the file opened as FD on the target
   and put it in SB.  Return 0 on success, or -1 if an error
   occurs (and set *TARGET_ERRNO).  */
extern int target_fileio_fstat (int fd, struct stat *sb,
				fileio_error *target_errno);

/* Close FD on the target.  Return 0, or -1 if an error occurs
   (and set *TARGET_ERRNO).  */
extern int target_fileio_close (int fd, fileio_error *target_errno);

/* Unlink FILENAME on the target, in the filesystem as seen by INF.
   If INF is NULL, use the filesystem seen by the debugger (GDB or,
   for remote targets, the remote stub).  Return 0, or -1 if an error
   occurs (and set *TARGET_ERRNO).  */
extern int target_fileio_unlink (struct inferior *inf,
				 const char *filename,
				 fileio_error *target_errno);

/* Read value of symbolic link FILENAME on the target, in the
   filesystem as seen by INF.  If INF is NULL, use the filesystem seen
   by the debugger (GDB or, for remote targets, the remote stub).
   Return a null-terminated string allocated via xmalloc, or NULL if
   an error occurs (and set *TARGET_ERRNO).  */
extern std::optional<std::string> target_fileio_readlink
    (struct inferior *inf, const char *filename, fileio_error *target_errno);

/* Read target file FILENAME, in the filesystem as seen by INF.  If
   INF is NULL, use the filesystem seen by the debugger (GDB or, for
   remote targets, the remote stub).  The return value will be -1 if
   the transfer fails or is not supported; 0 if the object is empty;
   or the length of the object otherwise.  If a positive value is
   returned, a sufficiently large buffer will be allocated using
   xmalloc and returned in *BUF_P containing the contents of the
   object.

   This method should be used for objects sufficiently small to store
   in a single xmalloc'd buffer, when no fixed bound on the object's
   size is known in advance.  */
extern LONGEST target_fileio_read_alloc (struct inferior *inf,
					 const char *filename,
					 gdb_byte **buf_p);

/* Read target file FILENAME, in the filesystem as seen by INF.  If
   INF is NULL, use the filesystem seen by the debugger (GDB or, for
   remote targets, the remote stub).  The result is NUL-terminated and
   returned as a string, allocated using xmalloc.  If an error occurs
   or the transfer is unsupported, NULL is returned.  Empty objects
   are returned as allocated but empty strings.  A warning is issued
   if the result contains any embedded NUL bytes.  */
extern gdb::unique_xmalloc_ptr<char> target_fileio_read_stralloc
    (struct inferior *inf, const char *filename);

/* Invalidate the target associated with open handles that were open
   on target TARG, since we're about to close (and maybe destroy) the
   target.  The handles remain open from the client's perspective, but
   trying to do anything with them other than closing them will fail
   with EIO.  */
extern void fileio_handles_invalidate_target (target_ops *targ);

/* Tracepoint-related operations.  */

extern void target_trace_init ();

extern void target_download_tracepoint (bp_location *location);

extern bool target_can_download_tracepoint ();

extern void target_download_trace_state_variable (const trace_state_variable &tsv);

extern void target_enable_tracepoint (bp_location *loc);

extern void target_disable_tracepoint (bp_location *loc);

extern void target_trace_start ();

extern void target_trace_set_readonly_regions ();

extern int target_get_trace_status (trace_status *ts);

extern void target_get_tracepoint_status (tracepoint *tp, uploaded_tp *utp);

extern void target_trace_stop ();

extern int target_trace_find (trace_find_type type, int num, CORE_ADDR addr1,
			      CORE_ADDR addr2, int *tpp);

extern bool target_get_trace_state_variable_value (int tsv, LONGEST *val);

extern int target_save_trace_data (const char *filename);

extern int target_upload_tracepoints (uploaded_tp **utpp);

extern int target_upload_trace_state_variables (uploaded_tsv **utsvp);

extern LONGEST target_get_raw_trace_data (gdb_byte *buf, ULONGEST offset,
					  LONGEST len);

extern int target_get_min_fast_tracepoint_insn_len ();

extern void target_set_disconnected_tracing (int val);

extern void target_set_circular_trace_buffer (int val);

extern void target_set_trace_buffer_size (LONGEST val);

extern bool target_set_trace_notes (const char *user, const char *notes,
				    const char *stopnotes);

extern bool target_get_tib_address (ptid_t ptid, CORE_ADDR *addr);

extern void target_set_permissions ();

extern bool target_static_tracepoint_marker_at
  (CORE_ADDR addr, static_tracepoint_marker *marker);

extern std::vector<static_tracepoint_marker>
  target_static_tracepoint_markers_by_strid (const char *marker_id);

extern traceframe_info_up target_traceframe_info ();

extern bool target_use_agent (bool use);

extern bool target_can_use_agent ();

extern bool target_augmented_libraries_svr4_read ();

extern bool target_supports_memory_tagging ();

extern bool target_fetch_memtags (CORE_ADDR address, size_t len,
				  gdb::byte_vector &tags, int type);

extern bool target_store_memtags (CORE_ADDR address, size_t len,
				  const gdb::byte_vector &tags, int type);

extern x86_xsave_layout target_fetch_x86_xsave_layout ();

/* Command logging facility.  */

extern void target_log_command (const char *p);

extern int target_core_of_thread (ptid_t ptid);

/* See to_get_unwinder in struct target_ops.  */
extern const struct frame_unwind *target_get_unwinder (void);

/* See to_get_tailcall_unwinder in struct target_ops.  */
extern const struct frame_unwind *target_get_tailcall_unwinder (void);

/* This implements basic memory verification, reading target memory
   and performing the comparison here (as opposed to accelerated
   verification making use of the qCRC packet, for example).  */

extern int simple_verify_memory (struct target_ops* ops,
				 const gdb_byte *data,
				 CORE_ADDR memaddr, ULONGEST size);

/* Verify that the memory in the [MEMADDR, MEMADDR+SIZE) range matches
   the contents of [DATA,DATA+SIZE).  Returns 1 if there's a match, 0
   if there's a mismatch, and -1 if an error is encountered while
   reading memory.  Throws an error if the functionality is found not
   to be supported by the current target.  */
int target_verify_memory (const gdb_byte *data,
			  CORE_ADDR memaddr, ULONGEST size);

/* Routines for maintenance of the target structures...

   add_target:   Add a target to the list of all possible targets.
   This only makes sense for targets that should be activated using
   the "target TARGET_NAME ..." command.

   push_target:  Make this target the top of the stack of currently used
   targets, within its particular stratum of the stack.  Result
   is 0 if now atop the stack, nonzero if not on top (maybe
   should warn user).

   unpush_target: Remove this from the stack of currently used targets,
   no matter where it is on the list.  Returns 0 if no
   change, 1 if removed from stack.  */

/* Type of callback called when the user activates a target with
   "target TARGET_NAME".  The callback routine takes the rest of the
   parameters from the command, and (if successful) pushes a new
   target onto the stack.  */
typedef void target_open_ftype (const char *args, int from_tty);

/* Add the target described by INFO to the list of possible targets
   and add a new command 'target $(INFO->shortname)'.  Set COMPLETER
   as the command's completer if not NULL.  */

extern void add_target (const target_info &info,
			target_open_ftype *func,
			completer_ftype *completer = NULL);

/* Adds a command ALIAS for the target described by INFO and marks it
   deprecated.  This is useful for maintaining backwards compatibility
   when renaming targets.  */

extern void add_deprecated_target_alias (const target_info &info,
					 const char *alias);

/* A unique_ptr helper to unpush a target.  */

struct target_unpusher
{
  void operator() (struct target_ops *ops) const;
};

/* A unique_ptr that unpushes a target on destruction.  */

typedef std::unique_ptr<struct target_ops, target_unpusher> target_unpush_up;

extern void target_pre_inferior (int);

extern void target_preopen (int);

extern CORE_ADDR target_translate_tls_address (struct objfile *objfile,
					       CORE_ADDR offset);

/* Return the "section" containing the specified address.  */
const struct target_section *target_section_by_addr (struct target_ops *target,
						     CORE_ADDR addr);

/* Return the target section table this target (or the targets
   beneath) currently manipulate.  */

extern const std::vector<target_section> *target_get_section_table
  (struct target_ops *target);

/* Default implementation of get_section_table for dummy_target.  */

extern const std::vector<target_section> *default_get_section_table ();

/* From mem-break.c */

extern int memory_remove_breakpoint (struct target_ops *,
				     struct gdbarch *, struct bp_target_info *,
				     enum remove_bp_reason);

extern int memory_insert_breakpoint (struct target_ops *,
				     struct gdbarch *, struct bp_target_info *);

/* Convenience template use to add memory breakpoints support to a
   target.  */

template <typename BaseTarget>
struct memory_breakpoint_target : public BaseTarget
{
  int insert_breakpoint (struct gdbarch *gdbarch,
			 struct bp_target_info *bp_tgt) override
  { return memory_insert_breakpoint (this, gdbarch, bp_tgt); }

  int remove_breakpoint (struct gdbarch *gdbarch,
			 struct bp_target_info *bp_tgt,
			 enum remove_bp_reason reason) override
  { return memory_remove_breakpoint (this, gdbarch, bp_tgt, reason); }
};

/* Check whether the memory at the breakpoint's placed address still
   contains the expected breakpoint instruction.  */

extern int memory_validate_breakpoint (struct gdbarch *gdbarch,
				       struct bp_target_info *bp_tgt);

extern int default_memory_remove_breakpoint (struct gdbarch *,
					     struct bp_target_info *);

extern int default_memory_insert_breakpoint (struct gdbarch *,
					     struct bp_target_info *);


/* From target.c */

extern void initialize_targets (void);

extern void noprocess (void) ATTRIBUTE_NORETURN;

extern void target_require_runnable (void);

/* Find the target at STRATUM.  If no target is at that stratum,
   return NULL.  */

struct target_ops *find_target_at (enum strata stratum);

/* Read OS data object of type TYPE from the target, and return it in XML
   format.  The return value follows the same rules as target_read_stralloc.  */

extern std::optional<gdb::char_vector> target_get_osdata (const char *type);

/* Stuff that should be shared among the various remote targets.  */


/* Timeout limit for response from target.  */
extern int remote_timeout;



/* Set the show memory breakpoints mode to show, and return a
   scoped_restore to restore it back to the current value.  */
extern scoped_restore_tmpl<int>
    make_scoped_restore_show_memory_breakpoints (int show);

/* True if we should trust readonly sections from the
   executable when reading memory.  */
extern bool trust_readonly;

extern bool may_write_registers;
extern bool may_write_memory;
extern bool may_insert_breakpoints;
extern bool may_insert_tracepoints;
extern bool may_insert_fast_tracepoints;
extern bool may_stop;

extern void update_target_permissions (void);


/* Imported from machine dependent code.  */

/* See to_enable_btrace in struct target_ops.  */
extern struct btrace_target_info *
  target_enable_btrace (thread_info *tp, const struct btrace_config *);

/* See to_disable_btrace in struct target_ops.  */
extern void target_disable_btrace (struct btrace_target_info *btinfo);

/* See to_teardown_btrace in struct target_ops.  */
extern void target_teardown_btrace (struct btrace_target_info *btinfo);

/* See to_read_btrace in struct target_ops.  */
extern enum btrace_error target_read_btrace (struct btrace_data *,
					     struct btrace_target_info *,
					     enum btrace_read_type);

/* See to_btrace_conf in struct target_ops.  */
extern const struct btrace_config *
  target_btrace_conf (const struct btrace_target_info *);

/* See to_stop_recording in struct target_ops.  */
extern void target_stop_recording (void);

/* See to_save_record in struct target_ops.  */
extern void target_save_record (const char *filename);

/* Query if the target supports deleting the execution log.  */
extern int target_supports_delete_record (void);

/* See to_delete_record in struct target_ops.  */
extern void target_delete_record (void);

/* See to_record_method.  */
extern enum record_method target_record_method (ptid_t ptid);

/* See to_record_is_replaying in struct target_ops.  */
extern int target_record_is_replaying (ptid_t ptid);

/* See to_record_will_replay in struct target_ops.  */
extern int target_record_will_replay (ptid_t ptid, int dir);

/* See to_record_stop_replaying in struct target_ops.  */
extern void target_record_stop_replaying (void);

/* See to_goto_record_begin in struct target_ops.  */
extern void target_goto_record_begin (void);

/* See to_goto_record_end in struct target_ops.  */
extern void target_goto_record_end (void);

/* See to_goto_record in struct target_ops.  */
extern void target_goto_record (ULONGEST insn);

/* See to_insn_history.  */
extern void target_insn_history (int size, gdb_disassembly_flags flags);

/* See to_insn_history_from.  */
extern void target_insn_history_from (ULONGEST from, int size,
				      gdb_disassembly_flags flags);

/* See to_insn_history_range.  */
extern void target_insn_history_range (ULONGEST begin, ULONGEST end,
				       gdb_disassembly_flags flags);

/* See to_call_history.  */
extern void target_call_history (int size, record_print_flags flags);

/* See to_call_history_from.  */
extern void target_call_history_from (ULONGEST begin, int size,
				      record_print_flags flags);

/* See to_call_history_range.  */
extern void target_call_history_range (ULONGEST begin, ULONGEST end,
				       record_print_flags flags);

/* See to_prepare_to_generate_core.  */
extern void target_prepare_to_generate_core (void);

/* See to_done_generating_core.  */
extern void target_done_generating_core (void);

#endif /* !defined (TARGET_H) */
