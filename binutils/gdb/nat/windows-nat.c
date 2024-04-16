/* Internal interfaces for the Windows code
   Copyright (C) 1995-2024 Free Software Foundation, Inc.

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
#include "nat/windows-nat.h"
#include "gdbsupport/common-debug.h"
#include "target/target.h"

#undef GetModuleFileNameEx

#ifndef __CYGWIN__
#define GetModuleFileNameEx GetModuleFileNameExA
#else
#include <sys/cygwin.h>
#define __USEWIDE
#define GetModuleFileNameEx GetModuleFileNameExW
#endif

namespace windows_nat
{

/* The most recent event from WaitForDebugEvent.  Unlike
   current_event, this is guaranteed never to come from a pending
   stop.  This is important because only data from the most recent
   event from WaitForDebugEvent can be used when calling
   ContinueDebugEvent.  */
static DEBUG_EVENT last_wait_event;

AdjustTokenPrivileges_ftype *AdjustTokenPrivileges;
DebugActiveProcessStop_ftype *DebugActiveProcessStop;
DebugBreakProcess_ftype *DebugBreakProcess;
DebugSetProcessKillOnExit_ftype *DebugSetProcessKillOnExit;
EnumProcessModules_ftype *EnumProcessModules;
#ifdef __x86_64__
EnumProcessModulesEx_ftype *EnumProcessModulesEx;
#endif
GetModuleInformation_ftype *GetModuleInformation;
GetModuleFileNameExA_ftype *GetModuleFileNameExA;
GetModuleFileNameExW_ftype *GetModuleFileNameExW;
LookupPrivilegeValueA_ftype *LookupPrivilegeValueA;
OpenProcessToken_ftype *OpenProcessToken;
GetCurrentConsoleFont_ftype *GetCurrentConsoleFont;
GetConsoleFontSize_ftype *GetConsoleFontSize;
#ifdef __x86_64__
Wow64SuspendThread_ftype *Wow64SuspendThread;
Wow64GetThreadContext_ftype *Wow64GetThreadContext;
Wow64SetThreadContext_ftype *Wow64SetThreadContext;
Wow64GetThreadSelectorEntry_ftype *Wow64GetThreadSelectorEntry;
#endif
GenerateConsoleCtrlEvent_ftype *GenerateConsoleCtrlEvent;

#define GetThreadDescription dyn_GetThreadDescription
typedef HRESULT WINAPI (GetThreadDescription_ftype) (HANDLE, PWSTR *);
static GetThreadDescription_ftype *GetThreadDescription;

InitializeProcThreadAttributeList_ftype *InitializeProcThreadAttributeList;
UpdateProcThreadAttribute_ftype *UpdateProcThreadAttribute;
DeleteProcThreadAttributeList_ftype *DeleteProcThreadAttributeList;

/* Note that 'debug_events' must be locally defined in the relevant
   functions.  */
