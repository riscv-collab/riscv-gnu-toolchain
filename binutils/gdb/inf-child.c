/* Base/prototype target for default child (native) targets.

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

/* This file provides a common base class/target that all native
   target implementations extend, by calling inf_child_target to get a
   new prototype target and then overriding target methods as
   necessary.  */

#include "defs.h"
#include "regcache.h"
#include "memattr.h"
#include "symtab.h"
#include "target.h"
#include "inferior.h"
#include <sys/stat.h>
#include "inf-child.h"
#include "gdbsupport/fileio.h"
#include "gdbsupport/agent.h"
#include "gdbsupport/gdb_wait.h"
#include "gdbsupport/filestuff.h"

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

static const target_info inf_child_target_info = {
  "native",
  N_("Native process"),
  N_("Native process (started by the \"run\" command).")
};

const target_info &
inf_child_target::info () const
{
  return inf_child_target_info;
}

/* See inf-child.h.  */

target_waitstatus
host_status_to_waitstatus (int hoststatus)
{
  if (WIFEXITED (hoststatus))
    return target_waitstatus ().set_exited (WEXITSTATUS (hoststatus));
  else if (!WIFSTOPPED (hoststatus))
    return target_waitstatus ().set_signalled
      (gdb_signal_from_host (WTERMSIG (hoststatus)));
  else
    return target_waitstatus ().set_stopped
      (gdb_signal_from_host (WSTOPSIG (hoststatus)));
}

inf_child_target::~inf_child_target ()
{}

void
inf_child_target::post_attach (int pid)
{
  /* This target doesn't require a meaningful "post attach" operation
     by a debugger.  */
}

/* Get ready to modify the registers array.  On machines which store
   individual registers, this doesn't need to do anything.  On
   machines which store all the registers in one fell swoop, this
   makes sure that registers contains all the registers from the
   program being debugged.  */

void
inf_child_target::prepare_to_store (struct regcache *regcache)
{
}

bool
inf_child_target::supports_terminal_ours ()
{
  return true;
}

void
inf_child_target::terminal_init ()
{
  child_terminal_init (this);
}

void
inf_child_target::terminal_inferior ()
{
  child_terminal_inferior (this);
}

void
inf_child_target::terminal_save_inferior ()
{
  child_terminal_save_inferior (this);
}

void
inf_child_target::terminal_ours_for_output ()
{
  child_terminal_ours_for_output (this);
}

void
inf_child_target::terminal_ours ()
{
  child_terminal_ours (this);
}

void
inf_child_target::interrupt ()
{
  child_interrupt (this);
}

void
inf_child_target::pass_ctrlc ()
{
  child_pass_ctrlc (this);
}

void
inf_child_target::terminal_info (const char *args, int from_tty)
{
  child_terminal_info (this, args, from_tty);
}

/* True if the user did "target native".  In that case, we won't
   unpush the child target automatically when the last inferior is
   gone.  */
static int inf_child_explicitly_opened;

/* See inf-child.h.  */

void
inf_child_open_target (const char *arg, int from_tty)
{
  target_ops *target = get_native_target ();

  /* There's always only ever one native target, and if we get here,
     it better be an inf-child target.  */
  gdb_assert (dynamic_cast<inf_child_target *> (target) != NULL);

  target_preopen (from_tty);
  current_inferior ()->push_target (target);
  inf_child_explicitly_opened = 1;
  if (from_tty)
    gdb_printf ("Done.  Use the \"run\" command to start a process.\n");
}

/* Implement the to_disconnect target_ops method.  */

void
inf_child_target::disconnect (const char *args, int from_tty)
{
  if (args != NULL)
    error (_("Argument given to \"disconnect\"."));

  /* This offers to detach/kill current inferiors, and then pops all
     targets.  */
  target_preopen (from_tty);
}

/* Implement the to_close target_ops method.  */

void
inf_child_target::close ()
{
  /* In case we were forcibly closed.  */
  inf_child_explicitly_opened = 0;
}

void
inf_child_target::mourn_inferior ()
{
  generic_mourn_inferior ();
  maybe_unpush_target ();
}

/* See inf-child.h.  */

void
inf_child_target::maybe_unpush_target ()
{
  if (!inf_child_explicitly_opened)
    current_inferior ()->unpush_target (this);
}

bool
inf_child_target::can_run ()
{
  return true;
}

bool
inf_child_target::can_create_inferior ()
{
  return true;
}

bool
inf_child_target::can_attach ()
{
  return true;
}

const char *
inf_child_target::pid_to_exec_file (int pid)
{
  /* This target doesn't support translation of a process ID to the
     filename of the executable file.  */
  return NULL;
}

