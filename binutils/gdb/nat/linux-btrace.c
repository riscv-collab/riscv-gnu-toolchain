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

#include "gdbsupport/common-defs.h"
#include "linux-btrace.h"
#include "gdbsupport/common-regcache.h"
#include "gdbsupport/gdb_wait.h"
#include "x86-cpuid.h"
#include "gdbsupport/filestuff.h"
#include "gdbsupport/scoped_fd.h"
#include "gdbsupport/scoped_mmap.h"

#include <inttypes.h>

#include <sys/syscall.h>

#if HAVE_LINUX_PERF_EVENT_H && defined(SYS_perf_event_open)
#include <unistd.h>
#include <sys/mman.h>
#include <sys/user.h>
#include "nat/gdb_ptrace.h"
#include <sys/types.h>
#include <signal.h>

/* A branch trace record in perf_event.  */
struct perf_event_bts
{
  /* The linear address of the branch source.  */
  uint64_t from;

  /* The linear address of the branch destination.  */
  uint64_t to;
};

/* A perf_event branch trace sample.  */
struct perf_event_sample
{
  /* The perf_event sample header.  */
  struct perf_event_header header;

  /* The perf_event branch tracing payload.  */
  struct perf_event_bts bts;
};

/* Identify the cpu we're running on.  */
static struct btrace_cpu
btrace_this_cpu (void)
{
  struct btrace_cpu cpu;
  unsigned int eax, ebx, ecx, edx;
  int ok;

  memset (&cpu, 0, sizeof (cpu));

  ok = x86_cpuid (0, &eax, &ebx, &ecx, &edx);
  if (ok != 0)
    {
      if (ebx == signature_INTEL_ebx && ecx == signature_INTEL_ecx
	  && edx == signature_INTEL_edx)
	{
	  unsigned int cpuid, ignore;

	  ok = x86_cpuid (1, &cpuid, &ignore, &ignore, &ignore);
	  if (ok != 0)
	    {
	      cpu.vendor = CV_INTEL;

	      cpu.family = (cpuid >> 8) & 0xf;
	      if (cpu.family == 0xf)
		cpu.family += (cpuid >> 20) & 0xff;

	      cpu.model = (cpuid >> 4) & 0xf;
	      if ((cpu.family == 0x6) || ((cpu.family & 0xf) == 0xf))
		cpu.model += (cpuid >> 12) & 0xf0;
	    }
	}
      else if (ebx == signature_AMD_ebx && ecx == signature_AMD_ecx
	       && edx == signature_AMD_edx)
	cpu.vendor = CV_AMD;
    }

  return cpu;
}

/* Return non-zero if there is new data in PEVENT; zero otherwise.  */

static int
perf_event_new_data (const struct perf_event_buffer *pev)
{
  return *pev->data_head != pev->last_head;
}

/* Copy the last SIZE bytes from PEV ending at DATA_HEAD and return a pointer
   to the memory holding the copy.
   The caller is responsible for freeing the memory.  */

static gdb_byte *
perf_event_read (const struct perf_event_buffer *pev, __u64 data_head,
		 size_t size)
{
  const gdb_byte *begin, *end, *start, *stop;
  gdb_byte *buffer;
  size_t buffer_size;
  __u64 data_tail;

  if (size == 0)
    return NULL;

  /* We should never ask for more data than the buffer can hold.  */
  buffer_size = pev->size;
  gdb_assert (size <= buffer_size);

  /* If we ask for more data than we seem to have, we wrap around and read
     data from the end of the buffer.  This is already handled by the %
     BUFFER_SIZE operation, below.  Here, we just need to make sure that we
     don't underflow.

     Note that this is perfectly OK for perf event buffers where data_head
     doesn'grow indefinitely and instead wraps around to remain within the
     buffer's boundaries.  */
  if (data_head < size)
    data_head += buffer_size;

  gdb_assert (size <= data_head);
  data_tail = data_head - size;

  begin = pev->mem;
  start = begin + data_tail % buffer_size;
  stop = begin + data_head % buffer_size;

  buffer = (gdb_byte *) xmalloc (size);

  if (start < stop)
    memcpy (buffer, start, stop - start);
  else
    {
      end = begin + buffer_size;

      memcpy (buffer, start, end - start);
      memcpy (buffer + (end - start), begin, stop - begin);
    }

  return buffer;
}

