/* Trace file support in GDB.

   Copyright (C) 1997-2024 Free Software Foundation, Inc.

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
#include "tracefile.h"
#include "tracectf.h"
#include "exec.h"
#include "regcache.h"
#include "gdbsupport/byte-vector.h"
#include "gdbarch.h"
#include "gdbsupport/buildargv.h"
#include "inferior.h"

/* Helper macros.  */

#define TRACE_WRITE_R_BLOCK(writer, buf, size)	\
  writer->ops->frame_ops->write_r_block ((writer), (buf), (size))
#define TRACE_WRITE_M_BLOCK_HEADER(writer, addr, size)		  \
  writer->ops->frame_ops->write_m_block_header ((writer), (addr), \
						(size))
#define TRACE_WRITE_M_BLOCK_MEMORY(writer, buf, size)	  \
  writer->ops->frame_ops->write_m_block_memory ((writer), (buf), \
						(size))
#define TRACE_WRITE_V_BLOCK(writer, num, val)	\
  writer->ops->frame_ops->write_v_block ((writer), (num), (val))

/* A unique pointer policy class for trace_file_writer.  */

struct trace_file_writer_deleter
{
  void operator() (struct trace_file_writer *writer)
  {
    writer->ops->dtor (writer);
    xfree (writer);
  }
};

/* A unique_ptr specialization for trace_file_writer.  */

typedef std::unique_ptr<trace_file_writer, trace_file_writer_deleter>
    trace_file_writer_up;

/* Save tracepoint data to file named FILENAME through WRITER.  WRITER
   determines the trace file format.  If TARGET_DOES_SAVE is non-zero,
   the save is performed on the target, otherwise GDB obtains all trace
   data and saves it locally.  */

