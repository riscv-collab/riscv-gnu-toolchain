/* Linux namespaces(7) support.

   Copyright (C) 2015-2024 Free Software Foundation, Inc.

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
#include "nat/linux-namespaces.h"
#include "gdbsupport/filestuff.h"
#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include "gdbsupport/gdb_wait.h"
#include <signal.h>
#include <sched.h>
#include "gdbsupport/scope-exit.h"

/* See nat/linux-namespaces.h.  */
bool debug_linux_namespaces;

/* Handle systems without fork.  */

static inline pid_t
do_fork (void)
{
#ifdef HAVE_FORK
  return fork ();
#else
  errno = ENOSYS;
  return -1;
#endif
}

/* Handle systems without setns.  */

static inline int
do_setns (int fd, int nstype)
{
#ifdef HAVE_SETNS
  return setns (fd, nstype);
#elif defined __NR_setns
  return syscall (__NR_setns, fd, nstype);
#else
  errno = ENOSYS;
  return -1;
#endif
}

/* Handle systems without MSG_CMSG_CLOEXEC.  */

#ifndef MSG_CMSG_CLOEXEC
#define MSG_CMSG_CLOEXEC 0
#endif

/* A Linux namespace.  */

struct linux_ns
{
  /* Filename of this namespace's entries in /proc/PID/ns.  */
  const char *filename;

  /* Nonzero if this object has been initialized.  */
  int initialized;

  /* Nonzero if this namespace is supported on this system.  */
  int supported;

  /* ID of the namespace the calling process is in, used to
     see if other processes share the namespace.  The code in
     this file assumes that the calling process never changes
     namespace.  */
  ino_t id;
};

/* Return the absolute filename of process PID's /proc/PID/ns
   entry for namespace NS.  The returned value persists until
   this function is next called.  */

static const char *
linux_ns_filename (struct linux_ns *ns, int pid)
{
  static char filename[PATH_MAX];

  gdb_assert (pid > 0);
  xsnprintf (filename, sizeof (filename), "/proc/%d/ns/%s", pid,
	     ns->filename);

  return filename;
}

/* Return a representation of the caller's TYPE namespace, or
   NULL if TYPE namespaces are not supported on this system.  */

static struct linux_ns *
linux_ns_get_namespace (enum linux_ns_type type)
{
  static struct linux_ns namespaces[NUM_LINUX_NS_TYPES] =
    {
      { "ipc" },
      { "mnt" },
      { "net" },
      { "pid" },
      { "user" },
      { "uts" },
    };
  struct linux_ns *ns;

  gdb_assert (type >= 0 && type < NUM_LINUX_NS_TYPES);
  ns = &namespaces[type];

  if (!ns->initialized)
    {
      struct stat sb;

      if (stat (linux_ns_filename (ns, getpid ()), &sb) == 0)
	{
	  ns->id = sb.st_ino;

	  ns->supported = 1;
	}

      ns->initialized = 1;
    }

  return ns->supported ? ns : NULL;
}

/* See nat/linux-namespaces.h.  */

int
linux_ns_same (pid_t pid, enum linux_ns_type type)
{
  struct linux_ns *ns = linux_ns_get_namespace (type);
  const char *filename;
  struct stat sb;

  /* If the kernel does not support TYPE namespaces then there's
     effectively only one TYPE namespace that all processes on
     the system share.  */
  if (ns == NULL)
    return 1;

  /* Stat PID's TYPE namespace entry to get the namespace ID.  This
     might fail if the process died, or if we don't have the right
     permissions (though we should be attached by this time so this
     seems unlikely).  In any event, we can't make any decisions and
     must throw.  */
  filename = linux_ns_filename (ns, pid);
  if (stat (filename, &sb) != 0)
    perror_with_name (filename);

  return sb.st_ino == ns->id;
}

