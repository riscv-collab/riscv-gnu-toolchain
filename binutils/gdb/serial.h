/* Remote serial support interface definitions for GDB, the GNU Debugger.
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

#ifndef SERIAL_H
#define SERIAL_H

#ifdef USE_WIN32API
#include <winsock2.h>
#include <windows.h>
#endif

struct ui_file;

/* For most routines, if a failure is indicated, then errno should be
   examined.  */

/* Terminal state pointer.  This is specific to each type of
   interface.  */

typedef void *serial_ttystate;
struct serial;
struct serial_ops;

/* Speed in bits per second, or -1 which means don't mess with the speed.  */

extern int baud_rate;

/* Parity for serial port  */

extern int serial_parity;

/* Create a new serial for OPS.  The new serial is not opened.  */

/* Try to open NAME.  Returns a new `struct serial *' on success, NULL
   on failure.  The new serial object has a reference count of 1.
   Note that some open calls can block and, if possible, should be
   written to be non-blocking, with calls to ui_look_hook so they can
   be cancelled.  An async interface for open could be added to GDB if
   necessary.  */

extern struct serial *serial_open (const char *name);

/* Open a new serial stream using OPS.  */

extern struct serial *serial_open_ops (const struct serial_ops *ops);

/* Returns true if SCB is open.  */

extern int serial_is_open (struct serial *scb);

/* Find an already opened serial stream using a file handle.  */

extern struct serial *serial_for_fd (int fd);

/* Open a new serial stream using a file handle.  */

extern struct serial *serial_fdopen (const int fd);

/* Push out all buffers, close the device and unref SCB.  */

extern void serial_close (struct serial *scb);

/* Increment reference count of SCB.  */

extern void serial_ref (struct serial *scb);

/* Decrement reference count of SCB.  */

extern void serial_unref (struct serial *scb);

/* Create a pipe, and put the read end in FILDES[0], and the write end
   in FILDES[1].  Returns 0 for success, negative value for error (in
   which case errno contains the error).  */

extern int gdb_pipe (int fildes[2]);

/* Create a pipe with each end wrapped in a `struct serial' interface.
   Put the read end in scbs[0], and the write end in scbs[1].  Returns
   0 for success, negative value for error (in which case errno
   contains the error).  */

extern int serial_pipe (struct serial *scbs[2]);

/* Push out all buffers and destroy SCB without closing the device.  */

extern void serial_un_fdopen (struct serial *scb);

/* Read one char from the serial device with TIMEOUT seconds to wait
   or -1 to wait forever.  Use timeout of 0 to effect a poll.
   Infinite waits are not permitted.  Returns unsigned char if ok, else
   one of the following codes.  Note that all error return-codes are
   guaranteed to be < 0.  */

enum serial_rc {
  SERIAL_ERROR = -1,	/* General error.  */
  SERIAL_TIMEOUT = -2,	/* Timeout or data-not-ready during read.
			   Unfortunately, through
			   deprecated_ui_loop_hook (), this can also
			   be a QUIT indication.  */
  SERIAL_EOF = -3	/* General end-of-file or remote target
			   connection closed, indication.  Includes
			   things like the line dropping dead.  */
};

extern int serial_readchar (struct serial *scb, int timeout);

/* Write COUNT bytes from BUF to the port SCB.  Throws exception on
   error.  */

extern void serial_write (struct serial *scb, const void *buf, size_t count);

/* Write a printf style string onto the serial port.  */

extern void serial_printf (struct serial *desc, 
			   const char *,...) ATTRIBUTE_PRINTF (2, 3);

/* Allow pending output to drain.  */

extern int serial_drain_output (struct serial *);

/* Flush (discard) pending output.  Might also flush input (if this
   system can't flush only output).  */

extern int serial_flush_output (struct serial *);

/* Flush pending input.  Might also flush output (if this system can't
   flush only input).  */

extern int serial_flush_input (struct serial *);

/* Send a break between 0.25 and 0.5 seconds long.  */

extern void serial_send_break (struct serial *scb);

/* Turn the port into raw mode.  */

extern void serial_raw (struct serial *scb);

/* Return a pointer to a newly malloc'd ttystate containing the state
   of the tty.  */

extern serial_ttystate serial_get_tty_state (struct serial *scb);

/* Return a pointer to a newly malloc'd ttystate containing a copy
   of the state in TTYSTATE.  */

extern serial_ttystate serial_copy_tty_state (struct serial *scb,
					      serial_ttystate ttystate);

/* Set the state of the tty to TTYSTATE.  The change is immediate.
   When changing to or from raw mode, input might be discarded.
   Returns 0 for success, negative value for error (in which case
   errno contains the error).  */

extern int serial_set_tty_state (struct serial *scb, serial_ttystate ttystate);

/* gdb_printf a user-comprehensible description of ttystate on
   the specified STREAM.  FIXME: At present this sends output to the
   default stream - GDB_STDOUT.  */

extern void serial_print_tty_state (struct serial *scb,
				    serial_ttystate ttystate,
				    struct ui_file *);

/* Set the baudrate to the decimal value supplied.  Throws exception
   on error.  */

extern void serial_setbaudrate (struct serial *scb, int rate);

/* Set the number of stop bits to the value specified.  Returns 0 for
   success, -1 for failure.  */

