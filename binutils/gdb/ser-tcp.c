/* Serial interface for raw TCP connections on Un*x like systems.

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
#include "ser-tcp.h"
#include "gdbcmd.h"
#include "cli/cli-decode.h"
#include "cli/cli-setshow.h"
#include "gdbsupport/filestuff.h"
#include "gdbsupport/netstuff.h"

#include <sys/types.h>

#ifdef HAVE_SYS_FILIO_H
#include <sys/filio.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#include "gdbsupport/gdb_sys_time.h"

#ifdef USE_WIN32API
#include <ws2tcpip.h>
#ifndef ETIMEDOUT
#define ETIMEDOUT WSAETIMEDOUT
#endif
/* Gnulib defines close too, but gnulib's replacement
   doesn't call closesocket unless we import the
   socketlib module.  */
#undef close
#define close(fd) closesocket (fd)
#define ioctl ioctlsocket
#else
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#endif

#include <signal.h>
#include "gdbsupport/gdb_select.h"
#include <algorithm>

#ifndef HAVE_SOCKLEN_T
typedef int socklen_t;
#endif

/* For "set tcp" and "show tcp".  */

static struct cmd_list_element *tcp_set_cmdlist;
static struct cmd_list_element *tcp_show_cmdlist;

/* Whether to auto-retry refused connections.  */

static bool tcp_auto_retry = true;

/* Timeout period for connections, in seconds.  */

static unsigned int tcp_retry_limit = 15;

/* How many times per second to poll deprecated_ui_loop_hook.  */

#define POLL_INTERVAL 5

/* Helper function to wait a while.  If SOCK is not -1, wait on its
   file descriptor.  Otherwise just wait on a timeout, updating
   *POLLS.  Returns -1 on timeout or interrupt and set OUT_ERROR,
   otherwise the value of select.  */

static int
wait_for_connect (int sock, unsigned int *polls, ULONGEST *out_error)
{
  struct timeval t;
  int n;

  /* While we wait for the connect to complete, 
     poll the UI so it can update or the user can 
     interrupt.  */
  if (deprecated_ui_loop_hook && deprecated_ui_loop_hook (0))
    {
      *out_error = EINTR;
      return -1;
    }

  /* Check for timeout.  */
  if (*polls > tcp_retry_limit * POLL_INTERVAL)
    {
      *out_error = ETIMEDOUT;
      return -1;
    }

  /* Back off to polling once per second after the first POLL_INTERVAL
     polls.  */
  if (*polls < POLL_INTERVAL)
    {
      t.tv_sec = 0;
      t.tv_usec = 1000000 / POLL_INTERVAL;
    }
  else
    {
      t.tv_sec = 1;
      t.tv_usec = 0;
    }

  if (sock >= 0)
    {
      fd_set rset, wset, eset;

      FD_ZERO (&rset);
      FD_SET (sock, &rset);
      wset = rset;
      eset = rset;

      /* POSIX systems return connection success or failure by signalling
	 wset.  Windows systems return success in wset and failure in
	 eset.

	 We must call select here, rather than gdb_select, because
	 the serial structure has not yet been initialized - the
	 MinGW select wrapper will not know that this FD refers
	 to a socket.  */
      n = select (sock + 1, &rset, &wset, &eset, &t);
    }
  else
    /* Use gdb_select here, since we have no file descriptors, and on
       Windows, plain select doesn't work in that case.  */
    n = interruptible_select (0, NULL, NULL, NULL, &t);

  /* If we didn't time out, only count it as one poll.  */
  if (n > 0 || *polls < POLL_INTERVAL)
    (*polls)++;
  else
    (*polls) += POLL_INTERVAL;

  return n;
}

/* A helper to get the error number for either Windows or POSIX.  */
static ULONGEST
get_error ()
{
#ifdef USE_WIN32API
  return WSAGetLastError ();
#else
  return errno;
#endif
}

/* Try to connect to the host represented by AINFO.  If the connection
   succeeds, return its socket.  Otherwise, return -1 and set OUT_ERROR
   accordingly.  POLLS is used when 'connect' returns EINPROGRESS, and
   we need to invoke 'wait_for_connect' to obtain the status.  */