/* We need to use setns(2) to handle filesystem access in mount
   namespaces other than our own, but this isn't permitted for
   multithreaded processes.  GDB is multithreaded when compiled
   with Guile support, and may become multithreaded if compiled
   with Python support.  We deal with this by spawning a single-
   threaded helper process to access mount namespaces other than
   our own.

   The helper process is started the first time a call to setns
   is required.  The main process (GDB or gdbserver) communicates
   with the helper via sockets, passing file descriptors where
   necessary using SCM_RIGHTS.  Once started the helper process
   runs until the main process terminates; when this happens the
   helper will receive socket errors, notice that its parent died,
   and exit accordingly (see mnsh_maybe_mourn_peer).

   The protocol is that the main process sends a request in a
   single message, and the helper replies to every message it
   receives with a single-message response.  If the helper
   receives a message it does not understand it will reply with
   a MNSH_MSG_ERROR message.  The main process checks all
   responses it receives with gdb_assert, so if the main process
   receives something unexpected (which includes MNSH_MSG_ERROR)
   the main process will call internal_error.

   For avoidance of doubt, if the helper process receives a
   message it doesn't handle it will reply with MNSH_MSG_ERROR.
   If the main process receives MNSH_MSG_ERROR at any time then
   it will call internal_error.  If internal_error causes the
   main process to exit, the helper will notice this and also
   exit.  The helper will not exit until the main process
   terminates, so if the user continues through internal_error
   the helper will still be there awaiting requests from the
   main process.

   Messages in both directions have the following payload:

   - TYPE (enum mnsh_msg_type, always sent) - the message type.
   - INT1 and
   - INT2 (int, always sent, though not always used) - two
	   values whose meaning is message-type-dependent.
	   See enum mnsh_msg_type documentation below.
   - FD (int, optional, sent using SCM_RIGHTS) - an open file
	 descriptor.
   - BUF (unstructured data, optional) - some data with message-
	  type-dependent meaning.

   Note that the helper process is the child of a call to fork,
   so all code in the helper must be async-signal-safe.  */

/* Mount namespace helper message types.  */

enum mnsh_msg_type
  {
    /* A communication error occurred.  Receipt of this message
       by either end will cause an assertion failure in the main
       process.  */
    MNSH_MSG_ERROR,

    /* Requests, sent from the main process to the helper.  */

    /* A request that the helper call setns.  Arguments should
       be passed in FD and INT1.  Helper should respond with a
       MNSH_RET_INT.  */
    MNSH_REQ_SETNS,

    /* A request that the helper call open.  Arguments should
       be passed in BUF, INT1 and INT2.  The filename (in BUF)
       should include a terminating NUL character.  The helper
       should respond with a MNSH_RET_FD.  */
    MNSH_REQ_OPEN,

    /* A request that the helper call unlink.  The single
       argument (the filename) should be passed in BUF, and
       should include a terminating NUL character.  The helper
       should respond with a MNSH_RET_INT.  */
    MNSH_REQ_UNLINK,

    /* A request that the helper call readlink.  The single
       argument (the filename) should be passed in BUF, and
       should include a terminating NUL character.  The helper
       should respond with a MNSH_RET_INTSTR.  */
    MNSH_REQ_READLINK,

    /* Responses, sent to the main process from the helper.  */

    /* Return an integer in INT1 and errno in INT2.  */
    MNSH_RET_INT,

    /* Return a file descriptor in FD if one was opened or an
       integer in INT1 otherwise.  Return errno in INT2.  */
    MNSH_RET_FD,

    /* Return an integer in INT1, errno in INT2, and optionally
       some data in BUF.  */
    MNSH_RET_INTSTR,
  };

/* Print a string representation of a message using debug_printf.
   This function is not async-signal-safe so should never be
   called from the helper.  */

static void
mnsh_debug_print_message (enum mnsh_msg_type type,
			  int fd, int int1, int int2,
			  const void *buf, int bufsiz)
{
  gdb_byte *c = (gdb_byte *) buf;
  gdb_byte *cl = c + bufsiz;

  switch (type)
    {
    case MNSH_MSG_ERROR:
      debug_printf ("ERROR");
      break;

    case MNSH_REQ_SETNS:
      debug_printf ("SETNS");
      break;

    case MNSH_REQ_OPEN:
      debug_printf ("OPEN");
      break;

    case MNSH_REQ_UNLINK:
      debug_printf ("UNLINK");
      break;

    case MNSH_REQ_READLINK:
      debug_printf ("READLINK");
      break;

    case MNSH_RET_INT:
      debug_printf ("INT");
      break;

    case MNSH_RET_FD:
      debug_printf ("FD");
      break;

    case MNSH_RET_INTSTR:
      debug_printf ("INTSTR");
      break;

    default:
      debug_printf ("unknown-packet-%d", type);
    }

  debug_printf (" %d %d %d \"", fd, int1, int2);

  for (; c < cl; c++)
    debug_printf (*c >= ' ' && *c <= '~' ? "%c" : "\\%o", *c);

  debug_printf ("\"");
}