#define SERIAL_1_STOPBITS 1
#define SERIAL_1_AND_A_HALF_STOPBITS 2	/* 1.5 bits, snicker...  */
#define SERIAL_2_STOPBITS 3

extern int serial_setstopbits (struct serial *scb, int num);

#define GDBPARITY_NONE     0
#define GDBPARITY_ODD      1
#define GDBPARITY_EVEN     2

/* Set parity for serial port. Returns 0 for success, -1 for failure.  */

extern int serial_setparity (struct serial *scb, int parity);

/* Asynchronous serial interface: */

/* Can the serial device support asynchronous mode?  */

extern int serial_can_async_p (struct serial *scb);

/* Has the serial device been put in asynchronous mode?  */

extern int serial_is_async_p (struct serial *scb);

/* For ASYNC enabled devices, register a callback and enable
   asynchronous mode.  To disable asynchronous mode, register a NULL
   callback.  */

typedef void (serial_event_ftype) (struct serial *scb, void *context);
extern void serial_async (struct serial *scb,
			  serial_event_ftype *handler, void *context);

/* Trace/debug mechanism.

   serial_debug() enables/disables internal debugging.
   serial_debug_p() indicates the current debug state.  */

extern void serial_debug (struct serial *scb, int debug_p);

extern int serial_debug_p (struct serial *scb);


/* Details of an instance of a serial object.  */

struct serial
  {
    /* serial objects are ref counted (but not the underlying
       connection, just the object's lifetime in memory).  */
    int refcnt;

    int fd;			/* File descriptor */
    /* File descriptor for a separate error stream that should be
       immediately forwarded to gdb_stderr.  This may be -1.
       If != -1, this descriptor should be non-blocking or
       ops->avail should be non-NULL.  */
    int error_fd;               
    const struct serial_ops *ops; /* Function vector */
    void *state;       		/* Local context info for open FD */
    serial_ttystate ttystate;	/* Not used (yet) */
    int bufcnt;			/* Amount of data remaining in receive
				   buffer.  -ve for sticky errors.  */
    unsigned char *bufp;	/* Current byte */
    unsigned char buf[BUFSIZ];	/* Da buffer itself */
    char *name;			/* The name of the device or host */
    struct serial *next;	/* Pointer to the next `struct serial *' */
    int debug_p;		/* Trace this serial devices operation.  */
    int async_state;		/* Async internal state.  */
    void *async_context;	/* Async event thread's context */
    serial_event_ftype *async_handler;/* Async event handler */
  };

struct serial_ops
  {
    const char *name;
    void (*open) (struct serial *, const char *name);
    void (*close) (struct serial *);
    int (*fdopen) (struct serial *, int fd);
    int (*readchar) (struct serial *, int timeout);
    void (*write) (struct serial *, const void *buf, size_t count);
    /* Discard pending output */
    int (*flush_output) (struct serial *);
    /* Discard pending input */
    int (*flush_input) (struct serial *);
    void (*send_break) (struct serial *);
    void (*go_raw) (struct serial *);
    serial_ttystate (*get_tty_state) (struct serial *);
    serial_ttystate (*copy_tty_state) (struct serial *, serial_ttystate);
    int (*set_tty_state) (struct serial *, serial_ttystate);
    void (*print_tty_state) (struct serial *, serial_ttystate,
			     struct ui_file *);
    void (*setbaudrate) (struct serial *, int rate);
    int (*setstopbits) (struct serial *, int num);
    /* Set the value PARITY as parity setting for serial object.
       Return 0 in the case of success.  */
    int (*setparity) (struct serial *, int parity);
    /* Wait for output to drain.  */
    int (*drain_output) (struct serial *);
    /* Change the serial device into/out of asynchronous mode, call
       the specified function when ever there is something
       interesting.  */
    void (*async) (struct serial *scb, int async_p);
    /* Perform a low-level read operation, reading (at most) COUNT
       bytes into SCB->BUF.  Return zero at end of file.  */
    int (*read_prim)(struct serial *scb, size_t count);
    /* Perform a low-level write operation, writing (at most) COUNT
       bytes from BUF.  */
    int (*write_prim)(struct serial *scb, const void *buf, size_t count);
    /* Return that number of bytes that can be read from FD
       without blocking.  Return value of -1 means that the
       read will not block even if less that requested bytes
       are available.  */
    int (*avail)(struct serial *scb, int fd);

#ifdef USE_WIN32API
    /* Return a handle to wait on, indicating available data from SCB
       when signaled, in *READ.  Return a handle indicating errors
       in *EXCEPT.  */
    void (*wait_handle) (struct serial *scb, HANDLE *read, HANDLE *except);
    void (*done_wait_handle) (struct serial *scb);
#endif /* USE_WIN32API */
  };

/* Add a new serial interface to the interface list.  */

extern void serial_add_interface (const struct serial_ops * optable);

/* File in which to record the remote debugging session.  */

extern void serial_log_command (struct target_ops *self, const char *);

#ifdef USE_WIN32API

/* Windows-only: find or create handles that we can wait on for this
   serial device.  */
extern void serial_wait_handle (struct serial *, HANDLE *, HANDLE *);

/* Windows-only: signal that we are done with the wait handles.  */
extern void serial_done_wait_handle (struct serial *);

#endif /* USE_WIN32API */

#endif /* SERIAL_H */
