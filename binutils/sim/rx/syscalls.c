/* syscalls.c --- implement system calls for the RX simulator.

Copyright (C) 2005-2024 Free Software Foundation, Inc.
Contributed by Red Hat, Inc.

This file is part of the GNU simulators.

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

/* This must come before any other includes.  */
#include "defs.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>

#include "sim/callback.h"

#include "cpu.h"
#include "mem.h"
#include "syscalls.h"
#include "target-newlib-syscall.h"

/* The current syscall callbacks we're using.  */
static struct host_callback_struct *callbacks;

void
set_callbacks (struct host_callback_struct *cb)
{
  callbacks = cb;
}

struct host_callback_struct *
get_callbacks (void)
{
  return callbacks;
}


/* Arguments 1..4 are in R1..R4, remainder on stack.

   Return value in R1..R4 as needed.
     structs bigger than 16 bytes: pointer pushed on stack last

   We only support arguments that fit in general registers.

   The system call number is in R5.  We expect ssycalls to look like
   this in libgloss:

   _exit:
   	mov	#SYS_exit, r5
	int	#255
	rts
*/

int argp, stackp;

static int
arg (void)
{
  int rv = 0;
  argp++;

  if (argp < 4)
    return get_reg (argp);

  rv = mem_get_si (get_reg (sp) + stackp);
  stackp += 4;
  return rv;
}

static void
read_target (char *buffer, int address, int count, int asciiz)
{
  char byte;
  while (count > 0)
    {
      byte = mem_get_qi (address++);
      *buffer++ = byte;
      if (asciiz && (byte == 0))
	return;
      count--;
    }
}

static void
write_target (char *buffer, int address, int count, int asciiz)
{
  char byte;
  while (count > 0)
    {
      byte = *buffer++;
      mem_put_qi (address++, byte);
      if (asciiz && (byte == 0))
	return;
      count--;
    }
}

#define PTRSZ (A16 ? 2 : 3)

static char *callnames[] = {
  "SYS_zero",
  "SYS_exit",
  "SYS_open",
  "SYS_close",
  "SYS_read",
  "SYS_write",
  "SYS_lseek",
  "SYS_unlink",
  "SYS_getpid",
  "SYS_kill",
  "SYS_fstat",
  "SYS_sbrk",
  "SYS_argvlen",
  "SYS_argv",
  "SYS_chdir",
  "SYS_stat",
  "SYS_chmod",
  "SYS_utime",
  "SYS_time",
  "SYS_gettimeofday",
  "SYS_times",
  "SYS_link"
};

int
rx_syscall (int id)
{
  static char buf[256];
  int rv;

  argp = 0;
  stackp = 4;
  if (trace)
    printf ("\033[31m/* SYSCALL(%d) = %s */\033[0m\n", id, id <= TARGET_NEWLIB_SYS_link ? callnames[id] : "unknown");
  switch (id)
    {
    case TARGET_NEWLIB_SYS_exit:
      {
	int ec = arg ();
	if (verbose)
	  printf ("[exit %d]\n", ec);
	return RX_MAKE_EXITED (ec);
      }
      break;

    case TARGET_NEWLIB_SYS_open:
      {
	int oflags, cflags;
	int path = arg ();
	/* The open function is defined as taking a variable number of arguments
	   because the third parameter to it is optional:
	     open (const char * filename, int flags, ...);
	   Hence the oflags and cflags arguments will be on the stack and we need
	   to skip the (empty) argument registers r3 and r4.  */
	argp = 4;
	oflags = arg ();
	cflags = arg ();

	read_target (buf, path, 256, 1);
	if (trace)
	  printf ("open(\"%s\",0x%x,%#o) = ", buf, oflags, cflags);

	if (callbacks)
	  /* The callback vector ignores CFLAGS.  */
	  rv = callbacks->open (callbacks, buf, oflags);
	else
	  {
	    int h_oflags = 0;

	    if (oflags & 0x0001)
	      h_oflags |= O_WRONLY;
	    if (oflags & 0x0002)
	      h_oflags |= O_RDWR;
	    if (oflags & 0x0200)
	      h_oflags |= O_CREAT;
	    if (oflags & 0x0008)
	      h_oflags |= O_APPEND;
	    if (oflags & 0x0400)
	      h_oflags |= O_TRUNC;
	    rv = open (buf, h_oflags, cflags);
	  }
	if (trace)
	  printf ("%d\n", rv);
	put_reg (1, rv);
      }
      break;

    case TARGET_NEWLIB_SYS_close:
      {
	int fd = arg ();

	if (callbacks)
	  rv = callbacks->close (callbacks, fd);
	else if (fd > 2)
	  rv = close (fd);
	else
	  rv = 0;
	if (trace)
	  printf ("close(%d) = %d\n", fd, rv);
	put_reg (1, rv);
      }
      break;

    case TARGET_NEWLIB_SYS_read:
      {
	int fd = arg ();
	int addr = arg ();
	int count = arg ();

	if (count > sizeof (buf))
	  count = sizeof (buf);
	if (callbacks)
	  rv = callbacks->read (callbacks, fd, buf, count);
	else
	  rv = read (fd, buf, count);
	if (trace)
	  printf ("read(%d,%d) = %d\n", fd, count, rv);
	if (rv > 0)
	  write_target (buf, addr, rv, 0);
	put_reg (1, rv);
      }
      break;

    case TARGET_NEWLIB_SYS_write:
      {
	int fd = arg ();
	int addr = arg ();
	int count = arg ();

	if (count > sizeof (buf))
	  count = sizeof (buf);
	if (trace)
	  printf ("write(%d,0x%x,%d)\n", fd, addr, count);
	read_target (buf, addr, count, 0);
	if (trace)
	  fflush (stdout);
	if (callbacks)
	  rv = callbacks->write (callbacks, fd, buf, count);
	else
	  rv = write (fd, buf, count);
	if (trace)
	  printf ("write(%d,%d) = %d\n", fd, count, rv);
	put_reg (1, rv);
      }
      break;

    case TARGET_NEWLIB_SYS_getpid:
      put_reg (1, 42);
      break;

    case TARGET_NEWLIB_SYS_gettimeofday:
      {
	int tvaddr = arg ();
	struct timeval tv;

	rv = gettimeofday (&tv, 0);
	if (trace)
	  printf ("gettimeofday: %" PRId64 " sec %" PRId64 " usec to 0x%x\n",
		  (int64_t)tv.tv_sec, (int64_t)tv.tv_usec, tvaddr);
	mem_put_si (tvaddr, tv.tv_sec);
	mem_put_si (tvaddr + 4, tv.tv_usec);
	put_reg (1, rv);
      }
      break;

    case TARGET_NEWLIB_SYS_kill:
      {
	int pid = arg ();
	int sig = arg ();
	if (pid == 42)
	  {
	    if (verbose)
	      printf ("[signal %d]\n", sig);
	    return RX_MAKE_STOPPED (sig);
	  }
      }
      break;

    case 11:
      {
	int heaptop_arg = arg ();
	if (trace)
	  printf ("sbrk: heap top set to %x\n", heaptop_arg);
	heaptop = heaptop_arg;
	if (heapbottom == 0)
	  heapbottom = heaptop_arg;
      }
      break;

    case 255:
      {
	int addr = arg ();
	mem_put_si (addr, rx_cycles + mem_usage_cycles());
      }
      break;

    }
  return RX_MAKE_STEPPED ();
}
