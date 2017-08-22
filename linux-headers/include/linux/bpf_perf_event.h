/* Copyright (c) 2016 Facebook
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 */
#ifndef __LINUX_BPF_PERF_EVENT_H__
#define __LINUX_BPF_PERF_EVENT_H__

#include <linux/types.h>
#include <linux/ptrace.h>

struct bpf_perf_event_data {
	struct pt_regs regs;
	__u64 sample_period;
};

#endif /* __LINUX_BPF_PERF_EVENT_H__ */