/* Copy the perf event buffer data from PEV.
   Store a pointer to the copy into DATA and its size in SIZE.  */

static void
perf_event_read_all (struct perf_event_buffer *pev, gdb_byte **data,
		     size_t *psize)
{
  size_t size;
  __u64 data_head;

  data_head = *pev->data_head;
  size = pev->size;

  *data = perf_event_read (pev, data_head, size);
  *psize = size;

  pev->last_head = data_head;
}

/* Try to determine the start address of the Linux kernel.  */

static uint64_t
linux_determine_kernel_start (void)
{
  static uint64_t kernel_start;
  static int cached;

  if (cached != 0)
    return kernel_start;

  cached = 1;

  gdb_file_up file = gdb_fopen_cloexec ("/proc/kallsyms", "r");
  if (file == NULL)
    return kernel_start;

  while (!feof (file.get ()))
    {
      char buffer[1024], symbol[8], *line;
      uint64_t addr;
      int match;

      line = fgets (buffer, sizeof (buffer), file.get ());
      if (line == NULL)
	break;

      match = sscanf (line, "%" SCNx64 " %*[tT] %7s", &addr, symbol);
      if (match != 2)
	continue;

      if (strcmp (symbol, "_text") == 0)
	{
	  kernel_start = addr;
	  break;
	}
    }

  return kernel_start;
}

/* Check whether an address is in the kernel.  */

static inline int
perf_event_is_kernel_addr (uint64_t addr)
{
  uint64_t kernel_start;

  kernel_start = linux_determine_kernel_start ();
  if (kernel_start != 0ull)
    return (addr >= kernel_start);

  /* If we don't know the kernel's start address, let's check the most
     significant bit.  This will work at least for 64-bit kernels.  */
  return ((addr & (1ull << 63)) != 0);
}

/* Check whether a perf event record should be skipped.  */

static inline int
perf_event_skip_bts_record (const struct perf_event_bts *bts)
{
  /* The hardware may report branches from kernel into user space.  Branches
     from user into kernel space will be suppressed.  We filter the former to
     provide a consistent branch trace excluding kernel.  */
  return perf_event_is_kernel_addr (bts->from);
}

/* Perform a few consistency checks on a perf event sample record.  This is
   meant to catch cases when we get out of sync with the perf event stream.  */

static inline int
perf_event_sample_ok (const struct perf_event_sample *sample)
{
  if (sample->header.type != PERF_RECORD_SAMPLE)
    return 0;

  if (sample->header.size != sizeof (*sample))
    return 0;

  return 1;
}

/* Branch trace is collected in a circular buffer [begin; end) as pairs of from
   and to addresses (plus a header).

   Start points into that buffer at the next sample position.
   We read the collected samples backwards from start.

   While reading the samples, we convert the information into a list of blocks.
   For two adjacent samples s1 and s2, we form a block b such that b.begin =
   s1.to and b.end = s2.from.

   In case the buffer overflows during sampling, one sample may have its lower
   part at the end and its upper part at the beginning of the buffer.  */