/* Forward declaration.  */

static void mnsh_maybe_mourn_peer (void);

/* Send a message.  The argument SOCK is the file descriptor of the
   sending socket, the other arguments are the payload to send.
   Return the number of bytes sent on success.  Return -1 on failure
   and set errno appropriately.  This function is called by both the
   main process and the helper so must be async-signal-safe.  */

static ssize_t
mnsh_send_message (int sock, enum mnsh_msg_type type,
		   int fd, int int1, int int2,
		   const void *buf, int bufsiz)
{
  struct msghdr msg;
  struct iovec iov[4];
  char fdbuf[CMSG_SPACE (sizeof (fd))];
  ssize_t size;

  /* Build the basic TYPE, INT1, INT2 message.  */
  memset (&msg, 0, sizeof (msg));
  msg.msg_iov = iov;

  iov[0].iov_base = &type;
  iov[0].iov_len = sizeof (type);
  iov[1].iov_base = &int1;
  iov[1].iov_len = sizeof (int1);
  iov[2].iov_base = &int2;
  iov[2].iov_len = sizeof (int2);

  msg.msg_iovlen = 3;

  /* Append BUF if supplied.  */
  if (buf != NULL && bufsiz > 0)
    {
      iov[3].iov_base = alloca (bufsiz);
      memcpy (iov[3].iov_base, buf, bufsiz);
      iov[3].iov_len = bufsiz;

      msg.msg_iovlen ++;
    }

  /* Attach FD if supplied.  */
  if (fd >= 0)
    {
      struct cmsghdr *cmsg;

      msg.msg_control = fdbuf;
      msg.msg_controllen = sizeof (fdbuf);

      cmsg = CMSG_FIRSTHDR (&msg);
      cmsg->cmsg_level = SOL_SOCKET;
      cmsg->cmsg_type = SCM_RIGHTS;
      cmsg->cmsg_len = CMSG_LEN (sizeof (int));

      memcpy (CMSG_DATA (cmsg), &fd, sizeof (int));

      msg.msg_controllen = cmsg->cmsg_len;
    }

  /* Send the message.  */
  size = sendmsg (sock, &msg, 0);

  if (size < 0)
    mnsh_maybe_mourn_peer ();

  if (debug_linux_namespaces)
    {
      debug_printf ("mnsh: send: ");
      mnsh_debug_print_message (type, fd, int1, int2, buf, bufsiz);
      debug_printf (" -> %s\n", pulongest (size));
    }

  return size;
}

/* Receive a message.  The argument SOCK is the file descriptor of
   the receiving socket, the other arguments point to storage for
   the received payload.  Returns the number of bytes stored into
   BUF on success, which may be zero in the event no BUF was sent.
   Return -1 on failure and set errno appropriately.  This function
   is called from both the main process and the helper and must be
   async-signal-safe.  */

