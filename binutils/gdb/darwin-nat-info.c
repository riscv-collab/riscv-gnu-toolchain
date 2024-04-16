/* Darwin support for GDB, the GNU debugger.
   Copyright (C) 1997-2024 Free Software Foundation, Inc.

   Contributed by Apple Computer, Inc.

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

/* The name of the ppc_thread_state structure, and the names of its
   members, have been changed for Unix conformance reasons.  The easiest
   way to have gdb build on systems with the older names and systems
   with the newer names is to build this compilation unit with the
   non-conformant define below.  This doesn't seem to cause the resulting
   binary any problems but it seems like it could cause us problems in
   the future.  It'd be good to remove this at some point when compiling on
   Tiger is no longer important.  */

#include "defs.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "gdbcore.h"
#include "value.h"
#include "gdbcmd.h"
#include "inferior.h"
#include "gdbarch.h"

#include <sys/sysctl.h>

#include "darwin-nat.h"

#include <mach/thread_info.h>
#include <mach/thread_act.h>
#include <mach/task.h>
#include <mach/vm_map.h>
#include <mach/mach_port.h>
#include <mach/mach_init.h>
#include <mach/mach_vm.h>

#define CHECK_ARGS(what, args) do { \
  if ((NULL == args) || ((args[0] != '0') && (args[1] != 'x'))) \
    error(_("%s must be specified with 0x..."), what);		\
} while (0)