static void
trace_save (const char *filename, struct trace_file_writer *writer,
	    int target_does_save)
{
  struct trace_status *ts = current_trace_status ();
  struct uploaded_tp *uploaded_tps = NULL, *utp;
  struct uploaded_tsv *uploaded_tsvs = NULL, *utsv;

  ULONGEST offset = 0;
#define MAX_TRACE_UPLOAD 2000
  gdb::byte_vector buf (std::max (MAX_TRACE_UPLOAD, trace_regblock_size));
  bfd_endian byte_order = gdbarch_byte_order (current_inferior ()->arch ());

  /* If the target is to save the data to a file on its own, then just
     send the command and be done with it.  */
  if (target_does_save)
    {
      if (!writer->ops->target_save (writer, filename))
	error (_("Target failed to save trace data to '%s'."),
	       filename);
      return;
    }

  /* Get the trace status first before opening the file, so if the
     target is losing, we can get out without touching files.  Since
     we're just calling this for side effects, we ignore the
     result.  */
  target_get_trace_status (ts);

  writer->ops->start (writer, filename);

  writer->ops->write_header (writer);

  /* Write descriptive info.  */

  /* Write out the size of a register block.  */
  writer->ops->write_regblock_type (writer, trace_regblock_size);

  /* Write out the target description info.  */
  writer->ops->write_tdesc (writer);

  /* Write out status of the tracing run (aka "tstatus" info).  */
  writer->ops->write_status (writer, ts);

  /* Note that we want to upload tracepoints and save those, rather
     than simply writing out the local ones, because the user may have
     changed tracepoints in GDB in preparation for a future tracing
     run, or maybe just mass-deleted all types of breakpoints as part
     of cleaning up.  So as not to contaminate the session, leave the
     data in its uploaded form, don't make into real tracepoints.  */

  /* Get trace state variables first, they may be checked when parsing
     uploaded commands.  */

  target_upload_trace_state_variables (&uploaded_tsvs);

  for (utsv = uploaded_tsvs; utsv; utsv = utsv->next)
    writer->ops->write_uploaded_tsv (writer, utsv);

  free_uploaded_tsvs (&uploaded_tsvs);

  target_upload_tracepoints (&uploaded_tps);

  for (utp = uploaded_tps; utp; utp = utp->next)
    target_get_tracepoint_status (NULL, utp);

  for (utp = uploaded_tps; utp; utp = utp->next)
    writer->ops->write_uploaded_tp (writer, utp);

  free_uploaded_tps (&uploaded_tps);

  /* Mark the end of the definition section.  */
  writer->ops->write_definition_end (writer);

  /* Get and write the trace data proper.  */
  while (1)
    {
      LONGEST gotten = 0;

      /* The writer supports writing the contents of trace buffer
	  directly to trace file.  Don't parse the contents of trace
	  buffer.  */
      if (writer->ops->write_trace_buffer != NULL)
	{
	  /* We ask for big blocks, in the hopes of efficiency, but
	     will take less if the target has packet size limitations
	     or some such.  */
	  gotten = target_get_raw_trace_data (buf.data (), offset,
					      MAX_TRACE_UPLOAD);
	  if (gotten < 0)
	    error (_("Failure to get requested trace buffer data"));
	  /* No more data is forthcoming, we're done.  */
	  if (gotten == 0)
	    break;

	  writer->ops->write_trace_buffer (writer, buf.data (), gotten);

	  offset += gotten;
	}
      else
	{
	  uint16_t tp_num;
	  uint32_t tf_size;
	  /* Parse the trace buffers according to how data are stored
	     in trace buffer in GDBserver.  */

	  gotten = target_get_raw_trace_data (buf.data (), offset, 6);

	  if (gotten == 0)
	    break;

	  /* Read the first six bytes in, which is the tracepoint
	     number and trace frame size.  */
	  tp_num = (uint16_t)
	    extract_unsigned_integer (&((buf.data ())[0]), 2, byte_order);

	  tf_size = (uint32_t)
	    extract_unsigned_integer (&((buf.data ())[2]), 4, byte_order);

	  writer->ops->frame_ops->start (writer, tp_num);
	  gotten = 6;

	  if (tf_size > 0)
	    {
	      unsigned int block;

	      offset += 6;

	      for (block = 0; block < tf_size; )
		{
		  gdb_byte block_type;

		  /* We'll fetch one block each time, in order to
		     handle the extremely large 'M' block.  We first
		     fetch one byte to get the type of the block.  */
		  gotten = target_get_raw_trace_data (buf.data (),
						      offset, 1);
		  if (gotten < 1)
		    error (_("Failure to get requested trace buffer data"));

		  gotten = 1;
		  block += 1;
		  offset += 1;

		  block_type = buf[0];
		  switch (block_type)
		    {
		    case 'R':
		      gotten
			= target_get_raw_trace_data (buf.data (), offset,
						     trace_regblock_size);
		      if (gotten < trace_regblock_size)
			error (_("Failure to get requested trace"
				 " buffer data"));

		      TRACE_WRITE_R_BLOCK (writer, buf.data (),
					   trace_regblock_size);
		      break;
		    case 'M':
		      {
			unsigned short mlen;
			ULONGEST addr;
			LONGEST t;
			int j;

			t = target_get_raw_trace_data (buf.data (),
						       offset, 10);
			if (t < 10)
			  error (_("Failure to get requested trace"
				   " buffer data"));

			offset += 10;
			block += 10;

			gotten = 0;
			addr = (ULONGEST)
			  extract_unsigned_integer (buf.data (), 8,
						    byte_order);
			mlen = (unsigned short)
			  extract_unsigned_integer (&((buf.data ())[8]), 2,
						    byte_order);

			TRACE_WRITE_M_BLOCK_HEADER (writer, addr,
						    mlen);

			/* The memory contents in 'M' block may be
			   very large.  Fetch the data from the target
			   and write them into file one by one.  */
			for (j = 0; j < mlen; )
			  {
			    unsigned int read_length;

			    if (mlen - j > MAX_TRACE_UPLOAD)
			      read_length = MAX_TRACE_UPLOAD;
			    else
			      read_length = mlen - j;

			    t = target_get_raw_trace_data (buf.data (),
							   offset + j,
							   read_length);
			    if (t < read_length)
			      error (_("Failure to get requested"
				       " trace buffer data"));

			    TRACE_WRITE_M_BLOCK_MEMORY (writer,
							buf.data (),
							read_length);

			    j += read_length;
			    gotten += read_length;
			  }

			break;
		      }
		    case 'V':
		      {
			int vnum;
			LONGEST val;

			gotten
			  = target_get_raw_trace_data (buf.data (),
						       offset, 12);
			if (gotten < 12)
			  error (_("Failure to get requested"
				   " trace buffer data"));

			vnum  = (int) extract_signed_integer (buf.data (),
							      4,
							      byte_order);
			val
			  = extract_signed_integer (&((buf.data ())[4]),
						    8, byte_order);

			TRACE_WRITE_V_BLOCK (writer, vnum, val);
		      }
		      break;
		    default:
		      error (_("Unknown block type '%c' (0x%x) in"
			       " trace frame"),
			     block_type, block_type);
		    }

		  block += gotten;
		  offset += gotten;
		}
	    }
	  else
	    offset += gotten;

	  writer->ops->frame_ops->end (writer);
	}
    }

  writer->ops->end (writer);
}