static std::vector<btrace_block> *
perf_event_read_bts (btrace_target_info *tinfo, const uint8_t *begin,
		     const uint8_t *end, const uint8_t *start, size_t size)
{
  std::vector<btrace_block> *btrace = new std::vector<btrace_block>;
  struct perf_event_sample sample;
  size_t read = 0;
  struct btrace_block block = { 0, 0 };

  gdb_assert (begin <= start);
  gdb_assert (start <= end);

  /* The first block ends at the current pc.  */
  reg_buffer_common *regcache = get_thread_regcache_for_ptid (tinfo->ptid);
  block.end = regcache_read_pc (regcache);

  /* The buffer may contain a partial record as its last entry (i.e. when the
     buffer size is not a multiple of the sample size).  */
  read = sizeof (sample) - 1;

  for (; read < size; read += sizeof (sample))
    {
      const struct perf_event_sample *psample;

      /* Find the next perf_event sample in a backwards traversal.  */
      start -= sizeof (sample);

      /* If we're still inside the buffer, we're done.  */
      if (begin <= start)
	psample = (const struct perf_event_sample *) start;
      else
	{
	  int missing;

	  /* We're to the left of the ring buffer, we will wrap around and
	     reappear at the very right of the ring buffer.  */

	  missing = (begin - start);
	  start = (end - missing);

	  /* If the entire sample is missing, we're done.  */
	  if (missing == sizeof (sample))
	    psample = (const struct perf_event_sample *) start;
	  else
	    {
	      uint8_t *stack;

	      /* The sample wrapped around.  The lower part is at the end and
		 the upper part is at the beginning of the buffer.  */
	      stack = (uint8_t *) &sample;

	      /* Copy the two parts so we have a contiguous sample.  */
	      memcpy (stack, start, missing);
	      memcpy (stack + missing, begin, sizeof (sample) - missing);

	      psample = &sample;
	    }
	}

      if (!perf_event_sample_ok (psample))
	{
	  warning (_("Branch trace may be incomplete."));
	  break;
	}

      if (perf_event_skip_bts_record (&psample->bts))
	continue;

      /* We found a valid sample, so we can complete the current block.  */
      block.begin = psample->bts.to;

      btrace->push_back (block);

      /* Start the next block.  */
      block.end = psample->bts.from;
    }

  /* Push the last block (i.e. the first one of inferior execution), as well.
     We don't know where it ends, but we know where it starts.  If we're
     reading delta trace, we can fill in the start address later on.
     Otherwise we will prune it.  */
  block.begin = 0;
  btrace->push_back (block);

  return btrace;
}

/* Check whether an Intel cpu supports BTS.  */

static int
intel_supports_bts (const struct btrace_cpu *cpu)
{
  switch (cpu->family)
    {
    case 0x6:
      switch (cpu->model)
	{
	case 0x1a: /* Nehalem */
	case 0x1f:
	case 0x1e:
	case 0x2e:
	case 0x25: /* Westmere */
	case 0x2c:
	case 0x2f:
	case 0x2a: /* Sandy Bridge */
	case 0x2d:
	case 0x3a: /* Ivy Bridge */

	  /* AAJ122: LBR, BTM, or BTS records may have incorrect branch
	     "from" information afer an EIST transition, T-states, C1E, or
	     Adaptive Thermal Throttling.  */
	  return 0;
	}
    }

  return 1;
}

/* Check whether the cpu supports BTS.  */

static int
cpu_supports_bts (void)
{
  struct btrace_cpu cpu;

  cpu = btrace_this_cpu ();
  switch (cpu.vendor)
    {
    default:
      /* Don't know about others.  Let's assume they do.  */
      return 1;

    case CV_INTEL:
      return intel_supports_bts (&cpu);

    case CV_AMD:
      return 0;
    }
}

/* The perf_event_open syscall failed.  Try to print a helpful error
   message.  */

static void
diagnose_perf_event_open_fail ()
{
  switch (errno)
    {
    case EPERM:
    case EACCES:
      {
	static const char filename[] = "/proc/sys/kernel/perf_event_paranoid";
	errno = 0;
	gdb_file_up file = gdb_fopen_cloexec (filename, "r");
	if (file.get () == nullptr)
	  error (_("Failed to open %s (%s).  Your system does not support "
		   "process recording."), filename, safe_strerror (errno));

	int level, found = fscanf (file.get (), "%d", &level);
	if (found == 1 && level > 2)
	  error (_("You do not have permission to record the process.  "
		   "Try setting %s to 2 or less."), filename);
      }

      break;
    }

  error (_("Failed to start recording: %s"), safe_strerror (errno));
}

/* Get the linux version of a btrace_target_info.  */

static linux_btrace_target_info *
get_linux_btrace_target_info (btrace_target_info *gtinfo)
{
  return gdb::checked_static_cast<linux_btrace_target_info *> (gtinfo);
}

/* Enable branch tracing in BTS format.  */

