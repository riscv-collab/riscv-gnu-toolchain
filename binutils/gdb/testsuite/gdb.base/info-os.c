/* This testcase is part of GDB, the GNU debugger.

   Copyright 2011-2024 Free Software Foundation, Inc.

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

#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <stdio.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/* System V IPC identifiers.  */
static int shmid = -1, semid = -1, msqid = -1;

/* Delete any System V IPC resources that were allocated.  */

static void
ipc_cleanup (void)
{
  if (shmid >= 0)
    shmctl (shmid, IPC_RMID, NULL);
  if (semid >= 0)
    semctl (semid, 0, IPC_RMID, NULL);
  if (msqid >= 0)
    msgctl (msqid, IPC_RMID, NULL);
}

void *
thread_proc (void *args)
{
  pthread_mutex_lock (&mutex);
  pthread_mutex_unlock (&mutex);

  return NULL;
}

int
main (void)
{
  const int flags = IPC_CREAT | 0666;
  key_t shmkey = 3925, semkey = 7428, msgkey = 5294;
  FILE *fd;
  pthread_t thread;
  struct sockaddr_in sock_addr;
  int sock;
  unsigned short port;
  socklen_t size;
  int status, try, retries = 1000;

  atexit (ipc_cleanup);

  for (try = 0; try < retries; ++try)
    {
      shmid = shmget (shmkey, 4096, flags | IPC_EXCL);
      if (shmid >= 0)
	break;

      ++shmkey;
    }

  if (shmid < 0)
    {
      printf ("Cannot create shared-memory region after %d tries.\n", retries);
      return 1;
    }

  for (try = 0; try < retries; ++try)
    {
      semid = semget (semkey, 1, flags | IPC_EXCL);
      if (semid >= 0)
	break;

      ++semkey;
    }

  if (semid < 0)
    {
      printf ("Cannot create semaphore after %d tries.\n", retries);
      return 1;
    }

  for (try = 0; try < retries; ++try)
    {
      msqid = msgget (msgkey, flags | IPC_EXCL);
      if (msqid >= 0)
	break;

      ++msgkey;
    }

  if (msqid < 0)
    {
      printf ("Cannot create message queue after %d tries.\n", retries);
      return 1;
    }

  fd = fopen ("/dev/null", "r");

  /* Lock the mutex to prevent the new thread from finishing immediately.  */
  pthread_mutex_lock (&mutex);
  pthread_create (&thread, NULL, thread_proc, 0);
 
  sock = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock < 0)
    {
      printf ("Cannot create socket.\n");
      return 1;
    }
 
  sock_addr.sin_family = AF_INET;
  sock_addr.sin_port = 0; /* Bind to a free port.  */
  sock_addr.sin_addr.s_addr = htonl (INADDR_ANY);

  status = bind (sock, (struct sockaddr *) &sock_addr, sizeof (sock_addr));
  if (status < 0)
    {
      printf ("Cannot bind socket.\n");
      return 1;
    }

  /* Find the assigned port number of the socket.  */
  size = sizeof (sock_addr);
  status = getsockname (sock, (struct sockaddr *) &sock_addr, &size);
  if (status < 0)
    {
      printf ("Cannot find name of socket.\n");
      return 1;
    }
  port = ntohs (sock_addr.sin_port);

  status = listen (sock, 1);
  if (status < 0)
    {
      printf ("Cannot listen on socket.\n");
      return 1;
    }

  /* Set breakpoint here.  */

  fclose (fd);
  close (sock);

  pthread_mutex_unlock (&mutex);
  pthread_join (thread, NULL);

  return 0;
}