static void
tsave_command (const char *args, int from_tty)
{
  int target_does_save = 0;
  char **argv;
  char *filename = NULL;
  int generate_ctf = 0;

  if (args == NULL)
    error_no_arg (_("file in which to save trace data"));

  gdb_argv built_argv (args);
  argv = built_argv.get ();

  for (; *argv; ++argv)
    {
      if (strcmp (*argv, "-r") == 0)
	target_does_save = 1;
      else if (strcmp (*argv, "-ctf") == 0)
	generate_ctf = 1;
      else if (**argv == '-')
	error (_("unknown option `%s'"), *argv);
      else
	filename = *argv;
    }

  if (!filename)
    error_no_arg (_("file in which to save trace data"));

  if (generate_ctf)
    trace_save_ctf (filename, target_does_save);
  else
    trace_save_tfile (filename, target_does_save);

  if (from_tty)
    gdb_printf (_("Trace data saved to %s '%s'.\n"),
		generate_ctf ? "directory" : "file", filename);
}

/* Save the trace data to file FILENAME of tfile format.  */

void
trace_save_tfile (const char *filename, int target_does_save)
{
  trace_file_writer_up writer (tfile_trace_file_writer_new ());
  trace_save (filename, writer.get (), target_does_save);
}

/* Save the trace data to dir DIRNAME of ctf format.  */

void
trace_save_ctf (const char *dirname, int target_does_save)
{
  trace_file_writer_up writer (ctf_trace_file_writer_new ());
  trace_save (dirname, writer.get (), target_does_save);
}

/* Fetch register data from tracefile, shared for both tfile and
   ctf.  */

void
tracefile_fetch_registers (struct regcache *regcache, int regno)
{
  struct gdbarch *gdbarch = regcache->arch ();
  struct tracepoint *tp = get_tracepoint (get_tracepoint_number ());
  int regn;

  /* We get here if no register data has been found.  Mark registers
     as unavailable.  */
  for (regn = 0; regn < gdbarch_num_regs (gdbarch); regn++)
    regcache->raw_supply (regn, NULL);

  /* We can often usefully guess that the PC is going to be the same
     as the address of the tracepoint.  */
  if (tp == nullptr || !tp->has_locations ())
    return;

  /* But don't try to guess if tracepoint is multi-location...  */
  if (tp->has_multiple_locations ())
    {
      warning (_("Tracepoint %d has multiple "
		 "locations, cannot infer $pc"),
	       tp->number);
      return;
    }
  /* ... or does while-stepping.  */
  else if (tp->step_count > 0)
    {
      warning (_("Tracepoint %d does while-stepping, "
		 "cannot infer $pc"),
	       tp->number);
      return;
    }

  /* Guess what we can from the tracepoint location.  */
  gdbarch_guess_tracepoint_registers (gdbarch, regcache,
				      tp->first_loc ().address);
}

/* This is the implementation of target_ops method to_has_all_memory.  */

bool
tracefile_target::has_all_memory ()
{
  return true;
}

/* This is the implementation of target_ops method to_has_memory.  */

bool
tracefile_target::has_memory ()
{
  return true;
}

/* This is the implementation of target_ops method to_has_stack.
   The target has a stack when GDB has already selected one trace
   frame.  */

bool
tracefile_target::has_stack ()
{
  return get_traceframe_number () != -1;
}

/* This is the implementation of target_ops method to_has_registers.
   The target has registers when GDB has already selected one trace
   frame.  */

bool
tracefile_target::has_registers ()
{
  return get_traceframe_number () != -1;
}

/* This is the implementation of target_ops method to_thread_alive.
   tracefile has one thread faked by GDB.  */

bool
tracefile_target::thread_alive (ptid_t ptid)
{
  return true;
}

/* This is the implementation of target_ops method to_get_trace_status.
   The trace status for a file is that tracing can never be run.  */

int
tracefile_target::get_trace_status (struct trace_status *ts)
{
  /* Other bits of trace status were collected as part of opening the
     trace files, so nothing to do here.  */

  return -1;
}

void _initialize_tracefile ();
void
_initialize_tracefile ()
{
  add_com ("tsave", class_trace, tsave_command, _("\
Save the trace data to a file.\n\
Use the '-ctf' option to save the data to CTF format.\n\
Use the '-r' option to direct the target to save directly to the file,\n\
using its own filesystem."));
}