static int
try_connect (const struct addrinfo *ainfo, unsigned int *polls,
	     ULONGEST *out_error)
{
  int sock = gdb_socket_cloexec (ainfo->ai_family, ainfo->ai_socktype,
				 ainfo->ai_protocol);

  if (sock < 0)
    {
      *out_error = get_error ();
      return -1;
    }

  /* Set socket nonblocking.  */
#ifdef USE_WIN32API
  u_long ioarg = 1;
#else
  int ioarg = 1;
#endif

  ioctl (sock, FIONBIO, &ioarg);

  /* Use Non-blocking connect.  connect() will return 0 if connected
     already.  */
  if (connect (sock, ainfo->ai_addr, ainfo->ai_addrlen) < 0)
    {
      ULONGEST err = get_error ();

      /* If we've got a "connection refused" error, just return
	 -1.  The caller will know what to do.  */
      if (
#ifdef USE_WIN32API
	  err == WSAECONNREFUSED
#else
	  err == ECONNREFUSED
#endif
	  )
	{
	  close (sock);
	  *out_error = err;
	  return -1;
	}

      if (
	  /* Any other error (except EINPROGRESS) will be "swallowed"
	     here.  We return without specifying a return value, and
	     set errno if the caller wants to inspect what
	     happened.  */
#ifdef USE_WIN32API
	  /* Under Windows, calling "connect" with a non-blocking socket
	     results in WSAEWOULDBLOCK, not WSAEINPROGRESS.  */
	  err != WSAEWOULDBLOCK
#else
	  err != EINPROGRESS
#endif
	  )
	{
	  close (sock);
	  *out_error = err;
	  return -1;
	}

      /* Looks like we need to wait for the connect.  */
      int n;

      do
	n = wait_for_connect (sock, polls, out_error);
      while (n == 0);

      if (n < 0)
	{
	  /* A negative value here means that we either timed out or
	     got interrupted by the user.  Just return.  */
	  close (sock);
	  /* OUT_ERROR was set by wait_for_connect, above.  */
	  return -1;
	}
    }

  /* Got something.  Is it an error?  */
  int err;
  socklen_t len = sizeof (err);

  /* On Windows, the fourth parameter to getsockopt is a "char *";
     on UNIX systems it is generally "void *".  The cast to "char *"
     is OK everywhere, since in C++ any data pointer type can be
     implicitly converted to "void *".  */
  int ret = getsockopt (sock, SOL_SOCKET, SO_ERROR, (char *) &err, &len);

  if (ret < 0)
    {
      *out_error = get_error ();
      close (sock);
      return -1;
    }
  else if (ret == 0 && err != 0)
    {
      *out_error = err;
      close (sock);
      return -1;
    }

  /* The connection succeeded.  Return the socket.  */
  return sock;
}

/* Open a tcp socket.  */

void
net_open (struct serial *scb, const char *name)
{
  struct addrinfo hint;
  struct addrinfo *ainfo;

  memset (&hint, 0, sizeof (hint));
  /* Assume no prefix will be passed, therefore we should use
     AF_UNSPEC.  */
  hint.ai_family = AF_UNSPEC;
  hint.ai_socktype = SOCK_STREAM;
  hint.ai_protocol = IPPROTO_TCP;

  parsed_connection_spec parsed = parse_connection_spec (name, &hint);

  if (parsed.port_str.empty ())
    error (_("Missing port on hostname '%s'"), name);

  int r = getaddrinfo (parsed.host_str.c_str (),
		       parsed.port_str.c_str (),
		       &hint, &ainfo);

  if (r != 0)
    error (_("%s: cannot resolve name: %s\n"), name, gai_strerror (r));

  scoped_free_addrinfo free_ainfo (ainfo);

  /* Flag to indicate whether we've got a connection refused.  It will
     be true if any of the connections tried was refused.  */
  bool got_connrefused;
  /* If a connection succeeds, SUCCESS_AINFO will point to the
     'struct addrinfo' that succeed.  */
  struct addrinfo *success_ainfo = NULL;
  unsigned int polls = 0;
  ULONGEST last_error = 0;

  /* Assume the worst.  */
  scb->fd = -1;

  do
    {
      got_connrefused = false;

      for (addrinfo *iter = ainfo; iter != NULL; iter = iter->ai_next)
	{
	  /* Iterate over the list of possible addresses to connect
	     to.  For each, we'll try to connect and see if it
	     succeeds.  */
	  int sock = try_connect (iter, &polls, &last_error);

	  if (sock >= 0)
	    {
	      /* We've gotten a successful connection.  Save its
		 'struct addrinfo', the socket, and break.  */
	      success_ainfo = iter;
	      scb->fd = sock;
	      break;
	    }
	  else if (
#ifdef USE_WIN32API
		   last_error == WSAECONNREFUSED
#else
		   last_error == ECONNREFUSED
#endif
		   )
	    got_connrefused = true;
	}
    }
  /* Just retry if:

     - tcp_auto_retry is true, and
     - We haven't gotten a connection yet, and
     - Any of our connection attempts returned with ECONNREFUSED, and
     - wait_for_connect signals that we can keep going.  */
  while (tcp_auto_retry
	 && success_ainfo == NULL
	 && got_connrefused
	 && wait_for_connect (-1, &polls, &last_error) >= 0);

  if (success_ainfo == NULL)
    {
      net_close (scb);
#ifdef USE_WIN32API
      throw_winerror_with_name (_("could not connect"), last_error);
#else
      perror_with_name (_("could not connect"), last_error);
#endif
    }

  /* Turn off nonblocking.  */
#ifdef USE_WIN32API
  u_long ioarg = 0;
#else
  int ioarg = 0;
#endif

  ioctl (scb->fd, FIONBIO, &ioarg);

  if (success_ainfo->ai_protocol == IPPROTO_TCP)
    {
      /* Disable Nagle algorithm.  Needed in some cases.  */
      int tmp = 1;

      setsockopt (scb->fd, IPPROTO_TCP, TCP_NODELAY,
		  (char *) &tmp, sizeof (tmp));
    }

#ifdef SIGPIPE
  /* If we don't do this, then GDB simply exits
     when the remote side dies.  */
  signal (SIGPIPE, SIG_IGN);
#endif
}