static ssize_t
mnsh_recv_message (int sock, enum mnsh_msg_type *type,
		   int *fd, int *int1, int *int2,
		   void *buf, int bufsiz)
{
  struct msghdr msg;
  struct iovec iov[4];
  char fdbuf[CMSG_SPACE (sizeof (*fd))];
  struct cmsghdr *cmsg;
  ssize_t size, fixed_size;
  int i;

  /* Build the message to receive data into.  */
  memset (&msg, 0, sizeof (msg));
  msg.msg_iov = iov;

  iov[0].iov_base = type;
  iov[0].iov_len = sizeof (*type);
  iov[1].iov_base = int1;
  iov[1].iov_len = sizeof (*int1);
  iov[2].iov_base = int2;
  iov[2].iov_len = sizeof (*int2);
  iov[3].iov_base = buf;
  iov[3].iov_len = bufsiz;

  msg.msg_iovlen = 4;

  for (fixed_size = i = 0; i < msg.msg_iovlen - 1; i++)
    fixed_size += iov[i].iov_len;

  msg.msg_control = fdbuf;
  msg.msg_controllen = sizeof (fdbuf);

  /* Receive the message.  */
  size = recvmsg (sock, &msg, MSG_CMSG_CLOEXEC);
  if (size < 0)
    {
      if (debug_linux_namespaces)
	debug_printf ("namespace-helper: recv failed (%s)\n",
		      pulongest (size));

      mnsh_maybe_mourn_peer ();

      return size;
    }

  /* Check for truncation.  */
  if (size < fixed_size || (msg.msg_flags & (MSG_TRUNC | MSG_CTRUNC)))
    {
      if (debug_linux_namespaces)
	debug_printf ("namespace-helper: recv truncated (%s 0x%x)\n",
		      pulongest (size), msg.msg_flags);

      mnsh_maybe_mourn_peer ();

      errno = EBADMSG;
      return -1;
    }

  /* Unpack the file descriptor if supplied.  */
  cmsg = CMSG_FIRSTHDR (&msg);
  if (cmsg != NULL
      && cmsg->cmsg_len == CMSG_LEN (sizeof (int))
      && cmsg->cmsg_level == SOL_SOCKET
      && cmsg->cmsg_type == SCM_RIGHTS)
    memcpy (fd, CMSG_DATA (cmsg), sizeof (int));
  else
    *fd = -1;

  if (debug_linux_namespaces)
    {
      debug_printf ("mnsh: recv: ");
      mnsh_debug_print_message (*type, *fd, *int1, *int2, buf,
				size - fixed_size);
      debug_printf ("\n");
    }

  /* Return the number of bytes of data in BUF.  */
  return size - fixed_size;
}

/* Shortcuts for returning results from the helper.  */

#define mnsh_return_int(sock, result, error) \
  mnsh_send_message (sock, MNSH_RET_INT, -1, result, error, NULL, 0)

#define mnsh_return_fd(sock, fd, error) \
  mnsh_send_message (sock, MNSH_RET_FD, \
		     (fd) < 0 ? -1 : (fd), \
		     (fd) < 0 ? (fd) : 0, \
		     error, NULL, 0)

#define mnsh_return_intstr(sock, result, buf, bufsiz, error) \
  mnsh_send_message (sock, MNSH_RET_INTSTR, -1, result, error, \
		     buf, bufsiz)

/* Handle a MNSH_REQ_SETNS message.  Must be async-signal-safe.  */

static ssize_t
mnsh_handle_setns (int sock, int fd, int nstype)
{
  int result = do_setns (fd, nstype);

  return mnsh_return_int (sock, result, errno);
}

/* Handle a MNSH_REQ_OPEN message.  Must be async-signal-safe.  */

static ssize_t
mnsh_handle_open (int sock, const char *filename,
		  int flags, mode_t mode)
{
  scoped_fd fd = gdb_open_cloexec (filename, flags, mode);
  return mnsh_return_fd (sock, fd.get (), errno);
}

/* Handle a MNSH_REQ_UNLINK message.  Must be async-signal-safe.  */

static ssize_t
mnsh_handle_unlink (int sock, const char *filename)
{
  int result = unlink (filename);

  return mnsh_return_int (sock, result, errno);
}

/* Handle a MNSH_REQ_READLINK message.  Must be async-signal-safe.  */

static ssize_t
mnsh_handle_readlink (int sock, const char *filename)
{
  char buf[PATH_MAX];
  int len = readlink (filename, buf, sizeof (buf));

  return mnsh_return_intstr (sock, len,
			     buf, len < 0 ? 0 : len,
			     errno);
}

/* The helper process.  Never returns.  Must be async-signal-safe.  */

static void mnsh_main (int sock) ATTRIBUTE_NORETURN;