#define DEBUG_EVENTS(fmt, ...) \
  debug_prefixed_printf_cond (debug_events, "windows events", fmt, \
			      ## __VA_ARGS__)

void
windows_thread_info::suspend ()
{
  if (suspended != 0)
    return;

  if (SuspendThread (h) == (DWORD) -1)
    {
      DWORD err = GetLastError ();

      /* We get Access Denied (5) when trying to suspend
	 threads that Windows started on behalf of the
	 debuggee, usually when those threads are just
	 about to exit.
	 We can get Invalid Handle (6) if the main thread
	 has exited.  */
      if (err != ERROR_INVALID_HANDLE && err != ERROR_ACCESS_DENIED)
	warning (_("SuspendThread (tid=0x%x) failed. (winerr %u: %s)"),
		 (unsigned) tid, (unsigned) err, strwinerror (err));
      suspended = -1;
    }
  else
    suspended = 1;
}

void
windows_thread_info::resume ()
{
  if (suspended > 0)
    {
      stopped_at_software_breakpoint = false;

      if (ResumeThread (h) == (DWORD) -1)
	{
	  DWORD err = GetLastError ();
	  warning (_("warning: ResumeThread (tid=0x%x) failed. (winerr %u: %s)"),
		   (unsigned) tid, (unsigned) err, strwinerror (err));
	}
    }
  suspended = 0;
}

const char *
windows_thread_info::thread_name ()
{
  if (GetThreadDescription != nullptr)
    {
      PWSTR value;
      HRESULT result = GetThreadDescription (h, &value);
      if (SUCCEEDED (result))
	{
	  int needed = WideCharToMultiByte (CP_ACP, 0, value, -1, nullptr, 0,
					    nullptr, nullptr);
	  if (needed != 0)
	    {
	      /* USED_DEFAULT is how we detect that the encoding
		 conversion had to fall back to the substitution
		 character.  It seems better to just reject bad
		 conversions here.  */
	      BOOL used_default = FALSE;
	      gdb::unique_xmalloc_ptr<char> new_name
		((char *) xmalloc (needed));
	      if (WideCharToMultiByte (CP_ACP, 0, value, -1,
				       new_name.get (), needed,
				       nullptr, &used_default) == needed
		  && !used_default
		  && strlen (new_name.get ()) > 0)
		name = std::move (new_name);
	    }
	  LocalFree (value);
	}
    }

  return name.get ();
}

/* Try to determine the executable filename.

   EXE_NAME_RET is a pointer to a buffer whose size is EXE_NAME_MAX_LEN.

   Upon success, the filename is stored inside EXE_NAME_RET, and
   this function returns nonzero.

   Otherwise, this function returns zero and the contents of
   EXE_NAME_RET is undefined.  */

int
windows_process_info::get_exec_module_filename (char *exe_name_ret,
						size_t exe_name_max_len)
{
  DWORD len;
  HMODULE dh_buf;
  DWORD cbNeeded;

  cbNeeded = 0;
#ifdef __x86_64__
  if (wow64_process)
    {
      if (!EnumProcessModulesEx (handle,
				 &dh_buf, sizeof (HMODULE), &cbNeeded,
				 LIST_MODULES_32BIT)
	  || !cbNeeded)
	return 0;
    }
  else
#endif
    {
      if (!EnumProcessModules (handle,
			       &dh_buf, sizeof (HMODULE), &cbNeeded)
	  || !cbNeeded)
	return 0;
    }

  /* We know the executable is always first in the list of modules,
     which we just fetched.  So no need to fetch more.  */

#ifdef __CYGWIN__
  {
    /* Cygwin prefers that the path be in /x/y/z format, so extract
       the filename into a temporary buffer first, and then convert it
       to POSIX format into the destination buffer.  */
    wchar_t *pathbuf = (wchar_t *) alloca (exe_name_max_len * sizeof (wchar_t));

    len = GetModuleFileNameEx (handle,
			       dh_buf, pathbuf, exe_name_max_len);
    if (len == 0)
      {
	unsigned err = (unsigned) GetLastError ();
	throw_winerror_with_name (_("Error getting executable filename"), err);
      }
    if (cygwin_conv_path (CCP_WIN_W_TO_POSIX, pathbuf, exe_name_ret,
			  exe_name_max_len) < 0)
      error (_("Error converting executable filename to POSIX: %d."), errno);
  }
#else
  len = GetModuleFileNameEx (handle,
			     dh_buf, exe_name_ret, exe_name_max_len);
  if (len == 0)
    {
      unsigned err = (unsigned) GetLastError ();
      throw_winerror_with_name (_("Error getting executable filename"), err);
    }
#endif

    return 1;	/* success */
}

const char *
windows_process_info::pid_to_exec_file (int pid)
{
  static char path[MAX_PATH];
#ifdef __CYGWIN__
  /* Try to find exe name as symlink target of /proc/<pid>/exe.  */
  int nchars;
  char procexe[sizeof ("/proc/4294967295/exe")];

  xsnprintf (procexe, sizeof (procexe), "/proc/%u/exe", pid);
  nchars = readlink (procexe, path, sizeof(path));
  if (nchars > 0 && nchars < sizeof (path))
    {
      path[nchars] = '\0';	/* Got it */
      return path;
    }
#endif

  /* If we get here then either Cygwin is hosed, this isn't a Cygwin version
     of gdb, or we're trying to debug a non-Cygwin windows executable.  */
  if (!get_exec_module_filename (path, sizeof (path)))
    path[0] = '\0';

  return path;
}

/* Return the name of the DLL referenced by H at ADDRESS.  UNICODE
   determines what sort of string is read from the inferior.  Returns
   the name of the DLL, or NULL on error.  If a name is returned, it
   is stored in a static buffer which is valid until the next call to
   get_image_name.  */

static const char *
get_image_name (HANDLE h, void *address, int unicode)
{
#ifdef __CYGWIN__
  static char buf[MAX_PATH];
#else
  static char buf[(2 * MAX_PATH) + 1];
#endif
  DWORD size = unicode ? sizeof (WCHAR) : sizeof (char);
  char *address_ptr;
  int len = 0;
  char b[2];
  SIZE_T done;

  /* Attempt to read the name of the dll that was detected.
     This is documented to work only when actively debugging
     a program.  It will not work for attached processes.  */
  if (address == NULL)
    return NULL;

  /* See if we could read the address of a string, and that the
     address isn't null.  */
  if (!ReadProcessMemory (h, address,  &address_ptr,
			  sizeof (address_ptr), &done)
      || done != sizeof (address_ptr)
      || !address_ptr)
    return NULL;

  /* Find the length of the string.  */
  while (ReadProcessMemory (h, address_ptr + len++ * size, &b, size, &done)
	 && (b[0] != 0 || b[size - 1] != 0) && done == size)
    continue;

  if (!unicode)
    ReadProcessMemory (h, address_ptr, buf, len, &done);
  else
    {
      WCHAR *unicode_address = (WCHAR *) alloca (len * sizeof (WCHAR));
      ReadProcessMemory (h, address_ptr, unicode_address, len * sizeof (WCHAR),
			 &done);
#ifdef __CYGWIN__
      wcstombs (buf, unicode_address, MAX_PATH);
#else
      WideCharToMultiByte (CP_ACP, 0, unicode_address, len, buf, sizeof buf,
			   0, 0);
#endif
    }

  return buf;
}

/* See nat/windows-nat.h.  */

bool
windows_process_info::handle_ms_vc_exception (const EXCEPTION_RECORD *rec)
{
  if (rec->NumberParameters >= 3
      && (rec->ExceptionInformation[0] & 0xffffffff) == 0x1000)
    {
      DWORD named_thread_id;
      windows_thread_info *named_thread;
      CORE_ADDR thread_name_target;

      thread_name_target = rec->ExceptionInformation[1];
      named_thread_id = (DWORD) (0xffffffff & rec->ExceptionInformation[2]);

      if (named_thread_id == (DWORD) -1)
	named_thread_id = current_event.dwThreadId;

      named_thread = thread_rec (ptid_t (current_event.dwProcessId,
					 named_thread_id, 0),
				 DONT_INVALIDATE_CONTEXT);
      if (named_thread != NULL)
	{
	  int thread_name_len;
	  gdb::unique_xmalloc_ptr<char> thread_name
	    = target_read_string (thread_name_target, 1025, &thread_name_len);
	  if (thread_name_len > 0)
	    {
	      thread_name.get ()[thread_name_len - 1] = '\0';
	      named_thread->name = std::move (thread_name);
	    }
	}

      return true;
    }

  return false;
}

/* The exception thrown by a program to tell the debugger the name of
   a thread.  The exception record contains an ID of a thread and a
   name to give it.  This exception has no documented name, but MSDN
   dubs it "MS_VC_EXCEPTION" in one code example.  */
#define MS_VC_EXCEPTION 0x406d1388

handle_exception_result
windows_process_info::handle_exception (struct target_waitstatus *ourstatus,
					bool debug_exceptions)
{
#define DEBUG_EXCEPTION_SIMPLE(x)       if (debug_exceptions) \
  debug_printf ("gdb: Target exception %s at %s\n", x, \
    host_address_to_string (\
      current_event.u.Exception.ExceptionRecord.ExceptionAddress))

  EXCEPTION_RECORD *rec = &current_event.u.Exception.ExceptionRecord;
  DWORD code = rec->ExceptionCode;
  handle_exception_result result = HANDLE_EXCEPTION_HANDLED;

  memcpy (&siginfo_er, rec, sizeof siginfo_er);

  /* Record the context of the current thread.  */
  thread_rec (ptid_t (current_event.dwProcessId, current_event.dwThreadId, 0),
	      DONT_SUSPEND);

  last_sig = GDB_SIGNAL_0;

  switch (code)
    {
    case EXCEPTION_ACCESS_VIOLATION:
      DEBUG_EXCEPTION_SIMPLE ("EXCEPTION_ACCESS_VIOLATION");
      ourstatus->set_stopped (GDB_SIGNAL_SEGV);
      if (handle_access_violation (rec))
	return HANDLE_EXCEPTION_UNHANDLED;
      break;
    case STATUS_STACK_OVERFLOW:
      DEBUG_EXCEPTION_SIMPLE ("STATUS_STACK_OVERFLOW");
      ourstatus->set_stopped (GDB_SIGNAL_SEGV);
      break;
    case STATUS_FLOAT_DENORMAL_OPERAND:
      DEBUG_EXCEPTION_SIMPLE ("STATUS_FLOAT_DENORMAL_OPERAND");
      ourstatus->set_stopped (GDB_SIGNAL_FPE);
      break;
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
      DEBUG_EXCEPTION_SIMPLE ("EXCEPTION_ARRAY_BOUNDS_EXCEEDED");
      ourstatus->set_stopped (GDB_SIGNAL_FPE);
      break;
    case STATUS_FLOAT_INEXACT_RESULT:
      DEBUG_EXCEPTION_SIMPLE ("STATUS_FLOAT_INEXACT_RESULT");
      ourstatus->set_stopped (GDB_SIGNAL_FPE);
      break;
    case STATUS_FLOAT_INVALID_OPERATION:
      DEBUG_EXCEPTION_SIMPLE ("STATUS_FLOAT_INVALID_OPERATION");
      ourstatus->set_stopped (GDB_SIGNAL_FPE);
      break;
    case STATUS_FLOAT_OVERFLOW:
      DEBUG_EXCEPTION_SIMPLE ("STATUS_FLOAT_OVERFLOW");
      ourstatus->set_stopped (GDB_SIGNAL_FPE);
      break;
    case STATUS_FLOAT_STACK_CHECK:
      DEBUG_EXCEPTION_SIMPLE ("STATUS_FLOAT_STACK_CHECK");
      ourstatus->set_stopped (GDB_SIGNAL_FPE);
      break;
    case STATUS_FLOAT_UNDERFLOW:
      DEBUG_EXCEPTION_SIMPLE ("STATUS_FLOAT_UNDERFLOW");
      ourstatus->set_stopped (GDB_SIGNAL_FPE);
      break;
    case STATUS_FLOAT_DIVIDE_BY_ZERO:
      DEBUG_EXCEPTION_SIMPLE ("STATUS_FLOAT_DIVIDE_BY_ZERO");
      ourstatus->set_stopped (GDB_SIGNAL_FPE);
      break;
    case STATUS_INTEGER_DIVIDE_BY_ZERO:
      DEBUG_EXCEPTION_SIMPLE ("STATUS_INTEGER_DIVIDE_BY_ZERO");
      ourstatus->set_stopped (GDB_SIGNAL_FPE);
      break;
    case STATUS_INTEGER_OVERFLOW:
      DEBUG_EXCEPTION_SIMPLE ("STATUS_INTEGER_OVERFLOW");
      ourstatus->set_stopped (GDB_SIGNAL_FPE);
      break;
    case EXCEPTION_BREAKPOINT:
#ifdef __x86_64__
      if (ignore_first_breakpoint)
	{
	  /* For WOW64 processes, there are always 2 breakpoint exceptions
	     on startup, first a BREAKPOINT for the 64bit ntdll.dll,
	     then a WX86_BREAKPOINT for the 32bit ntdll.dll.
	     Here we only care about the WX86_BREAKPOINT's.  */
	  DEBUG_EXCEPTION_SIMPLE ("EXCEPTION_BREAKPOINT - ignore_first_breakpoint");
	  ourstatus->set_spurious ();
	  ignore_first_breakpoint = false;
	  break;
	}
      else if (wow64_process)
	{
	  /* This breakpoint exception is triggered for WOW64 processes when
	     reaching an int3 instruction in 64bit code.
	     gdb checks for int3 in case of SIGTRAP, this fails because
	     Wow64GetThreadContext can only report the pc of 32bit code, and
	     gdb lets the target process continue.
	     So handle it as SIGINT instead, then the target is stopped
	     unconditionally.  */
	  DEBUG_EXCEPTION_SIMPLE ("EXCEPTION_BREAKPOINT - wow64_process");
	  rec->ExceptionCode = DBG_CONTROL_C;
	  ourstatus->set_stopped (GDB_SIGNAL_INT);
	  break;
	}
#endif
      [[fallthrough]];
    case STATUS_WX86_BREAKPOINT:
      DEBUG_EXCEPTION_SIMPLE ("EXCEPTION_BREAKPOINT");
      ourstatus->set_stopped (GDB_SIGNAL_TRAP);
      break;
    case DBG_CONTROL_C:
      DEBUG_EXCEPTION_SIMPLE ("DBG_CONTROL_C");
      ourstatus->set_stopped (GDB_SIGNAL_INT);
      break;
    case DBG_CONTROL_BREAK:
      DEBUG_EXCEPTION_SIMPLE ("DBG_CONTROL_BREAK");
      ourstatus->set_stopped (GDB_SIGNAL_INT);
      break;
    case EXCEPTION_SINGLE_STEP:
    case STATUS_WX86_SINGLE_STEP:
      DEBUG_EXCEPTION_SIMPLE ("EXCEPTION_SINGLE_STEP");
      ourstatus->set_stopped (GDB_SIGNAL_TRAP);
      break;
    case EXCEPTION_ILLEGAL_INSTRUCTION:
      DEBUG_EXCEPTION_SIMPLE ("EXCEPTION_ILLEGAL_INSTRUCTION");
      ourstatus->set_stopped (GDB_SIGNAL_ILL);
      break;
    case EXCEPTION_PRIV_INSTRUCTION:
      DEBUG_EXCEPTION_SIMPLE ("EXCEPTION_PRIV_INSTRUCTION");
      ourstatus->set_stopped (GDB_SIGNAL_ILL);
      break;
    case EXCEPTION_NONCONTINUABLE_EXCEPTION:
      DEBUG_EXCEPTION_SIMPLE ("EXCEPTION_NONCONTINUABLE_EXCEPTION");
      ourstatus->set_stopped (GDB_SIGNAL_ILL);
      break;
    case MS_VC_EXCEPTION:
      DEBUG_EXCEPTION_SIMPLE ("MS_VC_EXCEPTION");
      if (handle_ms_vc_exception (rec))
	{
	  ourstatus->set_stopped (GDB_SIGNAL_TRAP);
	  result = HANDLE_EXCEPTION_IGNORED;
	  break;
	}
	/* treat improperly formed exception as unknown */
	[[fallthrough]];
    default:
      /* Treat unhandled first chance exceptions specially.  */
      if (current_event.u.Exception.dwFirstChance)
	return HANDLE_EXCEPTION_UNHANDLED;
      debug_printf ("gdb: unknown target exception 0x%08x at %s\n",
	(unsigned) current_event.u.Exception.ExceptionRecord.ExceptionCode,
	host_address_to_string (
	  current_event.u.Exception.ExceptionRecord.ExceptionAddress));
      ourstatus->set_stopped (GDB_SIGNAL_UNKNOWN);
      break;
    }

  if (ourstatus->kind () == TARGET_WAITKIND_STOPPED)
    last_sig = ourstatus->sig ();

  return result;

#undef DEBUG_EXCEPTION_SIMPLE
}

/* See nat/windows-nat.h.  */

void
windows_process_info::add_dll (LPVOID load_addr)
{
  HMODULE dummy_hmodule;
  DWORD cb_needed;
  HMODULE *hmodules;
  int i;

#ifdef __x86_64__
  if (wow64_process)
    {
      if (EnumProcessModulesEx (handle, &dummy_hmodule,
				sizeof (HMODULE), &cb_needed,
				LIST_MODULES_32BIT) == 0)
	return;
    }
  else
#endif
    {
      if (EnumProcessModules (handle, &dummy_hmodule,
			      sizeof (HMODULE), &cb_needed) == 0)
	return;
    }

  if (cb_needed < 1)
    return;

  hmodules = (HMODULE *) alloca (cb_needed);
#ifdef __x86_64__
  if (wow64_process)
    {
      if (EnumProcessModulesEx (handle, hmodules,
				cb_needed, &cb_needed,
				LIST_MODULES_32BIT) == 0)
	return;
    }
  else
#endif
    {
      if (EnumProcessModules (handle, hmodules,
			      cb_needed, &cb_needed) == 0)
	return;
    }

  char system_dir[MAX_PATH];
  char syswow_dir[MAX_PATH];
  size_t system_dir_len = 0;
  bool convert_syswow_dir = false;
#ifdef __x86_64__
  if (wow64_process)
#endif
    {
      /* This fails on 32bit Windows because it has no SysWOW64 directory,
	 and in this case a path conversion isn't necessary.  */
      UINT len = GetSystemWow64DirectoryA (syswow_dir, sizeof (syswow_dir));
      if (len > 0)
	{
	  /* Check that we have passed a large enough buffer.  */
	  gdb_assert (len < sizeof (syswow_dir));

	  len = GetSystemDirectoryA (system_dir, sizeof (system_dir));
	  /* Error check.  */
	  gdb_assert (len != 0);
	  /* Check that we have passed a large enough buffer.  */
	  gdb_assert (len < sizeof (system_dir));

	  strcat (system_dir, "\\");
	  strcat (syswow_dir, "\\");
	  system_dir_len = strlen (system_dir);

	  convert_syswow_dir = true;
	}

    }
  for (i = 1; i < (int) (cb_needed / sizeof (HMODULE)); i++)
    {
      MODULEINFO mi;
#ifdef __USEWIDE
      wchar_t dll_name[MAX_PATH];
      char dll_name_mb[MAX_PATH];
#else
      char dll_name[MAX_PATH];
#endif
      const char *name;
      if (GetModuleInformation (handle, hmodules[i],
				&mi, sizeof (mi)) == 0)
	continue;

      if (GetModuleFileNameEx (handle, hmodules[i],
			       dll_name, sizeof (dll_name)) == 0)
	continue;
#ifdef __USEWIDE
      wcstombs (dll_name_mb, dll_name, MAX_PATH);
      name = dll_name_mb;
#else
      name = dll_name;
#endif
      /* Convert the DLL path of 32bit processes returned by
	 GetModuleFileNameEx from the 64bit system directory to the
	 32bit syswow64 directory if necessary.  */
      std::string syswow_dll_path;
      if (convert_syswow_dir
	  && strncasecmp (name, system_dir, system_dir_len) == 0
	  && strchr (name + system_dir_len, '\\') == nullptr)
	{
	  syswow_dll_path = syswow_dir;
	  syswow_dll_path += name + system_dir_len;
	  name = syswow_dll_path.c_str();
	}

      /* Record the DLL if either LOAD_ADDR is NULL or the address
	 at which the DLL was loaded is equal to LOAD_ADDR.  */
      if (!(load_addr != nullptr && mi.lpBaseOfDll != load_addr))
	{
	  handle_load_dll (name, mi.lpBaseOfDll);
	  if (load_addr != nullptr)
	    return;
	}
    }
}

/* See nat/windows-nat.h.  */

void
windows_process_info::dll_loaded_event ()
{
  gdb_assert (current_event.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT);

  LOAD_DLL_DEBUG_INFO *event = &current_event.u.LoadDll;
  const char *dll_name;

  /* Try getting the DLL name via the lpImageName field of the event.
     Note that Microsoft documents this fields as strictly optional,
     in the sense that it might be NULL.  And the first DLL event in
     particular is explicitly documented as "likely not pass[ed]"
     (source: MSDN LOAD_DLL_DEBUG_INFO structure).  */
  dll_name = get_image_name (handle, event->lpImageName, event->fUnicode);
  /* If the DLL name could not be gleaned via lpImageName, try harder
     by enumerating all the DLLs loaded into the inferior, looking for
     one that is loaded at base address = lpBaseOfDll. */
  if (dll_name != nullptr)
    handle_load_dll (dll_name, event->lpBaseOfDll);
  else if (event->lpBaseOfDll != nullptr)
    add_dll (event->lpBaseOfDll);
}

/* See nat/windows-nat.h.  */

void
windows_process_info::add_all_dlls ()
{
  add_dll (nullptr);
}

/* See nat/windows-nat.h.  */

bool
windows_process_info::matching_pending_stop (bool debug_events)
{
  /* If there are pending stops, and we might plausibly hit one of
     them, we don't want to actually continue the inferior -- we just
     want to report the stop.  In this case, we just pretend to
     continue.  See the comment by the definition of "pending_stops"
     for details on why this is needed.  */
  for (const auto &item : pending_stops)
    {
      if (desired_stop_thread_id == -1
	  || desired_stop_thread_id == item.thread_id)
	{
	  DEBUG_EVENTS ("pending stop anticipated, desired=0x%x, item=0x%x",
			desired_stop_thread_id, item.thread_id);
	  return true;
	}
    }

  return false;
}

/* See nat/windows-nat.h.  */

std::optional<pending_stop>
windows_process_info::fetch_pending_stop (bool debug_events)
{
  std::optional<pending_stop> result;
  for (auto iter = pending_stops.begin ();
       iter != pending_stops.end ();
       ++iter)
    {
      if (desired_stop_thread_id == -1
	  || desired_stop_thread_id == iter->thread_id)
	{
	  result = *iter;
	  current_event = iter->event;

	  DEBUG_EVENTS ("pending stop found in 0x%x (desired=0x%x)",
			iter->thread_id, desired_stop_thread_id);

	  pending_stops.erase (iter);
	  break;
	}
    }

  return result;
}

/* See nat/windows-nat.h.  */

BOOL
continue_last_debug_event (DWORD continue_status, bool debug_events)
{
  DEBUG_EVENTS ("ContinueDebugEvent (cpid=%d, ctid=0x%x, %s)",
		(unsigned) last_wait_event.dwProcessId,
		(unsigned) last_wait_event.dwThreadId,
		continue_status == DBG_CONTINUE ?
		"DBG_CONTINUE" : "DBG_EXCEPTION_NOT_HANDLED");

  return ContinueDebugEvent (last_wait_event.dwProcessId,
			     last_wait_event.dwThreadId,
			     continue_status);
}

/* See nat/windows-nat.h.  */

BOOL
wait_for_debug_event (DEBUG_EVENT *event, DWORD timeout)
{
  BOOL result = WaitForDebugEvent (event, timeout);
  if (result)
    last_wait_event = *event;
  return result;
}

/* Flags to pass to UpdateProcThreadAttribute.  */
#define relocate_aslr_flags ((0x2 << 8) | (0x2 << 16))

/* Attribute to pass to UpdateProcThreadAttribute.  */
#define mitigation_policy 0x00020007

/* Pick one of the symbols as a sentinel.  */
#ifdef PROCESS_CREATION_MITIGATION_POLICY_FORCE_RELOCATE_IMAGES_ALWAYS_OFF

static_assert ((PROCESS_CREATION_MITIGATION_POLICY_FORCE_RELOCATE_IMAGES_ALWAYS_OFF
		| PROCESS_CREATION_MITIGATION_POLICY_BOTTOM_UP_ASLR_ALWAYS_OFF)
	       == relocate_aslr_flags,
	       "check that ASLR flag values are correct");

static_assert (PROC_THREAD_ATTRIBUTE_MITIGATION_POLICY == mitigation_policy,
	       "check that mitigation policy value is correct");

#endif

/* Helper template for the CreateProcess wrappers.

   FUNC is the type of the underlying CreateProcess call.  CHAR is the
   character type to use, and INFO is the "startupinfo" type to use.

   DO_CREATE_PROCESS is the underlying CreateProcess function to use;
   the remaining arguments are passed to it.  */
template<typename FUNC, typename CHAR, typename INFO>
BOOL
create_process_wrapper (FUNC *do_create_process, const CHAR *image,
			CHAR *command_line, DWORD flags,
			void *environment, const CHAR *cur_dir,
			bool no_randomization,
			INFO *startup_info,
			PROCESS_INFORMATION *process_info)
{
  if (no_randomization && disable_randomization_available ())
    {
      static bool tried_and_failed;

      if (!tried_and_failed)
	{
	  /* Windows 8 is required for the real declaration, but to
	     allow building on earlier versions of Windows, we declare
	     the type locally.  */
	  struct gdb_extended_info
	  {
	    INFO StartupInfo;
	    gdb_lpproc_thread_attribute_list lpAttributeList;
	  };

#	  ifndef EXTENDED_STARTUPINFO_PRESENT
#	   define EXTENDED_STARTUPINFO_PRESENT 0x00080000
#	  endif

	  gdb_extended_info info_ex {};

	  if (startup_info != nullptr)
	    info_ex.StartupInfo = *startup_info;
	  info_ex.StartupInfo.cb = sizeof (info_ex);
	  SIZE_T size = 0;
	  /* Ignore the result here.  The documentation says the first
	     call always fails, by design.  */
	  InitializeProcThreadAttributeList (nullptr, 1, 0, &size);
	  info_ex.lpAttributeList
	    = (gdb_lpproc_thread_attribute_list) alloca (size);
	  InitializeProcThreadAttributeList (info_ex.lpAttributeList,
					     1, 0, &size);

	  std::optional<BOOL> return_value;
	  DWORD attr_flags = relocate_aslr_flags;
	  if (!UpdateProcThreadAttribute (info_ex.lpAttributeList, 0,
					  mitigation_policy,
					  &attr_flags,
					  sizeof (attr_flags),
					  nullptr, nullptr))
	    tried_and_failed = true;
	  else
	    {
	      BOOL result = do_create_process (image, command_line,
					       nullptr, nullptr,
					       TRUE,
					       (flags
						| EXTENDED_STARTUPINFO_PRESENT),
					       environment,
					       cur_dir,
					       &info_ex.StartupInfo,
					       process_info);
	      if (result)
		return_value = result;
	      else if (GetLastError () == ERROR_INVALID_PARAMETER)
		tried_and_failed = true;
	      else
		return_value = FALSE;
	    }

	  DeleteProcThreadAttributeList (info_ex.lpAttributeList);

	  if (return_value.has_value ())
	    return *return_value;
	}
    }

  return do_create_process (image,
			    command_line, /* command line */
			    nullptr,	  /* Security */
			    nullptr,	  /* thread */
			    TRUE,	  /* inherit handles */
			    flags,	  /* start flags */
			    environment,  /* environment */
			    cur_dir,	  /* current directory */
			    startup_info,
			    process_info);
}

/* See nat/windows-nat.h.  */

BOOL
create_process (const char *image, char *command_line, DWORD flags,
		void *environment, const char *cur_dir,
		bool no_randomization,
		STARTUPINFOA *startup_info,
		PROCESS_INFORMATION *process_info)
{
  return create_process_wrapper (CreateProcessA, image, command_line, flags,
				 environment, cur_dir, no_randomization,
				 startup_info, process_info);
}

#ifdef __CYGWIN__

/* See nat/windows-nat.h.  */

BOOL
create_process (const wchar_t *image, wchar_t *command_line, DWORD flags,
		void *environment, const wchar_t *cur_dir,
		bool no_randomization,
		STARTUPINFOW *startup_info,
		PROCESS_INFORMATION *process_info)
{
  return create_process_wrapper (CreateProcessW, image, command_line, flags,
				 environment, cur_dir, no_randomization,
				 startup_info, process_info);
}

#endif /* __CYGWIN__ */

/* Define dummy functions which always return error for the rare cases where
   these functions could not be found.  */
template<typename... T>
BOOL WINAPI
bad (T... args)
{
  return FALSE;
}

template<typename... T>
DWORD WINAPI
bad (T... args)
{
  return 0;
}

static BOOL WINAPI
bad_GetCurrentConsoleFont (HANDLE w, BOOL bMaxWindow, CONSOLE_FONT_INFO *f)
{
  f->nFont = 0;
  return 1;
}

static COORD WINAPI
bad_GetConsoleFontSize (HANDLE w, DWORD nFont)
{
  COORD size;
  size.X = 8;
  size.Y = 12;
  return size;
}
 
/* See windows-nat.h.  */

bool
disable_randomization_available ()
{
  return (InitializeProcThreadAttributeList != nullptr
	  && UpdateProcThreadAttribute != nullptr
	  && DeleteProcThreadAttributeList != nullptr);
}

/* See windows-nat.h.  */

bool
initialize_loadable ()
{
  bool result = true;
  HMODULE hm = NULL;

#define GPA(m, func)					\
  func = (func ## _ftype *) GetProcAddress (m, #func)

  hm = LoadLibrary (TEXT ("kernel32.dll"));
  if (hm)
    {
      GPA (hm, DebugActiveProcessStop);
      GPA (hm, DebugBreakProcess);
      GPA (hm, DebugSetProcessKillOnExit);
      GPA (hm, GetConsoleFontSize);
      GPA (hm, DebugActiveProcessStop);
      GPA (hm, GetCurrentConsoleFont);
#ifdef __x86_64__
      GPA (hm, Wow64SuspendThread);
      GPA (hm, Wow64GetThreadContext);
      GPA (hm, Wow64SetThreadContext);
      GPA (hm, Wow64GetThreadSelectorEntry);
#endif
      GPA (hm, GenerateConsoleCtrlEvent);
      GPA (hm, GetThreadDescription);

      GPA (hm, InitializeProcThreadAttributeList);
      GPA (hm, UpdateProcThreadAttribute);
      GPA (hm, DeleteProcThreadAttributeList);
    }

  /* Set variables to dummy versions of these processes if the function
     wasn't found in kernel32.dll.  */
  if (!DebugBreakProcess)
    DebugBreakProcess = bad;
  if (!DebugActiveProcessStop || !DebugSetProcessKillOnExit)
    {
      DebugActiveProcessStop = bad;
      DebugSetProcessKillOnExit = bad;
    }
  if (!GetConsoleFontSize)
    GetConsoleFontSize = bad_GetConsoleFontSize;
  if (!GetCurrentConsoleFont)
    GetCurrentConsoleFont = bad_GetCurrentConsoleFont;

  /* Load optional functions used for retrieving filename information
     associated with the currently debugged process or its dlls.  */
  hm = LoadLibrary (TEXT ("psapi.dll"));
  if (hm)
    {
      GPA (hm, EnumProcessModules);
#ifdef __x86_64__
      GPA (hm, EnumProcessModulesEx);
#endif
      GPA (hm, GetModuleInformation);
      GPA (hm, GetModuleFileNameExA);
      GPA (hm, GetModuleFileNameExW);
    }

  if (!EnumProcessModules || !GetModuleInformation
      || !GetModuleFileNameExA || !GetModuleFileNameExW)
    {
      /* Set variables to dummy versions of these processes if the function
	 wasn't found in psapi.dll.  */
      EnumProcessModules = bad;
      GetModuleInformation = bad;
      GetModuleFileNameExA = bad;
      GetModuleFileNameExW = bad;

      result = false;
    }

  hm = LoadLibrary (TEXT ("advapi32.dll"));
  if (hm)
    {
      GPA (hm, OpenProcessToken);
      GPA (hm, LookupPrivilegeValueA);
      GPA (hm, AdjustTokenPrivileges);
      /* Only need to set one of these since if OpenProcessToken fails nothing
	 else is needed.  */
      if (!OpenProcessToken || !LookupPrivilegeValueA
	  || !AdjustTokenPrivileges)
	OpenProcessToken = bad;
    }

  /* On some versions of Windows, this function is only available in
     KernelBase.dll, not kernel32.dll.  */
  if (GetThreadDescription == nullptr)
    {
      hm = LoadLibrary (TEXT ("KernelBase.dll"));
      if (hm)
	GPA (hm, GetThreadDescription);
    }

#undef GPA

  return result;
}

}
