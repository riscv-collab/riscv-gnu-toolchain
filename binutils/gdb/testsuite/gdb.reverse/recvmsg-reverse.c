/* This testcase is part of GDB, the GNU debugger.

   Copyright 2008-2024 Free Software Foundation, Inc.

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

#define _GNU_SOURCE
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

void
marker1 (void)
{
}

void
marker2 (void)
{
}

char wdata[] = "abcdef";

struct iovec wv[1] = {
  { wdata, 6 },
};

char wc[CMSG_SPACE (sizeof (struct ucred)) + CMSG_SPACE (sizeof (int))];

struct msghdr wmsg = {
  0, 0,
  wv, 1,
  wc, sizeof wc,
  0,
};

char rdata[5] = "xxxx";

struct iovec rv[2] = {
  {&rdata[2], 2},
  {&rdata[0], 2},
};

char rc[CMSG_SPACE (sizeof (struct ucred)) + 7];

struct msghdr rmsg = {
  0, 0,
  rv, 2,
  rc, sizeof rc,
  0,
};

int fds[2];

int
main (void)
{
  int itrue = 1;
  /* prepare cmsg to send */
  struct cmsghdr *cm1 = CMSG_FIRSTHDR (&wmsg);
  cm1->cmsg_len = CMSG_LEN (sizeof (struct ucred));
  cm1->cmsg_level = AF_UNIX;
  cm1->cmsg_type = SCM_CREDENTIALS;
  struct ucred *uc = (void *) CMSG_DATA (cm1);
  uc->pid = getpid ();
  uc->uid = getuid ();
  uc->gid = getgid ();
  struct cmsghdr *cm2 = CMSG_NXTHDR (&wmsg, cm1);
  cm2->cmsg_len = CMSG_LEN (sizeof (int));
  cm2->cmsg_level = AF_UNIX;
  cm2->cmsg_type = SCM_RIGHTS;
  int *pfd = (void *) CMSG_DATA (cm2);
  *pfd = 2;
  /* do the syscalls */
  marker1 ();
  socketpair (AF_UNIX, SOCK_DGRAM, 0, fds);
  setsockopt (fds[0], SOL_SOCKET, SO_PASSCRED, &itrue, sizeof itrue);
  sendmsg (fds[1], &wmsg, 0);
  recvmsg (fds[0], &rmsg, 0);
  marker2 ();
  return 0;
}