#define PRINT_FIELD(structure, field) \
  gdb_printf(_(#field":\t%#lx\n"), (unsigned long) (structure)->field)

#define PRINT_TV_FIELD(structure, field) \
  gdb_printf(_(#field":\t%u.%06u sec\n"),			\
	     (unsigned) (structure)->field.seconds,		\
	     (unsigned) (structure)->field.microseconds)

#define task_self mach_task_self
#define task_by_unix_pid task_for_pid
#define port_name_array_t mach_port_array_t
#define port_type_array_t mach_port_array_t

static void
info_mach_tasks_command (const char *args, int from_tty)
{
  int sysControl[4];
  int count, index;
  size_t length;
  struct kinfo_proc *procInfo;

  sysControl[0] = CTL_KERN;
  sysControl[1] = KERN_PROC;
  sysControl[2] = KERN_PROC_ALL;

  sysctl (sysControl, 3, NULL, &length, NULL, 0);
  procInfo = (struct kinfo_proc *) xmalloc (length);
  sysctl (sysControl, 3, procInfo, &length, NULL, 0);

  count = (length / sizeof (struct kinfo_proc));
  gdb_printf (_("%d processes:\n"), count);
  for (index = 0; index < count; ++index)
    {
      kern_return_t result;
      mach_port_t taskPort;

      result =
	task_by_unix_pid (mach_task_self (), procInfo[index].kp_proc.p_pid,
			  &taskPort);
      if (KERN_SUCCESS == result)
	{
	  gdb_printf (_("    %s is %d has task %#x\n"),
		      procInfo[index].kp_proc.p_comm,
		      procInfo[index].kp_proc.p_pid, taskPort);
	}
      else
	{
	  gdb_printf (_("    %s is %d unknown task port\n"),
		      procInfo[index].kp_proc.p_comm,
		      procInfo[index].kp_proc.p_pid);
	}
    }

  xfree (procInfo);
}

static task_t
get_task_from_args (const char *args)
{
  task_t task;
  char *eptr;

  if (args == NULL || *args == 0)
    {
      if (inferior_ptid == null_ptid)
	gdb_printf (_("No inferior running\n"));

      darwin_inferior *priv = get_darwin_inferior (current_inferior ());

      return priv->task;
    }
  if (strcmp (args, "gdb") == 0)
    return mach_task_self ();
  task = strtoul (args, &eptr, 0);
  if (*eptr)
    {
      gdb_printf (_("cannot parse task id '%s'\n"), args);
      return TASK_NULL;
    }
  return task;
}

static void
info_mach_task_command (const char *args, int from_tty)
{
  union
  {
    struct task_basic_info basic;
    struct task_events_info events;
    struct task_thread_times_info thread_times;
  } task_info_data;

  kern_return_t result;
  unsigned int info_count;
  task_t task;

  task = get_task_from_args (args);
  if (task == TASK_NULL)
    return;

  gdb_printf (_("TASK_BASIC_INFO for 0x%x:\n"), task);
  info_count = TASK_BASIC_INFO_COUNT;
  result = task_info (task,
		      TASK_BASIC_INFO,
		      (task_info_t) & task_info_data.basic, &info_count);
  MACH_CHECK_ERROR (result);

  PRINT_FIELD (&task_info_data.basic, suspend_count);
  PRINT_FIELD (&task_info_data.basic, virtual_size);
  PRINT_FIELD (&task_info_data.basic, resident_size);
  PRINT_TV_FIELD (&task_info_data.basic, user_time);
  PRINT_TV_FIELD (&task_info_data.basic, system_time);
  gdb_printf (_("\nTASK_EVENTS_INFO:\n"));
  info_count = TASK_EVENTS_INFO_COUNT;
  result = task_info (task,
		      TASK_EVENTS_INFO,
		      (task_info_t) & task_info_data.events, &info_count);
  MACH_CHECK_ERROR (result);

  PRINT_FIELD (&task_info_data.events, faults);
#if 0
  PRINT_FIELD (&task_info_data.events, zero_fills);
  PRINT_FIELD (&task_info_data.events, reactivations);
#endif
  PRINT_FIELD (&task_info_data.events, pageins);
  PRINT_FIELD (&task_info_data.events, cow_faults);
  PRINT_FIELD (&task_info_data.events, messages_sent);
  PRINT_FIELD (&task_info_data.events, messages_received);
  gdb_printf (_("\nTASK_THREAD_TIMES_INFO:\n"));
  info_count = TASK_THREAD_TIMES_INFO_COUNT;
  result = task_info (task,
		      TASK_THREAD_TIMES_INFO,
		      (task_info_t) & task_info_data.thread_times,
		      &info_count);
  MACH_CHECK_ERROR (result);
  PRINT_TV_FIELD (&task_info_data.thread_times, user_time);
  PRINT_TV_FIELD (&task_info_data.thread_times, system_time);
}

static void
info_mach_ports_command (const char *args, int from_tty)
{
  port_name_array_t names;
  port_type_array_t types;
  unsigned int name_count, type_count;
  kern_return_t result;
  int index;
  task_t task;

  task = get_task_from_args (args);
  if (task == TASK_NULL)
    return;

  result = mach_port_names (task, &names, &name_count, &types, &type_count);
  MACH_CHECK_ERROR (result);

  gdb_assert (name_count == type_count);

  gdb_printf (_("Ports for task 0x%x:\n"), task);
  gdb_printf (_("port   type\n"));
  for (index = 0; index < name_count; ++index)
    {
      mach_port_t port = names[index];
      unsigned int j;
      struct type_descr
      {
	mach_port_type_t type;
	const char *name;
	mach_port_right_t right;
      };
      static struct type_descr descrs[] =
	{
	  {MACH_PORT_TYPE_SEND, "send", MACH_PORT_RIGHT_SEND},
	  {MACH_PORT_TYPE_SEND_ONCE, "send-once", MACH_PORT_RIGHT_SEND_ONCE},
	  {MACH_PORT_TYPE_RECEIVE, "receive", MACH_PORT_RIGHT_RECEIVE},
	  {MACH_PORT_TYPE_PORT_SET, "port-set", MACH_PORT_RIGHT_PORT_SET},
	  {MACH_PORT_TYPE_DEAD_NAME, "dead", MACH_PORT_RIGHT_DEAD_NAME}
	};

      gdb_printf (_("%04x: %08x "), port, types[index]);
      for (j = 0; j < sizeof(descrs) / sizeof(*descrs); j++)
	if (types[index] & descrs[j].type)
	  {
	    mach_port_urefs_t ref;
	    kern_return_t ret;

	    gdb_printf (_(" %s("), descrs[j].name);
	    ret = mach_port_get_refs (task, port, descrs[j].right, &ref);
	    if (ret != KERN_SUCCESS)
	      gdb_printf (_("??"));
	    else
	      gdb_printf (_("%u"), ref);
	    gdb_printf (_(" refs)"));
	  }
      
      if (task == task_self ())
	{
	  if (port == task_self())
	    gdb_printf (_(" gdb-task"));
	  else if (port == darwin_host_self)
	    gdb_printf (_(" host-self"));
	  else if (port == darwin_ex_port)
	    gdb_printf (_(" gdb-exception"));
	  else if (port == darwin_port_set)
	    gdb_printf (_(" gdb-port_set"));
	  else if (inferior_ptid != null_ptid)
	    {
	      struct inferior *inf = current_inferior ();
	      darwin_inferior *priv = get_darwin_inferior (inf);

	      if (port == priv->task)
		gdb_printf (_(" inferior-task"));
	      else if (port == priv->notify_port)
		gdb_printf (_(" inferior-notify"));
	      else
		{
		  for (int k = 0; k < priv->exception_info.count; k++)
		    if (port == priv->exception_info.ports[k])
		      {
			gdb_printf (_(" inferior-excp-port"));
			break;
		      }

		  for (darwin_thread_t *t : priv->threads)
		    {
		      if (port == t->gdb_port)
			{
			  gdb_printf (_(" inferior-thread for 0x%x"),
				      priv->task);
			  break;
			}
		    }

		}
	    }
	}
      gdb_printf (_("\n"));
    }

  vm_deallocate (task_self (), (vm_address_t) names,
		 (name_count * sizeof (mach_port_t)));
  vm_deallocate (task_self (), (vm_address_t) types,
		 (type_count * sizeof (mach_port_type_t)));
}


static void
darwin_debug_port_info (task_t task, mach_port_t port)
{
  kern_return_t kret;
  mach_port_status_t status;
  mach_msg_type_number_t len = sizeof (status);

  kret = mach_port_get_attributes
    (task, port, MACH_PORT_RECEIVE_STATUS, (mach_port_info_t)&status, &len);
  MACH_CHECK_ERROR (kret);

  gdb_printf (_("Port 0x%lx in task 0x%lx:\n"), (unsigned long) port,
	      (unsigned long) task);
  gdb_printf (_("  port set: 0x%x\n"), status.mps_pset);
  gdb_printf (_("     seqno: 0x%x\n"), status.mps_seqno);
  gdb_printf (_("   mscount: 0x%x\n"), status.mps_mscount);
  gdb_printf (_("    qlimit: 0x%x\n"), status.mps_qlimit);
  gdb_printf (_("  msgcount: 0x%x\n"), status.mps_msgcount);
  gdb_printf (_("  sorights: 0x%x\n"), status.mps_sorights);
  gdb_printf (_("   srights: 0x%x\n"), status.mps_srights);
  gdb_printf (_(" pdrequest: 0x%x\n"), status.mps_pdrequest);
  gdb_printf (_(" nsrequest: 0x%x\n"), status.mps_nsrequest);
  gdb_printf (_("     flags: 0x%x\n"), status.mps_flags);
}

static void
info_mach_port_command (const char *args, int from_tty)
{
  task_t task;
  mach_port_t port;

  CHECK_ARGS (_("Task and port"), args);
  sscanf (args, "0x%x 0x%x", &task, &port);

  darwin_debug_port_info (task, port);
}

static void
info_mach_threads_command (const char *args, int from_tty)
{
  thread_array_t threads;
  unsigned int thread_count;
  kern_return_t result;
  task_t task;
  int i;

  task = get_task_from_args (args);
  if (task == TASK_NULL)
    return;

  result = task_threads (task, &threads, &thread_count);
  MACH_CHECK_ERROR (result);

  gdb_printf (_("Threads in task %#x:\n"), task);
  for (i = 0; i < thread_count; ++i)
    {
      gdb_printf (_("    %#x\n"), threads[i]);
      mach_port_deallocate (task_self (), threads[i]);
    }

  vm_deallocate (task_self (), (vm_address_t) threads,
		 (thread_count * sizeof (thread_t)));
}

static void
info_mach_thread_command (const char *args, int from_tty)
{
  union
  {
    struct thread_basic_info basic;
  } thread_info_data;

  thread_t thread;
  kern_return_t result;
  unsigned int info_count;

  CHECK_ARGS (_("Thread"), args);
  sscanf (args, "0x%x", &thread);

  gdb_printf (_("THREAD_BASIC_INFO\n"));
  info_count = THREAD_BASIC_INFO_COUNT;
  result = thread_info (thread,
			THREAD_BASIC_INFO,
			(thread_info_t) & thread_info_data.basic,
			&info_count);
  MACH_CHECK_ERROR (result);

#if 0
  PRINT_FIELD (&thread_info_data.basic, user_time);
  PRINT_FIELD (&thread_info_data.basic, system_time);
#endif
  PRINT_FIELD (&thread_info_data.basic, cpu_usage);
  PRINT_FIELD (&thread_info_data.basic, run_state);
  PRINT_FIELD (&thread_info_data.basic, flags);
  PRINT_FIELD (&thread_info_data.basic, suspend_count);
  PRINT_FIELD (&thread_info_data.basic, sleep_time);
}

static const char *
unparse_protection (vm_prot_t p)
{
  switch (p)
    {
    case VM_PROT_NONE:
      return "---";
    case VM_PROT_READ:
      return "r--";
    case VM_PROT_WRITE:
      return "-w-";
    case VM_PROT_READ | VM_PROT_WRITE:
      return "rw-";
    case VM_PROT_EXECUTE:
      return "--x";
    case VM_PROT_EXECUTE | VM_PROT_READ:
      return "r-x";
    case VM_PROT_EXECUTE | VM_PROT_WRITE:
      return "-wx";
    case VM_PROT_EXECUTE | VM_PROT_WRITE | VM_PROT_READ:
      return "rwx";
    default:
      return "???";
    }
}

static const char *
unparse_inheritance (vm_inherit_t i)
{
  switch (i)
    {
    case VM_INHERIT_SHARE:
      return _("share");
    case VM_INHERIT_COPY:
      return _("copy ");
    case VM_INHERIT_NONE:
      return _("none ");
    default:
      return _("???  ");
    }
}

static const char *
unparse_share_mode (unsigned char p)
{
  switch (p)
    {
    case SM_COW:
      return _("cow");
    case SM_PRIVATE:
      return _("private");
    case SM_EMPTY:
      return _("empty");
    case SM_SHARED:
      return _("shared");
    case SM_TRUESHARED:
      return _("true-shrd");
    case SM_PRIVATE_ALIASED:
      return _("prv-alias");
    case SM_SHARED_ALIASED:
      return _("shr-alias");
    default:
      return _("???");
    }
}

static const char *
unparse_user_tag (unsigned int tag)
{
  switch (tag)
    {
    case 0:
      return _("default");
    case VM_MEMORY_MALLOC:
      return _("malloc");
    case VM_MEMORY_MALLOC_SMALL:
      return _("malloc_small");
    case VM_MEMORY_MALLOC_LARGE:
      return _("malloc_large");
    case VM_MEMORY_MALLOC_HUGE:
      return _("malloc_huge");
    case VM_MEMORY_SBRK:
      return _("sbrk");
    case VM_MEMORY_REALLOC:
      return _("realloc");
    case VM_MEMORY_MALLOC_TINY:
      return _("malloc_tiny");
    case VM_MEMORY_ANALYSIS_TOOL:
      return _("analysis_tool");
    case VM_MEMORY_MACH_MSG:
      return _("mach_msg");
    case VM_MEMORY_IOKIT:
      return _("iokit");
    case VM_MEMORY_STACK:
      return _("stack");
    case VM_MEMORY_GUARD:
      return _("guard");
    case VM_MEMORY_SHARED_PMAP:
      return _("shared_pmap");
    case VM_MEMORY_DYLIB:
      return _("dylib");
    case VM_MEMORY_APPKIT:
      return _("appkit");
    case VM_MEMORY_FOUNDATION:
      return _("foundation");
    default:
      return NULL;
    }
}

static void
darwin_debug_regions (task_t task, mach_vm_address_t address, int max)
{
  kern_return_t kret;
  vm_region_basic_info_data_64_t info, prev_info;
  mach_vm_address_t prev_address;
  mach_vm_size_t size, prev_size;

  mach_port_t object_name;
  mach_msg_type_number_t count;

  int nsubregions = 0;
  int num_printed = 0;

  count = VM_REGION_BASIC_INFO_COUNT_64;
  kret = mach_vm_region (task, &address, &size, VM_REGION_BASIC_INFO_64,
			 (vm_region_info_t) &info, &count, &object_name);
  if (kret != KERN_SUCCESS)
    {
      gdb_printf (_("No memory regions."));
      return;
    }
  memcpy (&prev_info, &info, sizeof (vm_region_basic_info_data_64_t));
  prev_address = address;
  prev_size = size;
  nsubregions = 1;

  for (;;)
    {
      int print = 0;
      int done = 0;

      address = prev_address + prev_size;

      /* Check to see if address space has wrapped around.  */
      if (address == 0)
	print = done = 1;

      if (!done)
	{
	  count = VM_REGION_BASIC_INFO_COUNT_64;
	  kret =
	    mach_vm_region (task, &address, &size, VM_REGION_BASIC_INFO_64,
		 	      (vm_region_info_t) &info, &count, &object_name);
	  if (kret != KERN_SUCCESS)
	    {
	      size = 0;
	      print = done = 1;
	    }
	}

      if (address != prev_address + prev_size)
	print = 1;

      if ((info.protection != prev_info.protection)
	  || (info.max_protection != prev_info.max_protection)
	  || (info.inheritance != prev_info.inheritance)
	  || (info.shared != prev_info.reserved)
	  || (info.reserved != prev_info.reserved))
	print = 1;

      if (print)
	{
	  gdbarch *arch = current_inferior ()->arch ();
	  gdb_printf (_("%s-%s %s/%s  %s %s %s"),
		      paddress (arch, prev_address),
		      paddress (arch, prev_address + prev_size),
		      unparse_protection (prev_info.protection),
		      unparse_protection (prev_info.max_protection),
		      unparse_inheritance (prev_info.inheritance),
		      prev_info.shared ? _("shrd") : _("priv"),
		      prev_info.reserved ? _("reserved") : _("not-rsvd"));

	  if (nsubregions > 1)
	    gdb_printf (_(" (%d sub-rgn)"), nsubregions);

	  gdb_printf (_("\n"));

	  prev_address = address;
	  prev_size = size;
	  memcpy (&prev_info, &info, sizeof (vm_region_basic_info_data_64_t));
	  nsubregions = 1;

	  num_printed++;
	}
      else
	{
	  prev_size += size;
	  nsubregions++;
	}

      if ((max > 0) && (num_printed >= max))
	done = 1;

      if (done)
	break;
    }
}

static void
darwin_debug_regions_recurse (task_t task)
{
  mach_vm_address_t r_start;
  mach_vm_size_t r_size;
  natural_t r_depth;
  mach_msg_type_number_t r_info_size;
  vm_region_submap_short_info_data_64_t r_info;
  kern_return_t kret;
  struct ui_out *uiout = current_uiout;

  ui_out_emit_table table_emitter (uiout, 9, -1, "regions");

  if (gdbarch_addr_bit (current_inferior ()->arch ()) <= 32)
    {
      uiout->table_header (10, ui_left, "start", "Start");
      uiout->table_header (10, ui_left, "end", "End");
    }
  else
    {
      uiout->table_header (18, ui_left, "start", "Start");
      uiout->table_header (18, ui_left, "end", "End");
    }
  uiout->table_header (3, ui_left, "min-prot", "Min");
  uiout->table_header (3, ui_left, "max-prot", "Max");
  uiout->table_header (5, ui_left, "inheritence", "Inh");
  uiout->table_header (9, ui_left, "share-mode", "Shr");
  uiout->table_header (1, ui_left, "depth", "D");
  uiout->table_header (3, ui_left, "submap", "Sm");
  uiout->table_header (0, ui_noalign, "tag", "Tag");

  uiout->table_body ();

  r_start = 0;
  r_depth = 0;
  while (1)
    {
      const char *tag;

      r_info_size = VM_REGION_SUBMAP_SHORT_INFO_COUNT_64;
      r_size = -1;
      kret = mach_vm_region_recurse (task, &r_start, &r_size, &r_depth,
				     (vm_region_recurse_info_t) &r_info,
				     &r_info_size);
      if (kret != KERN_SUCCESS)
	break;

      {
	ui_out_emit_tuple tuple_emitter (uiout, "regions-row");
	gdbarch *arch = current_inferior ()->arch ();

	uiout->field_core_addr ("start", arch, r_start);
	uiout->field_core_addr ("end", arch, r_start + r_size);
	uiout->field_string ("min-prot",
			     unparse_protection (r_info.protection));
	uiout->field_string ("max-prot",
			     unparse_protection (r_info.max_protection));
	uiout->field_string ("inheritence",
			     unparse_inheritance (r_info.inheritance));
	uiout->field_string ("share-mode",
			     unparse_share_mode (r_info.share_mode));
	uiout->field_signed ("depth", r_depth);
	uiout->field_string ("submap",
			     r_info.is_submap ? _("sm ") : _("obj"));
	tag = unparse_user_tag (r_info.user_tag);
	if (tag)
	  uiout->field_string ("tag", tag);
	else
	  uiout->field_signed ("tag", r_info.user_tag);
      }

      uiout->text ("\n");

      if (r_info.is_submap)
	r_depth++;
      else
	r_start += r_size;
    }
}


static void
darwin_debug_region (task_t task, mach_vm_address_t address)
{
  darwin_debug_regions (task, address, 1);
}

static void
info_mach_regions_command (const char *args, int from_tty)
{
  task_t task;

  task = get_task_from_args (args);
  if (task == TASK_NULL)
    return;
  
  darwin_debug_regions (task, 0, -1);
}

static void
info_mach_regions_recurse_command (const char *args, int from_tty)
{
  task_t task;

  task = get_task_from_args (args);
  if (task == TASK_NULL)
    return;
  
  darwin_debug_regions_recurse (task);
}

static void
info_mach_region_command (const char *exp, int from_tty)
{
  struct value *val;
  mach_vm_address_t address;
  struct inferior *inf;

  expression_up expr = parse_expression (exp);
  val = expr->evaluate ();
  if (TYPE_IS_REFERENCE (val->type ()))
    {
      val = value_ind (val);
    }
  address = value_as_address (val);

  if (inferior_ptid == null_ptid)
    error (_("Inferior not available"));

  inf = current_inferior ();
  darwin_inferior *priv = get_darwin_inferior (inf);
  darwin_debug_region (priv->task, address);
}

static void
disp_exception (const darwin_exception_info *info)
{
  int i;

  gdb_printf (_("%d exceptions:\n"), info->count);
  for (i = 0; i < info->count; i++)
    {
      exception_mask_t mask = info->masks[i];

      gdb_printf (_("port 0x%04x, behavior: "), info->ports[i]);
      switch (info->behaviors[i])
	{
	case EXCEPTION_DEFAULT:
	  gdb_printf (_("default"));
	  break;
	case EXCEPTION_STATE:
	  gdb_printf (_("state"));
	  break;
	case EXCEPTION_STATE_IDENTITY:
	  gdb_printf (_("state-identity"));
	  break;
	default:
	  gdb_printf (_("0x%x"), info->behaviors[i]);
	}
      gdb_printf (_(", masks:"));
      if (mask & EXC_MASK_BAD_ACCESS)
	gdb_printf (_(" BAD_ACCESS"));
      if (mask & EXC_MASK_BAD_INSTRUCTION)
	gdb_printf (_(" BAD_INSTRUCTION"));
      if (mask & EXC_MASK_ARITHMETIC)
	gdb_printf (_(" ARITHMETIC"));
      if (mask & EXC_MASK_EMULATION)
	gdb_printf (_(" EMULATION"));
      if (mask & EXC_MASK_SOFTWARE)
	gdb_printf (_(" SOFTWARE"));
      if (mask & EXC_MASK_BREAKPOINT)
	gdb_printf (_(" BREAKPOINT"));
      if (mask & EXC_MASK_SYSCALL)
	gdb_printf (_(" SYSCALL"));
      if (mask & EXC_MASK_MACH_SYSCALL)
	gdb_printf (_(" MACH_SYSCALL"));
      if (mask & EXC_MASK_RPC_ALERT)
	gdb_printf (_(" RPC_ALERT"));
      if (mask & EXC_MASK_CRASH)
	gdb_printf (_(" CRASH"));
      gdb_printf (_("\n"));
    }
}

static void
info_mach_exceptions_command (const char *args, int from_tty)
{
  kern_return_t kret;
  darwin_exception_info info;

  info.count = sizeof (info.ports) / sizeof (info.ports[0]);

  if (args != NULL)
    {
      if (strcmp (args, "saved") == 0)
	{
	  if (inferior_ptid == null_ptid)
	    gdb_printf (_("No inferior running\n"));

	  darwin_inferior *priv = get_darwin_inferior (current_inferior ());

	  disp_exception (&priv->exception_info);
	  return;
	}
      else if (strcmp (args, "host") == 0)
	{
	  /* FIXME: This needs a privileged host port!  */
	  kret = host_get_exception_ports
	    (darwin_host_self, EXC_MASK_ALL, info.masks,
	     &info.count, info.ports, info.behaviors, info.flavors);
	  MACH_CHECK_ERROR (kret);
	  disp_exception (&info);
	}
      else
	error (_("Parameter is saved, host or none"));
    }
  else
    {
      struct inferior *inf;

      if (inferior_ptid == null_ptid)
	gdb_printf (_("No inferior running\n"));
      inf = current_inferior ();
      
      darwin_inferior *priv = get_darwin_inferior (inf);

      kret = task_get_exception_ports
	(priv->task, EXC_MASK_ALL, info.masks,
	 &info.count, info.ports, info.behaviors, info.flavors);
      MACH_CHECK_ERROR (kret);
      disp_exception (&info);
    }
}

void _initialize_darwin_info_commands ();
void
_initialize_darwin_info_commands ()
{
  add_info ("mach-tasks", info_mach_tasks_command,
	    _("Get list of tasks in system."));
  add_info ("mach-ports", info_mach_ports_command,
	    _("Get list of ports in a task."));
  add_info ("mach-port", info_mach_port_command,
	    _("Get info on a specific port."));
  add_info ("mach-task", info_mach_task_command,
	    _("Get info on a specific task."));
  add_info ("mach-threads", info_mach_threads_command,
	    _("Get list of threads in a task."));
  add_info ("mach-thread", info_mach_thread_command,
	    _("Get info on a specific thread."));

  add_info ("mach-regions", info_mach_regions_command,
	    _("Get information on all mach region for the task."));
  add_info ("mach-regions-rec", info_mach_regions_recurse_command,
	    _("Get information on all mach sub region for the task."));
  add_info ("mach-region", info_mach_region_command,
	    _("Get information on mach region at given address."));

  add_info ("mach-exceptions", info_mach_exceptions_command,
	    _("Disp mach exceptions."));
}