/* Implementation of to_fileio_open.  */

int
inf_child_target::fileio_open (struct inferior *inf, const char *filename,
			       int flags, int mode, int warn_if_slow,
			       fileio_error *target_errno)
{
  int nat_flags;
  mode_t nat_mode;
  int fd;

  if (fileio_to_host_openflags (flags, &nat_flags) == -1
      || fileio_to_host_mode (mode, &nat_mode) == -1)
    {
      *target_errno = FILEIO_EINVAL;
      return -1;
    }

  fd = gdb_open_cloexec (filename, nat_flags, nat_mode).release ();
  if (fd == -1)
    *target_errno = host_to_fileio_error (errno);

  return fd;
}

/* Implementation of to_fileio_pwrite.  */

int
inf_child_target::fileio_pwrite (int fd, const gdb_byte *write_buf, int len,
				 ULONGEST offset, fileio_error *target_errno)
{
  int ret;

#ifdef HAVE_PWRITE
  ret = pwrite (fd, write_buf, len, (long) offset);
#else
  ret = -1;
#endif
  /* If we have no pwrite or it failed for this file, use lseek/write.  */
  if (ret == -1)
    {
      ret = lseek (fd, (long) offset, SEEK_SET);
      if (ret != -1)
	ret = write (fd, write_buf, len);
    }

  if (ret == -1)
    *target_errno = host_to_fileio_error (errno);

  return ret;
}

/* Implementation of to_fileio_pread.  */

int
inf_child_target::fileio_pread (int fd, gdb_byte *read_buf, int len,
				ULONGEST offset, fileio_error *target_errno)
{
  int ret;

#ifdef HAVE_PREAD
  ret = pread (fd, read_buf, len, (long) offset);
#else
  ret = -1;
#endif
  /* If we have no pread or it failed for this file, use lseek/read.  */
  if (ret == -1)
    {
      ret = lseek (fd, (long) offset, SEEK_SET);
      if (ret != -1)
	ret = read (fd, read_buf, len);
    }

  if (ret == -1)
    *target_errno = host_to_fileio_error (errno);

  return ret;
}

/* Implementation of to_fileio_fstat.  */

int
inf_child_target::fileio_fstat (int fd, struct stat *sb, fileio_error *target_errno)
{
  int ret;

  ret = fstat (fd, sb);
  if (ret == -1)
    *target_errno = host_to_fileio_error (errno);

  return ret;
}

/* Implementation of to_fileio_close.  */

int
inf_child_target::fileio_close (int fd, fileio_error *target_errno)
{
  int ret;

  ret = ::close (fd);
  if (ret == -1)
    *target_errno = host_to_fileio_error (errno);

  return ret;
}

/* Implementation of to_fileio_unlink.  */

int
inf_child_target::fileio_unlink (struct inferior *inf, const char *filename,
				 fileio_error *target_errno)
{
  int ret;

  ret = unlink (filename);
  if (ret == -1)
    *target_errno = host_to_fileio_error (errno);

  return ret;
}

/* Implementation of to_fileio_readlink.  */

std::optional<std::string>
inf_child_target::fileio_readlink (struct inferior *inf, const char *filename,
				   fileio_error *target_errno)
{
  /* We support readlink only on systems that also provide a compile-time
     maximum path length (PATH_MAX), at least for now.  */
#if defined (PATH_MAX)
  char buf[PATH_MAX];
  int len;

  len = readlink (filename, buf, sizeof buf);
  if (len < 0)
    {
      *target_errno = host_to_fileio_error (errno);
      return {};
    }

  return std::string (buf, len);
#else
  *target_errno = FILEIO_ENOSYS;
  return {};
#endif
}

bool
inf_child_target::use_agent (bool use)
{
  if (agent_loaded_p ())
    {
      ::use_agent = use;
      return true;
    }
  else
    return false;
}

bool
inf_child_target::can_use_agent ()
{
  return agent_loaded_p ();
}

void
inf_child_target::follow_exec (inferior *follow_inf, ptid_t ptid,
			       const char *execd_pathname)
{
  inferior *orig_inf = current_inferior ();

  process_stratum_target::follow_exec (follow_inf, ptid, execd_pathname);

  if (orig_inf != follow_inf)
    {
      /* If the target was implicitly push in the original inferior, unpush
	 it.  */
      scoped_restore_current_thread restore_thread;
      switch_to_inferior_no_thread (orig_inf);
      maybe_unpush_target ();
    }
}

/* See inf-child.h.  */

void
add_inf_child_target (inf_child_target *target)
{
  set_native_target (target);
  add_target (inf_child_target_info, inf_child_open_target);
}
