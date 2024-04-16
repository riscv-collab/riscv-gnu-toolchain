/* Linux-dependent part of branch trace support for GDB, and GDBserver.

   Copyright (C) 2013-2024 Free Software Foundation, Inc.

   Contributed by Intel Corp. <markus.t.metzger@intel.com>

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

#ifndef NAT_LINUX_BTRACE_H
#define NAT_LINUX_BTRACE_H

#include "gdbsupport/btrace-common.h"
#include "gdbsupport/gdb-checked-static-cast.h"
#if HAVE_LINUX_PERF_EVENT_H
#  include <linux/perf_event.h>
#endif

struct target_ops;

#if HAVE_LINUX_PERF_EVENT_H
/* A Linux perf event buffer.  */
struct perf_event_buffer
{
  /* The mapped memory.  */
  const uint8_t *mem;

  /* The size of the mapped memory in bytes.  */
  size_t size;

  /* A pointer to the data_head field for this buffer. */
  volatile __u64 *data_head;

  /* The data_head value from the last read.  */
  __u64 last_head;
};
#endif /* HAVE_LINUX_PERF_EVENT_H */

/* Branch trace target information per thread.  */
struct linux_btrace_target_info final : public btrace_target_info
{
  linux_btrace_target_info (ptid_t ptid)
    : btrace_target_info (ptid)
    {}

#if HAVE_LINUX_PERF_EVENT_H
  /* The Linux perf_event configuration for collecting the branch trace.  */
  struct perf_event_attr attr {};

  /* The perf event file.  */
  int file = -1;

  /* The perf event configuration page.  */
  volatile struct perf_event_mmap_page *header = nullptr;

  /* The perf event buffer containing the trace data.  */
  struct perf_event_buffer pev {};
#endif /* HAVE_LINUX_PERF_EVENT_H */
};

/* See to_enable_btrace in target.h.  */
extern struct btrace_target_info *
  linux_enable_btrace (ptid_t ptid, const struct btrace_config *conf);

/* See to_disable_btrace in target.h.  */
extern enum btrace_error linux_disable_btrace (struct btrace_target_info *ti);

/* See to_read_btrace in target.h.  */
extern enum btrace_error linux_read_btrace (struct btrace_data *btrace,
					    struct btrace_target_info *btinfo,
					    enum btrace_read_type type);

/* See to_btrace_conf in target.h.  */
extern const struct btrace_config *
  linux_btrace_conf (const struct btrace_target_info *);

#endif /* NAT_LINUX_BTRACE_H */