static struct btrace_target_info *
linux_enable_bts (ptid_t ptid, const struct btrace_config_bts *conf)
{
  size_t size, pages;
  __u64 data_offset;
  int pid, pg;

  if (!cpu_supports_bts ())
    error (_("BTS support has been disabled for the target cpu."));

  std::unique_ptr<linux_btrace_target_info> tinfo
    { std::make_unique<linux_btrace_target_info> (ptid) };

  tinfo->conf.format = BTRACE_FORMAT_BTS;

  tinfo->attr.size = sizeof (tinfo->attr);
  tinfo->attr.type = PERF_TYPE_HARDWARE;
  tinfo->attr.config = PERF_COUNT_HW_BRANCH_INSTRUCTIONS;
  tinfo->attr.sample_period = 1;

  /* We sample from and to address.  */
  tinfo->attr.sample_type = PERF_SAMPLE_IP | PERF_SAMPLE_ADDR;

  tinfo->attr.exclude_kernel = 1;
  tinfo->attr.exclude_hv = 1;
  tinfo->attr.exclude_idle = 1;

  pid = ptid.lwp ();
  if (pid == 0)
    pid = ptid.pid ();

  errno = 0;
  scoped_fd fd (syscall (SYS_perf_event_open, &tinfo->attr, pid, -1, -1, 0));
  if (fd.get () < 0)
    diagnose_perf_event_open_fail ();

  /* Convert the requested size in bytes to pages (rounding up).  */
  pages = ((size_t) conf->size / PAGE_SIZE
	   + ((conf->size % PAGE_SIZE) == 0 ? 0 : 1));
  /* We need at least one page.  */
  if (pages == 0)
    pages = 1;

  /* The buffer size can be requested in powers of two pages.  Adjust PAGES
     to the next power of two.  */
  for (pg = 0; pages != ((size_t) 1 << pg); ++pg)
    if ((pages & ((size_t) 1 << pg)) != 0)
      pages += ((size_t) 1 << pg);

  /* We try to allocate the requested size.
     If that fails, try to get as much as we can.  */
  scoped_mmap data;
  for (; pages > 0; pages >>= 1)
    {
      size_t length;
      __u64 data_size;

      data_size = (__u64) pages * PAGE_SIZE;

      /* Don't ask for more than we can represent in the configuration.  */
      if ((__u64) UINT_MAX < data_size)
	continue;

      size = (size_t) data_size;
      length = size + PAGE_SIZE;

      /* Check for overflows.  */
      if ((__u64) length != data_size + PAGE_SIZE)
	continue;

      errno = 0;
      /* The number of pages we request needs to be a power of two.  */
      data.reset (nullptr, length, PROT_READ, MAP_SHARED, fd.get (), 0);
      if (data.get () != MAP_FAILED)
	break;
    }

  if (pages == 0)
    error (_("Failed to map trace buffer: %s."), safe_strerror (errno));

  struct perf_event_mmap_page *header = (struct perf_event_mmap_page *)
    data.get ();
  data_offset = PAGE_SIZE;

#if defined (PERF_ATTR_SIZE_VER5)
  if (offsetof (struct perf_event_mmap_page, data_size) <= header->size)
    {
      __u64 data_size;

      data_offset = header->data_offset;
      data_size = header->data_size;

      size = (unsigned int) data_size;

      /* Check for overflows.  */
      if ((__u64) size != data_size)
	error (_("Failed to determine trace buffer size."));
    }
#endif /* defined (PERF_ATTR_SIZE_VER5) */

  tinfo->pev.size = size;
  tinfo->pev.data_head = &header->data_head;
  tinfo->pev.mem = (const uint8_t *) data.release () + data_offset;
  tinfo->pev.last_head = 0ull;
  tinfo->header = header;
  tinfo->file = fd.release ();

  tinfo->conf.bts.size = (unsigned int) size;
  return tinfo.release ();
}

#if defined (PERF_ATTR_SIZE_VER5)

/* Determine the event type.  */