static void
mnsh_main (int sock)
{
  while (1)
    {
      enum mnsh_msg_type type;
      int fd = -1, int1, int2;
      char buf[PATH_MAX];
      ssize_t size, response = -1;

      size = mnsh_recv_message (sock, &type,
				&fd, &int1, &int2,
				buf, sizeof (buf));

      if (size >= 0 && size < sizeof (buf))
	{
	  switch (type)
	    {
	    case MNSH_REQ_SETNS:
	      if (fd > 0)
		response = mnsh_handle_setns (sock, fd, int1);
	      break;

	    case MNSH_REQ_OPEN:
	      if (size > 0 && buf[size - 1] == '\0')
		response = mnsh_handle_open (sock, buf, int1, int2);
	      break;

	    case MNSH_REQ_UNLINK:
	      if (size > 0 && buf[size - 1] == '\0')
		response = mnsh_handle_unlink (sock, buf);
	      break;

	    case MNSH_REQ_READLINK:
	      if (size > 0 && buf[size - 1] == '\0')
		response = mnsh_handle_readlink (sock, buf);
	      break;

	    default:
	      break; /* Handled below.  */
	    }
	}

      /* Close any file descriptors we were passed.  */
      if (fd >= 0)
	close (fd);

      /* Can't handle this message, bounce it back.  */
      if (response < 0)
	{
	  if (size < 0)
	    size = 0;

	  mnsh_send_message (sock, MNSH_MSG_ERROR,
			     -1, int1, int2, buf, size);
	}
    }
}

/* The mount namespace helper process.  */

struct linux_mnsh
{
  /* PID of helper.  */
  pid_t pid;

  /* Socket for communication.  */
  int sock;

  /* ID of the mount namespace the helper is currently in.  */
  ino_t nsid;
};

/* In the helper process this is set to the PID of the process that
   created the helper (i.e. GDB or gdbserver).  In the main process
   this is set to zero.  Used by mnsh_maybe_mourn_peer.  */
static int mnsh_creator_pid = 0;

/* Return an object representing the mount namespace helper process.
   If no mount namespace helper process has been started then start
   one.  Return NULL if no mount namespace helper process could be
   started.  */

static struct linux_mnsh *
linux_mntns_get_helper (void)
{
  static struct linux_mnsh *helper = NULL;

  if (helper == NULL)
    {
      static struct linux_mnsh h;
      struct linux_ns *ns;
      pid_t helper_creator = getpid ();
      int sv[2];

      ns = linux_ns_get_namespace (LINUX_NS_MNT);
      if (ns == NULL)
	return NULL;

      if (gdb_socketpair_cloexec (AF_UNIX, SOCK_STREAM, 0, sv) < 0)
	return NULL;

      h.pid = do_fork ();
      if (h.pid < 0)
	{
	  int saved_errno = errno;

	  close (sv[0]);
	  close (sv[1]);

	  errno = saved_errno;
	  return NULL;
	}

      if (h.pid == 0)
	{
	  /* Child process.  */
	  close (sv[0]);

	  mnsh_creator_pid = helper_creator;

	  /* Debug printing isn't async-signal-safe.  */
	  debug_linux_namespaces = 0;

	  mnsh_main (sv[1]);
	}

      /* Parent process.  */
      close (sv[1]);

      helper = &h;
      helper->sock = sv[0];
      helper->nsid = ns->id;

      if (debug_linux_namespaces)
	debug_printf ("Started mount namespace helper process %d\n",
		      helper->pid);
    }

  return helper;
}

/* Check whether the other process died and act accordingly.  Called
   whenever a socket error occurs, from both the main process and the
   helper.  Must be async-signal-safe when called from the helper.  */

static void
mnsh_maybe_mourn_peer (void)
{
  if (mnsh_creator_pid != 0)
    {
      /* We're in the helper.  Check if our current parent is the
	 process that started us.  If it isn't, then our original
	 parent died and we've been reparented.  Exit immediately
	 if that's the case.  */
      if (getppid () != mnsh_creator_pid)
	_exit (0);
    }
  else
    {
      /* We're in the main process.  */

      struct linux_mnsh *helper = linux_mntns_get_helper ();
      int status;
      pid_t pid;

      if (helper->pid < 0)
	{
	  /* We already mourned it.  */
	  return;
	}

      pid = waitpid (helper->pid, &status, WNOHANG);
      if (pid == 0)
	{
	  /* The helper is still alive.  */
	  return;
	}
      else if (pid == -1)
	{
	  if (errno == ECHILD)
	    warning (_("mount namespace helper vanished?"));
	  else
	    internal_warning (_("unhandled error %d"), errno);
	}
      else if (pid == helper->pid)
	{
	  if (WIFEXITED (status))
	    warning (_("mount namespace helper exited with status %d"),
		     WEXITSTATUS (status));
	  else if (WIFSIGNALED (status))
	    warning (_("mount namespace helper killed by signal %d"),
		     WTERMSIG (status));
	  else
	    internal_warning (_("unhandled status %d"), status);
	}
      else
	internal_warning (_("unknown pid %d"), pid);

      /* Something unrecoverable happened.  */
      helper->pid = -1;
    }
}

