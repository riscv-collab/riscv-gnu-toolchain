/* Serial interface for local domain connections on Un*x like systems.

   Copyright (C) 1992-2024 Free Software Foundation, Inc.

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

#include "defs.h"
#include "serial.h"
#include "ser-base.h"

#include <sys/socket.h>
#include <sys/un.h>

#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX sizeof(((struct sockaddr_un *) NULL)->sun_path)
#endif

/* Open an AF_UNIX socket.  */

static void
uds_open (struct serial *scb, const char *name)
{
  struct sockaddr_un addr;

  if (strlen (name) > UNIX_PATH_MAX - 1)
    error (_("The socket name is too long.  It may be no longer than %s bytes."),
	   pulongest (UNIX_PATH_MAX - 1L));

  memset (&addr, 0, sizeof addr);
  addr.sun_family = AF_UNIX;
  strncpy (addr.sun_path, name, UNIX_PATH_MAX - 1);

  scb->fd = socket (AF_UNIX, SOCK_STREAM, 0);
  if (scb->fd < 0)
    perror_with_name (_("could not open socket"));

  if (connect (scb->fd, (struct sockaddr *) &addr,
	       sizeof (struct sockaddr_un)) < 0)
    {
      int saved = errno;
      close (scb->fd);
      perror_with_name (_("could not connect to remote"), saved);
    }
}

static void
uds_close (struct serial *scb)
{
  if (scb->fd == -1)
    return;

  close (scb->fd);
  scb->fd = -1;
}

static int
uds_read_prim (struct serial *scb, size_t count)
{
  int result = recv (scb->fd, scb->buf, count, 0);
  if (result == -1 && errno != EINTR)
    perror_with_name ("error while reading");
  return result;
}

static int
uds_write_prim (struct serial *scb, const void *buf, size_t count)
{
  int result = send (scb->fd, buf, count, 0);
  if (result == -1 && errno != EINTR)
    perror_with_name ("error while writing");
  return result;
}

/* The local socket ops.  */

static const struct serial_ops uds_ops =
{
  "local",
  uds_open,
  uds_close,
  NULL,
  ser_base_readchar,
  ser_base_write,
  ser_base_flush_output,
  ser_base_flush_input,
  ser_base_send_break,
  ser_base_raw,
  ser_base_get_tty_state,
  ser_base_copy_tty_state,
  ser_base_set_tty_state,
  ser_base_print_tty_state,
  ser_base_setbaudrate,
  ser_base_setstopbits,
  ser_base_setparity,
  ser_base_drain_output,
  ser_base_async,
  uds_read_prim,
  uds_write_prim
};

void _initialize_ser_socket ();
void
_initialize_ser_socket ()
{
  serial_add_interface (&uds_ops);
}