static int
perf_event_pt_event_type ()
{
  static const char filename[] = "/sys/bus/event_source/devices/intel_pt/type";

  errno = 0;
  gdb_file_up file = gdb_fopen_cloexec (filename, "r");
  if (file.get () == nullptr)
    switch (errno)
      {
      case EACCES:
      case EFAULT:
      case EPERM:
	error (_("Failed to open %s (%s).  You do not have permission "
		 "to use Intel PT."), filename, safe_strerror (errno));

      case ENOTDIR:
      case ENOENT:
	error (_("Failed to open %s (%s).  Your system does not support "
		 "Intel PT."), filename, safe_strerror (errno));

      default:
	error (_("Failed to open %s: %s."), filename, safe_strerror (errno));
      }

  int type, found = fscanf (file.get (), "%d", &type);
  if (found != 1)
    error (_("Failed to read the PT event type from %s."), filename);

  return type;
}

/* Enable branch tracing in Intel Processor Trace format.  */

static struct btrace_target_info *
linux_enable_pt (ptid_t ptid, const struct btrace_config_pt *conf)
{
  size_t pages;
  int pid, pg;

  pid = ptid.lwp ();
  if (pid == 0)
    pid = ptid.pid ();

  std::unique_ptr<linux_btrace_target_info> tinfo
    { std::make_unique<linux_btrace_target_info> (ptid) };

  tinfo->conf.format = BTRACE_FORMAT_PT;

  tinfo->attr.size = sizeof (tinfo->attr);
  tinfo->attr.type = perf_event_pt_event_type ();

  tinfo->attr.exclude_kernel = 1;
  tinfo->attr.exclude_hv = 1;
  tinfo->attr.exclude_idle = 1;

  errno = 0;
  scoped_fd fd (syscall (SYS_perf_event_open, &tinfo->attr, pid, -1, -1, 0));
  if (fd.get () < 0)
    diagnose_perf_event_open_fail ();

  /* Allocate the configuration page. */
  scoped_mmap data (nullptr, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
		    fd.get (), 0);
  if (data.get () == MAP_FAILED)
    error (_("Failed to map trace user page: %s."), safe_strerror (errno));

  struct perf_event_mmap_page *header = (struct perf_event_mmap_page *)
    data.get ();

  header->aux_offset = header->data_offset + header->data_size;

  /* Convert the requested size in bytes to pages (rounding up).  */
  pages = ((size_t) conf->size / PAGE_SIZE
	   + ((conf->size % PAGE_SIZE) == 0 ? 0 : 1));
  /* We need at least one page.  */
  if (pages == 0)
    pages = 1;

  /* The buffer size can be requested in powers of two pages.  Adjust PAGES
     to the next power of two.  */
  for (pg = 0; pages != ((size_t) 1 << pg); ++pg)
    if ((pages & ((size_t) 1 << pg)) != 0)
      pages += ((size_t) 1 << pg);

  /* We try to allocate the requested size.
     If that fails, try to get as much as we can.  */
  scoped_mmap aux;
  for (; pages > 0; pages >>= 1)
    {
      size_t length;
      __u64 data_size;

      data_size = (__u64) pages * PAGE_SIZE;

      /* Don't ask for more than we can represent in the configuration.  */
      if ((__u64) UINT_MAX < data_size)
	continue;

      length = (size_t) data_size;

      /* Check for overflows.  */
      if ((__u64) length != data_size)
	continue;

      header->aux_size = data_size;

      errno = 0;
      aux.reset (nullptr, length, PROT_READ, MAP_SHARED, fd.get (),
		 header->aux_offset);
      if (aux.get () != MAP_FAILED)
	break;
    }

  if (pages == 0)
    error (_("Failed to map trace buffer: %s."), safe_strerror (errno));

  tinfo->pev.size = aux.size ();
  tinfo->pev.mem = (const uint8_t *) aux.release ();
  tinfo->pev.data_head = &header->aux_head;
  tinfo->header = (struct perf_event_mmap_page *) data.release ();
  gdb_assert (tinfo->header == header);
  tinfo->file = fd.release ();

  tinfo->conf.pt.size = (unsigned int) tinfo->pev.size;
  return tinfo.release ();
}

#else /* !defined (PERF_ATTR_SIZE_VER5) */

static struct btrace_target_info *
linux_enable_pt (ptid_t ptid, const struct btrace_config_pt *conf)
{
  error (_("Intel Processor Trace support was disabled at compile time."));
}

#endif /* !defined (PERF_ATTR_SIZE_VER5) */