/* Shortcuts for sending messages to the helper.  */

#define mnsh_send_setns(helper, fd, nstype) \
  mnsh_send_message (helper->sock, MNSH_REQ_SETNS, fd, nstype, 0, \
		     NULL, 0)

#define mnsh_send_open(helper, filename, flags, mode) \
  mnsh_send_message (helper->sock, MNSH_REQ_OPEN, -1, flags, mode, \
		     filename, strlen (filename) + 1)

#define mnsh_send_unlink(helper, filename) \
  mnsh_send_message (helper->sock, MNSH_REQ_UNLINK, -1, 0, 0, \
		     filename, strlen (filename) + 1)

#define mnsh_send_readlink(helper, filename) \
  mnsh_send_message (helper->sock, MNSH_REQ_READLINK, -1, 0, 0, \
		     filename, strlen (filename) + 1)

/* Receive a message from the helper.  Issue an assertion failure if
   the message isn't a correctly-formatted MNSH_RET_INT.  Set RESULT
   and ERROR and return 0 on success.  Set errno and return -1 on
   failure.  */

static int
mnsh_recv_int (struct linux_mnsh *helper, int *result, int *error)
{
  enum mnsh_msg_type type;
  char buf[PATH_MAX];
  ssize_t size;
  int fd;

  size = mnsh_recv_message (helper->sock, &type, &fd,
			    result, error,
			    buf, sizeof (buf));
  if (size < 0)
    return -1;

  gdb_assert (type == MNSH_RET_INT);
  gdb_assert (fd == -1);
  gdb_assert (size == 0);

  return 0;
}

/* Receive a message from the helper.  Issue an assertion failure if
   the message isn't a correctly-formatted MNSH_RET_FD.  Set FD and
   ERROR and return 0 on success.  Set errno and return -1 on
   failure.  */

static int
mnsh_recv_fd (struct linux_mnsh *helper, int *fd, int *error)
{
  enum mnsh_msg_type type;
  char buf[PATH_MAX];
  ssize_t size;
  int result;

  size = mnsh_recv_message (helper->sock, &type, fd,
			    &result, error,
			    buf, sizeof (buf));
  if (size < 0)
    return -1;

  gdb_assert (type == MNSH_RET_FD);
  gdb_assert (size == 0);

  if (*fd < 0)
    {
      gdb_assert (result < 0);
      *fd = result;
    }

  return 0;
}

/* Receive a message from the helper.  Issue an assertion failure if
   the message isn't a correctly-formatted MNSH_RET_INTSTR.  Set
   RESULT and ERROR and optionally store data in BUF, then return
   the number of bytes stored in BUF on success (this may be zero).
   Set errno and return -1 on error.  */

static ssize_t
mnsh_recv_intstr (struct linux_mnsh *helper,
		  int *result, int *error,
		  void *buf, int bufsiz)
{
  enum mnsh_msg_type type;
  ssize_t size;
  int fd;

  size = mnsh_recv_message (helper->sock, &type, &fd,
			    result, error,
			    buf, bufsiz);

  if (size < 0)
    return -1;

  gdb_assert (type == MNSH_RET_INTSTR);
  gdb_assert (fd == -1);

  return size;
}

/* Return values for linux_mntns_access_fs.  */

enum mnsh_fs_code
  {
    /* Something went wrong, errno is set.  */
    MNSH_FS_ERROR = -1,

    /* The main process is in the correct mount namespace.
       The caller should access the filesystem directly.  */
    MNSH_FS_DIRECT,

    /* The helper is in the correct mount namespace.
       The caller should access the filesystem via the helper.  */
    MNSH_FS_HELPER
  };

/* Return a value indicating how the caller should access the
   mount namespace of process PID.  */