void
net_close (struct serial *scb)
{
  if (scb->fd == -1)
    return;

  close (scb->fd);
  scb->fd = -1;
}

int
net_read_prim (struct serial *scb, size_t count)
{
  /* Need to cast to silence -Wpointer-sign on MinGW, as Winsock's
     'recv' takes 'char *' as second argument, while 'scb->buf' is
     'unsigned char *'.  */
  int result = recv (scb->fd, (char *) scb->buf, count, 0);
  if (result == -1 && errno != EINTR)
    perror_with_name ("error while reading");
  return result;
}

int
net_write_prim (struct serial *scb, const void *buf, size_t count)
{
  /* On Windows, the second parameter to send is a "const char *"; on
     UNIX systems it is generally "const void *".  The cast to "const
     char *" is OK everywhere, since in C++ any data pointer type can
     be implicitly converted to "const void *".  */
  int result = send (scb->fd, (const char *) buf, count, 0);
  if (result == -1 && errno != EINTR)
    perror_with_name ("error while writing");
  return result;
}

void
ser_tcp_send_break (struct serial *scb)
{
  /* Send telnet IAC and BREAK characters.  */
  serial_write (scb, "\377\363", 2);
}

#ifndef USE_WIN32API

/* The TCP ops.  */

static const struct serial_ops tcp_ops =
{
  "tcp",
  net_open,
  net_close,
  NULL,
  ser_base_readchar,
  ser_base_write,
  ser_base_flush_output,
  ser_base_flush_input,
  ser_tcp_send_break,
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
  net_read_prim,
  net_write_prim
};

#endif /* USE_WIN32API */

void _initialize_ser_tcp ();
void
_initialize_ser_tcp ()
{
#ifdef USE_WIN32API
  /* Do nothing; the TCP serial operations will be initialized in
     ser-mingw.c.  */
#else
  serial_add_interface (&tcp_ops);
#endif /* USE_WIN32API */

  add_setshow_prefix_cmd ("tcp", class_maintenance,
			  _("\
TCP protocol specific variables.\n\
Configure variables specific to remote TCP connections."),
			  _("\
TCP protocol specific variables.\n\
Configure variables specific to remote TCP connections."),
			  &tcp_set_cmdlist, &tcp_show_cmdlist,
			  &setlist, &showlist);

  add_setshow_boolean_cmd ("auto-retry", class_obscure,
			   &tcp_auto_retry, _("\
Set auto-retry on socket connect."), _("\
Show auto-retry on socket connect."),
			   NULL, NULL, NULL,
			   &tcp_set_cmdlist, &tcp_show_cmdlist);

  add_setshow_uinteger_cmd ("connect-timeout", class_obscure,
			    &tcp_retry_limit, _("\
Set timeout limit in seconds for socket connection."), _("\
Show timeout limit in seconds for socket connection."), _("\
If set to \"unlimited\", GDB will keep attempting to establish a\n\
connection forever, unless interrupted with Ctrl-c.\n\
The default is 15 seconds."),
			    NULL, NULL,
			    &tcp_set_cmdlist, &tcp_show_cmdlist);
}