/* See linux-btrace.h.  */

struct btrace_target_info *
linux_enable_btrace (ptid_t ptid, const struct btrace_config *conf)
{
  switch (conf->format)
    {
    case BTRACE_FORMAT_NONE:
      error (_("Bad branch trace format."));

    default:
      error (_("Unknown branch trace format."));

    case BTRACE_FORMAT_BTS:
      return linux_enable_bts (ptid, &conf->bts);

    case BTRACE_FORMAT_PT:
      return linux_enable_pt (ptid, &conf->pt);
    }
}

/* Disable BTS tracing.  */

static void
linux_disable_bts (struct linux_btrace_target_info *tinfo)
{
  munmap ((void *) tinfo->header, tinfo->pev.size + PAGE_SIZE);
  close (tinfo->file);
}

/* Disable Intel Processor Trace tracing.  */

static void
linux_disable_pt (struct linux_btrace_target_info *tinfo)
{
  munmap ((void *) tinfo->pev.mem, tinfo->pev.size);
  munmap ((void *) tinfo->header, PAGE_SIZE);
  close (tinfo->file);
}

/* See linux-btrace.h.  */

enum btrace_error
linux_disable_btrace (struct btrace_target_info *gtinfo)
{
  linux_btrace_target_info *tinfo
    = get_linux_btrace_target_info (gtinfo);

  switch (tinfo->conf.format)
    {
    case BTRACE_FORMAT_NONE:
      return BTRACE_ERR_NOT_SUPPORTED;

    case BTRACE_FORMAT_BTS:
      linux_disable_bts (tinfo);
      delete tinfo;
      return BTRACE_ERR_NONE;

    case BTRACE_FORMAT_PT:
      linux_disable_pt (tinfo);
      delete tinfo;
      return BTRACE_ERR_NONE;
    }

  return BTRACE_ERR_NOT_SUPPORTED;
}

/* Read branch trace data in BTS format for the thread given by TINFO into
   BTRACE using the TYPE reading method.  */

static enum btrace_error
linux_read_bts (btrace_data_bts *btrace, linux_btrace_target_info *tinfo,
		enum btrace_read_type type)
{
  const uint8_t *begin, *end, *start;
  size_t buffer_size, size;
  __u64 data_head = 0, data_tail;
  unsigned int retries = 5;

  /* For delta reads, we return at least the partial last block containing
     the current PC.  */
  if (type == BTRACE_READ_NEW && !perf_event_new_data (&tinfo->pev))
    return BTRACE_ERR_NONE;

  buffer_size = tinfo->pev.size;
  data_tail = tinfo->pev.last_head;

  /* We may need to retry reading the trace.  See below.  */
  while (retries--)
    {
      data_head = *tinfo->pev.data_head;

      /* Delete any leftover trace from the previous iteration.  */
      delete btrace->blocks;
      btrace->blocks = nullptr;

      if (type == BTRACE_READ_DELTA)
	{
	  __u64 data_size;

	  /* Determine the number of bytes to read and check for buffer
	     overflows.  */

	  /* Check for data head overflows.  We might be able to recover from
	     those but they are very unlikely and it's not really worth the
	     effort, I think.  */
	  if (data_head < data_tail)
	    return BTRACE_ERR_OVERFLOW;

	  /* If the buffer is smaller than the trace delta, we overflowed.  */
	  data_size = data_head - data_tail;
	  if (buffer_size < data_size)
	    return BTRACE_ERR_OVERFLOW;

	  /* DATA_SIZE <= BUFFER_SIZE and therefore fits into a size_t.  */
	  size = (size_t) data_size;
	}
      else
	{
	  /* Read the entire buffer.  */
	  size = buffer_size;

	  /* Adjust the size if the buffer has not overflowed, yet.  */
	  if (data_head < size)
	    size = (size_t) data_head;
	}

      /* Data_head keeps growing; the buffer itself is circular.  */
      begin = tinfo->pev.mem;
      start = begin + data_head % buffer_size;

      if (data_head <= buffer_size)
	end = start;
      else
	end = begin + tinfo->pev.size;

      btrace->blocks = perf_event_read_bts (tinfo, begin, end, start, size);

      /* The stopping thread notifies its ptracer before it is scheduled out.
	 On multi-core systems, the debugger might therefore run while the
	 kernel might be writing the last branch trace records.

	 Let's check whether the data head moved while we read the trace.  */
      if (data_head == *tinfo->pev.data_head)
	break;
    }

