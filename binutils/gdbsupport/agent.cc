/* Shared utility routines for GDB to interact with agent.

   Copyright (C) 2009-2024 Free Software Foundation, Inc.

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

#include "common-defs.h"
#include "target/target.h"
#include "gdbsupport/symbol.h"
#include <unistd.h>
#include "filestuff.h"

#define IPA_SYM_STRUCT_NAME ipa_sym_addresses_common
#include "agent.h"

bool debug_agent = false;

/* A stdarg wrapper for debug_vprintf.  */

static void ATTRIBUTE_PRINTF (1, 2)
debug_agent_printf (const char *fmt, ...)
{
  va_list ap;

  if (!debug_agent)
    return;
  va_start (ap, fmt);
  debug_vprintf (fmt, ap);
  va_end (ap);
}

#define DEBUG_AGENT debug_agent_printf

/* Global flag to determine using agent or not.  */
bool use_agent = false;

/* Addresses of in-process agent's symbols both GDB and GDBserver cares
   about.  */

struct ipa_sym_addresses_common
{
  CORE_ADDR addr_helper_thread_id;
  CORE_ADDR addr_cmd_buf;
  CORE_ADDR addr_capability;
};

/* Cache of the helper thread id.  FIXME: this global should be made
   per-process.  */
static uint32_t helper_thread_id = 0;

static struct
{
  const char *name;
  int offset;
} symbol_list[] = {
  IPA_SYM(helper_thread_id),
  IPA_SYM(cmd_buf),
  IPA_SYM(capability),
};

static struct ipa_sym_addresses_common ipa_sym_addrs;

static bool all_agent_symbols_looked_up = false;

bool
agent_loaded_p (void)
{
  return all_agent_symbols_looked_up;
}

/* Look up all symbols needed by agent.  Return 0 if all the symbols are
   found, return non-zero otherwise.  */

int
agent_look_up_symbols (void *arg)
{
  all_agent_symbols_looked_up = false;

  for (int i = 0; i < sizeof (symbol_list) / sizeof (symbol_list[0]); i++)
    {
      CORE_ADDR *addrp =
	(CORE_ADDR *) ((char *) &ipa_sym_addrs + symbol_list[i].offset);
      struct objfile *objfile = (struct objfile *) arg;

      if (find_minimal_symbol_address (symbol_list[i].name, addrp,
				       objfile) != 0)
	{
	  DEBUG_AGENT ("symbol `%s' not found\n", symbol_list[i].name);
	  return -1;
	}
    }

  all_agent_symbols_looked_up = true;
  return 0;
}

static unsigned int
agent_get_helper_thread_id (void)
{
  if  (helper_thread_id == 0)
    {
      if (target_read_uint32 (ipa_sym_addrs.addr_helper_thread_id,
			      &helper_thread_id))
	warning (_("Error reading helper thread's id in lib"));
    }

  return helper_thread_id;
}

#ifdef HAVE_SYS_UN_H
#include <sys/socket.h>
#include <sys/un.h>
#define SOCK_DIR P_tmpdir

#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX sizeof(((struct sockaddr_un *) NULL)->sun_path)
#endif

#endif

/* Connects to synchronization socket.  PID is the pid of inferior, which is
   used to set up the connection socket.  */

static int
gdb_connect_sync_socket (int pid)
{
#ifdef HAVE_SYS_UN_H
  struct sockaddr_un addr = {};
  int res, fd;
  char path[UNIX_PATH_MAX];

  res = xsnprintf (path, UNIX_PATH_MAX, "%s/gdb_ust%d", P_tmpdir, pid);
  if (res >= UNIX_PATH_MAX)
    return -1;

  res = fd = gdb_socket_cloexec (PF_UNIX, SOCK_STREAM, 0);
  if (res == -1)
    {
      warning (_("error opening sync socket: %s"), safe_strerror (errno));
      return -1;
    }

  addr.sun_family = AF_UNIX;

  res = xsnprintf (addr.sun_path, UNIX_PATH_MAX, "%s", path);
  if (res >= UNIX_PATH_MAX)
    {
      warning (_("string overflow allocating socket name"));
      close (fd);
      return -1;
    }

  res = connect (fd, (struct sockaddr *) &addr, sizeof (addr));
  if (res == -1)
    {
      warning (_("error connecting sync socket (%s): %s. "
		 "Make sure the directory exists and that it is writable."),
		 path, safe_strerror (errno));
      close (fd);
      return -1;
    }

  return fd;
#else
  return -1;
#endif
}

/* Execute an agent command in the inferior.  PID is the value of pid
   of the inferior.  CMD is the buffer for command.  It is assumed to
   be at least IPA_CMD_BUF_SIZE bytes long.  GDB or GDBserver will
   store the command into it and fetch the return result from CMD.
   The interaction between GDB/GDBserver and the agent is synchronized
   by a synchronization socket.  Return zero if success, otherwise
   return non-zero.  */

int
agent_run_command (int pid, char *cmd, int len)
{
  int fd;
  int tid = agent_get_helper_thread_id ();
  ptid_t ptid = ptid_t (pid, tid);

  int ret = target_write_memory (ipa_sym_addrs.addr_cmd_buf,
				 (gdb_byte *) cmd, len);

  if (ret != 0)
    {
      warning (_("unable to write"));
      return -1;
    }

  DEBUG_AGENT ("agent: resumed helper thread\n");

  /* Resume helper thread.  */
  target_continue_no_signal (ptid);

  fd = gdb_connect_sync_socket (pid);
  if (fd >= 0)
    {
      char buf[1] = "";

      DEBUG_AGENT ("agent: signalling helper thread\n");

      do
	{
	  ret = write (fd, buf, 1);
	} while (ret == -1 && errno == EINTR);

	DEBUG_AGENT ("agent: waiting for helper thread's response\n");

      do
	{
	  ret = read (fd, buf, 1);
	} while (ret == -1 && errno == EINTR);

      close (fd);

      DEBUG_AGENT ("agent: helper thread's response received\n");
    }
  else
    return -1;

  /* Need to read response with the inferior stopped.  */
  if (ptid != null_ptid)
    {
      /* Stop thread PTID.  */
      DEBUG_AGENT ("agent: stop helper thread\n");
      target_stop_and_wait (ptid);
    }

  if (fd >= 0)
    {
      if (target_read_memory (ipa_sym_addrs.addr_cmd_buf, (gdb_byte *) cmd,
			      IPA_CMD_BUF_SIZE))
	{
	  warning (_("Error reading command response"));
	  return -1;
	}
    }

  return 0;
}

/* Each bit of it stands for a capability of agent.  */
static uint32_t agent_capability = 0;

/* Return true if agent has capability AGENT_CAP, otherwise return false.  */

bool
agent_capability_check (enum agent_capa agent_capa)
{
  if (agent_capability == 0)
    {
      if (target_read_uint32 (ipa_sym_addrs.addr_capability,
			      &agent_capability))
	warning (_("Error reading capability of agent"));
    }
  return (agent_capability & agent_capa) != 0;
}

/* Invalidate the cache of agent capability, so we'll read it from inferior
   again.  Call it when launches a new program or reconnect to remote stub.  */

void
agent_capability_invalidate (void)
{
  agent_capability = 0;
}
