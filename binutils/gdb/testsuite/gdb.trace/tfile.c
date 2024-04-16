/* This testcase is part of GDB, the GNU debugger.

   Copyright 2010-2024 Free Software Foundation, Inc.

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

/* This program does two things; it generates valid trace files, and
   it can also be traced so as to test trace file creation from
   GDB.  */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdint.h>

char spbuf[200];

char trbuf[1000];
char *trptr;
char *tfsizeptr;

/* These globals are put in the trace buffer.  */

int testglob = 31415;

int testglob2 = 271828;

/* But these below are not.  */

const int constglob = 10000;

int nonconstglob = 14124;

int
start_trace_file (char *filename)
{
  int fd;
  mode_t mode = S_IRUSR | S_IWUSR;

#ifdef S_IRGRP
  mode |= S_IRGRP;
#endif

#ifdef S_IROTH
  mode |= S_IROTH;
#endif

  fd = open (filename, O_WRONLY|O_CREAT|O_APPEND, mode);

  if (fd < 0)
    return fd;

  /* Write a file header, with a high-bit-set char to indicate a
     binary file, plus a hint as what this file is, and a version
     number in case of future needs.  */
  write (fd, "\x7fTRACE0\n", 8);

  return fd;
}

void
finish_trace_file (int fd)
{
  close (fd);
}

static void
tfile_write_64 (uint64_t value)
{
  memcpy (trptr, &value, sizeof (uint64_t));
  trptr += sizeof (uint64_t);
}

static void
tfile_write_16 (uint16_t value)
{
  memcpy (trptr, &value, sizeof (uint16_t));
  trptr += sizeof (uint16_t);
}

static void
tfile_write_8 (uint8_t value)
{
  memcpy (trptr, &value, sizeof (uint8_t));
  trptr += sizeof (uint8_t);
}

static void
tfile_write_addr (char *addr)
{
  tfile_write_64 ((uint64_t) (uintptr_t) addr);
}

static void
tfile_write_buf (const void *addr, size_t size)
{
  memcpy (trptr, addr, size);
  trptr += size;
}

void
add_memory_block (char *addr, int size)
{
  tfile_write_8 ('M');
  tfile_write_addr (addr);
  tfile_write_16 (size);
  tfile_write_buf (addr, size);
}

/* Adjust a function's address to account for architectural
   particularities.  */

static uintptr_t
adjust_function_address (uintptr_t func_addr)
{
#if defined(__thumb__) || defined(__thumb2__)
  /* Although Thumb functions are two-byte aligned, function
     pointers have the Thumb bit set.  Clear it.  */
  return func_addr & ~1;
#elif defined __powerpc64__ && _CALL_ELF != 2
  /* Get function address from function descriptor.  */
  return *(uintptr_t *) func_addr;
#else
  return func_addr;
#endif
}

/* Get a function's address as an integer.  */

#define FUNCTION_ADDRESS(FUN) adjust_function_address ((uintptr_t) &FUN)

void
write_basic_trace_file (void)
{
  int fd, int_x;
  unsigned long long func_addr;

  fd = start_trace_file (TFILE_DIR "tfile-basic.tf");

  /* The next part of the file consists of newline-separated lines
     defining status, tracepoints, etc.  The section is terminated by
     an empty line.  */

  /* Dump the size of the R (register) blocks in traceframes.  */
  snprintf (spbuf, sizeof spbuf, "R %x\n", 500 /* FIXME get from arch */);
  write (fd, spbuf, strlen (spbuf));

  /* Dump trace status, in the general form of the qTstatus reply.  */
  snprintf (spbuf, sizeof spbuf, "status 0;tstop:0;tframes:1;tcreated:1;tfree:100;tsize:1000\n");
  write (fd, spbuf, strlen (spbuf));

  /* Dump tracepoint definitions, in syntax similar to that used
     for reconnection uploads.  */
  func_addr = FUNCTION_ADDRESS (write_basic_trace_file);

  snprintf (spbuf, sizeof spbuf, "tp T1:%llx:E:0:0\n", func_addr);
  write (fd, spbuf, strlen (spbuf));
  /* (Note that we would only need actions defined if we wanted to
     test tdump.) */

  /* Empty line marks the end of the definition section.  */
  write (fd, "\n", 1);

  /* Make up a simulated trace buffer.  */
  /* (Encapsulate better if we're going to do lots of this; note that
     buffer endianness is the target program's endianness.) */
  trptr = trbuf;
  tfile_write_16 (1);

  tfsizeptr = trptr;
  trptr += 4;
  add_memory_block ((char *) &testglob, sizeof (testglob));
  /* Divide a variable between two separate memory blocks.  */
  add_memory_block ((char *) &testglob2, 1);
  add_memory_block (((char*) &testglob2) + 1, sizeof (testglob2) - 1);
  /* Go back and patch in the frame size.  */
  int_x = trptr - tfsizeptr - sizeof (int);
  memcpy (tfsizeptr, &int_x, 4);

  /* Write end of tracebuffer marker.  */
  memset (trptr, 0, 6);
  trptr += 6;

  write (fd, trbuf, trptr - trbuf);

  finish_trace_file (fd);
}

/* Convert number NIB to a hex digit.  */

static int
tohex (int nib)
{
  if (nib < 10)
    return '0' + nib;
  else
    return 'a' + nib - 10;
}

int
bin2hex (const char *bin, char *hex, int count)
{
  int i;

  for (i = 0; i < count; i++)
    {
      *hex++ = tohex ((*bin >> 4) & 0xf);
      *hex++ = tohex (*bin++ & 0xf);
    }
  *hex = 0;
  return i;
}

void
write_error_trace_file (void)
{
  int fd;
  const char made_up[] = "made-up error";
  char hex[(sizeof (made_up) - 1) * 2 + 1];
  int len = sizeof (made_up) - 1;

  fd = start_trace_file (TFILE_DIR "tfile-error.tf");

  /* The next part of the file consists of newline-separated lines
     defining status, tracepoints, etc.  The section is terminated by
     an empty line.  */

  /* Dump the size of the R (register) blocks in traceframes.  */
  snprintf (spbuf, sizeof spbuf, "R %x\n", 500 /* FIXME get from arch */);
  write (fd, spbuf, strlen (spbuf));

  bin2hex (made_up, hex, len);

  /* Dump trace status, in the general form of the qTstatus reply.  */
  snprintf (spbuf, sizeof spbuf,
	    "status 0;"
	    "terror:%s:1;"
	    "tframes:0;tcreated:0;tfree:100;tsize:1000\n",
	    hex);
  write (fd, spbuf, strlen (spbuf));

  /* Dump tracepoint definitions, in syntax similar to that used
     for reconnection uploads.  */
  snprintf (spbuf, sizeof spbuf, "tp T1:%llx:E:0:0\n",
	    (unsigned long long) FUNCTION_ADDRESS (write_basic_trace_file));
  write (fd, spbuf, strlen (spbuf));
  /* (Note that we would only need actions defined if we wanted to
     test tdump.) */

  /* Empty line marks the end of the definition section.  */
  write (fd, "\n", 1);

  trptr = trbuf;

  /* Write end of tracebuffer marker.  */
  memset (trptr, 0, 6);
  trptr += 6;

  write (fd, trbuf, trptr - trbuf);

  finish_trace_file (fd);
}

void
done_making_trace_files (void)
{
}

int
main (void)
{
  write_basic_trace_file ();

  write_error_trace_file ();

  done_making_trace_files ();

  return 0;
}