  tinfo->pev.last_head = data_head;

  /* Prune the incomplete last block (i.e. the first one of inferior execution)
     if we're not doing a delta read.  There is no way of filling in its zeroed
     BEGIN element.  */
  if (!btrace->blocks->empty () && type != BTRACE_READ_DELTA)
    btrace->blocks->pop_back ();

  return BTRACE_ERR_NONE;
}

/* Fill in the Intel Processor Trace configuration information.  */

static void
linux_fill_btrace_pt_config (struct btrace_data_pt_config *conf)
{
  conf->cpu = btrace_this_cpu ();
}

/* Read branch trace data in Intel Processor Trace format for the thread
   given by TINFO into BTRACE using the TYPE reading method.  */

static enum btrace_error
linux_read_pt (btrace_data_pt *btrace, linux_btrace_target_info *tinfo,
	       enum btrace_read_type type)
{
  linux_fill_btrace_pt_config (&btrace->config);

  switch (type)
    {
    case BTRACE_READ_DELTA:
      /* We don't support delta reads.  The data head (i.e. aux_head) wraps
	 around to stay inside the aux buffer.  */
      return BTRACE_ERR_NOT_SUPPORTED;

    case BTRACE_READ_NEW:
      if (!perf_event_new_data (&tinfo->pev))
	return BTRACE_ERR_NONE;
      [[fallthrough]];
    case BTRACE_READ_ALL:
      perf_event_read_all (&tinfo->pev, &btrace->data, &btrace->size);
      return BTRACE_ERR_NONE;
    }

  internal_error (_("Unknown btrace read type."));
}

/* See linux-btrace.h.  */

enum btrace_error
linux_read_btrace (struct btrace_data *btrace,
		   struct btrace_target_info *gtinfo,
		   enum btrace_read_type type)
{
  linux_btrace_target_info *tinfo
    = get_linux_btrace_target_info (gtinfo);

  switch (tinfo->conf.format)
    {
    case BTRACE_FORMAT_NONE:
      return BTRACE_ERR_NOT_SUPPORTED;

    case BTRACE_FORMAT_BTS:
      /* We read btrace in BTS format.  */
      btrace->format = BTRACE_FORMAT_BTS;
      btrace->variant.bts.blocks = NULL;

      return linux_read_bts (&btrace->variant.bts, tinfo, type);

    case BTRACE_FORMAT_PT:
      /* We read btrace in Intel Processor Trace format.  */
      btrace->format = BTRACE_FORMAT_PT;
      btrace->variant.pt.data = NULL;
      btrace->variant.pt.size = 0;

      return linux_read_pt (&btrace->variant.pt, tinfo, type);
    }

  internal_error (_("Unkown branch trace format."));
}

/* See linux-btrace.h.  */

const struct btrace_config *
linux_btrace_conf (const struct btrace_target_info *tinfo)
{
  return &tinfo->conf;
}

#else /* !HAVE_LINUX_PERF_EVENT_H */

/* See linux-btrace.h.  */

struct btrace_target_info *
linux_enable_btrace (ptid_t ptid, const struct btrace_config *conf)
{
  return NULL;
}

/* See linux-btrace.h.  */

enum btrace_error
linux_disable_btrace (struct btrace_target_info *tinfo)
{
  return BTRACE_ERR_NOT_SUPPORTED;
}

/* See linux-btrace.h.  */

enum btrace_error
linux_read_btrace (struct btrace_data *btrace,
		   struct btrace_target_info *tinfo,
		   enum btrace_read_type type)
{
  return BTRACE_ERR_NOT_SUPPORTED;
}

/* See linux-btrace.h.  */

const struct btrace_config *
linux_btrace_conf (const struct btrace_target_info *tinfo)
{
  return NULL;
}

#endif /* !HAVE_LINUX_PERF_EVENT_H */