static enum mnsh_fs_code
linux_mntns_access_fs (pid_t pid)
{
  struct linux_ns *ns;
  struct stat sb;
  struct linux_mnsh *helper;
  ssize_t size;
  int fd;

  if (pid == getpid ())
    return MNSH_FS_DIRECT;

  ns = linux_ns_get_namespace (LINUX_NS_MNT);
  if (ns == NULL)
    return MNSH_FS_DIRECT;

  fd = gdb_open_cloexec (linux_ns_filename (ns, pid), O_RDONLY, 0).release ();
  if (fd < 0)
    return MNSH_FS_ERROR;

  SCOPE_EXIT
    {
      int save_errno = errno;
      close (fd);
      errno = save_errno;
    };

  if (fstat (fd, &sb) != 0)
    return MNSH_FS_ERROR;

  if (sb.st_ino == ns->id)
    return MNSH_FS_DIRECT;

  helper = linux_mntns_get_helper ();
  if (helper == NULL)
    return MNSH_FS_ERROR;

  if (sb.st_ino != helper->nsid)
    {
      int result, error;

      size = mnsh_send_setns (helper, fd, 0);
      if (size < 0)
	return MNSH_FS_ERROR;

      if (mnsh_recv_int (helper, &result, &error) != 0)
	return MNSH_FS_ERROR;

      if (result != 0)
	{
	  /* ENOSYS indicates that an entire function is unsupported
	     (it's not appropriate for our versions of open/unlink/
	     readlink to sometimes return with ENOSYS depending on how
	     they're called) so we convert ENOSYS to ENOTSUP if setns
	     fails.  */
	  if (error == ENOSYS)
	    error = ENOTSUP;

	  errno = error;
	  return MNSH_FS_ERROR;
	}

      helper->nsid = sb.st_ino;
    }

  return MNSH_FS_HELPER;
}

/* See nat/linux-namespaces.h.  */

int
linux_mntns_open_cloexec (pid_t pid, const char *filename,
			  int flags, mode_t mode)
{
  enum mnsh_fs_code access = linux_mntns_access_fs (pid);
  struct linux_mnsh *helper;
  int fd, error;
  ssize_t size;

  if (access == MNSH_FS_ERROR)
    return -1;

  if (access == MNSH_FS_DIRECT)
    return gdb_open_cloexec (filename, flags, mode).release ();

  gdb_assert (access == MNSH_FS_HELPER);

  helper = linux_mntns_get_helper ();

  size = mnsh_send_open (helper, filename, flags, mode);
  if (size < 0)
    return -1;

  if (mnsh_recv_fd (helper, &fd, &error) != 0)
    return -1;

  if (fd < 0)
    errno = error;

  return fd;
}

/* See nat/linux-namespaces.h.  */

int
linux_mntns_unlink (pid_t pid, const char *filename)
{
  enum mnsh_fs_code access = linux_mntns_access_fs (pid);
  struct linux_mnsh *helper;
  int ret, error;
  ssize_t size;

  if (access == MNSH_FS_ERROR)
    return -1;

  if (access == MNSH_FS_DIRECT)
    return unlink (filename);

  gdb_assert (access == MNSH_FS_HELPER);

  helper = linux_mntns_get_helper ();

  size = mnsh_send_unlink (helper, filename);
  if (size < 0)
    return -1;

  if (mnsh_recv_int (helper, &ret, &error) != 0)
    return -1;

  if (ret != 0)
    errno = error;

  return ret;
}

/* See nat/linux-namespaces.h.  */

ssize_t
linux_mntns_readlink (pid_t pid, const char *filename,
		      char *buf, size_t bufsiz)
{
  enum mnsh_fs_code access = linux_mntns_access_fs (pid);
  struct linux_mnsh *helper;
  int ret, error;
  ssize_t size;

  if (access == MNSH_FS_ERROR)
    return -1;

  if (access == MNSH_FS_DIRECT)
    return readlink (filename, buf, bufsiz);

  gdb_assert (access == MNSH_FS_HELPER);

  helper = linux_mntns_get_helper ();

  size = mnsh_send_readlink (helper, filename);
  if (size < 0)
    return -1;

  size = mnsh_recv_intstr (helper, &ret, &error, buf, bufsiz);

  if (size < 0)
    {
      ret = -1;
      errno = error;
    }
  else
    gdb_assert (size == ret);

  return ret;
}
