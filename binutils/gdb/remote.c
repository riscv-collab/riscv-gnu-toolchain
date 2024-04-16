/* Remote target communications for serial-line targets in custom GDB protocol

   Copyright (C) 1988-2024 Free Software Foundation, Inc.

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

/* See the GDB User Guide for details of the GDB remote protocol.  */

#include "defs.h"
#include <ctype.h>
#include <fcntl.h>
#include "inferior.h"
#include "infrun.h"
#include "bfd.h"
#include "symfile.h"
#include "target.h"
#include "process-stratum-target.h"
#include "gdbcmd.h"
#include "objfiles.h"
#include "gdbthread.h"
#include "remote.h"
#include "remote-notif.h"
#include "regcache.h"
#include "value.h"
#include "observable.h"
#include "solib.h"
#include "cli/cli-decode.h"
#include "cli/cli-setshow.h"
#include "target-descriptions.h"
#include "gdb_bfd.h"
#include "gdbsupport/filestuff.h"
#include "gdbsupport/rsp-low.h"
#include "disasm.h"
#include "location.h"

#include "gdbsupport/gdb_sys_time.h"

#include "gdbsupport/event-loop.h"
#include "event-top.h"
#include "inf-loop.h"

#include <signal.h>
#include "serial.h"

#include "gdbcore.h"

#include "remote-fileio.h"
#include "gdbsupport/fileio.h"
#include <sys/stat.h>
#include "xml-support.h"

#include "memory-map.h"

#include "tracepoint.h"
#include "ax.h"
#include "ax-gdb.h"
#include "gdbsupport/agent.h"
#include "btrace.h"
#include "record-btrace.h"
#include "gdbsupport/scoped_restore.h"
#include "gdbsupport/environ.h"
#include "gdbsupport/byte-vector.h"
#include "gdbsupport/search.h"
#include <algorithm>
#include <iterator>
#include <unordered_map>
#include "async-event.h"
#include "gdbsupport/selftest.h"

/* The remote target.  */

static const char remote_doc[] = N_("\
Use a remote computer via a serial line, using a gdb-specific protocol.\n\
Specify the serial device it is connected to\n\
(e.g. /dev/ttyS0, /dev/ttya, COM1, etc.).");

/* See remote.h  */

bool remote_debug = false;

#define OPAQUETHREADBYTES 8

/* a 64 bit opaque identifier */
typedef unsigned char threadref[OPAQUETHREADBYTES];

struct gdb_ext_thread_info;
struct threads_listing_context;
typedef int (*rmt_thread_action) (threadref *ref, void *context);
struct protocol_feature;
struct packet_reg;

struct stop_reply;
typedef std::unique_ptr<stop_reply> stop_reply_up;

/* Generic configuration support for packets the stub optionally
   supports.  Allows the user to specify the use of the packet as well
   as allowing GDB to auto-detect support in the remote stub.  */

enum packet_support
  {
    PACKET_SUPPORT_UNKNOWN = 0,
    PACKET_ENABLE,
    PACKET_DISABLE
  };

/* Convert the packet support auto_boolean to a name used for gdb printing.  */

static const char *
get_packet_support_name (auto_boolean support)
{
  switch (support)
    {
      case AUTO_BOOLEAN_TRUE:
	return "on";
      case AUTO_BOOLEAN_FALSE:
	return "off";
      case AUTO_BOOLEAN_AUTO:
	return "auto";
      default:
	gdb_assert_not_reached ("invalid var_auto_boolean");
    }
}

/* Convert the target type (future remote target or currently connected target)
   to a name used for gdb printing.  */

static const char *
get_target_type_name (bool target_connected)
{
  if (target_connected)
    return _("on the current remote target");
  else
    return _("on future remote targets");
}

/* Analyze a packet's return value and update the packet config
   accordingly.  */

enum packet_result
{
  PACKET_ERROR,
  PACKET_OK,
  PACKET_UNKNOWN
};

/* Enumeration of packets for a remote target.  */

enum {
  PACKET_vCont = 0,
  PACKET_X,
  PACKET_qSymbol,
  PACKET_P,
  PACKET_p,
  PACKET_Z0,
  PACKET_Z1,
  PACKET_Z2,
  PACKET_Z3,
  PACKET_Z4,
  PACKET_vFile_setfs,
  PACKET_vFile_open,
  PACKET_vFile_pread,
  PACKET_vFile_pwrite,
  PACKET_vFile_close,
  PACKET_vFile_unlink,
  PACKET_vFile_readlink,
  PACKET_vFile_fstat,
  PACKET_qXfer_auxv,
  PACKET_qXfer_features,
  PACKET_qXfer_exec_file,
  PACKET_qXfer_libraries,
  PACKET_qXfer_libraries_svr4,
  PACKET_qXfer_memory_map,
  PACKET_qXfer_osdata,
  PACKET_qXfer_threads,
  PACKET_qXfer_statictrace_read,
  PACKET_qXfer_traceframe_info,
  PACKET_qXfer_uib,
  PACKET_qGetTIBAddr,
  PACKET_qGetTLSAddr,
  PACKET_qSupported,
  PACKET_qTStatus,
  PACKET_QPassSignals,
  PACKET_QCatchSyscalls,
  PACKET_QProgramSignals,
  PACKET_QSetWorkingDir,
  PACKET_QStartupWithShell,
  PACKET_QEnvironmentHexEncoded,
  PACKET_QEnvironmentReset,
  PACKET_QEnvironmentUnset,
  PACKET_qCRC,
  PACKET_qSearch_memory,
  PACKET_vAttach,
  PACKET_vRun,
  PACKET_QStartNoAckMode,
  PACKET_vKill,
  PACKET_qXfer_siginfo_read,
  PACKET_qXfer_siginfo_write,
  PACKET_qAttached,

  /* Support for conditional tracepoints.  */
  PACKET_ConditionalTracepoints,

  /* Support for target-side breakpoint conditions.  */
  PACKET_ConditionalBreakpoints,

  /* Support for target-side breakpoint commands.  */
  PACKET_BreakpointCommands,

  /* Support for fast tracepoints.  */
  PACKET_FastTracepoints,

  /* Support for static tracepoints.  */
  PACKET_StaticTracepoints,

  /* Support for installing tracepoints while a trace experiment is
     running.  */
  PACKET_InstallInTrace,

  PACKET_bc,
  PACKET_bs,
  PACKET_TracepointSource,
  PACKET_QAllow,
  PACKET_qXfer_fdpic,
  PACKET_QDisableRandomization,
  PACKET_QAgent,
  PACKET_QTBuffer_size,
  PACKET_Qbtrace_off,
  PACKET_Qbtrace_bts,
  PACKET_Qbtrace_pt,
  PACKET_qXfer_btrace,

  /* Support for the QNonStop packet.  */
  PACKET_QNonStop,

  /* Support for the QThreadEvents packet.  */
  PACKET_QThreadEvents,

  /* Support for the QThreadOptions packet.  */
  PACKET_QThreadOptions,

  /* Support for multi-process extensions.  */
  PACKET_multiprocess_feature,

  /* Support for enabling and disabling tracepoints while a trace
     experiment is running.  */
  PACKET_EnableDisableTracepoints_feature,

  /* Support for collecting strings using the tracenz bytecode.  */
  PACKET_tracenz_feature,

  /* Support for continuing to run a trace experiment while GDB is
     disconnected.  */
  PACKET_DisconnectedTracing_feature,

  /* Support for qXfer:libraries-svr4:read with a non-empty annex.  */
  PACKET_augmented_libraries_svr4_read_feature,

  /* Support for the qXfer:btrace-conf:read packet.  */
  PACKET_qXfer_btrace_conf,

  /* Support for the Qbtrace-conf:bts:size packet.  */
  PACKET_Qbtrace_conf_bts_size,

  /* Support for swbreak+ feature.  */
  PACKET_swbreak_feature,

  /* Support for hwbreak+ feature.  */
  PACKET_hwbreak_feature,

  /* Support for fork events.  */
  PACKET_fork_event_feature,

  /* Support for vfork events.  */
  PACKET_vfork_event_feature,

  /* Support for the Qbtrace-conf:pt:size packet.  */
  PACKET_Qbtrace_conf_pt_size,

  /* Support for exec events.  */
  PACKET_exec_event_feature,

  /* Support for query supported vCont actions.  */
  PACKET_vContSupported,

  /* Support remote CTRL-C.  */
  PACKET_vCtrlC,

  /* Support TARGET_WAITKIND_NO_RESUMED.  */
  PACKET_no_resumed,

  /* Support for memory tagging, allocation tag fetch/store
     packets and the tag violation stop replies.  */
  PACKET_memory_tagging_feature,

  PACKET_MAX
};

struct threads_listing_context;

/* Stub vCont actions support.

   Each field is a boolean flag indicating whether the stub reports
   support for the corresponding action.  */

struct vCont_action_support
{
  /* vCont;t */
  bool t = false;

  /* vCont;r */
  bool r = false;

  /* vCont;s */
  bool s = false;

  /* vCont;S */
  bool S = false;
};

/* About this many threadids fit in a packet.  */

#define MAXTHREADLISTRESULTS 32

/* Data for the vFile:pread readahead cache.  */

struct readahead_cache
{
  /* Invalidate the readahead cache.  */
  void invalidate ();

  /* Invalidate the readahead cache if it is holding data for FD.  */
  void invalidate_fd (int fd);

  /* Serve pread from the readahead cache.  Returns number of bytes
     read, or 0 if the request can't be served from the cache.  */
  int pread (int fd, gdb_byte *read_buf, size_t len, ULONGEST offset);

  /* The file descriptor for the file that is being cached.  -1 if the
     cache is invalid.  */
  int fd = -1;

  /* The offset into the file that the cache buffer corresponds
     to.  */
  ULONGEST offset = 0;

  /* The buffer holding the cache contents.  */
  gdb::byte_vector buf;

  /* Cache hit and miss counters.  */
  ULONGEST hit_count = 0;
  ULONGEST miss_count = 0;
};

/* Description of the remote protocol for a given architecture.  */

struct packet_reg
{
  long offset; /* Offset into G packet.  */
  long regnum; /* GDB's internal register number.  */
  LONGEST pnum; /* Remote protocol register number.  */
  int in_g_packet; /* Always part of G packet.  */
  /* long size in bytes;  == register_size (arch, regnum);
     at present.  */
  /* char *name; == gdbarch_register_name (arch, regnum);
     at present.  */
};

struct remote_arch_state
{
  explicit remote_arch_state (struct gdbarch *gdbarch);

  /* Description of the remote protocol registers.  */
  long sizeof_g_packet;

  /* Description of the remote protocol registers indexed by REGNUM
     (making an array gdbarch_num_regs in size).  */
  std::unique_ptr<packet_reg[]> regs;

  /* This is the size (in chars) of the first response to the ``g''
     packet.  It is used as a heuristic when determining the maximum
     size of memory-read and memory-write packets.  A target will
     typically only reserve a buffer large enough to hold the ``g''
     packet.  The size does not include packet overhead (headers and
     trailers).  */
  long actual_register_packet_size;

  /* This is the maximum size (in chars) of a non read/write packet.
     It is also used as a cap on the size of read/write packets.  */
  long remote_packet_size;
};

/* Description of the remote protocol state for the currently
   connected target.  This is per-target state, and independent of the
   selected architecture.  */

class remote_state
{
public:

  remote_state ();
  ~remote_state ();

  /* Get the remote arch state for GDBARCH.  */
  struct remote_arch_state *get_remote_arch_state (struct gdbarch *gdbarch);

  void create_async_event_handler ()
  {
    gdb_assert (m_async_event_handler_token == nullptr);
    m_async_event_handler_token
      = ::create_async_event_handler ([] (gdb_client_data data)
				      {
					inferior_event_handler (INF_REG_EVENT);
				      },
				      nullptr, "remote");
  }

  void mark_async_event_handler ()
  {
    gdb_assert (this->is_async_p ());
    ::mark_async_event_handler (m_async_event_handler_token);
  }

  void clear_async_event_handler ()
  { ::clear_async_event_handler (m_async_event_handler_token); }

  bool async_event_handler_marked () const
  { return ::async_event_handler_marked (m_async_event_handler_token); }

  void delete_async_event_handler ()
  {
    if (m_async_event_handler_token != nullptr)
      ::delete_async_event_handler (&m_async_event_handler_token);
  }

  bool is_async_p () const
  {
    /* We're async whenever the serial device is.  */
    gdb_assert (this->remote_desc != nullptr);
    return serial_is_async_p (this->remote_desc);
  }

  bool can_async_p () const
  {
    /* We can async whenever the serial device can.  */
    gdb_assert (this->remote_desc != nullptr);
    return serial_can_async_p (this->remote_desc);
  }

public: /* data */

  /* A buffer to use for incoming packets, and its current size.  The
     buffer is grown dynamically for larger incoming packets.
     Outgoing packets may also be constructed in this buffer.
     The size of the buffer is always at least REMOTE_PACKET_SIZE;
     REMOTE_PACKET_SIZE should be used to limit the length of outgoing
     packets.  */
  gdb::char_vector buf;

  /* True if we're going through initial connection setup (finding out
     about the remote side's threads, relocating symbols, etc.).  */
  bool starting_up = false;

  /* If we negotiated packet size explicitly (and thus can bypass
     heuristics for the largest packet size that will not overflow
     a buffer in the stub), this will be set to that packet size.
     Otherwise zero, meaning to use the guessed size.  */
  long explicit_packet_size = 0;

  /* True, if in no ack mode.  That is, neither GDB nor the stub will
     expect acks from each other.  The connection is assumed to be
     reliable.  */
  bool noack_mode = false;

  /* True if we're connected in extended remote mode.  */
  bool extended = false;

  /* True if we resumed the target and we're waiting for the target to
     stop.  In the mean time, we can't start another command/query.
     The remote server wouldn't be ready to process it, so we'd
     timeout waiting for a reply that would never come and eventually
     we'd close the connection.  This can happen in asynchronous mode
     because we allow GDB commands while the target is running.  */
  bool waiting_for_stop_reply = false;

  /* The status of the stub support for the various vCont actions.  */
  vCont_action_support supports_vCont;

  /* True if the user has pressed Ctrl-C, but the target hasn't
     responded to that.  */
  bool ctrlc_pending_p = false;

  /* True if we saw a Ctrl-C while reading or writing from/to the
     remote descriptor.  At that point it is not safe to send a remote
     interrupt packet, so we instead remember we saw the Ctrl-C and
     process it once we're done with sending/receiving the current
     packet, which should be shortly.  If however that takes too long,
     and the user presses Ctrl-C again, we offer to disconnect.  */
  bool got_ctrlc_during_io = false;

  /* Descriptor for I/O to remote machine.  Initialize it to NULL so that
     remote_open knows that we don't have a file open when the program
     starts.  */
  struct serial *remote_desc = nullptr;

  /* These are the threads which we last sent to the remote system.  The
     TID member will be -1 for all or -2 for not sent yet.  */
  ptid_t general_thread = null_ptid;
  ptid_t continue_thread = null_ptid;

  /* This is the traceframe which we last selected on the remote system.
     It will be -1 if no traceframe is selected.  */
  int remote_traceframe_number = -1;

  char *last_pass_packet = nullptr;

  /* The last QProgramSignals packet sent to the target.  We bypass
     sending a new program signals list down to the target if the new
     packet is exactly the same as the last we sent.  IOW, we only let
     the target know about program signals list changes.  */
  char *last_program_signals_packet = nullptr;

  /* Similarly, the last QThreadEvents state we sent to the
     target.  */
  bool last_thread_events = false;

  gdb_signal last_sent_signal = GDB_SIGNAL_0;

  bool last_sent_step = false;

  /* The execution direction of the last resume we got.  */
  exec_direction_kind last_resume_exec_dir = EXEC_FORWARD;

  char *finished_object = nullptr;
  char *finished_annex = nullptr;
  ULONGEST finished_offset = 0;

  /* Should we try the 'ThreadInfo' query packet?

     This variable (NOT available to the user: auto-detect only!)
     determines whether GDB will use the new, simpler "ThreadInfo"
     query or the older, more complex syntax for thread queries.
     This is an auto-detect variable (set to true at each connect,
     and set to false when the target fails to recognize it).  */
  bool use_threadinfo_query = false;
  bool use_threadextra_query = false;

  threadref echo_nextthread {};
  threadref nextthread {};
  threadref resultthreadlist[MAXTHREADLISTRESULTS] {};

  /* The state of remote notification.  */
  struct remote_notif_state *notif_state = nullptr;

  /* The branch trace configuration.  */
  struct btrace_config btrace_config {};

  /* The argument to the last "vFile:setfs:" packet we sent, used
     to avoid sending repeated unnecessary "vFile:setfs:" packets.
     Initialized to -1 to indicate that no "vFile:setfs:" packet
     has yet been sent.  */
  int fs_pid = -1;

  /* A readahead cache for vFile:pread.  Often, reading a binary
     involves a sequence of small reads.  E.g., when parsing an ELF
     file.  A readahead cache helps mostly the case of remote
     debugging on a connection with higher latency, due to the
     request/reply nature of the RSP.  We only cache data for a single
     file descriptor at a time.  */
  struct readahead_cache readahead_cache;

  /* The list of already fetched and acknowledged stop events.  This
     queue is used for notification Stop, and other notifications
     don't need queue for their events, because the notification
     events of Stop can't be consumed immediately, so that events
     should be queued first, and be consumed by remote_wait_{ns,as}
     one per time.  Other notifications can consume their events
     immediately, so queue is not needed for them.  */
  std::vector<stop_reply_up> stop_reply_queue;

  /* FIXME: cagney/1999-09-23: Even though getpkt was called with
     ``forever'' still use the normal timeout mechanism.  This is
     currently used by the ASYNC code to guarentee that target reads
     during the initial connect always time-out.  Once getpkt has been
     modified to return a timeout indication and, in turn
     remote_wait()/wait_for_inferior() have gained a timeout parameter
     this can go away.  */
  bool wait_forever_enabled_p = true;

  /* The set of thread options the target reported it supports, via
     qSupported.  */
  gdb_thread_options supported_thread_options = 0;

private:
  /* Asynchronous signal handle registered as event loop source for
     when we have pending events ready to be passed to the core.  */
  async_event_handler *m_async_event_handler_token = nullptr;

  /* Mapping of remote protocol data for each gdbarch.  Usually there
     is only one entry here, though we may see more with stubs that
     support multi-process.  */
  std::unordered_map<struct gdbarch *, remote_arch_state>
    m_arch_states;
};

static const target_info remote_target_info = {
  "remote",
  N_("Remote target using gdb-specific protocol"),
  remote_doc
};

/* Description of a remote packet.  */

struct packet_description
{
  /* Name of the packet used for gdb output.  */
  const char *name;

  /* Title of the packet, used by the set/show remote name-packet
     commands to identify the individual packages and gdb output.  */
  const char *title;
};

/* Configuration of a remote packet.  */

struct packet_config
{
  /* If auto, GDB auto-detects support for this packet or feature,
     either through qSupported, or by trying the packet and looking
     at the response.  If true, GDB assumes the target supports this
     packet.  If false, the packet is disabled.  Configs that don't
     have an associated command always have this set to auto.  */
  enum auto_boolean detect;

  /* Does the target support this packet?  */
  enum packet_support support;
};

/* User configurable variables for the number of characters in a
   memory read/write packet.  MIN (rsa->remote_packet_size,
   rsa->sizeof_g_packet) is the default.  Some targets need smaller
   values (fifo overruns, et.al.) and some users need larger values
   (speed up transfers).  The variables ``preferred_*'' (the user
   request), ``current_*'' (what was actually set) and ``forced_*''
   (Positive - a soft limit, negative - a hard limit).  */

struct memory_packet_config
{
  const char *name;
  long size;
  int fixed_p;
};

/* These global variables contain the default configuration for every new
   remote_feature object.  */
static memory_packet_config memory_read_packet_config =
{
  "memory-read-packet-size",
};
static memory_packet_config memory_write_packet_config =
{
  "memory-write-packet-size",
};

/* This global array contains packet descriptions (name and title).  */
static packet_description packets_descriptions[PACKET_MAX];
/* This global array contains the default configuration for every new
   per-remote target array.  */
static packet_config remote_protocol_packets[PACKET_MAX];

/* Description of a remote target's features.  It stores the configuration
   and provides functions to determine supported features of the target.  */

struct remote_features
{
  remote_features ()
  {
    m_memory_read_packet_config = memory_read_packet_config;
    m_memory_write_packet_config = memory_write_packet_config;

    std::copy (std::begin (remote_protocol_packets),
	       std::end (remote_protocol_packets),
	       std::begin (m_protocol_packets));
  }
  ~remote_features () = default;

  DISABLE_COPY_AND_ASSIGN (remote_features);

  /* Returns whether a given packet defined by its enum value is supported.  */
  enum packet_support packet_support (int) const;

  /* Returns the packet's corresponding "set remote foo-packet" command
     state.  See struct packet_config for more details.  */
  enum auto_boolean packet_set_cmd_state (int packet) const
  { return m_protocol_packets[packet].detect; }

  /* Returns true if the multi-process extensions are in effect.  */
  int remote_multi_process_p () const
  { return packet_support (PACKET_multiprocess_feature) == PACKET_ENABLE; }

  /* Returns true if fork events are supported.  */
  int remote_fork_event_p () const
  { return packet_support (PACKET_fork_event_feature) == PACKET_ENABLE; }

  /* Returns true if vfork events are supported.  */
  int remote_vfork_event_p () const
  { return packet_support (PACKET_vfork_event_feature) == PACKET_ENABLE; }

  /* Returns true if exec events are supported.  */
  int remote_exec_event_p () const
  { return packet_support (PACKET_exec_event_feature) == PACKET_ENABLE; }

  /* Returns true if memory tagging is supported, false otherwise.  */
  bool remote_memory_tagging_p () const
  { return packet_support (PACKET_memory_tagging_feature) == PACKET_ENABLE; }

  /* Reset all packets back to "unknown support".  Called when opening a
     new connection to a remote target.  */
  void reset_all_packet_configs_support ();

/* Check result value in BUF for packet WHICH_PACKET and update the packet's
   support configuration accordingly.  */
  packet_result packet_ok (const char *buf, const int which_packet);
  packet_result packet_ok (const gdb::char_vector &buf, const int which_packet);

  /* Configuration of a remote target's memory read packet.  */
  memory_packet_config m_memory_read_packet_config;
  /* Configuration of a remote target's memory write packet.  */
  memory_packet_config m_memory_write_packet_config;

  /* The per-remote target array which stores a remote's packet
     configurations.  */
  packet_config m_protocol_packets[PACKET_MAX];
};

class remote_target : public process_stratum_target
{
public:
  remote_target () = default;
  ~remote_target () override;

  const target_info &info () const override
  { return remote_target_info; }

  const char *connection_string () override;

  thread_control_capabilities get_thread_control_capabilities () override
  { return tc_schedlock; }

  /* Open a remote connection.  */
  static void open (const char *, int);

  void close () override;

  void detach (inferior *, int) override;
  void disconnect (const char *, int) override;

  void commit_requested_thread_options ();

  void commit_resumed () override;
  void resume (ptid_t, int, enum gdb_signal) override;
  ptid_t wait (ptid_t, struct target_waitstatus *, target_wait_flags) override;
  bool has_pending_events () override;

  void fetch_registers (struct regcache *, int) override;
  void store_registers (struct regcache *, int) override;
  void prepare_to_store (struct regcache *) override;

  int insert_breakpoint (struct gdbarch *, struct bp_target_info *) override;

  int remove_breakpoint (struct gdbarch *, struct bp_target_info *,
			 enum remove_bp_reason) override;


  bool stopped_by_sw_breakpoint () override;
  bool supports_stopped_by_sw_breakpoint () override;

  bool stopped_by_hw_breakpoint () override;

  bool supports_stopped_by_hw_breakpoint () override;

  bool stopped_by_watchpoint () override;

  bool stopped_data_address (CORE_ADDR *) override;

  bool watchpoint_addr_within_range (CORE_ADDR, CORE_ADDR, int) override;

  int can_use_hw_breakpoint (enum bptype, int, int) override;

  int insert_hw_breakpoint (struct gdbarch *, struct bp_target_info *) override;

  int remove_hw_breakpoint (struct gdbarch *, struct bp_target_info *) override;

  int region_ok_for_hw_watchpoint (CORE_ADDR, int) override;

  int insert_watchpoint (CORE_ADDR, int, enum target_hw_bp_type,
			 struct expression *) override;

  int remove_watchpoint (CORE_ADDR, int, enum target_hw_bp_type,
			 struct expression *) override;

  void kill () override;

  void load (const char *, int) override;

  void mourn_inferior () override;

  void pass_signals (gdb::array_view<const unsigned char>) override;

  int set_syscall_catchpoint (int, bool, int,
			      gdb::array_view<const int>) override;

  void program_signals (gdb::array_view<const unsigned char>) override;

  bool thread_alive (ptid_t ptid) override;

  const char *thread_name (struct thread_info *) override;

  void update_thread_list () override;

  std::string pid_to_str (ptid_t) override;

  const char *extra_thread_info (struct thread_info *) override;

  ptid_t get_ada_task_ptid (long lwp, ULONGEST thread) override;

  thread_info *thread_handle_to_thread_info (const gdb_byte *thread_handle,
					     int handle_len,
					     inferior *inf) override;

  gdb::array_view<const gdb_byte> thread_info_to_thread_handle (struct thread_info *tp)
    override;

  void stop (ptid_t) override;

  void interrupt () override;

  void pass_ctrlc () override;

  enum target_xfer_status xfer_partial (enum target_object object,
					const char *annex,
					gdb_byte *readbuf,
					const gdb_byte *writebuf,
					ULONGEST offset, ULONGEST len,
					ULONGEST *xfered_len) override;

  ULONGEST get_memory_xfer_limit () override;

  void rcmd (const char *command, struct ui_file *output) override;

  const char *pid_to_exec_file (int pid) override;

  void log_command (const char *cmd) override
  {
    serial_log_command (this, cmd);
  }

  CORE_ADDR get_thread_local_address (ptid_t ptid,
				      CORE_ADDR load_module_addr,
				      CORE_ADDR offset) override;

  bool can_execute_reverse () override;

  std::vector<mem_region> memory_map () override;

  void flash_erase (ULONGEST address, LONGEST length) override;

  void flash_done () override;

  const struct target_desc *read_description () override;

  int search_memory (CORE_ADDR start_addr, ULONGEST search_space_len,
		     const gdb_byte *pattern, ULONGEST pattern_len,
		     CORE_ADDR *found_addrp) override;

  bool can_async_p () override;

  bool is_async_p () override;

  void async (bool) override;

  int async_wait_fd () override;

  void thread_events (int) override;

  bool supports_set_thread_options (gdb_thread_options) override;

  int can_do_single_step () override;

  void terminal_inferior () override;

  void terminal_ours () override;

  bool supports_non_stop () override;

  bool supports_multi_process () override;

  bool supports_disable_randomization () override;

  bool filesystem_is_local () override;


  int fileio_open (struct inferior *inf, const char *filename,
		   int flags, int mode, int warn_if_slow,
		   fileio_error *target_errno) override;

  int fileio_pwrite (int fd, const gdb_byte *write_buf, int len,
		     ULONGEST offset, fileio_error *target_errno) override;

  int fileio_pread (int fd, gdb_byte *read_buf, int len,
		    ULONGEST offset, fileio_error *target_errno) override;

  int fileio_fstat (int fd, struct stat *sb, fileio_error *target_errno) override;

  int fileio_close (int fd, fileio_error *target_errno) override;

  int fileio_unlink (struct inferior *inf,
		     const char *filename,
		     fileio_error *target_errno) override;

  std::optional<std::string>
    fileio_readlink (struct inferior *inf,
		     const char *filename,
		     fileio_error *target_errno) override;

  bool supports_enable_disable_tracepoint () override;

  bool supports_string_tracing () override;

  int remote_supports_cond_tracepoints ();

  bool supports_evaluation_of_breakpoint_conditions () override;

  int remote_supports_fast_tracepoints ();

  int remote_supports_static_tracepoints ();

  int remote_supports_install_in_trace ();

  bool can_run_breakpoint_commands () override;

  void trace_init () override;

  void download_tracepoint (struct bp_location *location) override;

  bool can_download_tracepoint () override;

  void download_trace_state_variable (const trace_state_variable &tsv) override;

  void enable_tracepoint (struct bp_location *location) override;

  void disable_tracepoint (struct bp_location *location) override;

  void trace_set_readonly_regions () override;

  void trace_start () override;

  int get_trace_status (struct trace_status *ts) override;

  void get_tracepoint_status (tracepoint *tp, struct uploaded_tp *utp)
    override;

  void trace_stop () override;

  int trace_find (enum trace_find_type type, int num,
		  CORE_ADDR addr1, CORE_ADDR addr2, int *tpp) override;

  bool get_trace_state_variable_value (int tsv, LONGEST *val) override;

  int save_trace_data (const char *filename) override;

  int upload_tracepoints (struct uploaded_tp **utpp) override;

  int upload_trace_state_variables (struct uploaded_tsv **utsvp) override;

  LONGEST get_raw_trace_data (gdb_byte *buf, ULONGEST offset, LONGEST len) override;

  int get_min_fast_tracepoint_insn_len () override;

  void set_disconnected_tracing (int val) override;

  void set_circular_trace_buffer (int val) override;

  void set_trace_buffer_size (LONGEST val) override;

  bool set_trace_notes (const char *user, const char *notes,
			const char *stopnotes) override;

  int core_of_thread (ptid_t ptid) override;

  int verify_memory (const gdb_byte *data,
		     CORE_ADDR memaddr, ULONGEST size) override;


  bool get_tib_address (ptid_t ptid, CORE_ADDR *addr) override;

  void set_permissions () override;

  bool static_tracepoint_marker_at (CORE_ADDR,
				    struct static_tracepoint_marker *marker)
    override;

  std::vector<static_tracepoint_marker>
    static_tracepoint_markers_by_strid (const char *id) override;

  traceframe_info_up traceframe_info () override;

  bool use_agent (bool use) override;
  bool can_use_agent () override;

  struct btrace_target_info *
    enable_btrace (thread_info *tp, const struct btrace_config *conf) override;

  void disable_btrace (struct btrace_target_info *tinfo) override;

  void teardown_btrace (struct btrace_target_info *tinfo) override;

  enum btrace_error read_btrace (struct btrace_data *data,
				 struct btrace_target_info *btinfo,
				 enum btrace_read_type type) override;

  const struct btrace_config *btrace_conf (const struct btrace_target_info *) override;
  bool augmented_libraries_svr4_read () override;
  void follow_fork (inferior *, ptid_t, target_waitkind, bool, bool) override;
  void follow_clone (ptid_t child_ptid) override;
  void follow_exec (inferior *, ptid_t, const char *) override;
  int insert_fork_catchpoint (int) override;
  int remove_fork_catchpoint (int) override;
  int insert_vfork_catchpoint (int) override;
  int remove_vfork_catchpoint (int) override;
  int insert_exec_catchpoint (int) override;
  int remove_exec_catchpoint (int) override;
  enum exec_direction_kind execution_direction () override;

  bool supports_memory_tagging () override;

  bool fetch_memtags (CORE_ADDR address, size_t len,
		      gdb::byte_vector &tags, int type) override;

  bool store_memtags (CORE_ADDR address, size_t len,
		      const gdb::byte_vector &tags, int type) override;

public: /* Remote specific methods.  */

  void remote_download_command_source (int num, ULONGEST addr,
				       struct command_line *cmds);

  void remote_file_put (const char *local_file, const char *remote_file,
			int from_tty);
  void remote_file_get (const char *remote_file, const char *local_file,
			int from_tty);
  void remote_file_delete (const char *remote_file, int from_tty);

  int remote_hostio_pread (int fd, gdb_byte *read_buf, int len,
			   ULONGEST offset, fileio_error *remote_errno);
  int remote_hostio_pwrite (int fd, const gdb_byte *write_buf, int len,
			    ULONGEST offset, fileio_error *remote_errno);
  int remote_hostio_pread_vFile (int fd, gdb_byte *read_buf, int len,
				 ULONGEST offset, fileio_error *remote_errno);

  int remote_hostio_send_command (int command_bytes, int which_packet,
				  fileio_error *remote_errno, const char **attachment,
				  int *attachment_len);
  int remote_hostio_set_filesystem (struct inferior *inf,
				    fileio_error *remote_errno);
  /* We should get rid of this and use fileio_open directly.  */
  int remote_hostio_open (struct inferior *inf, const char *filename,
			  int flags, int mode, int warn_if_slow,
			  fileio_error *remote_errno);
  int remote_hostio_close (int fd, fileio_error *remote_errno);

  int remote_hostio_unlink (inferior *inf, const char *filename,
			    fileio_error *remote_errno);

  struct remote_state *get_remote_state ();

  long get_remote_packet_size (void);
  long get_memory_packet_size (struct memory_packet_config *config);

  long get_memory_write_packet_size ();
  long get_memory_read_packet_size ();

  char *append_pending_thread_resumptions (char *p, char *endp,
					   ptid_t ptid);
  static void open_1 (const char *name, int from_tty, int extended_p);
  void start_remote (int from_tty, int extended_p);
  void remote_detach_1 (struct inferior *inf, int from_tty);

  char *append_resumption (char *p, char *endp,
			   ptid_t ptid, int step, gdb_signal siggnal);
  int remote_resume_with_vcont (ptid_t scope_ptid, int step,
				gdb_signal siggnal);

  thread_info *add_current_inferior_and_thread (const char *wait_status);

  ptid_t wait_ns (ptid_t ptid, struct target_waitstatus *status,
		  target_wait_flags options);
  ptid_t wait_as (ptid_t ptid, target_waitstatus *status,
		  target_wait_flags options);

  ptid_t process_stop_reply (stop_reply_up stop_reply,
			     target_waitstatus *status);

  ptid_t select_thread_for_ambiguous_stop_reply
    (const struct target_waitstatus &status);

  void remote_notice_new_inferior (ptid_t currthread, bool executing);

  void print_one_stopped_thread (thread_info *thread);
  void process_initial_stop_replies (int from_tty);

  thread_info *remote_add_thread (ptid_t ptid, bool running, bool executing,
				  bool silent_p);

  void btrace_sync_conf (const btrace_config *conf);

  void remote_btrace_maybe_reopen ();

  void remove_new_children (threads_listing_context *context);
  void kill_new_fork_children (inferior *inf);
  void discard_pending_stop_replies (struct inferior *inf);
  int stop_reply_queue_length ();

  void check_pending_events_prevent_wildcard_vcont
    (bool *may_global_wildcard_vcont);

  void discard_pending_stop_replies_in_queue ();
  stop_reply_up remote_notif_remove_queued_reply (ptid_t ptid);
  stop_reply_up queued_stop_reply (ptid_t ptid);
  int peek_stop_reply (ptid_t ptid);
  void remote_parse_stop_reply (const char *buf, stop_reply *event);

  void remote_stop_ns (ptid_t ptid);
  void remote_interrupt_as ();
  void remote_interrupt_ns ();

  char *remote_get_noisy_reply ();
  int remote_query_attached (int pid);
  inferior *remote_add_inferior (bool fake_pid_p, int pid, int attached,
				 int try_open_exec);

  ptid_t remote_current_thread (ptid_t oldpid);
  ptid_t get_current_thread (const char *wait_status);

  void set_thread (ptid_t ptid, int gen);
  void set_general_thread (ptid_t ptid);
  void set_continue_thread (ptid_t ptid);
  void set_general_process ();

  char *write_ptid (char *buf, const char *endbuf, ptid_t ptid);

  int remote_unpack_thread_info_response (const char *pkt, threadref *expectedref,
					  gdb_ext_thread_info *info);
  int remote_get_threadinfo (threadref *threadid, int fieldset,
			     gdb_ext_thread_info *info);

  int parse_threadlist_response (const char *pkt, int result_limit,
				 threadref *original_echo,
				 threadref *resultlist,
				 int *doneflag);
  int remote_get_threadlist (int startflag, threadref *nextthread,
			     int result_limit, int *done, int *result_count,
			     threadref *threadlist);

  int remote_threadlist_iterator (rmt_thread_action stepfunction,
				  void *context, int looplimit);

  int remote_get_threads_with_ql (threads_listing_context *context);
  int remote_get_threads_with_qxfer (threads_listing_context *context);
  int remote_get_threads_with_qthreadinfo (threads_listing_context *context);

  void extended_remote_restart ();

  void get_offsets ();

  void remote_check_symbols ();

  void remote_supported_packet (const struct protocol_feature *feature,
				enum packet_support support,
				const char *argument);

  void remote_query_supported ();

  void remote_packet_size (const protocol_feature *feature,
			   packet_support support, const char *value);
  void remote_supported_thread_options (const protocol_feature *feature,
					enum packet_support support,
					const char *value);

  void remote_serial_quit_handler ();

  void remote_detach_pid (int pid);

  void remote_vcont_probe ();

  void remote_resume_with_hc (ptid_t ptid, int step,
			      gdb_signal siggnal);

  void send_interrupt_sequence ();
  void interrupt_query ();

  void remote_notif_get_pending_events (const notif_client *nc);

  int fetch_register_using_p (struct regcache *regcache,
			      packet_reg *reg);
  int send_g_packet ();
  void process_g_packet (struct regcache *regcache);
  void fetch_registers_using_g (struct regcache *regcache);
  int store_register_using_P (const struct regcache *regcache,
			      packet_reg *reg);
  void store_registers_using_G (const struct regcache *regcache);

  void set_remote_traceframe ();

  void check_binary_download (CORE_ADDR addr);

  target_xfer_status remote_write_bytes_aux (const char *header,
					     CORE_ADDR memaddr,
					     const gdb_byte *myaddr,
					     ULONGEST len_units,
					     int unit_size,
					     ULONGEST *xfered_len_units,
					     char packet_format,
					     int use_length);

  target_xfer_status remote_write_bytes (CORE_ADDR memaddr,
					 const gdb_byte *myaddr, ULONGEST len,
					 int unit_size, ULONGEST *xfered_len);

  target_xfer_status remote_read_bytes_1 (CORE_ADDR memaddr, gdb_byte *myaddr,
					  ULONGEST len_units,
					  int unit_size, ULONGEST *xfered_len_units);

  target_xfer_status remote_xfer_live_readonly_partial (gdb_byte *readbuf,
							ULONGEST memaddr,
							ULONGEST len,
							int unit_size,
							ULONGEST *xfered_len);

  target_xfer_status remote_read_bytes (CORE_ADDR memaddr,
					gdb_byte *myaddr, ULONGEST len,
					int unit_size,
					ULONGEST *xfered_len);

  packet_result remote_send_printf (const char *format, ...)
    ATTRIBUTE_PRINTF (2, 3);

  target_xfer_status remote_flash_write (ULONGEST address,
					 ULONGEST length, ULONGEST *xfered_len,
					 const gdb_byte *data);

  int readchar (int timeout);

  void remote_serial_write (const char *str, int len);
  void remote_serial_send_break ();

  int putpkt (const char *buf);
  int putpkt_binary (const char *buf, int cnt);

  int putpkt (const gdb::char_vector &buf)
  {
    return putpkt (buf.data ());
  }

  void skip_frame ();
  long read_frame (gdb::char_vector *buf_p);
  int getpkt (gdb::char_vector *buf, bool forever = false,
	      bool *is_notif = nullptr);
  int remote_vkill (int pid);
  void remote_kill_k ();

  void extended_remote_disable_randomization (int val);
  int extended_remote_run (const std::string &args);

  void send_environment_packet (const char *action,
				const char *packet,
				const char *value);

  void extended_remote_environment_support ();
  void extended_remote_set_inferior_cwd ();

  target_xfer_status remote_write_qxfer (const char *object_name,
					 const char *annex,
					 const gdb_byte *writebuf,
					 ULONGEST offset, LONGEST len,
					 ULONGEST *xfered_len,
					 const unsigned int which_packet);

  target_xfer_status remote_read_qxfer (const char *object_name,
					const char *annex,
					gdb_byte *readbuf, ULONGEST offset,
					LONGEST len,
					ULONGEST *xfered_len,
					const unsigned int which_packet);

  void push_stop_reply (stop_reply_up new_event);

  bool vcont_r_supported ();

  remote_features m_features;

private:

  bool start_remote_1 (int from_tty, int extended_p);

  /* The remote state.  Don't reference this directly.  Use the
     get_remote_state method instead.  */
  remote_state m_remote_state;
};

static const target_info extended_remote_target_info = {
  "extended-remote",
  N_("Extended remote target using gdb-specific protocol"),
  remote_doc
};

/* Set up the extended remote target by extending the standard remote
   target and adding to it.  */

class extended_remote_target final : public remote_target
{
public:
  const target_info &info () const override
  { return extended_remote_target_info; }

  /* Open an extended-remote connection.  */
  static void open (const char *, int);

  bool can_create_inferior () override { return true; }
  void create_inferior (const char *, const std::string &,
			char **, int) override;

  void detach (inferior *, int) override;

  bool can_attach () override { return true; }
  void attach (const char *, int) override;

  void post_attach (int) override;
  bool supports_disable_randomization () override;
};

struct stop_reply : public notif_event
{
  /* The identifier of the thread about this event  */
  ptid_t ptid;

  /* The remote state this event is associated with.  When the remote
     connection, represented by a remote_state object, is closed,
     all the associated stop_reply events should be released.  */
  struct remote_state *rs;

  struct target_waitstatus ws;

  /* The architecture associated with the expedited registers.  */
  gdbarch *arch;

  /* Expedited registers.  This makes remote debugging a bit more
     efficient for those targets that provide critical registers as
     part of their normal status mechanism (as another roundtrip to
     fetch them is avoided).  */
  std::vector<cached_reg_t> regcache;

  enum target_stop_reason stop_reason;

  CORE_ADDR watch_data_address;

  int core;
};

/* Return TARGET as a remote_target if it is one, else nullptr.  */

static remote_target *
as_remote_target (process_stratum_target *target)
{
  return dynamic_cast<remote_target *> (target);
}

/* See remote.h.  */

bool
is_remote_target (process_stratum_target *target)
{
  return as_remote_target (target) != nullptr;
}

/* Per-program-space data key.  */
static const registry<program_space>::key<char, gdb::xfree_deleter<char>>
  remote_pspace_data;

/* The variable registered as the control variable used by the
   remote exec-file commands.  While the remote exec-file setting is
   per-program-space, the set/show machinery uses this as the
   location of the remote exec-file value.  */
static std::string remote_exec_file_var;

/* The size to align memory write packets, when practical.  The protocol
   does not guarantee any alignment, and gdb will generate short
   writes and unaligned writes, but even as a best-effort attempt this
   can improve bulk transfers.  For instance, if a write is misaligned
   relative to the target's data bus, the stub may need to make an extra
   round trip fetching data from the target.  This doesn't make a
   huge difference, but it's easy to do, so we try to be helpful.

   The alignment chosen is arbitrary; usually data bus width is
   important here, not the possibly larger cache line size.  */
enum { REMOTE_ALIGN_WRITES = 16 };

/* Prototypes for local functions.  */

static int hexnumlen (ULONGEST num);

static int stubhex (int ch);

static int hexnumstr (char *, ULONGEST);

static int hexnumnstr (char *, ULONGEST, int);

static CORE_ADDR remote_address_masked (CORE_ADDR);

static int stub_unpack_int (const char *buff, int fieldlength);

static void set_remote_protocol_packet_cmd (const char *args, int from_tty,
					    cmd_list_element *c);

static void show_packet_config_cmd (ui_file *file,
				    const unsigned int which_packet,
				    remote_target *remote);

static void show_remote_protocol_packet_cmd (struct ui_file *file,
					     int from_tty,
					     struct cmd_list_element *c,
					     const char *value);

static ptid_t read_ptid (const char *buf, const char **obuf);

static bool remote_read_description_p (struct target_ops *target);

static void remote_console_output (const char *msg);

static void remote_btrace_reset (remote_state *rs);

static void remote_unpush_and_throw (remote_target *target);

/* For "remote".  */

static struct cmd_list_element *remote_cmdlist;

/* For "set remote" and "show remote".  */

static struct cmd_list_element *remote_set_cmdlist;
static struct cmd_list_element *remote_show_cmdlist;

/* Controls whether GDB is willing to use range stepping.  */

static bool use_range_stepping = true;

/* From the remote target's point of view, each thread is in one of these three
   states.  */
enum class resume_state
{
  /* Not resumed - we haven't been asked to resume this thread.  */
  NOT_RESUMED,

  /* We have been asked to resume this thread, but haven't sent a vCont action
     for it yet.  We'll need to consider it next time commit_resume is
     called.  */
  RESUMED_PENDING_VCONT,

  /* We have been asked to resume this thread, and we have sent a vCont action
     for it.  */
  RESUMED,
};

/* Information about a thread's pending vCont-resume.  Used when a thread is in
   the remote_resume_state::RESUMED_PENDING_VCONT state.  remote_target::resume
   stores this information which is then picked up by
   remote_target::commit_resume to know which is the proper action for this
   thread to include in the vCont packet.  */
struct resumed_pending_vcont_info
{
  /* True if the last resume call for this thread was a step request, false
     if a continue request.  */
  bool step;

  /* The signal specified in the last resume call for this thread.  */
  gdb_signal sig;
};

/* Private data that we'll store in (struct thread_info)->priv.  */
struct remote_thread_info : public private_thread_info
{
  std::string extra;
  std::string name;
  int core = -1;

  /* Thread handle, perhaps a pthread_t or thread_t value, stored as a
     sequence of bytes.  */
  gdb::byte_vector thread_handle;

  /* Whether the target stopped for a breakpoint/watchpoint.  */
  enum target_stop_reason stop_reason = TARGET_STOPPED_BY_NO_REASON;

  /* This is set to the data address of the access causing the target
     to stop for a watchpoint.  */
  CORE_ADDR watch_data_address = 0;

  /* Get the thread's resume state.  */
  enum resume_state get_resume_state () const
  {
    return m_resume_state;
  }

  /* Put the thread in the NOT_RESUMED state.  */
  void set_not_resumed ()
  {
    m_resume_state = resume_state::NOT_RESUMED;
  }

  /* Put the thread in the RESUMED_PENDING_VCONT state.  */
  void set_resumed_pending_vcont (bool step, gdb_signal sig)
  {
    m_resume_state = resume_state::RESUMED_PENDING_VCONT;
    m_resumed_pending_vcont_info.step = step;
    m_resumed_pending_vcont_info.sig = sig;
  }

  /* Get the information this thread's pending vCont-resumption.

     Must only be called if the thread is in the RESUMED_PENDING_VCONT resume
     state.  */
  const struct resumed_pending_vcont_info &resumed_pending_vcont_info () const
  {
    gdb_assert (m_resume_state == resume_state::RESUMED_PENDING_VCONT);

    return m_resumed_pending_vcont_info;
  }

  /* Put the thread in the VCONT_RESUMED state.  */
  void set_resumed ()
  {
    m_resume_state = resume_state::RESUMED;
  }

private:
  /* Resume state for this thread.  This is used to implement vCont action
     coalescing (only when the target operates in non-stop mode).

     remote_target::resume moves the thread to the RESUMED_PENDING_VCONT state,
     which notes that this thread must be considered in the next commit_resume
     call.

     remote_target::commit_resume sends a vCont packet with actions for the
     threads in the RESUMED_PENDING_VCONT state and moves them to the
     VCONT_RESUMED state.

     When reporting a stop to the core for a thread, that thread is moved back
     to the NOT_RESUMED state.  */
  enum resume_state m_resume_state = resume_state::NOT_RESUMED;

  /* Extra info used if the thread is in the RESUMED_PENDING_VCONT state.  */
  struct resumed_pending_vcont_info m_resumed_pending_vcont_info;
};

remote_state::remote_state ()
  : buf (400)
{
}

remote_state::~remote_state ()
{
  xfree (this->last_pass_packet);
  xfree (this->last_program_signals_packet);
  xfree (this->finished_object);
  xfree (this->finished_annex);
}

/* Utility: generate error from an incoming stub packet.  */
static void
trace_error (char *buf)
{
  if (*buf++ != 'E')
    return;			/* not an error msg */
  switch (*buf)
    {
    case '1':			/* malformed packet error */
      if (*++buf == '0')	/*   general case: */
	error (_("remote.c: error in outgoing packet."));
      else
	error (_("remote.c: error in outgoing packet at field #%ld."),
	       strtol (buf, NULL, 16));
    default:
      error (_("Target returns error code '%s'."), buf);
    }
}

/* Utility: wait for reply from stub, while accepting "O" packets.  */

char *
remote_target::remote_get_noisy_reply ()
{
  struct remote_state *rs = get_remote_state ();

  do				/* Loop on reply from remote stub.  */
    {
      char *buf;

      QUIT;			/* Allow user to bail out with ^C.  */
      getpkt (&rs->buf);
      buf = rs->buf.data ();
      if (buf[0] == 'E')
	trace_error (buf);
      else if (startswith (buf, "qRelocInsn:"))
	{
	  ULONGEST ul;
	  CORE_ADDR from, to, org_to;
	  const char *p, *pp;
	  int adjusted_size = 0;
	  int relocated = 0;

	  p = buf + strlen ("qRelocInsn:");
	  pp = unpack_varlen_hex (p, &ul);
	  if (*pp != ';')
	    error (_("invalid qRelocInsn packet: %s"), buf);
	  from = ul;

	  p = pp + 1;
	  unpack_varlen_hex (p, &ul);
	  to = ul;

	  org_to = to;

	  try
	    {
	      gdbarch_relocate_instruction (current_inferior ()->arch (),
					    &to, from);
	      relocated = 1;
	    }
	  catch (const gdb_exception &ex)
	    {
	      if (ex.error == MEMORY_ERROR)
		{
		  /* Propagate memory errors silently back to the
		     target.  The stub may have limited the range of
		     addresses we can write to, for example.  */
		}
	      else
		{
		  /* Something unexpectedly bad happened.  Be verbose
		     so we can tell what, and propagate the error back
		     to the stub, so it doesn't get stuck waiting for
		     a response.  */
		  exception_fprintf (gdb_stderr, ex,
				     _("warning: relocating instruction: "));
		}
	      putpkt ("E01");
	    }

	  if (relocated)
	    {
	      adjusted_size = to - org_to;

	      xsnprintf (buf, rs->buf.size (), "qRelocInsn:%x", adjusted_size);
	      putpkt (buf);
	    }
	}
      else if (buf[0] == 'O' && buf[1] != 'K')
	remote_console_output (buf + 1);	/* 'O' message from stub */
      else
	return buf;		/* Here's the actual reply.  */
    }
  while (1);
}

struct remote_arch_state *
remote_state::get_remote_arch_state (struct gdbarch *gdbarch)
{
  remote_arch_state *rsa;

  auto it = this->m_arch_states.find (gdbarch);
  if (it == this->m_arch_states.end ())
    {
      auto p = this->m_arch_states.emplace (std::piecewise_construct,
					    std::forward_as_tuple (gdbarch),
					    std::forward_as_tuple (gdbarch));
      rsa = &p.first->second;

      /* Make sure that the packet buffer is plenty big enough for
	 this architecture.  */
      if (this->buf.size () < rsa->remote_packet_size)
	this->buf.resize (2 * rsa->remote_packet_size);
    }
  else
    rsa = &it->second;

  return rsa;
}

/* Fetch the global remote target state.  */

remote_state *
remote_target::get_remote_state ()
{
  /* Make sure that the remote architecture state has been
     initialized, because doing so might reallocate rs->buf.  Any
     function which calls getpkt also needs to be mindful of changes
     to rs->buf, but this call limits the number of places which run
     into trouble.  */
  m_remote_state.get_remote_arch_state (current_inferior ()->arch ());

  return &m_remote_state;
}

/* Fetch the remote exec-file from the current program space.  */

static const char *
get_remote_exec_file (void)
{
  char *remote_exec_file;

  remote_exec_file = remote_pspace_data.get (current_program_space);
  if (remote_exec_file == NULL)
    return "";

  return remote_exec_file;
}

/* Set the remote exec file for PSPACE.  */

static void
set_pspace_remote_exec_file (struct program_space *pspace,
			     const char *remote_exec_file)
{
  char *old_file = remote_pspace_data.get (pspace);

  xfree (old_file);
  remote_pspace_data.set (pspace, xstrdup (remote_exec_file));
}

/* The "set/show remote exec-file" set command hook.  */

static void
set_remote_exec_file (const char *ignored, int from_tty,
		      struct cmd_list_element *c)
{
  set_pspace_remote_exec_file (current_program_space,
			       remote_exec_file_var.c_str ());
}

/* The "set/show remote exec-file" show command hook.  */

static void
show_remote_exec_file (struct ui_file *file, int from_tty,
		       struct cmd_list_element *cmd, const char *value)
{
  gdb_printf (file, "%s\n", get_remote_exec_file ());
}

static int
map_regcache_remote_table (struct gdbarch *gdbarch, struct packet_reg *regs)
{
  int regnum, num_remote_regs, offset;
  struct packet_reg **remote_regs;

  for (regnum = 0; regnum < gdbarch_num_regs (gdbarch); regnum++)
    {
      struct packet_reg *r = &regs[regnum];

      if (register_size (gdbarch, regnum) == 0)
	/* Do not try to fetch zero-sized (placeholder) registers.  */
	r->pnum = -1;
      else
	r->pnum = gdbarch_remote_register_number (gdbarch, regnum);

      r->regnum = regnum;
    }

  /* Define the g/G packet format as the contents of each register
     with a remote protocol number, in order of ascending protocol
     number.  */

  remote_regs = XALLOCAVEC (struct packet_reg *, gdbarch_num_regs (gdbarch));
  for (num_remote_regs = 0, regnum = 0;
       regnum < gdbarch_num_regs (gdbarch);
       regnum++)
    if (regs[regnum].pnum != -1)
      remote_regs[num_remote_regs++] = &regs[regnum];

  std::sort (remote_regs, remote_regs + num_remote_regs,
	     [] (const packet_reg *a, const packet_reg *b)
	      { return a->pnum < b->pnum; });

  for (regnum = 0, offset = 0; regnum < num_remote_regs; regnum++)
    {
      remote_regs[regnum]->in_g_packet = 1;
      remote_regs[regnum]->offset = offset;
      offset += register_size (gdbarch, remote_regs[regnum]->regnum);
    }

  return offset;
}

/* Given the architecture described by GDBARCH, return the remote
   protocol register's number and the register's offset in the g/G
   packets of GDB register REGNUM, in PNUM and POFFSET respectively.
   If the target does not have a mapping for REGNUM, return false,
   otherwise, return true.  */

int
remote_register_number_and_offset (struct gdbarch *gdbarch, int regnum,
				   int *pnum, int *poffset)
{
  gdb_assert (regnum < gdbarch_num_regs (gdbarch));

  std::vector<packet_reg> regs (gdbarch_num_regs (gdbarch));

  map_regcache_remote_table (gdbarch, regs.data ());

  *pnum = regs[regnum].pnum;
  *poffset = regs[regnum].offset;

  return *pnum != -1;
}

remote_arch_state::remote_arch_state (struct gdbarch *gdbarch)
{
  /* Use the architecture to build a regnum<->pnum table, which will be
     1:1 unless a feature set specifies otherwise.  */
  this->regs.reset (new packet_reg [gdbarch_num_regs (gdbarch)] ());

  /* Record the maximum possible size of the g packet - it may turn out
     to be smaller.  */
  this->sizeof_g_packet
    = map_regcache_remote_table (gdbarch, this->regs.get ());

  /* Default maximum number of characters in a packet body.  Many
     remote stubs have a hardwired buffer size of 400 bytes
     (c.f. BUFMAX in m68k-stub.c and i386-stub.c).  BUFMAX-1 is used
     as the maximum packet-size to ensure that the packet and an extra
     NUL character can always fit in the buffer.  This stops GDB
     trashing stubs that try to squeeze an extra NUL into what is
     already a full buffer (As of 1999-12-04 that was most stubs).  */
  this->remote_packet_size = 400 - 1;

  /* This one is filled in when a ``g'' packet is received.  */
  this->actual_register_packet_size = 0;

  /* Should rsa->sizeof_g_packet needs more space than the
     default, adjust the size accordingly.  Remember that each byte is
     encoded as two characters.  32 is the overhead for the packet
     header / footer.  NOTE: cagney/1999-10-26: I suspect that 8
     (``$NN:G...#NN'') is a better guess, the below has been padded a
     little.  */
  if (this->sizeof_g_packet > ((this->remote_packet_size - 32) / 2))
    this->remote_packet_size = (this->sizeof_g_packet * 2 + 32);
}

/* Get a pointer to the current remote target.  If not connected to a
   remote target, return NULL.  */

static remote_target *
get_current_remote_target ()
{
  target_ops *proc_target = current_inferior ()->process_target ();
  return dynamic_cast<remote_target *> (proc_target);
}

/* Return the current allowed size of a remote packet.  This is
   inferred from the current architecture, and should be used to
   limit the length of outgoing packets.  */
long
remote_target::get_remote_packet_size ()
{
  struct remote_state *rs = get_remote_state ();
  remote_arch_state *rsa
    = rs->get_remote_arch_state (current_inferior ()->arch ());

  if (rs->explicit_packet_size)
    return rs->explicit_packet_size;

  return rsa->remote_packet_size;
}

static struct packet_reg *
packet_reg_from_regnum (struct gdbarch *gdbarch, struct remote_arch_state *rsa,
			long regnum)
{
  if (regnum < 0 && regnum >= gdbarch_num_regs (gdbarch))
    return NULL;
  else
    {
      struct packet_reg *r = &rsa->regs[regnum];

      gdb_assert (r->regnum == regnum);
      return r;
    }
}

static struct packet_reg *
packet_reg_from_pnum (struct gdbarch *gdbarch, struct remote_arch_state *rsa,
		      LONGEST pnum)
{
  int i;

  for (i = 0; i < gdbarch_num_regs (gdbarch); i++)
    {
      struct packet_reg *r = &rsa->regs[i];

      if (r->pnum == pnum)
	return r;
    }
  return NULL;
}

/* Allow the user to specify what sequence to send to the remote
   when he requests a program interruption: Although ^C is usually
   what remote systems expect (this is the default, here), it is
   sometimes preferable to send a break.  On other systems such
   as the Linux kernel, a break followed by g, which is Magic SysRq g
   is required in order to interrupt the execution.  */
const char interrupt_sequence_control_c[] = "Ctrl-C";
const char interrupt_sequence_break[] = "BREAK";
const char interrupt_sequence_break_g[] = "BREAK-g";
static const char *const interrupt_sequence_modes[] =
  {
    interrupt_sequence_control_c,
    interrupt_sequence_break,
    interrupt_sequence_break_g,
    NULL
  };
static const char *interrupt_sequence_mode = interrupt_sequence_control_c;

static void
show_interrupt_sequence (struct ui_file *file, int from_tty,
			 struct cmd_list_element *c,
			 const char *value)
{
  if (interrupt_sequence_mode == interrupt_sequence_control_c)
    gdb_printf (file,
		_("Send the ASCII ETX character (Ctrl-c) "
		  "to the remote target to interrupt the "
		  "execution of the program.\n"));
  else if (interrupt_sequence_mode == interrupt_sequence_break)
    gdb_printf (file,
		_("send a break signal to the remote target "
		  "to interrupt the execution of the program.\n"));
  else if (interrupt_sequence_mode == interrupt_sequence_break_g)
    gdb_printf (file,
		_("Send a break signal and 'g' a.k.a. Magic SysRq g to "
		  "the remote target to interrupt the execution "
		  "of Linux kernel.\n"));
  else
    internal_error (_("Invalid value for interrupt_sequence_mode: %s."),
		    interrupt_sequence_mode);
}

/* This boolean variable specifies whether interrupt_sequence is sent
   to the remote target when gdb connects to it.
   This is mostly needed when you debug the Linux kernel: The Linux kernel
   expects BREAK g which is Magic SysRq g for connecting gdb.  */
static bool interrupt_on_connect = false;

/* This variable is used to implement the "set/show remotebreak" commands.
   Since these commands are now deprecated in favor of "set/show remote
   interrupt-sequence", it no longer has any effect on the code.  */
static bool remote_break;

static void
set_remotebreak (const char *args, int from_tty, struct cmd_list_element *c)
{
  if (remote_break)
    interrupt_sequence_mode = interrupt_sequence_break;
  else
    interrupt_sequence_mode = interrupt_sequence_control_c;
}

static void
show_remotebreak (struct ui_file *file, int from_tty,
		  struct cmd_list_element *c,
		  const char *value)
{
}

/* This variable sets the number of bits in an address that are to be
   sent in a memory ("M" or "m") packet.  Normally, after stripping
   leading zeros, the entire address would be sent.  This variable
   restricts the address to REMOTE_ADDRESS_SIZE bits.  HISTORY: The
   initial implementation of remote.c restricted the address sent in
   memory packets to ``host::sizeof long'' bytes - (typically 32
   bits).  Consequently, for 64 bit targets, the upper 32 bits of an
   address was never sent.  Since fixing this bug may cause a break in
   some remote targets this variable is principally provided to
   facilitate backward compatibility.  */

static unsigned int remote_address_size;


/* The default max memory-write-packet-size, when the setting is
   "fixed".  The 16k is historical.  (It came from older GDB's using
   alloca for buffers and the knowledge (folklore?) that some hosts
   don't cope very well with large alloca calls.)  */
#define DEFAULT_MAX_MEMORY_PACKET_SIZE_FIXED 16384

/* The minimum remote packet size for memory transfers.  Ensures we
   can write at least one byte.  */
#define MIN_MEMORY_PACKET_SIZE 20

/* Get the memory packet size, assuming it is fixed.  */

static long
get_fixed_memory_packet_size (struct memory_packet_config *config)
{
  gdb_assert (config->fixed_p);

  if (config->size <= 0)
    return DEFAULT_MAX_MEMORY_PACKET_SIZE_FIXED;
  else
    return config->size;
}

/* Compute the current size of a read/write packet.  Since this makes
   use of ``actual_register_packet_size'' the computation is dynamic.  */

long
remote_target::get_memory_packet_size (struct memory_packet_config *config)
{
  struct remote_state *rs = get_remote_state ();
  remote_arch_state *rsa
    = rs->get_remote_arch_state (current_inferior ()->arch ());

  long what_they_get;
  if (config->fixed_p)
    what_they_get = get_fixed_memory_packet_size (config);
  else
    {
      what_they_get = get_remote_packet_size ();
      /* Limit the packet to the size specified by the user.  */
      if (config->size > 0
	  && what_they_get > config->size)
	what_they_get = config->size;

      /* Limit it to the size of the targets ``g'' response unless we have
	 permission from the stub to use a larger packet size.  */
      if (rs->explicit_packet_size == 0
	  && rsa->actual_register_packet_size > 0
	  && what_they_get > rsa->actual_register_packet_size)
	what_they_get = rsa->actual_register_packet_size;
    }
  if (what_they_get < MIN_MEMORY_PACKET_SIZE)
    what_they_get = MIN_MEMORY_PACKET_SIZE;

  /* Make sure there is room in the global buffer for this packet
     (including its trailing NUL byte).  */
  if (rs->buf.size () < what_they_get + 1)
    rs->buf.resize (2 * what_they_get);

  return what_they_get;
}

/* Update the size of a read/write packet.  If they user wants
   something really big then do a sanity check.  */

static void
set_memory_packet_size (const char *args, struct memory_packet_config *config,
			bool target_connected)
{
  int fixed_p = config->fixed_p;
  long size = config->size;

  if (args == NULL)
    error (_("Argument required (integer, \"fixed\" or \"limit\")."));
  else if (strcmp (args, "hard") == 0
      || strcmp (args, "fixed") == 0)
    fixed_p = 1;
  else if (strcmp (args, "soft") == 0
	   || strcmp (args, "limit") == 0)
    fixed_p = 0;
  else
    {
      char *end;

      size = strtoul (args, &end, 0);
      if (args == end)
	error (_("Invalid %s (bad syntax)."), config->name);

      /* Instead of explicitly capping the size of a packet to or
	 disallowing it, the user is allowed to set the size to
	 something arbitrarily large.  */
    }

  /* Extra checks?  */
  if (fixed_p && !config->fixed_p)
    {
      /* So that the query shows the correct value.  */
      long query_size = (size <= 0
			 ? DEFAULT_MAX_MEMORY_PACKET_SIZE_FIXED
			 : size);

      if (target_connected
	  && !query (_("The target may not be able to correctly handle a %s\n"
		       "of %ld bytes.  Change the packet size? "),
		     config->name, query_size))
	error (_("Packet size not changed."));
      else if (!target_connected
	       && !query (_("Future remote targets may not be able to "
			    "correctly handle a %s\nof %ld bytes.  Change the "
			    "packet size for future remote targets? "),
			  config->name, query_size))
	error (_("Packet size not changed."));
    }
  /* Update the config.  */
  config->fixed_p = fixed_p;
  config->size = size;

  const char *target_type = get_target_type_name (target_connected);
  gdb_printf (_("The %s %s is set to \"%s\".\n"), config->name, target_type,
	      args);

}

/* Show the memory-read or write-packet size configuration CONFIG of the
   target REMOTE.  If REMOTE is nullptr, the default configuration for future
   remote targets should be passed in CONFIG.  */

static void
show_memory_packet_size (memory_packet_config *config, remote_target *remote)
{
  const char *target_type = get_target_type_name (remote != nullptr);

  if (config->size == 0)
    gdb_printf (_("The %s %s is 0 (default). "), config->name, target_type);
  else
    gdb_printf (_("The %s %s is %ld. "), config->name, target_type,
		config->size);

  if (config->fixed_p)
    gdb_printf (_("Packets are fixed at %ld bytes.\n"),
		get_fixed_memory_packet_size (config));
  else
    {
      if (remote != nullptr)
	gdb_printf (_("Packets are limited to %ld bytes.\n"),
		    remote->get_memory_packet_size (config));
      else
	gdb_puts ("The actual limit will be further reduced "
		  "dependent on the target.\n");
    }
}

/* Configure the memory-write-packet size of the currently selected target.  If
   no target is available, the default configuration for future remote targets
   is configured.  */

static void
set_memory_write_packet_size (const char *args, int from_tty)
{
  remote_target *remote = get_current_remote_target ();
  if (remote != nullptr)
    {
      set_memory_packet_size
	(args, &remote->m_features.m_memory_write_packet_config, true);
    }
  else
    {
      memory_packet_config* config = &memory_write_packet_config;
      set_memory_packet_size (args, config, false);
    }
}

/* Display the memory-write-packet size of the currently selected target.  If
   no target is available, the default configuration for future remote targets
   is shown.  */

static void
show_memory_write_packet_size (const char *args, int from_tty)
{
  remote_target *remote = get_current_remote_target ();
  if (remote != nullptr)
    show_memory_packet_size (&remote->m_features.m_memory_write_packet_config,
			     remote);
  else
    show_memory_packet_size (&memory_write_packet_config, nullptr);
}

/* Show the number of hardware watchpoints that can be used.  */

static void
show_hardware_watchpoint_limit (struct ui_file *file, int from_tty,
				struct cmd_list_element *c,
				const char *value)
{
  gdb_printf (file, _("The maximum number of target hardware "
		      "watchpoints is %s.\n"), value);
}

/* Show the length limit (in bytes) for hardware watchpoints.  */

static void
show_hardware_watchpoint_length_limit (struct ui_file *file, int from_tty,
				       struct cmd_list_element *c,
				       const char *value)
{
  gdb_printf (file, _("The maximum length (in bytes) of a target "
		      "hardware watchpoint is %s.\n"), value);
}

/* Show the number of hardware breakpoints that can be used.  */

static void
show_hardware_breakpoint_limit (struct ui_file *file, int from_tty,
				struct cmd_list_element *c,
				const char *value)
{
  gdb_printf (file, _("The maximum number of target hardware "
		      "breakpoints is %s.\n"), value);
}

/* Controls the maximum number of characters to display in the debug output
   for each remote packet.  The remaining characters are omitted.  */

static int remote_packet_max_chars = 512;

/* Show the maximum number of characters to display for each remote packet
   when remote debugging is enabled.  */

static void
show_remote_packet_max_chars (struct ui_file *file, int from_tty,
			      struct cmd_list_element *c,
			      const char *value)
{
  gdb_printf (file, _("Number of remote packet characters to "
		      "display is %s.\n"), value);
}

long
remote_target::get_memory_write_packet_size ()
{
  return get_memory_packet_size (&m_features.m_memory_write_packet_config);
}

/* Configure the memory-read-packet size of the currently selected target.  If
   no target is available, the default configuration for future remote targets
   is adapted.  */

static void
set_memory_read_packet_size (const char *args, int from_tty)
{
  remote_target *remote = get_current_remote_target ();
  if (remote != nullptr)
    set_memory_packet_size
      (args, &remote->m_features.m_memory_read_packet_config, true);
  else
    {
      memory_packet_config* config = &memory_read_packet_config;
      set_memory_packet_size (args, config, false);
    }

}

/* Display the memory-read-packet size of the currently selected target.  If
   no target is available, the default configuration for future remote targets
   is shown.  */

static void
show_memory_read_packet_size (const char *args, int from_tty)
{
  remote_target *remote = get_current_remote_target ();
  if (remote != nullptr)
    show_memory_packet_size (&remote->m_features.m_memory_read_packet_config,
			     remote);
  else
    show_memory_packet_size (&memory_read_packet_config, nullptr);
}

long
remote_target::get_memory_read_packet_size ()
{
  long size = get_memory_packet_size (&m_features.m_memory_read_packet_config);

  /* FIXME: cagney/1999-11-07: Functions like getpkt() need to get an
     extra buffer size argument before the memory read size can be
     increased beyond this.  */
  if (size > get_remote_packet_size ())
    size = get_remote_packet_size ();
  return size;
}

static enum packet_support packet_config_support (const packet_config *config);


static void
set_remote_protocol_packet_cmd (const char *args, int from_tty,
				cmd_list_element *c)
{
  remote_target *remote = get_current_remote_target ();
  gdb_assert (c->var.has_value ());

  auto *default_config = static_cast<packet_config *> (c->context ());
  const int packet_idx = std::distance (remote_protocol_packets,
					default_config);

  if (packet_idx >= 0 && packet_idx < PACKET_MAX)
    {
      const char *name = packets_descriptions[packet_idx].name;
      const auto_boolean value = c->var->get<auto_boolean> ();
      const char *support = get_packet_support_name (value);
      const char *target_type = get_target_type_name (remote != nullptr);

      if (remote != nullptr)
	remote->m_features.m_protocol_packets[packet_idx].detect = value;
      else
	remote_protocol_packets[packet_idx].detect = value;

      gdb_printf (_("Support for the '%s' packet %s is set to \"%s\".\n"), name,
		  target_type, support);
      return;
    }

  internal_error (_("Could not find config for %s"), c->name);
}

static void
show_packet_config_cmd (ui_file *file, const unsigned int which_packet,
			remote_target *remote)
{
  const char *support = "internal-error";
  const char *target_type = get_target_type_name (remote != nullptr);

  packet_config *config;
  if (remote != nullptr)
    config = &remote->m_features.m_protocol_packets[which_packet];
  else
    config = &remote_protocol_packets[which_packet];

  switch (packet_config_support (config))
    {
    case PACKET_ENABLE:
      support = "enabled";
      break;
    case PACKET_DISABLE:
      support = "disabled";
      break;
    case PACKET_SUPPORT_UNKNOWN:
      support = "unknown";
      break;
    }
  switch (config->detect)
    {
    case AUTO_BOOLEAN_AUTO:
      gdb_printf (file,
		  _("Support for the '%s' packet %s is \"auto\", "
		    "currently %s.\n"),
		  packets_descriptions[which_packet].name, target_type,
		  support);
      break;
    case AUTO_BOOLEAN_TRUE:
    case AUTO_BOOLEAN_FALSE:
      gdb_printf (file,
		  _("Support for the '%s' packet %s is \"%s\".\n"),
		  packets_descriptions[which_packet].name, target_type,
		  get_packet_support_name (config->detect));
      break;
    }
}

static void
add_packet_config_cmd (const unsigned int which_packet, const char *name,
		       const char *title, int legacy)
{
  packets_descriptions[which_packet].name = name;
  packets_descriptions[which_packet].title = title;

  packet_config *config = &remote_protocol_packets[which_packet];

  gdb::unique_xmalloc_ptr<char> set_doc
    = xstrprintf ("Set use of remote protocol `%s' (%s) packet.",
		  name, title);
  gdb::unique_xmalloc_ptr<char> show_doc
    = xstrprintf ("Show current use of remote protocol `%s' (%s) packet.",
		  name, title);
  /* set/show TITLE-packet {auto,on,off} */
  gdb::unique_xmalloc_ptr<char> cmd_name = xstrprintf ("%s-packet", title);
  set_show_commands cmds
    = add_setshow_auto_boolean_cmd (cmd_name.release (), class_obscure,
				    &config->detect, set_doc.get (),
				    show_doc.get (), NULL, /* help_doc */
				    set_remote_protocol_packet_cmd,
				    show_remote_protocol_packet_cmd,
				    &remote_set_cmdlist, &remote_show_cmdlist);
  cmds.show->set_context (config);
  cmds.set->set_context (config);

  /* set/show remote NAME-packet {auto,on,off} -- legacy.  */
  if (legacy)
    {
      /* It's not clear who should take ownership of the LEGACY_NAME string
	 created below, so, for now, place the string into a static vector
	 which ensures the strings is released when GDB exits.  */
      static std::vector<gdb::unique_xmalloc_ptr<char>> legacy_names;
      gdb::unique_xmalloc_ptr<char> legacy_name
	= xstrprintf ("%s-packet", name);
      add_alias_cmd (legacy_name.get (), cmds.set, class_obscure, 0,
		     &remote_set_cmdlist);
      add_alias_cmd (legacy_name.get (), cmds.show, class_obscure, 0,
		     &remote_show_cmdlist);
      legacy_names.emplace_back (std::move (legacy_name));
    }
}

static enum packet_result
packet_check_result (const char *buf)
{
  if (buf[0] != '\0')
    {
      /* The stub recognized the packet request.  Check that the
	 operation succeeded.  */
      if (buf[0] == 'E'
	  && isxdigit (buf[1]) && isxdigit (buf[2])
	  && buf[3] == '\0')
	/* "Enn"  - definitely an error.  */
	return PACKET_ERROR;

      /* Always treat "E." as an error.  This will be used for
	 more verbose error messages, such as E.memtypes.  */
      if (buf[0] == 'E' && buf[1] == '.')
	return PACKET_ERROR;

      /* The packet may or may not be OK.  Just assume it is.  */
      return PACKET_OK;
    }
  else
    /* The stub does not support the packet.  */
    return PACKET_UNKNOWN;
}

static enum packet_result
packet_check_result (const gdb::char_vector &buf)
{
  return packet_check_result (buf.data ());
}

packet_result
remote_features::packet_ok (const char *buf, const int which_packet)
{
  packet_config *config = &m_protocol_packets[which_packet];
  packet_description *descr = &packets_descriptions[which_packet];

  enum packet_result result;

  if (config->detect != AUTO_BOOLEAN_TRUE
      && config->support == PACKET_DISABLE)
    internal_error (_("packet_ok: attempt to use a disabled packet"));

  result = packet_check_result (buf);
  switch (result)
    {
    case PACKET_OK:
    case PACKET_ERROR:
      /* The stub recognized the packet request.  */
      if (config->support == PACKET_SUPPORT_UNKNOWN)
	{
	  remote_debug_printf ("Packet %s (%s) is supported",
			       descr->name, descr->title);
	  config->support = PACKET_ENABLE;
	}
      break;
    case PACKET_UNKNOWN:
      /* The stub does not support the packet.  */
      if (config->detect == AUTO_BOOLEAN_AUTO
	  && config->support == PACKET_ENABLE)
	{
	  /* If the stub previously indicated that the packet was
	     supported then there is a protocol error.  */
	  error (_("Protocol error: %s (%s) conflicting enabled responses."),
		 descr->name, descr->title);
	}
      else if (config->detect == AUTO_BOOLEAN_TRUE)
	{
	  /* The user set it wrong.  */
	  error (_("Enabled packet %s (%s) not recognized by stub"),
		 descr->name, descr->title);
	}

      remote_debug_printf ("Packet %s (%s) is NOT supported", descr->name,
			   descr->title);
      config->support = PACKET_DISABLE;
      break;
    }

  return result;
}

packet_result
remote_features::packet_ok (const gdb::char_vector &buf, const int which_packet)
{
  return packet_ok (buf.data (), which_packet);
}

/* Returns whether a given packet or feature is supported.  This takes
   into account the state of the corresponding "set remote foo-packet"
   command, which may be used to bypass auto-detection.  */

static enum packet_support
packet_config_support (const packet_config *config)
{
  switch (config->detect)
    {
    case AUTO_BOOLEAN_TRUE:
      return PACKET_ENABLE;
    case AUTO_BOOLEAN_FALSE:
      return PACKET_DISABLE;
    case AUTO_BOOLEAN_AUTO:
      return config->support;
    default:
      gdb_assert_not_reached ("bad switch");
    }
}

packet_support
remote_features::packet_support (int packet) const
{
  const packet_config *config = &m_protocol_packets[packet];
  return packet_config_support (config);
}

static void
show_remote_protocol_packet_cmd (struct ui_file *file, int from_tty,
				 struct cmd_list_element *c,
				 const char *value)
{
  remote_target *remote = get_current_remote_target ();
  gdb_assert (c->var.has_value ());

  auto *default_config = static_cast<packet_config *> (c->context ());
  const int packet_idx = std::distance (remote_protocol_packets,
					default_config);

  if (packet_idx >= 0 && packet_idx < PACKET_MAX)
    {
       show_packet_config_cmd (file, packet_idx, remote);
       return;
    }
  internal_error (_("Could not find config for %s"), c->name);
}

/* Should we try one of the 'Z' requests?  */

enum Z_packet_type
{
  Z_PACKET_SOFTWARE_BP,
  Z_PACKET_HARDWARE_BP,
  Z_PACKET_WRITE_WP,
  Z_PACKET_READ_WP,
  Z_PACKET_ACCESS_WP,
  NR_Z_PACKET_TYPES
};

/* For compatibility with older distributions.  Provide a ``set remote
   Z-packet ...'' command that updates all the Z packet types.  */

static enum auto_boolean remote_Z_packet_detect;

static void
set_remote_protocol_Z_packet_cmd (const char *args, int from_tty,
				  struct cmd_list_element *c)
{
  remote_target *remote = get_current_remote_target ();
  int i;

  for (i = 0; i < NR_Z_PACKET_TYPES; i++)
    {
      if (remote != nullptr)
	remote->m_features.m_protocol_packets[PACKET_Z0 + i].detect
	  = remote_Z_packet_detect;
      else
	remote_protocol_packets[PACKET_Z0 + i].detect = remote_Z_packet_detect;
    }

  const char *support = get_packet_support_name (remote_Z_packet_detect);
  const char *target_type = get_target_type_name (remote != nullptr);
  gdb_printf (_("Use of Z packets %s is set to \"%s\".\n"), target_type,
	      support);

}

static void
show_remote_protocol_Z_packet_cmd (struct ui_file *file, int from_tty,
				   struct cmd_list_element *c,
				   const char *value)
{
  remote_target *remote = get_current_remote_target ();
  int i;

  for (i = 0; i < NR_Z_PACKET_TYPES; i++)
    show_packet_config_cmd (file, PACKET_Z0 + i, remote);
}

/* Insert fork catchpoint target routine.  If fork events are enabled
   then return success, nothing more to do.  */

int
remote_target::insert_fork_catchpoint (int pid)
{
  return !m_features.remote_fork_event_p ();
}

/* Remove fork catchpoint target routine.  Nothing to do, just
   return success.  */

int
remote_target::remove_fork_catchpoint (int pid)
{
  return 0;
}

/* Insert vfork catchpoint target routine.  If vfork events are enabled
   then return success, nothing more to do.  */

int
remote_target::insert_vfork_catchpoint (int pid)
{
  return !m_features.remote_vfork_event_p ();
}

/* Remove vfork catchpoint target routine.  Nothing to do, just
   return success.  */

int
remote_target::remove_vfork_catchpoint (int pid)
{
  return 0;
}

/* Insert exec catchpoint target routine.  If exec events are
   enabled, just return success.  */

int
remote_target::insert_exec_catchpoint (int pid)
{
  return !m_features.remote_exec_event_p ();
}

/* Remove exec catchpoint target routine.  Nothing to do, just
   return success.  */

int
remote_target::remove_exec_catchpoint (int pid)
{
  return 0;
}



/* Take advantage of the fact that the TID field is not used, to tag
   special ptids with it set to != 0.  */
static const ptid_t magic_null_ptid (42000, -1, 1);
static const ptid_t not_sent_ptid (42000, -2, 1);
static const ptid_t any_thread_ptid (42000, 0, 1);

/* Find out if the stub attached to PID (and hence GDB should offer to
   detach instead of killing it when bailing out).  */

int
remote_target::remote_query_attached (int pid)
{
  struct remote_state *rs = get_remote_state ();
  size_t size = get_remote_packet_size ();

  if (m_features.packet_support (PACKET_qAttached) == PACKET_DISABLE)
    return 0;

  if (m_features.remote_multi_process_p ())
    xsnprintf (rs->buf.data (), size, "qAttached:%x", pid);
  else
    xsnprintf (rs->buf.data (), size, "qAttached");

  putpkt (rs->buf);
  getpkt (&rs->buf);

  switch (m_features.packet_ok (rs->buf, PACKET_qAttached))
    {
    case PACKET_OK:
      if (strcmp (rs->buf.data (), "1") == 0)
	return 1;
      break;
    case PACKET_ERROR:
      warning (_("Remote failure reply: %s"), rs->buf.data ());
      break;
    case PACKET_UNKNOWN:
      break;
    }

  return 0;
}

/* Add PID to GDB's inferior table.  If FAKE_PID_P is true, then PID
   has been invented by GDB, instead of reported by the target.  Since
   we can be connected to a remote system before before knowing about
   any inferior, mark the target with execution when we find the first
   inferior.  If ATTACHED is 1, then we had just attached to this
   inferior.  If it is 0, then we just created this inferior.  If it
   is -1, then try querying the remote stub to find out if it had
   attached to the inferior or not.  If TRY_OPEN_EXEC is true then
   attempt to open this inferior's executable as the main executable
   if no main executable is open already.  */

inferior *
remote_target::remote_add_inferior (bool fake_pid_p, int pid, int attached,
				    int try_open_exec)
{
  struct inferior *inf;

  /* Check whether this process we're learning about is to be
     considered attached, or if is to be considered to have been
     spawned by the stub.  */
  if (attached == -1)
    attached = remote_query_attached (pid);

  if (gdbarch_has_global_solist (current_inferior ()->arch ()))
    {
      /* If the target shares code across all inferiors, then every
	 attach adds a new inferior.  */
      inf = add_inferior (pid);

      /* ... and every inferior is bound to the same program space.
	 However, each inferior may still have its own address
	 space.  */
      inf->aspace = maybe_new_address_space ();
      inf->pspace = current_program_space;
    }
  else
    {
      /* In the traditional debugging scenario, there's a 1-1 match
	 between program/address spaces.  We simply bind the inferior
	 to the program space's address space.  */
      inf = current_inferior ();

      /* However, if the current inferior is already bound to a
	 process, find some other empty inferior.  */
      if (inf->pid != 0)
	{
	  inf = nullptr;
	  for (inferior *it : all_inferiors ())
	    if (it->pid == 0)
	      {
		inf = it;
		break;
	      }
	}
      if (inf == nullptr)
	{
	  /* Since all inferiors were already bound to a process, add
	     a new inferior.  */
	  inf = add_inferior_with_spaces ();
	}
      switch_to_inferior_no_thread (inf);
      inf->push_target (this);
      inferior_appeared (inf, pid);
    }

  inf->attach_flag = attached;
  inf->fake_pid_p = fake_pid_p;

  /* If no main executable is currently open then attempt to
     open the file that was executed to create this inferior.  */
  if (try_open_exec && get_exec_file (0) == NULL)
    exec_file_locate_attach (pid, 0, 1);

  /* Check for exec file mismatch, and let the user solve it.  */
  validate_exec_file (1);

  return inf;
}

static remote_thread_info *get_remote_thread_info (thread_info *thread);
static remote_thread_info *get_remote_thread_info (remote_target *target,
						   ptid_t ptid);

/* Add thread PTID to GDB's thread list.  Tag it as executing/running
   according to EXECUTING and RUNNING respectively.  If SILENT_P (or the
   remote_state::starting_up flag) is true then the new thread is added
   silently, otherwise the new thread will be announced to the user.  */

thread_info *
remote_target::remote_add_thread (ptid_t ptid, bool running, bool executing,
				  bool silent_p)
{
  struct remote_state *rs = get_remote_state ();
  struct thread_info *thread;

  /* GDB historically didn't pull threads in the initial connection
     setup.  If the remote target doesn't even have a concept of
     threads (e.g., a bare-metal target), even if internally we
     consider that a single-threaded target, mentioning a new thread
     might be confusing to the user.  Be silent then, preserving the
     age old behavior.  */
  if (rs->starting_up || silent_p)
    thread = add_thread_silent (this, ptid);
  else
    thread = add_thread (this, ptid);

  if (executing)
    get_remote_thread_info (thread)->set_resumed ();
  set_executing (this, ptid, executing);
  set_running (this, ptid, running);

  return thread;
}

/* Come here when we learn about a thread id from the remote target.
   It may be the first time we hear about such thread, so take the
   opportunity to add it to GDB's thread list.  In case this is the
   first time we're noticing its corresponding inferior, add it to
   GDB's inferior list as well.  EXECUTING indicates whether the
   thread is (internally) executing or stopped.  */

void
remote_target::remote_notice_new_inferior (ptid_t currthread, bool executing)
{
  /* In non-stop mode, we assume new found threads are (externally)
     running until proven otherwise with a stop reply.  In all-stop,
     we can only get here if all threads are stopped.  */
  bool running = target_is_non_stop_p ();

  /* If this is a new thread, add it to GDB's thread list.
     If we leave it up to WFI to do this, bad things will happen.  */

  thread_info *tp = this->find_thread (currthread);
  if (tp != NULL && tp->state == THREAD_EXITED)
    {
      /* We're seeing an event on a thread id we knew had exited.
	 This has to be a new thread reusing the old id.  Add it.  */
      remote_add_thread (currthread, running, executing, false);
      return;
    }

  if (!in_thread_list (this, currthread))
    {
      struct inferior *inf = NULL;
      int pid = currthread.pid ();

      if (inferior_ptid.is_pid ()
	  && pid == inferior_ptid.pid ())
	{
	  /* inferior_ptid has no thread member yet.  This can happen
	     with the vAttach -> remote_wait,"TAAthread:" path if the
	     stub doesn't support qC.  This is the first stop reported
	     after an attach, so this is the main thread.  Update the
	     ptid in the thread list.  */
	  if (in_thread_list (this, ptid_t (pid)))
	    thread_change_ptid (this, inferior_ptid, currthread);
	  else
	    {
	      thread_info *thr
		= remote_add_thread (currthread, running, executing, false);
	      switch_to_thread (thr);
	    }
	  return;
	}

      if (magic_null_ptid == inferior_ptid)
	{
	  /* inferior_ptid is not set yet.  This can happen with the
	     vRun -> remote_wait,"TAAthread:" path if the stub
	     doesn't support qC.  This is the first stop reported
	     after an attach, so this is the main thread.  Update the
	     ptid in the thread list.  */
	  thread_change_ptid (this, inferior_ptid, currthread);
	  return;
	}

      /* When connecting to a target remote, or to a target
	 extended-remote which already was debugging an inferior, we
	 may not know about it yet.  Add it before adding its child
	 thread, so notifications are emitted in a sensible order.  */
      if (find_inferior_pid (this, currthread.pid ()) == NULL)
	{
	  bool fake_pid_p = !m_features.remote_multi_process_p ();

	  inf = remote_add_inferior (fake_pid_p,
				     currthread.pid (), -1, 1);
	}

      /* This is really a new thread.  Add it.  */
      thread_info *new_thr
	= remote_add_thread (currthread, running, executing, false);

      /* If we found a new inferior, let the common code do whatever
	 it needs to with it (e.g., read shared libraries, insert
	 breakpoints), unless we're just setting up an all-stop
	 connection.  */
      if (inf != NULL)
	{
	  struct remote_state *rs = get_remote_state ();

	  if (!rs->starting_up)
	    notice_new_inferior (new_thr, executing, 0);
	}
    }
}

/* Return THREAD's private thread data, creating it if necessary.  */

static remote_thread_info *
get_remote_thread_info (thread_info *thread)
{
  gdb_assert (thread != NULL);

  if (thread->priv == NULL)
    thread->priv.reset (new remote_thread_info);

  return gdb::checked_static_cast<remote_thread_info *> (thread->priv.get ());
}

/* Return PTID's private thread data, creating it if necessary.  */

static remote_thread_info *
get_remote_thread_info (remote_target *target, ptid_t ptid)
{
  thread_info *thr = target->find_thread (ptid);
  return get_remote_thread_info (thr);
}

/* Call this function as a result of
   1) A halt indication (T packet) containing a thread id
   2) A direct query of currthread
   3) Successful execution of set thread */

static void
record_currthread (struct remote_state *rs, ptid_t currthread)
{
  rs->general_thread = currthread;
}

/* If 'QPassSignals' is supported, tell the remote stub what signals
   it can simply pass through to the inferior without reporting.  */

void
remote_target::pass_signals (gdb::array_view<const unsigned char> pass_signals)
{
  if (m_features.packet_support (PACKET_QPassSignals) != PACKET_DISABLE)
    {
      char *pass_packet, *p;
      int count = 0;
      struct remote_state *rs = get_remote_state ();

      gdb_assert (pass_signals.size () < 256);
      for (size_t i = 0; i < pass_signals.size (); i++)
	{
	  if (pass_signals[i])
	    count++;
	}
      pass_packet = (char *) xmalloc (count * 3 + strlen ("QPassSignals:") + 1);
      strcpy (pass_packet, "QPassSignals:");
      p = pass_packet + strlen (pass_packet);
      for (size_t i = 0; i < pass_signals.size (); i++)
	{
	  if (pass_signals[i])
	    {
	      if (i >= 16)
		*p++ = tohex (i >> 4);
	      *p++ = tohex (i & 15);
	      if (count)
		*p++ = ';';
	      else
		break;
	      count--;
	    }
	}
      *p = 0;
      if (!rs->last_pass_packet || strcmp (rs->last_pass_packet, pass_packet))
	{
	  putpkt (pass_packet);
	  getpkt (&rs->buf);
	  m_features.packet_ok (rs->buf, PACKET_QPassSignals);
	  xfree (rs->last_pass_packet);
	  rs->last_pass_packet = pass_packet;
	}
      else
	xfree (pass_packet);
    }
}

/* If 'QCatchSyscalls' is supported, tell the remote stub
   to report syscalls to GDB.  */

int
remote_target::set_syscall_catchpoint (int pid, bool needed, int any_count,
				       gdb::array_view<const int> syscall_counts)
{
  const char *catch_packet;
  enum packet_result result;
  int n_sysno = 0;

  if (m_features.packet_support (PACKET_QCatchSyscalls) == PACKET_DISABLE)
    {
      /* Not supported.  */
      return 1;
    }

  if (needed && any_count == 0)
    {
      /* Count how many syscalls are to be caught.  */
      for (size_t i = 0; i < syscall_counts.size (); i++)
	{
	  if (syscall_counts[i] != 0)
	    n_sysno++;
	}
    }

  remote_debug_printf ("pid %d needed %d any_count %d n_sysno %d",
		       pid, needed, any_count, n_sysno);

  std::string built_packet;
  if (needed)
    {
      /* Prepare a packet with the sysno list, assuming max 8+1
	 characters for a sysno.  If the resulting packet size is too
	 big, fallback on the non-selective packet.  */
      const int maxpktsz = strlen ("QCatchSyscalls:1") + n_sysno * 9 + 1;
      built_packet.reserve (maxpktsz);
      built_packet = "QCatchSyscalls:1";
      if (any_count == 0)
	{
	  /* Add in each syscall to be caught.  */
	  for (size_t i = 0; i < syscall_counts.size (); i++)
	    {
	      if (syscall_counts[i] != 0)
		string_appendf (built_packet, ";%zx", i);
	    }
	}
      if (built_packet.size () > get_remote_packet_size ())
	{
	  /* catch_packet too big.  Fallback to less efficient
	     non selective mode, with GDB doing the filtering.  */
	  catch_packet = "QCatchSyscalls:1";
	}
      else
	catch_packet = built_packet.c_str ();
    }
  else
    catch_packet = "QCatchSyscalls:0";

  struct remote_state *rs = get_remote_state ();

  putpkt (catch_packet);
  getpkt (&rs->buf);
  result = m_features.packet_ok (rs->buf, PACKET_QCatchSyscalls);
  if (result == PACKET_OK)
    return 0;
  else
    return -1;
}

/* If 'QProgramSignals' is supported, tell the remote stub what
   signals it should pass through to the inferior when detaching.  */

void
remote_target::program_signals (gdb::array_view<const unsigned char> signals)
{
  if (m_features.packet_support (PACKET_QProgramSignals) != PACKET_DISABLE)
    {
      char *packet, *p;
      int count = 0;
      struct remote_state *rs = get_remote_state ();

      gdb_assert (signals.size () < 256);
      for (size_t i = 0; i < signals.size (); i++)
	{
	  if (signals[i])
	    count++;
	}
      packet = (char *) xmalloc (count * 3 + strlen ("QProgramSignals:") + 1);
      strcpy (packet, "QProgramSignals:");
      p = packet + strlen (packet);
      for (size_t i = 0; i < signals.size (); i++)
	{
	  if (signal_pass_state (i))
	    {
	      if (i >= 16)
		*p++ = tohex (i >> 4);
	      *p++ = tohex (i & 15);
	      if (count)
		*p++ = ';';
	      else
		break;
	      count--;
	    }
	}
      *p = 0;
      if (!rs->last_program_signals_packet
	  || strcmp (rs->last_program_signals_packet, packet) != 0)
	{
	  putpkt (packet);
	  getpkt (&rs->buf);
	  m_features.packet_ok (rs->buf, PACKET_QProgramSignals);
	  xfree (rs->last_program_signals_packet);
	  rs->last_program_signals_packet = packet;
	}
      else
	xfree (packet);
    }
}

/* If PTID is MAGIC_NULL_PTID, don't set any thread.  If PTID is
   MINUS_ONE_PTID, set the thread to -1, so the stub returns the
   thread.  If GEN is set, set the general thread, if not, then set
   the step/continue thread.  */
void
remote_target::set_thread (ptid_t ptid, int gen)
{
  struct remote_state *rs = get_remote_state ();
  ptid_t state = gen ? rs->general_thread : rs->continue_thread;
  char *buf = rs->buf.data ();
  char *endbuf = buf + get_remote_packet_size ();

  if (state == ptid)
    return;

  *buf++ = 'H';
  *buf++ = gen ? 'g' : 'c';
  if (ptid == magic_null_ptid)
    xsnprintf (buf, endbuf - buf, "0");
  else if (ptid == any_thread_ptid)
    xsnprintf (buf, endbuf - buf, "0");
  else if (ptid == minus_one_ptid)
    xsnprintf (buf, endbuf - buf, "-1");
  else
    write_ptid (buf, endbuf, ptid);
  putpkt (rs->buf);
  getpkt (&rs->buf);
  if (gen)
    rs->general_thread = ptid;
  else
    rs->continue_thread = ptid;
}

void
remote_target::set_general_thread (ptid_t ptid)
{
  set_thread (ptid, 1);
}

void
remote_target::set_continue_thread (ptid_t ptid)
{
  set_thread (ptid, 0);
}

/* Change the remote current process.  Which thread within the process
   ends up selected isn't important, as long as it is the same process
   as what INFERIOR_PTID points to.

   This comes from that fact that there is no explicit notion of
   "selected process" in the protocol.  The selected process for
   general operations is the process the selected general thread
   belongs to.  */

void
remote_target::set_general_process ()
{
  /* If the remote can't handle multiple processes, don't bother.  */
  if (!m_features.remote_multi_process_p ())
    return;

  remote_state *rs = get_remote_state ();

  /* We only need to change the remote current thread if it's pointing
     at some other process.  */
  if (rs->general_thread.pid () != inferior_ptid.pid ())
    set_general_thread (inferior_ptid);
}


/* Return nonzero if this is the main thread that we made up ourselves
   to model non-threaded targets as single-threaded.  */

static int
remote_thread_always_alive (ptid_t ptid)
{
  if (ptid == magic_null_ptid)
    /* The main thread is always alive.  */
    return 1;

  if (ptid.pid () != 0 && ptid.lwp () == 0)
    /* The main thread is always alive.  This can happen after a
       vAttach, if the remote side doesn't support
       multi-threading.  */
    return 1;

  return 0;
}

/* Return nonzero if the thread PTID is still alive on the remote
   system.  */

bool
remote_target::thread_alive (ptid_t ptid)
{
  struct remote_state *rs = get_remote_state ();
  char *p, *endp;

  /* Check if this is a thread that we made up ourselves to model
     non-threaded targets as single-threaded.  */
  if (remote_thread_always_alive (ptid))
    return 1;

  p = rs->buf.data ();
  endp = p + get_remote_packet_size ();

  *p++ = 'T';
  write_ptid (p, endp, ptid);

  putpkt (rs->buf);
  getpkt (&rs->buf);
  return (rs->buf[0] == 'O' && rs->buf[1] == 'K');
}

/* Return a pointer to a thread name if we know it and NULL otherwise.
   The thread_info object owns the memory for the name.  */

const char *
remote_target::thread_name (struct thread_info *info)
{
  if (info->priv != NULL)
    {
      const std::string &name = get_remote_thread_info (info)->name;
      return !name.empty () ? name.c_str () : NULL;
    }

  return NULL;
}

/* About these extended threadlist and threadinfo packets.  They are
   variable length packets but, the fields within them are often fixed
   length.  They are redundant enough to send over UDP as is the
   remote protocol in general.  There is a matching unit test module
   in libstub.  */

/* WARNING: This threadref data structure comes from the remote O.S.,
   libstub protocol encoding, and remote.c.  It is not particularly
   changeable.  */

/* Right now, the internal structure is int. We want it to be bigger.
   Plan to fix this.  */

typedef int gdb_threadref;	/* Internal GDB thread reference.  */

/* gdb_ext_thread_info is an internal GDB data structure which is
   equivalent to the reply of the remote threadinfo packet.  */

struct gdb_ext_thread_info
  {
    threadref threadid;		/* External form of thread reference.  */
    int active;			/* Has state interesting to GDB?
				   regs, stack.  */
    char display[256];		/* Brief state display, name,
				   blocked/suspended.  */
    char shortname[32];		/* To be used to name threads.  */
    char more_display[256];	/* Long info, statistics, queue depth,
				   whatever.  */
  };

/* The volume of remote transfers can be limited by submitting
   a mask containing bits specifying the desired information.
   Use a union of these values as the 'selection' parameter to
   get_thread_info.  FIXME: Make these TAG names more thread specific.  */

#define TAG_THREADID 1
#define TAG_EXISTS 2
#define TAG_DISPLAY 4
#define TAG_THREADNAME 8
#define TAG_MOREDISPLAY 16

#define BUF_THREAD_ID_SIZE (OPAQUETHREADBYTES * 2)

static const char *unpack_nibble (const char *buf, int *val);

static const char *unpack_byte (const char *buf, int *value);

static char *pack_int (char *buf, int value);

static const char *unpack_int (const char *buf, int *value);

static const char *unpack_string (const char *src, char *dest, int length);

static char *pack_threadid (char *pkt, threadref *id);

static const char *unpack_threadid (const char *inbuf, threadref *id);

void int_to_threadref (threadref *id, int value);

static int threadref_to_int (threadref *ref);

static void copy_threadref (threadref *dest, threadref *src);

static int threadmatch (threadref *dest, threadref *src);

static char *pack_threadinfo_request (char *pkt, int mode,
				      threadref *id);

static char *pack_threadlist_request (char *pkt, int startflag,
				      int threadcount,
				      threadref *nextthread);

static int remote_newthread_step (threadref *ref, void *context);


/* Write a PTID to BUF.  ENDBUF points to one-passed-the-end of the
   buffer we're allowed to write to.  Returns
   BUF+CHARACTERS_WRITTEN.  */

char *
remote_target::write_ptid (char *buf, const char *endbuf, ptid_t ptid)
{
  int pid, tid;

  if (m_features.remote_multi_process_p ())
    {
      pid = ptid.pid ();
      if (pid < 0)
	buf += xsnprintf (buf, endbuf - buf, "p-%x.", -pid);
      else
	buf += xsnprintf (buf, endbuf - buf, "p%x.", pid);
    }
  tid = ptid.lwp ();
  if (tid < 0)
    buf += xsnprintf (buf, endbuf - buf, "-%x", -tid);
  else
    buf += xsnprintf (buf, endbuf - buf, "%x", tid);

  return buf;
}

/* Extract a PTID from BUF.  If non-null, OBUF is set to one past the
   last parsed char.  Returns null_ptid if no thread id is found, and
   throws an error if the thread id has an invalid format.  */

static ptid_t
read_ptid (const char *buf, const char **obuf)
{
  const char *p = buf;
  const char *pp;
  ULONGEST pid = 0, tid = 0;

  if (*p == 'p')
    {
      /* Multi-process ptid.  */
      pp = unpack_varlen_hex (p + 1, &pid);
      if (*pp != '.')
	error (_("invalid remote ptid: %s"), p);

      p = pp;
      pp = unpack_varlen_hex (p + 1, &tid);
      if (obuf)
	*obuf = pp;
      return ptid_t (pid, tid);
    }

  /* No multi-process.  Just a tid.  */
  pp = unpack_varlen_hex (p, &tid);

  /* Return null_ptid when no thread id is found.  */
  if (p == pp)
    {
      if (obuf)
	*obuf = pp;
      return null_ptid;
    }

  /* Since the stub is not sending a process id, default to what's
     current_inferior, unless it doesn't have a PID yet.  If so,
     then since there's no way to know the pid of the reported
     threads, use the magic number.  */
  inferior *inf = current_inferior ();
  if (inf->pid == 0)
    pid = magic_null_ptid.pid ();
  else
    pid = inf->pid;

  if (obuf)
    *obuf = pp;
  return ptid_t (pid, tid);
}

static int
stubhex (int ch)
{
  if (ch >= 'a' && ch <= 'f')
    return ch - 'a' + 10;
  if (ch >= '0' && ch <= '9')
    return ch - '0';
  if (ch >= 'A' && ch <= 'F')
    return ch - 'A' + 10;
  return -1;
}

static int
stub_unpack_int (const char *buff, int fieldlength)
{
  int nibble;
  int retval = 0;

  while (fieldlength)
    {
      nibble = stubhex (*buff++);
      retval |= nibble;
      fieldlength--;
      if (fieldlength)
	retval = retval << 4;
    }
  return retval;
}

static const char *
unpack_nibble (const char *buf, int *val)
{
  *val = fromhex (*buf++);
  return buf;
}

static const char *
unpack_byte (const char *buf, int *value)
{
  *value = stub_unpack_int (buf, 2);
  return buf + 2;
}

static char *
pack_int (char *buf, int value)
{
  buf = pack_hex_byte (buf, (value >> 24) & 0xff);
  buf = pack_hex_byte (buf, (value >> 16) & 0xff);
  buf = pack_hex_byte (buf, (value >> 8) & 0x0ff);
  buf = pack_hex_byte (buf, (value & 0xff));
  return buf;
}

static const char *
unpack_int (const char *buf, int *value)
{
  *value = stub_unpack_int (buf, 8);
  return buf + 8;
}

#if 0			/* Currently unused, uncomment when needed.  */
static char *pack_string (char *pkt, char *string);

static char *
pack_string (char *pkt, char *string)
{
  char ch;
  int len;

  len = strlen (string);
  if (len > 200)
    len = 200;		/* Bigger than most GDB packets, junk???  */
  pkt = pack_hex_byte (pkt, len);
  while (len-- > 0)
    {
      ch = *string++;
      if ((ch == '\0') || (ch == '#'))
	ch = '*';		/* Protect encapsulation.  */
      *pkt++ = ch;
    }
  return pkt;
}
#endif /* 0 (unused) */

static const char *
unpack_string (const char *src, char *dest, int length)
{
  while (length--)
    *dest++ = *src++;
  *dest = '\0';
  return src;
}

static char *
pack_threadid (char *pkt, threadref *id)
{
  char *limit;
  unsigned char *altid;

  altid = (unsigned char *) id;
  limit = pkt + BUF_THREAD_ID_SIZE;
  while (pkt < limit)
    pkt = pack_hex_byte (pkt, *altid++);
  return pkt;
}


static const char *
unpack_threadid (const char *inbuf, threadref *id)
{
  char *altref;
  const char *limit = inbuf + BUF_THREAD_ID_SIZE;
  int x, y;

  altref = (char *) id;

  while (inbuf < limit)
    {
      x = stubhex (*inbuf++);
      y = stubhex (*inbuf++);
      *altref++ = (x << 4) | y;
    }
  return inbuf;
}

/* Externally, threadrefs are 64 bits but internally, they are still
   ints.  This is due to a mismatch of specifications.  We would like
   to use 64bit thread references internally.  This is an adapter
   function.  */

void
int_to_threadref (threadref *id, int value)
{
  unsigned char *scan;

  scan = (unsigned char *) id;
  {
    int i = 4;
    while (i--)
      *scan++ = 0;
  }
  *scan++ = (value >> 24) & 0xff;
  *scan++ = (value >> 16) & 0xff;
  *scan++ = (value >> 8) & 0xff;
  *scan++ = (value & 0xff);
}

static int
threadref_to_int (threadref *ref)
{
  int i, value = 0;
  unsigned char *scan;

  scan = *ref;
  scan += 4;
  i = 4;
  while (i-- > 0)
    value = (value << 8) | ((*scan++) & 0xff);
  return value;
}

static void
copy_threadref (threadref *dest, threadref *src)
{
  int i;
  unsigned char *csrc, *cdest;

  csrc = (unsigned char *) src;
  cdest = (unsigned char *) dest;
  i = 8;
  while (i--)
    *cdest++ = *csrc++;
}

static int
threadmatch (threadref *dest, threadref *src)
{
  /* Things are broken right now, so just assume we got a match.  */
#if 0
  unsigned char *srcp, *destp;
  int i, result;
  srcp = (char *) src;
  destp = (char *) dest;

  result = 1;
  while (i-- > 0)
    result &= (*srcp++ == *destp++) ? 1 : 0;
  return result;
#endif
  return 1;
}

/*
   threadid:1,        # always request threadid
   context_exists:2,
   display:4,
   unique_name:8,
   more_display:16
 */

/* Encoding:  'Q':8,'P':8,mask:32,threadid:64 */

static char *
pack_threadinfo_request (char *pkt, int mode, threadref *id)
{
  *pkt++ = 'q';				/* Info Query */
  *pkt++ = 'P';				/* process or thread info */
  pkt = pack_int (pkt, mode);		/* mode */
  pkt = pack_threadid (pkt, id);	/* threadid */
  *pkt = '\0';				/* terminate */
  return pkt;
}

/* These values tag the fields in a thread info response packet.  */
/* Tagging the fields allows us to request specific fields and to
   add more fields as time goes by.  */

#define TAG_THREADID 1		/* Echo the thread identifier.  */
#define TAG_EXISTS 2		/* Is this process defined enough to
				   fetch registers and its stack?  */
#define TAG_DISPLAY 4		/* A short thing maybe to put on a window */
#define TAG_THREADNAME 8	/* string, maps 1-to-1 with a thread is.  */
#define TAG_MOREDISPLAY 16	/* Whatever the kernel wants to say about
				   the process.  */

int
remote_target::remote_unpack_thread_info_response (const char *pkt,
						   threadref *expectedref,
						   gdb_ext_thread_info *info)
{
  struct remote_state *rs = get_remote_state ();
  int mask, length;
  int tag;
  threadref ref;
  const char *limit = pkt + rs->buf.size (); /* Plausible parsing limit.  */
  int retval = 1;

  /* info->threadid = 0; FIXME: implement zero_threadref.  */
  info->active = 0;
  info->display[0] = '\0';
  info->shortname[0] = '\0';
  info->more_display[0] = '\0';

  /* Assume the characters indicating the packet type have been
     stripped.  */
  pkt = unpack_int (pkt, &mask);	/* arg mask */
  pkt = unpack_threadid (pkt, &ref);

  if (mask == 0)
    warning (_("Incomplete response to threadinfo request."));
  if (!threadmatch (&ref, expectedref))
    {			/* This is an answer to a different request.  */
      warning (_("ERROR RMT Thread info mismatch."));
      return 0;
    }
  copy_threadref (&info->threadid, &ref);

  /* Loop on tagged fields , try to bail if something goes wrong.  */

  /* Packets are terminated with nulls.  */
  while ((pkt < limit) && mask && *pkt)
    {
      pkt = unpack_int (pkt, &tag);	/* tag */
      pkt = unpack_byte (pkt, &length);	/* length */
      if (!(tag & mask))		/* Tags out of synch with mask.  */
	{
	  warning (_("ERROR RMT: threadinfo tag mismatch."));
	  retval = 0;
	  break;
	}
      if (tag == TAG_THREADID)
	{
	  if (length != 16)
	    {
	      warning (_("ERROR RMT: length of threadid is not 16."));
	      retval = 0;
	      break;
	    }
	  pkt = unpack_threadid (pkt, &ref);
	  mask = mask & ~TAG_THREADID;
	  continue;
	}
      if (tag == TAG_EXISTS)
	{
	  info->active = stub_unpack_int (pkt, length);
	  pkt += length;
	  mask = mask & ~(TAG_EXISTS);
	  if (length > 8)
	    {
	      warning (_("ERROR RMT: 'exists' length too long."));
	      retval = 0;
	      break;
	    }
	  continue;
	}
      if (tag == TAG_THREADNAME)
	{
	  pkt = unpack_string (pkt, &info->shortname[0], length);
	  mask = mask & ~TAG_THREADNAME;
	  continue;
	}
      if (tag == TAG_DISPLAY)
	{
	  pkt = unpack_string (pkt, &info->display[0], length);
	  mask = mask & ~TAG_DISPLAY;
	  continue;
	}
      if (tag == TAG_MOREDISPLAY)
	{
	  pkt = unpack_string (pkt, &info->more_display[0], length);
	  mask = mask & ~TAG_MOREDISPLAY;
	  continue;
	}
      warning (_("ERROR RMT: unknown thread info tag."));
      break;			/* Not a tag we know about.  */
    }
  return retval;
}

int
remote_target::remote_get_threadinfo (threadref *threadid,
				      int fieldset,
				      gdb_ext_thread_info *info)
{
  struct remote_state *rs = get_remote_state ();
  int result;

  pack_threadinfo_request (rs->buf.data (), fieldset, threadid);
  putpkt (rs->buf);
  getpkt (&rs->buf);

  if (rs->buf[0] == '\0')
    return 0;

  result = remote_unpack_thread_info_response (&rs->buf[2],
					       threadid, info);
  return result;
}

/*    Format: i'Q':8,i"L":8,initflag:8,batchsize:16,lastthreadid:32   */

static char *
pack_threadlist_request (char *pkt, int startflag, int threadcount,
			 threadref *nextthread)
{
  *pkt++ = 'q';			/* info query packet */
  *pkt++ = 'L';			/* Process LIST or threadLIST request */
  pkt = pack_nibble (pkt, startflag);		/* initflag 1 bytes */
  pkt = pack_hex_byte (pkt, threadcount);	/* threadcount 2 bytes */
  pkt = pack_threadid (pkt, nextthread);	/* 64 bit thread identifier */
  *pkt = '\0';
  return pkt;
}

/* Encoding:   'q':8,'M':8,count:16,done:8,argthreadid:64,(threadid:64)* */

int
remote_target::parse_threadlist_response (const char *pkt, int result_limit,
					  threadref *original_echo,
					  threadref *resultlist,
					  int *doneflag)
{
  struct remote_state *rs = get_remote_state ();
  int count, resultcount, done;

  resultcount = 0;
  /* Assume the 'q' and 'M chars have been stripped.  */
  const char *limit = pkt + (rs->buf.size () - BUF_THREAD_ID_SIZE);
  /* done parse past here */
  pkt = unpack_byte (pkt, &count);	/* count field */
  pkt = unpack_nibble (pkt, &done);
  /* The first threadid is the argument threadid.  */
  pkt = unpack_threadid (pkt, original_echo);	/* should match query packet */
  while ((count-- > 0) && (pkt < limit))
    {
      pkt = unpack_threadid (pkt, resultlist++);
      if (resultcount++ >= result_limit)
	break;
    }
  if (doneflag)
    *doneflag = done;
  return resultcount;
}

/* Fetch the next batch of threads from the remote.  Returns -1 if the
   qL packet is not supported, 0 on error and 1 on success.  */

int
remote_target::remote_get_threadlist (int startflag, threadref *nextthread,
				      int result_limit, int *done, int *result_count,
				      threadref *threadlist)
{
  struct remote_state *rs = get_remote_state ();
  int result = 1;

  /* Truncate result limit to be smaller than the packet size.  */
  if ((((result_limit + 1) * BUF_THREAD_ID_SIZE) + 10)
      >= get_remote_packet_size ())
    result_limit = (get_remote_packet_size () / BUF_THREAD_ID_SIZE) - 2;

  pack_threadlist_request (rs->buf.data (), startflag, result_limit,
			   nextthread);
  putpkt (rs->buf);
  getpkt (&rs->buf);
  if (rs->buf[0] == '\0')
    {
      /* Packet not supported.  */
      return -1;
    }

  *result_count =
    parse_threadlist_response (&rs->buf[2], result_limit,
			       &rs->echo_nextthread, threadlist, done);

  if (!threadmatch (&rs->echo_nextthread, nextthread))
    {
      /* FIXME: This is a good reason to drop the packet.  */
      /* Possibly, there is a duplicate response.  */
      /* Possibilities :
	 retransmit immediatly - race conditions
	 retransmit after timeout - yes
	 exit
	 wait for packet, then exit
       */
      warning (_("HMM: threadlist did not echo arg thread, dropping it."));
      return 0;			/* I choose simply exiting.  */
    }
  if (*result_count <= 0)
    {
      if (*done != 1)
	{
	  warning (_("RMT ERROR : failed to get remote thread list."));
	  result = 0;
	}
      return result;		/* break; */
    }
  if (*result_count > result_limit)
    {
      *result_count = 0;
      warning (_("RMT ERROR: threadlist response longer than requested."));
      return 0;
    }
  return result;
}

/* Fetch the list of remote threads, with the qL packet, and call
   STEPFUNCTION for each thread found.  Stops iterating and returns 1
   if STEPFUNCTION returns true.  Stops iterating and returns 0 if the
   STEPFUNCTION returns false.  If the packet is not supported,
   returns -1.  */

int
remote_target::remote_threadlist_iterator (rmt_thread_action stepfunction,
					   void *context, int looplimit)
{
  struct remote_state *rs = get_remote_state ();
  int done, i, result_count;
  int startflag = 1;
  int result = 1;
  int loopcount = 0;

  done = 0;
  while (!done)
    {
      if (loopcount++ > looplimit)
	{
	  result = 0;
	  warning (_("Remote fetch threadlist -infinite loop-."));
	  break;
	}
      result = remote_get_threadlist (startflag, &rs->nextthread,
				      MAXTHREADLISTRESULTS,
				      &done, &result_count,
				      rs->resultthreadlist);
      if (result <= 0)
	break;
      /* Clear for later iterations.  */
      startflag = 0;
      /* Setup to resume next batch of thread references, set nextthread.  */
      if (result_count >= 1)
	copy_threadref (&rs->nextthread,
			&rs->resultthreadlist[result_count - 1]);
      i = 0;
      while (result_count--)
	{
	  if (!(*stepfunction) (&rs->resultthreadlist[i++], context))
	    {
	      result = 0;
	      break;
	    }
	}
    }
  return result;
}

/* A thread found on the remote target.  */

struct thread_item
{
  explicit thread_item (ptid_t ptid_)
  : ptid (ptid_)
  {}

  thread_item (thread_item &&other) = default;
  thread_item &operator= (thread_item &&other) = default;

  DISABLE_COPY_AND_ASSIGN (thread_item);

  /* The thread's PTID.  */
  ptid_t ptid;

  /* The thread's extra info.  */
  std::string extra;

  /* The thread's name.  */
  std::string name;

  /* The core the thread was running on.  -1 if not known.  */
  int core = -1;

  /* The thread handle associated with the thread.  */
  gdb::byte_vector thread_handle;
};

/* Context passed around to the various methods listing remote
   threads.  As new threads are found, they're added to the ITEMS
   vector.  */

struct threads_listing_context
{
  /* Return true if this object contains an entry for a thread with ptid
     PTID.  */

  bool contains_thread (ptid_t ptid) const
  {
    auto match_ptid = [&] (const thread_item &item)
      {
	return item.ptid == ptid;
      };

    auto it = std::find_if (this->items.begin (),
			    this->items.end (),
			    match_ptid);

    return it != this->items.end ();
  }

  /* Remove the thread with ptid PTID.  */

  void remove_thread (ptid_t ptid)
  {
    auto match_ptid = [&] (const thread_item &item)
      {
	return item.ptid == ptid;
      };

    auto it = std::remove_if (this->items.begin (),
			      this->items.end (),
			      match_ptid);

    if (it != this->items.end ())
      this->items.erase (it);
  }

  /* The threads found on the remote target.  */
  std::vector<thread_item> items;
};

static int
remote_newthread_step (threadref *ref, void *data)
{
  struct threads_listing_context *context
    = (struct threads_listing_context *) data;
  int pid = inferior_ptid.pid ();
  int lwp = threadref_to_int (ref);
  ptid_t ptid (pid, lwp);

  context->items.emplace_back (ptid);

  return 1;			/* continue iterator */
}

#define CRAZY_MAX_THREADS 1000

ptid_t
remote_target::remote_current_thread (ptid_t oldpid)
{
  struct remote_state *rs = get_remote_state ();

  putpkt ("qC");
  getpkt (&rs->buf);
  if (rs->buf[0] == 'Q' && rs->buf[1] == 'C')
    {
      const char *obuf;
      ptid_t result;

      result = read_ptid (&rs->buf[2], &obuf);
      if (*obuf != '\0')
	remote_debug_printf ("warning: garbage in qC reply");

      return result;
    }
  else
    return oldpid;
}

/* List remote threads using the deprecated qL packet.  */

int
remote_target::remote_get_threads_with_ql (threads_listing_context *context)
{
  if (remote_threadlist_iterator (remote_newthread_step, context,
				  CRAZY_MAX_THREADS) >= 0)
    return 1;

  return 0;
}

#if defined(HAVE_LIBEXPAT)

static void
start_thread (struct gdb_xml_parser *parser,
	      const struct gdb_xml_element *element,
	      void *user_data,
	      std::vector<gdb_xml_value> &attributes)
{
  struct threads_listing_context *data
    = (struct threads_listing_context *) user_data;
  struct gdb_xml_value *attr;

  char *id = (char *) xml_find_attribute (attributes, "id")->value.get ();
  ptid_t ptid = read_ptid (id, NULL);

  data->items.emplace_back (ptid);
  thread_item &item = data->items.back ();

  attr = xml_find_attribute (attributes, "core");
  if (attr != NULL)
    item.core = *(ULONGEST *) attr->value.get ();

  attr = xml_find_attribute (attributes, "name");
  if (attr != NULL)
    item.name = (const char *) attr->value.get ();

  attr = xml_find_attribute (attributes, "handle");
  if (attr != NULL)
    item.thread_handle = hex2bin ((const char *) attr->value.get ());
}

static void
end_thread (struct gdb_xml_parser *parser,
	    const struct gdb_xml_element *element,
	    void *user_data, const char *body_text)
{
  struct threads_listing_context *data
    = (struct threads_listing_context *) user_data;

  if (body_text != NULL && *body_text != '\0')
    data->items.back ().extra = body_text;
}

const struct gdb_xml_attribute thread_attributes[] = {
  { "id", GDB_XML_AF_NONE, NULL, NULL },
  { "core", GDB_XML_AF_OPTIONAL, gdb_xml_parse_attr_ulongest, NULL },
  { "name", GDB_XML_AF_OPTIONAL, NULL, NULL },
  { "handle", GDB_XML_AF_OPTIONAL, NULL, NULL },
  { NULL, GDB_XML_AF_NONE, NULL, NULL }
};

const struct gdb_xml_element thread_children[] = {
  { NULL, NULL, NULL, GDB_XML_EF_NONE, NULL, NULL }
};

const struct gdb_xml_element threads_children[] = {
  { "thread", thread_attributes, thread_children,
    GDB_XML_EF_REPEATABLE | GDB_XML_EF_OPTIONAL,
    start_thread, end_thread },
  { NULL, NULL, NULL, GDB_XML_EF_NONE, NULL, NULL }
};

const struct gdb_xml_element threads_elements[] = {
  { "threads", NULL, threads_children,
    GDB_XML_EF_NONE, NULL, NULL },
  { NULL, NULL, NULL, GDB_XML_EF_NONE, NULL, NULL }
};

#endif

/* List remote threads using qXfer:threads:read.  */

int
remote_target::remote_get_threads_with_qxfer (threads_listing_context *context)
{
#if defined(HAVE_LIBEXPAT)
  if (m_features.packet_support (PACKET_qXfer_threads) == PACKET_ENABLE)
    {
      std::optional<gdb::char_vector> xml
	= target_read_stralloc (this, TARGET_OBJECT_THREADS, NULL);

      if (xml && (*xml)[0] != '\0')
	{
	  gdb_xml_parse_quick (_("threads"), "threads.dtd",
			       threads_elements, xml->data (), context);
	}

      return 1;
    }
#endif

  return 0;
}

/* List remote threads using qfThreadInfo/qsThreadInfo.  */

int
remote_target::remote_get_threads_with_qthreadinfo (threads_listing_context *context)
{
  struct remote_state *rs = get_remote_state ();

  if (rs->use_threadinfo_query)
    {
      const char *bufp;

      putpkt ("qfThreadInfo");
      getpkt (&rs->buf);
      bufp = rs->buf.data ();
      if (bufp[0] != '\0')		/* q packet recognized */
	{
	  while (*bufp++ == 'm')	/* reply contains one or more TID */
	    {
	      do
		{
		  ptid_t ptid = read_ptid (bufp, &bufp);
		  context->items.emplace_back (ptid);
		}
	      while (*bufp++ == ',');	/* comma-separated list */
	      putpkt ("qsThreadInfo");
	      getpkt (&rs->buf);
	      bufp = rs->buf.data ();
	    }
	  return 1;
	}
      else
	{
	  /* Packet not recognized.  */
	  rs->use_threadinfo_query = 0;
	}
    }

  return 0;
}

/* Return true if INF only has one non-exited thread.  */

static bool
has_single_non_exited_thread (inferior *inf)
{
  int count = 0;
  for (thread_info *tp ATTRIBUTE_UNUSED : inf->non_exited_threads ())
    if (++count > 1)
      break;
  return count == 1;
}

/* Implement the to_update_thread_list function for the remote
   targets.  */

void
remote_target::update_thread_list ()
{
  struct threads_listing_context context;
  int got_list = 0;

  /* We have a few different mechanisms to fetch the thread list.  Try
     them all, starting with the most preferred one first, falling
     back to older methods.  */
  if (remote_get_threads_with_qxfer (&context)
      || remote_get_threads_with_qthreadinfo (&context)
      || remote_get_threads_with_ql (&context))
    {
      got_list = 1;

      if (context.items.empty ()
	  && remote_thread_always_alive (inferior_ptid))
	{
	  /* Some targets don't really support threads, but still
	     reply an (empty) thread list in response to the thread
	     listing packets, instead of replying "packet not
	     supported".  Exit early so we don't delete the main
	     thread.  */
	  return;
	}

      /* CONTEXT now holds the current thread list on the remote
	 target end.  Delete GDB-side threads no longer found on the
	 target.  */
      for (thread_info *tp : all_threads_safe ())
	{
	  if (tp->inf->process_target () != this)
	    continue;

	  if (!context.contains_thread (tp->ptid))
	    {
	      /* Do not remove the thread if it is the last thread in
		 the inferior.  This situation happens when we have a
		 pending exit process status to process.  Otherwise we
		 may end up with a seemingly live inferior (i.e.  pid
		 != 0) that has no threads.  */
	      if (has_single_non_exited_thread (tp->inf))
		continue;

	      /* Do not remove the thread if we've requested to be
		 notified of its exit.  For example, the thread may be
		 displaced stepping, infrun will need to handle the
		 exit event, and displaced stepping info is recorded
		 in the thread object.  If we deleted the thread now,
		 we'd lose that info.  */
	      if ((tp->thread_options () & GDB_THREAD_OPTION_EXIT) != 0)
		continue;

	      /* Not found.  */
	      delete_thread (tp);
	    }
	}

      /* Remove any unreported fork/vfork/clone child threads from
	 CONTEXT so that we don't interfere with follow
	 fork/vfork/clone, which is where creation of such threads is
	 handled.  */
      remove_new_children (&context);

      /* And now add threads we don't know about yet to our list.  */
      for (thread_item &item : context.items)
	{
	  if (item.ptid != null_ptid)
	    {
	      /* In non-stop mode, we assume new found threads are
		 executing until proven otherwise with a stop reply.
		 In all-stop, we can only get here if all threads are
		 stopped.  */
	      bool executing = target_is_non_stop_p ();

	      remote_notice_new_inferior (item.ptid, executing);

	      thread_info *tp = this->find_thread (item.ptid);
	      remote_thread_info *info = get_remote_thread_info (tp);
	      info->core = item.core;
	      info->extra = std::move (item.extra);
	      info->name = std::move (item.name);
	      info->thread_handle = std::move (item.thread_handle);
	    }
	}
    }

  if (!got_list)
    {
      /* If no thread listing method is supported, then query whether
	 each known thread is alive, one by one, with the T packet.
	 If the target doesn't support threads at all, then this is a
	 no-op.  See remote_thread_alive.  */
      prune_threads ();
    }
}

/*
 * Collect a descriptive string about the given thread.
 * The target may say anything it wants to about the thread
 * (typically info about its blocked / runnable state, name, etc.).
 * This string will appear in the info threads display.
 *
 * Optional: targets are not required to implement this function.
 */

const char *
remote_target::extra_thread_info (thread_info *tp)
{
  struct remote_state *rs = get_remote_state ();
  int set;
  threadref id;
  struct gdb_ext_thread_info threadinfo;

  if (rs->remote_desc == 0)		/* paranoia */
    internal_error (_("remote_threads_extra_info"));

  if (tp->ptid == magic_null_ptid
      || (tp->ptid.pid () != 0 && tp->ptid.lwp () == 0))
    /* This is the main thread which was added by GDB.  The remote
       server doesn't know about it.  */
    return NULL;

  std::string &extra = get_remote_thread_info (tp)->extra;

  /* If already have cached info, use it.  */
  if (!extra.empty ())
    return extra.c_str ();

  if (m_features.packet_support (PACKET_qXfer_threads) == PACKET_ENABLE)
    {
      /* If we're using qXfer:threads:read, then the extra info is
	 included in the XML.  So if we didn't have anything cached,
	 it's because there's really no extra info.  */
      return NULL;
    }

  if (rs->use_threadextra_query)
    {
      char *b = rs->buf.data ();
      char *endb = b + get_remote_packet_size ();

      xsnprintf (b, endb - b, "qThreadExtraInfo,");
      b += strlen (b);
      write_ptid (b, endb, tp->ptid);

      putpkt (rs->buf);
      getpkt (&rs->buf);
      if (rs->buf[0] != 0)
	{
	  extra.resize (strlen (rs->buf.data ()) / 2);
	  hex2bin (rs->buf.data (), (gdb_byte *) &extra[0], extra.size ());
	  return extra.c_str ();
	}
    }

  /* If the above query fails, fall back to the old method.  */
  rs->use_threadextra_query = 0;
  set = TAG_THREADID | TAG_EXISTS | TAG_THREADNAME
    | TAG_MOREDISPLAY | TAG_DISPLAY;
  int_to_threadref (&id, tp->ptid.lwp ());
  if (remote_get_threadinfo (&id, set, &threadinfo))
    if (threadinfo.active)
      {
	if (*threadinfo.shortname)
	  string_appendf (extra, " Name: %s", threadinfo.shortname);
	if (*threadinfo.display)
	  {
	    if (!extra.empty ())
	      extra += ',';
	    string_appendf (extra, " State: %s", threadinfo.display);
	  }
	if (*threadinfo.more_display)
	  {
	    if (!extra.empty ())
	      extra += ',';
	    string_appendf (extra, " Priority: %s", threadinfo.more_display);
	  }
	return extra.c_str ();
      }
  return NULL;
}


bool
remote_target::static_tracepoint_marker_at (CORE_ADDR addr,
					    struct static_tracepoint_marker *marker)
{
  struct remote_state *rs = get_remote_state ();
  char *p = rs->buf.data ();

  xsnprintf (p, get_remote_packet_size (), "qTSTMat:");
  p += strlen (p);
  p += hexnumstr (p, addr);
  putpkt (rs->buf);
  getpkt (&rs->buf);
  p = rs->buf.data ();

  if (*p == 'E')
    error (_("Remote failure reply: %s"), p);

  if (*p++ == 'm')
    {
      parse_static_tracepoint_marker_definition (p, NULL, marker);
      return true;
    }

  return false;
}

std::vector<static_tracepoint_marker>
remote_target::static_tracepoint_markers_by_strid (const char *strid)
{
  struct remote_state *rs = get_remote_state ();
  std::vector<static_tracepoint_marker> markers;
  const char *p;
  static_tracepoint_marker marker;

  /* Ask for a first packet of static tracepoint marker
     definition.  */
  putpkt ("qTfSTM");
  getpkt (&rs->buf);
  p = rs->buf.data ();
  if (*p == 'E')
    error (_("Remote failure reply: %s"), p);

  while (*p++ == 'm')
    {
      do
	{
	  parse_static_tracepoint_marker_definition (p, &p, &marker);

	  if (strid == NULL || marker.str_id == strid)
	    markers.push_back (std::move (marker));
	}
      while (*p++ == ',');	/* comma-separated list */
      /* Ask for another packet of static tracepoint definition.  */
      putpkt ("qTsSTM");
      getpkt (&rs->buf);
      p = rs->buf.data ();
    }

  return markers;
}


/* Implement the to_get_ada_task_ptid function for the remote targets.  */

ptid_t
remote_target::get_ada_task_ptid (long lwp, ULONGEST thread)
{
  return ptid_t (inferior_ptid.pid (), lwp);
}


/* Restart the remote side; this is an extended protocol operation.  */

void
remote_target::extended_remote_restart ()
{
  struct remote_state *rs = get_remote_state ();

  /* Send the restart command; for reasons I don't understand the
     remote side really expects a number after the "R".  */
  xsnprintf (rs->buf.data (), get_remote_packet_size (), "R%x", 0);
  putpkt (rs->buf);

  remote_fileio_reset ();
}

/* Clean up connection to a remote debugger.  */

void
remote_target::close ()
{
  /* Make sure we leave stdin registered in the event loop.  */
  terminal_ours ();

  trace_reset_local_state ();

  delete this;
}

remote_target::~remote_target ()
{
  struct remote_state *rs = get_remote_state ();

  /* Check for NULL because we may get here with a partially
     constructed target/connection.  */
  if (rs->remote_desc == nullptr)
    return;

  serial_close (rs->remote_desc);

  /* We are destroying the remote target, so we should discard
     everything of this target.  */
  discard_pending_stop_replies_in_queue ();

  rs->delete_async_event_handler ();

  delete rs->notif_state;
}

/* Query the remote side for the text, data and bss offsets.  */

void
remote_target::get_offsets ()
{
  struct remote_state *rs = get_remote_state ();
  char *buf;
  char *ptr;
  int lose, num_segments = 0, do_sections, do_segments;
  CORE_ADDR text_addr, data_addr, bss_addr, segments[2];

  if (current_program_space->symfile_object_file == NULL)
    return;

  putpkt ("qOffsets");
  getpkt (&rs->buf);
  buf = rs->buf.data ();

  if (buf[0] == '\000')
    return;			/* Return silently.  Stub doesn't support
				   this command.  */
  if (buf[0] == 'E')
    {
      warning (_("Remote failure reply: %s"), buf);
      return;
    }

  /* Pick up each field in turn.  This used to be done with scanf, but
     scanf will make trouble if CORE_ADDR size doesn't match
     conversion directives correctly.  The following code will work
     with any size of CORE_ADDR.  */
  text_addr = data_addr = bss_addr = 0;
  ptr = buf;
  lose = 0;

  if (startswith (ptr, "Text="))
    {
      ptr += 5;
      /* Don't use strtol, could lose on big values.  */
      while (*ptr && *ptr != ';')
	text_addr = (text_addr << 4) + fromhex (*ptr++);

      if (startswith (ptr, ";Data="))
	{
	  ptr += 6;
	  while (*ptr && *ptr != ';')
	    data_addr = (data_addr << 4) + fromhex (*ptr++);
	}
      else
	lose = 1;

      if (!lose && startswith (ptr, ";Bss="))
	{
	  ptr += 5;
	  while (*ptr && *ptr != ';')
	    bss_addr = (bss_addr << 4) + fromhex (*ptr++);

	  if (bss_addr != data_addr)
	    warning (_("Target reported unsupported offsets: %s"), buf);
	}
      else
	lose = 1;
    }
  else if (startswith (ptr, "TextSeg="))
    {
      ptr += 8;
      /* Don't use strtol, could lose on big values.  */
      while (*ptr && *ptr != ';')
	text_addr = (text_addr << 4) + fromhex (*ptr++);
      num_segments = 1;

      if (startswith (ptr, ";DataSeg="))
	{
	  ptr += 9;
	  while (*ptr && *ptr != ';')
	    data_addr = (data_addr << 4) + fromhex (*ptr++);
	  num_segments++;
	}
    }
  else
    lose = 1;

  if (lose)
    error (_("Malformed response to offset query, %s"), buf);
  else if (*ptr != '\0')
    warning (_("Target reported unsupported offsets: %s"), buf);

  objfile *objf = current_program_space->symfile_object_file;
  section_offsets offs = objf->section_offsets;

  symfile_segment_data_up data = get_symfile_segment_data (objf->obfd.get ());
  do_segments = (data != NULL);
  do_sections = num_segments == 0;

  if (num_segments > 0)
    {
      segments[0] = text_addr;
      segments[1] = data_addr;
    }
  /* If we have two segments, we can still try to relocate everything
     by assuming that the .text and .data offsets apply to the whole
     text and data segments.  Convert the offsets given in the packet
     to base addresses for symfile_map_offsets_to_segments.  */
  else if (data != nullptr && data->segments.size () == 2)
    {
      segments[0] = data->segments[0].base + text_addr;
      segments[1] = data->segments[1].base + data_addr;
      num_segments = 2;
    }
  /* If the object file has only one segment, assume that it is text
     rather than data; main programs with no writable data are rare,
     but programs with no code are useless.  Of course the code might
     have ended up in the data segment... to detect that we would need
     the permissions here.  */
  else if (data && data->segments.size () == 1)
    {
      segments[0] = data->segments[0].base + text_addr;
      num_segments = 1;
    }
  /* There's no way to relocate by segment.  */
  else
    do_segments = 0;

  if (do_segments)
    {
      int ret = symfile_map_offsets_to_segments (objf->obfd.get (),
						 data.get (), offs,
						 num_segments, segments);

      if (ret == 0 && !do_sections)
	error (_("Can not handle qOffsets TextSeg "
		 "response with this symbol file"));

      if (ret > 0)
	do_sections = 0;
    }

  if (do_sections)
    {
      offs[SECT_OFF_TEXT (objf)] = text_addr;

      /* This is a temporary kludge to force data and bss to use the
	 same offsets because that's what nlmconv does now.  The real
	 solution requires changes to the stub and remote.c that I
	 don't have time to do right now.  */

      offs[SECT_OFF_DATA (objf)] = data_addr;
      offs[SECT_OFF_BSS (objf)] = data_addr;
    }

  objfile_relocate (objf, offs);
}

/* Send interrupt_sequence to remote target.  */

void
remote_target::send_interrupt_sequence ()
{
  if (interrupt_sequence_mode == interrupt_sequence_control_c)
    remote_serial_write ("\x03", 1);
  else if (interrupt_sequence_mode == interrupt_sequence_break)
    remote_serial_send_break ();
  else if (interrupt_sequence_mode == interrupt_sequence_break_g)
    {
      remote_serial_send_break ();
      remote_serial_write ("g", 1);
    }
  else
    internal_error (_("Invalid value for interrupt_sequence_mode: %s."),
		    interrupt_sequence_mode);
}

/* If STOP_REPLY is a T stop reply, look for the "thread" register,
   and extract the PTID.  Returns NULL_PTID if not found.  */

static ptid_t
stop_reply_extract_thread (const char *stop_reply)
{
  if (stop_reply[0] == 'T' && strlen (stop_reply) > 3)
    {
      const char *p;

      /* Txx r:val ; r:val (...)  */
      p = &stop_reply[3];

      /* Look for "register" named "thread".  */
      while (*p != '\0')
	{
	  const char *p1;

	  p1 = strchr (p, ':');
	  if (p1 == NULL)
	    return null_ptid;

	  if (strncmp (p, "thread", p1 - p) == 0)
	    return read_ptid (++p1, &p);

	  p1 = strchr (p, ';');
	  if (p1 == NULL)
	    return null_ptid;
	  p1++;

	  p = p1;
	}
    }

  return null_ptid;
}

/* Determine the remote side's current thread.  If we have a stop
   reply handy (in WAIT_STATUS), maybe it's a T stop reply with a
   "thread" register we can extract the current thread from.  If not,
   ask the remote which is the current thread with qC.  The former
   method avoids a roundtrip.  */

ptid_t
remote_target::get_current_thread (const char *wait_status)
{
  ptid_t ptid = null_ptid;

  /* Note we don't use remote_parse_stop_reply as that makes use of
     the target architecture, which we haven't yet fully determined at
     this point.  */
  if (wait_status != NULL)
    ptid = stop_reply_extract_thread (wait_status);
  if (ptid == null_ptid)
    ptid = remote_current_thread (inferior_ptid);

  return ptid;
}

/* Query the remote target for which is the current thread/process,
   add it to our tables, and update INFERIOR_PTID.  The caller is
   responsible for setting the state such that the remote end is ready
   to return the current thread.

   This function is called after handling the '?' or 'vRun' packets,
   whose response is a stop reply from which we can also try
   extracting the thread.  If the target doesn't support the explicit
   qC query, we infer the current thread from that stop reply, passed
   in in WAIT_STATUS, which may be NULL.

   The function returns pointer to the main thread of the inferior. */

thread_info *
remote_target::add_current_inferior_and_thread (const char *wait_status)
{
  bool fake_pid_p = false;

  switch_to_no_thread ();

  /* Now, if we have thread information, update the current thread's
     ptid.  */
  ptid_t curr_ptid = get_current_thread (wait_status);

  if (curr_ptid != null_ptid)
    {
      if (!m_features.remote_multi_process_p ())
	fake_pid_p = true;
    }
  else
    {
      /* Without this, some commands which require an active target
	 (such as kill) won't work.  This variable serves (at least)
	 double duty as both the pid of the target process (if it has
	 such), and as a flag indicating that a target is active.  */
      curr_ptid = magic_null_ptid;
      fake_pid_p = true;
    }

  remote_add_inferior (fake_pid_p, curr_ptid.pid (), -1, 1);

  /* Add the main thread and switch to it.  Don't try reading
     registers yet, since we haven't fetched the target description
     yet.  */
  thread_info *tp = add_thread_silent (this, curr_ptid);
  switch_to_thread_no_regs (tp);

  return tp;
}

/* Print info about a thread that was found already stopped on
   connection.  */

void
remote_target::print_one_stopped_thread (thread_info *thread)
{
  target_waitstatus ws;

  /* If there is a pending waitstatus, use it.  If there isn't it's because
     the thread's stop was reported with TARGET_WAITKIND_STOPPED / GDB_SIGNAL_0
     and process_initial_stop_replies decided it wasn't interesting to save
     and report to the core.  */
  if (thread->has_pending_waitstatus ())
    {
      ws = thread->pending_waitstatus ();
      thread->clear_pending_waitstatus ();
    }
  else
    {
      ws.set_stopped (GDB_SIGNAL_0);
    }

  switch_to_thread (thread);
  thread->set_stop_pc (get_frame_pc (get_current_frame ()));
  set_current_sal_from_frame (get_current_frame ());

  /* For "info program".  */
  set_last_target_status (this, thread->ptid, ws);

  if (ws.kind () == TARGET_WAITKIND_STOPPED)
    {
      enum gdb_signal sig = ws.sig ();

      if (signal_print_state (sig))
	notify_signal_received (sig);
    }

  notify_normal_stop (nullptr, 1);
}

/* Process all initial stop replies the remote side sent in response
   to the ? packet.  These indicate threads that were already stopped
   on initial connection.  We mark these threads as stopped and print
   their current frame before giving the user the prompt.  */

void
remote_target::process_initial_stop_replies (int from_tty)
{
  int pending_stop_replies = stop_reply_queue_length ();
  struct thread_info *selected = NULL;
  struct thread_info *lowest_stopped = NULL;
  struct thread_info *first = NULL;

  /* This is only used when the target is non-stop.  */
  gdb_assert (target_is_non_stop_p ());

  /* Consume the initial pending events.  */
  while (pending_stop_replies-- > 0)
    {
      ptid_t waiton_ptid = minus_one_ptid;
      ptid_t event_ptid;
      struct target_waitstatus ws;
      int ignore_event = 0;

      event_ptid = target_wait (waiton_ptid, &ws, TARGET_WNOHANG);
      if (remote_debug)
	print_target_wait_results (waiton_ptid, event_ptid, ws);

      switch (ws.kind ())
	{
	case TARGET_WAITKIND_IGNORE:
	case TARGET_WAITKIND_NO_RESUMED:
	case TARGET_WAITKIND_SIGNALLED:
	case TARGET_WAITKIND_EXITED:
	  /* We shouldn't see these, but if we do, just ignore.  */
	  remote_debug_printf ("event ignored");
	  ignore_event = 1;
	  break;

	default:
	  break;
	}

      if (ignore_event)
	continue;

      thread_info *evthread = this->find_thread (event_ptid);

      if (ws.kind () == TARGET_WAITKIND_STOPPED)
	{
	  enum gdb_signal sig = ws.sig ();

	  /* Stubs traditionally report SIGTRAP as initial signal,
	     instead of signal 0.  Suppress it.  */
	  if (sig == GDB_SIGNAL_TRAP)
	    sig = GDB_SIGNAL_0;
	  evthread->set_stop_signal (sig);
	  ws.set_stopped (sig);
	}

      if (ws.kind () != TARGET_WAITKIND_STOPPED
	  || ws.sig () != GDB_SIGNAL_0)
	evthread->set_pending_waitstatus (ws);

      set_executing (this, event_ptid, false);
      set_running (this, event_ptid, false);
      get_remote_thread_info (evthread)->set_not_resumed ();
    }

  /* "Notice" the new inferiors before anything related to
     registers/memory.  */
  for (inferior *inf : all_non_exited_inferiors (this))
    {
      inf->needs_setup = true;

      if (non_stop)
	{
	  thread_info *thread = any_live_thread_of_inferior (inf);
	  notice_new_inferior (thread, thread->state == THREAD_RUNNING,
			       from_tty);
	}
    }

  /* If all-stop on top of non-stop, pause all threads.  Note this
     records the threads' stop pc, so must be done after "noticing"
     the inferiors.  */
  if (!non_stop)
    {
      {
	/* At this point, the remote target is not async.  It needs to be for
	   the poll in stop_all_threads to consider events from it, so enable
	   it temporarily.  */
	gdb_assert (!this->is_async_p ());
	SCOPE_EXIT { target_async (false); };
	target_async (true);
	stop_all_threads ("remote connect in all-stop");
      }

      /* If all threads of an inferior were already stopped, we
	 haven't setup the inferior yet.  */
      for (inferior *inf : all_non_exited_inferiors (this))
	{
	  if (inf->needs_setup)
	    {
	      thread_info *thread = any_live_thread_of_inferior (inf);
	      switch_to_thread_no_regs (thread);
	      setup_inferior (0);
	    }
	}
    }

  /* Now go over all threads that are stopped, and print their current
     frame.  If all-stop, then if there's a signalled thread, pick
     that as current.  */
  for (thread_info *thread : all_non_exited_threads (this))
    {
      if (first == NULL)
	first = thread;

      if (!non_stop)
	thread->set_running (false);
      else if (thread->state != THREAD_STOPPED)
	continue;

      if (selected == nullptr && thread->has_pending_waitstatus ())
	selected = thread;

      if (lowest_stopped == NULL
	  || thread->inf->num < lowest_stopped->inf->num
	  || thread->per_inf_num < lowest_stopped->per_inf_num)
	lowest_stopped = thread;

      if (non_stop)
	print_one_stopped_thread (thread);
    }

  /* In all-stop, we only print the status of one thread, and leave
     others with their status pending.  */
  if (!non_stop)
    {
      thread_info *thread = selected;
      if (thread == NULL)
	thread = lowest_stopped;
      if (thread == NULL)
	thread = first;

      print_one_stopped_thread (thread);
    }
}

/* Mark a remote_target as starting (by setting the starting_up flag within
   its remote_state) for the lifetime of this object.  The reference count
   on the remote target is temporarily incremented, to prevent the target
   being deleted under our feet.  */

struct scoped_mark_target_starting
{
  /* Constructor, TARGET is the target to be marked as starting, its
     reference count will be incremented.  */
  scoped_mark_target_starting (remote_target *target)
    : m_remote_target (remote_target_ref::new_reference (target)),
      m_restore_starting_up (set_starting_up_flag (target))
  { /* Nothing.  */ }

private:

  /* Helper function, set the starting_up flag on TARGET and return an
     object which, when it goes out of scope, will restore the previous
     value of the starting_up flag.  */
  static scoped_restore_tmpl<bool>
  set_starting_up_flag (remote_target *target)
  {
    remote_state *rs = target->get_remote_state ();
    gdb_assert (!rs->starting_up);
    return make_scoped_restore (&rs->starting_up, true);
  }

  /* A gdb::ref_ptr pointer to a remote_target.  */
  using remote_target_ref = gdb::ref_ptr<remote_target, target_ops_ref_policy>;

  /* A reference to the target on which we are operating.  */
  remote_target_ref m_remote_target;

  /* An object which restores the previous value of the starting_up flag
     when it goes out of scope.  */
  scoped_restore_tmpl<bool> m_restore_starting_up;
};

/* Transfer ownership of the stop_reply owned by EVENT to a
   stop_reply_up object.  */

static stop_reply_up
as_stop_reply_up (notif_event_up event)
{
  auto *stop_reply = static_cast<struct stop_reply *> (event.release ());
  return stop_reply_up (stop_reply);
}

/* Helper for remote_target::start_remote, start the remote connection and
   sync state.  Return true if everything goes OK, otherwise, return false.
   This function exists so that the scoped_restore created within it will
   expire before we return to remote_target::start_remote.  */

bool
remote_target::start_remote_1 (int from_tty, int extended_p)
{
  REMOTE_SCOPED_DEBUG_ENTER_EXIT;

  struct remote_state *rs = get_remote_state ();

  /* Signal other parts that we're going through the initial setup,
     and so things may not be stable yet.  E.g., we don't try to
     install tracepoints until we've relocated symbols.  Also, a
     Ctrl-C before we're connected and synced up can't interrupt the
     target.  Instead, it offers to drop the (potentially wedged)
     connection.  */
  scoped_mark_target_starting target_is_starting (this);

  QUIT;

  if (interrupt_on_connect)
    send_interrupt_sequence ();

  /* Ack any packet which the remote side has already sent.  */
  remote_serial_write ("+", 1);

  /* The first packet we send to the target is the optional "supported
     packets" request.  If the target can answer this, it will tell us
     which later probes to skip.  */
  remote_query_supported ();

  /* Check vCont support and set the remote state's vCont_action_support
     attribute.  */
  remote_vcont_probe ();

  /* If the stub wants to get a QAllow, compose one and send it.  */
  if (m_features.packet_support (PACKET_QAllow) != PACKET_DISABLE)
    set_permissions ();

  /* gdbserver < 7.7 (before its fix from 2013-12-11) did reply to any
     unknown 'v' packet with string "OK".  "OK" gets interpreted by GDB
     as a reply to known packet.  For packet "vFile:setfs:" it is an
     invalid reply and GDB would return error in
     remote_hostio_set_filesystem, making remote files access impossible.
     Disable "vFile:setfs:" in such case.  Do not disable other 'v' packets as
     other "vFile" packets get correctly detected even on gdbserver < 7.7.  */
  {
    const char v_mustreplyempty[] = "vMustReplyEmpty";

    putpkt (v_mustreplyempty);
    getpkt (&rs->buf);
    if (strcmp (rs->buf.data (), "OK") == 0)
      {
	m_features.m_protocol_packets[PACKET_vFile_setfs].support
	  = PACKET_DISABLE;
      }
    else if (strcmp (rs->buf.data (), "") != 0)
      error (_("Remote replied unexpectedly to '%s': %s"), v_mustreplyempty,
	     rs->buf.data ());
  }

  /* Next, we possibly activate noack mode.

     If the QStartNoAckMode packet configuration is set to AUTO,
     enable noack mode if the stub reported a wish for it with
     qSupported.

     If set to TRUE, then enable noack mode even if the stub didn't
     report it in qSupported.  If the stub doesn't reply OK, the
     session ends with an error.

     If FALSE, then don't activate noack mode, regardless of what the
     stub claimed should be the default with qSupported.  */

  if (m_features.packet_support (PACKET_QStartNoAckMode) != PACKET_DISABLE)
    {
      putpkt ("QStartNoAckMode");
      getpkt (&rs->buf);
      if (m_features.packet_ok (rs->buf, PACKET_QStartNoAckMode) == PACKET_OK)
	rs->noack_mode = 1;
    }

  if (extended_p)
    {
      /* Tell the remote that we are using the extended protocol.  */
      putpkt ("!");
      getpkt (&rs->buf);
    }

  /* Let the target know which signals it is allowed to pass down to
     the program.  */
  update_signals_program_target ();

  /* Next, if the target can specify a description, read it.  We do
     this before anything involving memory or registers.  */
  target_find_description ();

  /* Next, now that we know something about the target, update the
     address spaces in the program spaces.  */
  update_address_spaces ();

  /* On OSs where the list of libraries is global to all
     processes, we fetch them early.  */
  if (gdbarch_has_global_solist (current_inferior ()->arch ()))
    solib_add (NULL, from_tty, auto_solib_add);

  if (target_is_non_stop_p ())
    {
      if (m_features.packet_support (PACKET_QNonStop) != PACKET_ENABLE)
	error (_("Non-stop mode requested, but remote "
		 "does not support non-stop"));

      putpkt ("QNonStop:1");
      getpkt (&rs->buf);

      if (strcmp (rs->buf.data (), "OK") != 0)
	error (_("Remote refused setting non-stop mode with: %s"),
	       rs->buf.data ());

      /* Find about threads and processes the stub is already
	 controlling.  We default to adding them in the running state.
	 The '?' query below will then tell us about which threads are
	 stopped.  */
      this->update_thread_list ();
    }
  else if (m_features.packet_support (PACKET_QNonStop) == PACKET_ENABLE)
    {
      /* Don't assume that the stub can operate in all-stop mode.
	 Request it explicitly.  */
      putpkt ("QNonStop:0");
      getpkt (&rs->buf);

      if (strcmp (rs->buf.data (), "OK") != 0)
	error (_("Remote refused setting all-stop mode with: %s"),
	       rs->buf.data ());
    }

  /* Upload TSVs regardless of whether the target is running or not.  The
     remote stub, such as GDBserver, may have some predefined or builtin
     TSVs, even if the target is not running.  */
  if (get_trace_status (current_trace_status ()) != -1)
    {
      struct uploaded_tsv *uploaded_tsvs = NULL;

      upload_trace_state_variables (&uploaded_tsvs);
      merge_uploaded_trace_state_variables (&uploaded_tsvs);
    }

  /* Check whether the target is running now.  */
  putpkt ("?");
  getpkt (&rs->buf);

  if (!target_is_non_stop_p ())
    {
      char *wait_status = NULL;

      if (rs->buf[0] == 'W' || rs->buf[0] == 'X')
	{
	  if (!extended_p)
	    error (_("The target is not running (try extended-remote?)"));
	  return false;
	}
      else
	{
	  /* Save the reply for later.  */
	  wait_status = (char *) alloca (strlen (rs->buf.data ()) + 1);
	  strcpy (wait_status, rs->buf.data ());
	}

      /* Fetch thread list.  */
      target_update_thread_list ();

      /* Let the stub know that we want it to return the thread.  */
      set_continue_thread (minus_one_ptid);

      if (thread_count (this) == 0)
	{
	  /* Target has no concept of threads at all.  GDB treats
	     non-threaded target as single-threaded; add a main
	     thread.  */
	  thread_info *tp = add_current_inferior_and_thread (wait_status);
	  get_remote_thread_info (tp)->set_resumed ();
	}
      else
	{
	  /* We have thread information; select the thread the target
	     says should be current.  If we're reconnecting to a
	     multi-threaded program, this will ideally be the thread
	     that last reported an event before GDB disconnected.  */
	  ptid_t curr_thread = get_current_thread (wait_status);
	  if (curr_thread == null_ptid)
	    {
	      /* Odd... The target was able to list threads, but not
		 tell us which thread was current (no "thread"
		 register in T stop reply?).  Just pick the first
		 thread in the thread list then.  */

	      remote_debug_printf ("warning: couldn't determine remote "
				   "current thread; picking first in list.");

	      for (thread_info *tp : all_non_exited_threads (this,
							     minus_one_ptid))
		{
		  switch_to_thread (tp);
		  break;
		}
	    }
	  else
	    switch_to_thread (this->find_thread (curr_thread));

	  get_remote_thread_info (inferior_thread ())->set_resumed ();
	}

      /* init_wait_for_inferior should be called before get_offsets in order
	 to manage `inserted' flag in bp loc in a correct state.
	 breakpoint_init_inferior, called from init_wait_for_inferior, set
	 `inserted' flag to 0, while before breakpoint_re_set, called from
	 start_remote, set `inserted' flag to 1.  In the initialization of
	 inferior, breakpoint_init_inferior should be called first, and then
	 breakpoint_re_set can be called.  If this order is broken, state of
	 `inserted' flag is wrong, and cause some problems on breakpoint
	 manipulation.  */
      init_wait_for_inferior ();

      get_offsets ();		/* Get text, data & bss offsets.  */

      /* If we could not find a description using qXfer, and we know
	 how to do it some other way, try again.  This is not
	 supported for non-stop; it could be, but it is tricky if
	 there are no stopped threads when we connect.  */
      if (remote_read_description_p (this)
	  && gdbarch_target_desc (current_inferior ()->arch ()) == NULL)
	{
	  target_clear_description ();
	  target_find_description ();
	}

      /* Use the previously fetched status.  */
      gdb_assert (wait_status != NULL);
      notif_event_up reply
	= remote_notif_parse (this, &notif_client_stop, wait_status);
      push_stop_reply (as_stop_reply_up (std::move (reply)));

      ::start_remote (from_tty); /* Initialize gdb process mechanisms.  */
    }
  else
    {
      /* Clear WFI global state.  Do this before finding about new
	 threads and inferiors, and setting the current inferior.
	 Otherwise we would clear the proceed status of the current
	 inferior when we want its stop_soon state to be preserved
	 (see notice_new_inferior).  */
      init_wait_for_inferior ();

      /* In non-stop, we will either get an "OK", meaning that there
	 are no stopped threads at this time; or, a regular stop
	 reply.  In the latter case, there may be more than one thread
	 stopped --- we pull them all out using the vStopped
	 mechanism.  */
      if (strcmp (rs->buf.data (), "OK") != 0)
	{
	  const notif_client *notif = &notif_client_stop;

	  /* remote_notif_get_pending_replies acks this one, and gets
	     the rest out.  */
	  rs->notif_state->pending_event[notif_client_stop.id]
	    = remote_notif_parse (this, notif, rs->buf.data ());
	  remote_notif_get_pending_events (notif);
	}

      if (thread_count (this) == 0)
	{
	  if (!extended_p)
	    error (_("The target is not running (try extended-remote?)"));
	  return false;
	}

      /* Report all signals during attach/startup.  */
      pass_signals ({});

      /* If there are already stopped threads, mark them stopped and
	 report their stops before giving the prompt to the user.  */
      process_initial_stop_replies (from_tty);

      if (target_can_async_p ())
	target_async (true);
    }

  /* Give the target a chance to look up symbols.  */
  for (inferior *inf : all_inferiors (this))
    {
      /* The inferiors that exist at this point were created from what
	 was found already running on the remote side, so we know they
	 have execution.  */
      gdb_assert (this->has_execution (inf));

      /* No use without a symbol-file.  */
      if (inf->pspace->symfile_object_file == nullptr)
	continue;

      /* Need to switch to a specific thread, because remote_check_symbols
	 uses INFERIOR_PTID to set the general thread.  */
      scoped_restore_current_thread restore_thread;
      thread_info *thread = any_thread_of_inferior (inf);
      switch_to_thread (thread);
      this->remote_check_symbols ();
    }

  /* Possibly the target has been engaged in a trace run started
     previously; find out where things are at.  */
  if (get_trace_status (current_trace_status ()) != -1)
    {
      struct uploaded_tp *uploaded_tps = NULL;

      if (current_trace_status ()->running)
	gdb_printf (_("Trace is already running on the target.\n"));

      upload_tracepoints (&uploaded_tps);

      merge_uploaded_tracepoints (&uploaded_tps);
    }

  /* Possibly the target has been engaged in a btrace record started
     previously; find out where things are at.  */
  remote_btrace_maybe_reopen ();

  return true;
}

/* Start the remote connection and sync state.  */

void
remote_target::start_remote (int from_tty, int extended_p)
{
  if (start_remote_1 (from_tty, extended_p)
      && breakpoints_should_be_inserted_now ())
    insert_breakpoints ();
}

const char *
remote_target::connection_string ()
{
  remote_state *rs = get_remote_state ();

  if (rs->remote_desc->name != NULL)
    return rs->remote_desc->name;
  else
    return NULL;
}

/* Open a connection to a remote debugger.
   NAME is the filename used for communication.  */

void
remote_target::open (const char *name, int from_tty)
{
  open_1 (name, from_tty, 0);
}

/* Open a connection to a remote debugger using the extended
   remote gdb protocol.  NAME is the filename used for communication.  */

void
extended_remote_target::open (const char *name, int from_tty)
{
  open_1 (name, from_tty, 1 /*extended_p */);
}

void
remote_features::reset_all_packet_configs_support ()
{
  int i;

  for (i = 0; i < PACKET_MAX; i++)
    m_protocol_packets[i].support = PACKET_SUPPORT_UNKNOWN;
}

/* Initialize all packet configs.  */

static void
init_all_packet_configs (void)
{
  int i;

  for (i = 0; i < PACKET_MAX; i++)
    {
      remote_protocol_packets[i].detect = AUTO_BOOLEAN_AUTO;
      remote_protocol_packets[i].support = PACKET_SUPPORT_UNKNOWN;
    }
}

/* Symbol look-up.  */

void
remote_target::remote_check_symbols ()
{
  char *tmp;
  int end;

  /* It doesn't make sense to send a qSymbol packet for an inferior that
     doesn't have execution, because the remote side doesn't know about
     inferiors without execution.  */
  gdb_assert (target_has_execution ());

  if (m_features.packet_support (PACKET_qSymbol) == PACKET_DISABLE)
    return;

  /* Make sure the remote is pointing at the right process.  Note
     there's no way to select "no process".  */
  set_general_process ();

  /* Allocate a message buffer.  We can't reuse the input buffer in RS,
     because we need both at the same time.  */
  gdb::char_vector msg (get_remote_packet_size ());
  gdb::char_vector reply (get_remote_packet_size ());

  /* Invite target to request symbol lookups.  */

  putpkt ("qSymbol::");
  getpkt (&reply);
  m_features.packet_ok (reply, PACKET_qSymbol);

  while (startswith (reply.data (), "qSymbol:"))
    {
      struct bound_minimal_symbol sym;

      tmp = &reply[8];
      end = hex2bin (tmp, reinterpret_cast <gdb_byte *> (msg.data ()),
		     strlen (tmp) / 2);
      msg[end] = '\0';
      sym = lookup_minimal_symbol (msg.data (), NULL, NULL);
      if (sym.minsym == NULL)
	xsnprintf (msg.data (), get_remote_packet_size (), "qSymbol::%s",
		   &reply[8]);
      else
	{
	  int addr_size = gdbarch_addr_bit (current_inferior ()->arch ()) / 8;
	  CORE_ADDR sym_addr = sym.value_address ();

	  /* If this is a function address, return the start of code
	     instead of any data function descriptor.  */
	  sym_addr = gdbarch_convert_from_func_ptr_addr
	    (current_inferior ()->arch (), sym_addr,
	     current_inferior ()->top_target ());

	  xsnprintf (msg.data (), get_remote_packet_size (), "qSymbol:%s:%s",
		     phex_nz (sym_addr, addr_size), &reply[8]);
	}

      putpkt (msg.data ());
      getpkt (&reply);
    }
}

static struct serial *
remote_serial_open (const char *name)
{
  static int udp_warning = 0;

  /* FIXME: Parsing NAME here is a hack.  But we want to warn here instead
     of in ser-tcp.c, because it is the remote protocol assuming that the
     serial connection is reliable and not the serial connection promising
     to be.  */
  if (!udp_warning && startswith (name, "udp:"))
    {
      warning (_("The remote protocol may be unreliable over UDP.\n"
		 "Some events may be lost, rendering further debugging "
		 "impossible."));
      udp_warning = 1;
    }

  return serial_open (name);
}

/* Inform the target of our permission settings.  The permission flags
   work without this, but if the target knows the settings, it can do
   a couple things.  First, it can add its own check, to catch cases
   that somehow manage to get by the permissions checks in target
   methods.  Second, if the target is wired to disallow particular
   settings (for instance, a system in the field that is not set up to
   be able to stop at a breakpoint), it can object to any unavailable
   permissions.  */

void
remote_target::set_permissions ()
{
  struct remote_state *rs = get_remote_state ();

  xsnprintf (rs->buf.data (), get_remote_packet_size (), "QAllow:"
	     "WriteReg:%x;WriteMem:%x;"
	     "InsertBreak:%x;InsertTrace:%x;"
	     "InsertFastTrace:%x;Stop:%x",
	     may_write_registers, may_write_memory,
	     may_insert_breakpoints, may_insert_tracepoints,
	     may_insert_fast_tracepoints, may_stop);
  putpkt (rs->buf);
  getpkt (&rs->buf);

  /* If the target didn't like the packet, warn the user.  Do not try
     to undo the user's settings, that would just be maddening.  */
  if (strcmp (rs->buf.data (), "OK") != 0)
    warning (_("Remote refused setting permissions with: %s"),
	     rs->buf.data ());
}

/* This type describes each known response to the qSupported
   packet.  */
struct protocol_feature
{
  /* The name of this protocol feature.  */
  const char *name;

  /* The default for this protocol feature.  */
  enum packet_support default_support;

  /* The function to call when this feature is reported, or after
     qSupported processing if the feature is not supported.
     The first argument points to this structure.  The second
     argument indicates whether the packet requested support be
     enabled, disabled, or probed (or the default, if this function
     is being called at the end of processing and this feature was
     not reported).  The third argument may be NULL; if not NULL, it
     is a NUL-terminated string taken from the packet following
     this feature's name and an equals sign.  */
  void (*func) (remote_target *remote, const struct protocol_feature *,
		enum packet_support, const char *);

  /* The corresponding packet for this feature.  Only used if
     FUNC is remote_supported_packet.  */
  int packet;
};

static void
remote_supported_packet (remote_target *remote,
			 const struct protocol_feature *feature,
			 enum packet_support support,
			 const char *argument)
{
  if (argument)
    {
      warning (_("Remote qSupported response supplied an unexpected value for"
		 " \"%s\"."), feature->name);
      return;
    }

  remote->m_features.m_protocol_packets[feature->packet].support = support;
}

void
remote_target::remote_packet_size (const protocol_feature *feature,
				   enum packet_support support,
				   const char *value)
{
  struct remote_state *rs = get_remote_state ();

  int packet_size;
  char *value_end;

  if (support != PACKET_ENABLE)
    return;

  if (value == NULL || *value == '\0')
    {
      warning (_("Remote target reported \"%s\" without a size."),
	       feature->name);
      return;
    }

  errno = 0;
  packet_size = strtol (value, &value_end, 16);
  if (errno != 0 || *value_end != '\0' || packet_size < 0)
    {
      warning (_("Remote target reported \"%s\" with a bad size: \"%s\"."),
	       feature->name, value);
      return;
    }

  /* Record the new maximum packet size.  */
  rs->explicit_packet_size = packet_size;
}

static void
remote_packet_size (remote_target *remote, const protocol_feature *feature,
		    enum packet_support support, const char *value)
{
  remote->remote_packet_size (feature, support, value);
}

void
remote_target::remote_supported_thread_options (const protocol_feature *feature,
						enum packet_support support,
						const char *value)
{
  struct remote_state *rs = get_remote_state ();

  m_features.m_protocol_packets[feature->packet].support = support;

  if (support != PACKET_ENABLE)
    return;

  if (value == nullptr || *value == '\0')
    {
      warning (_("Remote target reported \"%s\" without supported options."),
	       feature->name);
      return;
    }

  ULONGEST options = 0;
  const char *p = unpack_varlen_hex (value, &options);

  if (*p != '\0')
    {
      warning (_("Remote target reported \"%s\" with "
		 "bad thread options: \"%s\"."),
	       feature->name, value);
      return;
    }

  /* Record the set of supported options.  */
  rs->supported_thread_options = (gdb_thread_option) options;
}

static void
remote_supported_thread_options (remote_target *remote,
				 const protocol_feature *feature,
				 enum packet_support support,
				 const char *value)
{
  remote->remote_supported_thread_options (feature, support, value);
}

static const struct protocol_feature remote_protocol_features[] = {
  { "PacketSize", PACKET_DISABLE, remote_packet_size, -1 },
  { "qXfer:auxv:read", PACKET_DISABLE, remote_supported_packet,
    PACKET_qXfer_auxv },
  { "qXfer:exec-file:read", PACKET_DISABLE, remote_supported_packet,
    PACKET_qXfer_exec_file },
  { "qXfer:features:read", PACKET_DISABLE, remote_supported_packet,
    PACKET_qXfer_features },
  { "qXfer:libraries:read", PACKET_DISABLE, remote_supported_packet,
    PACKET_qXfer_libraries },
  { "qXfer:libraries-svr4:read", PACKET_DISABLE, remote_supported_packet,
    PACKET_qXfer_libraries_svr4 },
  { "augmented-libraries-svr4-read", PACKET_DISABLE,
    remote_supported_packet, PACKET_augmented_libraries_svr4_read_feature },
  { "qXfer:memory-map:read", PACKET_DISABLE, remote_supported_packet,
    PACKET_qXfer_memory_map },
  { "qXfer:osdata:read", PACKET_DISABLE, remote_supported_packet,
    PACKET_qXfer_osdata },
  { "qXfer:threads:read", PACKET_DISABLE, remote_supported_packet,
    PACKET_qXfer_threads },
  { "qXfer:traceframe-info:read", PACKET_DISABLE, remote_supported_packet,
    PACKET_qXfer_traceframe_info },
  { "QPassSignals", PACKET_DISABLE, remote_supported_packet,
    PACKET_QPassSignals },
  { "QCatchSyscalls", PACKET_DISABLE, remote_supported_packet,
    PACKET_QCatchSyscalls },
  { "QProgramSignals", PACKET_DISABLE, remote_supported_packet,
    PACKET_QProgramSignals },
  { "QSetWorkingDir", PACKET_DISABLE, remote_supported_packet,
    PACKET_QSetWorkingDir },
  { "QStartupWithShell", PACKET_DISABLE, remote_supported_packet,
    PACKET_QStartupWithShell },
  { "QEnvironmentHexEncoded", PACKET_DISABLE, remote_supported_packet,
    PACKET_QEnvironmentHexEncoded },
  { "QEnvironmentReset", PACKET_DISABLE, remote_supported_packet,
    PACKET_QEnvironmentReset },
  { "QEnvironmentUnset", PACKET_DISABLE, remote_supported_packet,
    PACKET_QEnvironmentUnset },
  { "QStartNoAckMode", PACKET_DISABLE, remote_supported_packet,
    PACKET_QStartNoAckMode },
  { "multiprocess", PACKET_DISABLE, remote_supported_packet,
    PACKET_multiprocess_feature },
  { "QNonStop", PACKET_DISABLE, remote_supported_packet, PACKET_QNonStop },
  { "qXfer:siginfo:read", PACKET_DISABLE, remote_supported_packet,
    PACKET_qXfer_siginfo_read },
  { "qXfer:siginfo:write", PACKET_DISABLE, remote_supported_packet,
    PACKET_qXfer_siginfo_write },
  { "ConditionalTracepoints", PACKET_DISABLE, remote_supported_packet,
    PACKET_ConditionalTracepoints },
  { "ConditionalBreakpoints", PACKET_DISABLE, remote_supported_packet,
    PACKET_ConditionalBreakpoints },
  { "BreakpointCommands", PACKET_DISABLE, remote_supported_packet,
    PACKET_BreakpointCommands },
  { "FastTracepoints", PACKET_DISABLE, remote_supported_packet,
    PACKET_FastTracepoints },
  { "StaticTracepoints", PACKET_DISABLE, remote_supported_packet,
    PACKET_StaticTracepoints },
  {"InstallInTrace", PACKET_DISABLE, remote_supported_packet,
   PACKET_InstallInTrace},
  { "DisconnectedTracing", PACKET_DISABLE, remote_supported_packet,
    PACKET_DisconnectedTracing_feature },
  { "ReverseContinue", PACKET_DISABLE, remote_supported_packet,
    PACKET_bc },
  { "ReverseStep", PACKET_DISABLE, remote_supported_packet,
    PACKET_bs },
  { "TracepointSource", PACKET_DISABLE, remote_supported_packet,
    PACKET_TracepointSource },
  { "QAllow", PACKET_DISABLE, remote_supported_packet,
    PACKET_QAllow },
  { "EnableDisableTracepoints", PACKET_DISABLE, remote_supported_packet,
    PACKET_EnableDisableTracepoints_feature },
  { "qXfer:fdpic:read", PACKET_DISABLE, remote_supported_packet,
    PACKET_qXfer_fdpic },
  { "qXfer:uib:read", PACKET_DISABLE, remote_supported_packet,
    PACKET_qXfer_uib },
  { "QDisableRandomization", PACKET_DISABLE, remote_supported_packet,
    PACKET_QDisableRandomization },
  { "QAgent", PACKET_DISABLE, remote_supported_packet, PACKET_QAgent},
  { "QTBuffer:size", PACKET_DISABLE,
    remote_supported_packet, PACKET_QTBuffer_size},
  { "tracenz", PACKET_DISABLE, remote_supported_packet, PACKET_tracenz_feature },
  { "Qbtrace:off", PACKET_DISABLE, remote_supported_packet, PACKET_Qbtrace_off },
  { "Qbtrace:bts", PACKET_DISABLE, remote_supported_packet, PACKET_Qbtrace_bts },
  { "Qbtrace:pt", PACKET_DISABLE, remote_supported_packet, PACKET_Qbtrace_pt },
  { "qXfer:btrace:read", PACKET_DISABLE, remote_supported_packet,
    PACKET_qXfer_btrace },
  { "qXfer:btrace-conf:read", PACKET_DISABLE, remote_supported_packet,
    PACKET_qXfer_btrace_conf },
  { "Qbtrace-conf:bts:size", PACKET_DISABLE, remote_supported_packet,
    PACKET_Qbtrace_conf_bts_size },
  { "swbreak", PACKET_DISABLE, remote_supported_packet, PACKET_swbreak_feature },
  { "hwbreak", PACKET_DISABLE, remote_supported_packet, PACKET_hwbreak_feature },
  { "fork-events", PACKET_DISABLE, remote_supported_packet,
    PACKET_fork_event_feature },
  { "vfork-events", PACKET_DISABLE, remote_supported_packet,
    PACKET_vfork_event_feature },
  { "exec-events", PACKET_DISABLE, remote_supported_packet,
    PACKET_exec_event_feature },
  { "Qbtrace-conf:pt:size", PACKET_DISABLE, remote_supported_packet,
    PACKET_Qbtrace_conf_pt_size },
  { "vContSupported", PACKET_DISABLE, remote_supported_packet, PACKET_vContSupported },
  { "QThreadEvents", PACKET_DISABLE, remote_supported_packet, PACKET_QThreadEvents },
  { "QThreadOptions", PACKET_DISABLE, remote_supported_thread_options,
    PACKET_QThreadOptions },
  { "no-resumed", PACKET_DISABLE, remote_supported_packet, PACKET_no_resumed },
  { "memory-tagging", PACKET_DISABLE, remote_supported_packet,
    PACKET_memory_tagging_feature },
};

static char *remote_support_xml;

/* Register string appended to "xmlRegisters=" in qSupported query.  */

void
register_remote_support_xml (const char *xml)
{
#if defined(HAVE_LIBEXPAT)
  if (remote_support_xml == NULL)
    remote_support_xml = concat ("xmlRegisters=", xml, (char *) NULL);
  else
    {
      char *copy = xstrdup (remote_support_xml + 13);
      char *saveptr;
      char *p = strtok_r (copy, ",", &saveptr);

      do
	{
	  if (strcmp (p, xml) == 0)
	    {
	      /* already there */
	      xfree (copy);
	      return;
	    }
	}
      while ((p = strtok_r (NULL, ",", &saveptr)) != NULL);
      xfree (copy);

      remote_support_xml = reconcat (remote_support_xml,
				     remote_support_xml, ",", xml,
				     (char *) NULL);
    }
#endif
}

static void
remote_query_supported_append (std::string *msg, const char *append)
{
  if (!msg->empty ())
    msg->append (";");
  msg->append (append);
}

void
remote_target::remote_query_supported ()
{
  struct remote_state *rs = get_remote_state ();
  char *next;
  int i;
  unsigned char seen [ARRAY_SIZE (remote_protocol_features)];

  /* The packet support flags are handled differently for this packet
     than for most others.  We treat an error, a disabled packet, and
     an empty response identically: any features which must be reported
     to be used will be automatically disabled.  An empty buffer
     accomplishes this, since that is also the representation for a list
     containing no features.  */

  rs->buf[0] = 0;
  if (m_features.packet_support (PACKET_qSupported) != PACKET_DISABLE)
    {
      std::string q;

      if (m_features.packet_set_cmd_state (PACKET_multiprocess_feature)
	  != AUTO_BOOLEAN_FALSE)
	remote_query_supported_append (&q, "multiprocess+");

      if (m_features.packet_set_cmd_state (PACKET_swbreak_feature)
	  != AUTO_BOOLEAN_FALSE)
	remote_query_supported_append (&q, "swbreak+");

      if (m_features.packet_set_cmd_state (PACKET_hwbreak_feature)
	  != AUTO_BOOLEAN_FALSE)
	remote_query_supported_append (&q, "hwbreak+");

      remote_query_supported_append (&q, "qRelocInsn+");

      if (m_features.packet_set_cmd_state (PACKET_fork_event_feature)
	  != AUTO_BOOLEAN_FALSE)
	remote_query_supported_append (&q, "fork-events+");

      if (m_features.packet_set_cmd_state (PACKET_vfork_event_feature)
	  != AUTO_BOOLEAN_FALSE)
	remote_query_supported_append (&q, "vfork-events+");

      if (m_features.packet_set_cmd_state (PACKET_exec_event_feature)
	  != AUTO_BOOLEAN_FALSE)
	remote_query_supported_append (&q, "exec-events+");

      if (m_features.packet_set_cmd_state (PACKET_vContSupported)
	  != AUTO_BOOLEAN_FALSE)
	remote_query_supported_append (&q, "vContSupported+");

      if (m_features.packet_set_cmd_state (PACKET_QThreadEvents)
	  != AUTO_BOOLEAN_FALSE)
	remote_query_supported_append (&q, "QThreadEvents+");

      if (m_features.packet_set_cmd_state (PACKET_QThreadOptions)
	  != AUTO_BOOLEAN_FALSE)
	remote_query_supported_append (&q, "QThreadOptions+");

      if (m_features.packet_set_cmd_state (PACKET_no_resumed)
	  != AUTO_BOOLEAN_FALSE)
	remote_query_supported_append (&q, "no-resumed+");

      if (m_features.packet_set_cmd_state (PACKET_memory_tagging_feature)
	  != AUTO_BOOLEAN_FALSE)
	remote_query_supported_append (&q, "memory-tagging+");

      /* Keep this one last to work around a gdbserver <= 7.10 bug in
	 the qSupported:xmlRegisters=i386 handling.  */
      if (remote_support_xml != NULL
	  && (m_features.packet_support (PACKET_qXfer_features)
	      != PACKET_DISABLE))
	remote_query_supported_append (&q, remote_support_xml);

      q = "qSupported:" + q;
      putpkt (q.c_str ());

      getpkt (&rs->buf);

      /* If an error occurred, warn, but do not return - just reset the
	 buffer to empty and go on to disable features.  */
      if (m_features.packet_ok (rs->buf, PACKET_qSupported) == PACKET_ERROR)
	{
	  warning (_("Remote failure reply: %s"), rs->buf.data ());
	  rs->buf[0] = 0;
	}
    }

  memset (seen, 0, sizeof (seen));

  next = rs->buf.data ();
  while (*next)
    {
      enum packet_support is_supported;
      char *p, *end, *name_end, *value;

      /* First separate out this item from the rest of the packet.  If
	 there's another item after this, we overwrite the separator
	 (terminated strings are much easier to work with).  */
      p = next;
      end = strchr (p, ';');
      if (end == NULL)
	{
	  end = p + strlen (p);
	  next = end;
	}
      else
	{
	  *end = '\0';
	  next = end + 1;

	  if (end == p)
	    {
	      warning (_("empty item in \"qSupported\" response"));
	      continue;
	    }
	}

      name_end = strchr (p, '=');
      if (name_end)
	{
	  /* This is a name=value entry.  */
	  is_supported = PACKET_ENABLE;
	  value = name_end + 1;
	  *name_end = '\0';
	}
      else
	{
	  value = NULL;
	  switch (end[-1])
	    {
	    case '+':
	      is_supported = PACKET_ENABLE;
	      break;

	    case '-':
	      is_supported = PACKET_DISABLE;
	      break;

	    case '?':
	      is_supported = PACKET_SUPPORT_UNKNOWN;
	      break;

	    default:
	      warning (_("unrecognized item \"%s\" "
			 "in \"qSupported\" response"), p);
	      continue;
	    }
	  end[-1] = '\0';
	}

      for (i = 0; i < ARRAY_SIZE (remote_protocol_features); i++)
	if (strcmp (remote_protocol_features[i].name, p) == 0)
	  {
	    const struct protocol_feature *feature;

	    seen[i] = 1;
	    feature = &remote_protocol_features[i];
	    feature->func (this, feature, is_supported, value);
	    break;
	  }
    }

  /* If we increased the packet size, make sure to increase the global
     buffer size also.  We delay this until after parsing the entire
     qSupported packet, because this is the same buffer we were
     parsing.  */
  if (rs->buf.size () < rs->explicit_packet_size)
    rs->buf.resize (rs->explicit_packet_size);

  /* Handle the defaults for unmentioned features.  */
  for (i = 0; i < ARRAY_SIZE (remote_protocol_features); i++)
    if (!seen[i])
      {
	const struct protocol_feature *feature;

	feature = &remote_protocol_features[i];
	feature->func (this, feature, feature->default_support, NULL);
      }
}

/* Serial QUIT handler for the remote serial descriptor.

   Defers handling a Ctrl-C until we're done with the current
   command/response packet sequence, unless:

   - We're setting up the connection.  Don't send a remote interrupt
     request, as we're not fully synced yet.  Quit immediately
     instead.

   - The target has been resumed in the foreground
     (target_terminal::is_ours is false) with a synchronous resume
     packet, and we're blocked waiting for the stop reply, thus a
     Ctrl-C should be immediately sent to the target.

   - We get a second Ctrl-C while still within the same serial read or
     write.  In that case the serial is seemingly wedged --- offer to
     quit/disconnect.

   - We see a second Ctrl-C without target response, after having
     previously interrupted the target.  In that case the target/stub
     is probably wedged --- offer to quit/disconnect.
*/

void
remote_target::remote_serial_quit_handler ()
{
  struct remote_state *rs = get_remote_state ();

  if (check_quit_flag ())
    {
      /* If we're starting up, we're not fully synced yet.  Quit
	 immediately.  */
      if (rs->starting_up)
	quit ();
      else if (rs->got_ctrlc_during_io)
	{
	  if (query (_("The target is not responding to GDB commands.\n"
		       "Stop debugging it? ")))
	    remote_unpush_and_throw (this);
	}
      /* If ^C has already been sent once, offer to disconnect.  */
      else if (!target_terminal::is_ours () && rs->ctrlc_pending_p)
	interrupt_query ();
      /* All-stop protocol, and blocked waiting for stop reply.  Send
	 an interrupt request.  */
      else if (!target_terminal::is_ours () && rs->waiting_for_stop_reply)
	target_interrupt ();
      else
	rs->got_ctrlc_during_io = 1;
    }
}

/* The remote_target that is current while the quit handler is
   overridden with remote_serial_quit_handler.  */
static remote_target *curr_quit_handler_target;

static void
remote_serial_quit_handler ()
{
  curr_quit_handler_target->remote_serial_quit_handler ();
}

/* Remove the remote target from the target stack of each inferior
   that is using it.  Upper targets depend on it so remove them
   first.  */

static void
remote_unpush_target (remote_target *target)
{
  /* We have to unpush the target from all inferiors, even those that
     aren't running.  */
  scoped_restore_current_inferior restore_current_inferior;

  for (inferior *inf : all_inferiors (target))
    {
      switch_to_inferior_no_thread (inf);
      inf->pop_all_targets_at_and_above (process_stratum);
      generic_mourn_inferior ();
    }

  /* Don't rely on target_close doing this when the target is popped
     from the last remote inferior above, because something may be
     holding a reference to the target higher up on the stack, meaning
     target_close won't be called yet.  We lost the connection to the
     target, so clear these now, otherwise we may later throw
     TARGET_CLOSE_ERROR while trying to tell the remote target to
     close the file.  */
  fileio_handles_invalidate_target (target);
}

static void
remote_unpush_and_throw (remote_target *target)
{
  remote_unpush_target (target);
  throw_error (TARGET_CLOSE_ERROR, _("Disconnected from target."));
}

void
remote_target::open_1 (const char *name, int from_tty, int extended_p)
{
  remote_target *curr_remote = get_current_remote_target ();

  if (name == 0)
    error (_("To open a remote debug connection, you need to specify what\n"
	   "serial device is attached to the remote system\n"
	   "(e.g. /dev/ttyS0, /dev/ttya, COM1, etc.)."));

  /* If we're connected to a running target, target_preopen will kill it.
     Ask this question first, before target_preopen has a chance to kill
     anything.  */
  if (curr_remote != NULL && !target_has_execution ())
    {
      if (from_tty
	  && !query (_("Already connected to a remote target.  Disconnect? ")))
	error (_("Still connected."));
    }

  /* Here the possibly existing remote target gets unpushed.  */
  target_preopen (from_tty);

  remote_fileio_reset ();
  reopen_exec_file ();
  reread_symbols (from_tty);

  remote_target *remote
    = (extended_p ? new extended_remote_target () : new remote_target ());
  target_ops_up target_holder (remote);

  remote_state *rs = remote->get_remote_state ();

  /* See FIXME above.  */
  if (!target_async_permitted)
    rs->wait_forever_enabled_p = true;

  rs->remote_desc = remote_serial_open (name);

  if (baud_rate != -1)
    {
      try
	{
	  serial_setbaudrate (rs->remote_desc, baud_rate);
	}
      catch (const gdb_exception_error &)
	{
	  /* The requested speed could not be set.  Error out to
	     top level after closing remote_desc.  Take care to
	     set remote_desc to NULL to avoid closing remote_desc
	     more than once.  */
	  serial_close (rs->remote_desc);
	  rs->remote_desc = NULL;
	  throw;
	}
    }

  serial_setparity (rs->remote_desc, serial_parity);
  serial_raw (rs->remote_desc);

  /* If there is something sitting in the buffer we might take it as a
     response to a command, which would be bad.  */
  serial_flush_input (rs->remote_desc);

  if (from_tty)
    {
      gdb_puts ("Remote debugging using ");
      gdb_puts (name);
      gdb_puts ("\n");
    }

  /* Switch to using the remote target now.  */
  current_inferior ()->push_target (std::move (target_holder));

  /* Register extra event sources in the event loop.  */
  rs->create_async_event_handler ();

  rs->notif_state = remote_notif_state_allocate (remote);

  /* Reset the target state; these things will be queried either by
     remote_query_supported or as they are needed.  */
  remote->m_features.reset_all_packet_configs_support ();
  rs->explicit_packet_size = 0;
  rs->noack_mode = 0;
  rs->extended = extended_p;
  rs->waiting_for_stop_reply = 0;
  rs->ctrlc_pending_p = 0;
  rs->got_ctrlc_during_io = 0;

  rs->general_thread = not_sent_ptid;
  rs->continue_thread = not_sent_ptid;
  rs->remote_traceframe_number = -1;

  rs->last_resume_exec_dir = EXEC_FORWARD;

  /* Probe for ability to use "ThreadInfo" query, as required.  */
  rs->use_threadinfo_query = 1;
  rs->use_threadextra_query = 1;

  rs->readahead_cache.invalidate ();

  if (target_async_permitted)
    {
      /* FIXME: cagney/1999-09-23: During the initial connection it is
	 assumed that the target is already ready and able to respond to
	 requests.  Unfortunately remote_start_remote() eventually calls
	 wait_for_inferior() with no timeout.  wait_forever_enabled_p gets
	 around this.  Eventually a mechanism that allows
	 wait_for_inferior() to expect/get timeouts will be
	 implemented.  */
      rs->wait_forever_enabled_p = false;
    }

  /* First delete any symbols previously loaded from shared libraries.  */
  no_shared_libraries (NULL, 0);

  /* Start the remote connection.  If error() or QUIT, discard this
     target (we'd otherwise be in an inconsistent state) and then
     propogate the error on up the exception chain.  This ensures that
     the caller doesn't stumble along blindly assuming that the
     function succeeded.  The CLI doesn't have this problem but other
     UI's, such as MI do.

     FIXME: cagney/2002-05-19: Instead of re-throwing the exception,
     this function should return an error indication letting the
     caller restore the previous state.  Unfortunately the command
     ``target remote'' is directly wired to this function making that
     impossible.  On a positive note, the CLI side of this problem has
     been fixed - the function set_cmd_context() makes it possible for
     all the ``target ....'' commands to share a common callback
     function.  See cli-dump.c.  */
  {

    try
      {
	remote->start_remote (from_tty, extended_p);
      }
    catch (const gdb_exception &ex)
      {
	/* Pop the partially set up target - unless something else did
	   already before throwing the exception.  */
	if (ex.error != TARGET_CLOSE_ERROR)
	  remote_unpush_target (remote);
	throw;
      }
  }

  remote_btrace_reset (rs);

  if (target_async_permitted)
    rs->wait_forever_enabled_p = true;
}

/* Determine if WS represents a fork status.  */

static bool
is_fork_status (target_waitkind kind)
{
  return (kind == TARGET_WAITKIND_FORKED
	  || kind == TARGET_WAITKIND_VFORKED);
}

/* Return a reference to the field where a pending child status, if
   there's one, is recorded.  If there's no child event pending, the
   returned waitstatus has TARGET_WAITKIND_IGNORE kind.  */

static const target_waitstatus &
thread_pending_status (struct thread_info *thread)
{
  return (thread->has_pending_waitstatus ()
	  ? thread->pending_waitstatus ()
	  : thread->pending_follow);
}

/* Return THREAD's pending status if it is a pending fork/vfork (but
   not clone) parent, else return nullptr.  */

static const target_waitstatus *
thread_pending_fork_status (struct thread_info *thread)
{
  const target_waitstatus &ws = thread_pending_status (thread);

  if (!is_fork_status (ws.kind ()))
    return nullptr;

  return &ws;
}

/* Return THREAD's pending status if is is a pending fork/vfork/clone
   event, else return nullptr.  */

static const target_waitstatus *
thread_pending_child_status (thread_info *thread)
{
  const target_waitstatus &ws = thread_pending_status (thread);

  if (!is_new_child_status (ws.kind ()))
    return nullptr;

  return &ws;
}

/* Detach the specified process.  */

void
remote_target::remote_detach_pid (int pid)
{
  struct remote_state *rs = get_remote_state ();

  /* This should not be necessary, but the handling for D;PID in
     GDBserver versions prior to 8.2 incorrectly assumes that the
     selected process points to the same process we're detaching,
     leading to misbehavior (and possibly GDBserver crashing) when it
     does not.  Since it's easy and cheap, work around it by forcing
     GDBserver to select GDB's current process.  */
  set_general_process ();

  if (m_features.remote_multi_process_p ())
    xsnprintf (rs->buf.data (), get_remote_packet_size (), "D;%x", pid);
  else
    strcpy (rs->buf.data (), "D");

  putpkt (rs->buf);
  getpkt (&rs->buf);

  if (rs->buf[0] == 'O' && rs->buf[1] == 'K')
    ;
  else if (rs->buf[0] == '\0')
    error (_("Remote doesn't know how to detach"));
  else
    {
      /* It is possible that we have an unprocessed exit event for this
	 pid.  If this is the case then we can ignore the failure to detach
	 and just pretend that the detach worked, as far as the user is
	 concerned, the process exited immediately after the detach.  */
      bool process_has_already_exited = false;
      remote_notif_get_pending_events (&notif_client_stop);
      for (stop_reply_up &reply : rs->stop_reply_queue)
	{
	  if (reply->ptid.pid () != pid)
	    continue;

	  enum target_waitkind kind = reply->ws.kind ();
	  if (kind == TARGET_WAITKIND_EXITED
	      || kind == TARGET_WAITKIND_SIGNALLED)
	    {
	      process_has_already_exited = true;
	      remote_debug_printf
		("detach failed, but process already exited");
	      break;
	    }
	}

      if (!process_has_already_exited)
	error (_("can't detach process: %s"), (char *) rs->buf.data ());
    }
}

/* This detaches a program to which we previously attached, using
   inferior_ptid to identify the process.  After this is done, GDB
   can be used to debug some other program.  We better not have left
   any breakpoints in the target program or it'll die when it hits
   one.  */

void
remote_target::remote_detach_1 (inferior *inf, int from_tty)
{
  int pid = inferior_ptid.pid ();
  struct remote_state *rs = get_remote_state ();
  int is_fork_parent;

  if (!target_has_execution ())
    error (_("No process to detach from."));

  target_announce_detach (from_tty);

  if (!gdbarch_has_global_breakpoints (current_inferior ()->arch ()))
    {
      /* If we're in breakpoints-always-inserted mode, or the inferior
	 is running, we have to remove breakpoints before detaching.
	 We don't do this in common code instead because not all
	 targets support removing breakpoints while the target is
	 running.  The remote target / gdbserver does, though.  */
      remove_breakpoints_inf (current_inferior ());
    }

  /* Tell the remote target to detach.  */
  remote_detach_pid (pid);

  /* Exit only if this is the only active inferior.  */
  if (from_tty && !rs->extended && number_of_live_inferiors (this) == 1)
    gdb_puts (_("Ending remote debugging.\n"));

  /* See if any thread of the inferior we are detaching has a pending fork
     status.  In that case, we must detach from the child resulting from
     that fork.  */
  for (thread_info *thread : inf->non_exited_threads ())
    {
      const target_waitstatus *ws = thread_pending_fork_status (thread);

      if (ws == nullptr)
	continue;

      remote_detach_pid (ws->child_ptid ().pid ());
    }

  /* Check also for any pending fork events in the stop reply queue.  */
  remote_notif_get_pending_events (&notif_client_stop);
  for (stop_reply_up &reply : rs->stop_reply_queue)
    {
      if (reply->ptid.pid () != pid)
	continue;

      if (!is_fork_status (reply->ws.kind ()))
	continue;

      remote_detach_pid (reply->ws.child_ptid ().pid ());
    }

  thread_info *tp = this->find_thread (inferior_ptid);

  /* Check to see if we are detaching a fork parent.  Note that if we
     are detaching a fork child, tp == NULL.  */
  is_fork_parent = (tp != NULL
		    && tp->pending_follow.kind () == TARGET_WAITKIND_FORKED);

  /* If doing detach-on-fork, we don't mourn, because that will delete
     breakpoints that should be available for the followed inferior.  */
  if (!is_fork_parent)
    {
      /* Save the pid as a string before mourning, since that will
	 unpush the remote target, and we need the string after.  */
      std::string infpid = target_pid_to_str (ptid_t (pid));

      target_mourn_inferior (inferior_ptid);
      if (print_inferior_events)
	gdb_printf (_("[Inferior %d (%s) detached]\n"),
		    inf->num, infpid.c_str ());
    }
  else
    {
      switch_to_no_thread ();
      detach_inferior (current_inferior ());
    }
}

void
remote_target::detach (inferior *inf, int from_tty)
{
  remote_detach_1 (inf, from_tty);
}

void
extended_remote_target::detach (inferior *inf, int from_tty)
{
  remote_detach_1 (inf, from_tty);
}

/* Target follow-fork function for remote targets.  On entry, and
   at return, the current inferior is the fork parent.

   Note that although this is currently only used for extended-remote,
   it is named remote_follow_fork in anticipation of using it for the
   remote target as well.  */

void
remote_target::follow_fork (inferior *child_inf, ptid_t child_ptid,
			    target_waitkind fork_kind, bool follow_child,
			    bool detach_fork)
{
  process_stratum_target::follow_fork (child_inf, child_ptid,
				       fork_kind, follow_child, detach_fork);

  if ((fork_kind == TARGET_WAITKIND_FORKED
       && m_features.remote_fork_event_p ())
      || (fork_kind == TARGET_WAITKIND_VFORKED
	  && m_features.remote_vfork_event_p ()))
    {
      /* When following the parent and detaching the child, we detach
	 the child here.  For the case of following the child and
	 detaching the parent, the detach is done in the target-
	 independent follow fork code in infrun.c.  We can't use
	 target_detach when detaching an unfollowed child because
	 the client side doesn't know anything about the child.  */
      if (detach_fork && !follow_child)
	{
	  /* Detach the fork child.  */
	  remote_detach_pid (child_ptid.pid ());
	}
    }
}

void
remote_target::follow_clone (ptid_t child_ptid)
{
  remote_add_thread (child_ptid, false, false, false);
}

/* Target follow-exec function for remote targets.  Save EXECD_PATHNAME
   in the program space of the new inferior.  */

void
remote_target::follow_exec (inferior *follow_inf, ptid_t ptid,
			    const char *execd_pathname)
{
  process_stratum_target::follow_exec (follow_inf, ptid, execd_pathname);

  /* We know that this is a target file name, so if it has the "target:"
     prefix we strip it off before saving it in the program space.  */
  if (is_target_filename (execd_pathname))
    execd_pathname += strlen (TARGET_SYSROOT_PREFIX);

  set_pspace_remote_exec_file (follow_inf->pspace, execd_pathname);
}

/* Same as remote_detach, but don't send the "D" packet; just disconnect.  */

void
remote_target::disconnect (const char *args, int from_tty)
{
  if (args)
    error (_("Argument given to \"disconnect\" when remotely debugging."));

  /* Make sure we unpush even the extended remote targets.  Calling
     target_mourn_inferior won't unpush, and
     remote_target::mourn_inferior won't unpush if there is more than
     one inferior left.  */
  remote_unpush_target (this);

  if (from_tty)
    gdb_puts ("Ending remote debugging.\n");
}

/* Attach to the process specified by ARGS.  If FROM_TTY is non-zero,
   be chatty about it.  */

void
extended_remote_target::attach (const char *args, int from_tty)
{
  struct remote_state *rs = get_remote_state ();
  int pid;
  char *wait_status = NULL;

  pid = parse_pid_to_attach (args);

  /* Remote PID can be freely equal to getpid, do not check it here the same
     way as in other targets.  */

  if (m_features.packet_support (PACKET_vAttach) == PACKET_DISABLE)
    error (_("This target does not support attaching to a process"));

  target_announce_attach (from_tty, pid);

  xsnprintf (rs->buf.data (), get_remote_packet_size (), "vAttach;%x", pid);
  putpkt (rs->buf);
  getpkt (&rs->buf);

  switch (m_features.packet_ok (rs->buf, PACKET_vAttach))
    {
    case PACKET_OK:
      if (!target_is_non_stop_p ())
	{
	  /* Save the reply for later.  */
	  wait_status = (char *) alloca (strlen (rs->buf.data ()) + 1);
	  strcpy (wait_status, rs->buf.data ());
	}
      else if (strcmp (rs->buf.data (), "OK") != 0)
	error (_("Attaching to %s failed with: %s"),
	       target_pid_to_str (ptid_t (pid)).c_str (),
	       rs->buf.data ());
      break;
    case PACKET_UNKNOWN:
      error (_("This target does not support attaching to a process"));
    default:
      error (_("Attaching to %s failed"),
	     target_pid_to_str (ptid_t (pid)).c_str ());
    }

  switch_to_inferior_no_thread (remote_add_inferior (false, pid, 1, 0));

  inferior_ptid = ptid_t (pid);

  if (target_is_non_stop_p ())
    {
      /* Get list of threads.  */
      update_thread_list ();

      thread_info *thread = first_thread_of_inferior (current_inferior ());
      if (thread != nullptr)
	switch_to_thread (thread);

      /* Invalidate our notion of the remote current thread.  */
      record_currthread (rs, minus_one_ptid);
    }
  else
    {
      /* Now, if we have thread information, update the main thread's
	 ptid.  */
      ptid_t curr_ptid = remote_current_thread (ptid_t (pid));

      /* Add the main thread to the thread list.  We add the thread
	 silently in this case (the final true parameter).  */
      thread_info *thr = remote_add_thread (curr_ptid, true, true, true);

      switch_to_thread (thr);
    }

  /* Next, if the target can specify a description, read it.  We do
     this before anything involving memory or registers.  */
  target_find_description ();

  if (!target_is_non_stop_p ())
    {
      /* Use the previously fetched status.  */
      gdb_assert (wait_status != NULL);

      notif_event_up reply
	= remote_notif_parse (this, &notif_client_stop, wait_status);
      push_stop_reply (as_stop_reply_up (std::move (reply)));
    }
  else
    {
      gdb_assert (wait_status == NULL);

      gdb_assert (target_can_async_p ());
    }
}

/* Implementation of the to_post_attach method.  */

void
extended_remote_target::post_attach (int pid)
{
  /* Get text, data & bss offsets.  */
  get_offsets ();

  /* In certain cases GDB might not have had the chance to start
     symbol lookup up until now.  This could happen if the debugged
     binary is not using shared libraries, the vsyscall page is not
     present (on Linux) and the binary itself hadn't changed since the
     debugging process was started.  */
  if (current_program_space->symfile_object_file != NULL)
    remote_check_symbols();
}


/* Check for the availability of vCont.  This function should also check
   the response.  */

void
remote_target::remote_vcont_probe ()
{
  remote_state *rs = get_remote_state ();
  char *buf;

  strcpy (rs->buf.data (), "vCont?");
  putpkt (rs->buf);
  getpkt (&rs->buf);
  buf = rs->buf.data ();

  /* Make sure that the features we assume are supported.  */
  if (startswith (buf, "vCont"))
    {
      char *p = &buf[5];
      int support_c, support_C;

      rs->supports_vCont.s = 0;
      rs->supports_vCont.S = 0;
      support_c = 0;
      support_C = 0;
      rs->supports_vCont.t = 0;
      rs->supports_vCont.r = 0;
      while (p && *p == ';')
	{
	  p++;
	  if (*p == 's' && (*(p + 1) == ';' || *(p + 1) == 0))
	    rs->supports_vCont.s = 1;
	  else if (*p == 'S' && (*(p + 1) == ';' || *(p + 1) == 0))
	    rs->supports_vCont.S = 1;
	  else if (*p == 'c' && (*(p + 1) == ';' || *(p + 1) == 0))
	    support_c = 1;
	  else if (*p == 'C' && (*(p + 1) == ';' || *(p + 1) == 0))
	    support_C = 1;
	  else if (*p == 't' && (*(p + 1) == ';' || *(p + 1) == 0))
	    rs->supports_vCont.t = 1;
	  else if (*p == 'r' && (*(p + 1) == ';' || *(p + 1) == 0))
	    rs->supports_vCont.r = 1;

	  p = strchr (p, ';');
	}

      /* If c, and C are not all supported, we can't use vCont.  Clearing
	 BUF will make packet_ok disable the packet.  */
      if (!support_c || !support_C)
	buf[0] = 0;
    }

  m_features.packet_ok (rs->buf, PACKET_vCont);
}

/* Helper function for building "vCont" resumptions.  Write a
   resumption to P.  ENDP points to one-passed-the-end of the buffer
   we're allowed to write to.  Returns BUF+CHARACTERS_WRITTEN.  The
   thread to be resumed is PTID; STEP and SIGGNAL indicate whether the
   resumed thread should be single-stepped and/or signalled.  If PTID
   equals minus_one_ptid, then all threads are resumed; if PTID
   represents a process, then all threads of the process are
   resumed.  */

char *
remote_target::append_resumption (char *p, char *endp,
				  ptid_t ptid, int step, gdb_signal siggnal)
{
  struct remote_state *rs = get_remote_state ();

  if (step && siggnal != GDB_SIGNAL_0)
    p += xsnprintf (p, endp - p, ";S%02x", siggnal);
  else if (step
	   /* GDB is willing to range step.  */
	   && use_range_stepping
	   /* Target supports range stepping.  */
	   && rs->supports_vCont.r
	   /* We don't currently support range stepping multiple
	      threads with a wildcard (though the protocol allows it,
	      so stubs shouldn't make an active effort to forbid
	      it).  */
	   && !(m_features.remote_multi_process_p () && ptid.is_pid ()))
    {
      struct thread_info *tp;

      if (ptid == minus_one_ptid)
	{
	  /* If we don't know about the target thread's tid, then
	     we're resuming magic_null_ptid (see caller).  */
	  tp = this->find_thread (magic_null_ptid);
	}
      else
	tp = this->find_thread (ptid);
      gdb_assert (tp != NULL);

      if (tp->control.may_range_step)
	{
	  int addr_size = gdbarch_addr_bit (current_inferior ()->arch ()) / 8;

	  p += xsnprintf (p, endp - p, ";r%s,%s",
			  phex_nz (tp->control.step_range_start,
				   addr_size),
			  phex_nz (tp->control.step_range_end,
				   addr_size));
	}
      else
	p += xsnprintf (p, endp - p, ";s");
    }
  else if (step)
    p += xsnprintf (p, endp - p, ";s");
  else if (siggnal != GDB_SIGNAL_0)
    p += xsnprintf (p, endp - p, ";C%02x", siggnal);
  else
    p += xsnprintf (p, endp - p, ";c");

  if (m_features.remote_multi_process_p () && ptid.is_pid ())
    {
      ptid_t nptid;

      /* All (-1) threads of process.  */
      nptid = ptid_t (ptid.pid (), -1);

      p += xsnprintf (p, endp - p, ":");
      p = write_ptid (p, endp, nptid);
    }
  else if (ptid != minus_one_ptid)
    {
      p += xsnprintf (p, endp - p, ":");
      p = write_ptid (p, endp, ptid);
    }

  return p;
}

/* Clear the thread's private info on resume.  */

static void
resume_clear_thread_private_info (struct thread_info *thread)
{
  if (thread->priv != NULL)
    {
      remote_thread_info *priv = get_remote_thread_info (thread);

      priv->stop_reason = TARGET_STOPPED_BY_NO_REASON;
      priv->watch_data_address = 0;
    }
}

/* Append a vCont continue-with-signal action for threads that have a
   non-zero stop signal.  */

char *
remote_target::append_pending_thread_resumptions (char *p, char *endp,
						  ptid_t ptid)
{
  for (thread_info *thread : all_non_exited_threads (this, ptid))
    if (inferior_ptid != thread->ptid
	&& thread->stop_signal () != GDB_SIGNAL_0)
      {
	p = append_resumption (p, endp, thread->ptid,
			       0, thread->stop_signal ());
	thread->set_stop_signal (GDB_SIGNAL_0);
	resume_clear_thread_private_info (thread);
      }

  return p;
}

/* Set the target running, using the packets that use Hc
   (c/s/C/S).  */

void
remote_target::remote_resume_with_hc (ptid_t ptid, int step,
				      gdb_signal siggnal)
{
  struct remote_state *rs = get_remote_state ();
  char *buf;

  rs->last_sent_signal = siggnal;
  rs->last_sent_step = step;

  /* The c/s/C/S resume packets use Hc, so set the continue
     thread.  */
  if (ptid == minus_one_ptid)
    set_continue_thread (any_thread_ptid);
  else
    set_continue_thread (ptid);

  for (thread_info *thread : all_non_exited_threads (this))
    resume_clear_thread_private_info (thread);

  buf = rs->buf.data ();
  if (::execution_direction == EXEC_REVERSE)
    {
      /* We don't pass signals to the target in reverse exec mode.  */
      if (info_verbose && siggnal != GDB_SIGNAL_0)
	warning (_(" - Can't pass signal %d to target in reverse: ignored."),
		 siggnal);

      if (step && m_features.packet_support (PACKET_bs) == PACKET_DISABLE)
	error (_("Remote reverse-step not supported."));
      if (!step && m_features.packet_support (PACKET_bc) == PACKET_DISABLE)
	error (_("Remote reverse-continue not supported."));

      strcpy (buf, step ? "bs" : "bc");
    }
  else if (siggnal != GDB_SIGNAL_0)
    {
      buf[0] = step ? 'S' : 'C';
      buf[1] = tohex (((int) siggnal >> 4) & 0xf);
      buf[2] = tohex (((int) siggnal) & 0xf);
      buf[3] = '\0';
    }
  else
    strcpy (buf, step ? "s" : "c");

  putpkt (buf);
}

/* Resume the remote inferior by using a "vCont" packet.  SCOPE_PTID,
   STEP, and SIGGNAL have the same meaning as in target_resume.  This
   function returns non-zero iff it resumes the inferior.

   This function issues a strict subset of all possible vCont commands
   at the moment.  */

int
remote_target::remote_resume_with_vcont (ptid_t scope_ptid, int step,
					 enum gdb_signal siggnal)
{
  struct remote_state *rs = get_remote_state ();
  char *p;
  char *endp;

  /* No reverse execution actions defined for vCont.  */
  if (::execution_direction == EXEC_REVERSE)
    return 0;

  if (m_features.packet_support (PACKET_vCont) == PACKET_DISABLE)
    return 0;

  p = rs->buf.data ();
  endp = p + get_remote_packet_size ();

  /* If we could generate a wider range of packets, we'd have to worry
     about overflowing BUF.  Should there be a generic
     "multi-part-packet" packet?  */

  p += xsnprintf (p, endp - p, "vCont");

  if (scope_ptid == magic_null_ptid)
    {
      /* MAGIC_NULL_PTID means that we don't have any active threads,
	 so we don't have any TID numbers the inferior will
	 understand.  Make sure to only send forms that do not specify
	 a TID.  */
      append_resumption (p, endp, minus_one_ptid, step, siggnal);
    }
  else if (scope_ptid == minus_one_ptid || scope_ptid.is_pid ())
    {
      /* Resume all threads (of all processes, or of a single
	 process), with preference for INFERIOR_PTID.  This assumes
	 inferior_ptid belongs to the set of all threads we are about
	 to resume.  */
      if (step || siggnal != GDB_SIGNAL_0)
	{
	  /* Step inferior_ptid, with or without signal.  */
	  p = append_resumption (p, endp, inferior_ptid, step, siggnal);
	}

      /* Also pass down any pending signaled resumption for other
	 threads not the current.  */
      p = append_pending_thread_resumptions (p, endp, scope_ptid);

      /* And continue others without a signal.  */
      append_resumption (p, endp, scope_ptid, /*step=*/ 0, GDB_SIGNAL_0);
    }
  else
    {
      /* Scheduler locking; resume only SCOPE_PTID.  */
      append_resumption (p, endp, scope_ptid, step, siggnal);
    }

  gdb_assert (strlen (rs->buf.data ()) < get_remote_packet_size ());
  putpkt (rs->buf);

  if (target_is_non_stop_p ())
    {
      /* In non-stop, the stub replies to vCont with "OK".  The stop
	 reply will be reported asynchronously by means of a `%Stop'
	 notification.  */
      getpkt (&rs->buf);
      if (strcmp (rs->buf.data (), "OK") != 0)
	error (_("Unexpected vCont reply in non-stop mode: %s"),
	       rs->buf.data ());
    }

  return 1;
}

/* Tell the remote machine to resume.  */

void
remote_target::resume (ptid_t scope_ptid, int step, enum gdb_signal siggnal)
{
  struct remote_state *rs = get_remote_state ();

  /* When connected in non-stop mode, the core resumes threads
     individually.  Resuming remote threads directly in target_resume
     would thus result in sending one packet per thread.  Instead, to
     minimize roundtrip latency, here we just store the resume
     request (put the thread in RESUMED_PENDING_VCONT state); the actual remote
     resumption will be done in remote_target::commit_resume, where we'll be
     able to do vCont action coalescing.  */
  if (target_is_non_stop_p () && ::execution_direction != EXEC_REVERSE)
    {
      remote_thread_info *remote_thr
	= get_remote_thread_info (inferior_thread ());

      /* We don't expect the core to ask to resume an already resumed (from
	 its point of view) thread.  */
      gdb_assert (remote_thr->get_resume_state () == resume_state::NOT_RESUMED);

      remote_thr->set_resumed_pending_vcont (step, siggnal);

      /* There's actually nothing that says that the core can't
	 request a wildcard resume in non-stop mode, though.  It's
	 just that we know it doesn't currently, so we don't bother
	 with it.  */
      gdb_assert (scope_ptid == inferior_ptid);
      return;
    }

  commit_requested_thread_options ();

  /* In all-stop, we can't mark REMOTE_ASYNC_GET_PENDING_EVENTS_TOKEN
     (explained in remote-notif.c:handle_notification) so
     remote_notif_process is not called.  We need find a place where
     it is safe to start a 'vNotif' sequence.  It is good to do it
     before resuming inferior, because inferior was stopped and no RSP
     traffic at that moment.  */
  if (!target_is_non_stop_p ())
    remote_notif_process (rs->notif_state, &notif_client_stop);

  rs->last_resume_exec_dir = ::execution_direction;

  /* Prefer vCont, and fallback to s/c/S/C, which use Hc.  */
  if (!remote_resume_with_vcont (scope_ptid, step, siggnal))
    remote_resume_with_hc (scope_ptid, step, siggnal);

  /* Update resumed state tracked by the remote target.  */
  for (thread_info *tp : all_non_exited_threads (this, scope_ptid))
    get_remote_thread_info (tp)->set_resumed ();

  /* We've just told the target to resume.  The remote server will
     wait for the inferior to stop, and then send a stop reply.  In
     the mean time, we can't start another command/query ourselves
     because the stub wouldn't be ready to process it.  This applies
     only to the base all-stop protocol, however.  In non-stop (which
     only supports vCont), the stub replies with an "OK", and is
     immediate able to process further serial input.  */
  if (!target_is_non_stop_p ())
    rs->waiting_for_stop_reply = 1;
}

/* Private per-inferior info for target remote processes.  */

struct remote_inferior : public private_inferior
{
  /* Whether we can send a wildcard vCont for this process.  */
  bool may_wildcard_vcont = true;
};

/* Get the remote private inferior data associated to INF.  */

static remote_inferior *
get_remote_inferior (inferior *inf)
{
  if (inf->priv == NULL)
    inf->priv.reset (new remote_inferior);

  return gdb::checked_static_cast<remote_inferior *> (inf->priv.get ());
}

/* Class used to track the construction of a vCont packet in the
   outgoing packet buffer.  This is used to send multiple vCont
   packets if we have more actions than would fit a single packet.  */

class vcont_builder
{
public:
  explicit vcont_builder (remote_target *remote)
    : m_remote (remote)
  {
    restart ();
  }

  void flush ();
  void push_action (ptid_t ptid, bool step, gdb_signal siggnal);

private:
  void restart ();

  /* The remote target.  */
  remote_target *m_remote;

  /* Pointer to the first action.  P points here if no action has been
     appended yet.  */
  char *m_first_action;

  /* Where the next action will be appended.  */
  char *m_p;

  /* The end of the buffer.  Must never write past this.  */
  char *m_endp;
};

/* Prepare the outgoing buffer for a new vCont packet.  */

void
vcont_builder::restart ()
{
  struct remote_state *rs = m_remote->get_remote_state ();

  m_p = rs->buf.data ();
  m_endp = m_p + m_remote->get_remote_packet_size ();
  m_p += xsnprintf (m_p, m_endp - m_p, "vCont");
  m_first_action = m_p;
}

/* If the vCont packet being built has any action, send it to the
   remote end.  */

void
vcont_builder::flush ()
{
  struct remote_state *rs;

  if (m_p == m_first_action)
    return;

  rs = m_remote->get_remote_state ();
  m_remote->putpkt (rs->buf);
  m_remote->getpkt (&rs->buf);
  if (strcmp (rs->buf.data (), "OK") != 0)
    error (_("Unexpected vCont reply in non-stop mode: %s"), rs->buf.data ());
}

/* The largest action is range-stepping, with its two addresses.  This
   is more than sufficient.  If a new, bigger action is created, it'll
   quickly trigger a failed assertion in append_resumption (and we'll
   just bump this).  */
#define MAX_ACTION_SIZE 200

/* Append a new vCont action in the outgoing packet being built.  If
   the action doesn't fit the packet along with previous actions, push
   what we've got so far to the remote end and start over a new vCont
   packet (with the new action).  */

void
vcont_builder::push_action (ptid_t ptid, bool step, gdb_signal siggnal)
{
  char buf[MAX_ACTION_SIZE + 1];

  char *endp = m_remote->append_resumption (buf, buf + sizeof (buf),
					    ptid, step, siggnal);

  /* Check whether this new action would fit in the vCont packet along
     with previous actions.  If not, send what we've got so far and
     start a new vCont packet.  */
  size_t rsize = endp - buf;
  if (rsize > m_endp - m_p)
    {
      flush ();
      restart ();

      /* Should now fit.  */
      gdb_assert (rsize <= m_endp - m_p);
    }

  memcpy (m_p, buf, rsize);
  m_p += rsize;
  *m_p = '\0';
}

/* to_commit_resume implementation.  */

void
remote_target::commit_resumed ()
{
  /* If connected in all-stop mode, we'd send the remote resume
     request directly from remote_resume.  Likewise if
     reverse-debugging, as there are no defined vCont actions for
     reverse execution.  */
  if (!target_is_non_stop_p () || ::execution_direction == EXEC_REVERSE)
    return;

  commit_requested_thread_options ();

  /* Try to send wildcard actions ("vCont;c" or "vCont;c:pPID.-1")
     instead of resuming all threads of each process individually.
     However, if any thread of a process must remain halted, we can't
     send wildcard resumes and must send one action per thread.

     Care must be taken to not resume threads/processes the server
     side already told us are stopped, but the core doesn't know about
     yet, because the events are still in the vStopped notification
     queue.  For example:

       #1 => vCont s:p1.1;c
       #2 <= OK
       #3 <= %Stopped T05 p1.1
       #4 => vStopped
       #5 <= T05 p1.2
       #6 => vStopped
       #7 <= OK
       #8 (infrun handles the stop for p1.1 and continues stepping)
       #9 => vCont s:p1.1;c

     The last vCont above would resume thread p1.2 by mistake, because
     the server has no idea that the event for p1.2 had not been
     handled yet.

     The server side must similarly ignore resume actions for the
     thread that has a pending %Stopped notification (and any other
     threads with events pending), until GDB acks the notification
     with vStopped.  Otherwise, e.g., the following case is
     mishandled:

       #1 => g  (or any other packet)
       #2 <= [registers]
       #3 <= %Stopped T05 p1.2
       #4 => vCont s:p1.1;c
       #5 <= OK

     Above, the server must not resume thread p1.2.  GDB can't know
     that p1.2 stopped until it acks the %Stopped notification, and
     since from GDB's perspective all threads should be running, it
     sends a "c" action.

     Finally, special care must also be given to handling fork/vfork
     events.  A (v)fork event actually tells us that two processes
     stopped -- the parent and the child.  Until we follow the fork,
     we must not resume the child.  Therefore, if we have a pending
     fork follow, we must not send a global wildcard resume action
     (vCont;c).  We can still send process-wide wildcards though.  */

  /* Start by assuming a global wildcard (vCont;c) is possible.  */
  bool may_global_wildcard_vcont = true;

  /* And assume every process is individually wildcard-able too.  */
  for (inferior *inf : all_non_exited_inferiors (this))
    {
      remote_inferior *priv = get_remote_inferior (inf);

      priv->may_wildcard_vcont = true;
    }

  /* Check for any pending events (not reported or processed yet) and
     disable process and global wildcard resumes appropriately.  */
  check_pending_events_prevent_wildcard_vcont (&may_global_wildcard_vcont);

  bool any_pending_vcont_resume = false;

  for (thread_info *tp : all_non_exited_threads (this))
    {
      remote_thread_info *priv = get_remote_thread_info (tp);

      /* If a thread of a process is not meant to be resumed, then we
	 can't wildcard that process.  */
      if (priv->get_resume_state () == resume_state::NOT_RESUMED)
	{
	  get_remote_inferior (tp->inf)->may_wildcard_vcont = false;

	  /* And if we can't wildcard a process, we can't wildcard
	     everything either.  */
	  may_global_wildcard_vcont = false;
	  continue;
	}

      if (priv->get_resume_state () == resume_state::RESUMED_PENDING_VCONT)
	any_pending_vcont_resume = true;

      /* If a thread is the parent of an unfollowed fork/vfork/clone,
	 then we can't do a global wildcard, as that would resume the
	 pending child.  */
      if (thread_pending_child_status (tp) != nullptr)
	may_global_wildcard_vcont = false;
    }

  /* We didn't have any resumed thread pending a vCont resume, so nothing to
     do.  */
  if (!any_pending_vcont_resume)
    return;

  /* Now let's build the vCont packet(s).  Actions must be appended
     from narrower to wider scopes (thread -> process -> global).  If
     we end up with too many actions for a single packet vcont_builder
     flushes the current vCont packet to the remote side and starts a
     new one.  */
  struct vcont_builder vcont_builder (this);

  /* Threads first.  */
  for (thread_info *tp : all_non_exited_threads (this))
    {
      remote_thread_info *remote_thr = get_remote_thread_info (tp);

      /* If the thread was previously vCont-resumed, no need to send a specific
	 action for it.  If we didn't receive a resume request for it, don't
	 send an action for it either.  */
      if (remote_thr->get_resume_state () != resume_state::RESUMED_PENDING_VCONT)
	continue;

      gdb_assert (!thread_is_in_step_over_chain (tp));

      /* We should never be commit-resuming a thread that has a stop reply.
	 Otherwise, we would end up reporting a stop event for a thread while
	 it is running on the remote target.  */
      remote_state *rs = get_remote_state ();
      for (const auto &stop_reply : rs->stop_reply_queue)
	gdb_assert (stop_reply->ptid != tp->ptid);

      const resumed_pending_vcont_info &info
	= remote_thr->resumed_pending_vcont_info ();

      /* Check if we need to send a specific action for this thread.  If not,
	 it will be included in a wildcard resume instead.  */
      if (info.step || info.sig != GDB_SIGNAL_0
	  || !get_remote_inferior (tp->inf)->may_wildcard_vcont)
	vcont_builder.push_action (tp->ptid, info.step, info.sig);

      remote_thr->set_resumed ();
    }

  /* Now check whether we can send any process-wide wildcard.  This is
     to avoid sending a global wildcard in the case nothing is
     supposed to be resumed.  */
  bool any_process_wildcard = false;

  for (inferior *inf : all_non_exited_inferiors (this))
    {
      if (get_remote_inferior (inf)->may_wildcard_vcont)
	{
	  any_process_wildcard = true;
	  break;
	}
    }

  if (any_process_wildcard)
    {
      /* If all processes are wildcard-able, then send a single "c"
	 action, otherwise, send an "all (-1) threads of process"
	 continue action for each running process, if any.  */
      if (may_global_wildcard_vcont)
	{
	  vcont_builder.push_action (minus_one_ptid,
				     false, GDB_SIGNAL_0);
	}
      else
	{
	  for (inferior *inf : all_non_exited_inferiors (this))
	    {
	      if (get_remote_inferior (inf)->may_wildcard_vcont)
		{
		  vcont_builder.push_action (ptid_t (inf->pid),
					     false, GDB_SIGNAL_0);
		}
	    }
	}
    }

  vcont_builder.flush ();
}

/* Implementation of target_has_pending_events.  */

bool
remote_target::has_pending_events ()
{
  if (target_can_async_p ())
    {
      remote_state *rs = get_remote_state ();

      if (rs->async_event_handler_marked ())
	return true;

      /* Note that BUFCNT can be negative, indicating sticky
	 error.  */
      if (rs->remote_desc->bufcnt != 0)
	return true;
    }
  return false;
}



/* Non-stop version of target_stop.  Uses `vCont;t' to stop a remote
   thread, all threads of a remote process, or all threads of all
   processes.  */

void
remote_target::remote_stop_ns (ptid_t ptid)
{
  struct remote_state *rs = get_remote_state ();
  char *p = rs->buf.data ();
  char *endp = p + get_remote_packet_size ();

  /* If any thread that needs to stop was resumed but pending a vCont
     resume, generate a phony stop_reply.  However, first check
     whether the thread wasn't resumed with a signal.  Generating a
     phony stop in that case would result in losing the signal.  */
  bool needs_commit = false;
  for (thread_info *tp : all_non_exited_threads (this, ptid))
    {
      remote_thread_info *remote_thr = get_remote_thread_info (tp);

      if (remote_thr->get_resume_state ()
	  == resume_state::RESUMED_PENDING_VCONT)
	{
	  const resumed_pending_vcont_info &info
	    = remote_thr->resumed_pending_vcont_info ();
	  if (info.sig != GDB_SIGNAL_0)
	    {
	      /* This signal must be forwarded to the inferior.  We
		 could commit-resume just this thread, but its simpler
		 to just commit-resume everything.  */
	      needs_commit = true;
	      break;
	    }
	}
    }

  if (needs_commit)
    commit_resumed ();
  else
    for (thread_info *tp : all_non_exited_threads (this, ptid))
      {
	remote_thread_info *remote_thr = get_remote_thread_info (tp);

	if (remote_thr->get_resume_state ()
	    == resume_state::RESUMED_PENDING_VCONT)
	  {
	    remote_debug_printf ("Enqueueing phony stop reply for thread pending "
				 "vCont-resume (%d, %ld, %s)", tp->ptid.pid(),
				 tp->ptid.lwp (),
				 pulongest (tp->ptid.tid ()));

	    /* Check that the thread wasn't resumed with a signal.
	       Generating a phony stop would result in losing the
	       signal.  */
	    const resumed_pending_vcont_info &info
	      = remote_thr->resumed_pending_vcont_info ();
	    gdb_assert (info.sig == GDB_SIGNAL_0);

	    stop_reply_up sr = std::make_unique<stop_reply> ();
	    sr->ptid = tp->ptid;
	    sr->rs = rs;
	    sr->ws.set_stopped (GDB_SIGNAL_0);
	    sr->arch = tp->inf->arch ();
	    sr->stop_reason = TARGET_STOPPED_BY_NO_REASON;
	    sr->watch_data_address = 0;
	    sr->core = 0;
	    this->push_stop_reply (std::move (sr));

	    /* Pretend that this thread was actually resumed on the
	       remote target, then stopped.  If we leave it in the
	       RESUMED_PENDING_VCONT state and the commit_resumed
	       method is called while the stop reply is still in the
	       queue, we'll end up reporting a stop event to the core
	       for that thread while it is running on the remote
	       target... that would be bad.  */
	    remote_thr->set_resumed ();
	  }
      }

  if (!rs->supports_vCont.t)
    error (_("Remote server does not support stopping threads"));

  if (ptid == minus_one_ptid
      || (!m_features.remote_multi_process_p () && ptid.is_pid ()))
    p += xsnprintf (p, endp - p, "vCont;t");
  else
    {
      ptid_t nptid;

      p += xsnprintf (p, endp - p, "vCont;t:");

      if (ptid.is_pid ())
	  /* All (-1) threads of process.  */
	nptid = ptid_t (ptid.pid (), -1);
      else
	{
	  /* Small optimization: if we already have a stop reply for
	     this thread, no use in telling the stub we want this
	     stopped.  */
	  if (peek_stop_reply (ptid))
	    return;

	  nptid = ptid;
	}

      write_ptid (p, endp, nptid);
    }

  /* In non-stop, we get an immediate OK reply.  The stop reply will
     come in asynchronously by notification.  */
  putpkt (rs->buf);
  getpkt (&rs->buf);
  if (strcmp (rs->buf.data (), "OK") != 0)
    error (_("Stopping %s failed: %s"), target_pid_to_str (ptid).c_str (),
	   rs->buf.data ());
}

/* All-stop version of target_interrupt.  Sends a break or a ^C to
   interrupt the remote target.  It is undefined which thread of which
   process reports the interrupt.  */

void
remote_target::remote_interrupt_as ()
{
  struct remote_state *rs = get_remote_state ();

  rs->ctrlc_pending_p = 1;

  /* If the inferior is stopped already, but the core didn't know
     about it yet, just ignore the request.  The pending stop events
     will be collected in remote_wait.  */
  if (stop_reply_queue_length () > 0)
    return;

  /* Send interrupt_sequence to remote target.  */
  send_interrupt_sequence ();
}

/* Non-stop version of target_interrupt.  Uses `vCtrlC' to interrupt
   the remote target.  It is undefined which thread of which process
   reports the interrupt.  Throws an error if the packet is not
   supported by the server.  */

void
remote_target::remote_interrupt_ns ()
{
  struct remote_state *rs = get_remote_state ();
  char *p = rs->buf.data ();
  char *endp = p + get_remote_packet_size ();

  xsnprintf (p, endp - p, "vCtrlC");

  /* In non-stop, we get an immediate OK reply.  The stop reply will
     come in asynchronously by notification.  */
  putpkt (rs->buf);
  getpkt (&rs->buf);

  switch (m_features.packet_ok (rs->buf, PACKET_vCtrlC))
    {
    case PACKET_OK:
      break;
    case PACKET_UNKNOWN:
      error (_("No support for interrupting the remote target."));
    case PACKET_ERROR:
      error (_("Interrupting target failed: %s"), rs->buf.data ());
    }
}

/* Implement the to_stop function for the remote targets.  */

void
remote_target::stop (ptid_t ptid)
{
  REMOTE_SCOPED_DEBUG_ENTER_EXIT;

  if (target_is_non_stop_p ())
    remote_stop_ns (ptid);
  else
    {
      /* We don't currently have a way to transparently pause the
	 remote target in all-stop mode.  Interrupt it instead.  */
      remote_interrupt_as ();
    }
}

/* Implement the to_interrupt function for the remote targets.  */

void
remote_target::interrupt ()
{
  REMOTE_SCOPED_DEBUG_ENTER_EXIT;

  if (target_is_non_stop_p ())
    remote_interrupt_ns ();
  else
    remote_interrupt_as ();
}

/* Implement the to_pass_ctrlc function for the remote targets.  */

void
remote_target::pass_ctrlc ()
{
  REMOTE_SCOPED_DEBUG_ENTER_EXIT;

  struct remote_state *rs = get_remote_state ();

  /* If we're starting up, we're not fully synced yet.  Quit
     immediately.  */
  if (rs->starting_up)
    quit ();
  /* If ^C has already been sent once, offer to disconnect.  */
  else if (rs->ctrlc_pending_p)
    interrupt_query ();
  else
    target_interrupt ();
}

/* Ask the user what to do when an interrupt is received.  */

void
remote_target::interrupt_query ()
{
  struct remote_state *rs = get_remote_state ();

  if (rs->waiting_for_stop_reply && rs->ctrlc_pending_p)
    {
      if (query (_("The target is not responding to interrupt requests.\n"
		   "Stop debugging it? ")))
	{
	  remote_unpush_target (this);
	  throw_error (TARGET_CLOSE_ERROR, _("Disconnected from target."));
	}
    }
  else
    {
      if (query (_("Interrupted while waiting for the program.\n"
		   "Give up waiting? ")))
	quit ();
    }
}

/* Enable/disable target terminal ownership.  Most targets can use
   terminal groups to control terminal ownership.  Remote targets are
   different in that explicit transfer of ownership to/from GDB/target
   is required.  */

void
remote_target::terminal_inferior ()
{
  /* NOTE: At this point we could also register our selves as the
     recipient of all input.  Any characters typed could then be
     passed on down to the target.  */
}

void
remote_target::terminal_ours ()
{
}

static void
remote_console_output (const char *msg)
{
  const char *p;

  for (p = msg; p[0] && p[1]; p += 2)
    {
      char tb[2];
      char c = fromhex (p[0]) * 16 + fromhex (p[1]);

      tb[0] = c;
      tb[1] = 0;
      gdb_stdtarg->puts (tb);
    }
  gdb_stdtarg->flush ();
}

/* Return the length of the stop reply queue.  */

int
remote_target::stop_reply_queue_length ()
{
  remote_state *rs = get_remote_state ();
  return rs->stop_reply_queue.size ();
}

static void
remote_notif_stop_parse (remote_target *remote,
			 const notif_client *self, const char *buf,
			 struct notif_event *event)
{
  remote->remote_parse_stop_reply (buf, (struct stop_reply *) event);
}

static void
remote_notif_stop_ack (remote_target *remote,
		       const notif_client *self, const char *buf,
		       notif_event_up event)
{
  stop_reply_up stop_reply = as_stop_reply_up (std::move (event));

  /* acknowledge */
  putpkt (remote, self->ack_command);

  /* Kind can be TARGET_WAITKIND_IGNORE if we have meanwhile discarded
     the notification.  It was left in the queue because we need to
     acknowledge it and pull the rest of the notifications out.  */
  if (stop_reply->ws.kind () != TARGET_WAITKIND_IGNORE)
    remote->push_stop_reply (std::move (stop_reply));
}

static int
remote_notif_stop_can_get_pending_events (remote_target *remote,
					  const notif_client *self)
{
  /* We can't get pending events in remote_notif_process for
     notification stop, and we have to do this in remote_wait_ns
     instead.  If we fetch all queued events from stub, remote stub
     may exit and we have no chance to process them back in
     remote_wait_ns.  */
  remote_state *rs = remote->get_remote_state ();
  rs->mark_async_event_handler ();
  return 0;
}

static notif_event_up
remote_notif_stop_alloc_reply ()
{
  return notif_event_up (new struct stop_reply ());
}

/* A client of notification Stop.  */

const notif_client notif_client_stop =
{
  "Stop",
  "vStopped",
  remote_notif_stop_parse,
  remote_notif_stop_ack,
  remote_notif_stop_can_get_pending_events,
  remote_notif_stop_alloc_reply,
  REMOTE_NOTIF_STOP,
};

/* If CONTEXT contains any fork/vfork/clone child threads that have
   not been reported yet, remove them from the CONTEXT list.  If such
   a thread exists it is because we are stopped at a fork/vfork/clone
   catchpoint and have not yet called follow_fork/follow_clone, which
   will set up the host-side data structures for the new child.  */

void
remote_target::remove_new_children (threads_listing_context *context)
{
  const notif_client *notif = &notif_client_stop;

  /* For any threads stopped at a (v)fork/clone event, remove the
     corresponding child threads from the CONTEXT list.  */
  for (thread_info *thread : all_non_exited_threads (this))
    {
      const target_waitstatus *ws = thread_pending_child_status (thread);

      if (ws == nullptr)
	continue;

      context->remove_thread (ws->child_ptid ());
    }

  /* Check for any pending (v)fork/clone events (not reported or
     processed yet) in process PID and remove those child threads from
     the CONTEXT list as well.  */
  remote_notif_get_pending_events (notif);
  for (auto &event : get_remote_state ()->stop_reply_queue)
    if (is_new_child_status (event->ws.kind ()))
      context->remove_thread (event->ws.child_ptid ());
    else if (event->ws.kind () == TARGET_WAITKIND_THREAD_EXITED)
      context->remove_thread (event->ptid);
}

/* Check whether any event pending in the vStopped queue would prevent a
   global or process wildcard vCont action.  Set *may_global_wildcard to
   false if we can't do a global wildcard (vCont;c), and clear the event
   inferior's may_wildcard_vcont flag if we can't do a process-wide
   wildcard resume (vCont;c:pPID.-1).  */

void
remote_target::check_pending_events_prevent_wildcard_vcont
  (bool *may_global_wildcard)
{
  const notif_client *notif = &notif_client_stop;

  remote_notif_get_pending_events (notif);
  for (auto &event : get_remote_state ()->stop_reply_queue)
    {
      if (event->ws.kind () == TARGET_WAITKIND_NO_RESUMED
	  || event->ws.kind () == TARGET_WAITKIND_NO_HISTORY)
	continue;

      if (event->ws.kind () == TARGET_WAITKIND_FORKED
	  || event->ws.kind () == TARGET_WAITKIND_VFORKED)
	*may_global_wildcard = false;

      /* This may be the first time we heard about this process.
	 Regardless, we must not do a global wildcard resume, otherwise
	 we'd resume this process too.  */
      *may_global_wildcard = false;
      if (event->ptid != null_ptid)
	{
	  inferior *inf = find_inferior_ptid (this, event->ptid);
	  if (inf != NULL)
	    get_remote_inferior (inf)->may_wildcard_vcont = false;
	}
    }
}

/* Discard all pending stop replies of inferior INF.  */

void
remote_target::discard_pending_stop_replies (struct inferior *inf)
{
  struct remote_state *rs = get_remote_state ();
  struct remote_notif_state *rns = rs->notif_state;

  /* This function can be notified when an inferior exists.  When the
     target is not remote, the notification state is NULL.  */
  if (rs->remote_desc == NULL)
    return;

  struct notif_event *notif_event
    = rns->pending_event[notif_client_stop.id].get ();
  auto *reply = static_cast<stop_reply *> (notif_event);

  /* Discard the in-flight notification.  */
  if (reply != NULL && reply->ptid.pid () == inf->pid)
    {
      /* Leave the notification pending, since the server expects that
	 we acknowledge it with vStopped.  But clear its contents, so
	 that later on when we acknowledge it, we also discard it.  */
      remote_debug_printf
	("discarding in-flight notification: ptid: %s, ws: %s\n",
	 reply->ptid.to_string().c_str(),
	 reply->ws.to_string ().c_str ());
      reply->ws.set_ignore ();
    }

  /* Discard the stop replies we have already pulled with
     vStopped.  */
  auto iter = std::remove_if (rs->stop_reply_queue.begin (),
			      rs->stop_reply_queue.end (),
			      [=] (const stop_reply_up &event)
			      {
				return event->ptid.pid () == inf->pid;
			      });
  for (auto it = iter; it != rs->stop_reply_queue.end (); ++it)
    remote_debug_printf
      ("discarding queued stop reply: ptid: %s, ws: %s\n",
       (*it)->ptid.to_string().c_str(),
       (*it)->ws.to_string ().c_str ());
  rs->stop_reply_queue.erase (iter, rs->stop_reply_queue.end ());
}

/* Discard the stop replies for RS in stop_reply_queue.  */

void
remote_target::discard_pending_stop_replies_in_queue ()
{
  remote_state *rs = get_remote_state ();

  /* Discard the stop replies we have already pulled with
     vStopped.  */
  auto iter = std::remove_if (rs->stop_reply_queue.begin (),
			      rs->stop_reply_queue.end (),
			      [=] (const stop_reply_up &event)
			      {
				return event->rs == rs;
			      });
  rs->stop_reply_queue.erase (iter, rs->stop_reply_queue.end ());
}

/* Remove the first reply in 'stop_reply_queue' which matches
   PTID.  */

stop_reply_up
remote_target::remote_notif_remove_queued_reply (ptid_t ptid)
{
  remote_state *rs = get_remote_state ();

  auto iter = std::find_if (rs->stop_reply_queue.begin (),
			    rs->stop_reply_queue.end (),
			    [=] (const stop_reply_up &event)
			    {
			      return event->ptid.matches (ptid);
			    });
  stop_reply_up result;
  if (iter != rs->stop_reply_queue.end ())
    {
      result = std::move (*iter);
      rs->stop_reply_queue.erase (iter);
    }

  if (notif_debug)
    gdb_printf (gdb_stdlog,
		"notif: discard queued event: 'Stop' in %s\n",
		ptid.to_string ().c_str ());

  return result;
}

/* Look for a queued stop reply belonging to PTID.  If one is found,
   remove it from the queue, and return it.  Returns NULL if none is
   found.  If there are still queued events left to process, tell the
   event loop to get back to target_wait soon.  */

stop_reply_up
remote_target::queued_stop_reply (ptid_t ptid)
{
  remote_state *rs = get_remote_state ();
  stop_reply_up r = remote_notif_remove_queued_reply (ptid);

  if (!rs->stop_reply_queue.empty () && target_can_async_p ())
    {
      /* There's still at least an event left.  */
      rs->mark_async_event_handler ();
    }

  return r;
}

/* Push a fully parsed stop reply in the stop reply queue.  Since we
   know that we now have at least one queued event left to pass to the
   core side, tell the event loop to get back to target_wait soon.  */

void
remote_target::push_stop_reply (stop_reply_up new_event)
{
  remote_state *rs = get_remote_state ();
  rs->stop_reply_queue.push_back (std::move (new_event));

  if (notif_debug)
    gdb_printf (gdb_stdlog,
		"notif: push 'Stop' %s to queue %d\n",
		new_event->ptid.to_string ().c_str (),
		int (rs->stop_reply_queue.size ()));

  /* Mark the pending event queue only if async mode is currently enabled.
     If async mode is not currently enabled, then, if it later becomes
     enabled, and there are events in this queue, we will mark the event
     token at that point, see remote_target::async.  */
  if (target_is_async_p ())
    rs->mark_async_event_handler ();
}

/* Returns true if we have a stop reply for PTID.  */

int
remote_target::peek_stop_reply (ptid_t ptid)
{
  remote_state *rs = get_remote_state ();
  for (auto &event : rs->stop_reply_queue)
    if (ptid == event->ptid
	&& event->ws.kind () == TARGET_WAITKIND_STOPPED)
      return 1;
  return 0;
}

/* Helper for remote_parse_stop_reply.  Return nonzero if the substring
   starting with P and ending with PEND matches PREFIX.  */

static int
strprefix (const char *p, const char *pend, const char *prefix)
{
  for ( ; p < pend; p++, prefix++)
    if (*p != *prefix)
      return 0;
  return *prefix == '\0';
}

/* Parse the stop reply in BUF.  Either the function succeeds, and the
   result is stored in EVENT, or throws an error.  */

void
remote_target::remote_parse_stop_reply (const char *buf, stop_reply *event)
{
  remote_arch_state *rsa = NULL;
  ULONGEST addr;
  const char *p;
  int skipregs = 0;

  event->ptid = null_ptid;
  event->rs = get_remote_state ();
  event->ws.set_ignore ();
  event->stop_reason = TARGET_STOPPED_BY_NO_REASON;
  event->regcache.clear ();
  event->core = -1;

  switch (buf[0])
    {
    case 'T':		/* Status with PC, SP, FP, ...	*/
      /* Expedited reply, containing Signal, {regno, reg} repeat.  */
      /*  format is:  'Tssn...:r...;n...:r...;n...:r...;#cc', where
	    ss = signal number
	    n... = register number
	    r... = register contents
      */

      p = &buf[3];	/* after Txx */
      while (*p)
	{
	  const char *p1;
	  int fieldsize;

	  p1 = strchr (p, ':');
	  if (p1 == NULL)
	    error (_("Malformed packet(a) (missing colon): %s\n\
Packet: '%s'\n"),
		   p, buf);
	  if (p == p1)
	    error (_("Malformed packet(a) (missing register number): %s\n\
Packet: '%s'\n"),
		   p, buf);

	  /* Some "registers" are actually extended stop information.
	     Note if you're adding a new entry here: GDB 7.9 and
	     earlier assume that all register "numbers" that start
	     with an hex digit are real register numbers.  Make sure
	     the server only sends such a packet if it knows the
	     client understands it.  */

	  if (strprefix (p, p1, "thread"))
	    event->ptid = read_ptid (++p1, &p);
	  else if (strprefix (p, p1, "syscall_entry"))
	    {
	      ULONGEST sysno;

	      p = unpack_varlen_hex (++p1, &sysno);
	      event->ws.set_syscall_entry ((int) sysno);
	    }
	  else if (strprefix (p, p1, "syscall_return"))
	    {
	      ULONGEST sysno;

	      p = unpack_varlen_hex (++p1, &sysno);
	      event->ws.set_syscall_return ((int) sysno);
	    }
	  else if (strprefix (p, p1, "watch")
		   || strprefix (p, p1, "rwatch")
		   || strprefix (p, p1, "awatch"))
	    {
	      event->stop_reason = TARGET_STOPPED_BY_WATCHPOINT;
	      p = unpack_varlen_hex (++p1, &addr);
	      event->watch_data_address = (CORE_ADDR) addr;
	    }
	  else if (strprefix (p, p1, "swbreak"))
	    {
	      event->stop_reason = TARGET_STOPPED_BY_SW_BREAKPOINT;

	      /* Make sure the stub doesn't forget to indicate support
		 with qSupported.  */
	      if (m_features.packet_support (PACKET_swbreak_feature)
		  != PACKET_ENABLE)
		error (_("Unexpected swbreak stop reason"));

	      /* The value part is documented as "must be empty",
		 though we ignore it, in case we ever decide to make
		 use of it in a backward compatible way.  */
	      p = strchrnul (p1 + 1, ';');
	    }
	  else if (strprefix (p, p1, "hwbreak"))
	    {
	      event->stop_reason = TARGET_STOPPED_BY_HW_BREAKPOINT;

	      /* Make sure the stub doesn't forget to indicate support
		 with qSupported.  */
	      if (m_features.packet_support (PACKET_hwbreak_feature)
		  != PACKET_ENABLE)
		error (_("Unexpected hwbreak stop reason"));

	      /* See above.  */
	      p = strchrnul (p1 + 1, ';');
	    }
	  else if (strprefix (p, p1, "library"))
	    {
	      event->ws.set_loaded ();
	      p = strchrnul (p1 + 1, ';');
	    }
	  else if (strprefix (p, p1, "replaylog"))
	    {
	      event->ws.set_no_history ();
	      /* p1 will indicate "begin" or "end", but it makes
		 no difference for now, so ignore it.  */
	      p = strchrnul (p1 + 1, ';');
	    }
	  else if (strprefix (p, p1, "core"))
	    {
	      ULONGEST c;

	      p = unpack_varlen_hex (++p1, &c);
	      event->core = c;
	    }
	  else if (strprefix (p, p1, "fork"))
	    event->ws.set_forked (read_ptid (++p1, &p));
	  else if (strprefix (p, p1, "vfork"))
	    event->ws.set_vforked (read_ptid (++p1, &p));
	  else if (strprefix (p, p1, "clone"))
	    event->ws.set_thread_cloned (read_ptid (++p1, &p));
	  else if (strprefix (p, p1, "vforkdone"))
	    {
	      event->ws.set_vfork_done ();
	      p = strchrnul (p1 + 1, ';');
	    }
	  else if (strprefix (p, p1, "exec"))
	    {
	      ULONGEST ignored;
	      int pathlen;

	      /* Determine the length of the execd pathname.  */
	      p = unpack_varlen_hex (++p1, &ignored);
	      pathlen = (p - p1) / 2;

	      /* Save the pathname for event reporting and for
		 the next run command.  */
	      gdb::unique_xmalloc_ptr<char> pathname
		((char *) xmalloc (pathlen + 1));
	      hex2bin (p1, (gdb_byte *) pathname.get (), pathlen);
	      pathname.get ()[pathlen] = '\0';

	      /* This is freed during event handling.  */
	      event->ws.set_execd (std::move (pathname));

	      /* Skip the registers included in this packet, since
		 they may be for an architecture different from the
		 one used by the original program.  */
	      skipregs = 1;
	    }
	  else if (strprefix (p, p1, "create"))
	    {
	      event->ws.set_thread_created ();
	      p = strchrnul (p1 + 1, ';');
	    }
	  else
	    {
	      ULONGEST pnum;
	      const char *p_temp;

	      if (skipregs)
		{
		  p = strchrnul (p1 + 1, ';');
		  p++;
		  continue;
		}

	      /* Maybe a real ``P'' register number.  */
	      p_temp = unpack_varlen_hex (p, &pnum);
	      /* If the first invalid character is the colon, we got a
		 register number.  Otherwise, it's an unknown stop
		 reason.  */
	      if (p_temp == p1)
		{
		  /* If we haven't parsed the event's thread yet, find
		     it now, in order to find the architecture of the
		     reported expedited registers.  */
		  if (event->ptid == null_ptid)
		    {
		      /* If there is no thread-id information then leave
			 the event->ptid as null_ptid.  Later in
			 process_stop_reply we will pick a suitable
			 thread.  */
		      const char *thr = strstr (p1 + 1, ";thread:");
		      if (thr != NULL)
			event->ptid = read_ptid (thr + strlen (";thread:"),
						 NULL);
		    }

		  if (rsa == NULL)
		    {
		      inferior *inf
			= (event->ptid == null_ptid
			   ? NULL
			   : find_inferior_ptid (this, event->ptid));
		      /* If this is the first time we learn anything
			 about this process, skip the registers
			 included in this packet, since we don't yet
			 know which architecture to use to parse them.
			 We'll determine the architecture later when
			 we process the stop reply and retrieve the
			 target description, via
			 remote_notice_new_inferior ->
			 post_create_inferior.  */
		      if (inf == NULL)
			{
			  p = strchrnul (p1 + 1, ';');
			  p++;
			  continue;
			}

		      event->arch = inf->arch ();
		      rsa = event->rs->get_remote_arch_state (event->arch);
		    }

		  packet_reg *reg
		    = packet_reg_from_pnum (event->arch, rsa, pnum);
		  cached_reg_t cached_reg;

		  if (reg == NULL)
		    error (_("Remote sent bad register number %s: %s\n\
Packet: '%s'\n"),
			   hex_string (pnum), p, buf);

		  cached_reg.num = reg->regnum;
		  cached_reg.data.reset ((gdb_byte *)
					 xmalloc (register_size (event->arch,
								 reg->regnum)));

		  p = p1 + 1;
		  fieldsize = hex2bin (p, cached_reg.data.get (),
				       register_size (event->arch, reg->regnum));
		  p += 2 * fieldsize;
		  if (fieldsize < register_size (event->arch, reg->regnum))
		    warning (_("Remote reply is too short: %s"), buf);

		  event->regcache.push_back (std::move (cached_reg));
		}
	      else
		{
		  /* Not a number.  Silently skip unknown optional
		     info.  */
		  p = strchrnul (p1 + 1, ';');
		}
	    }

	  if (*p != ';')
	    error (_("Remote register badly formatted: %s\nhere: %s"),
		   buf, p);
	  ++p;
	}

      if (event->ws.kind () != TARGET_WAITKIND_IGNORE)
	break;

      [[fallthrough]];
    case 'S':		/* Old style status, just signal only.  */
      {
	int sig;

	sig = (fromhex (buf[1]) << 4) + fromhex (buf[2]);
	if (GDB_SIGNAL_FIRST <= sig && sig < GDB_SIGNAL_LAST)
	  event->ws.set_stopped ((enum gdb_signal) sig);
	else
	  event->ws.set_stopped (GDB_SIGNAL_UNKNOWN);
      }
      break;
    case 'w':		/* Thread exited.  */
      {
	ULONGEST value;

	p = unpack_varlen_hex (&buf[1], &value);
	event->ws.set_thread_exited (value);
	if (*p != ';')
	  error (_("stop reply packet badly formatted: %s"), buf);
	event->ptid = read_ptid (++p, NULL);
	break;
      }
    case 'W':		/* Target exited.  */
    case 'X':
      {
	ULONGEST value;

	/* GDB used to accept only 2 hex chars here.  Stubs should
	   only send more if they detect GDB supports multi-process
	   support.  */
	p = unpack_varlen_hex (&buf[1], &value);

	if (buf[0] == 'W')
	  {
	    /* The remote process exited.  */
	    event->ws.set_exited (value);
	  }
	else
	  {
	    /* The remote process exited with a signal.  */
	    if (GDB_SIGNAL_FIRST <= value && value < GDB_SIGNAL_LAST)
	      event->ws.set_signalled ((enum gdb_signal) value);
	    else
	      event->ws.set_signalled (GDB_SIGNAL_UNKNOWN);
	  }

	/* If no process is specified, return null_ptid, and let the
	   caller figure out the right process to use.  */
	int pid = 0;
	if (*p == '\0')
	  ;
	else if (*p == ';')
	  {
	    p++;

	    if (*p == '\0')
	      ;
	    else if (startswith (p, "process:"))
	      {
		ULONGEST upid;

		p += sizeof ("process:") - 1;
		unpack_varlen_hex (p, &upid);
		pid = upid;
	      }
	    else
	      error (_("unknown stop reply packet: %s"), buf);
	  }
	else
	  error (_("unknown stop reply packet: %s"), buf);
	event->ptid = ptid_t (pid);
      }
      break;
    case 'N':
      event->ws.set_no_resumed ();
      event->ptid = minus_one_ptid;
      break;
    }
}

/* When the stub wants to tell GDB about a new notification reply, it
   sends a notification (%Stop, for example).  Those can come it at
   any time, hence, we have to make sure that any pending
   putpkt/getpkt sequence we're making is finished, before querying
   the stub for more events with the corresponding ack command
   (vStopped, for example).  E.g., if we started a vStopped sequence
   immediately upon receiving the notification, something like this
   could happen:

    1.1) --> Hg 1
    1.2) <-- OK
    1.3) --> g
    1.4) <-- %Stop
    1.5) --> vStopped
    1.6) <-- (registers reply to step #1.3)

   Obviously, the reply in step #1.6 would be unexpected to a vStopped
   query.

   To solve this, whenever we parse a %Stop notification successfully,
   we mark the REMOTE_ASYNC_GET_PENDING_EVENTS_TOKEN, and carry on
   doing whatever we were doing:

    2.1) --> Hg 1
    2.2) <-- OK
    2.3) --> g
    2.4) <-- %Stop
      <GDB marks the REMOTE_ASYNC_GET_PENDING_EVENTS_TOKEN>
    2.5) <-- (registers reply to step #2.3)

   Eventually after step #2.5, we return to the event loop, which
   notices there's an event on the
   REMOTE_ASYNC_GET_PENDING_EVENTS_TOKEN event and calls the
   associated callback --- the function below.  At this point, we're
   always safe to start a vStopped sequence. :

    2.6) --> vStopped
    2.7) <-- T05 thread:2
    2.8) --> vStopped
    2.9) --> OK
*/

void
remote_target::remote_notif_get_pending_events (const notif_client *nc)
{
  struct remote_state *rs = get_remote_state ();

  if (rs->notif_state->pending_event[nc->id] != NULL)
    {
      if (notif_debug)
	gdb_printf (gdb_stdlog,
		    "notif: process: '%s' ack pending event\n",
		    nc->name);

      /* acknowledge */
      nc->ack (this, nc, rs->buf.data (),
	       std::move (rs->notif_state->pending_event[nc->id]));

      while (1)
	{
	  getpkt (&rs->buf);
	  if (strcmp (rs->buf.data (), "OK") == 0)
	    break;
	  else
	    remote_notif_ack (this, nc, rs->buf.data ());
	}
    }
  else
    {
      if (notif_debug)
	gdb_printf (gdb_stdlog,
		    "notif: process: '%s' no pending reply\n",
		    nc->name);
    }
}

/* Wrapper around remote_target::remote_notif_get_pending_events to
   avoid having to export the whole remote_target class.  */

void
remote_notif_get_pending_events (remote_target *remote, const notif_client *nc)
{
  remote->remote_notif_get_pending_events (nc);
}

/* Called from process_stop_reply when the stop packet we are responding
   to didn't include a process-id or thread-id.  STATUS is the stop event
   we are responding to.

   It is the task of this function to select a suitable thread (or process)
   and return its ptid, this is the thread (or process) we will assume the
   stop event came from.

   In some cases there isn't really any choice about which thread (or
   process) is selected, a basic remote with a single process containing a
   single thread might choose not to send any process-id or thread-id in
   its stop packets, this function will select and return the one and only
   thread.

   However, if a target supports multiple threads (or processes) and still
   doesn't include a thread-id (or process-id) in its stop packet then
   first, this is a badly behaving target, and second, we're going to have
   to select a thread (or process) at random and use that.  This function
   will print a warning to the user if it detects that there is the
   possibility that GDB is guessing which thread (or process) to
   report.

   Note that this is called before GDB fetches the updated thread list from the
   target.  So it's possible for the stop reply to be ambiguous and for GDB to
   not realize it.  For example, if there's initially one thread, the target
   spawns a second thread, and then sends a stop reply without an id that
   concerns the first thread.  GDB will assume the stop reply is about the
   first thread - the only thread it knows about - without printing a warning.
   Anyway, if the remote meant for the stop reply to be about the second thread,
   then it would be really broken, because GDB doesn't know about that thread
   yet.  */

ptid_t
remote_target::select_thread_for_ambiguous_stop_reply
  (const target_waitstatus &status)
{
  REMOTE_SCOPED_DEBUG_ENTER_EXIT;

  /* Some stop events apply to all threads in an inferior, while others
     only apply to a single thread.  */
  bool process_wide_stop
    = (status.kind () == TARGET_WAITKIND_EXITED
       || status.kind () == TARGET_WAITKIND_SIGNALLED);

  remote_debug_printf ("process_wide_stop = %d", process_wide_stop);

  thread_info *first_resumed_thread = nullptr;
  bool ambiguous = false;

  /* Consider all non-exited threads of the target, find the first resumed
     one.  */
  for (thread_info *thr : all_non_exited_threads (this))
    {
      remote_thread_info *remote_thr = get_remote_thread_info (thr);

      if (remote_thr->get_resume_state () != resume_state::RESUMED)
	continue;

      if (first_resumed_thread == nullptr)
	first_resumed_thread = thr;
      else if (!process_wide_stop
	       || first_resumed_thread->ptid.pid () != thr->ptid.pid ())
	ambiguous = true;
    }

  gdb_assert (first_resumed_thread != nullptr);

  remote_debug_printf ("first resumed thread is %s",
		       pid_to_str (first_resumed_thread->ptid).c_str ());
  remote_debug_printf ("is this guess ambiguous? = %d", ambiguous);

  /* Warn if the remote target is sending ambiguous stop replies.  */
  if (ambiguous)
    {
      static bool warned = false;

      if (!warned)
	{
	  /* If you are seeing this warning then the remote target has
	     stopped without specifying a thread-id, but the target
	     does have multiple threads (or inferiors), and so GDB is
	     having to guess which thread stopped.

	     Examples of what might cause this are the target sending
	     and 'S' stop packet, or a 'T' stop packet and not
	     including a thread-id.

	     Additionally, the target might send a 'W' or 'X packet
	     without including a process-id, when the target has
	     multiple running inferiors.  */
	  if (process_wide_stop)
	    warning (_("multi-inferior target stopped without "
		       "sending a process-id, using first "
		       "non-exited inferior"));
	  else
	    warning (_("multi-threaded target stopped without "
		       "sending a thread-id, using first "
		       "non-exited thread"));
	  warned = true;
	}
    }

  /* If this is a stop for all threads then don't use a particular threads
     ptid, instead create a new ptid where only the pid field is set.  */
  if (process_wide_stop)
    return ptid_t (first_resumed_thread->ptid.pid ());
  else
    return first_resumed_thread->ptid;
}

/* Called when it is decided that STOP_REPLY holds the info of the
   event that is to be returned to the core.  This function always
   destroys STOP_REPLY.  */

ptid_t
remote_target::process_stop_reply (stop_reply_up stop_reply,
				   struct target_waitstatus *status)
{
  *status = stop_reply->ws;
  ptid_t ptid = stop_reply->ptid;

  /* If no thread/process was reported by the stub then select a suitable
     thread/process.  */
  if (ptid == null_ptid)
    ptid = select_thread_for_ambiguous_stop_reply (*status);
  gdb_assert (ptid != null_ptid);

  if (status->kind () != TARGET_WAITKIND_EXITED
      && status->kind () != TARGET_WAITKIND_SIGNALLED
      && status->kind () != TARGET_WAITKIND_NO_RESUMED)
    {
      remote_notice_new_inferior (ptid, false);

      /* Expedited registers.  */
      if (!stop_reply->regcache.empty ())
	{
	  /* 'w' stop replies don't cary expedited registers (which
	     wouldn't make any sense for a thread that is gone
	     already).  */
	  gdb_assert (status->kind () != TARGET_WAITKIND_THREAD_EXITED);

	  regcache *regcache
	    = get_thread_arch_regcache (find_inferior_ptid (this, ptid), ptid,
					stop_reply->arch);

	  for (cached_reg_t &reg : stop_reply->regcache)
	    regcache->raw_supply (reg.num, reg.data.get ());
	}

      remote_thread_info *remote_thr = get_remote_thread_info (this, ptid);
      remote_thr->core = stop_reply->core;
      remote_thr->stop_reason = stop_reply->stop_reason;
      remote_thr->watch_data_address = stop_reply->watch_data_address;

      if (target_is_non_stop_p ())
	{
	  /* If the target works in non-stop mode, a stop-reply indicates that
	     only this thread stopped.  */
	  remote_thr->set_not_resumed ();
	}
      else
	{
	  /* If the target works in all-stop mode, a stop-reply indicates that
	     all the target's threads stopped.  */
	  for (thread_info *tp : all_non_exited_threads (this))
	    get_remote_thread_info (tp)->set_not_resumed ();
	}
    }

  return ptid;
}

/* The non-stop mode version of target_wait.  */

ptid_t
remote_target::wait_ns (ptid_t ptid, struct target_waitstatus *status,
			target_wait_flags options)
{
  struct remote_state *rs = get_remote_state ();
  int ret;
  bool is_notif = false;

  /* If in non-stop mode, get out of getpkt even if a
     notification is received.	*/

  ret = getpkt (&rs->buf, false /* forever */, &is_notif);
  while (1)
    {
      if (ret != -1 && !is_notif)
	switch (rs->buf[0])
	  {
	  case 'E':		/* Error of some sort.	*/
	    /* We're out of sync with the target now.  Did it continue
	       or not?  We can't tell which thread it was in non-stop,
	       so just ignore this.  */
	    warning (_("Remote failure reply: %s"), rs->buf.data ());
	    break;
	  case 'O':		/* Console output.  */
	    remote_console_output (&rs->buf[1]);
	    break;
	  default:
	    warning (_("Invalid remote reply: %s"), rs->buf.data ());
	    break;
	  }

      /* Acknowledge a pending stop reply that may have arrived in the
	 mean time.  */
      if (rs->notif_state->pending_event[notif_client_stop.id] != NULL)
	remote_notif_get_pending_events (&notif_client_stop);

      /* If indeed we noticed a stop reply, we're done.  */
      stop_reply_up stop_reply = queued_stop_reply (ptid);
      if (stop_reply != NULL)
	return process_stop_reply (std::move (stop_reply), status);

      /* Still no event.  If we're just polling for an event, then
	 return to the event loop.  */
      if (options & TARGET_WNOHANG)
	{
	  status->set_ignore ();
	  return minus_one_ptid;
	}

      /* Otherwise do a blocking wait.  */
      ret = getpkt (&rs->buf, true /* forever */, &is_notif);
    }
}

/* Return the first resumed thread.  */

static ptid_t
first_remote_resumed_thread (remote_target *target)
{
  for (thread_info *tp : all_non_exited_threads (target, minus_one_ptid))
    if (tp->resumed ())
      return tp->ptid;
  return null_ptid;
}

/* Wait until the remote machine stops, then return, storing status in
   STATUS just as `wait' would.  */

ptid_t
remote_target::wait_as (ptid_t ptid, target_waitstatus *status,
			target_wait_flags options)
{
  struct remote_state *rs = get_remote_state ();
  ptid_t event_ptid = null_ptid;
  char *buf;
  stop_reply_up stop_reply;

 again:

  status->set_ignore ();

  stop_reply = queued_stop_reply (ptid);
  if (stop_reply != NULL)
    {
      /* None of the paths that push a stop reply onto the queue should
	 have set the waiting_for_stop_reply flag.  */
      gdb_assert (!rs->waiting_for_stop_reply);
      event_ptid = process_stop_reply (std::move (stop_reply), status);
    }
  else
    {
      bool forever = ((options & TARGET_WNOHANG) == 0
		      && rs->wait_forever_enabled_p);

      if (!rs->waiting_for_stop_reply)
	{
	  status->set_no_resumed ();
	  return minus_one_ptid;
	}

      /* FIXME: cagney/1999-09-27: If we're in async mode we should
	 _never_ wait for ever -> test on target_is_async_p().
	 However, before we do that we need to ensure that the caller
	 knows how to take the target into/out of async mode.  */
      bool is_notif;
      int ret = getpkt (&rs->buf, forever, &is_notif);

      /* GDB gets a notification.  Return to core as this event is
	 not interesting.  */
      if (ret != -1 && is_notif)
	return minus_one_ptid;

      if (ret == -1 && (options & TARGET_WNOHANG) != 0)
	return minus_one_ptid;

      buf = rs->buf.data ();

      /* Assume that the target has acknowledged Ctrl-C unless we receive
	 an 'F' or 'O' packet.  */
      if (buf[0] != 'F' && buf[0] != 'O')
	rs->ctrlc_pending_p = 0;

      switch (buf[0])
	{
	case 'E':		/* Error of some sort.	*/
	  /* We're out of sync with the target now.  Did it continue or
	     not?  Not is more likely, so report a stop.  */
	  rs->waiting_for_stop_reply = 0;

	  warning (_("Remote failure reply: %s"), buf);
	  status->set_stopped (GDB_SIGNAL_0);
	  break;
	case 'F':		/* File-I/O request.  */
	  /* GDB may access the inferior memory while handling the File-I/O
	     request, but we don't want GDB accessing memory while waiting
	     for a stop reply.  See the comments in putpkt_binary.  Set
	     waiting_for_stop_reply to 0 temporarily.  */
	  rs->waiting_for_stop_reply = 0;
	  remote_fileio_request (this, buf, rs->ctrlc_pending_p);
	  rs->ctrlc_pending_p = 0;
	  /* GDB handled the File-I/O request, and the target is running
	     again.  Keep waiting for events.  */
	  rs->waiting_for_stop_reply = 1;
	  break;
	case 'N': case 'T': case 'S': case 'X': case 'W': case 'w':
	  {
	    /* There is a stop reply to handle.  */
	    rs->waiting_for_stop_reply = 0;

	    stop_reply
	      = as_stop_reply_up (remote_notif_parse (this,
						      &notif_client_stop,
						      rs->buf.data ()));

	    event_ptid = process_stop_reply (std::move (stop_reply), status);
	    break;
	  }
	case 'O':		/* Console output.  */
	  remote_console_output (buf + 1);
	  break;
	case '\0':
	  if (rs->last_sent_signal != GDB_SIGNAL_0)
	    {
	      /* Zero length reply means that we tried 'S' or 'C' and the
		 remote system doesn't support it.  */
	      target_terminal::ours_for_output ();
	      gdb_printf
		("Can't send signals to this remote system.  %s not sent.\n",
		 gdb_signal_to_name (rs->last_sent_signal));
	      rs->last_sent_signal = GDB_SIGNAL_0;
	      target_terminal::inferior ();

	      strcpy (buf, rs->last_sent_step ? "s" : "c");
	      putpkt (buf);
	      break;
	    }
	  [[fallthrough]];
	default:
	  warning (_("Invalid remote reply: %s"), buf);
	  break;
	}
    }

  if (status->kind () == TARGET_WAITKIND_NO_RESUMED)
    return minus_one_ptid;
  else if (status->kind () == TARGET_WAITKIND_IGNORE)
    {
      /* Nothing interesting happened.  If we're doing a non-blocking
	 poll, we're done.  Otherwise, go back to waiting.  */
      if (options & TARGET_WNOHANG)
	return minus_one_ptid;
      else
	goto again;
    }
  else if (status->kind () != TARGET_WAITKIND_EXITED
	   && status->kind () != TARGET_WAITKIND_SIGNALLED)
    {
      if (event_ptid != null_ptid)
	record_currthread (rs, event_ptid);
      else
	event_ptid = first_remote_resumed_thread (this);
    }
  else
    {
      /* A process exit.  Invalidate our notion of current thread.  */
      record_currthread (rs, minus_one_ptid);
      /* It's possible that the packet did not include a pid.  */
      if (event_ptid == null_ptid)
	event_ptid = first_remote_resumed_thread (this);
      /* EVENT_PTID could still be NULL_PTID.  Double-check.  */
      if (event_ptid == null_ptid)
	event_ptid = magic_null_ptid;
    }

  return event_ptid;
}

/* Wait until the remote machine stops, then return, storing status in
   STATUS just as `wait' would.  */

ptid_t
remote_target::wait (ptid_t ptid, struct target_waitstatus *status,
		     target_wait_flags options)
{
  REMOTE_SCOPED_DEBUG_ENTER_EXIT;

  remote_state *rs = get_remote_state ();

  /* Start by clearing the flag that asks for our wait method to be called,
     we'll mark it again at the end if needed.  If the target is not in
     async mode then the async token should not be marked.  */
  if (target_is_async_p ())
    rs->clear_async_event_handler ();
  else
    gdb_assert (!rs->async_event_handler_marked ());

  ptid_t event_ptid;

  if (target_is_non_stop_p ())
    event_ptid = wait_ns (ptid, status, options);
  else
    event_ptid = wait_as (ptid, status, options);

  if (target_is_async_p ())
    {
      /* If there are events left in the queue, or unacknowledged
	 notifications, then tell the event loop to call us again.  */
      if (!rs->stop_reply_queue.empty ()
	  || rs->notif_state->pending_event[notif_client_stop.id] != nullptr)
	rs->mark_async_event_handler ();
    }

  return event_ptid;
}

/* Fetch a single register using a 'p' packet.  */

int
remote_target::fetch_register_using_p (struct regcache *regcache,
				       packet_reg *reg)
{
  struct gdbarch *gdbarch = regcache->arch ();
  struct remote_state *rs = get_remote_state ();
  char *buf, *p;
  gdb_byte *regp = (gdb_byte *) alloca (register_size (gdbarch, reg->regnum));
  int i;

  if (m_features.packet_support (PACKET_p) == PACKET_DISABLE)
    return 0;

  if (reg->pnum == -1)
    return 0;

  p = rs->buf.data ();
  *p++ = 'p';
  p += hexnumstr (p, reg->pnum);
  *p++ = '\0';
  putpkt (rs->buf);
  getpkt (&rs->buf);

  buf = rs->buf.data ();

  switch (m_features.packet_ok (rs->buf, PACKET_p))
    {
    case PACKET_OK:
      break;
    case PACKET_UNKNOWN:
      return 0;
    case PACKET_ERROR:
      error (_("Could not fetch register \"%s\"; remote failure reply '%s'"),
	     gdbarch_register_name (regcache->arch (), reg->regnum),
	     buf);
    }

  /* If this register is unfetchable, tell the regcache.  */
  if (buf[0] == 'x')
    {
      regcache->raw_supply (reg->regnum, NULL);
      return 1;
    }

  /* Otherwise, parse and supply the value.  */
  p = buf;
  i = 0;
  while (p[0] != 0)
    {
      if (p[1] == 0)
	error (_("fetch_register_using_p: early buf termination"));

      regp[i++] = fromhex (p[0]) * 16 + fromhex (p[1]);
      p += 2;
    }
  regcache->raw_supply (reg->regnum, regp);
  return 1;
}

/* Fetch the registers included in the target's 'g' packet.  */

int
remote_target::send_g_packet ()
{
  struct remote_state *rs = get_remote_state ();
  int buf_len;

  xsnprintf (rs->buf.data (), get_remote_packet_size (), "g");
  putpkt (rs->buf);
  getpkt (&rs->buf);
  if (packet_check_result (rs->buf) == PACKET_ERROR)
    error (_("Could not read registers; remote failure reply '%s'"),
	   rs->buf.data ());

  /* We can get out of synch in various cases.  If the first character
     in the buffer is not a hex character, assume that has happened
     and try to fetch another packet to read.  */
  while ((rs->buf[0] < '0' || rs->buf[0] > '9')
	 && (rs->buf[0] < 'A' || rs->buf[0] > 'F')
	 && (rs->buf[0] < 'a' || rs->buf[0] > 'f')
	 && rs->buf[0] != 'x')	/* New: unavailable register value.  */
    {
      remote_debug_printf ("Bad register packet; fetching a new packet");
      getpkt (&rs->buf);
    }

  buf_len = strlen (rs->buf.data ());

  /* Sanity check the received packet.  */
  if (buf_len % 2 != 0)
    error (_("Remote 'g' packet reply is of odd length: %s"), rs->buf.data ());

  return buf_len / 2;
}

void
remote_target::process_g_packet (struct regcache *regcache)
{
  struct gdbarch *gdbarch = regcache->arch ();
  struct remote_state *rs = get_remote_state ();
  remote_arch_state *rsa = rs->get_remote_arch_state (gdbarch);
  int i, buf_len;
  char *p;
  char *regs;

  buf_len = strlen (rs->buf.data ());

  /* Further sanity checks, with knowledge of the architecture.  */
  if (buf_len > 2 * rsa->sizeof_g_packet)
    error (_("Remote 'g' packet reply is too long (expected %ld bytes, got %d "
	     "bytes): %s"),
	   rsa->sizeof_g_packet, buf_len / 2,
	   rs->buf.data ());

  /* Save the size of the packet sent to us by the target.  It is used
     as a heuristic when determining the max size of packets that the
     target can safely receive.  */
  if (rsa->actual_register_packet_size == 0)
    rsa->actual_register_packet_size = buf_len;

  /* If this is smaller than we guessed the 'g' packet would be,
     update our records.  A 'g' reply that doesn't include a register's
     value implies either that the register is not available, or that
     the 'p' packet must be used.  */
  if (buf_len < 2 * rsa->sizeof_g_packet)
    {
      long sizeof_g_packet = buf_len / 2;

      for (i = 0; i < gdbarch_num_regs (gdbarch); i++)
	{
	  long offset = rsa->regs[i].offset;
	  long reg_size = register_size (gdbarch, i);

	  if (rsa->regs[i].pnum == -1)
	    continue;

	  if (offset >= sizeof_g_packet)
	    rsa->regs[i].in_g_packet = 0;
	  else if (offset + reg_size > sizeof_g_packet)
	    error (_("Truncated register %d in remote 'g' packet"), i);
	  else
	    rsa->regs[i].in_g_packet = 1;
	}

      /* Looks valid enough, we can assume this is the correct length
	 for a 'g' packet.  It's important not to adjust
	 rsa->sizeof_g_packet if we have truncated registers otherwise
	 this "if" won't be run the next time the method is called
	 with a packet of the same size and one of the internal errors
	 below will trigger instead.  */
      rsa->sizeof_g_packet = sizeof_g_packet;
    }

  regs = (char *) alloca (rsa->sizeof_g_packet);

  /* Unimplemented registers read as all bits zero.  */
  memset (regs, 0, rsa->sizeof_g_packet);

  /* Reply describes registers byte by byte, each byte encoded as two
     hex characters.  Suck them all up, then supply them to the
     register cacheing/storage mechanism.  */

  p = rs->buf.data ();
  for (i = 0; i < rsa->sizeof_g_packet; i++)
    {
      if (p[0] == 0 || p[1] == 0)
	/* This shouldn't happen - we adjusted sizeof_g_packet above.  */
	internal_error (_("unexpected end of 'g' packet reply"));

      if (p[0] == 'x' && p[1] == 'x')
	regs[i] = 0;		/* 'x' */
      else
	regs[i] = fromhex (p[0]) * 16 + fromhex (p[1]);
      p += 2;
    }

  for (i = 0; i < gdbarch_num_regs (gdbarch); i++)
    {
      struct packet_reg *r = &rsa->regs[i];
      long reg_size = register_size (gdbarch, i);

      if (r->in_g_packet)
	{
	  if ((r->offset + reg_size) * 2 > strlen (rs->buf.data ()))
	    /* This shouldn't happen - we adjusted in_g_packet above.  */
	    internal_error (_("unexpected end of 'g' packet reply"));
	  else if (rs->buf[r->offset * 2] == 'x')
	    {
	      gdb_assert (r->offset * 2 < strlen (rs->buf.data ()));
	      /* The register isn't available, mark it as such (at
		 the same time setting the value to zero).  */
	      regcache->raw_supply (r->regnum, NULL);
	    }
	  else
	    regcache->raw_supply (r->regnum, regs + r->offset);
	}
    }
}

void
remote_target::fetch_registers_using_g (struct regcache *regcache)
{
  send_g_packet ();
  process_g_packet (regcache);
}

/* Make the remote selected traceframe match GDB's selected
   traceframe.  */

void
remote_target::set_remote_traceframe ()
{
  int newnum;
  struct remote_state *rs = get_remote_state ();

  if (rs->remote_traceframe_number == get_traceframe_number ())
    return;

  /* Avoid recursion, remote_trace_find calls us again.  */
  rs->remote_traceframe_number = get_traceframe_number ();

  newnum = target_trace_find (tfind_number,
			      get_traceframe_number (), 0, 0, NULL);

  /* Should not happen.  If it does, all bets are off.  */
  if (newnum != get_traceframe_number ())
    warning (_("could not set remote traceframe"));
}

void
remote_target::fetch_registers (struct regcache *regcache, int regnum)
{
  struct gdbarch *gdbarch = regcache->arch ();
  struct remote_state *rs = get_remote_state ();
  remote_arch_state *rsa = rs->get_remote_arch_state (gdbarch);
  int i;

  set_remote_traceframe ();
  set_general_thread (regcache->ptid ());

  if (regnum >= 0)
    {
      packet_reg *reg = packet_reg_from_regnum (gdbarch, rsa, regnum);

      gdb_assert (reg != NULL);

      /* If this register might be in the 'g' packet, try that first -
	 we are likely to read more than one register.  If this is the
	 first 'g' packet, we might be overly optimistic about its
	 contents, so fall back to 'p'.  */
      if (reg->in_g_packet)
	{
	  fetch_registers_using_g (regcache);
	  if (reg->in_g_packet)
	    return;
	}

      if (fetch_register_using_p (regcache, reg))
	return;

      /* This register is not available.  */
      regcache->raw_supply (reg->regnum, NULL);

      return;
    }

  fetch_registers_using_g (regcache);

  for (i = 0; i < gdbarch_num_regs (gdbarch); i++)
    if (!rsa->regs[i].in_g_packet)
      if (!fetch_register_using_p (regcache, &rsa->regs[i]))
	{
	  /* This register is not available.  */
	  regcache->raw_supply (i, NULL);
	}
}

/* Prepare to store registers.  Since we may send them all (using a
   'G' request), we have to read out the ones we don't want to change
   first.  */

void
remote_target::prepare_to_store (struct regcache *regcache)
{
  struct remote_state *rs = get_remote_state ();
  remote_arch_state *rsa = rs->get_remote_arch_state (regcache->arch ());
  int i;

  /* Make sure the entire registers array is valid.  */
  switch (m_features.packet_support (PACKET_P))
    {
    case PACKET_DISABLE:
    case PACKET_SUPPORT_UNKNOWN:
      /* Make sure all the necessary registers are cached.  */
      for (i = 0; i < gdbarch_num_regs (regcache->arch ()); i++)
	if (rsa->regs[i].in_g_packet)
	  regcache->raw_update (rsa->regs[i].regnum);
      break;
    case PACKET_ENABLE:
      break;
    }
}

/* Helper: Attempt to store REGNUM using the P packet.  Return fail IFF
   packet was not recognized.  */

int
remote_target::store_register_using_P (const struct regcache *regcache,
				       packet_reg *reg)
{
  struct gdbarch *gdbarch = regcache->arch ();
  struct remote_state *rs = get_remote_state ();
  /* Try storing a single register.  */
  char *buf = rs->buf.data ();
  gdb_byte *regp = (gdb_byte *) alloca (register_size (gdbarch, reg->regnum));
  char *p;

  if (m_features.packet_support (PACKET_P) == PACKET_DISABLE)
    return 0;

  if (reg->pnum == -1)
    return 0;

  xsnprintf (buf, get_remote_packet_size (), "P%s=", phex_nz (reg->pnum, 0));
  p = buf + strlen (buf);
  regcache->raw_collect (reg->regnum, regp);
  bin2hex (regp, p, register_size (gdbarch, reg->regnum));
  putpkt (rs->buf);
  getpkt (&rs->buf);

  switch (m_features.packet_ok (rs->buf, PACKET_P))
    {
    case PACKET_OK:
      return 1;
    case PACKET_ERROR:
      error (_("Could not write register \"%s\"; remote failure reply '%s'"),
	     gdbarch_register_name (gdbarch, reg->regnum), rs->buf.data ());
    case PACKET_UNKNOWN:
      return 0;
    default:
      internal_error (_("Bad result from packet_ok"));
    }
}

/* Store register REGNUM, or all registers if REGNUM == -1, from the
   contents of the register cache buffer.  FIXME: ignores errors.  */

void
remote_target::store_registers_using_G (const struct regcache *regcache)
{
  struct remote_state *rs = get_remote_state ();
  remote_arch_state *rsa = rs->get_remote_arch_state (regcache->arch ());
  gdb_byte *regs;
  char *p;

  /* Extract all the registers in the regcache copying them into a
     local buffer.  */
  {
    int i;

    regs = (gdb_byte *) alloca (rsa->sizeof_g_packet);
    memset (regs, 0, rsa->sizeof_g_packet);
    for (i = 0; i < gdbarch_num_regs (regcache->arch ()); i++)
      {
	struct packet_reg *r = &rsa->regs[i];

	if (r->in_g_packet)
	  regcache->raw_collect (r->regnum, regs + r->offset);
      }
  }

  /* Command describes registers byte by byte,
     each byte encoded as two hex characters.  */
  p = rs->buf.data ();
  *p++ = 'G';
  bin2hex (regs, p, rsa->sizeof_g_packet);
  putpkt (rs->buf);
  getpkt (&rs->buf);
  if (packet_check_result (rs->buf) == PACKET_ERROR)
    error (_("Could not write registers; remote failure reply '%s'"),
	   rs->buf.data ());
}

/* Store register REGNUM, or all registers if REGNUM == -1, from the contents
   of the register cache buffer.  FIXME: ignores errors.  */

void
remote_target::store_registers (struct regcache *regcache, int regnum)
{
  struct gdbarch *gdbarch = regcache->arch ();
  struct remote_state *rs = get_remote_state ();
  remote_arch_state *rsa = rs->get_remote_arch_state (gdbarch);
  int i;

  set_remote_traceframe ();
  set_general_thread (regcache->ptid ());

  if (regnum >= 0)
    {
      packet_reg *reg = packet_reg_from_regnum (gdbarch, rsa, regnum);

      gdb_assert (reg != NULL);

      /* Always prefer to store registers using the 'P' packet if
	 possible; we often change only a small number of registers.
	 Sometimes we change a larger number; we'd need help from a
	 higher layer to know to use 'G'.  */
      if (store_register_using_P (regcache, reg))
	return;

      /* For now, don't complain if we have no way to write the
	 register.  GDB loses track of unavailable registers too
	 easily.  Some day, this may be an error.  We don't have
	 any way to read the register, either...  */
      if (!reg->in_g_packet)
	return;

      store_registers_using_G (regcache);
      return;
    }

  store_registers_using_G (regcache);

  for (i = 0; i < gdbarch_num_regs (gdbarch); i++)
    if (!rsa->regs[i].in_g_packet)
      if (!store_register_using_P (regcache, &rsa->regs[i]))
	/* See above for why we do not issue an error here.  */
	continue;
}


/* Return the number of hex digits in num.  */

static int
hexnumlen (ULONGEST num)
{
  int i;

  for (i = 0; num != 0; i++)
    num >>= 4;

  return std::max (i, 1);
}

/* Set BUF to the minimum number of hex digits representing NUM.  */

static int
hexnumstr (char *buf, ULONGEST num)
{
  int len = hexnumlen (num);

  return hexnumnstr (buf, num, len);
}


/* Set BUF to the hex digits representing NUM, padded to WIDTH characters.  */

static int
hexnumnstr (char *buf, ULONGEST num, int width)
{
  int i;

  buf[width] = '\0';

  for (i = width - 1; i >= 0; i--)
    {
      buf[i] = "0123456789abcdef"[(num & 0xf)];
      num >>= 4;
    }

  return width;
}

/* Mask all but the least significant REMOTE_ADDRESS_SIZE bits.  */

static CORE_ADDR
remote_address_masked (CORE_ADDR addr)
{
  unsigned int address_size = remote_address_size;

  /* If "remoteaddresssize" was not set, default to target address size.  */
  if (!address_size)
    address_size = gdbarch_addr_bit (current_inferior ()->arch ());

  if (address_size > 0
      && address_size < (sizeof (ULONGEST) * 8))
    {
      /* Only create a mask when that mask can safely be constructed
	 in a ULONGEST variable.  */
      ULONGEST mask = 1;

      mask = (mask << address_size) - 1;
      addr &= mask;
    }
  return addr;
}

/* Determine whether the remote target supports binary downloading.
   This is accomplished by sending a no-op memory write of zero length
   to the target at the specified address. It does not suffice to send
   the whole packet, since many stubs strip the eighth bit and
   subsequently compute a wrong checksum, which causes real havoc with
   remote_write_bytes.

   NOTE: This can still lose if the serial line is not eight-bit
   clean.  In cases like this, the user should clear "remote
   X-packet".  */

void
remote_target::check_binary_download (CORE_ADDR addr)
{
  struct remote_state *rs = get_remote_state ();

  switch (m_features.packet_support (PACKET_X))
    {
    case PACKET_DISABLE:
      break;
    case PACKET_ENABLE:
      break;
    case PACKET_SUPPORT_UNKNOWN:
      {
	char *p;

	p = rs->buf.data ();
	*p++ = 'X';
	p += hexnumstr (p, (ULONGEST) addr);
	*p++ = ',';
	p += hexnumstr (p, (ULONGEST) 0);
	*p++ = ':';
	*p = '\0';

	putpkt_binary (rs->buf.data (), (int) (p - rs->buf.data ()));
	getpkt (&rs->buf);

	if (rs->buf[0] == '\0')
	  {
	    remote_debug_printf ("binary downloading NOT supported by target");
	    m_features.m_protocol_packets[PACKET_X].support = PACKET_DISABLE;
	  }
	else
	  {
	    remote_debug_printf ("binary downloading supported by target");
	    m_features.m_protocol_packets[PACKET_X].support = PACKET_ENABLE;
	  }
	break;
      }
    }
}

/* Helper function to resize the payload in order to try to get a good
   alignment.  We try to write an amount of data such that the next write will
   start on an address aligned on REMOTE_ALIGN_WRITES.  */

static int
align_for_efficient_write (int todo, CORE_ADDR memaddr)
{
  return ((memaddr + todo) & ~(REMOTE_ALIGN_WRITES - 1)) - memaddr;
}

/* Write memory data directly to the remote machine.
   This does not inform the data cache; the data cache uses this.
   HEADER is the starting part of the packet.
   MEMADDR is the address in the remote memory space.
   MYADDR is the address of the buffer in our space.
   LEN_UNITS is the number of addressable units to write.
   UNIT_SIZE is the length in bytes of an addressable unit.
   PACKET_FORMAT should be either 'X' or 'M', and indicates if we
   should send data as binary ('X'), or hex-encoded ('M').

   The function creates packet of the form
       <HEADER><ADDRESS>,<LENGTH>:<DATA>

   where encoding of <DATA> is terminated by PACKET_FORMAT.

   If USE_LENGTH is 0, then the <LENGTH> field and the preceding comma
   are omitted.

   Return the transferred status, error or OK (an
   'enum target_xfer_status' value).  Save the number of addressable units
   transferred in *XFERED_LEN_UNITS.  Only transfer a single packet.

   On a platform with an addressable memory size of 2 bytes (UNIT_SIZE == 2), an
   exchange between gdb and the stub could look like (?? in place of the
   checksum):

   -> $m1000,4#??
   <- aaaabbbbccccdddd

   -> $M1000,3:eeeeffffeeee#??
   <- OK

   -> $m1000,4#??
   <- eeeeffffeeeedddd  */

target_xfer_status
remote_target::remote_write_bytes_aux (const char *header, CORE_ADDR memaddr,
				       const gdb_byte *myaddr,
				       ULONGEST len_units,
				       int unit_size,
				       ULONGEST *xfered_len_units,
				       char packet_format, int use_length)
{
  struct remote_state *rs = get_remote_state ();
  char *p;
  char *plen = NULL;
  int plenlen = 0;
  int todo_units;
  int units_written;
  int payload_capacity_bytes;
  int payload_length_bytes;

  if (packet_format != 'X' && packet_format != 'M')
    internal_error (_("remote_write_bytes_aux: bad packet format"));

  if (len_units == 0)
    return TARGET_XFER_EOF;

  payload_capacity_bytes = get_memory_write_packet_size ();

  /* The packet buffer will be large enough for the payload;
     get_memory_packet_size ensures this.  */
  rs->buf[0] = '\0';

  /* Compute the size of the actual payload by subtracting out the
     packet header and footer overhead: "$M<memaddr>,<len>:...#nn".  */

  payload_capacity_bytes -= strlen ("$,:#NN");
  if (!use_length)
    /* The comma won't be used.  */
    payload_capacity_bytes += 1;
  payload_capacity_bytes -= strlen (header);
  payload_capacity_bytes -= hexnumlen (memaddr);

  /* Construct the packet excluding the data: "<header><memaddr>,<len>:".  */

  strcat (rs->buf.data (), header);
  p = rs->buf.data () + strlen (header);

  /* Compute a best guess of the number of bytes actually transfered.  */
  if (packet_format == 'X')
    {
      /* Best guess at number of bytes that will fit.  */
      todo_units = std::min (len_units,
			     (ULONGEST) payload_capacity_bytes / unit_size);
      if (use_length)
	payload_capacity_bytes -= hexnumlen (todo_units);
      todo_units = std::min (todo_units, payload_capacity_bytes / unit_size);
    }
  else
    {
      /* Number of bytes that will fit.  */
      todo_units
	= std::min (len_units,
		    (ULONGEST) (payload_capacity_bytes / unit_size) / 2);
      if (use_length)
	payload_capacity_bytes -= hexnumlen (todo_units);
      todo_units = std::min (todo_units,
			     (payload_capacity_bytes / unit_size) / 2);
    }

  if (todo_units <= 0)
    internal_error (_("minimum packet size too small to write data"));

  /* If we already need another packet, then try to align the end
     of this packet to a useful boundary.  */
  if (todo_units > 2 * REMOTE_ALIGN_WRITES && todo_units < len_units)
    todo_units = align_for_efficient_write (todo_units, memaddr);

  /* Append "<memaddr>".  */
  memaddr = remote_address_masked (memaddr);
  p += hexnumstr (p, (ULONGEST) memaddr);

  if (use_length)
    {
      /* Append ",".  */
      *p++ = ',';

      /* Append the length and retain its location and size.  It may need to be
	 adjusted once the packet body has been created.  */
      plen = p;
      plenlen = hexnumstr (p, (ULONGEST) todo_units);
      p += plenlen;
    }

  /* Append ":".  */
  *p++ = ':';
  *p = '\0';

  /* Append the packet body.  */
  if (packet_format == 'X')
    {
      /* Binary mode.  Send target system values byte by byte, in
	 increasing byte addresses.  Only escape certain critical
	 characters.  */
      payload_length_bytes =
	  remote_escape_output (myaddr, todo_units, unit_size, (gdb_byte *) p,
				&units_written, payload_capacity_bytes);

      /* If not all TODO units fit, then we'll need another packet.  Make
	 a second try to keep the end of the packet aligned.  Don't do
	 this if the packet is tiny.  */
      if (units_written < todo_units && units_written > 2 * REMOTE_ALIGN_WRITES)
	{
	  int new_todo_units;

	  new_todo_units = align_for_efficient_write (units_written, memaddr);

	  if (new_todo_units != units_written)
	    payload_length_bytes =
		remote_escape_output (myaddr, new_todo_units, unit_size,
				      (gdb_byte *) p, &units_written,
				      payload_capacity_bytes);
	}

      p += payload_length_bytes;
      if (use_length && units_written < todo_units)
	{
	  /* Escape chars have filled up the buffer prematurely,
	     and we have actually sent fewer units than planned.
	     Fix-up the length field of the packet.  Use the same
	     number of characters as before.  */
	  plen += hexnumnstr (plen, (ULONGEST) units_written,
			      plenlen);
	  *plen = ':';  /* overwrite \0 from hexnumnstr() */
	}
    }
  else
    {
      /* Normal mode: Send target system values byte by byte, in
	 increasing byte addresses.  Each byte is encoded as a two hex
	 value.  */
      p += 2 * bin2hex (myaddr, p, todo_units * unit_size);
      units_written = todo_units;
    }

  putpkt_binary (rs->buf.data (), (int) (p - rs->buf.data ()));
  getpkt (&rs->buf);

  if (rs->buf[0] == 'E')
    return TARGET_XFER_E_IO;

  /* Return UNITS_WRITTEN, not TODO_UNITS, in case escape chars caused us to
     send fewer units than we'd planned.  */
  *xfered_len_units = (ULONGEST) units_written;
  return (*xfered_len_units != 0) ? TARGET_XFER_OK : TARGET_XFER_EOF;
}

/* Write memory data directly to the remote machine.
   This does not inform the data cache; the data cache uses this.
   MEMADDR is the address in the remote memory space.
   MYADDR is the address of the buffer in our space.
   LEN is the number of bytes.

   Return the transferred status, error or OK (an
   'enum target_xfer_status' value).  Save the number of bytes
   transferred in *XFERED_LEN.  Only transfer a single packet.  */

target_xfer_status
remote_target::remote_write_bytes (CORE_ADDR memaddr, const gdb_byte *myaddr,
				   ULONGEST len, int unit_size,
				   ULONGEST *xfered_len)
{
  const char *packet_format = NULL;

  /* Check whether the target supports binary download.  */
  check_binary_download (memaddr);

  switch (m_features.packet_support (PACKET_X))
    {
    case PACKET_ENABLE:
      packet_format = "X";
      break;
    case PACKET_DISABLE:
      packet_format = "M";
      break;
    case PACKET_SUPPORT_UNKNOWN:
      internal_error (_("remote_write_bytes: bad internal state"));
    default:
      internal_error (_("bad switch"));
    }

  return remote_write_bytes_aux (packet_format,
				 memaddr, myaddr, len, unit_size, xfered_len,
				 packet_format[0], 1);
}

/* Read memory data directly from the remote machine.
   This does not use the data cache; the data cache uses this.
   MEMADDR is the address in the remote memory space.
   MYADDR is the address of the buffer in our space.
   LEN_UNITS is the number of addressable memory units to read..
   UNIT_SIZE is the length in bytes of an addressable unit.

   Return the transferred status, error or OK (an
   'enum target_xfer_status' value).  Save the number of bytes
   transferred in *XFERED_LEN_UNITS.

   See the comment of remote_write_bytes_aux for an example of
   memory read/write exchange between gdb and the stub.  */

target_xfer_status
remote_target::remote_read_bytes_1 (CORE_ADDR memaddr, gdb_byte *myaddr,
				    ULONGEST len_units,
				    int unit_size, ULONGEST *xfered_len_units)
{
  struct remote_state *rs = get_remote_state ();
  int buf_size_bytes;		/* Max size of packet output buffer.  */
  char *p;
  int todo_units;
  int decoded_bytes;

  buf_size_bytes = get_memory_read_packet_size ();
  /* The packet buffer will be large enough for the payload;
     get_memory_packet_size ensures this.  */

  /* Number of units that will fit.  */
  todo_units = std::min (len_units,
			 (ULONGEST) (buf_size_bytes / unit_size) / 2);

  /* Construct "m"<memaddr>","<len>".  */
  memaddr = remote_address_masked (memaddr);
  p = rs->buf.data ();
  *p++ = 'm';
  p += hexnumstr (p, (ULONGEST) memaddr);
  *p++ = ',';
  p += hexnumstr (p, (ULONGEST) todo_units);
  *p = '\0';
  putpkt (rs->buf);
  getpkt (&rs->buf);
  if (rs->buf[0] == 'E'
      && isxdigit (rs->buf[1]) && isxdigit (rs->buf[2])
      && rs->buf[3] == '\0')
    return TARGET_XFER_E_IO;
  /* Reply describes memory byte by byte, each byte encoded as two hex
     characters.  */
  p = rs->buf.data ();
  decoded_bytes = hex2bin (p, myaddr, todo_units * unit_size);
  /* Return what we have.  Let higher layers handle partial reads.  */
  *xfered_len_units = (ULONGEST) (decoded_bytes / unit_size);
  return (*xfered_len_units != 0) ? TARGET_XFER_OK : TARGET_XFER_EOF;
}

/* Using the set of read-only target sections of remote, read live
   read-only memory.

   For interface/parameters/return description see target.h,
   to_xfer_partial.  */

target_xfer_status
remote_target::remote_xfer_live_readonly_partial (gdb_byte *readbuf,
						  ULONGEST memaddr,
						  ULONGEST len,
						  int unit_size,
						  ULONGEST *xfered_len)
{
  const struct target_section *secp;

  secp = target_section_by_addr (this, memaddr);
  if (secp != NULL
      && (bfd_section_flags (secp->the_bfd_section) & SEC_READONLY))
    {
      ULONGEST memend = memaddr + len;

      const std::vector<target_section> *table
	= target_get_section_table (this);
      for (const target_section &p : *table)
	{
	  if (memaddr >= p.addr)
	    {
	      if (memend <= p.endaddr)
		{
		  /* Entire transfer is within this section.  */
		  return remote_read_bytes_1 (memaddr, readbuf, len, unit_size,
					      xfered_len);
		}
	      else if (memaddr >= p.endaddr)
		{
		  /* This section ends before the transfer starts.  */
		  continue;
		}
	      else
		{
		  /* This section overlaps the transfer.  Just do half.  */
		  len = p.endaddr - memaddr;
		  return remote_read_bytes_1 (memaddr, readbuf, len, unit_size,
					      xfered_len);
		}
	    }
	}
    }

  return TARGET_XFER_EOF;
}

/* Similar to remote_read_bytes_1, but it reads from the remote stub
   first if the requested memory is unavailable in traceframe.
   Otherwise, fall back to remote_read_bytes_1.  */

target_xfer_status
remote_target::remote_read_bytes (CORE_ADDR memaddr,
				  gdb_byte *myaddr, ULONGEST len, int unit_size,
				  ULONGEST *xfered_len)
{
  if (len == 0)
    return TARGET_XFER_EOF;

  if (get_traceframe_number () != -1)
    {
      std::vector<mem_range> available;

      /* If we fail to get the set of available memory, then the
	 target does not support querying traceframe info, and so we
	 attempt reading from the traceframe anyway (assuming the
	 target implements the old QTro packet then).  */
      if (traceframe_available_memory (&available, memaddr, len))
	{
	  if (available.empty () || available[0].start != memaddr)
	    {
	      enum target_xfer_status res;

	      /* Don't read into the traceframe's available
		 memory.  */
	      if (!available.empty ())
		{
		  LONGEST oldlen = len;

		  len = available[0].start - memaddr;
		  gdb_assert (len <= oldlen);
		}

	      /* This goes through the topmost target again.  */
	      res = remote_xfer_live_readonly_partial (myaddr, memaddr,
						       len, unit_size, xfered_len);
	      if (res == TARGET_XFER_OK)
		return TARGET_XFER_OK;
	      else
		{
		  /* No use trying further, we know some memory starting
		     at MEMADDR isn't available.  */
		  *xfered_len = len;
		  return (*xfered_len != 0) ?
		    TARGET_XFER_UNAVAILABLE : TARGET_XFER_EOF;
		}
	    }

	  /* Don't try to read more than how much is available, in
	     case the target implements the deprecated QTro packet to
	     cater for older GDBs (the target's knowledge of read-only
	     sections may be outdated by now).  */
	  len = available[0].length;
	}
    }

  return remote_read_bytes_1 (memaddr, myaddr, len, unit_size, xfered_len);
}



/* Sends a packet with content determined by the printf format string
   FORMAT and the remaining arguments, then gets the reply.  Returns
   whether the packet was a success, a failure, or unknown.  */

packet_result
remote_target::remote_send_printf (const char *format, ...)
{
  struct remote_state *rs = get_remote_state ();
  int max_size = get_remote_packet_size ();
  va_list ap;

  va_start (ap, format);

  rs->buf[0] = '\0';
  int size = vsnprintf (rs->buf.data (), max_size, format, ap);

  va_end (ap);

  if (size >= max_size)
    internal_error (_("Too long remote packet."));

  if (putpkt (rs->buf) < 0)
    error (_("Communication problem with target."));

  rs->buf[0] = '\0';
  getpkt (&rs->buf);

  return packet_check_result (rs->buf);
}

/* Flash writing can take quite some time.  We'll set
   effectively infinite timeout for flash operations.
   In future, we'll need to decide on a better approach.  */
static const int remote_flash_timeout = 1000;

void
remote_target::flash_erase (ULONGEST address, LONGEST length)
{
  int addr_size = gdbarch_addr_bit (current_inferior ()->arch ()) / 8;
  enum packet_result ret;
  scoped_restore restore_timeout
    = make_scoped_restore (&remote_timeout, remote_flash_timeout);

  ret = remote_send_printf ("vFlashErase:%s,%s",
			    phex (address, addr_size),
			    phex (length, 4));
  switch (ret)
    {
    case PACKET_UNKNOWN:
      error (_("Remote target does not support flash erase"));
    case PACKET_ERROR:
      error (_("Error erasing flash with vFlashErase packet"));
    default:
      break;
    }
}

target_xfer_status
remote_target::remote_flash_write (ULONGEST address,
				   ULONGEST length, ULONGEST *xfered_len,
				   const gdb_byte *data)
{
  scoped_restore restore_timeout
    = make_scoped_restore (&remote_timeout, remote_flash_timeout);
  return remote_write_bytes_aux ("vFlashWrite:", address, data, length, 1,
				 xfered_len,'X', 0);
}

void
remote_target::flash_done ()
{
  int ret;

  scoped_restore restore_timeout
    = make_scoped_restore (&remote_timeout, remote_flash_timeout);

  ret = remote_send_printf ("vFlashDone");

  switch (ret)
    {
    case PACKET_UNKNOWN:
      error (_("Remote target does not support vFlashDone"));
    case PACKET_ERROR:
      error (_("Error finishing flash operation"));
    default:
      break;
    }
}


/* Stuff for dealing with the packets which are part of this protocol.
   See comment at top of file for details.  */

/* Read a single character from the remote end.  The current quit
   handler is overridden to avoid quitting in the middle of packet
   sequence, as that would break communication with the remote server.
   See remote_serial_quit_handler for more detail.  */

int
remote_target::readchar (int timeout)
{
  int ch;
  struct remote_state *rs = get_remote_state ();

  try
    {
      scoped_restore restore_quit_target
	= make_scoped_restore (&curr_quit_handler_target, this);
      scoped_restore restore_quit
	= make_scoped_restore (&quit_handler, ::remote_serial_quit_handler);

      rs->got_ctrlc_during_io = 0;

      ch = serial_readchar (rs->remote_desc, timeout);

      if (rs->got_ctrlc_during_io)
	set_quit_flag ();
    }
  catch (const gdb_exception_error &ex)
    {
      remote_unpush_target (this);
      throw_error (TARGET_CLOSE_ERROR,
		   _("Remote communication error.  "
		     "Target disconnected: %s"),
		   ex.what ());
    }

  if (ch >= 0)
    return ch;

  if (ch == SERIAL_EOF)
    {
      remote_unpush_target (this);
      throw_error (TARGET_CLOSE_ERROR, _("Remote connection closed"));
    }

  return ch;
}

/* Wrapper for serial_write that closes the target and throws if
   writing fails.  The current quit handler is overridden to avoid
   quitting in the middle of packet sequence, as that would break
   communication with the remote server.  See
   remote_serial_quit_handler for more detail.  */

void
remote_target::remote_serial_write (const char *str, int len)
{
  struct remote_state *rs = get_remote_state ();

  scoped_restore restore_quit_target
    = make_scoped_restore (&curr_quit_handler_target, this);
  scoped_restore restore_quit
    = make_scoped_restore (&quit_handler, ::remote_serial_quit_handler);

  rs->got_ctrlc_during_io = 0;

  try
    {
      serial_write (rs->remote_desc, str, len);
    }
  catch (const gdb_exception_error &ex)
    {
      remote_unpush_target (this);
      throw_error (TARGET_CLOSE_ERROR,
		   _("Remote communication error.  "
		     "Target disconnected: %s"),
		   ex.what ());
    }

  if (rs->got_ctrlc_during_io)
    set_quit_flag ();
}

void
remote_target::remote_serial_send_break ()
{
  struct remote_state *rs = get_remote_state ();

  try
    {
      serial_send_break (rs->remote_desc);
    }
  catch (const gdb_exception_error &ex)
    {
      remote_unpush_target (this);
      throw_error (TARGET_CLOSE_ERROR,
		   _("Remote communication error.  "
		     "Target disconnected: %s"),
		   ex.what ());
    }
}

/* Return a string representing an escaped version of BUF, of len N.
   E.g. \n is converted to \\n, \t to \\t, etc.  */

static std::string
escape_buffer (const char *buf, int n)
{
  string_file stb;

  stb.putstrn (buf, n, '\\');
  return stb.release ();
}

int
remote_target::putpkt (const char *buf)
{
  return putpkt_binary (buf, strlen (buf));
}

/* Wrapper around remote_target::putpkt to avoid exporting
   remote_target.  */

int
putpkt (remote_target *remote, const char *buf)
{
  return remote->putpkt (buf);
}

/* Send a packet to the remote machine, with error checking.  The data
   of the packet is in BUF.  The string in BUF can be at most
   get_remote_packet_size () - 5 to account for the $, # and checksum,
   and for a possible /0 if we are debugging (remote_debug) and want
   to print the sent packet as a string.  */

int
remote_target::putpkt_binary (const char *buf, int cnt)
{
  struct remote_state *rs = get_remote_state ();
  int i;
  unsigned char csum = 0;
  gdb::def_vector<char> data (cnt + 6);
  char *buf2 = data.data ();

  int ch;
  int tcount = 0;
  char *p;

  /* Catch cases like trying to read memory or listing threads while
     we're waiting for a stop reply.  The remote server wouldn't be
     ready to handle this request, so we'd hang and timeout.  We don't
     have to worry about this in synchronous mode, because in that
     case it's not possible to issue a command while the target is
     running.  This is not a problem in non-stop mode, because in that
     case, the stub is always ready to process serial input.  */
  if (!target_is_non_stop_p ()
      && target_is_async_p ()
      && rs->waiting_for_stop_reply)
    {
      error (_("Cannot execute this command while the target is running.\n"
	       "Use the \"interrupt\" command to stop the target\n"
	       "and then try again."));
    }

  /* Copy the packet into buffer BUF2, encapsulating it
     and giving it a checksum.  */

  p = buf2;
  *p++ = '$';

  for (i = 0; i < cnt; i++)
    {
      csum += buf[i];
      *p++ = buf[i];
    }
  *p++ = '#';
  *p++ = tohex ((csum >> 4) & 0xf);
  *p++ = tohex (csum & 0xf);

  /* Send it over and over until we get a positive ack.  */

  while (1)
    {
      if (remote_debug)
	{
	  *p = '\0';

	  int len = (int) (p - buf2);
	  int max_chars;

	  if (remote_packet_max_chars < 0)
	    max_chars = len;
	  else
	    max_chars = remote_packet_max_chars;

	  std::string str
	    = escape_buffer (buf2, std::min (len, max_chars));

	  if (len > max_chars)
	    remote_debug_printf_nofunc
	      ("Sending packet: %s [%d bytes omitted]", str.c_str (),
	       len - max_chars);
	  else
	    remote_debug_printf_nofunc ("Sending packet: %s", str.c_str ());
	}
      remote_serial_write (buf2, p - buf2);

      /* If this is a no acks version of the remote protocol, send the
	 packet and move on.  */
      if (rs->noack_mode)
	break;

      /* Read until either a timeout occurs (-2) or '+' is read.
	 Handle any notification that arrives in the mean time.  */
      while (1)
	{
	  ch = readchar (remote_timeout);

	  switch (ch)
	    {
	    case '+':
	      remote_debug_printf_nofunc ("Received Ack");
	      return 1;
	    case '-':
	      remote_debug_printf_nofunc ("Received Nak");
	      [[fallthrough]];
	    case SERIAL_TIMEOUT:
	      tcount++;
	      if (tcount > 3)
		return 0;
	      break;		/* Retransmit buffer.  */
	    case '$':
	      {
		remote_debug_printf ("Packet instead of Ack, ignoring it");
		/* It's probably an old response sent because an ACK
		   was lost.  Gobble up the packet and ack it so it
		   doesn't get retransmitted when we resend this
		   packet.  */
		skip_frame ();
		remote_serial_write ("+", 1);
		continue;	/* Now, go look for +.  */
	      }

	    case '%':
	      {
		int val;

		/* If we got a notification, handle it, and go back to looking
		   for an ack.  */
		/* We've found the start of a notification.  Now
		   collect the data.  */
		val = read_frame (&rs->buf);
		if (val >= 0)
		  {
		    remote_debug_printf_nofunc
		      ("  Notification received: %s",
		       escape_buffer (rs->buf.data (), val).c_str ());

		    handle_notification (rs->notif_state, rs->buf.data ());
		    /* We're in sync now, rewait for the ack.  */
		    tcount = 0;
		  }
		else
		  remote_debug_printf_nofunc ("Junk: %c%s", ch & 0177,
					      rs->buf.data ());
		continue;
	      }
	    default:
	      remote_debug_printf_nofunc ("Junk: %c%s", ch & 0177,
					  rs->buf.data ());
	      continue;
	    }
	  break;		/* Here to retransmit.  */
	}

#if 0
      /* This is wrong.  If doing a long backtrace, the user should be
	 able to get out next time we call QUIT, without anything as
	 violent as interrupt_query.  If we want to provide a way out of
	 here without getting to the next QUIT, it should be based on
	 hitting ^C twice as in remote_wait.  */
      if (quit_flag)
	{
	  quit_flag = 0;
	  interrupt_query ();
	}
#endif
    }

  return 0;
}

/* Come here after finding the start of a frame when we expected an
   ack.  Do our best to discard the rest of this packet.  */

void
remote_target::skip_frame ()
{
  int c;

  while (1)
    {
      c = readchar (remote_timeout);
      switch (c)
	{
	case SERIAL_TIMEOUT:
	  /* Nothing we can do.  */
	  return;
	case '#':
	  /* Discard the two bytes of checksum and stop.  */
	  c = readchar (remote_timeout);
	  if (c >= 0)
	    c = readchar (remote_timeout);

	  return;
	case '*':		/* Run length encoding.  */
	  /* Discard the repeat count.  */
	  c = readchar (remote_timeout);
	  if (c < 0)
	    return;
	  break;
	default:
	  /* A regular character.  */
	  break;
	}
    }
}

/* Come here after finding the start of the frame.  Collect the rest
   into *BUF, verifying the checksum, length, and handling run-length
   compression.  NUL terminate the buffer.  If there is not enough room,
   expand *BUF.

   Returns -1 on error, number of characters in buffer (ignoring the
   trailing NULL) on success. (could be extended to return one of the
   SERIAL status indications).  */

long
remote_target::read_frame (gdb::char_vector *buf_p)
{
  unsigned char csum;
  long bc;
  int c;
  char *buf = buf_p->data ();
  struct remote_state *rs = get_remote_state ();

  csum = 0;
  bc = 0;

  while (1)
    {
      c = readchar (remote_timeout);
      switch (c)
	{
	case SERIAL_TIMEOUT:
	  remote_debug_printf ("Timeout in mid-packet, retrying");
	  return -1;

	case '$':
	  remote_debug_printf ("Saw new packet start in middle of old one");
	  return -1;		/* Start a new packet, count retries.  */

	case '#':
	  {
	    unsigned char pktcsum;
	    int check_0 = 0;
	    int check_1 = 0;

	    buf[bc] = '\0';

	    check_0 = readchar (remote_timeout);
	    if (check_0 >= 0)
	      check_1 = readchar (remote_timeout);

	    if (check_0 == SERIAL_TIMEOUT || check_1 == SERIAL_TIMEOUT)
	      {
		remote_debug_printf ("Timeout in checksum, retrying");
		return -1;
	      }
	    else if (check_0 < 0 || check_1 < 0)
	      {
		remote_debug_printf ("Communication error in checksum");
		return -1;
	      }

	    /* Don't recompute the checksum; with no ack packets we
	       don't have any way to indicate a packet retransmission
	       is necessary.  */
	    if (rs->noack_mode)
	      return bc;

	    pktcsum = (fromhex (check_0) << 4) | fromhex (check_1);
	    if (csum == pktcsum)
	      return bc;

	    remote_debug_printf
	      ("Bad checksum, sentsum=0x%x, csum=0x%x, buf=%s",
	       pktcsum, csum, escape_buffer (buf, bc).c_str ());

	    /* Number of characters in buffer ignoring trailing
	       NULL.  */
	    return -1;
	  }
	case '*':		/* Run length encoding.  */
	  {
	    int repeat;

	    csum += c;
	    c = readchar (remote_timeout);
	    csum += c;
	    repeat = c - ' ' + 3;	/* Compute repeat count.  */

	    /* The character before ``*'' is repeated.  */

	    if (repeat > 0 && repeat <= 255 && bc > 0)
	      {
		if (bc + repeat - 1 >= buf_p->size () - 1)
		  {
		    /* Make some more room in the buffer.  */
		    buf_p->resize (buf_p->size () + repeat);
		    buf = buf_p->data ();
		  }

		memset (&buf[bc], buf[bc - 1], repeat);
		bc += repeat;
		continue;
	      }

	    buf[bc] = '\0';
	    gdb_printf (_("Invalid run length encoding: %s\n"), buf);
	    return -1;
	  }
	default:
	  if (bc >= buf_p->size () - 1)
	    {
	      /* Make some more room in the buffer.  */
	      buf_p->resize (buf_p->size () * 2);
	      buf = buf_p->data ();
	    }

	  buf[bc++] = c;
	  csum += c;
	  continue;
	}
    }
}

/* Set this to the maximum number of seconds to wait instead of waiting forever
   in target_wait().  If this timer times out, then it generates an error and
   the command is aborted.  This replaces most of the need for timeouts in the
   GDB test suite, and makes it possible to distinguish between a hung target
   and one with slow communications.  */

static int watchdog = 0;
static void
show_watchdog (struct ui_file *file, int from_tty,
	       struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Watchdog timer is %s.\n"), value);
}

/* Read a packet from the remote machine, with error checking, and
   store it in *BUF.  Resize *BUF if necessary to hold the result.  If
   FOREVER, wait forever rather than timing out; this is used (in
   synchronous mode) to wait for a target that is is executing user
   code to stop.  If FOREVER == false, this function is allowed to time
   out gracefully and return an indication of this to the caller.
   Otherwise return the number of bytes read.  If IS_NOTIF is not
   NULL, then consider receiving a notification enough reason to
   return to the caller.  In this case, *IS_NOTIF is an output boolean
   that indicates whether *BUF holds a notification or not (a regular
   packet).  */

int
remote_target::getpkt (gdb::char_vector *buf, bool forever, bool *is_notif)
{
  struct remote_state *rs = get_remote_state ();
  int c;
  int tries;
  int timeout;
  int val = -1;

  strcpy (buf->data (), "timeout");

  if (forever)
    timeout = watchdog > 0 ? watchdog : -1;
  else if (is_notif != nullptr)
    timeout = 0; /* There should already be a char in the buffer.  If
		    not, bail out.  */
  else
    timeout = remote_timeout;

#define MAX_TRIES 3

  /* Process any number of notifications, and then return when
     we get a packet.  */
  for (;;)
    {
      /* If we get a timeout or bad checksum, retry up to MAX_TRIES
	 times.  */
      for (tries = 1; tries <= MAX_TRIES; tries++)
	{
	  /* This can loop forever if the remote side sends us
	     characters continuously, but if it pauses, we'll get
	     SERIAL_TIMEOUT from readchar because of timeout.  Then
	     we'll count that as a retry.

	     Note that even when forever is set, we will only wait
	     forever prior to the start of a packet.  After that, we
	     expect characters to arrive at a brisk pace.  They should
	     show up within remote_timeout intervals.  */
	  do
	    c = readchar (timeout);
	  while (c != SERIAL_TIMEOUT && c != '$' && c != '%');

	  if (c == SERIAL_TIMEOUT)
	    {
	      if (is_notif != nullptr)
		return -1; /* Don't complain, it's normal to not get
			      anything in this case.  */

	      if (forever)	/* Watchdog went off?  Kill the target.  */
		{
		  remote_unpush_target (this);
		  throw_error (TARGET_CLOSE_ERROR,
			       _("Watchdog timeout has expired.  "
				 "Target detached."));
		}

	      remote_debug_printf ("Timed out.");
	    }
	  else
	    {
	      /* We've found the start of a packet or notification.
		 Now collect the data.  */
	      val = read_frame (buf);
	      if (val >= 0)
		break;
	    }

	  remote_serial_write ("-", 1);
	}

      if (tries > MAX_TRIES)
	{
	  /* We have tried hard enough, and just can't receive the
	     packet/notification.  Give up.  */
	  gdb_printf (_("Ignoring packet error, continuing...\n"));

	  /* Skip the ack char if we're in no-ack mode.  */
	  if (!rs->noack_mode)
	    remote_serial_write ("+", 1);
	  return -1;
	}

      /* If we got an ordinary packet, return that to our caller.  */
      if (c == '$')
	{
	  if (remote_debug)
	    {
	      int max_chars;

	      if (remote_packet_max_chars < 0)
		max_chars = val;
	      else
		max_chars = remote_packet_max_chars;

	      std::string str
		= escape_buffer (buf->data (),
				 std::min (val, max_chars));

	      if (val > max_chars)
		remote_debug_printf_nofunc
		  ("Packet received: %s [%d bytes omitted]", str.c_str (),
		   val - max_chars);
	      else
		remote_debug_printf_nofunc ("Packet received: %s",
					    str.c_str ());
	    }

	  /* Skip the ack char if we're in no-ack mode.  */
	  if (!rs->noack_mode)
	    remote_serial_write ("+", 1);
	  if (is_notif != NULL)
	    *is_notif = false;
	  return val;
	}

       /* If we got a notification, handle it, and go back to looking
	 for a packet.  */
      else
	{
	  gdb_assert (c == '%');

	  remote_debug_printf_nofunc
	    ("  Notification received: %s",
	     escape_buffer (buf->data (), val).c_str ());

	  if (is_notif != NULL)
	    *is_notif = true;

	  handle_notification (rs->notif_state, buf->data ());

	  /* Notifications require no acknowledgement.  */

	  if (is_notif != nullptr)
	    return val;
	}
    }
}

/* Kill any new fork children of inferior INF that haven't been
   processed by follow_fork.  */

void
remote_target::kill_new_fork_children (inferior *inf)
{
  remote_state *rs = get_remote_state ();
  const notif_client *notif = &notif_client_stop;

  /* Kill the fork child threads of any threads in inferior INF that are stopped
     at a fork event.  */
  for (thread_info *thread : inf->non_exited_threads ())
    {
      const target_waitstatus *ws = thread_pending_fork_status (thread);

      if (ws == nullptr)
	continue;

      int child_pid = ws->child_ptid ().pid ();
      int res = remote_vkill (child_pid);

      if (res != 0)
	error (_("Can't kill fork child process %d"), child_pid);
    }

  /* Check for any pending fork events (not reported or processed yet)
     in inferior INF and kill those fork child threads as well.  */
  remote_notif_get_pending_events (notif);
  for (auto &event : rs->stop_reply_queue)
    {
      if (event->ptid.pid () != inf->pid)
	continue;

      if (!is_fork_status (event->ws.kind ()))
	continue;

      int child_pid = event->ws.child_ptid ().pid ();
      int res = remote_vkill (child_pid);

      if (res != 0)
	error (_("Can't kill fork child process %d"), child_pid);
    }
}


/* Target hook to kill the current inferior.  */

void
remote_target::kill ()
{
  int res = -1;
  inferior *inf = find_inferior_pid (this, inferior_ptid.pid ());

  gdb_assert (inf != nullptr);

  if (m_features.packet_support (PACKET_vKill) != PACKET_DISABLE)
    {
      /* If we're stopped while forking and we haven't followed yet,
	 kill the child task.  We need to do this before killing the
	 parent task because if this is a vfork then the parent will
	 be sleeping.  */
      kill_new_fork_children (inf);

      res = remote_vkill (inf->pid);
      if (res == 0)
	{
	  target_mourn_inferior (inferior_ptid);
	  return;
	}
    }

  /* If we are in 'target remote' mode and we are killing the only
     inferior, then we will tell gdbserver to exit and unpush the
     target.  */
  if (res == -1 && !m_features.remote_multi_process_p ()
      && number_of_live_inferiors (this) == 1)
    {
      remote_kill_k ();

      /* We've killed the remote end, we get to mourn it.  If we are
	 not in extended mode, mourning the inferior also unpushes
	 remote_ops from the target stack, which closes the remote
	 connection.  */
      target_mourn_inferior (inferior_ptid);

      return;
    }

  error (_("Can't kill process"));
}

/* Send a kill request to the target using the 'vKill' packet.  */

int
remote_target::remote_vkill (int pid)
{
  if (m_features.packet_support (PACKET_vKill) == PACKET_DISABLE)
    return -1;

  remote_state *rs = get_remote_state ();

  /* Tell the remote target to detach.  */
  xsnprintf (rs->buf.data (), get_remote_packet_size (), "vKill;%x", pid);
  putpkt (rs->buf);
  getpkt (&rs->buf);

  switch (m_features.packet_ok (rs->buf, PACKET_vKill))
    {
    case PACKET_OK:
      return 0;
    case PACKET_ERROR:
      return 1;
    case PACKET_UNKNOWN:
      return -1;
    default:
      internal_error (_("Bad result from packet_ok"));
    }
}

/* Send a kill request to the target using the 'k' packet.  */

void
remote_target::remote_kill_k ()
{
  /* Catch errors so the user can quit from gdb even when we
     aren't on speaking terms with the remote system.  */
  try
    {
      putpkt ("k");
    }
  catch (const gdb_exception_error &ex)
    {
      if (ex.error == TARGET_CLOSE_ERROR)
	{
	  /* If we got an (EOF) error that caused the target
	     to go away, then we're done, that's what we wanted.
	     "k" is susceptible to cause a premature EOF, given
	     that the remote server isn't actually required to
	     reply to "k", and it can happen that it doesn't
	     even get to reply ACK to the "k".  */
	  return;
	}

      /* Otherwise, something went wrong.  We didn't actually kill
	 the target.  Just propagate the exception, and let the
	 user or higher layers decide what to do.  */
      throw;
    }
}

void
remote_target::mourn_inferior ()
{
  struct remote_state *rs = get_remote_state ();

  /* We're no longer interested in notification events of an inferior
     that exited or was killed/detached.  */
  discard_pending_stop_replies (current_inferior ());

  /* In 'target remote' mode with one inferior, we close the connection.  */
  if (!rs->extended && number_of_live_inferiors (this) <= 1)
    {
      remote_unpush_target (this);
      return;
    }

  /* In case we got here due to an error, but we're going to stay
     connected.  */
  rs->waiting_for_stop_reply = 0;

  /* If the current general thread belonged to the process we just
     detached from or has exited, the remote side current general
     thread becomes undefined.  Considering a case like this:

     - We just got here due to a detach.
     - The process that we're detaching from happens to immediately
       report a global breakpoint being hit in non-stop mode, in the
       same thread we had selected before.
     - GDB attaches to this process again.
     - This event happens to be the next event we handle.

     GDB would consider that the current general thread didn't need to
     be set on the stub side (with Hg), since for all it knew,
     GENERAL_THREAD hadn't changed.

     Notice that although in all-stop mode, the remote server always
     sets the current thread to the thread reporting the stop event,
     that doesn't happen in non-stop mode; in non-stop, the stub *must
     not* change the current thread when reporting a breakpoint hit,
     due to the decoupling of event reporting and event handling.

     To keep things simple, we always invalidate our notion of the
     current thread.  */
  record_currthread (rs, minus_one_ptid);

  /* Call common code to mark the inferior as not running.  */
  generic_mourn_inferior ();
}

bool
extended_remote_target::supports_disable_randomization ()
{
  return (m_features.packet_support (PACKET_QDisableRandomization)
	  == PACKET_ENABLE);
}

void
remote_target::extended_remote_disable_randomization (int val)
{
  struct remote_state *rs = get_remote_state ();
  char *reply;

  xsnprintf (rs->buf.data (), get_remote_packet_size (),
	     "QDisableRandomization:%x", val);
  putpkt (rs->buf);
  reply = remote_get_noisy_reply ();
  if (*reply == '\0')
    error (_("Target does not support QDisableRandomization."));
  if (strcmp (reply, "OK") != 0)
    error (_("Bogus QDisableRandomization reply from target: %s"), reply);
}

int
remote_target::extended_remote_run (const std::string &args)
{
  struct remote_state *rs = get_remote_state ();
  int len;
  const char *remote_exec_file = get_remote_exec_file ();

  /* If the user has disabled vRun support, or we have detected that
     support is not available, do not try it.  */
  if (m_features.packet_support (PACKET_vRun) == PACKET_DISABLE)
    return -1;

  strcpy (rs->buf.data (), "vRun;");
  len = strlen (rs->buf.data ());

  if (strlen (remote_exec_file) * 2 + len >= get_remote_packet_size ())
    error (_("Remote file name too long for run packet"));
  len += 2 * bin2hex ((gdb_byte *) remote_exec_file, rs->buf.data () + len,
		      strlen (remote_exec_file));

  if (!args.empty ())
    {
      int i;

      gdb_argv argv (args.c_str ());
      for (i = 0; argv[i] != NULL; i++)
	{
	  if (strlen (argv[i]) * 2 + 1 + len >= get_remote_packet_size ())
	    error (_("Argument list too long for run packet"));
	  rs->buf[len++] = ';';
	  len += 2 * bin2hex ((gdb_byte *) argv[i], rs->buf.data () + len,
			      strlen (argv[i]));
	}
    }

  rs->buf[len++] = '\0';

  putpkt (rs->buf);
  getpkt (&rs->buf);

  switch (m_features.packet_ok (rs->buf, PACKET_vRun))
    {
    case PACKET_OK:
      /* We have a wait response.  All is well.  */
      return 0;
    case PACKET_UNKNOWN:
      return -1;
    case PACKET_ERROR:
      if (remote_exec_file[0] == '\0')
	error (_("Running the default executable on the remote target failed; "
		 "try \"set remote exec-file\"?"));
      else
	error (_("Running \"%s\" on the remote target failed"),
	       remote_exec_file);
    default:
      gdb_assert_not_reached ("bad switch");
    }
}

/* Helper function to send set/unset environment packets.  ACTION is
   either "set" or "unset".  PACKET is either "QEnvironmentHexEncoded"
   or "QEnvironmentUnsetVariable".  VALUE is the variable to be
   sent.  */

void
remote_target::send_environment_packet (const char *action,
					const char *packet,
					const char *value)
{
  remote_state *rs = get_remote_state ();

  /* Convert the environment variable to an hex string, which
     is the best format to be transmitted over the wire.  */
  std::string encoded_value = bin2hex ((const gdb_byte *) value,
					 strlen (value));

  xsnprintf (rs->buf.data (), get_remote_packet_size (),
	     "%s:%s", packet, encoded_value.c_str ());

  putpkt (rs->buf);
  getpkt (&rs->buf);
  if (strcmp (rs->buf.data (), "OK") != 0)
    warning (_("Unable to %s environment variable '%s' on remote."),
	     action, value);
}

/* Helper function to handle the QEnvironment* packets.  */

void
remote_target::extended_remote_environment_support ()
{
  remote_state *rs = get_remote_state ();

  if (m_features.packet_support (PACKET_QEnvironmentReset) != PACKET_DISABLE)
    {
      putpkt ("QEnvironmentReset");
      getpkt (&rs->buf);
      if (strcmp (rs->buf.data (), "OK") != 0)
	warning (_("Unable to reset environment on remote."));
    }

  gdb_environ *e = &current_inferior ()->environment;

  if (m_features.packet_support (PACKET_QEnvironmentHexEncoded)
      != PACKET_DISABLE)
    {
      for (const std::string &el : e->user_set_env ())
	send_environment_packet ("set", "QEnvironmentHexEncoded",
				 el.c_str ());
    }


  if (m_features.packet_support (PACKET_QEnvironmentUnset) != PACKET_DISABLE)
    for (const std::string &el : e->user_unset_env ())
      send_environment_packet ("unset", "QEnvironmentUnset", el.c_str ());
}

/* Helper function to set the current working directory for the
   inferior in the remote target.  */

void
remote_target::extended_remote_set_inferior_cwd ()
{
  if (m_features.packet_support (PACKET_QSetWorkingDir) != PACKET_DISABLE)
    {
      const std::string &inferior_cwd = current_inferior ()->cwd ();
      remote_state *rs = get_remote_state ();

      if (!inferior_cwd.empty ())
	{
	  std::string hexpath
	    = bin2hex ((const gdb_byte *) inferior_cwd.data (),
		       inferior_cwd.size ());

	  xsnprintf (rs->buf.data (), get_remote_packet_size (),
		     "QSetWorkingDir:%s", hexpath.c_str ());
	}
      else
	{
	  /* An empty inferior_cwd means that the user wants us to
	     reset the remote server's inferior's cwd.  */
	  xsnprintf (rs->buf.data (), get_remote_packet_size (),
		     "QSetWorkingDir:");
	}

      putpkt (rs->buf);
      getpkt (&rs->buf);
      if (m_features.packet_ok (rs->buf, PACKET_QSetWorkingDir) != PACKET_OK)
	error (_("\
Remote replied unexpectedly while setting the inferior's working\n\
directory: %s"),
	       rs->buf.data ());

    }
}

/* In the extended protocol we want to be able to do things like
   "run" and have them basically work as expected.  So we need
   a special create_inferior function.  We support changing the
   executable file and the command line arguments, but not the
   environment.  */

void
extended_remote_target::create_inferior (const char *exec_file,
					 const std::string &args,
					 char **env, int from_tty)
{
  int run_worked;
  char *stop_reply;
  struct remote_state *rs = get_remote_state ();
  const char *remote_exec_file = get_remote_exec_file ();

  /* If running asynchronously, register the target file descriptor
     with the event loop.  */
  if (target_can_async_p ())
    target_async (true);

  /* Disable address space randomization if requested (and supported).  */
  if (supports_disable_randomization ())
    extended_remote_disable_randomization (disable_randomization);

  /* If startup-with-shell is on, we inform gdbserver to start the
     remote inferior using a shell.  */
  if (m_features.packet_support (PACKET_QStartupWithShell) != PACKET_DISABLE)
    {
      xsnprintf (rs->buf.data (), get_remote_packet_size (),
		 "QStartupWithShell:%d", startup_with_shell ? 1 : 0);
      putpkt (rs->buf);
      getpkt (&rs->buf);
      if (strcmp (rs->buf.data (), "OK") != 0)
	error (_("\
Remote replied unexpectedly while setting startup-with-shell: %s"),
	       rs->buf.data ());
    }

  extended_remote_environment_support ();

  extended_remote_set_inferior_cwd ();

  /* Now restart the remote server.  */
  run_worked = extended_remote_run (args) != -1;
  if (!run_worked)
    {
      /* vRun was not supported.  Fail if we need it to do what the
	 user requested.  */
      if (remote_exec_file[0])
	error (_("Remote target does not support \"set remote exec-file\""));
      if (!args.empty ())
	error (_("Remote target does not support \"set args\" or run ARGS"));

      /* Fall back to "R".  */
      extended_remote_restart ();
    }

  /* vRun's success return is a stop reply.  */
  stop_reply = run_worked ? rs->buf.data () : NULL;
  add_current_inferior_and_thread (stop_reply);

  /* Get updated offsets, if the stub uses qOffsets.  */
  get_offsets ();
}


/* Given a location's target info BP_TGT and the packet buffer BUF,  output
   the list of conditions (in agent expression bytecode format), if any, the
   target needs to evaluate.  The output is placed into the packet buffer
   started from BUF and ended at BUF_END.  */

static int
remote_add_target_side_condition (struct gdbarch *gdbarch,
				  struct bp_target_info *bp_tgt, char *buf,
				  char *buf_end)
{
  if (bp_tgt->conditions.empty ())
    return 0;

  buf += strlen (buf);
  xsnprintf (buf, buf_end - buf, "%s", ";");
  buf++;

  /* Send conditions to the target.  */
  for (agent_expr *aexpr : bp_tgt->conditions)
    {
      xsnprintf (buf, buf_end - buf, "X%x,", (int) aexpr->buf.size ());
      buf += strlen (buf);
      for (int i = 0; i < aexpr->buf.size (); ++i)
	buf = pack_hex_byte (buf, aexpr->buf[i]);
      *buf = '\0';
    }
  return 0;
}

static void
remote_add_target_side_commands (struct gdbarch *gdbarch,
				 struct bp_target_info *bp_tgt, char *buf)
{
  if (bp_tgt->tcommands.empty ())
    return;

  buf += strlen (buf);

  sprintf (buf, ";cmds:%x,", bp_tgt->persist);
  buf += strlen (buf);

  /* Concatenate all the agent expressions that are commands into the
     cmds parameter.  */
  for (agent_expr *aexpr : bp_tgt->tcommands)
    {
      sprintf (buf, "X%x,", (int) aexpr->buf.size ());
      buf += strlen (buf);
      for (int i = 0; i < aexpr->buf.size (); ++i)
	buf = pack_hex_byte (buf, aexpr->buf[i]);
      *buf = '\0';
    }
}

/* Insert a breakpoint.  On targets that have software breakpoint
   support, we ask the remote target to do the work; on targets
   which don't, we insert a traditional memory breakpoint.  */

int
remote_target::insert_breakpoint (struct gdbarch *gdbarch,
				  struct bp_target_info *bp_tgt)
{
  /* Try the "Z" s/w breakpoint packet if it is not already disabled.
     If it succeeds, then set the support to PACKET_ENABLE.  If it
     fails, and the user has explicitly requested the Z support then
     report an error, otherwise, mark it disabled and go on.  */

  if (m_features.packet_support (PACKET_Z0) != PACKET_DISABLE)
    {
      CORE_ADDR addr = bp_tgt->reqstd_address;
      struct remote_state *rs;
      char *p, *endbuf;

      /* Make sure the remote is pointing at the right process, if
	 necessary.  */
      if (!gdbarch_has_global_breakpoints (current_inferior ()->arch ()))
	set_general_process ();

      rs = get_remote_state ();
      p = rs->buf.data ();
      endbuf = p + get_remote_packet_size ();

      *(p++) = 'Z';
      *(p++) = '0';
      *(p++) = ',';
      addr = (ULONGEST) remote_address_masked (addr);
      p += hexnumstr (p, addr);
      xsnprintf (p, endbuf - p, ",%d", bp_tgt->kind);

      if (supports_evaluation_of_breakpoint_conditions ())
	remote_add_target_side_condition (gdbarch, bp_tgt, p, endbuf);

      if (can_run_breakpoint_commands ())
	remote_add_target_side_commands (gdbarch, bp_tgt, p);

      putpkt (rs->buf);
      getpkt (&rs->buf);

      switch (m_features.packet_ok (rs->buf, PACKET_Z0))
	{
	case PACKET_ERROR:
	  return -1;
	case PACKET_OK:
	  return 0;
	case PACKET_UNKNOWN:
	  break;
	}
    }

  /* If this breakpoint has target-side commands but this stub doesn't
     support Z0 packets, throw error.  */
  if (!bp_tgt->tcommands.empty ())
    throw_error (NOT_SUPPORTED_ERROR, _("\
Target doesn't support breakpoints that have target side commands."));

  return memory_insert_breakpoint (this, gdbarch, bp_tgt);
}

int
remote_target::remove_breakpoint (struct gdbarch *gdbarch,
				  struct bp_target_info *bp_tgt,
				  enum remove_bp_reason reason)
{
  CORE_ADDR addr = bp_tgt->placed_address;
  struct remote_state *rs = get_remote_state ();

  if (m_features.packet_support (PACKET_Z0) != PACKET_DISABLE)
    {
      char *p = rs->buf.data ();
      char *endbuf = p + get_remote_packet_size ();

      /* Make sure the remote is pointing at the right process, if
	 necessary.  */
      if (!gdbarch_has_global_breakpoints (current_inferior ()->arch ()))
	set_general_process ();

      *(p++) = 'z';
      *(p++) = '0';
      *(p++) = ',';

      addr = (ULONGEST) remote_address_masked (bp_tgt->placed_address);
      p += hexnumstr (p, addr);
      xsnprintf (p, endbuf - p, ",%d", bp_tgt->kind);

      putpkt (rs->buf);
      getpkt (&rs->buf);

      return (rs->buf[0] == 'E');
    }

  return memory_remove_breakpoint (this, gdbarch, bp_tgt, reason);
}

static enum Z_packet_type
watchpoint_to_Z_packet (int type)
{
  switch (type)
    {
    case hw_write:
      return Z_PACKET_WRITE_WP;
      break;
    case hw_read:
      return Z_PACKET_READ_WP;
      break;
    case hw_access:
      return Z_PACKET_ACCESS_WP;
      break;
    default:
      internal_error (_("hw_bp_to_z: bad watchpoint type %d"), type);
    }
}

int
remote_target::insert_watchpoint (CORE_ADDR addr, int len,
				  enum target_hw_bp_type type, struct expression *cond)
{
  struct remote_state *rs = get_remote_state ();
  char *endbuf = rs->buf.data () + get_remote_packet_size ();
  char *p;
  enum Z_packet_type packet = watchpoint_to_Z_packet (type);

  if (m_features.packet_support ((to_underlying (PACKET_Z0)
				  + to_underlying (packet))) == PACKET_DISABLE)
    return 1;

  /* Make sure the remote is pointing at the right process, if
     necessary.  */
  if (!gdbarch_has_global_breakpoints (current_inferior ()->arch ()))
    set_general_process ();

  xsnprintf (rs->buf.data (), endbuf - rs->buf.data (), "Z%x,", packet);
  p = strchr (rs->buf.data (), '\0');
  addr = remote_address_masked (addr);
  p += hexnumstr (p, (ULONGEST) addr);
  xsnprintf (p, endbuf - p, ",%x", len);

  putpkt (rs->buf);
  getpkt (&rs->buf);

  switch (m_features.packet_ok (rs->buf, (to_underlying (PACKET_Z0)
					  + to_underlying (packet))))
    {
    case PACKET_ERROR:
      return -1;
    case PACKET_UNKNOWN:
      return 1;
    case PACKET_OK:
      return 0;
    }
  internal_error (_("remote_insert_watchpoint: reached end of function"));
}

bool
remote_target::watchpoint_addr_within_range (CORE_ADDR addr,
					     CORE_ADDR start, int length)
{
  CORE_ADDR diff = remote_address_masked (addr - start);

  return diff < length;
}


int
remote_target::remove_watchpoint (CORE_ADDR addr, int len,
				  enum target_hw_bp_type type, struct expression *cond)
{
  struct remote_state *rs = get_remote_state ();
  char *endbuf = rs->buf.data () + get_remote_packet_size ();
  char *p;
  enum Z_packet_type packet = watchpoint_to_Z_packet (type);

  if (m_features.packet_support ((to_underlying (PACKET_Z0)
				  + to_underlying (packet))) == PACKET_DISABLE)
    return -1;

  /* Make sure the remote is pointing at the right process, if
     necessary.  */
  if (!gdbarch_has_global_breakpoints (current_inferior ()->arch ()))
    set_general_process ();

  xsnprintf (rs->buf.data (), endbuf - rs->buf.data (), "z%x,", packet);
  p = strchr (rs->buf.data (), '\0');
  addr = remote_address_masked (addr);
  p += hexnumstr (p, (ULONGEST) addr);
  xsnprintf (p, endbuf - p, ",%x", len);
  putpkt (rs->buf);
  getpkt (&rs->buf);

  switch (m_features.packet_ok (rs->buf, (to_underlying (PACKET_Z0)
					  + to_underlying (packet))))
    {
    case PACKET_ERROR:
    case PACKET_UNKNOWN:
      return -1;
    case PACKET_OK:
      return 0;
    }
  internal_error (_("remote_remove_watchpoint: reached end of function"));
}


static int remote_hw_watchpoint_limit = -1;
static int remote_hw_watchpoint_length_limit = -1;
static int remote_hw_breakpoint_limit = -1;

int
remote_target::region_ok_for_hw_watchpoint (CORE_ADDR addr, int len)
{
  if (remote_hw_watchpoint_length_limit == 0)
    return 0;
  else if (remote_hw_watchpoint_length_limit < 0)
    return 1;
  else if (len <= remote_hw_watchpoint_length_limit)
    return 1;
  else
    return 0;
}

int
remote_target::can_use_hw_breakpoint (enum bptype type, int cnt, int ot)
{
  if (type == bp_hardware_breakpoint)
    {
      if (remote_hw_breakpoint_limit == 0)
	return 0;
      else if (remote_hw_breakpoint_limit < 0)
	return 1;
      else if (cnt <= remote_hw_breakpoint_limit)
	return 1;
    }
  else
    {
      if (remote_hw_watchpoint_limit == 0)
	return 0;
      else if (remote_hw_watchpoint_limit < 0)
	return 1;
      else if (ot)
	return -1;
      else if (cnt <= remote_hw_watchpoint_limit)
	return 1;
    }
  return -1;
}

/* The to_stopped_by_sw_breakpoint method of target remote.  */

bool
remote_target::stopped_by_sw_breakpoint ()
{
  struct thread_info *thread = inferior_thread ();

  return (thread->priv != NULL
	  && (get_remote_thread_info (thread)->stop_reason
	      == TARGET_STOPPED_BY_SW_BREAKPOINT));
}

/* The to_supports_stopped_by_sw_breakpoint method of target
   remote.  */

bool
remote_target::supports_stopped_by_sw_breakpoint ()
{
  return (m_features.packet_support (PACKET_swbreak_feature) == PACKET_ENABLE);
}

/* The to_stopped_by_hw_breakpoint method of target remote.  */

bool
remote_target::stopped_by_hw_breakpoint ()
{
  struct thread_info *thread = inferior_thread ();

  return (thread->priv != NULL
	  && (get_remote_thread_info (thread)->stop_reason
	      == TARGET_STOPPED_BY_HW_BREAKPOINT));
}

/* The to_supports_stopped_by_hw_breakpoint method of target
   remote.  */

bool
remote_target::supports_stopped_by_hw_breakpoint ()
{
  return (m_features.packet_support (PACKET_hwbreak_feature) == PACKET_ENABLE);
}

bool
remote_target::stopped_by_watchpoint ()
{
  struct thread_info *thread = inferior_thread ();

  return (thread->priv != NULL
	  && (get_remote_thread_info (thread)->stop_reason
	      == TARGET_STOPPED_BY_WATCHPOINT));
}

bool
remote_target::stopped_data_address (CORE_ADDR *addr_p)
{
  struct thread_info *thread = inferior_thread ();

  if (thread->priv != NULL
      && (get_remote_thread_info (thread)->stop_reason
	  == TARGET_STOPPED_BY_WATCHPOINT))
    {
      *addr_p = get_remote_thread_info (thread)->watch_data_address;
      return true;
    }

  return false;
}


int
remote_target::insert_hw_breakpoint (struct gdbarch *gdbarch,
				     struct bp_target_info *bp_tgt)
{
  CORE_ADDR addr = bp_tgt->reqstd_address;
  struct remote_state *rs;
  char *p, *endbuf;
  char *message;

  if (m_features.packet_support (PACKET_Z1) == PACKET_DISABLE)
    return -1;

  /* Make sure the remote is pointing at the right process, if
     necessary.  */
  if (!gdbarch_has_global_breakpoints (current_inferior ()->arch ()))
    set_general_process ();

  rs = get_remote_state ();
  p = rs->buf.data ();
  endbuf = p + get_remote_packet_size ();

  *(p++) = 'Z';
  *(p++) = '1';
  *(p++) = ',';

  addr = remote_address_masked (addr);
  p += hexnumstr (p, (ULONGEST) addr);
  xsnprintf (p, endbuf - p, ",%x", bp_tgt->kind);

  if (supports_evaluation_of_breakpoint_conditions ())
    remote_add_target_side_condition (gdbarch, bp_tgt, p, endbuf);

  if (can_run_breakpoint_commands ())
    remote_add_target_side_commands (gdbarch, bp_tgt, p);

  putpkt (rs->buf);
  getpkt (&rs->buf);

  switch (m_features.packet_ok (rs->buf, PACKET_Z1))
    {
    case PACKET_ERROR:
      if (rs->buf[1] == '.')
	{
	  message = strchr (&rs->buf[2], '.');
	  if (message)
	    error (_("Remote failure reply: %s"), message + 1);
	}
      return -1;
    case PACKET_UNKNOWN:
      return -1;
    case PACKET_OK:
      return 0;
    }
  internal_error (_("remote_insert_hw_breakpoint: reached end of function"));
}


int
remote_target::remove_hw_breakpoint (struct gdbarch *gdbarch,
				     struct bp_target_info *bp_tgt)
{
  CORE_ADDR addr;
  struct remote_state *rs = get_remote_state ();
  char *p = rs->buf.data ();
  char *endbuf = p + get_remote_packet_size ();

  if (m_features.packet_support (PACKET_Z1) == PACKET_DISABLE)
    return -1;

  /* Make sure the remote is pointing at the right process, if
     necessary.  */
  if (!gdbarch_has_global_breakpoints (current_inferior ()->arch ()))
    set_general_process ();

  *(p++) = 'z';
  *(p++) = '1';
  *(p++) = ',';

  addr = remote_address_masked (bp_tgt->placed_address);
  p += hexnumstr (p, (ULONGEST) addr);
  xsnprintf (p, endbuf  - p, ",%x", bp_tgt->kind);

  putpkt (rs->buf);
  getpkt (&rs->buf);

  switch (m_features.packet_ok (rs->buf, PACKET_Z1))
    {
    case PACKET_ERROR:
    case PACKET_UNKNOWN:
      return -1;
    case PACKET_OK:
      return 0;
    }
  internal_error (_("remote_remove_hw_breakpoint: reached end of function"));
}

/* Verify memory using the "qCRC:" request.  */

int
remote_target::verify_memory (const gdb_byte *data, CORE_ADDR lma, ULONGEST size)
{
  struct remote_state *rs = get_remote_state ();
  unsigned long host_crc, target_crc;
  char *tmp;

  /* It doesn't make sense to use qCRC if the remote target is
     connected but not running.  */
  if (target_has_execution ()
      && m_features.packet_support (PACKET_qCRC) != PACKET_DISABLE)
    {
      enum packet_result result;

      /* Make sure the remote is pointing at the right process.  */
      set_general_process ();

      /* FIXME: assumes lma can fit into long.  */
      xsnprintf (rs->buf.data (), get_remote_packet_size (), "qCRC:%lx,%lx",
		 (long) lma, (long) size);
      putpkt (rs->buf);

      /* Be clever; compute the host_crc before waiting for target
	 reply.  */
      host_crc = xcrc32 (data, size, 0xffffffff);

      getpkt (&rs->buf);

      result = m_features.packet_ok (rs->buf, PACKET_qCRC);
      if (result == PACKET_ERROR)
	return -1;
      else if (result == PACKET_OK)
	{
	  for (target_crc = 0, tmp = &rs->buf[1]; *tmp; tmp++)
	    target_crc = target_crc * 16 + fromhex (*tmp);

	  return (host_crc == target_crc);
	}
    }

  return simple_verify_memory (this, data, lma, size);
}

/* compare-sections command

   With no arguments, compares each loadable section in the exec bfd
   with the same memory range on the target, and reports mismatches.
   Useful for verifying the image on the target against the exec file.  */

static void
compare_sections_command (const char *args, int from_tty)
{
  asection *s;
  const char *sectname;
  bfd_size_type size;
  bfd_vma lma;
  int matched = 0;
  int mismatched = 0;
  int res;
  int read_only = 0;

  if (!current_program_space->exec_bfd ())
    error (_("command cannot be used without an exec file"));

  if (args != NULL && strcmp (args, "-r") == 0)
    {
      read_only = 1;
      args = NULL;
    }

  for (s = current_program_space->exec_bfd ()->sections; s; s = s->next)
    {
      if (!(s->flags & SEC_LOAD))
	continue;		/* Skip non-loadable section.  */

      if (read_only && (s->flags & SEC_READONLY) == 0)
	continue;		/* Skip writeable sections */

      size = bfd_section_size (s);
      if (size == 0)
	continue;		/* Skip zero-length section.  */

      sectname = bfd_section_name (s);
      if (args && strcmp (args, sectname) != 0)
	continue;		/* Not the section selected by user.  */

      matched = 1;		/* Do this section.  */
      lma = s->lma;

      gdb::byte_vector sectdata (size);
      bfd_get_section_contents (current_program_space->exec_bfd (), s,
				sectdata.data (), 0, size);

      res = target_verify_memory (sectdata.data (), lma, size);

      if (res == -1)
	error (_("target memory fault, section %s, range %s -- %s"), sectname,
	       paddress (current_inferior ()->arch (), lma),
	       paddress (current_inferior ()->arch (), lma + size));

      gdb_printf ("Section %s, range %s -- %s: ", sectname,
		  paddress (current_inferior ()->arch (), lma),
		  paddress (current_inferior ()->arch (), lma + size));
      if (res)
	gdb_printf ("matched.\n");
      else
	{
	  gdb_printf ("MIS-MATCHED!\n");
	  mismatched++;
	}
    }
  if (mismatched > 0)
    warning (_("One or more sections of the target image does "
	       "not match the loaded file"));
  if (args && !matched)
    gdb_printf (_("No loaded section named '%s'.\n"), args);
}

/* Write LEN bytes from WRITEBUF into OBJECT_NAME/ANNEX at OFFSET
   into remote target.  The number of bytes written to the remote
   target is returned, or -1 for error.  */

target_xfer_status
remote_target::remote_write_qxfer (const char *object_name,
				   const char *annex, const gdb_byte *writebuf,
				   ULONGEST offset, LONGEST len,
				   ULONGEST *xfered_len,
				   const unsigned int which_packet)
{
  int i, buf_len;
  ULONGEST n;
  struct remote_state *rs = get_remote_state ();
  int max_size = get_memory_write_packet_size ();

  if (m_features.packet_support (which_packet) == PACKET_DISABLE)
    return TARGET_XFER_E_IO;

  /* Insert header.  */
  i = snprintf (rs->buf.data (), max_size,
		"qXfer:%s:write:%s:%s:",
		object_name, annex ? annex : "",
		phex_nz (offset, sizeof offset));
  max_size -= (i + 1);

  /* Escape as much data as fits into rs->buf.  */
  buf_len = remote_escape_output
    (writebuf, len, 1, (gdb_byte *) rs->buf.data () + i, &max_size, max_size);

  if (putpkt_binary (rs->buf.data (), i + buf_len) < 0
      || getpkt (&rs->buf) < 0
      || m_features.packet_ok (rs->buf, which_packet) != PACKET_OK)
    return TARGET_XFER_E_IO;

  unpack_varlen_hex (rs->buf.data (), &n);

  *xfered_len = n;
  return (*xfered_len != 0) ? TARGET_XFER_OK : TARGET_XFER_EOF;
}

/* Read OBJECT_NAME/ANNEX from the remote target using a qXfer packet.
   Data at OFFSET, of up to LEN bytes, is read into READBUF; the
   number of bytes read is returned, or 0 for EOF, or -1 for error.
   The number of bytes read may be less than LEN without indicating an
   EOF.  PACKET is checked and updated to indicate whether the remote
   target supports this object.  */

target_xfer_status
remote_target::remote_read_qxfer (const char *object_name,
				  const char *annex,
				  gdb_byte *readbuf, ULONGEST offset,
				  LONGEST len,
				  ULONGEST *xfered_len,
				  const unsigned int which_packet)
{
  struct remote_state *rs = get_remote_state ();
  LONGEST i, n, packet_len;

  if (m_features.packet_support (which_packet) == PACKET_DISABLE)
    return TARGET_XFER_E_IO;

  /* Check whether we've cached an end-of-object packet that matches
     this request.  */
  if (rs->finished_object)
    {
      if (strcmp (object_name, rs->finished_object) == 0
	  && strcmp (annex ? annex : "", rs->finished_annex) == 0
	  && offset == rs->finished_offset)
	return TARGET_XFER_EOF;


      /* Otherwise, we're now reading something different.  Discard
	 the cache.  */
      xfree (rs->finished_object);
      xfree (rs->finished_annex);
      rs->finished_object = NULL;
      rs->finished_annex = NULL;
    }

  /* Request only enough to fit in a single packet.  The actual data
     may not, since we don't know how much of it will need to be escaped;
     the target is free to respond with slightly less data.  We subtract
     five to account for the response type and the protocol frame.  */
  n = std::min<LONGEST> (get_remote_packet_size () - 5, len);
  snprintf (rs->buf.data (), get_remote_packet_size () - 4,
	    "qXfer:%s:read:%s:%s,%s",
	    object_name, annex ? annex : "",
	    phex_nz (offset, sizeof offset),
	    phex_nz (n, sizeof n));
  i = putpkt (rs->buf);
  if (i < 0)
    return TARGET_XFER_E_IO;

  rs->buf[0] = '\0';
  packet_len = getpkt (&rs->buf);
  if (packet_len < 0
      || m_features.packet_ok (rs->buf, which_packet) != PACKET_OK)
    return TARGET_XFER_E_IO;

  if (rs->buf[0] != 'l' && rs->buf[0] != 'm')
    error (_("Unknown remote qXfer reply: %s"), rs->buf.data ());

  /* 'm' means there is (or at least might be) more data after this
     batch.  That does not make sense unless there's at least one byte
     of data in this reply.  */
  if (rs->buf[0] == 'm' && packet_len == 1)
    error (_("Remote qXfer reply contained no data."));

  /* Got some data.  */
  i = remote_unescape_input ((gdb_byte *) rs->buf.data () + 1,
			     packet_len - 1, readbuf, n);

  /* 'l' is an EOF marker, possibly including a final block of data,
     or possibly empty.  If we have the final block of a non-empty
     object, record this fact to bypass a subsequent partial read.  */
  if (rs->buf[0] == 'l' && offset + i > 0)
    {
      rs->finished_object = xstrdup (object_name);
      rs->finished_annex = xstrdup (annex ? annex : "");
      rs->finished_offset = offset + i;
    }

  if (i == 0)
    return TARGET_XFER_EOF;
  else
    {
      *xfered_len = i;
      return TARGET_XFER_OK;
    }
}

enum target_xfer_status
remote_target::xfer_partial (enum target_object object,
			     const char *annex, gdb_byte *readbuf,
			     const gdb_byte *writebuf, ULONGEST offset, ULONGEST len,
			     ULONGEST *xfered_len)
{
  struct remote_state *rs;
  int i;
  char *p2;
  char query_type;
  int unit_size
    = gdbarch_addressable_memory_unit_size (current_inferior ()->arch ());

  set_remote_traceframe ();
  set_general_thread (inferior_ptid);

  rs = get_remote_state ();

  /* Handle memory using the standard memory routines.  */
  if (object == TARGET_OBJECT_MEMORY)
    {
      /* If the remote target is connected but not running, we should
	 pass this request down to a lower stratum (e.g. the executable
	 file).  */
      if (!target_has_execution ())
	return TARGET_XFER_EOF;

      if (writebuf != NULL)
	return remote_write_bytes (offset, writebuf, len, unit_size,
				   xfered_len);
      else
	return remote_read_bytes (offset, readbuf, len, unit_size,
				  xfered_len);
    }

  /* Handle extra signal info using qxfer packets.  */
  if (object == TARGET_OBJECT_SIGNAL_INFO)
    {
      if (readbuf)
	return remote_read_qxfer ("siginfo", annex, readbuf, offset, len,
				  xfered_len, PACKET_qXfer_siginfo_read);
      else
	return remote_write_qxfer ("siginfo", annex, writebuf, offset, len,
				   xfered_len, PACKET_qXfer_siginfo_write);
    }

  if (object == TARGET_OBJECT_STATIC_TRACE_DATA)
    {
      if (readbuf)
	return remote_read_qxfer ("statictrace", annex,
				  readbuf, offset, len, xfered_len,
				  PACKET_qXfer_statictrace_read);
      else
	return TARGET_XFER_E_IO;
    }

  /* Only handle flash writes.  */
  if (writebuf != NULL)
    {
      switch (object)
	{
	case TARGET_OBJECT_FLASH:
	  return remote_flash_write (offset, len, xfered_len,
				     writebuf);

	default:
	  return TARGET_XFER_E_IO;
	}
    }

  /* Map pre-existing objects onto letters.  DO NOT do this for new
     objects!!!  Instead specify new query packets.  */
  switch (object)
    {
    case TARGET_OBJECT_AVR:
      query_type = 'R';
      break;

    case TARGET_OBJECT_AUXV:
      gdb_assert (annex == NULL);
      return remote_read_qxfer
	("auxv", annex, readbuf, offset, len, xfered_len, PACKET_qXfer_auxv);

    case TARGET_OBJECT_AVAILABLE_FEATURES:
      return remote_read_qxfer
	("features", annex, readbuf, offset, len, xfered_len,
	 PACKET_qXfer_features);

    case TARGET_OBJECT_LIBRARIES:
      return remote_read_qxfer
	("libraries", annex, readbuf, offset, len, xfered_len,
	 PACKET_qXfer_libraries);

    case TARGET_OBJECT_LIBRARIES_SVR4:
      return remote_read_qxfer
	("libraries-svr4", annex, readbuf, offset, len, xfered_len,
	 PACKET_qXfer_libraries_svr4);

    case TARGET_OBJECT_MEMORY_MAP:
      gdb_assert (annex == NULL);
      return remote_read_qxfer
	("memory-map", annex, readbuf, offset, len, xfered_len,
	 PACKET_qXfer_memory_map);

    case TARGET_OBJECT_OSDATA:
      /* Should only get here if we're connected.  */
      gdb_assert (rs->remote_desc);
      return remote_read_qxfer
	("osdata", annex, readbuf, offset, len, xfered_len,
	 PACKET_qXfer_osdata);

    case TARGET_OBJECT_THREADS:
      gdb_assert (annex == NULL);
      return remote_read_qxfer
	("threads", annex, readbuf, offset, len, xfered_len,
	 PACKET_qXfer_threads);

    case TARGET_OBJECT_TRACEFRAME_INFO:
      gdb_assert (annex == NULL);
      return remote_read_qxfer
	("traceframe-info", annex, readbuf, offset, len, xfered_len,
	 PACKET_qXfer_traceframe_info);

    case TARGET_OBJECT_FDPIC:
      return remote_read_qxfer
	("fdpic", annex, readbuf, offset, len, xfered_len, PACKET_qXfer_fdpic);

    case TARGET_OBJECT_OPENVMS_UIB:
      return remote_read_qxfer
	("uib", annex, readbuf, offset, len, xfered_len, PACKET_qXfer_uib);

    case TARGET_OBJECT_BTRACE:
      return remote_read_qxfer
	("btrace", annex, readbuf, offset, len, xfered_len,
	 PACKET_qXfer_btrace);

    case TARGET_OBJECT_BTRACE_CONF:
      return remote_read_qxfer
	("btrace-conf", annex, readbuf, offset, len, xfered_len,
	 PACKET_qXfer_btrace_conf);

    case TARGET_OBJECT_EXEC_FILE:
      return remote_read_qxfer
	("exec-file", annex, readbuf, offset, len, xfered_len,
	 PACKET_qXfer_exec_file);

    default:
      return TARGET_XFER_E_IO;
    }

  /* Minimum outbuf size is get_remote_packet_size ().  If LEN is not
     large enough let the caller deal with it.  */
  if (len < get_remote_packet_size ())
    return TARGET_XFER_E_IO;
  len = get_remote_packet_size ();

  /* Except for querying the minimum buffer size, target must be open.  */
  if (!rs->remote_desc)
    error (_("remote query is only available after target open"));

  gdb_assert (annex != NULL);
  gdb_assert (readbuf != NULL);

  p2 = rs->buf.data ();
  *p2++ = 'q';
  *p2++ = query_type;

  /* We used one buffer char for the remote protocol q command and
     another for the query type.  As the remote protocol encapsulation
     uses 4 chars plus one extra in case we are debugging
     (remote_debug), we have PBUFZIZ - 7 left to pack the query
     string.  */
  i = 0;
  while (annex[i] && (i < (get_remote_packet_size () - 8)))
    {
      /* Bad caller may have sent forbidden characters.  */
      gdb_assert (isprint (annex[i]) && annex[i] != '$' && annex[i] != '#');
      *p2++ = annex[i];
      i++;
    }
  *p2 = '\0';
  gdb_assert (annex[i] == '\0');

  i = putpkt (rs->buf);
  if (i < 0)
    return TARGET_XFER_E_IO;

  getpkt (&rs->buf);
  strcpy ((char *) readbuf, rs->buf.data ());

  *xfered_len = strlen ((char *) readbuf);
  return (*xfered_len != 0) ? TARGET_XFER_OK : TARGET_XFER_EOF;
}

/* Implementation of to_get_memory_xfer_limit.  */

ULONGEST
remote_target::get_memory_xfer_limit ()
{
  return get_memory_write_packet_size ();
}

int
remote_target::search_memory (CORE_ADDR start_addr, ULONGEST search_space_len,
			      const gdb_byte *pattern, ULONGEST pattern_len,
			      CORE_ADDR *found_addrp)
{
  int addr_size = gdbarch_addr_bit (current_inferior ()->arch ()) / 8;
  struct remote_state *rs = get_remote_state ();
  int max_size = get_memory_write_packet_size ();

  /* Number of packet bytes used to encode the pattern;
     this could be more than PATTERN_LEN due to escape characters.  */
  int escaped_pattern_len;
  /* Amount of pattern that was encodable in the packet.  */
  int used_pattern_len;
  int i;
  int found;
  ULONGEST found_addr;

  auto read_memory = [this] (CORE_ADDR addr, gdb_byte *result, size_t len)
    {
      return (target_read (this, TARGET_OBJECT_MEMORY, NULL, result, addr, len)
	      == len);
    };

  /* Don't go to the target if we don't have to.  This is done before
     checking packet_support to avoid the possibility that a success for this
     edge case means the facility works in general.  */
  if (pattern_len > search_space_len)
    return 0;
  if (pattern_len == 0)
    {
      *found_addrp = start_addr;
      return 1;
    }

  /* If we already know the packet isn't supported, fall back to the simple
     way of searching memory.  */

  if (m_features.packet_support (PACKET_qSearch_memory) == PACKET_DISABLE)
    {
      /* Target doesn't provided special support, fall back and use the
	 standard support (copy memory and do the search here).  */
      return simple_search_memory (read_memory, start_addr, search_space_len,
				   pattern, pattern_len, found_addrp);
    }

  /* Make sure the remote is pointing at the right process.  */
  set_general_process ();

  /* Insert header.  */
  i = snprintf (rs->buf.data (), max_size,
		"qSearch:memory:%s;%s;",
		phex_nz (start_addr, addr_size),
		phex_nz (search_space_len, sizeof (search_space_len)));
  max_size -= (i + 1);

  /* Escape as much data as fits into rs->buf.  */
  escaped_pattern_len =
    remote_escape_output (pattern, pattern_len, 1,
			  (gdb_byte *) rs->buf.data () + i,
			  &used_pattern_len, max_size);

  /* Bail if the pattern is too large.  */
  if (used_pattern_len != pattern_len)
    error (_("Pattern is too large to transmit to remote target."));

  if (putpkt_binary (rs->buf.data (), i + escaped_pattern_len) < 0
      || getpkt (&rs->buf) < 0
      || m_features.packet_ok (rs->buf, PACKET_qSearch_memory) != PACKET_OK)
    {
      /* The request may not have worked because the command is not
	 supported.  If so, fall back to the simple way.  */
      if (m_features.packet_support (PACKET_qSearch_memory) == PACKET_DISABLE)
	{
	  return simple_search_memory (read_memory, start_addr, search_space_len,
				       pattern, pattern_len, found_addrp);
	}
      return -1;
    }

  if (rs->buf[0] == '0')
    found = 0;
  else if (rs->buf[0] == '1')
    {
      found = 1;
      if (rs->buf[1] != ',')
	error (_("Unknown qSearch:memory reply: %s"), rs->buf.data ());
      unpack_varlen_hex (&rs->buf[2], &found_addr);
      *found_addrp = found_addr;
    }
  else
    error (_("Unknown qSearch:memory reply: %s"), rs->buf.data ());

  return found;
}

void
remote_target::rcmd (const char *command, struct ui_file *outbuf)
{
  struct remote_state *rs = get_remote_state ();
  char *p = rs->buf.data ();

  if (!rs->remote_desc)
    error (_("remote rcmd is only available after target open"));

  /* Send a NULL command across as an empty command.  */
  if (command == NULL)
    command = "";

  /* The query prefix.  */
  strcpy (rs->buf.data (), "qRcmd,");
  p = strchr (rs->buf.data (), '\0');

  if ((strlen (rs->buf.data ()) + strlen (command) * 2 + 8/*misc*/)
      > get_remote_packet_size ())
    error (_("\"monitor\" command ``%s'' is too long."), command);

  /* Encode the actual command.  */
  bin2hex ((const gdb_byte *) command, p, strlen (command));

  if (putpkt (rs->buf) < 0)
    error (_("Communication problem with target."));

  /* get/display the response */
  while (1)
    {
      char *buf;

      /* XXX - see also remote_get_noisy_reply().  */
      QUIT;			/* Allow user to bail out with ^C.  */
      rs->buf[0] = '\0';
      if (getpkt (&rs->buf) == -1)
	{
	  /* Timeout.  Continue to (try to) read responses.
	     This is better than stopping with an error, assuming the stub
	     is still executing the (long) monitor command.
	     If needed, the user can interrupt gdb using C-c, obtaining
	     an effect similar to stop on timeout.  */
	  continue;
	}
      buf = rs->buf.data ();
      if (buf[0] == '\0')
	error (_("Target does not support this command."));
      if (buf[0] == 'O' && buf[1] != 'K')
	{
	  remote_console_output (buf + 1); /* 'O' message from stub.  */
	  continue;
	}
      if (strcmp (buf, "OK") == 0)
	break;
      if (strlen (buf) == 3 && buf[0] == 'E'
	  && isxdigit (buf[1]) && isxdigit (buf[2]))
	{
	  error (_("Protocol error with Rcmd"));
	}
      for (p = buf; p[0] != '\0' && p[1] != '\0'; p += 2)
	{
	  char c = (fromhex (p[0]) << 4) + fromhex (p[1]);

	  gdb_putc (c, outbuf);
	}
      break;
    }
}

std::vector<mem_region>
remote_target::memory_map ()
{
  std::vector<mem_region> result;
  std::optional<gdb::char_vector> text
    = target_read_stralloc (current_inferior ()->top_target (),
			    TARGET_OBJECT_MEMORY_MAP, NULL);

  if (text)
    result = parse_memory_map (text->data ());

  return result;
}

/* Set of callbacks used to implement the 'maint packet' command.  */

struct cli_packet_command_callbacks : public send_remote_packet_callbacks
{
  /* Called before the packet is sent.  BUF is the packet content before
     the protocol specific prefix, suffix, and escaping is added.  */

  void sending (gdb::array_view<const char> &buf) override
  {
    gdb_puts ("sending: ");
    print_packet (buf);
    gdb_puts ("\n");
  }

  /* Called with BUF, the reply from the remote target.  */

  void received (gdb::array_view<const char> &buf) override
  {
    gdb_puts ("received: \"");
    print_packet (buf);
    gdb_puts ("\"\n");
  }

private:

  /* Print BUF o gdb_stdout.  Any non-printable bytes in BUF are printed as
     '\x??' with '??' replaced by the hexadecimal value of the byte.  */

  static void
  print_packet (gdb::array_view<const char> &buf)
  {
    string_file stb;

    for (int i = 0; i < buf.size (); ++i)
      {
	gdb_byte c = buf[i];
	if (isprint (c))
	  gdb_putc (c, &stb);
	else
	  gdb_printf (&stb, "\\x%02x", (unsigned char) c);
      }

    gdb_puts (stb.string ().c_str ());
  }
};

/* See remote.h.  */

void
send_remote_packet (gdb::array_view<const char> &buf,
		    send_remote_packet_callbacks *callbacks)
{
  if (buf.size () == 0 || buf.data ()[0] == '\0')
    error (_("a remote packet must not be empty"));

  remote_target *remote = get_current_remote_target ();
  if (remote == nullptr)
    error (_("packets can only be sent to a remote target"));

  callbacks->sending (buf);

  remote->putpkt_binary (buf.data (), buf.size ());
  remote_state *rs = remote->get_remote_state ();
  int bytes = remote->getpkt (&rs->buf);

  if (bytes < 0)
    error (_("error while fetching packet from remote target"));

  gdb::array_view<const char> view (&rs->buf[0], bytes);
  callbacks->received (view);
}

/* Entry point for the 'maint packet' command.  */

static void
cli_packet_command (const char *args, int from_tty)
{
  cli_packet_command_callbacks cb;
  gdb::array_view<const char> view
    = gdb::make_array_view (args, args == nullptr ? 0 : strlen (args));
  send_remote_packet (view, &cb);
}

#if 0
/* --------- UNIT_TEST for THREAD oriented PACKETS ------------------- */

static void display_thread_info (struct gdb_ext_thread_info *info);

static void threadset_test_cmd (char *cmd, int tty);

static void threadalive_test (char *cmd, int tty);

static void threadlist_test_cmd (char *cmd, int tty);

int get_and_display_threadinfo (threadref *ref);

static void threadinfo_test_cmd (char *cmd, int tty);

static int thread_display_step (threadref *ref, void *context);

static void threadlist_update_test_cmd (char *cmd, int tty);

static void init_remote_threadtests (void);

#define SAMPLE_THREAD  0x05060708	/* Truncated 64 bit threadid.  */

static void
threadset_test_cmd (const char *cmd, int tty)
{
  int sample_thread = SAMPLE_THREAD;

  gdb_printf (_("Remote threadset test\n"));
  set_general_thread (sample_thread);
}


static void
threadalive_test (const char *cmd, int tty)
{
  int sample_thread = SAMPLE_THREAD;
  int pid = inferior_ptid.pid ();
  ptid_t ptid = ptid_t (pid, sample_thread, 0);

  if (remote_thread_alive (ptid))
    gdb_printf ("PASS: Thread alive test\n");
  else
    gdb_printf ("FAIL: Thread alive test\n");
}

void output_threadid (char *title, threadref *ref);

void
output_threadid (char *title, threadref *ref)
{
  char hexid[20];

  pack_threadid (&hexid[0], ref);	/* Convert thread id into hex.  */
  hexid[16] = 0;
  gdb_printf ("%s  %s\n", title, (&hexid[0]));
}

static void
threadlist_test_cmd (const char *cmd, int tty)
{
  int startflag = 1;
  threadref nextthread;
  int done, result_count;
  threadref threadlist[3];

  gdb_printf ("Remote Threadlist test\n");
  if (!remote_get_threadlist (startflag, &nextthread, 3, &done,
			      &result_count, &threadlist[0]))
    gdb_printf ("FAIL: threadlist test\n");
  else
    {
      threadref *scan = threadlist;
      threadref *limit = scan + result_count;

      while (scan < limit)
	output_threadid (" thread ", scan++);
    }
}

void
display_thread_info (struct gdb_ext_thread_info *info)
{
  output_threadid ("Threadid: ", &info->threadid);
  gdb_printf ("Name: %s\n ", info->shortname);
  gdb_printf ("State: %s\n", info->display);
  gdb_printf ("other: %s\n\n", info->more_display);
}

int
get_and_display_threadinfo (threadref *ref)
{
  int result;
  int set;
  struct gdb_ext_thread_info threadinfo;

  set = TAG_THREADID | TAG_EXISTS | TAG_THREADNAME
    | TAG_MOREDISPLAY | TAG_DISPLAY;
  if (0 != (result = remote_get_threadinfo (ref, set, &threadinfo)))
    display_thread_info (&threadinfo);
  return result;
}

static void
threadinfo_test_cmd (const char *cmd, int tty)
{
  int athread = SAMPLE_THREAD;
  threadref thread;
  int set;

  int_to_threadref (&thread, athread);
  gdb_printf ("Remote Threadinfo test\n");
  if (!get_and_display_threadinfo (&thread))
    gdb_printf ("FAIL cannot get thread info\n");
}

static int
thread_display_step (threadref *ref, void *context)
{
  /* output_threadid(" threadstep ",ref); *//* simple test */
  return get_and_display_threadinfo (ref);
}

static void
threadlist_update_test_cmd (const char *cmd, int tty)
{
  gdb_printf ("Remote Threadlist update test\n");
  remote_threadlist_iterator (thread_display_step, 0, CRAZY_MAX_THREADS);
}

static void
init_remote_threadtests (void)
{
  add_com ("tlist", class_obscure, threadlist_test_cmd,
	   _("Fetch and print the remote list of "
	     "thread identifiers, one pkt only."));
  add_com ("tinfo", class_obscure, threadinfo_test_cmd,
	   _("Fetch and display info about one thread."));
  add_com ("tset", class_obscure, threadset_test_cmd,
	   _("Test setting to a different thread."));
  add_com ("tupd", class_obscure, threadlist_update_test_cmd,
	   _("Iterate through updating all remote thread info."));
  add_com ("talive", class_obscure, threadalive_test,
	   _("Remote thread alive test."));
}

#endif /* 0 */

/* Convert a thread ID to a string.  */

std::string
remote_target::pid_to_str (ptid_t ptid)
{
  if (ptid == null_ptid)
    return normal_pid_to_str (ptid);
  else if (ptid.is_pid ())
    {
      /* Printing an inferior target id.  */

      /* When multi-process extensions are off, there's no way in the
	 remote protocol to know the remote process id, if there's any
	 at all.  There's one exception --- when we're connected with
	 target extended-remote, and we manually attached to a process
	 with "attach PID".  We don't record anywhere a flag that
	 allows us to distinguish that case from the case of
	 connecting with extended-remote and the stub already being
	 attached to a process, and reporting yes to qAttached, hence
	 no smart special casing here.  */
      if (!m_features.remote_multi_process_p ())
	return "Remote target";

      return normal_pid_to_str (ptid);
    }
  else
    {
      if (magic_null_ptid == ptid)
	return "Thread <main>";
      else if (m_features.remote_multi_process_p ())
	if (ptid.lwp () == 0)
	  return normal_pid_to_str (ptid);
	else
	  return string_printf ("Thread %d.%ld",
				ptid.pid (), ptid.lwp ());
      else
	return string_printf ("Thread %ld", ptid.lwp ());
    }
}

/* Get the address of the thread local variable in OBJFILE which is
   stored at OFFSET within the thread local storage for thread PTID.  */

CORE_ADDR
remote_target::get_thread_local_address (ptid_t ptid, CORE_ADDR lm,
					 CORE_ADDR offset)
{
  if (m_features.packet_support (PACKET_qGetTLSAddr) != PACKET_DISABLE)
    {
      struct remote_state *rs = get_remote_state ();
      char *p = rs->buf.data ();
      char *endp = p + get_remote_packet_size ();
      enum packet_result result;

      strcpy (p, "qGetTLSAddr:");
      p += strlen (p);
      p = write_ptid (p, endp, ptid);
      *p++ = ',';
      p += hexnumstr (p, offset);
      *p++ = ',';
      p += hexnumstr (p, lm);
      *p++ = '\0';

      putpkt (rs->buf);
      getpkt (&rs->buf);
      result = m_features.packet_ok (rs->buf, PACKET_qGetTLSAddr);
      if (result == PACKET_OK)
	{
	  ULONGEST addr;

	  unpack_varlen_hex (rs->buf.data (), &addr);
	  return addr;
	}
      else if (result == PACKET_UNKNOWN)
	throw_error (TLS_GENERIC_ERROR,
		     _("Remote target doesn't support qGetTLSAddr packet"));
      else
	throw_error (TLS_GENERIC_ERROR,
		     _("Remote target failed to process qGetTLSAddr request"));
    }
  else
    throw_error (TLS_GENERIC_ERROR,
		 _("TLS not supported or disabled on this target"));
  /* Not reached.  */
  return 0;
}

/* Provide thread local base, i.e. Thread Information Block address.
   Returns 1 if ptid is found and thread_local_base is non zero.  */

bool
remote_target::get_tib_address (ptid_t ptid, CORE_ADDR *addr)
{
  if (m_features.packet_support (PACKET_qGetTIBAddr) != PACKET_DISABLE)
    {
      struct remote_state *rs = get_remote_state ();
      char *p = rs->buf.data ();
      char *endp = p + get_remote_packet_size ();
      enum packet_result result;

      strcpy (p, "qGetTIBAddr:");
      p += strlen (p);
      p = write_ptid (p, endp, ptid);
      *p++ = '\0';

      putpkt (rs->buf);
      getpkt (&rs->buf);
      result = m_features.packet_ok (rs->buf, PACKET_qGetTIBAddr);
      if (result == PACKET_OK)
	{
	  ULONGEST val;
	  unpack_varlen_hex (rs->buf.data (), &val);
	  if (addr)
	    *addr = (CORE_ADDR) val;
	  return true;
	}
      else if (result == PACKET_UNKNOWN)
	error (_("Remote target doesn't support qGetTIBAddr packet"));
      else
	error (_("Remote target failed to process qGetTIBAddr request"));
    }
  else
    error (_("qGetTIBAddr not supported or disabled on this target"));
  /* Not reached.  */
  return false;
}

/* Support for inferring a target description based on the current
   architecture and the size of a 'g' packet.  While the 'g' packet
   can have any size (since optional registers can be left off the
   end), some sizes are easily recognizable given knowledge of the
   approximate architecture.  */

struct remote_g_packet_guess
{
  remote_g_packet_guess (int bytes_, const struct target_desc *tdesc_)
    : bytes (bytes_),
      tdesc (tdesc_)
  {
  }

  int bytes;
  const struct target_desc *tdesc;
};

struct remote_g_packet_data
{
  std::vector<remote_g_packet_guess> guesses;
};

static const registry<gdbarch>::key<struct remote_g_packet_data>
     remote_g_packet_data_handle;

static struct remote_g_packet_data *
get_g_packet_data (struct gdbarch *gdbarch)
{
  struct remote_g_packet_data *data
    = remote_g_packet_data_handle.get (gdbarch);
  if (data == nullptr)
    data = remote_g_packet_data_handle.emplace (gdbarch);
  return data;
}

void
register_remote_g_packet_guess (struct gdbarch *gdbarch, int bytes,
				const struct target_desc *tdesc)
{
  struct remote_g_packet_data *data = get_g_packet_data (gdbarch);

  gdb_assert (tdesc != NULL);

  for (const remote_g_packet_guess &guess : data->guesses)
    if (guess.bytes == bytes)
      internal_error (_("Duplicate g packet description added for size %d"),
		      bytes);

  data->guesses.emplace_back (bytes, tdesc);
}

/* Return true if remote_read_description would do anything on this target
   and architecture, false otherwise.  */

static bool
remote_read_description_p (struct target_ops *target)
{
  remote_g_packet_data *data = get_g_packet_data (current_inferior ()->arch ());

  return !data->guesses.empty ();
}

const struct target_desc *
remote_target::read_description ()
{
  remote_g_packet_data *data = get_g_packet_data (current_inferior ()->arch ());

  /* Do not try this during initial connection, when we do not know
     whether there is a running but stopped thread.  */
  if (!target_has_execution () || inferior_ptid == null_ptid)
    return beneath ()->read_description ();

  if (!data->guesses.empty ())
    {
      int bytes = send_g_packet ();

      for (const remote_g_packet_guess &guess : data->guesses)
	if (guess.bytes == bytes)
	  return guess.tdesc;

      /* We discard the g packet.  A minor optimization would be to
	 hold on to it, and fill the register cache once we have selected
	 an architecture, but it's too tricky to do safely.  */
    }

  return beneath ()->read_description ();
}

/* Remote file transfer support.  This is host-initiated I/O, not
   target-initiated; for target-initiated, see remote-fileio.c.  */

/* If *LEFT is at least the length of STRING, copy STRING to
   *BUFFER, update *BUFFER to point to the new end of the buffer, and
   decrease *LEFT.  Otherwise raise an error.  */

static void
remote_buffer_add_string (char **buffer, int *left, const char *string)
{
  int len = strlen (string);

  if (len > *left)
    error (_("Packet too long for target."));

  memcpy (*buffer, string, len);
  *buffer += len;
  *left -= len;

  /* NUL-terminate the buffer as a convenience, if there is
     room.  */
  if (*left)
    **buffer = '\0';
}

/* If *LEFT is large enough, hex encode LEN bytes from BYTES into
   *BUFFER, update *BUFFER to point to the new end of the buffer, and
   decrease *LEFT.  Otherwise raise an error.  */

static void
remote_buffer_add_bytes (char **buffer, int *left, const gdb_byte *bytes,
			 int len)
{
  if (2 * len > *left)
    error (_("Packet too long for target."));

  bin2hex (bytes, *buffer, len);
  *buffer += 2 * len;
  *left -= 2 * len;

  /* NUL-terminate the buffer as a convenience, if there is
     room.  */
  if (*left)
    **buffer = '\0';
}

/* If *LEFT is large enough, convert VALUE to hex and add it to
   *BUFFER, update *BUFFER to point to the new end of the buffer, and
   decrease *LEFT.  Otherwise raise an error.  */

static void
remote_buffer_add_int (char **buffer, int *left, ULONGEST value)
{
  int len = hexnumlen (value);

  if (len > *left)
    error (_("Packet too long for target."));

  hexnumstr (*buffer, value);
  *buffer += len;
  *left -= len;

  /* NUL-terminate the buffer as a convenience, if there is
     room.  */
  if (*left)
    **buffer = '\0';
}

/* Parse an I/O result packet from BUFFER.  Set RETCODE to the return
   value, *REMOTE_ERRNO to the remote error number or FILEIO_SUCCESS if none
   was included, and *ATTACHMENT to point to the start of the annex
   if any.  The length of the packet isn't needed here; there may
   be NUL bytes in BUFFER, but they will be after *ATTACHMENT.

   Return 0 if the packet could be parsed, -1 if it could not.  If
   -1 is returned, the other variables may not be initialized.  */

static int
remote_hostio_parse_result (const char *buffer, int *retcode,
			    fileio_error *remote_errno, const char **attachment)
{
  char *p, *p2;

  *remote_errno = FILEIO_SUCCESS;
  *attachment = NULL;

  if (buffer[0] != 'F')
    return -1;

  errno = 0;
  *retcode = strtol (&buffer[1], &p, 16);
  if (errno != 0 || p == &buffer[1])
    return -1;

  /* Check for ",errno".  */
  if (*p == ',')
    {
      errno = 0;
      *remote_errno = (fileio_error) strtol (p + 1, &p2, 16);
      if (errno != 0 || p + 1 == p2)
	return -1;
      p = p2;
    }

  /* Check for ";attachment".  If there is no attachment, the
     packet should end here.  */
  if (*p == ';')
    {
      *attachment = p + 1;
      return 0;
    }
  else if (*p == '\0')
    return 0;
  else
    return -1;
}

/* Send a prepared I/O packet to the target and read its response.
   The prepared packet is in the global RS->BUF before this function
   is called, and the answer is there when we return.

   COMMAND_BYTES is the length of the request to send, which may include
   binary data.  WHICH_PACKET is the packet configuration to check
   before attempting a packet.  If an error occurs, *REMOTE_ERRNO
   is set to the error number and -1 is returned.  Otherwise the value
   returned by the function is returned.

   ATTACHMENT and ATTACHMENT_LEN should be non-NULL if and only if an
   attachment is expected; an error will be reported if there's a
   mismatch.  If one is found, *ATTACHMENT will be set to point into
   the packet buffer and *ATTACHMENT_LEN will be set to the
   attachment's length.  */

int
remote_target::remote_hostio_send_command (int command_bytes, int which_packet,
					   fileio_error *remote_errno, const char **attachment,
					   int *attachment_len)
{
  struct remote_state *rs = get_remote_state ();
  int ret, bytes_read;
  const char *attachment_tmp;

  if (m_features.packet_support (which_packet) == PACKET_DISABLE)
    {
      *remote_errno = FILEIO_ENOSYS;
      return -1;
    }

  putpkt_binary (rs->buf.data (), command_bytes);
  bytes_read = getpkt (&rs->buf);

  /* If it timed out, something is wrong.  Don't try to parse the
     buffer.  */
  if (bytes_read < 0)
    {
      *remote_errno = FILEIO_EINVAL;
      return -1;
    }

  switch (m_features.packet_ok (rs->buf, which_packet))
    {
    case PACKET_ERROR:
      *remote_errno = FILEIO_EINVAL;
      return -1;
    case PACKET_UNKNOWN:
      *remote_errno = FILEIO_ENOSYS;
      return -1;
    case PACKET_OK:
      break;
    }

  if (remote_hostio_parse_result (rs->buf.data (), &ret, remote_errno,
				  &attachment_tmp))
    {
      *remote_errno = FILEIO_EINVAL;
      return -1;
    }

  /* Make sure we saw an attachment if and only if we expected one.  */
  if ((attachment_tmp == NULL && attachment != NULL)
      || (attachment_tmp != NULL && attachment == NULL))
    {
      *remote_errno = FILEIO_EINVAL;
      return -1;
    }

  /* If an attachment was found, it must point into the packet buffer;
     work out how many bytes there were.  */
  if (attachment_tmp != NULL)
    {
      *attachment = attachment_tmp;
      *attachment_len = bytes_read - (*attachment - rs->buf.data ());
    }

  return ret;
}

/* See declaration.h.  */

void
readahead_cache::invalidate ()
{
  this->fd = -1;
}

/* See declaration.h.  */

void
readahead_cache::invalidate_fd (int fd)
{
  if (this->fd == fd)
    this->fd = -1;
}

/* Set the filesystem remote_hostio functions that take FILENAME
   arguments will use.  Return 0 on success, or -1 if an error
   occurs (and set *REMOTE_ERRNO).  */

int
remote_target::remote_hostio_set_filesystem (struct inferior *inf,
					     fileio_error *remote_errno)
{
  struct remote_state *rs = get_remote_state ();
  int required_pid = (inf == NULL || inf->fake_pid_p) ? 0 : inf->pid;
  char *p = rs->buf.data ();
  int left = get_remote_packet_size () - 1;
  char arg[9];
  int ret;

  if (m_features.packet_support (PACKET_vFile_setfs) == PACKET_DISABLE)
    return 0;

  if (rs->fs_pid != -1 && required_pid == rs->fs_pid)
    return 0;

  remote_buffer_add_string (&p, &left, "vFile:setfs:");

  xsnprintf (arg, sizeof (arg), "%x", required_pid);
  remote_buffer_add_string (&p, &left, arg);

  ret = remote_hostio_send_command (p - rs->buf.data (), PACKET_vFile_setfs,
				    remote_errno, NULL, NULL);

  if (m_features.packet_support (PACKET_vFile_setfs) == PACKET_DISABLE)
    return 0;

  if (ret == 0)
    rs->fs_pid = required_pid;

  return ret;
}

/* Implementation of to_fileio_open.  */

int
remote_target::remote_hostio_open (inferior *inf, const char *filename,
				   int flags, int mode, int warn_if_slow,
				   fileio_error *remote_errno)
{
  struct remote_state *rs = get_remote_state ();
  char *p = rs->buf.data ();
  int left = get_remote_packet_size () - 1;

  if (warn_if_slow)
    {
      static int warning_issued = 0;

      gdb_printf (_("Reading %s from remote target...\n"),
		  filename);

      if (!warning_issued)
	{
	  warning (_("File transfers from remote targets can be slow."
		     " Use \"set sysroot\" to access files locally"
		     " instead."));
	  warning_issued = 1;
	}
    }

  if (remote_hostio_set_filesystem (inf, remote_errno) != 0)
    return -1;

  remote_buffer_add_string (&p, &left, "vFile:open:");

  remote_buffer_add_bytes (&p, &left, (const gdb_byte *) filename,
			   strlen (filename));
  remote_buffer_add_string (&p, &left, ",");

  remote_buffer_add_int (&p, &left, flags);
  remote_buffer_add_string (&p, &left, ",");

  remote_buffer_add_int (&p, &left, mode);

  return remote_hostio_send_command (p - rs->buf.data (), PACKET_vFile_open,
				     remote_errno, NULL, NULL);
}

int
remote_target::fileio_open (struct inferior *inf, const char *filename,
			    int flags, int mode, int warn_if_slow,
			    fileio_error *remote_errno)
{
  return remote_hostio_open (inf, filename, flags, mode, warn_if_slow,
			     remote_errno);
}

/* Implementation of to_fileio_pwrite.  */

int
remote_target::remote_hostio_pwrite (int fd, const gdb_byte *write_buf, int len,
				     ULONGEST offset, fileio_error *remote_errno)
{
  struct remote_state *rs = get_remote_state ();
  char *p = rs->buf.data ();
  int left = get_remote_packet_size ();
  int out_len;

  rs->readahead_cache.invalidate_fd (fd);

  remote_buffer_add_string (&p, &left, "vFile:pwrite:");

  remote_buffer_add_int (&p, &left, fd);
  remote_buffer_add_string (&p, &left, ",");

  remote_buffer_add_int (&p, &left, offset);
  remote_buffer_add_string (&p, &left, ",");

  p += remote_escape_output (write_buf, len, 1, (gdb_byte *) p, &out_len,
			     (get_remote_packet_size ()
			      - (p - rs->buf.data ())));

  return remote_hostio_send_command (p - rs->buf.data (), PACKET_vFile_pwrite,
				     remote_errno, NULL, NULL);
}

int
remote_target::fileio_pwrite (int fd, const gdb_byte *write_buf, int len,
			      ULONGEST offset, fileio_error *remote_errno)
{
  return remote_hostio_pwrite (fd, write_buf, len, offset, remote_errno);
}

/* Helper for the implementation of to_fileio_pread.  Read the file
   from the remote side with vFile:pread.  */

int
remote_target::remote_hostio_pread_vFile (int fd, gdb_byte *read_buf, int len,
					  ULONGEST offset, fileio_error *remote_errno)
{
  struct remote_state *rs = get_remote_state ();
  char *p = rs->buf.data ();
  const char *attachment;
  int left = get_remote_packet_size ();
  int ret, attachment_len;
  int read_len;

  remote_buffer_add_string (&p, &left, "vFile:pread:");

  remote_buffer_add_int (&p, &left, fd);
  remote_buffer_add_string (&p, &left, ",");

  remote_buffer_add_int (&p, &left, len);
  remote_buffer_add_string (&p, &left, ",");

  remote_buffer_add_int (&p, &left, offset);

  ret = remote_hostio_send_command (p - rs->buf.data (), PACKET_vFile_pread,
				    remote_errno, &attachment,
				    &attachment_len);

  if (ret < 0)
    return ret;

  read_len = remote_unescape_input ((gdb_byte *) attachment, attachment_len,
				    read_buf, len);
  if (read_len != ret)
    error (_("Read returned %d, but %d bytes."), ret, (int) read_len);

  return ret;
}

/* See declaration.h.  */

int
readahead_cache::pread (int fd, gdb_byte *read_buf, size_t len,
			ULONGEST offset)
{
  if (this->fd == fd
      && this->offset <= offset
      && offset < this->offset + this->buf.size ())
    {
      ULONGEST max = this->offset + this->buf.size ();

      if (offset + len > max)
	len = max - offset;

      memcpy (read_buf, &this->buf[offset - this->offset], len);
      return len;
    }

  return 0;
}

/* Implementation of to_fileio_pread.  */

int
remote_target::remote_hostio_pread (int fd, gdb_byte *read_buf, int len,
				    ULONGEST offset, fileio_error *remote_errno)
{
  int ret;
  struct remote_state *rs = get_remote_state ();
  readahead_cache *cache = &rs->readahead_cache;

  ret = cache->pread (fd, read_buf, len, offset);
  if (ret > 0)
    {
      cache->hit_count++;

      remote_debug_printf ("readahead cache hit %s",
			   pulongest (cache->hit_count));
      return ret;
    }

  cache->miss_count++;

  remote_debug_printf ("readahead cache miss %s",
		       pulongest (cache->miss_count));

  cache->fd = fd;
  cache->offset = offset;
  cache->buf.resize (get_remote_packet_size ());

  ret = remote_hostio_pread_vFile (cache->fd, &cache->buf[0],
				   cache->buf.size (),
				   cache->offset, remote_errno);
  if (ret <= 0)
    {
      cache->invalidate_fd (fd);
      return ret;
    }

  cache->buf.resize (ret);
  return cache->pread (fd, read_buf, len, offset);
}

int
remote_target::fileio_pread (int fd, gdb_byte *read_buf, int len,
			     ULONGEST offset, fileio_error *remote_errno)
{
  return remote_hostio_pread (fd, read_buf, len, offset, remote_errno);
}

/* Implementation of to_fileio_close.  */

int
remote_target::remote_hostio_close (int fd, fileio_error *remote_errno)
{
  struct remote_state *rs = get_remote_state ();
  char *p = rs->buf.data ();
  int left = get_remote_packet_size () - 1;

  rs->readahead_cache.invalidate_fd (fd);

  remote_buffer_add_string (&p, &left, "vFile:close:");

  remote_buffer_add_int (&p, &left, fd);

  return remote_hostio_send_command (p - rs->buf.data (), PACKET_vFile_close,
				     remote_errno, NULL, NULL);
}

int
remote_target::fileio_close (int fd, fileio_error *remote_errno)
{
  return remote_hostio_close (fd, remote_errno);
}

/* Implementation of to_fileio_unlink.  */

int
remote_target::remote_hostio_unlink (inferior *inf, const char *filename,
				     fileio_error *remote_errno)
{
  struct remote_state *rs = get_remote_state ();
  char *p = rs->buf.data ();
  int left = get_remote_packet_size () - 1;

  if (remote_hostio_set_filesystem (inf, remote_errno) != 0)
    return -1;

  remote_buffer_add_string (&p, &left, "vFile:unlink:");

  remote_buffer_add_bytes (&p, &left, (const gdb_byte *) filename,
			   strlen (filename));

  return remote_hostio_send_command (p - rs->buf.data (), PACKET_vFile_unlink,
				     remote_errno, NULL, NULL);
}

int
remote_target::fileio_unlink (struct inferior *inf, const char *filename,
			      fileio_error *remote_errno)
{
  return remote_hostio_unlink (inf, filename, remote_errno);
}

/* Implementation of to_fileio_readlink.  */

std::optional<std::string>
remote_target::fileio_readlink (struct inferior *inf, const char *filename,
				fileio_error *remote_errno)
{
  struct remote_state *rs = get_remote_state ();
  char *p = rs->buf.data ();
  const char *attachment;
  int left = get_remote_packet_size ();
  int len, attachment_len;
  int read_len;

  if (remote_hostio_set_filesystem (inf, remote_errno) != 0)
    return {};

  remote_buffer_add_string (&p, &left, "vFile:readlink:");

  remote_buffer_add_bytes (&p, &left, (const gdb_byte *) filename,
			   strlen (filename));

  len = remote_hostio_send_command (p - rs->buf.data (), PACKET_vFile_readlink,
				    remote_errno, &attachment,
				    &attachment_len);

  if (len < 0)
    return {};

  std::string ret (len, '\0');

  read_len = remote_unescape_input ((gdb_byte *) attachment, attachment_len,
				    (gdb_byte *) &ret[0], len);
  if (read_len != len)
    error (_("Readlink returned %d, but %d bytes."), len, read_len);

  return ret;
}

/* Implementation of to_fileio_fstat.  */

int
remote_target::fileio_fstat (int fd, struct stat *st, fileio_error *remote_errno)
{
  struct remote_state *rs = get_remote_state ();
  char *p = rs->buf.data ();
  int left = get_remote_packet_size ();
  int attachment_len, ret;
  const char *attachment;
  struct fio_stat fst;
  int read_len;

  remote_buffer_add_string (&p, &left, "vFile:fstat:");

  remote_buffer_add_int (&p, &left, fd);

  ret = remote_hostio_send_command (p - rs->buf.data (), PACKET_vFile_fstat,
				    remote_errno, &attachment,
				    &attachment_len);
  if (ret < 0)
    {
      if (*remote_errno != FILEIO_ENOSYS)
	return ret;

      /* Strictly we should return -1, ENOSYS here, but when
	 "set sysroot remote:" was implemented in August 2008
	 BFD's need for a stat function was sidestepped with
	 this hack.  This was not remedied until March 2015
	 so we retain the previous behavior to avoid breaking
	 compatibility.

	 Note that the memset is a March 2015 addition; older
	 GDBs set st_size *and nothing else* so the structure
	 would have garbage in all other fields.  This might
	 break something but retaining the previous behavior
	 here would be just too wrong.  */

      memset (st, 0, sizeof (struct stat));
      st->st_size = INT_MAX;
      return 0;
    }

  read_len = remote_unescape_input ((gdb_byte *) attachment, attachment_len,
				    (gdb_byte *) &fst, sizeof (fst));

  if (read_len != ret)
    error (_("vFile:fstat returned %d, but %d bytes."), ret, read_len);

  if (read_len != sizeof (fst))
    error (_("vFile:fstat returned %d bytes, but expecting %d."),
	   read_len, (int) sizeof (fst));

  remote_fileio_to_host_stat (&fst, st);

  return 0;
}

/* Implementation of to_filesystem_is_local.  */

bool
remote_target::filesystem_is_local ()
{
  /* Valgrind GDB presents itself as a remote target but works
     on the local filesystem: it does not implement remote get
     and users are not expected to set a sysroot.  To handle
     this case we treat the remote filesystem as local if the
     sysroot is exactly TARGET_SYSROOT_PREFIX and if the stub
     does not support vFile:open.  */
  if (gdb_sysroot == TARGET_SYSROOT_PREFIX)
    {
      packet_support ps = m_features.packet_support (PACKET_vFile_open);

      if (ps == PACKET_SUPPORT_UNKNOWN)
	{
	  int fd;
	  fileio_error remote_errno;

	  /* Try opening a file to probe support.  The supplied
	     filename is irrelevant, we only care about whether
	     the stub recognizes the packet or not.  */
	  fd = remote_hostio_open (NULL, "just probing",
				   FILEIO_O_RDONLY, 0700, 0,
				   &remote_errno);

	  if (fd >= 0)
	    remote_hostio_close (fd, &remote_errno);

	  ps = m_features.packet_support (PACKET_vFile_open);
	}

      if (ps == PACKET_DISABLE)
	{
	  static int warning_issued = 0;

	  if (!warning_issued)
	    {
	      warning (_("remote target does not support file"
			 " transfer, attempting to access files"
			 " from local filesystem."));
	      warning_issued = 1;
	    }

	  return true;
	}
    }

  return false;
}

static char *
remote_hostio_error (fileio_error errnum)
{
  int host_error = fileio_error_to_host (errnum);

  if (host_error == -1)
    error (_("Unknown remote I/O error %d"), errnum);
  else
    error (_("Remote I/O error: %s"), safe_strerror (host_error));
}

/* A RAII wrapper around a remote file descriptor.  */

class scoped_remote_fd
{
public:
  scoped_remote_fd (remote_target *remote, int fd)
    : m_remote (remote), m_fd (fd)
  {
  }

  ~scoped_remote_fd ()
  {
    if (m_fd != -1)
      {
	try
	  {
	    fileio_error remote_errno;
	    m_remote->remote_hostio_close (m_fd, &remote_errno);
	  }
	catch (...)
	  {
	    /* Swallow exception before it escapes the dtor.  If
	       something goes wrong, likely the connection is gone,
	       and there's nothing else that can be done.  */
	  }
      }
  }

  DISABLE_COPY_AND_ASSIGN (scoped_remote_fd);

  /* Release ownership of the file descriptor, and return it.  */
  ATTRIBUTE_UNUSED_RESULT int release () noexcept
  {
    int fd = m_fd;
    m_fd = -1;
    return fd;
  }

  /* Return the owned file descriptor.  */
  int get () const noexcept
  {
    return m_fd;
  }

private:
  /* The remote target.  */
  remote_target *m_remote;

  /* The owned remote I/O file descriptor.  */
  int m_fd;
};

void
remote_file_put (const char *local_file, const char *remote_file, int from_tty)
{
  remote_target *remote = get_current_remote_target ();

  if (remote == nullptr)
    error (_("command can only be used with remote target"));

  remote->remote_file_put (local_file, remote_file, from_tty);
}

void
remote_target::remote_file_put (const char *local_file, const char *remote_file,
				int from_tty)
{
  int retcode, bytes, io_size;
  fileio_error remote_errno;
  int bytes_in_buffer;
  int saw_eof;
  ULONGEST offset;

  gdb_file_up file = gdb_fopen_cloexec (local_file, "rb");
  if (file == NULL)
    perror_with_name (local_file);

  scoped_remote_fd fd
    (this, remote_hostio_open (NULL,
			       remote_file, (FILEIO_O_WRONLY | FILEIO_O_CREAT
					     | FILEIO_O_TRUNC),
			       0700, 0, &remote_errno));
  if (fd.get () == -1)
    remote_hostio_error (remote_errno);

  /* Send up to this many bytes at once.  They won't all fit in the
     remote packet limit, so we'll transfer slightly fewer.  */
  io_size = get_remote_packet_size ();
  gdb::byte_vector buffer (io_size);

  bytes_in_buffer = 0;
  saw_eof = 0;
  offset = 0;
  while (bytes_in_buffer || !saw_eof)
    {
      if (!saw_eof)
	{
	  bytes = fread (buffer.data () + bytes_in_buffer, 1,
			 io_size - bytes_in_buffer,
			 file.get ());
	  if (bytes == 0)
	    {
	      if (ferror (file.get ()))
		error (_("Error reading %s."), local_file);
	      else
		{
		  /* EOF.  Unless there is something still in the
		     buffer from the last iteration, we are done.  */
		  saw_eof = 1;
		  if (bytes_in_buffer == 0)
		    break;
		}
	    }
	}
      else
	bytes = 0;

      bytes += bytes_in_buffer;
      bytes_in_buffer = 0;

      retcode = remote_hostio_pwrite (fd.get (), buffer.data (), bytes,
				      offset, &remote_errno);

      if (retcode < 0)
	remote_hostio_error (remote_errno);
      else if (retcode == 0)
	error (_("Remote write of %d bytes returned 0!"), bytes);
      else if (retcode < bytes)
	{
	  /* Short write.  Save the rest of the read data for the next
	     write.  */
	  bytes_in_buffer = bytes - retcode;
	  memmove (buffer.data (), buffer.data () + retcode, bytes_in_buffer);
	}

      offset += retcode;
    }

  if (remote_hostio_close (fd.release (), &remote_errno))
    remote_hostio_error (remote_errno);

  if (from_tty)
    gdb_printf (_("Successfully sent file \"%s\".\n"), local_file);
}

void
remote_file_get (const char *remote_file, const char *local_file, int from_tty)
{
  remote_target *remote = get_current_remote_target ();

  if (remote == nullptr)
    error (_("command can only be used with remote target"));

  remote->remote_file_get (remote_file, local_file, from_tty);
}

void
remote_target::remote_file_get (const char *remote_file, const char *local_file,
				int from_tty)
{
  fileio_error remote_errno;
  int bytes, io_size;
  ULONGEST offset;

  scoped_remote_fd fd
    (this, remote_hostio_open (NULL,
			       remote_file, FILEIO_O_RDONLY, 0, 0,
			       &remote_errno));
  if (fd.get () == -1)
    remote_hostio_error (remote_errno);

  gdb_file_up file = gdb_fopen_cloexec (local_file, "wb");
  if (file == NULL)
    perror_with_name (local_file);

  /* Send up to this many bytes at once.  They won't all fit in the
     remote packet limit, so we'll transfer slightly fewer.  */
  io_size = get_remote_packet_size ();
  gdb::byte_vector buffer (io_size);

  offset = 0;
  while (1)
    {
      bytes = remote_hostio_pread (fd.get (), buffer.data (), io_size, offset,
				   &remote_errno);
      if (bytes == 0)
	/* Success, but no bytes, means end-of-file.  */
	break;
      if (bytes == -1)
	remote_hostio_error (remote_errno);

      offset += bytes;

      bytes = fwrite (buffer.data (), 1, bytes, file.get ());
      if (bytes == 0)
	perror_with_name (local_file);
    }

  if (remote_hostio_close (fd.release (), &remote_errno))
    remote_hostio_error (remote_errno);

  if (from_tty)
    gdb_printf (_("Successfully fetched file \"%s\".\n"), remote_file);
}

void
remote_file_delete (const char *remote_file, int from_tty)
{
  remote_target *remote = get_current_remote_target ();

  if (remote == nullptr)
    error (_("command can only be used with remote target"));

  remote->remote_file_delete (remote_file, from_tty);
}

void
remote_target::remote_file_delete (const char *remote_file, int from_tty)
{
  int retcode;
  fileio_error remote_errno;

  retcode = remote_hostio_unlink (NULL, remote_file, &remote_errno);
  if (retcode == -1)
    remote_hostio_error (remote_errno);

  if (from_tty)
    gdb_printf (_("Successfully deleted file \"%s\".\n"), remote_file);
}

static void
remote_put_command (const char *args, int from_tty)
{
  if (args == NULL)
    error_no_arg (_("file to put"));

  gdb_argv argv (args);
  if (argv[0] == NULL || argv[1] == NULL || argv[2] != NULL)
    error (_("Invalid parameters to remote put"));

  remote_file_put (argv[0], argv[1], from_tty);
}

static void
remote_get_command (const char *args, int from_tty)
{
  if (args == NULL)
    error_no_arg (_("file to get"));

  gdb_argv argv (args);
  if (argv[0] == NULL || argv[1] == NULL || argv[2] != NULL)
    error (_("Invalid parameters to remote get"));

  remote_file_get (argv[0], argv[1], from_tty);
}

static void
remote_delete_command (const char *args, int from_tty)
{
  if (args == NULL)
    error_no_arg (_("file to delete"));

  gdb_argv argv (args);
  if (argv[0] == NULL || argv[1] != NULL)
    error (_("Invalid parameters to remote delete"));

  remote_file_delete (argv[0], from_tty);
}

bool
remote_target::can_execute_reverse ()
{
  if (m_features.packet_support (PACKET_bs) == PACKET_ENABLE
      || m_features.packet_support (PACKET_bc) == PACKET_ENABLE)
    return true;
  else
    return false;
}

bool
remote_target::supports_non_stop ()
{
  return true;
}

bool
remote_target::supports_disable_randomization ()
{
  /* Only supported in extended mode.  */
  return false;
}

bool
remote_target::supports_multi_process ()
{
  return m_features.remote_multi_process_p ();
}

int
remote_target::remote_supports_cond_tracepoints ()
{
  return (m_features.packet_support (PACKET_ConditionalTracepoints)
	  == PACKET_ENABLE);
}

bool
remote_target::supports_evaluation_of_breakpoint_conditions ()
{
  return (m_features.packet_support (PACKET_ConditionalBreakpoints)
	  == PACKET_ENABLE);
}

int
remote_target::remote_supports_fast_tracepoints ()
{
  return m_features.packet_support (PACKET_FastTracepoints) == PACKET_ENABLE;
}

int
remote_target::remote_supports_static_tracepoints ()
{
  return m_features.packet_support (PACKET_StaticTracepoints) == PACKET_ENABLE;
}

int
remote_target::remote_supports_install_in_trace ()
{
  return m_features.packet_support (PACKET_InstallInTrace) == PACKET_ENABLE;
}

bool
remote_target::supports_enable_disable_tracepoint ()
{
  return (m_features.packet_support (PACKET_EnableDisableTracepoints_feature)
	  == PACKET_ENABLE);
}

bool
remote_target::supports_string_tracing ()
{
  return m_features.packet_support (PACKET_tracenz_feature) == PACKET_ENABLE;
}

bool
remote_target::can_run_breakpoint_commands ()
{
  return m_features.packet_support (PACKET_BreakpointCommands) == PACKET_ENABLE;
}

void
remote_target::trace_init ()
{
  struct remote_state *rs = get_remote_state ();

  putpkt ("QTinit");
  remote_get_noisy_reply ();
  if (strcmp (rs->buf.data (), "OK") != 0)
    error (_("Target does not support this command."));
}

/* Recursive routine to walk through command list including loops, and
   download packets for each command.  */

void
remote_target::remote_download_command_source (int num, ULONGEST addr,
					       struct command_line *cmds)
{
  struct remote_state *rs = get_remote_state ();
  struct command_line *cmd;

  for (cmd = cmds; cmd; cmd = cmd->next)
    {
      QUIT;	/* Allow user to bail out with ^C.  */
      strcpy (rs->buf.data (), "QTDPsrc:");
      encode_source_string (num, addr, "cmd", cmd->line,
			    rs->buf.data () + strlen (rs->buf.data ()),
			    rs->buf.size () - strlen (rs->buf.data ()));
      putpkt (rs->buf);
      remote_get_noisy_reply ();
      if (strcmp (rs->buf.data (), "OK"))
	warning (_("Target does not support source download."));

      if (cmd->control_type == while_control
	  || cmd->control_type == while_stepping_control)
	{
	  remote_download_command_source (num, addr, cmd->body_list_0.get ());

	  QUIT;	/* Allow user to bail out with ^C.  */
	  strcpy (rs->buf.data (), "QTDPsrc:");
	  encode_source_string (num, addr, "cmd", "end",
				rs->buf.data () + strlen (rs->buf.data ()),
				rs->buf.size () - strlen (rs->buf.data ()));
	  putpkt (rs->buf);
	  remote_get_noisy_reply ();
	  if (strcmp (rs->buf.data (), "OK"))
	    warning (_("Target does not support source download."));
	}
    }
}

void
remote_target::download_tracepoint (struct bp_location *loc)
{
  CORE_ADDR tpaddr;
  char addrbuf[40];
  std::vector<std::string> tdp_actions;
  std::vector<std::string> stepping_actions;
  char *pkt;
  struct breakpoint *b = loc->owner;
  tracepoint *t = gdb::checked_static_cast<tracepoint *> (b);
  struct remote_state *rs = get_remote_state ();
  int ret;
  const char *err_msg = _("Tracepoint packet too large for target.");
  size_t size_left;

  /* We use a buffer other than rs->buf because we'll build strings
     across multiple statements, and other statements in between could
     modify rs->buf.  */
  gdb::char_vector buf (get_remote_packet_size ());

  encode_actions_rsp (loc, &tdp_actions, &stepping_actions);

  tpaddr = loc->address;
  strcpy (addrbuf, phex (tpaddr, sizeof (CORE_ADDR)));
  ret = snprintf (buf.data (), buf.size (), "QTDP:%x:%s:%c:%lx:%x",
		  b->number, addrbuf, /* address */
		  (b->enable_state == bp_enabled ? 'E' : 'D'),
		  t->step_count, t->pass_count);

  if (ret < 0 || ret >= buf.size ())
    error ("%s", err_msg);

  /* Fast tracepoints are mostly handled by the target, but we can
     tell the target how big of an instruction block should be moved
     around.  */
  if (b->type == bp_fast_tracepoint)
    {
      /* Only test for support at download time; we may not know
	 target capabilities at definition time.  */
      if (remote_supports_fast_tracepoints ())
	{
	  if (gdbarch_fast_tracepoint_valid_at (loc->gdbarch, tpaddr,
						NULL))
	    {
	      size_left = buf.size () - strlen (buf.data ());
	      ret = snprintf (buf.data () + strlen (buf.data ()),
			      size_left, ":F%x",
			      gdb_insn_length (loc->gdbarch, tpaddr));

	      if (ret < 0 || ret >= size_left)
		error ("%s", err_msg);
	    }
	  else
	    /* If it passed validation at definition but fails now,
	       something is very wrong.  */
	    internal_error (_("Fast tracepoint not valid during download"));
	}
      else
	/* Fast tracepoints are functionally identical to regular
	   tracepoints, so don't take lack of support as a reason to
	   give up on the trace run.  */
	warning (_("Target does not support fast tracepoints, "
		   "downloading %d as regular tracepoint"), b->number);
    }
  else if (b->type == bp_static_tracepoint
	   || b->type == bp_static_marker_tracepoint)
    {
      /* Only test for support at download time; we may not know
	 target capabilities at definition time.  */
      if (remote_supports_static_tracepoints ())
	{
	  struct static_tracepoint_marker marker;

	  if (target_static_tracepoint_marker_at (tpaddr, &marker))
	    {
	      size_left = buf.size () - strlen (buf.data ());
	      ret = snprintf (buf.data () + strlen (buf.data ()),
			      size_left, ":S");

	      if (ret < 0 || ret >= size_left)
		error ("%s", err_msg);
	    }
	  else
	    error (_("Static tracepoint not valid during download"));
	}
      else
	/* Fast tracepoints are functionally identical to regular
	   tracepoints, so don't take lack of support as a reason
	   to give up on the trace run.  */
	error (_("Target does not support static tracepoints"));
    }
  /* If the tracepoint has a conditional, make it into an agent
     expression and append to the definition.  */
  if (loc->cond)
    {
      /* Only test support at download time, we may not know target
	 capabilities at definition time.  */
      if (remote_supports_cond_tracepoints ())
	{
	  agent_expr_up aexpr = gen_eval_for_expr (tpaddr,
						   loc->cond.get ());

	  size_left = buf.size () - strlen (buf.data ());

	  ret = snprintf (buf.data () + strlen (buf.data ()),
			  size_left, ":X%x,", (int) aexpr->buf.size ());

	  if (ret < 0 || ret >= size_left)
	    error ("%s", err_msg);

	  size_left = buf.size () - strlen (buf.data ());

	  /* Two bytes to encode each aexpr byte, plus the terminating
	     null byte.  */
	  if (aexpr->buf.size () * 2 + 1 > size_left)
	    error ("%s", err_msg);

	  pkt = buf.data () + strlen (buf.data ());

	  for (int ndx = 0; ndx < aexpr->buf.size (); ++ndx)
	    pkt = pack_hex_byte (pkt, aexpr->buf[ndx]);
	  *pkt = '\0';
	}
      else
	warning (_("Target does not support conditional tracepoints, "
		   "ignoring tp %d cond"), b->number);
    }

  if (b->commands || !default_collect.empty ())
    {
      size_left = buf.size () - strlen (buf.data ());

      ret = snprintf (buf.data () + strlen (buf.data ()),
		      size_left, "-");

      if (ret < 0 || ret >= size_left)
	error ("%s", err_msg);
    }

  putpkt (buf.data ());
  remote_get_noisy_reply ();
  if (strcmp (rs->buf.data (), "OK"))
    error (_("Target does not support tracepoints."));

  /* do_single_steps (t); */
  for (auto action_it = tdp_actions.begin ();
       action_it != tdp_actions.end (); action_it++)
    {
      QUIT;	/* Allow user to bail out with ^C.  */

      bool has_more = ((action_it + 1) != tdp_actions.end ()
		       || !stepping_actions.empty ());

      ret = snprintf (buf.data (), buf.size (), "QTDP:-%x:%s:%s%c",
		      b->number, addrbuf, /* address */
		      action_it->c_str (),
		      has_more ? '-' : 0);

      if (ret < 0 || ret >= buf.size ())
	error ("%s", err_msg);

      putpkt (buf.data ());
      remote_get_noisy_reply ();
      if (strcmp (rs->buf.data (), "OK"))
	error (_("Error on target while setting tracepoints."));
    }

  for (auto action_it = stepping_actions.begin ();
       action_it != stepping_actions.end (); action_it++)
    {
      QUIT;	/* Allow user to bail out with ^C.  */

      bool is_first = action_it == stepping_actions.begin ();
      bool has_more = (action_it + 1) != stepping_actions.end ();

      ret = snprintf (buf.data (), buf.size (), "QTDP:-%x:%s:%s%s%s",
		      b->number, addrbuf, /* address */
		      is_first ? "S" : "",
		      action_it->c_str (),
		      has_more ? "-" : "");

      if (ret < 0 || ret >= buf.size ())
	error ("%s", err_msg);

      putpkt (buf.data ());
      remote_get_noisy_reply ();
      if (strcmp (rs->buf.data (), "OK"))
	error (_("Error on target while setting tracepoints."));
    }

  if (m_features.packet_support (PACKET_TracepointSource) == PACKET_ENABLE)
    {
      if (b->locspec != nullptr)
	{
	  ret = snprintf (buf.data (), buf.size (), "QTDPsrc:");

	  if (ret < 0 || ret >= buf.size ())
	    error ("%s", err_msg);

	  const char *str = b->locspec->to_string ();
	  encode_source_string (b->number, loc->address, "at", str,
				buf.data () + strlen (buf.data ()),
				buf.size () - strlen (buf.data ()));
	  putpkt (buf.data ());
	  remote_get_noisy_reply ();
	  if (strcmp (rs->buf.data (), "OK"))
	    warning (_("Target does not support source download."));
	}
      if (b->cond_string)
	{
	  ret = snprintf (buf.data (), buf.size (), "QTDPsrc:");

	  if (ret < 0 || ret >= buf.size ())
	    error ("%s", err_msg);

	  encode_source_string (b->number, loc->address,
				"cond", b->cond_string.get (),
				buf.data () + strlen (buf.data ()),
				buf.size () - strlen (buf.data ()));
	  putpkt (buf.data ());
	  remote_get_noisy_reply ();
	  if (strcmp (rs->buf.data (), "OK"))
	    warning (_("Target does not support source download."));
	}
      remote_download_command_source (b->number, loc->address,
				      breakpoint_commands (b));
    }
}

bool
remote_target::can_download_tracepoint ()
{
  struct remote_state *rs = get_remote_state ();
  struct trace_status *ts;
  int status;

  /* Don't try to install tracepoints until we've relocated our
     symbols, and fetched and merged the target's tracepoint list with
     ours.  */
  if (rs->starting_up)
    return false;

  ts = current_trace_status ();
  status = get_trace_status (ts);

  if (status == -1 || !ts->running_known || !ts->running)
    return false;

  /* If we are in a tracing experiment, but remote stub doesn't support
     installing tracepoint in trace, we have to return.  */
  if (!remote_supports_install_in_trace ())
    return false;

  return true;
}


void
remote_target::download_trace_state_variable (const trace_state_variable &tsv)
{
  struct remote_state *rs = get_remote_state ();
  char *p;

  xsnprintf (rs->buf.data (), get_remote_packet_size (), "QTDV:%x:%s:%x:",
	     tsv.number, phex ((ULONGEST) tsv.initial_value, 8),
	     tsv.builtin);
  p = rs->buf.data () + strlen (rs->buf.data ());
  if ((p - rs->buf.data ()) + tsv.name.length () * 2
      >= get_remote_packet_size ())
    error (_("Trace state variable name too long for tsv definition packet"));
  p += 2 * bin2hex ((gdb_byte *) (tsv.name.data ()), p, tsv.name.length ());
  *p++ = '\0';
  putpkt (rs->buf);
  remote_get_noisy_reply ();
  if (rs->buf[0] == '\0')
    error (_("Target does not support this command."));
  if (strcmp (rs->buf.data (), "OK") != 0)
    error (_("Error on target while downloading trace state variable."));
}

void
remote_target::enable_tracepoint (struct bp_location *location)
{
  struct remote_state *rs = get_remote_state ();

  xsnprintf (rs->buf.data (), get_remote_packet_size (), "QTEnable:%x:%s",
	     location->owner->number,
	     phex (location->address, sizeof (CORE_ADDR)));
  putpkt (rs->buf);
  remote_get_noisy_reply ();
  if (rs->buf[0] == '\0')
    error (_("Target does not support enabling tracepoints while a trace run is ongoing."));
  if (strcmp (rs->buf.data (), "OK") != 0)
    error (_("Error on target while enabling tracepoint."));
}

void
remote_target::disable_tracepoint (struct bp_location *location)
{
  struct remote_state *rs = get_remote_state ();

  xsnprintf (rs->buf.data (), get_remote_packet_size (), "QTDisable:%x:%s",
	     location->owner->number,
	     phex (location->address, sizeof (CORE_ADDR)));
  putpkt (rs->buf);
  remote_get_noisy_reply ();
  if (rs->buf[0] == '\0')
    error (_("Target does not support disabling tracepoints while a trace run is ongoing."));
  if (strcmp (rs->buf.data (), "OK") != 0)
    error (_("Error on target while disabling tracepoint."));
}

void
remote_target::trace_set_readonly_regions ()
{
  asection *s;
  bfd_size_type size;
  bfd_vma vma;
  int anysecs = 0;
  int offset = 0;
  bfd *abfd = current_program_space->exec_bfd ();

  if (!abfd)
    return;			/* No information to give.  */

  struct remote_state *rs = get_remote_state ();

  strcpy (rs->buf.data (), "QTro");
  offset = strlen (rs->buf.data ());
  for (s = abfd->sections; s; s = s->next)
    {
      char tmp1[40], tmp2[40];
      int sec_length;

      if ((s->flags & SEC_LOAD) == 0
	  /* || (s->flags & SEC_CODE) == 0 */
	  || (s->flags & SEC_READONLY) == 0)
	continue;

      anysecs = 1;
      vma = bfd_section_vma (s);
      size = bfd_section_size (s);
      bfd_sprintf_vma (abfd, tmp1, vma);
      bfd_sprintf_vma (abfd, tmp2, vma + size);
      sec_length = 1 + strlen (tmp1) + 1 + strlen (tmp2);
      if (offset + sec_length + 1 > rs->buf.size ())
	{
	  if (m_features.packet_support (PACKET_qXfer_traceframe_info)
	      != PACKET_ENABLE)
	    warning (_("\
Too many sections for read-only sections definition packet."));
	  break;
	}
      xsnprintf (rs->buf.data () + offset, rs->buf.size () - offset, ":%s,%s",
		 tmp1, tmp2);
      offset += sec_length;
    }
  if (anysecs)
    {
      putpkt (rs->buf);
      getpkt (&rs->buf);
    }
}

void
remote_target::trace_start ()
{
  struct remote_state *rs = get_remote_state ();

  putpkt ("QTStart");
  remote_get_noisy_reply ();
  if (rs->buf[0] == '\0')
    error (_("Target does not support this command."));
  if (strcmp (rs->buf.data (), "OK") != 0)
    error (_("Bogus reply from target: %s"), rs->buf.data ());
}

int
remote_target::get_trace_status (struct trace_status *ts)
{
  /* Initialize it just to avoid a GCC false warning.  */
  char *p = NULL;
  enum packet_result result;
  struct remote_state *rs = get_remote_state ();

  if (m_features.packet_support (PACKET_qTStatus) == PACKET_DISABLE)
    return -1;

  /* FIXME we need to get register block size some other way.  */
  trace_regblock_size
    = rs->get_remote_arch_state (current_inferior ()->arch ())->sizeof_g_packet;

  putpkt ("qTStatus");

  try
    {
      p = remote_get_noisy_reply ();
    }
  catch (const gdb_exception_error &ex)
    {
      if (ex.error != TARGET_CLOSE_ERROR)
	{
	  exception_fprintf (gdb_stderr, ex, "qTStatus: ");
	  return -1;
	}
      throw;
    }

  result = m_features.packet_ok (p, PACKET_qTStatus);

  /* If the remote target doesn't do tracing, flag it.  */
  if (result == PACKET_UNKNOWN)
    return -1;

  /* We're working with a live target.  */
  ts->filename = NULL;

  if (*p++ != 'T')
    error (_("Bogus trace status reply from target: %s"), rs->buf.data ());

  /* Function 'parse_trace_status' sets default value of each field of
     'ts' at first, so we don't have to do it here.  */
  parse_trace_status (p, ts);

  return ts->running;
}

void
remote_target::get_tracepoint_status (tracepoint *tp,
				      struct uploaded_tp *utp)
{
  struct remote_state *rs = get_remote_state ();
  char *reply;
  size_t size = get_remote_packet_size ();

  if (tp)
    {
      tp->hit_count = 0;
      tp->traceframe_usage = 0;
      for (bp_location &loc : tp->locations ())
	{
	  /* If the tracepoint was never downloaded, don't go asking for
	     any status.  */
	  if (tp->number_on_target == 0)
	    continue;
	  xsnprintf (rs->buf.data (), size, "qTP:%x:%s", tp->number_on_target,
		     phex_nz (loc.address, 0));
	  putpkt (rs->buf);
	  reply = remote_get_noisy_reply ();
	  if (reply && *reply)
	    {
	      if (*reply == 'V')
		parse_tracepoint_status (reply + 1, tp, utp);
	    }
	}
    }
  else if (utp)
    {
      utp->hit_count = 0;
      utp->traceframe_usage = 0;
      xsnprintf (rs->buf.data (), size, "qTP:%x:%s", utp->number,
		 phex_nz (utp->addr, 0));
      putpkt (rs->buf);
      reply = remote_get_noisy_reply ();
      if (reply && *reply)
	{
	  if (*reply == 'V')
	    parse_tracepoint_status (reply + 1, tp, utp);
	}
    }
}

void
remote_target::trace_stop ()
{
  struct remote_state *rs = get_remote_state ();

  putpkt ("QTStop");
  remote_get_noisy_reply ();
  if (rs->buf[0] == '\0')
    error (_("Target does not support this command."));
  if (strcmp (rs->buf.data (), "OK") != 0)
    error (_("Bogus reply from target: %s"), rs->buf.data ());
}

int
remote_target::trace_find (enum trace_find_type type, int num,
			   CORE_ADDR addr1, CORE_ADDR addr2,
			   int *tpp)
{
  struct remote_state *rs = get_remote_state ();
  char *endbuf = rs->buf.data () + get_remote_packet_size ();
  char *p, *reply;
  int target_frameno = -1, target_tracept = -1;

  /* Lookups other than by absolute frame number depend on the current
     trace selected, so make sure it is correct on the remote end
     first.  */
  if (type != tfind_number)
    set_remote_traceframe ();

  p = rs->buf.data ();
  strcpy (p, "QTFrame:");
  p = strchr (p, '\0');
  switch (type)
    {
    case tfind_number:
      xsnprintf (p, endbuf - p, "%x", num);
      break;
    case tfind_pc:
      xsnprintf (p, endbuf - p, "pc:%s", phex_nz (addr1, 0));
      break;
    case tfind_tp:
      xsnprintf (p, endbuf - p, "tdp:%x", num);
      break;
    case tfind_range:
      xsnprintf (p, endbuf - p, "range:%s:%s", phex_nz (addr1, 0),
		 phex_nz (addr2, 0));
      break;
    case tfind_outside:
      xsnprintf (p, endbuf - p, "outside:%s:%s", phex_nz (addr1, 0),
		 phex_nz (addr2, 0));
      break;
    default:
      error (_("Unknown trace find type %d"), type);
    }

  putpkt (rs->buf);
  reply = remote_get_noisy_reply ();
  if (*reply == '\0')
    error (_("Target does not support this command."));

  while (reply && *reply)
    switch (*reply)
      {
      case 'F':
	p = ++reply;
	target_frameno = (int) strtol (p, &reply, 16);
	if (reply == p)
	  error (_("Unable to parse trace frame number"));
	/* Don't update our remote traceframe number cache on failure
	   to select a remote traceframe.  */
	if (target_frameno == -1)
	  return -1;
	break;
      case 'T':
	p = ++reply;
	target_tracept = (int) strtol (p, &reply, 16);
	if (reply == p)
	  error (_("Unable to parse tracepoint number"));
	break;
      case 'O':		/* "OK"? */
	if (reply[1] == 'K' && reply[2] == '\0')
	  reply += 2;
	else
	  error (_("Bogus reply from target: %s"), reply);
	break;
      default:
	error (_("Bogus reply from target: %s"), reply);
      }
  if (tpp)
    *tpp = target_tracept;

  rs->remote_traceframe_number = target_frameno;
  return target_frameno;
}

bool
remote_target::get_trace_state_variable_value (int tsvnum, LONGEST *val)
{
  struct remote_state *rs = get_remote_state ();
  char *reply;
  ULONGEST uval;

  set_remote_traceframe ();

  xsnprintf (rs->buf.data (), get_remote_packet_size (), "qTV:%x", tsvnum);
  putpkt (rs->buf);
  reply = remote_get_noisy_reply ();
  if (reply && *reply)
    {
      if (*reply == 'V')
	{
	  unpack_varlen_hex (reply + 1, &uval);
	  *val = (LONGEST) uval;
	  return true;
	}
    }
  return false;
}

int
remote_target::save_trace_data (const char *filename)
{
  struct remote_state *rs = get_remote_state ();
  char *p, *reply;

  p = rs->buf.data ();
  strcpy (p, "QTSave:");
  p += strlen (p);
  if ((p - rs->buf.data ()) + strlen (filename) * 2
      >= get_remote_packet_size ())
    error (_("Remote file name too long for trace save packet"));
  p += 2 * bin2hex ((gdb_byte *) filename, p, strlen (filename));
  *p++ = '\0';
  putpkt (rs->buf);
  reply = remote_get_noisy_reply ();
  if (*reply == '\0')
    error (_("Target does not support this command."));
  if (strcmp (reply, "OK") != 0)
    error (_("Bogus reply from target: %s"), reply);
  return 0;
}

/* This is basically a memory transfer, but needs to be its own packet
   because we don't know how the target actually organizes its trace
   memory, plus we want to be able to ask for as much as possible, but
   not be unhappy if we don't get as much as we ask for.  */

LONGEST
remote_target::get_raw_trace_data (gdb_byte *buf, ULONGEST offset, LONGEST len)
{
  struct remote_state *rs = get_remote_state ();
  char *reply;
  char *p;
  int rslt;

  p = rs->buf.data ();
  strcpy (p, "qTBuffer:");
  p += strlen (p);
  p += hexnumstr (p, offset);
  *p++ = ',';
  p += hexnumstr (p, len);
  *p++ = '\0';

  putpkt (rs->buf);
  reply = remote_get_noisy_reply ();
  if (reply && *reply)
    {
      /* 'l' by itself means we're at the end of the buffer and
	 there is nothing more to get.  */
      if (*reply == 'l')
	return 0;

      /* Convert the reply into binary.  Limit the number of bytes to
	 convert according to our passed-in buffer size, rather than
	 what was returned in the packet; if the target is
	 unexpectedly generous and gives us a bigger reply than we
	 asked for, we don't want to crash.  */
      rslt = hex2bin (reply, buf, len);
      return rslt;
    }

  /* Something went wrong, flag as an error.  */
  return -1;
}

void
remote_target::set_disconnected_tracing (int val)
{
  struct remote_state *rs = get_remote_state ();

  if (m_features.packet_support (PACKET_DisconnectedTracing_feature)
      == PACKET_ENABLE)
    {
      char *reply;

      xsnprintf (rs->buf.data (), get_remote_packet_size (),
		 "QTDisconnected:%x", val);
      putpkt (rs->buf);
      reply = remote_get_noisy_reply ();
      if (*reply == '\0')
	error (_("Target does not support this command."));
      if (strcmp (reply, "OK") != 0)
	error (_("Bogus reply from target: %s"), reply);
    }
  else if (val)
    warning (_("Target does not support disconnected tracing."));
}

int
remote_target::core_of_thread (ptid_t ptid)
{
  thread_info *info = this->find_thread (ptid);

  if (info != NULL && info->priv != NULL)
    return get_remote_thread_info (info)->core;

  return -1;
}

void
remote_target::set_circular_trace_buffer (int val)
{
  struct remote_state *rs = get_remote_state ();
  char *reply;

  xsnprintf (rs->buf.data (), get_remote_packet_size (),
	     "QTBuffer:circular:%x", val);
  putpkt (rs->buf);
  reply = remote_get_noisy_reply ();
  if (*reply == '\0')
    error (_("Target does not support this command."));
  if (strcmp (reply, "OK") != 0)
    error (_("Bogus reply from target: %s"), reply);
}

traceframe_info_up
remote_target::traceframe_info ()
{
  std::optional<gdb::char_vector> text
    = target_read_stralloc (current_inferior ()->top_target (),
			    TARGET_OBJECT_TRACEFRAME_INFO,
			    NULL);
  if (text)
    return parse_traceframe_info (text->data ());

  return NULL;
}

/* Handle the qTMinFTPILen packet.  Returns the minimum length of
   instruction on which a fast tracepoint may be placed.  Returns -1
   if the packet is not supported, and 0 if the minimum instruction
   length is unknown.  */

int
remote_target::get_min_fast_tracepoint_insn_len ()
{
  struct remote_state *rs = get_remote_state ();
  char *reply;

  /* If we're not debugging a process yet, the IPA can't be
     loaded.  */
  if (!target_has_execution ())
    return 0;

  /* Make sure the remote is pointing at the right process.  */
  set_general_process ();

  xsnprintf (rs->buf.data (), get_remote_packet_size (), "qTMinFTPILen");
  putpkt (rs->buf);
  reply = remote_get_noisy_reply ();
  if (*reply == '\0')
    return -1;
  else
    {
      ULONGEST min_insn_len;

      unpack_varlen_hex (reply, &min_insn_len);

      return (int) min_insn_len;
    }
}

void
remote_target::set_trace_buffer_size (LONGEST val)
{
  if (m_features.packet_support (PACKET_QTBuffer_size) != PACKET_DISABLE)
    {
      struct remote_state *rs = get_remote_state ();
      char *buf = rs->buf.data ();
      char *endbuf = buf + get_remote_packet_size ();
      enum packet_result result;

      gdb_assert (val >= 0 || val == -1);
      buf += xsnprintf (buf, endbuf - buf, "QTBuffer:size:");
      /* Send -1 as literal "-1" to avoid host size dependency.  */
      if (val < 0)
	{
	  *buf++ = '-';
	  buf += hexnumstr (buf, (ULONGEST) -val);
	}
      else
	buf += hexnumstr (buf, (ULONGEST) val);

      putpkt (rs->buf);
      remote_get_noisy_reply ();
      result = m_features.packet_ok (rs->buf, PACKET_QTBuffer_size);

      if (result != PACKET_OK)
	warning (_("Bogus reply from target: %s"), rs->buf.data ());
    }
}

bool
remote_target::set_trace_notes (const char *user, const char *notes,
				const char *stop_notes)
{
  struct remote_state *rs = get_remote_state ();
  char *reply;
  char *buf = rs->buf.data ();
  char *endbuf = buf + get_remote_packet_size ();
  int nbytes;

  buf += xsnprintf (buf, endbuf - buf, "QTNotes:");
  if (user)
    {
      buf += xsnprintf (buf, endbuf - buf, "user:");
      nbytes = bin2hex ((gdb_byte *) user, buf, strlen (user));
      buf += 2 * nbytes;
      *buf++ = ';';
    }
  if (notes)
    {
      buf += xsnprintf (buf, endbuf - buf, "notes:");
      nbytes = bin2hex ((gdb_byte *) notes, buf, strlen (notes));
      buf += 2 * nbytes;
      *buf++ = ';';
    }
  if (stop_notes)
    {
      buf += xsnprintf (buf, endbuf - buf, "tstop:");
      nbytes = bin2hex ((gdb_byte *) stop_notes, buf, strlen (stop_notes));
      buf += 2 * nbytes;
      *buf++ = ';';
    }
  /* Ensure the buffer is terminated.  */
  *buf = '\0';

  putpkt (rs->buf);
  reply = remote_get_noisy_reply ();
  if (*reply == '\0')
    return false;

  if (strcmp (reply, "OK") != 0)
    error (_("Bogus reply from target: %s"), reply);

  return true;
}

bool
remote_target::use_agent (bool use)
{
  if (m_features.packet_support (PACKET_QAgent) != PACKET_DISABLE)
    {
      struct remote_state *rs = get_remote_state ();

      /* If the stub supports QAgent.  */
      xsnprintf (rs->buf.data (), get_remote_packet_size (), "QAgent:%d", use);
      putpkt (rs->buf);
      getpkt (&rs->buf);

      if (strcmp (rs->buf.data (), "OK") == 0)
	{
	  ::use_agent = use;
	  return true;
	}
    }

  return false;
}

bool
remote_target::can_use_agent ()
{
  return (m_features.packet_support (PACKET_QAgent) != PACKET_DISABLE);
}

#if defined (HAVE_LIBEXPAT)

/* Check the btrace document version.  */

static void
check_xml_btrace_version (struct gdb_xml_parser *parser,
			  const struct gdb_xml_element *element,
			  void *user_data,
			  std::vector<gdb_xml_value> &attributes)
{
  const char *version
    = (const char *) xml_find_attribute (attributes, "version")->value.get ();

  if (strcmp (version, "1.0") != 0)
    gdb_xml_error (parser, _("Unsupported btrace version: \"%s\""), version);
}

/* Parse a btrace "block" xml record.  */

static void
parse_xml_btrace_block (struct gdb_xml_parser *parser,
			const struct gdb_xml_element *element,
			void *user_data,
			std::vector<gdb_xml_value> &attributes)
{
  struct btrace_data *btrace;
  ULONGEST *begin, *end;

  btrace = (struct btrace_data *) user_data;

  switch (btrace->format)
    {
    case BTRACE_FORMAT_BTS:
      break;

    case BTRACE_FORMAT_NONE:
      btrace->format = BTRACE_FORMAT_BTS;
      btrace->variant.bts.blocks = new std::vector<btrace_block>;
      break;

    default:
      gdb_xml_error (parser, _("Btrace format error."));
    }

  begin = (ULONGEST *) xml_find_attribute (attributes, "begin")->value.get ();
  end = (ULONGEST *) xml_find_attribute (attributes, "end")->value.get ();
  btrace->variant.bts.blocks->emplace_back (*begin, *end);
}

/* Parse a "raw" xml record.  */

static void
parse_xml_raw (struct gdb_xml_parser *parser, const char *body_text,
	       gdb_byte **pdata, size_t *psize)
{
  gdb_byte *bin;
  size_t len, size;

  len = strlen (body_text);
  if (len % 2 != 0)
    gdb_xml_error (parser, _("Bad raw data size."));

  size = len / 2;

  gdb::unique_xmalloc_ptr<gdb_byte> data ((gdb_byte *) xmalloc (size));
  bin = data.get ();

  /* We use hex encoding - see gdbsupport/rsp-low.h.  */
  while (len > 0)
    {
      char hi, lo;

      hi = *body_text++;
      lo = *body_text++;

      if (hi == 0 || lo == 0)
	gdb_xml_error (parser, _("Bad hex encoding."));

      *bin++ = fromhex (hi) * 16 + fromhex (lo);
      len -= 2;
    }

  *pdata = data.release ();
  *psize = size;
}

/* Parse a btrace pt-config "cpu" xml record.  */

static void
parse_xml_btrace_pt_config_cpu (struct gdb_xml_parser *parser,
				const struct gdb_xml_element *element,
				void *user_data,
				std::vector<gdb_xml_value> &attributes)
{
  struct btrace_data *btrace;
  const char *vendor;
  ULONGEST *family, *model, *stepping;

  vendor
    = (const char *) xml_find_attribute (attributes, "vendor")->value.get ();
  family
    = (ULONGEST *) xml_find_attribute (attributes, "family")->value.get ();
  model
    = (ULONGEST *) xml_find_attribute (attributes, "model")->value.get ();
  stepping
    = (ULONGEST *) xml_find_attribute (attributes, "stepping")->value.get ();

  btrace = (struct btrace_data *) user_data;

  if (strcmp (vendor, "GenuineIntel") == 0)
    btrace->variant.pt.config.cpu.vendor = CV_INTEL;

  btrace->variant.pt.config.cpu.family = *family;
  btrace->variant.pt.config.cpu.model = *model;
  btrace->variant.pt.config.cpu.stepping = *stepping;
}

/* Parse a btrace pt "raw" xml record.  */

static void
parse_xml_btrace_pt_raw (struct gdb_xml_parser *parser,
			 const struct gdb_xml_element *element,
			 void *user_data, const char *body_text)
{
  struct btrace_data *btrace;

  btrace = (struct btrace_data *) user_data;
  parse_xml_raw (parser, body_text, &btrace->variant.pt.data,
		 &btrace->variant.pt.size);
}

/* Parse a btrace "pt" xml record.  */

static void
parse_xml_btrace_pt (struct gdb_xml_parser *parser,
		     const struct gdb_xml_element *element,
		     void *user_data,
		     std::vector<gdb_xml_value> &attributes)
{
  struct btrace_data *btrace;

  btrace = (struct btrace_data *) user_data;
  btrace->format = BTRACE_FORMAT_PT;
  btrace->variant.pt.config.cpu.vendor = CV_UNKNOWN;
  btrace->variant.pt.data = NULL;
  btrace->variant.pt.size = 0;
}

static const struct gdb_xml_attribute block_attributes[] = {
  { "begin", GDB_XML_AF_NONE, gdb_xml_parse_attr_ulongest, NULL },
  { "end", GDB_XML_AF_NONE, gdb_xml_parse_attr_ulongest, NULL },
  { NULL, GDB_XML_AF_NONE, NULL, NULL }
};

static const struct gdb_xml_attribute btrace_pt_config_cpu_attributes[] = {
  { "vendor", GDB_XML_AF_NONE, NULL, NULL },
  { "family", GDB_XML_AF_NONE, gdb_xml_parse_attr_ulongest, NULL },
  { "model", GDB_XML_AF_NONE, gdb_xml_parse_attr_ulongest, NULL },
  { "stepping", GDB_XML_AF_NONE, gdb_xml_parse_attr_ulongest, NULL },
  { NULL, GDB_XML_AF_NONE, NULL, NULL }
};

static const struct gdb_xml_element btrace_pt_config_children[] = {
  { "cpu", btrace_pt_config_cpu_attributes, NULL, GDB_XML_EF_OPTIONAL,
    parse_xml_btrace_pt_config_cpu, NULL },
  { NULL, NULL, NULL, GDB_XML_EF_NONE, NULL, NULL }
};

static const struct gdb_xml_element btrace_pt_children[] = {
  { "pt-config", NULL, btrace_pt_config_children, GDB_XML_EF_OPTIONAL, NULL,
    NULL },
  { "raw", NULL, NULL, GDB_XML_EF_OPTIONAL, NULL, parse_xml_btrace_pt_raw },
  { NULL, NULL, NULL, GDB_XML_EF_NONE, NULL, NULL }
};

static const struct gdb_xml_attribute btrace_attributes[] = {
  { "version", GDB_XML_AF_NONE, NULL, NULL },
  { NULL, GDB_XML_AF_NONE, NULL, NULL }
};

static const struct gdb_xml_element btrace_children[] = {
  { "block", block_attributes, NULL,
    GDB_XML_EF_REPEATABLE | GDB_XML_EF_OPTIONAL, parse_xml_btrace_block, NULL },
  { "pt", NULL, btrace_pt_children, GDB_XML_EF_OPTIONAL, parse_xml_btrace_pt,
    NULL },
  { NULL, NULL, NULL, GDB_XML_EF_NONE, NULL, NULL }
};

static const struct gdb_xml_element btrace_elements[] = {
  { "btrace", btrace_attributes, btrace_children, GDB_XML_EF_NONE,
    check_xml_btrace_version, NULL },
  { NULL, NULL, NULL, GDB_XML_EF_NONE, NULL, NULL }
};

#endif /* defined (HAVE_LIBEXPAT) */

/* Parse a branch trace xml document XML into DATA.  */

static void
parse_xml_btrace (struct btrace_data *btrace, const char *buffer)
{
#if defined (HAVE_LIBEXPAT)

  int errcode;
  btrace_data result;
  result.format = BTRACE_FORMAT_NONE;

  errcode = gdb_xml_parse_quick (_("btrace"), "btrace.dtd", btrace_elements,
				 buffer, &result);
  if (errcode != 0)
    error (_("Error parsing branch trace."));

  /* Keep parse results.  */
  *btrace = std::move (result);

#else  /* !defined (HAVE_LIBEXPAT) */

  error (_("Cannot process branch trace.  XML support was disabled at "
	   "compile time."));

#endif  /* !defined (HAVE_LIBEXPAT) */
}

#if defined (HAVE_LIBEXPAT)

/* Parse a btrace-conf "bts" xml record.  */

static void
parse_xml_btrace_conf_bts (struct gdb_xml_parser *parser,
			  const struct gdb_xml_element *element,
			  void *user_data,
			  std::vector<gdb_xml_value> &attributes)
{
  struct btrace_config *conf;
  struct gdb_xml_value *size;

  conf = (struct btrace_config *) user_data;
  conf->format = BTRACE_FORMAT_BTS;
  conf->bts.size = 0;

  size = xml_find_attribute (attributes, "size");
  if (size != NULL)
    conf->bts.size = (unsigned int) *(ULONGEST *) size->value.get ();
}

/* Parse a btrace-conf "pt" xml record.  */

static void
parse_xml_btrace_conf_pt (struct gdb_xml_parser *parser,
			  const struct gdb_xml_element *element,
			  void *user_data,
			  std::vector<gdb_xml_value> &attributes)
{
  struct btrace_config *conf;
  struct gdb_xml_value *size;

  conf = (struct btrace_config *) user_data;
  conf->format = BTRACE_FORMAT_PT;
  conf->pt.size = 0;

  size = xml_find_attribute (attributes, "size");
  if (size != NULL)
    conf->pt.size = (unsigned int) *(ULONGEST *) size->value.get ();
}

static const struct gdb_xml_attribute btrace_conf_pt_attributes[] = {
  { "size", GDB_XML_AF_OPTIONAL, gdb_xml_parse_attr_ulongest, NULL },
  { NULL, GDB_XML_AF_NONE, NULL, NULL }
};

static const struct gdb_xml_attribute btrace_conf_bts_attributes[] = {
  { "size", GDB_XML_AF_OPTIONAL, gdb_xml_parse_attr_ulongest, NULL },
  { NULL, GDB_XML_AF_NONE, NULL, NULL }
};

static const struct gdb_xml_element btrace_conf_children[] = {
  { "bts", btrace_conf_bts_attributes, NULL, GDB_XML_EF_OPTIONAL,
    parse_xml_btrace_conf_bts, NULL },
  { "pt", btrace_conf_pt_attributes, NULL, GDB_XML_EF_OPTIONAL,
    parse_xml_btrace_conf_pt, NULL },
  { NULL, NULL, NULL, GDB_XML_EF_NONE, NULL, NULL }
};

static const struct gdb_xml_attribute btrace_conf_attributes[] = {
  { "version", GDB_XML_AF_NONE, NULL, NULL },
  { NULL, GDB_XML_AF_NONE, NULL, NULL }
};

static const struct gdb_xml_element btrace_conf_elements[] = {
  { "btrace-conf", btrace_conf_attributes, btrace_conf_children,
    GDB_XML_EF_NONE, NULL, NULL },
  { NULL, NULL, NULL, GDB_XML_EF_NONE, NULL, NULL }
};

#endif /* defined (HAVE_LIBEXPAT) */

/* Parse a branch trace configuration xml document XML into CONF.  */

static void
parse_xml_btrace_conf (struct btrace_config *conf, const char *xml)
{
#if defined (HAVE_LIBEXPAT)

  int errcode;
  errcode = gdb_xml_parse_quick (_("btrace-conf"), "btrace-conf.dtd",
				 btrace_conf_elements, xml, conf);
  if (errcode != 0)
    error (_("Error parsing branch trace configuration."));

#else  /* !defined (HAVE_LIBEXPAT) */

  error (_("Cannot process the branch trace configuration.  XML support "
	   "was disabled at compile time."));

#endif  /* !defined (HAVE_LIBEXPAT) */
}

/* Reset our idea of our target's btrace configuration.  */

static void
remote_btrace_reset (remote_state *rs)
{
  memset (&rs->btrace_config, 0, sizeof (rs->btrace_config));
}

/* Synchronize the configuration with the target.  */

void
remote_target::btrace_sync_conf (const btrace_config *conf)
{
  struct remote_state *rs;
  char *buf, *pos, *endbuf;

  rs = get_remote_state ();
  buf = rs->buf.data ();
  endbuf = buf + get_remote_packet_size ();

  if (m_features.packet_support (PACKET_Qbtrace_conf_bts_size) == PACKET_ENABLE
      && conf->bts.size != rs->btrace_config.bts.size)
    {
      pos = buf;
      pos += xsnprintf (pos, endbuf - pos, "%s=0x%x",
			packets_descriptions[PACKET_Qbtrace_conf_bts_size].name,
			conf->bts.size);

      putpkt (buf);
      getpkt (&rs->buf);

      if (m_features.packet_ok (buf, PACKET_Qbtrace_conf_bts_size)
	  == PACKET_ERROR)
	{
	  if (buf[0] == 'E' && buf[1] == '.')
	    error (_("Failed to configure the BTS buffer size: %s"), buf + 2);
	  else
	    error (_("Failed to configure the BTS buffer size."));
	}

      rs->btrace_config.bts.size = conf->bts.size;
    }

  if (m_features.packet_support (PACKET_Qbtrace_conf_pt_size) == PACKET_ENABLE
      && conf->pt.size != rs->btrace_config.pt.size)
    {
      pos = buf;
      pos += xsnprintf (pos, endbuf - pos, "%s=0x%x",
			packets_descriptions[PACKET_Qbtrace_conf_pt_size].name,
			conf->pt.size);

      putpkt (buf);
      getpkt (&rs->buf);

      if (m_features.packet_ok (buf, PACKET_Qbtrace_conf_pt_size)
	  == PACKET_ERROR)
	{
	  if (buf[0] == 'E' && buf[1] == '.')
	    error (_("Failed to configure the trace buffer size: %s"), buf + 2);
	  else
	    error (_("Failed to configure the trace buffer size."));
	}

      rs->btrace_config.pt.size = conf->pt.size;
    }
}

/* Read TP's btrace configuration from the target and store it into CONF.  */

static void
btrace_read_config (thread_info *tp, btrace_config *conf)
{
  /* target_read_stralloc relies on INFERIOR_PTID.  */
  scoped_restore_current_thread restore_thread;
  switch_to_thread (tp);

  std::optional<gdb::char_vector> xml
    = target_read_stralloc (current_inferior ()->top_target (),
			    TARGET_OBJECT_BTRACE_CONF, "");
  if (xml)
    parse_xml_btrace_conf (conf, xml->data ());
}

/* Maybe reopen target btrace.  */

void
remote_target::remote_btrace_maybe_reopen ()
{
  struct remote_state *rs = get_remote_state ();
  int btrace_target_pushed = 0;
#if !defined (HAVE_LIBIPT)
  int warned = 0;
#endif

  /* Don't bother walking the entirety of the remote thread list when
     we know the feature isn't supported by the remote.  */
  if (m_features.packet_support (PACKET_qXfer_btrace_conf) != PACKET_ENABLE)
    return;

  for (thread_info *tp : all_non_exited_threads (this))
    {
      memset (&rs->btrace_config, 0x00, sizeof (struct btrace_config));
      btrace_read_config (tp, &rs->btrace_config);

      if (rs->btrace_config.format == BTRACE_FORMAT_NONE)
	continue;

#if !defined (HAVE_LIBIPT)
      if (rs->btrace_config.format == BTRACE_FORMAT_PT)
	{
	  if (!warned)
	    {
	      warned = 1;
	      warning (_("Target is recording using Intel Processor Trace "
			 "but support was disabled at compile time."));
	    }

	  continue;
	}
#endif /* !defined (HAVE_LIBIPT) */

      /* Push target, once, but before anything else happens.  This way our
	 changes to the threads will be cleaned up by unpushing the target
	 in case btrace_read_config () throws.  */
      if (!btrace_target_pushed)
	{
	  btrace_target_pushed = 1;
	  record_btrace_push_target ();
	  gdb_printf (_("Target is recording using %s.\n"),
		      btrace_format_string (rs->btrace_config.format));
	}

      tp->btrace.target
	= new btrace_target_info { tp->ptid, rs->btrace_config };
    }
}

/* Enable branch tracing.  */

struct btrace_target_info *
remote_target::enable_btrace (thread_info *tp,
			      const struct btrace_config *conf)
{
  struct packet_config *packet = NULL;
  struct remote_state *rs = get_remote_state ();
  char *buf = rs->buf.data ();
  char *endbuf = buf + get_remote_packet_size ();

  unsigned int which_packet;
  switch (conf->format)
    {
      case BTRACE_FORMAT_BTS:
	which_packet = PACKET_Qbtrace_bts;
	break;
      case BTRACE_FORMAT_PT:
	which_packet = PACKET_Qbtrace_pt;
	break;
      default:
	internal_error (_("Bad branch btrace format: %u."),
			(unsigned int) conf->format);
    }

  packet = &m_features.m_protocol_packets[which_packet];
  if (packet == NULL || packet_config_support (packet) != PACKET_ENABLE)
    error (_("Target does not support branch tracing."));

  btrace_sync_conf (conf);

  ptid_t ptid = tp->ptid;
  set_general_thread (ptid);

  buf += xsnprintf (buf, endbuf - buf, "%s",
		    packets_descriptions[which_packet].name);
  putpkt (rs->buf);
  getpkt (&rs->buf);

  if (m_features.packet_ok (rs->buf, which_packet) == PACKET_ERROR)
    {
      if (rs->buf[0] == 'E' && rs->buf[1] == '.')
	error (_("Could not enable branch tracing for %s: %s"),
	       target_pid_to_str (ptid).c_str (), &rs->buf[2]);
      else
	error (_("Could not enable branch tracing for %s."),
	       target_pid_to_str (ptid).c_str ());
    }

  btrace_target_info *tinfo = new btrace_target_info { ptid };

  /* If we fail to read the configuration, we lose some information, but the
     tracing itself is not impacted.  */
  try
    {
      btrace_read_config (tp, &tinfo->conf);
    }
  catch (const gdb_exception_error &err)
    {
      if (err.message != NULL)
	warning ("%s", err.what ());
    }

  return tinfo;
}

/* Disable branch tracing.  */

void
remote_target::disable_btrace (struct btrace_target_info *tinfo)
{
  struct remote_state *rs = get_remote_state ();
  char *buf = rs->buf.data ();
  char *endbuf = buf + get_remote_packet_size ();

  if (m_features.packet_support (PACKET_Qbtrace_off) != PACKET_ENABLE)
    error (_("Target does not support branch tracing."));

  set_general_thread (tinfo->ptid);

  buf += xsnprintf (buf, endbuf - buf, "%s",
		    packets_descriptions[PACKET_Qbtrace_off].name);
  putpkt (rs->buf);
  getpkt (&rs->buf);

  if (m_features.packet_ok (rs->buf, PACKET_Qbtrace_off) == PACKET_ERROR)
    {
      if (rs->buf[0] == 'E' && rs->buf[1] == '.')
	error (_("Could not disable branch tracing for %s: %s"),
	       target_pid_to_str (tinfo->ptid).c_str (), &rs->buf[2]);
      else
	error (_("Could not disable branch tracing for %s."),
	       target_pid_to_str (tinfo->ptid).c_str ());
    }

  delete tinfo;
}

/* Teardown branch tracing.  */

void
remote_target::teardown_btrace (struct btrace_target_info *tinfo)
{
  /* We must not talk to the target during teardown.  */
  delete tinfo;
}

/* Read the branch trace.  */

enum btrace_error
remote_target::read_btrace (struct btrace_data *btrace,
			    struct btrace_target_info *tinfo,
			    enum btrace_read_type type)
{
  const char *annex;

  if (m_features.packet_support (PACKET_qXfer_btrace) != PACKET_ENABLE)
    error (_("Target does not support branch tracing."));

#if !defined(HAVE_LIBEXPAT)
  error (_("Cannot process branch tracing result. XML parsing not supported."));
#endif

  switch (type)
    {
    case BTRACE_READ_ALL:
      annex = "all";
      break;
    case BTRACE_READ_NEW:
      annex = "new";
      break;
    case BTRACE_READ_DELTA:
      annex = "delta";
      break;
    default:
      internal_error (_("Bad branch tracing read type: %u."),
		      (unsigned int) type);
    }

  std::optional<gdb::char_vector> xml
    = target_read_stralloc (current_inferior ()->top_target (),
			    TARGET_OBJECT_BTRACE, annex);
  if (!xml)
    return BTRACE_ERR_UNKNOWN;

  parse_xml_btrace (btrace, xml->data ());

  return BTRACE_ERR_NONE;
}

const struct btrace_config *
remote_target::btrace_conf (const struct btrace_target_info *tinfo)
{
  return &tinfo->conf;
}

bool
remote_target::augmented_libraries_svr4_read ()
{
  return
    (m_features.packet_support (PACKET_augmented_libraries_svr4_read_feature)
     == PACKET_ENABLE);
}

/* Implementation of to_load.  */

void
remote_target::load (const char *name, int from_tty)
{
  generic_load (name, from_tty);
}

/* Accepts an integer PID; returns a string representing a file that
   can be opened on the remote side to get the symbols for the child
   process.  Returns NULL if the operation is not supported.  */

const char *
remote_target::pid_to_exec_file (int pid)
{
  static std::optional<gdb::char_vector> filename;
  char *annex = NULL;

  if (m_features.packet_support (PACKET_qXfer_exec_file) != PACKET_ENABLE)
    return NULL;

  inferior *inf = find_inferior_pid (this, pid);
  if (inf == NULL)
    internal_error (_("not currently attached to process %d"), pid);

  if (!inf->fake_pid_p)
    {
      const int annex_size = 9;

      annex = (char *) alloca (annex_size);
      xsnprintf (annex, annex_size, "%x", pid);
    }

  filename = target_read_stralloc (current_inferior ()->top_target (),
				   TARGET_OBJECT_EXEC_FILE, annex);

  return filename ? filename->data () : nullptr;
}

/* Implement the to_can_do_single_step target_ops method.  */

int
remote_target::can_do_single_step ()
{
  /* We can only tell whether target supports single step or not by
     supported s and S vCont actions if the stub supports vContSupported
     feature.  If the stub doesn't support vContSupported feature,
     we have conservatively to think target doesn't supports single
     step.  */
  if (m_features.packet_support (PACKET_vContSupported) == PACKET_ENABLE)
    {
      struct remote_state *rs = get_remote_state ();

      return rs->supports_vCont.s && rs->supports_vCont.S;
    }
  else
    return 0;
}

/* Implementation of the to_execution_direction method for the remote
   target.  */

enum exec_direction_kind
remote_target::execution_direction ()
{
  struct remote_state *rs = get_remote_state ();

  return rs->last_resume_exec_dir;
}

/* Return pointer to the thread_info struct which corresponds to
   THREAD_HANDLE (having length HANDLE_LEN).  */

thread_info *
remote_target::thread_handle_to_thread_info (const gdb_byte *thread_handle,
					     int handle_len,
					     inferior *inf)
{
  for (thread_info *tp : all_non_exited_threads (this))
    {
      remote_thread_info *priv = get_remote_thread_info (tp);

      if (tp->inf == inf && priv != NULL)
	{
	  if (handle_len != priv->thread_handle.size ())
	    error (_("Thread handle size mismatch: %d vs %zu (from remote)"),
		   handle_len, priv->thread_handle.size ());
	  if (memcmp (thread_handle, priv->thread_handle.data (),
		      handle_len) == 0)
	    return tp;
	}
    }

  return NULL;
}

gdb::array_view<const gdb_byte>
remote_target::thread_info_to_thread_handle (struct thread_info *tp)
{
  remote_thread_info *priv = get_remote_thread_info (tp);
  return priv->thread_handle;
}

bool
remote_target::can_async_p ()
{
  /* This flag should be checked in the common target.c code.  */
  gdb_assert (target_async_permitted);

  /* We're async whenever the serial device can.  */
  return get_remote_state ()->can_async_p ();
}

bool
remote_target::is_async_p ()
{
  /* We're async whenever the serial device is.  */
  return get_remote_state ()->is_async_p ();
}

/* Pass the SERIAL event on and up to the client.  One day this code
   will be able to delay notifying the client of an event until the
   point where an entire packet has been received.  */

static serial_event_ftype remote_async_serial_handler;

static void
remote_async_serial_handler (struct serial *scb, void *context)
{
  /* Don't propogate error information up to the client.  Instead let
     the client find out about the error by querying the target.  */
  inferior_event_handler (INF_REG_EVENT);
}

int
remote_target::async_wait_fd ()
{
  struct remote_state *rs = get_remote_state ();
  return rs->remote_desc->fd;
}

void
remote_target::async (bool enable)
{
  struct remote_state *rs = get_remote_state ();

  if (enable)
    {
      serial_async (rs->remote_desc, remote_async_serial_handler, rs);

      /* If there are pending events in the stop reply queue tell the
	 event loop to process them.  */
      if (!rs->stop_reply_queue.empty ())
	rs->mark_async_event_handler ();

      /* For simplicity, below we clear the pending events token
	 without remembering whether it is marked, so here we always
	 mark it.  If there's actually no pending notification to
	 process, this ends up being a no-op (other than a spurious
	 event-loop wakeup).  */
      if (target_is_non_stop_p ())
	mark_async_event_handler (rs->notif_state->get_pending_events_token);
    }
  else
    {
      serial_async (rs->remote_desc, NULL, NULL);
      /* If the core is disabling async, it doesn't want to be
	 disturbed with target events.  Clear all async event sources
	 too.  */
      rs->clear_async_event_handler ();

      if (target_is_non_stop_p ())
	clear_async_event_handler (rs->notif_state->get_pending_events_token);
    }
}

/* Implementation of the to_thread_events method.  */

void
remote_target::thread_events (int enable)
{
  struct remote_state *rs = get_remote_state ();
  size_t size = get_remote_packet_size ();

  if (m_features.packet_support (PACKET_QThreadEvents) == PACKET_DISABLE)
    return;

  if (rs->last_thread_events == enable)
    return;

  xsnprintf (rs->buf.data (), size, "QThreadEvents:%x", enable ? 1 : 0);
  putpkt (rs->buf);
  getpkt (&rs->buf);

  switch (m_features.packet_ok (rs->buf, PACKET_QThreadEvents))
    {
    case PACKET_OK:
      if (strcmp (rs->buf.data (), "OK") != 0)
	error (_("Remote refused setting thread events: %s"), rs->buf.data ());
      rs->last_thread_events = enable;
      break;
    case PACKET_ERROR:
      warning (_("Remote failure reply: %s"), rs->buf.data ());
      break;
    case PACKET_UNKNOWN:
      break;
    }
}

/* Implementation of the supports_set_thread_options target
   method.  */

bool
remote_target::supports_set_thread_options (gdb_thread_options options)
{
  remote_state *rs = get_remote_state ();
  return (m_features.packet_support (PACKET_QThreadOptions) == PACKET_ENABLE
	  && (rs->supported_thread_options & options) == options);
}

/* For coalescing reasons, actually sending the options to the target
   happens at resume time, via this function.  See target_resume for
   all-stop, and target_commit_resumed for non-stop.  */

void
remote_target::commit_requested_thread_options ()
{
  struct remote_state *rs = get_remote_state ();

  if (m_features.packet_support (PACKET_QThreadOptions) != PACKET_ENABLE)
    return;

  char *p = rs->buf.data ();
  char *endp = p + get_remote_packet_size ();

  /* Clear options for all threads by default.  Note that unlike
     vCont, the rightmost options that match a thread apply, so we
     don't have to worry about whether we can use wildcard ptids.  */
  strcpy (p, "QThreadOptions;0");
  p += strlen (p);

  /* Send the QThreadOptions packet stored in P.  */
  auto flush = [&] ()
    {
      *p++ = '\0';

      putpkt (rs->buf);
      getpkt (&rs->buf, 0);

      switch (m_features.packet_ok (rs->buf, PACKET_QThreadOptions))
	{
	case PACKET_OK:
	  if (strcmp (rs->buf.data (), "OK") != 0)
	    error (_("Remote refused setting thread options: %s"), rs->buf.data ());
	  break;
	case PACKET_ERROR:
	  error (_("Remote failure reply: %s"), rs->buf.data ());
	case PACKET_UNKNOWN:
	  gdb_assert_not_reached ("PACKET_UNKNOWN");
	  break;
	}
    };

  /* Prepare P for another QThreadOptions packet.  */
  auto restart = [&] ()
    {
      p = rs->buf.data ();
      strcpy (p, "QThreadOptions");
      p += strlen (p);
    };

  /* Now set non-zero options for threads that need them.  We don't
     bother with the case of all threads of a process wanting the same
     non-zero options as that's not an expected scenario.  */
  for (thread_info *tp : all_non_exited_threads (this))
    {
      gdb_thread_options options = tp->thread_options ();

      if (options == 0)
	continue;

      /* It might be possible to we have more threads with options
	 than can fit a single QThreadOptions packet.  So build each
	 options/thread pair in this separate buffer to make sure it
	 fits.  */
      constexpr size_t max_options_size = 100;
      char obuf[max_options_size];
      char *obuf_p = obuf;
      char *obuf_endp = obuf + max_options_size;

      *obuf_p++ = ';';
      obuf_p += xsnprintf (obuf_p, obuf_endp - obuf_p, "%s",
			   phex_nz (options, sizeof (options)));
      if (tp->ptid != magic_null_ptid)
	{
	  *obuf_p++ = ':';
	  obuf_p = write_ptid (obuf_p, obuf_endp, tp->ptid);
	}

      size_t osize = obuf_p - obuf;
      if (osize > endp - p)
	{
	  /* This new options/thread pair doesn't fit the packet
	     buffer.  Send what we have already.  */
	  flush ();
	  restart ();

	  /* Should now fit.  */
	  gdb_assert (osize <= endp - p);
	}

      memcpy (p, obuf, osize);
      p += osize;
    }

  flush ();
}

static void
show_remote_cmd (const char *args, int from_tty)
{
  /* We can't just use cmd_show_list here, because we want to skip
     the redundant "show remote Z-packet" and the legacy aliases.  */
  struct cmd_list_element *list = remote_show_cmdlist;
  struct ui_out *uiout = current_uiout;

  ui_out_emit_tuple tuple_emitter (uiout, "showlist");
  for (; list != NULL; list = list->next)
    if (strcmp (list->name, "Z-packet") == 0)
      continue;
    else if (list->type == not_set_cmd)
      /* Alias commands are exactly like the original, except they
	 don't have the normal type.  */
      continue;
    else
      {
	ui_out_emit_tuple option_emitter (uiout, "option");

	uiout->field_string ("name", list->name);
	uiout->text (":  ");
	if (list->type == show_cmd)
	  do_show_command (NULL, from_tty, list);
	else
	  cmd_func (list, NULL, from_tty);
      }
}

/* Some change happened in PSPACE's objfile list (obfiles added or removed),
   offer all inferiors using that program space a change to look up symbols.  */

static void
remote_objfile_changed_check_symbols (program_space *pspace)
{
  /* The affected program space is possibly shared by multiple inferiors.
     Consider sending a qSymbol packet for each of the inferiors using that
     program space.  */
  for (inferior *inf : all_inferiors ())
    {
      if (inf->pspace != pspace)
	continue;

      /* Check whether the inferior's process target is a remote target.  */
      remote_target *remote = as_remote_target (inf->process_target ());
      if (remote == nullptr)
	continue;

      /* When we are attaching or handling a fork child and the shared library
	 subsystem reads the list of loaded libraries, we receive new objfile
	 events in between each found library.  The libraries are read in an
	 undefined order, so if we gave the remote side a chance to look up
	 symbols between each objfile, we might give it an inconsistent picture
	 of the inferior.  It could appear that a library A appears loaded but
	 a library B does not, even though library A requires library B.  That
	 would present a state that couldn't normally exist in the inferior.

	 So, skip these events, we'll give the remote a chance to look up
	 symbols once all the loaded libraries and their symbols are known to
	 GDB.  */
      if (inf->in_initial_library_scan)
	continue;

      if (!remote->has_execution (inf))
	continue;

      /* Need to switch to a specific thread, because remote_check_symbols will
	 set the general thread using INFERIOR_PTID.

	 It's possible to have inferiors with no thread here, because we are
	 called very early in the connection process, while the inferior is
	 being set up, before threads are added.  Just skip it, start_remote_1
	 also calls remote_check_symbols when it's done setting things up.  */
      thread_info *thread = any_thread_of_inferior (inf);
      if (thread != nullptr)
	{
	  scoped_restore_current_thread restore_thread;
	  switch_to_thread (thread);
	  remote->remote_check_symbols ();
	}
  }
}

/* Function to be called whenever a new objfile (shlib) is detected.  */

static void
remote_new_objfile (struct objfile *objfile)
{
  remote_objfile_changed_check_symbols (objfile->pspace);
}

/* Pull all the tracepoints defined on the target and create local
   data structures representing them.  We don't want to create real
   tracepoints yet, we don't want to mess up the user's existing
   collection.  */

int
remote_target::upload_tracepoints (struct uploaded_tp **utpp)
{
  struct remote_state *rs = get_remote_state ();
  char *p;

  /* Ask for a first packet of tracepoint definition.  */
  putpkt ("qTfP");
  getpkt (&rs->buf);
  p = rs->buf.data ();
  while (*p && *p != 'l')
    {
      parse_tracepoint_definition (p, utpp);
      /* Ask for another packet of tracepoint definition.  */
      putpkt ("qTsP");
      getpkt (&rs->buf);
      p = rs->buf.data ();
    }
  return 0;
}

int
remote_target::upload_trace_state_variables (struct uploaded_tsv **utsvp)
{
  struct remote_state *rs = get_remote_state ();
  char *p;

  /* Ask for a first packet of variable definition.  */
  putpkt ("qTfV");
  getpkt (&rs->buf);
  p = rs->buf.data ();
  while (*p && *p != 'l')
    {
      parse_tsv_definition (p, utsvp);
      /* Ask for another packet of variable definition.  */
      putpkt ("qTsV");
      getpkt (&rs->buf);
      p = rs->buf.data ();
    }
  return 0;
}

/* The "set/show range-stepping" show hook.  */

static void
show_range_stepping (struct ui_file *file, int from_tty,
		     struct cmd_list_element *c,
		     const char *value)
{
  gdb_printf (file,
	      _("Debugger's willingness to use range stepping "
		"is %s.\n"), value);
}

/* Return true if the vCont;r action is supported by the remote
   stub.  */

bool
remote_target::vcont_r_supported ()
{
  return (m_features.packet_support (PACKET_vCont) == PACKET_ENABLE
	  && get_remote_state ()->supports_vCont.r);
}

/* The "set/show range-stepping" set hook.  */

static void
set_range_stepping (const char *ignore_args, int from_tty,
		    struct cmd_list_element *c)
{
  /* When enabling, check whether range stepping is actually supported
     by the target, and warn if not.  */
  if (use_range_stepping)
    {
      remote_target *remote = get_current_remote_target ();
      if (remote == NULL
	  || !remote->vcont_r_supported ())
	warning (_("Range stepping is not supported by the current target"));
    }
}

static void
show_remote_debug (struct ui_file *file, int from_tty,
		   struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Debugging of remote protocol is %s.\n"),
	      value);
}

static void
show_remote_timeout (struct ui_file *file, int from_tty,
		     struct cmd_list_element *c, const char *value)
{
  gdb_printf (file,
	      _("Timeout limit to wait for target to respond is %s.\n"),
	      value);
}

/* Implement the "supports_memory_tagging" target_ops method.  */

bool
remote_target::supports_memory_tagging ()
{
  return m_features.remote_memory_tagging_p ();
}

/* Create the qMemTags packet given ADDRESS, LEN and TYPE.  */

static void
create_fetch_memtags_request (gdb::char_vector &packet, CORE_ADDR address,
			      size_t len, int type)
{
  int addr_size = gdbarch_addr_bit (current_inferior ()->arch ()) / 8;

  std::string request = string_printf ("qMemTags:%s,%s:%s",
				       phex_nz (address, addr_size),
				       phex_nz (len, sizeof (len)),
				       phex_nz (type, sizeof (type)));

  strcpy (packet.data (), request.c_str ());
}

/* Parse the qMemTags packet reply into TAGS.

   Return true if successful, false otherwise.  */

static bool
parse_fetch_memtags_reply (const gdb::char_vector &reply,
			   gdb::byte_vector &tags)
{
  if (reply.empty () || reply[0] == 'E' || reply[0] != 'm')
    return false;

  /* Copy the tag data.  */
  tags = hex2bin (reply.data () + 1);

  return true;
}

/* Create the QMemTags packet given ADDRESS, LEN, TYPE and TAGS.  */

static void
create_store_memtags_request (gdb::char_vector &packet, CORE_ADDR address,
			      size_t len, int type,
			      const gdb::byte_vector &tags)
{
  int addr_size = gdbarch_addr_bit (current_inferior ()->arch ()) / 8;

  /* Put together the main packet, address and length.  */
  std::string request = string_printf ("QMemTags:%s,%s:%s:",
				       phex_nz (address, addr_size),
				       phex_nz (len, sizeof (len)),
				       phex_nz (type, sizeof (type)));
  request += bin2hex (tags.data (), tags.size ());

  /* Check if we have exceeded the maximum packet size.  */
  if (packet.size () < request.length ())
    error (_("Contents too big for packet QMemTags."));

  strcpy (packet.data (), request.c_str ());
}

/* Implement the "fetch_memtags" target_ops method.  */

bool
remote_target::fetch_memtags (CORE_ADDR address, size_t len,
			      gdb::byte_vector &tags, int type)
{
  /* Make sure the qMemTags packet is supported.  */
  if (!m_features.remote_memory_tagging_p ())
    gdb_assert_not_reached ("remote fetch_memtags called with packet disabled");

  struct remote_state *rs = get_remote_state ();

  create_fetch_memtags_request (rs->buf, address, len, type);

  putpkt (rs->buf);
  getpkt (&rs->buf);

  return parse_fetch_memtags_reply (rs->buf, tags);
}

/* Implement the "store_memtags" target_ops method.  */

bool
remote_target::store_memtags (CORE_ADDR address, size_t len,
			      const gdb::byte_vector &tags, int type)
{
  /* Make sure the QMemTags packet is supported.  */
  if (!m_features.remote_memory_tagging_p ())
    gdb_assert_not_reached ("remote store_memtags called with packet disabled");

  struct remote_state *rs = get_remote_state ();

  create_store_memtags_request (rs->buf, address, len, type, tags);

  putpkt (rs->buf);
  getpkt (&rs->buf);

  /* Verify if the request was successful.  */
  return packet_check_result (rs->buf.data ()) == PACKET_OK;
}

/* Return true if remote target T is non-stop.  */

bool
remote_target_is_non_stop_p (remote_target *t)
{
  scoped_restore_current_thread restore_thread;
  switch_to_target_no_thread (t);

  return target_is_non_stop_p ();
}

#if GDB_SELF_TEST

namespace selftests {

static void
test_memory_tagging_functions ()
{
  remote_target remote;

  struct packet_config *config
    = &remote.m_features.m_protocol_packets[PACKET_memory_tagging_feature];

  scoped_restore restore_memtag_support_
    = make_scoped_restore (&config->support);

  /* Test memory tagging packet support.  */
  config->support = PACKET_SUPPORT_UNKNOWN;
  SELF_CHECK (remote.supports_memory_tagging () == false);
  config->support = PACKET_DISABLE;
  SELF_CHECK (remote.supports_memory_tagging () == false);
  config->support = PACKET_ENABLE;
  SELF_CHECK (remote.supports_memory_tagging () == true);

  /* Setup testing.  */
  gdb::char_vector packet;
  gdb::byte_vector tags, bv;
  std::string expected, reply;
  packet.resize (32000);

  /* Test creating a qMemTags request.  */

  expected = "qMemTags:0,0:0";
  create_fetch_memtags_request (packet, 0x0, 0x0, 0);
  SELF_CHECK (strcmp (packet.data (), expected.c_str ()) == 0);

  expected = "qMemTags:deadbeef,10:1";
  create_fetch_memtags_request (packet, 0xdeadbeef, 16, 1);
  SELF_CHECK (strcmp (packet.data (), expected.c_str ()) == 0);

  /* Test parsing a qMemTags reply.  */

  /* Error reply, tags vector unmodified.  */
  reply = "E00";
  strcpy (packet.data (), reply.c_str ());
  tags.resize (0);
  SELF_CHECK (parse_fetch_memtags_reply (packet, tags) == false);
  SELF_CHECK (tags.size () == 0);

  /* Valid reply, tags vector updated.  */
  tags.resize (0);
  bv.resize (0);

  for (int i = 0; i < 5; i++)
    bv.push_back (i);

  reply = "m" + bin2hex (bv.data (), bv.size ());
  strcpy (packet.data (), reply.c_str ());

  SELF_CHECK (parse_fetch_memtags_reply (packet, tags) == true);
  SELF_CHECK (tags.size () == 5);

  for (int i = 0; i < 5; i++)
    SELF_CHECK (tags[i] == i);

  /* Test creating a QMemTags request.  */

  /* Empty tag data.  */
  tags.resize (0);
  expected = "QMemTags:0,0:0:";
  create_store_memtags_request (packet, 0x0, 0x0, 0, tags);
  SELF_CHECK (memcmp (packet.data (), expected.c_str (),
		      expected.length ()) == 0);

  /* Non-empty tag data.  */
  tags.resize (0);
  for (int i = 0; i < 5; i++)
    tags.push_back (i);
  expected = "QMemTags:deadbeef,ff:1:0001020304";
  create_store_memtags_request (packet, 0xdeadbeef, 255, 1, tags);
  SELF_CHECK (memcmp (packet.data (), expected.c_str (),
		      expected.length ()) == 0);
}

} // namespace selftests
#endif /* GDB_SELF_TEST */

void _initialize_remote ();
void
_initialize_remote ()
{
  add_target (remote_target_info, remote_target::open);
  add_target (extended_remote_target_info, extended_remote_target::open);

  /* Hook into new objfile notification.  */
  gdb::observers::new_objfile.attach (remote_new_objfile, "remote");
  gdb::observers::all_objfiles_removed.attach
    (remote_objfile_changed_check_symbols, "remote");

#if 0
  init_remote_threadtests ();
#endif

  /* set/show remote ...  */

  add_basic_prefix_cmd ("remote", class_maintenance, _("\
Remote protocol specific variables.\n\
Configure various remote-protocol specific variables such as\n\
the packets being used."),
			&remote_set_cmdlist,
			0 /* allow-unknown */, &setlist);
  add_prefix_cmd ("remote", class_maintenance, show_remote_cmd, _("\
Remote protocol specific variables.\n\
Configure various remote-protocol specific variables such as\n\
the packets being used."),
		  &remote_show_cmdlist,
		  0 /* allow-unknown */, &showlist);

  add_cmd ("compare-sections", class_obscure, compare_sections_command, _("\
Compare section data on target to the exec file.\n\
Argument is a single section name (default: all loaded sections).\n\
To compare only read-only loaded sections, specify the -r option."),
	   &cmdlist);

  add_cmd ("packet", class_maintenance, cli_packet_command, _("\
Send an arbitrary packet to a remote target.\n\
   maintenance packet TEXT\n\
If GDB is talking to an inferior via the GDB serial protocol, then\n\
this command sends the string TEXT to the inferior, and displays the\n\
response packet.  GDB supplies the initial `$' character, and the\n\
terminating `#' character and checksum."),
	   &maintenancelist);

  set_show_commands remotebreak_cmds
    = add_setshow_boolean_cmd ("remotebreak", no_class, &remote_break, _("\
Set whether to send break if interrupted."), _("\
Show whether to send break if interrupted."), _("\
If set, a break, instead of a cntrl-c, is sent to the remote target."),
			       set_remotebreak, show_remotebreak,
			       &setlist, &showlist);
  deprecate_cmd (remotebreak_cmds.set, "set remote interrupt-sequence");
  deprecate_cmd (remotebreak_cmds.show, "show remote interrupt-sequence");

  add_setshow_enum_cmd ("interrupt-sequence", class_support,
			interrupt_sequence_modes, &interrupt_sequence_mode,
			_("\
Set interrupt sequence to remote target."), _("\
Show interrupt sequence to remote target."), _("\
Valid value is \"Ctrl-C\", \"BREAK\" or \"BREAK-g\". The default is \"Ctrl-C\"."),
			NULL, show_interrupt_sequence,
			&remote_set_cmdlist,
			&remote_show_cmdlist);

  add_setshow_boolean_cmd ("interrupt-on-connect", class_support,
			   &interrupt_on_connect, _("\
Set whether interrupt-sequence is sent to remote target when gdb connects to."), _("\
Show whether interrupt-sequence is sent to remote target when gdb connects to."), _("\
If set, interrupt sequence is sent to remote target."),
			   NULL, NULL,
			   &remote_set_cmdlist, &remote_show_cmdlist);

  /* Install commands for configuring memory read/write packets.  */

  add_cmd ("remotewritesize", no_class, set_memory_write_packet_size, _("\
Set the maximum number of bytes per memory write packet (deprecated)."),
	   &setlist);
  add_cmd ("remotewritesize", no_class, show_memory_write_packet_size, _("\
Show the maximum number of bytes per memory write packet (deprecated)."),
	   &showlist);
  add_cmd ("memory-write-packet-size", no_class,
	   set_memory_write_packet_size, _("\
Set the maximum number of bytes per memory-write packet.\n\
Specify the number of bytes in a packet or 0 (zero) for the\n\
default packet size.  The actual limit is further reduced\n\
dependent on the target.  Specify \"fixed\" to disable the\n\
further restriction and \"limit\" to enable that restriction."),
	   &remote_set_cmdlist);
  add_cmd ("memory-read-packet-size", no_class,
	   set_memory_read_packet_size, _("\
Set the maximum number of bytes per memory-read packet.\n\
Specify the number of bytes in a packet or 0 (zero) for the\n\
default packet size.  The actual limit is further reduced\n\
dependent on the target.  Specify \"fixed\" to disable the\n\
further restriction and \"limit\" to enable that restriction."),
	   &remote_set_cmdlist);
  add_cmd ("memory-write-packet-size", no_class,
	   show_memory_write_packet_size,
	   _("Show the maximum number of bytes per memory-write packet."),
	   &remote_show_cmdlist);
  add_cmd ("memory-read-packet-size", no_class,
	   show_memory_read_packet_size,
	   _("Show the maximum number of bytes per memory-read packet."),
	   &remote_show_cmdlist);

  add_setshow_zuinteger_unlimited_cmd ("hardware-watchpoint-limit", no_class,
			    &remote_hw_watchpoint_limit, _("\
Set the maximum number of target hardware watchpoints."), _("\
Show the maximum number of target hardware watchpoints."), _("\
Specify \"unlimited\" for unlimited hardware watchpoints."),
			    NULL, show_hardware_watchpoint_limit,
			    &remote_set_cmdlist,
			    &remote_show_cmdlist);
  add_setshow_zuinteger_unlimited_cmd ("hardware-watchpoint-length-limit",
			    no_class,
			    &remote_hw_watchpoint_length_limit, _("\
Set the maximum length (in bytes) of a target hardware watchpoint."), _("\
Show the maximum length (in bytes) of a target hardware watchpoint."), _("\
Specify \"unlimited\" to allow watchpoints of unlimited size."),
			    NULL, show_hardware_watchpoint_length_limit,
			    &remote_set_cmdlist, &remote_show_cmdlist);
  add_setshow_zuinteger_unlimited_cmd ("hardware-breakpoint-limit", no_class,
			    &remote_hw_breakpoint_limit, _("\
Set the maximum number of target hardware breakpoints."), _("\
Show the maximum number of target hardware breakpoints."), _("\
Specify \"unlimited\" for unlimited hardware breakpoints."),
			    NULL, show_hardware_breakpoint_limit,
			    &remote_set_cmdlist, &remote_show_cmdlist);

  add_setshow_zuinteger_cmd ("remoteaddresssize", class_obscure,
			     &remote_address_size, _("\
Set the maximum size of the address (in bits) in a memory packet."), _("\
Show the maximum size of the address (in bits) in a memory packet."), NULL,
			     NULL,
			     NULL, /* FIXME: i18n: */
			     &setlist, &showlist);

  init_all_packet_configs ();

  add_packet_config_cmd (PACKET_X, "X", "binary-download", 1);

  add_packet_config_cmd (PACKET_vCont, "vCont", "verbose-resume", 0);

  add_packet_config_cmd (PACKET_QPassSignals, "QPassSignals", "pass-signals",
			 0);

  add_packet_config_cmd (PACKET_QCatchSyscalls, "QCatchSyscalls",
			 "catch-syscalls", 0);

  add_packet_config_cmd (PACKET_QProgramSignals, "QProgramSignals",
			 "program-signals", 0);

  add_packet_config_cmd (PACKET_QSetWorkingDir, "QSetWorkingDir",
			 "set-working-dir", 0);

  add_packet_config_cmd (PACKET_QStartupWithShell, "QStartupWithShell",
			 "startup-with-shell", 0);

  add_packet_config_cmd (PACKET_QEnvironmentHexEncoded,"QEnvironmentHexEncoded",
			 "environment-hex-encoded", 0);

  add_packet_config_cmd (PACKET_QEnvironmentReset, "QEnvironmentReset",
			 "environment-reset", 0);

  add_packet_config_cmd (PACKET_QEnvironmentUnset, "QEnvironmentUnset",
			 "environment-unset", 0);

  add_packet_config_cmd (PACKET_qSymbol, "qSymbol", "symbol-lookup", 0);

  add_packet_config_cmd (PACKET_P, "P", "set-register", 1);

  add_packet_config_cmd (PACKET_p, "p", "fetch-register", 1);

  add_packet_config_cmd (PACKET_Z0, "Z0", "software-breakpoint", 0);

  add_packet_config_cmd (PACKET_Z1, "Z1", "hardware-breakpoint", 0);

  add_packet_config_cmd (PACKET_Z2, "Z2", "write-watchpoint", 0);

  add_packet_config_cmd (PACKET_Z3, "Z3", "read-watchpoint", 0);

  add_packet_config_cmd (PACKET_Z4, "Z4", "access-watchpoint", 0);

  add_packet_config_cmd (PACKET_qXfer_auxv, "qXfer:auxv:read",
			 "read-aux-vector", 0);

  add_packet_config_cmd (PACKET_qXfer_exec_file, "qXfer:exec-file:read",
			 "pid-to-exec-file", 0);

  add_packet_config_cmd (PACKET_qXfer_features,
			 "qXfer:features:read", "target-features", 0);

  add_packet_config_cmd (PACKET_qXfer_libraries, "qXfer:libraries:read",
			 "library-info", 0);

  add_packet_config_cmd (PACKET_qXfer_libraries_svr4,
			 "qXfer:libraries-svr4:read", "library-info-svr4", 0);

  add_packet_config_cmd (PACKET_qXfer_memory_map, "qXfer:memory-map:read",
			 "memory-map", 0);

  add_packet_config_cmd (PACKET_qXfer_osdata, "qXfer:osdata:read", "osdata", 0);

  add_packet_config_cmd (PACKET_qXfer_threads, "qXfer:threads:read", "threads",
			 0);

  add_packet_config_cmd (PACKET_qXfer_siginfo_read, "qXfer:siginfo:read",
			 "read-siginfo-object", 0);

  add_packet_config_cmd (PACKET_qXfer_siginfo_write, "qXfer:siginfo:write",
			 "write-siginfo-object", 0);

  add_packet_config_cmd (PACKET_qXfer_traceframe_info,
			 "qXfer:traceframe-info:read", "traceframe-info", 0);

  add_packet_config_cmd (PACKET_qXfer_uib, "qXfer:uib:read",
			 "unwind-info-block", 0);

  add_packet_config_cmd (PACKET_qGetTLSAddr, "qGetTLSAddr",
			 "get-thread-local-storage-address", 0);

  add_packet_config_cmd (PACKET_qGetTIBAddr, "qGetTIBAddr",
			 "get-thread-information-block-address", 0);

  add_packet_config_cmd (PACKET_bc, "bc", "reverse-continue", 0);

  add_packet_config_cmd (PACKET_bs, "bs", "reverse-step", 0);

  add_packet_config_cmd (PACKET_qSupported, "qSupported", "supported-packets",
			 0);

  add_packet_config_cmd (PACKET_qSearch_memory, "qSearch:memory",
			 "search-memory", 0);

  add_packet_config_cmd (PACKET_qTStatus, "qTStatus", "trace-status", 0);

  add_packet_config_cmd (PACKET_vFile_setfs, "vFile:setfs", "hostio-setfs", 0);

  add_packet_config_cmd (PACKET_vFile_open, "vFile:open", "hostio-open", 0);

  add_packet_config_cmd (PACKET_vFile_pread, "vFile:pread", "hostio-pread", 0);

  add_packet_config_cmd (PACKET_vFile_pwrite, "vFile:pwrite", "hostio-pwrite",
			 0);

  add_packet_config_cmd (PACKET_vFile_close, "vFile:close", "hostio-close", 0);

  add_packet_config_cmd (PACKET_vFile_unlink, "vFile:unlink", "hostio-unlink",
			 0);

  add_packet_config_cmd (PACKET_vFile_readlink, "vFile:readlink",
			 "hostio-readlink", 0);

  add_packet_config_cmd (PACKET_vFile_fstat, "vFile:fstat", "hostio-fstat", 0);

  add_packet_config_cmd (PACKET_vAttach, "vAttach", "attach", 0);

  add_packet_config_cmd (PACKET_vRun, "vRun", "run", 0);

  add_packet_config_cmd (PACKET_QStartNoAckMode, "QStartNoAckMode", "noack", 0);

  add_packet_config_cmd (PACKET_vKill, "vKill", "kill", 0);

  add_packet_config_cmd (PACKET_qAttached, "qAttached", "query-attached", 0);

  add_packet_config_cmd (PACKET_ConditionalTracepoints,
			 "ConditionalTracepoints", "conditional-tracepoints",
			 0);

  add_packet_config_cmd (PACKET_ConditionalBreakpoints,
			 "ConditionalBreakpoints", "conditional-breakpoints",
			 0);

  add_packet_config_cmd (PACKET_BreakpointCommands, "BreakpointCommands",
			 "breakpoint-commands", 0);

  add_packet_config_cmd (PACKET_FastTracepoints, "FastTracepoints",
			 "fast-tracepoints", 0);

  add_packet_config_cmd (PACKET_TracepointSource, "TracepointSource",
			 "TracepointSource", 0);

  add_packet_config_cmd (PACKET_QAllow, "QAllow", "allow", 0);

  add_packet_config_cmd (PACKET_StaticTracepoints, "StaticTracepoints",
			 "static-tracepoints", 0);

  add_packet_config_cmd (PACKET_InstallInTrace, "InstallInTrace",
			 "install-in-trace", 0);

  add_packet_config_cmd (PACKET_qXfer_statictrace_read,
			 "qXfer:statictrace:read", "read-sdata-object", 0);

  add_packet_config_cmd (PACKET_qXfer_fdpic, "qXfer:fdpic:read",
			 "read-fdpic-loadmap", 0);

  add_packet_config_cmd (PACKET_QDisableRandomization, "QDisableRandomization",
			 "disable-randomization", 0);

  add_packet_config_cmd (PACKET_QAgent, "QAgent", "agent", 0);

  add_packet_config_cmd (PACKET_QTBuffer_size, "QTBuffer:size",
			 "trace-buffer-size", 0);

  add_packet_config_cmd (PACKET_Qbtrace_off, "Qbtrace:off", "disable-btrace",
			 0);

  add_packet_config_cmd (PACKET_Qbtrace_bts, "Qbtrace:bts", "enable-btrace-bts",
			 0);

  add_packet_config_cmd (PACKET_Qbtrace_pt, "Qbtrace:pt", "enable-btrace-pt",
			 0);

  add_packet_config_cmd (PACKET_qXfer_btrace, "qXfer:btrace", "read-btrace", 0);

  add_packet_config_cmd (PACKET_qXfer_btrace_conf, "qXfer:btrace-conf",
			 "read-btrace-conf", 0);

  add_packet_config_cmd (PACKET_Qbtrace_conf_bts_size, "Qbtrace-conf:bts:size",
			 "btrace-conf-bts-size", 0);

  add_packet_config_cmd (PACKET_multiprocess_feature, "multiprocess-feature",
			 "multiprocess-feature", 0);

  add_packet_config_cmd (PACKET_swbreak_feature, "swbreak-feature",
			 "swbreak-feature", 0);

  add_packet_config_cmd (PACKET_hwbreak_feature, "hwbreak-feature",
			 "hwbreak-feature", 0);

  add_packet_config_cmd (PACKET_fork_event_feature, "fork-event-feature",
			 "fork-event-feature", 0);

  add_packet_config_cmd (PACKET_vfork_event_feature, "vfork-event-feature",
			 "vfork-event-feature", 0);

  add_packet_config_cmd (PACKET_Qbtrace_conf_pt_size, "Qbtrace-conf:pt:size",
			 "btrace-conf-pt-size", 0);

  add_packet_config_cmd (PACKET_vContSupported, "vContSupported",
			 "verbose-resume-supported", 0);

  add_packet_config_cmd (PACKET_exec_event_feature, "exec-event-feature",
			 "exec-event-feature", 0);

  add_packet_config_cmd (PACKET_vCtrlC, "vCtrlC", "ctrl-c", 0);

  add_packet_config_cmd (PACKET_QThreadEvents, "QThreadEvents", "thread-events",
			 0);

  add_packet_config_cmd (PACKET_QThreadOptions, "QThreadOptions",
			 "thread-options", 0);

  add_packet_config_cmd (PACKET_no_resumed, "N stop reply",
			 "no-resumed-stop-reply", 0);

  add_packet_config_cmd (PACKET_memory_tagging_feature,
			 "memory-tagging-feature", "memory-tagging-feature", 0);

  /* Assert that we've registered "set remote foo-packet" commands
     for all packet configs.  */
  {
    int i;

    for (i = 0; i < PACKET_MAX; i++)
      {
	/* Ideally all configs would have a command associated.  Some
	   still don't though.  */
	int excepted;

	switch (i)
	  {
	  case PACKET_QNonStop:
	  case PACKET_EnableDisableTracepoints_feature:
	  case PACKET_tracenz_feature:
	  case PACKET_DisconnectedTracing_feature:
	  case PACKET_augmented_libraries_svr4_read_feature:
	  case PACKET_qCRC:
	    /* Additions to this list need to be well justified:
	       pre-existing packets are OK; new packets are not.  */
	    excepted = 1;
	    break;
	  default:
	    excepted = 0;
	    break;
	  }

	/* This catches both forgetting to add a config command, and
	   forgetting to remove a packet from the exception list.  */
	gdb_assert (excepted == (packets_descriptions[i].name == NULL));
      }
  }

  /* Keep the old ``set remote Z-packet ...'' working.  Each individual
     Z sub-packet has its own set and show commands, but users may
     have sets to this variable in their .gdbinit files (or in their
     documentation).  */
  add_setshow_auto_boolean_cmd ("Z-packet", class_obscure,
				&remote_Z_packet_detect, _("\
Set use of remote protocol `Z' packets."), _("\
Show use of remote protocol `Z' packets."), _("\
When set, GDB will attempt to use the remote breakpoint and watchpoint\n\
packets."),
				set_remote_protocol_Z_packet_cmd,
				show_remote_protocol_Z_packet_cmd,
				/* FIXME: i18n: Use of remote protocol
				   `Z' packets is %s.  */
				&remote_set_cmdlist, &remote_show_cmdlist);

  add_basic_prefix_cmd ("remote", class_files, _("\
Manipulate files on the remote system.\n\
Transfer files to and from the remote target system."),
			&remote_cmdlist,
			0 /* allow-unknown */, &cmdlist);

  add_cmd ("put", class_files, remote_put_command,
	   _("Copy a local file to the remote system."),
	   &remote_cmdlist);

  add_cmd ("get", class_files, remote_get_command,
	   _("Copy a remote file to the local system."),
	   &remote_cmdlist);

  add_cmd ("delete", class_files, remote_delete_command,
	   _("Delete a remote file."),
	   &remote_cmdlist);

  add_setshow_string_noescape_cmd ("exec-file", class_files,
				   &remote_exec_file_var, _("\
Set the remote pathname for \"run\"."), _("\
Show the remote pathname for \"run\"."), NULL,
				   set_remote_exec_file,
				   show_remote_exec_file,
				   &remote_set_cmdlist,
				   &remote_show_cmdlist);

  add_setshow_boolean_cmd ("range-stepping", class_run,
			   &use_range_stepping, _("\
Enable or disable range stepping."), _("\
Show whether target-assisted range stepping is enabled."), _("\
If on, and the target supports it, when stepping a source line, GDB\n\
tells the target to step the corresponding range of addresses itself instead\n\
of issuing multiple single-steps.  This speeds up source level\n\
stepping.  If off, GDB always issues single-steps, even if range\n\
stepping is supported by the target.  The default is on."),
			   set_range_stepping,
			   show_range_stepping,
			   &setlist,
			   &showlist);

  add_setshow_zinteger_cmd ("watchdog", class_maintenance, &watchdog, _("\
Set watchdog timer."), _("\
Show watchdog timer."), _("\
When non-zero, this timeout is used instead of waiting forever for a target\n\
to finish a low-level step or continue operation.  If the specified amount\n\
of time passes without a response from the target, an error occurs."),
			    NULL,
			    show_watchdog,
			    &setlist, &showlist);

  add_setshow_zuinteger_unlimited_cmd ("remote-packet-max-chars", no_class,
				       &remote_packet_max_chars, _("\
Set the maximum number of characters to display for each remote packet."), _("\
Show the maximum number of characters to display for each remote packet."), _("\
Specify \"unlimited\" to display all the characters."),
				       NULL, show_remote_packet_max_chars,
				       &setdebuglist, &showdebuglist);

  add_setshow_boolean_cmd ("remote", no_class, &remote_debug,
			   _("Set debugging of remote protocol."),
			   _("Show debugging of remote protocol."),
			   _("\
When enabled, each packet sent or received with the remote target\n\
is displayed."),
			   NULL,
			   show_remote_debug,
			   &setdebuglist, &showdebuglist);

  add_setshow_zuinteger_unlimited_cmd ("remotetimeout", no_class,
				       &remote_timeout, _("\
Set timeout limit to wait for target to respond."), _("\
Show timeout limit to wait for target to respond."), _("\
This value is used to set the time limit for gdb to wait for a response\n\
from the target."),
				       NULL,
				       show_remote_timeout,
				       &setlist, &showlist);

  /* Eventually initialize fileio.  See fileio.c */
  initialize_remote_fileio (&remote_set_cmdlist, &remote_show_cmdlist);

#if GDB_SELF_TEST
  selftests::register_test ("remote_memory_tagging",
			    selftests::test_memory_tagging_functions);
#endif
}
