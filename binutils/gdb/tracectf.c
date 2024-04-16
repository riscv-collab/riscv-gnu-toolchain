/* CTF format support.

   Copyright (C) 2012-2024 Free Software Foundation, Inc.
   Contributed by Hui Zhu <hui_zhu@mentor.com>
   Contributed by Yao Qi <yao@codesourcery.com>

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
#include "tracectf.h"
#include "tracepoint.h"
#include "regcache.h"
#include <sys/stat.h>
#include "exec.h"
#include "completer.h"
#include "inferior.h"
#include "gdbthread.h"
#include "tracefile.h"
#include <ctype.h>
#include <algorithm>
#include "gdbsupport/filestuff.h"
#include "gdbarch.h"

/* GDB saves trace buffers and other information (such as trace
   status) got from the remote target into Common Trace Format (CTF).
   The following types of information are expected to save in CTF:

   1. The length (in bytes) of register cache.  Event "register" will
   be defined in metadata, which includes the length.

   2. Trace status.  Event "status" is defined in metadata, which
   includes all aspects of trace status.

   3. Uploaded trace variables.  Event "tsv_def" is defined in
   metadata, which is about all aspects of a uploaded trace variable.
   Uploaded tracepoints.   Event "tp_def" is defined in meta, which
   is about all aspects of an uploaded tracepoint.  Note that the
   "sequence" (a CTF type, which is a dynamically-sized array.) is
   used for "actions" "step_actions" and "cmd_strings".

   4. Trace frames.  Each trace frame is composed by several blocks
   of different types ('R', 'M', 'V').  One trace frame is saved in
   one CTF packet and the blocks of this frame are saved as events.
   4.1: The trace frame related information (such as the number of
   tracepoint associated with this frame) is saved in the packet
   context.
   4.2: The block 'M', 'R' and 'V' are saved in event "memory",
   "register" and "tsv" respectively.
   4.3: When iterating over events, babeltrace can't tell iterator
   goes to a new packet, so we need a marker or anchor to tell GDB
   that iterator goes into a new packet or frame.  We define event
   "frame".  */

#define CTF_MAGIC		0xC1FC1FC1
#define CTF_SAVE_MAJOR		1
#define CTF_SAVE_MINOR		8

#define CTF_METADATA_NAME	"metadata"
#define CTF_DATASTREAM_NAME	"datastream"

/* Reserved event id.  */

#define CTF_EVENT_ID_REGISTER 0
#define CTF_EVENT_ID_TSV 1
#define CTF_EVENT_ID_MEMORY 2
#define CTF_EVENT_ID_FRAME 3
#define CTF_EVENT_ID_STATUS 4
#define CTF_EVENT_ID_TSV_DEF 5
#define CTF_EVENT_ID_TP_DEF 6

#define CTF_PID (2)

/* The state kept while writing the CTF datastream file.  */

struct trace_write_handler
{
  /* File descriptor of metadata.  */
  FILE *metadata_fd;
  /* File descriptor of traceframes.  */
  FILE *datastream_fd;

  /* This is the content size of the current packet.  */
  size_t content_size;

  /* This is the start offset of current packet.  */
  long packet_start;
};

/* Write metadata in FORMAT.  */

static void
ctf_save_write_metadata (struct trace_write_handler *handler,
			 const char *format, ...)
  ATTRIBUTE_PRINTF (2, 3);

static void
ctf_save_write_metadata (struct trace_write_handler *handler,
			 const char *format, ...)
{
  va_list args;

  va_start (args, format);
  if (vfprintf (handler->metadata_fd, format, args) < 0)
    error (_("Unable to write metadata file (%s)"),
	     safe_strerror (errno));
  va_end (args);
}

/* Write BUF of length SIZE to datastream file represented by
   HANDLER.  */

static int
ctf_save_write (struct trace_write_handler *handler,
		const gdb_byte *buf, size_t size)
{
  if (fwrite (buf, size, 1, handler->datastream_fd) != 1)
    error (_("Unable to write file for saving trace data (%s)"),
	   safe_strerror (errno));

  handler->content_size += size;

  return 0;
}

/* Write a unsigned 32-bit integer to datastream file represented by
   HANDLER.  */

#define ctf_save_write_uint32(HANDLER, U32) \
  ctf_save_write (HANDLER, (gdb_byte *) &U32, 4)

/* Write a signed 32-bit integer to datastream file represented by
   HANDLER.  */

#define ctf_save_write_int32(HANDLER, INT32) \
  ctf_save_write ((HANDLER), (gdb_byte *) &(INT32), 4)

/* Set datastream file position.  Update HANDLER->content_size
   if WHENCE is SEEK_CUR.  */

static int
ctf_save_fseek (struct trace_write_handler *handler, long offset,
		int whence)
{
  gdb_assert (whence != SEEK_END);
  gdb_assert (whence != SEEK_SET
	      || offset <= handler->content_size + handler->packet_start);

  if (fseek (handler->datastream_fd, offset, whence))
    error (_("Unable to seek file for saving trace data (%s)"),
	   safe_strerror (errno));

  if (whence == SEEK_CUR)
    handler->content_size += offset;

  return 0;
}

/* Change the datastream file position to align on ALIGN_SIZE,
   and write BUF to datastream file.  The size of BUF is SIZE.  */

static int
ctf_save_align_write (struct trace_write_handler *handler,
		      const gdb_byte *buf,
		      size_t size, size_t align_size)
{
  long offset
    = (align_up (handler->content_size, align_size)
       - handler->content_size);

  if (ctf_save_fseek (handler, offset, SEEK_CUR))
    return -1;

  if (ctf_save_write (handler, buf, size))
    return -1;

  return 0;
}

/* Write events to next new packet.  */

static void
ctf_save_next_packet (struct trace_write_handler *handler)
{
  handler->packet_start += (handler->content_size + 4);
  ctf_save_fseek (handler, handler->packet_start, SEEK_SET);
  handler->content_size = 0;
}

/* Write the CTF metadata header.  */

static void
ctf_save_metadata_header (struct trace_write_handler *handler)
{
  ctf_save_write_metadata (handler, "/* CTF %d.%d */\n",
			   CTF_SAVE_MAJOR, CTF_SAVE_MINOR);
  ctf_save_write_metadata (handler,
			   "typealias integer { size = 8; align = 8; "
			   "signed = false; encoding = ascii;}"
			   " := ascii;\n");
  ctf_save_write_metadata (handler,
			   "typealias integer { size = 8; align = 8; "
			   "signed = false; }"
			   " := uint8_t;\n");
  ctf_save_write_metadata (handler,
			   "typealias integer { size = 16; align = 16;"
			   "signed = false; } := uint16_t;\n");
  ctf_save_write_metadata (handler,
			   "typealias integer { size = 32; align = 32;"
			   "signed = false; } := uint32_t;\n");
  ctf_save_write_metadata (handler,
			   "typealias integer { size = 64; align = 64;"
			   "signed = false; base = hex;}"
			   " := uint64_t;\n");
  ctf_save_write_metadata (handler,
			   "typealias integer { size = 32; align = 32;"
			   "signed = true; } := int32_t;\n");
  ctf_save_write_metadata (handler,
			   "typealias integer { size = 64; align = 64;"
			   "signed = true; } := int64_t;\n");
  ctf_save_write_metadata (handler,
			   "typealias string { encoding = ascii;"
			   " } := chars;\n");
  ctf_save_write_metadata (handler, "\n");

  /* Get the byte order of the host and write CTF data in this byte
     order.  */
#if WORDS_BIGENDIAN
#define HOST_ENDIANNESS "be"
#else
#define HOST_ENDIANNESS "le"
#endif

  ctf_save_write_metadata (handler,
			   "\ntrace {\n"
			   "	major = %u;\n"
			   "	minor = %u;\n"
			   "	byte_order = %s;\n"
			   "	packet.header := struct {\n"
			   "		uint32_t magic;\n"
			   "	};\n"
			   "};\n"
			   "\n"
			   "stream {\n"
			   "	packet.context := struct {\n"
			   "		uint32_t content_size;\n"
			   "		uint32_t packet_size;\n"
			   "		uint16_t tpnum;\n"
			   "	};\n"
			   "	event.header := struct {\n"
			   "		uint32_t id;\n"
			   "	};\n"
			   "};\n",
			   CTF_SAVE_MAJOR, CTF_SAVE_MINOR,
			   HOST_ENDIANNESS);
  ctf_save_write_metadata (handler, "\n");
}

/* CTF trace writer.  */

struct ctf_trace_file_writer
{
  struct trace_file_writer base;

  /* States related to writing CTF trace file.  */
  struct trace_write_handler tcs;
};

/* This is the implementation of trace_file_write_ops method
   dtor.  */

static void
ctf_dtor (struct trace_file_writer *self)
{
  struct ctf_trace_file_writer *writer
    = (struct ctf_trace_file_writer *) self;

  if (writer->tcs.metadata_fd != NULL)
    fclose (writer->tcs.metadata_fd);

  if (writer->tcs.datastream_fd != NULL)
    fclose (writer->tcs.datastream_fd);

}

/* This is the implementation of trace_file_write_ops method
   target_save.  */

static int
ctf_target_save (struct trace_file_writer *self,
		 const char *dirname)
{
  /* Don't support save trace file to CTF format in the target.  */
  return 0;
}

/* This is the implementation of trace_file_write_ops method
   start.  It creates the directory DIRNAME, metadata and datastream
   in the directory.  */

static void
ctf_start (struct trace_file_writer *self, const char *dirname)
{
  struct ctf_trace_file_writer *writer
    = (struct ctf_trace_file_writer *) self;
  mode_t hmode = S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH;

  /* Create DIRNAME.  */
  if (mkdir (dirname, hmode) && errno != EEXIST)
    error (_("Unable to open directory '%s' for saving trace data (%s)"),
	   dirname, safe_strerror (errno));

  memset (&writer->tcs, '\0', sizeof (writer->tcs));

  std::string file_name = string_printf ("%s/%s", dirname, CTF_METADATA_NAME);

  writer->tcs.metadata_fd
    = gdb_fopen_cloexec (file_name.c_str (), "w").release ();
  if (writer->tcs.metadata_fd == NULL)
    error (_("Unable to open file '%s' for saving trace data (%s)"),
	   file_name.c_str (), safe_strerror (errno));

  ctf_save_metadata_header (&writer->tcs);

  file_name = string_printf ("%s/%s", dirname, CTF_DATASTREAM_NAME);
  writer->tcs.datastream_fd
    = gdb_fopen_cloexec (file_name.c_str (), "w").release ();
  if (writer->tcs.datastream_fd == NULL)
    error (_("Unable to open file '%s' for saving trace data (%s)"),
	   file_name.c_str (), safe_strerror (errno));
}

/* This is the implementation of trace_file_write_ops method
   write_header.  Write the types of events on trace variable and
   frame.  */

static void
ctf_write_header (struct trace_file_writer *self)
{
  struct ctf_trace_file_writer *writer
    = (struct ctf_trace_file_writer *) self;


  ctf_save_write_metadata (&writer->tcs, "\n");
  ctf_save_write_metadata (&writer->tcs,
			   "event {\n\tname = \"memory\";\n\tid = %u;\n"
			   "\tfields := struct { \n"
			   "\t\tuint64_t address;\n"
			   "\t\tuint16_t length;\n"
			   "\t\tuint8_t contents[length];\n"
			   "\t};\n"
			   "};\n", CTF_EVENT_ID_MEMORY);

  ctf_save_write_metadata (&writer->tcs, "\n");
  ctf_save_write_metadata (&writer->tcs,
			   "event {\n\tname = \"tsv\";\n\tid = %u;\n"
			   "\tfields := struct { \n"
			   "\t\tuint64_t val;\n"
			   "\t\tuint32_t num;\n"
			   "\t};\n"
			   "};\n", CTF_EVENT_ID_TSV);

  ctf_save_write_metadata (&writer->tcs, "\n");
  ctf_save_write_metadata (&writer->tcs,
			   "event {\n\tname = \"frame\";\n\tid = %u;\n"
			   "\tfields := struct { \n"
			   "\t};\n"
			   "};\n", CTF_EVENT_ID_FRAME);

  ctf_save_write_metadata (&writer->tcs, "\n");
  ctf_save_write_metadata (&writer->tcs,
			  "event {\n\tname = \"tsv_def\";\n"
			  "\tid = %u;\n\tfields := struct { \n"
			  "\t\tint64_t initial_value;\n"
			  "\t\tint32_t number;\n"
			  "\t\tint32_t builtin;\n"
			  "\t\tchars name;\n"
			  "\t};\n"
			  "};\n", CTF_EVENT_ID_TSV_DEF);

  ctf_save_write_metadata (&writer->tcs, "\n");
  ctf_save_write_metadata (&writer->tcs,
			   "event {\n\tname = \"tp_def\";\n"
			   "\tid = %u;\n\tfields := struct { \n"
			   "\t\tuint64_t addr;\n"
			   "\t\tuint64_t traceframe_usage;\n"
			   "\t\tint32_t number;\n"
			   "\t\tint32_t enabled;\n"
			   "\t\tint32_t step;\n"
			   "\t\tint32_t pass;\n"
			   "\t\tint32_t hit_count;\n"
			   "\t\tint32_t type;\n"
			   "\t\tchars cond;\n"

			  "\t\tuint32_t action_num;\n"
			  "\t\tchars actions[action_num];\n"

			  "\t\tuint32_t step_action_num;\n"
			  "\t\tchars step_actions[step_action_num];\n"

			  "\t\tchars at_string;\n"
			  "\t\tchars cond_string;\n"

			  "\t\tuint32_t cmd_num;\n"
			  "\t\tchars cmd_strings[cmd_num];\n"
			  "\t};\n"
			  "};\n", CTF_EVENT_ID_TP_DEF);

  gdb_assert (writer->tcs.content_size == 0);
  gdb_assert (writer->tcs.packet_start == 0);

  /* Create a new packet to contain this event.  */
  self->ops->frame_ops->start (self, 0);
}

/* This is the implementation of trace_file_write_ops method
   write_regblock_type.  Write the type of register event in
   metadata.  */

static void
ctf_write_regblock_type (struct trace_file_writer *self, int size)
{
  struct ctf_trace_file_writer *writer
    = (struct ctf_trace_file_writer *) self;

  ctf_save_write_metadata (&writer->tcs, "\n");

  ctf_save_write_metadata (&writer->tcs,
			   "event {\n\tname = \"register\";\n\tid = %u;\n"
			   "\tfields := struct { \n"
			   "\t\tascii contents[%d];\n"
			   "\t};\n"
			   "};\n",
			   CTF_EVENT_ID_REGISTER, size);
}

/* This is the implementation of trace_file_write_ops method
   write_status.  */

static void
ctf_write_status (struct trace_file_writer *self,
		  struct trace_status *ts)
{
  struct ctf_trace_file_writer *writer
    = (struct ctf_trace_file_writer *) self;
  uint32_t id;

  ctf_save_write_metadata (&writer->tcs, "\n");
  ctf_save_write_metadata (&writer->tcs,
			   "event {\n\tname = \"status\";\n\tid = %u;\n"
			   "\tfields := struct { \n"
			   "\t\tint32_t stop_reason;\n"
			   "\t\tint32_t stopping_tracepoint;\n"
			   "\t\tint32_t traceframe_count;\n"
			   "\t\tint32_t traceframes_created;\n"
			   "\t\tint32_t buffer_free;\n"
			   "\t\tint32_t buffer_size;\n"
			   "\t\tint32_t disconnected_tracing;\n"
			   "\t\tint32_t circular_buffer;\n"
			   "\t};\n"
			   "};\n",
			   CTF_EVENT_ID_STATUS);

  id = CTF_EVENT_ID_STATUS;
  /* Event Id.  */
  ctf_save_align_write (&writer->tcs, (gdb_byte *) &id, 4, 4);

  ctf_save_write_int32 (&writer->tcs, ts->stop_reason);
  ctf_save_write_int32 (&writer->tcs, ts->stopping_tracepoint);
  ctf_save_write_int32 (&writer->tcs, ts->traceframe_count);
  ctf_save_write_int32 (&writer->tcs, ts->traceframes_created);
  ctf_save_write_int32 (&writer->tcs, ts->buffer_free);
  ctf_save_write_int32 (&writer->tcs, ts->buffer_size);
  ctf_save_write_int32 (&writer->tcs, ts->disconnected_tracing);
  ctf_save_write_int32 (&writer->tcs, ts->circular_buffer);
}

/* This is the implementation of trace_file_write_ops method
   write_uploaded_tsv.  */

static void
ctf_write_uploaded_tsv (struct trace_file_writer *self,
			struct uploaded_tsv *tsv)
{
  struct ctf_trace_file_writer *writer
    = (struct ctf_trace_file_writer *) self;
  int32_t int32;
  int64_t int64;
  const gdb_byte zero = 0;

  /* Event Id.  */
  int32 = CTF_EVENT_ID_TSV_DEF;
  ctf_save_align_write (&writer->tcs, (gdb_byte *) &int32, 4, 4);

  /* initial_value */
  int64 = tsv->initial_value;
  ctf_save_align_write (&writer->tcs, (gdb_byte *) &int64, 8, 8);

  /* number */
  ctf_save_write_int32 (&writer->tcs, tsv->number);

  /* builtin */
  ctf_save_write_int32 (&writer->tcs, tsv->builtin);

  /* name */
  if (tsv->name != NULL)
    ctf_save_write (&writer->tcs, (gdb_byte *) tsv->name,
		    strlen (tsv->name));
  ctf_save_write (&writer->tcs, &zero, 1);
}

/* This is the implementation of trace_file_write_ops method
   write_uploaded_tp.  */

static void
ctf_write_uploaded_tp (struct trace_file_writer *self,
		       struct uploaded_tp *tp)
{
  struct ctf_trace_file_writer *writer
    = (struct ctf_trace_file_writer *) self;
  int32_t int32;
  int64_t int64;
  uint32_t u32;
  const gdb_byte zero = 0;

  /* Event Id.  */
  int32 = CTF_EVENT_ID_TP_DEF;
  ctf_save_align_write (&writer->tcs, (gdb_byte *) &int32, 4, 4);

  /* address */
  int64 = tp->addr;
  ctf_save_align_write (&writer->tcs, (gdb_byte *) &int64, 8, 8);

  /* traceframe_usage */
  int64 = tp->traceframe_usage;
  ctf_save_align_write (&writer->tcs, (gdb_byte *) &int64, 8, 8);

  /* number */
  ctf_save_write_int32 (&writer->tcs, tp->number);

  /* enabled */
  ctf_save_write_int32 (&writer->tcs, tp->enabled);

  /* step */
  ctf_save_write_int32 (&writer->tcs, tp->step);

  /* pass */
  ctf_save_write_int32 (&writer->tcs, tp->pass);

  /* hit_count */
  ctf_save_write_int32 (&writer->tcs, tp->hit_count);

  /* type */
  ctf_save_write_int32 (&writer->tcs, tp->type);

  /* condition  */
  if (tp->cond != NULL)
    ctf_save_write (&writer->tcs, (gdb_byte *) tp->cond.get (),
		    strlen (tp->cond.get ()));
  ctf_save_write (&writer->tcs, &zero, 1);

  /* actions */
  u32 = tp->actions.size ();
  ctf_save_align_write (&writer->tcs, (gdb_byte *) &u32, 4, 4);
  for (const auto &act : tp->actions)
    ctf_save_write (&writer->tcs, (gdb_byte *) act.get (),
		    strlen (act.get ()) + 1);

  /* step_actions */
  u32 = tp->step_actions.size ();
  ctf_save_align_write (&writer->tcs, (gdb_byte *) &u32, 4, 4);
  for (const auto &act : tp->step_actions)
    ctf_save_write (&writer->tcs, (gdb_byte *) act.get (),
		    strlen (act.get ()) + 1);

  /* at_string */
  if (tp->at_string != NULL)
    ctf_save_write (&writer->tcs, (gdb_byte *) tp->at_string.get (),
		    strlen (tp->at_string.get ()));
  ctf_save_write (&writer->tcs, &zero, 1);

  /* cond_string */
  if (tp->cond_string != NULL)
    ctf_save_write (&writer->tcs, (gdb_byte *) tp->cond_string.get (),
		    strlen (tp->cond_string.get ()));
  ctf_save_write (&writer->tcs, &zero, 1);

  /* cmd_strings */
  u32 = tp->cmd_strings.size ();
  ctf_save_align_write (&writer->tcs, (gdb_byte *) &u32, 4, 4);
  for (const auto &act : tp->cmd_strings)
    ctf_save_write (&writer->tcs, (gdb_byte *) act.get (),
		    strlen (act.get ()) + 1);

}

/* This is the implementation of trace_file_write_ops method
   write_tdesc.  */

static void
ctf_write_tdesc (struct trace_file_writer *self)
{
  /* Nothing so far. */
}

/* This is the implementation of trace_file_write_ops method
   write_definition_end.  */

static void
ctf_write_definition_end (struct trace_file_writer *self)
{
  self->ops->frame_ops->end (self);
}

/* This is the implementation of trace_file_write_ops method
   end.  */

static void
ctf_end (struct trace_file_writer *self)
{
  struct ctf_trace_file_writer *writer = (struct ctf_trace_file_writer *) self;

  gdb_assert (writer->tcs.content_size == 0);
}

/* This is the implementation of trace_frame_write_ops method
   start.  */

static void
ctf_write_frame_start (struct trace_file_writer *self, uint16_t tpnum)
{
  struct ctf_trace_file_writer *writer
    = (struct ctf_trace_file_writer *) self;
  uint32_t id = CTF_EVENT_ID_FRAME;
  uint32_t u32;

  /* Step 1: Write packet context.  */
  /* magic.  */
  u32 = CTF_MAGIC;
  ctf_save_write_uint32 (&writer->tcs, u32);
  /* content_size and packet_size..  We still don't know the value,
     write it later.  */
  ctf_save_fseek (&writer->tcs, 4, SEEK_CUR);
  ctf_save_fseek (&writer->tcs, 4, SEEK_CUR);
  /* Tracepoint number.  */
  ctf_save_write (&writer->tcs, (gdb_byte *) &tpnum, 2);

  /* Step 2: Write event "frame".  */
  /* Event Id.  */
  ctf_save_align_write (&writer->tcs, (gdb_byte *) &id, 4, 4);
}

/* This is the implementation of trace_frame_write_ops method
   write_r_block.  */

static void
ctf_write_frame_r_block (struct trace_file_writer *self,
			 gdb_byte *buf, int32_t size)
{
  struct ctf_trace_file_writer *writer
    = (struct ctf_trace_file_writer *) self;
  uint32_t id = CTF_EVENT_ID_REGISTER;

  /* Event Id.  */
  ctf_save_align_write (&writer->tcs, (gdb_byte *) &id, 4, 4);

  /* array contents.  */
  ctf_save_align_write (&writer->tcs, buf, size, 1);
}

/* This is the implementation of trace_frame_write_ops method
   write_m_block_header.  */

static void
ctf_write_frame_m_block_header (struct trace_file_writer *self,
				uint64_t addr, uint16_t length)
{
  struct ctf_trace_file_writer *writer
    = (struct ctf_trace_file_writer *) self;
  uint32_t event_id = CTF_EVENT_ID_MEMORY;

  /* Event Id.  */
  ctf_save_align_write (&writer->tcs, (gdb_byte *) &event_id, 4, 4);

  /* Address.  */
  ctf_save_align_write (&writer->tcs, (gdb_byte *) &addr, 8, 8);

  /* Length.  */
  ctf_save_align_write (&writer->tcs, (gdb_byte *) &length, 2, 2);
}

/* This is the implementation of trace_frame_write_ops method
   write_m_block_memory.  */

static void
ctf_write_frame_m_block_memory (struct trace_file_writer *self,
				gdb_byte *buf, uint16_t length)
{
  struct ctf_trace_file_writer *writer
    = (struct ctf_trace_file_writer *) self;

  /* Contents.  */
  ctf_save_align_write (&writer->tcs, (gdb_byte *) buf, length, 1);
}

/* This is the implementation of trace_frame_write_ops method
   write_v_block.  */

static void
ctf_write_frame_v_block (struct trace_file_writer *self,
			 int32_t num, uint64_t val)
{
  struct ctf_trace_file_writer *writer
    = (struct ctf_trace_file_writer *) self;
  uint32_t id = CTF_EVENT_ID_TSV;

  /* Event Id.  */
  ctf_save_align_write (&writer->tcs, (gdb_byte *) &id, 4, 4);

  /* val.  */
  ctf_save_align_write (&writer->tcs, (gdb_byte *) &val, 8, 8);
  /* num.  */
  ctf_save_align_write (&writer->tcs, (gdb_byte *) &num, 4, 4);
}

/* This is the implementation of trace_frame_write_ops method
   end.  */

static void
ctf_write_frame_end (struct trace_file_writer *self)
{
  struct ctf_trace_file_writer *writer
    = (struct ctf_trace_file_writer *) self;
  uint32_t u32;
  uint32_t t;

  /* Write the content size to packet header.  */
  ctf_save_fseek (&writer->tcs, writer->tcs.packet_start + 4,
		  SEEK_SET);
  u32 = writer->tcs.content_size * TARGET_CHAR_BIT;

  t = writer->tcs.content_size;
  ctf_save_write_uint32 (&writer->tcs, u32);

  /* Write the packet size.  */
  u32 += 4 * TARGET_CHAR_BIT;
  ctf_save_write_uint32 (&writer->tcs, u32);

  writer->tcs.content_size = t;

  /* Write zero at the end of the packet.  */
  ctf_save_fseek (&writer->tcs, writer->tcs.packet_start + t,
		  SEEK_SET);
  u32 = 0;
  ctf_save_write_uint32 (&writer->tcs, u32);
  writer->tcs.content_size = t;

  ctf_save_next_packet (&writer->tcs);
}

/* Operations to write various types of trace frames into CTF
   format.  */

static const struct trace_frame_write_ops ctf_write_frame_ops =
{
  ctf_write_frame_start,
  ctf_write_frame_r_block,
  ctf_write_frame_m_block_header,
  ctf_write_frame_m_block_memory,
  ctf_write_frame_v_block,
  ctf_write_frame_end,
};

/* Operations to write trace buffers into CTF format.  */

static const struct trace_file_write_ops ctf_write_ops =
{
  ctf_dtor,
  ctf_target_save,
  ctf_start,
  ctf_write_header,
  ctf_write_regblock_type,
  ctf_write_status,
  ctf_write_uploaded_tsv,
  ctf_write_uploaded_tp,
  ctf_write_tdesc,
  ctf_write_definition_end,
  NULL,
  &ctf_write_frame_ops,
  ctf_end,
};

/* Return a trace writer for CTF format.  */

struct trace_file_writer *
ctf_trace_file_writer_new (void)
{
  struct ctf_trace_file_writer *writer = XNEW (struct ctf_trace_file_writer);

  writer->base.ops = &ctf_write_ops;

  return (struct trace_file_writer *) writer;
}

#if HAVE_LIBBABELTRACE
/* Use libbabeltrace to read CTF data.  The libbabeltrace provides
   iterator to iterate over each event in CTF data and APIs to get
   details of event and packet, so it is very convenient to use
   libbabeltrace to access events in CTF.  */

#include <babeltrace/babeltrace.h>
#include <babeltrace/ctf/events.h>
#include <babeltrace/ctf/iterator.h>

/* The CTF target.  */

static const target_info ctf_target_info = {
  "ctf",
  N_("CTF file"),
  N_("(Use a CTF directory as a target.\n\
Specify the filename of the CTF directory.")
};

class ctf_target final : public tracefile_target
{
public:
  const target_info &info () const override
  { return ctf_target_info; }

  void close () override;
  void fetch_registers (struct regcache *, int) override;
  enum target_xfer_status xfer_partial (enum target_object object,
						const char *annex,
						gdb_byte *readbuf,
						const gdb_byte *writebuf,
						ULONGEST offset, ULONGEST len,
						ULONGEST *xfered_len) override;
  void files_info () override;
  int trace_find (enum trace_find_type type, int num,
			  CORE_ADDR addr1, CORE_ADDR addr2, int *tpp) override;
  bool get_trace_state_variable_value (int tsv, LONGEST *val) override;
  traceframe_info_up traceframe_info () override;
};

/* The struct pointer for current CTF directory.  */
static struct bt_context *ctx = NULL;
static struct bt_ctf_iter *ctf_iter = NULL;
/* The position of the first packet containing trace frame.  */
static struct bt_iter_pos *start_pos;

/* The name of CTF directory.  */
static gdb::unique_xmalloc_ptr<char> trace_dirname;

static ctf_target ctf_ops;

/* Destroy ctf iterator and context.  */

static void
ctf_destroy (void)
{
  if (ctf_iter != NULL)
    {
      bt_ctf_iter_destroy (ctf_iter);
      ctf_iter = NULL;
    }
  if (ctx != NULL)
    {
      bt_context_put (ctx);
      ctx = NULL;
    }
}

/* Open CTF trace data in DIRNAME.  */

static void
ctf_open_dir (const char *dirname)
{
  struct bt_iter_pos begin_pos;
  unsigned int count, i;
  struct bt_ctf_event_decl * const *list;

  ctx = bt_context_create ();
  if (ctx == NULL)
    error (_("Unable to create bt_context"));
  int handle_id = bt_context_add_trace (ctx, dirname, "ctf", NULL, NULL, NULL);
  if (handle_id < 0)
    {
      ctf_destroy ();
      error (_("Unable to use libbabeltrace on directory \"%s\""),
	     dirname);
    }

  begin_pos.type = BT_SEEK_BEGIN;
  ctf_iter = bt_ctf_iter_create (ctx, &begin_pos, NULL);
  if (ctf_iter == NULL)
    {
      ctf_destroy ();
      error (_("Unable to create bt_iterator"));
    }

  /* Look for the declaration of register block.  Get the length of
     array "contents" to set trace_regblock_size.  */

  bt_ctf_get_event_decl_list (handle_id, ctx, &list, &count);
  for (i = 0; i < count; i++)
    if (strcmp ("register", bt_ctf_get_decl_event_name (list[i])) == 0)
      {
	const struct bt_ctf_field_decl * const *field_list;
	const struct bt_declaration *decl;

	bt_ctf_get_decl_fields (list[i], BT_EVENT_FIELDS, &field_list,
				&count);

	gdb_assert (count == 1);
	gdb_assert (0 == strcmp ("contents",
				 bt_ctf_get_decl_field_name (field_list[0])));
	decl = bt_ctf_get_decl_from_field_decl (field_list[0]);
	trace_regblock_size = bt_ctf_get_array_len (decl);

	break;
      }
}

#define SET_INT32_FIELD(EVENT, SCOPE, VAR, FIELD)			\
  (VAR)->FIELD = (int) bt_ctf_get_int64 (bt_ctf_get_field ((EVENT),	\
							   (SCOPE),	\
							   #FIELD))

#define SET_ENUM_FIELD(EVENT, SCOPE, VAR, TYPE, FIELD)			\
  (VAR)->FIELD = (TYPE) bt_ctf_get_int64 (bt_ctf_get_field ((EVENT),	\
							    (SCOPE),	\
							    #FIELD))


/* EVENT is the "status" event and TS is filled in.  */

static void
ctf_read_status (struct bt_ctf_event *event, struct trace_status *ts)
{
  const struct bt_definition *scope
    = bt_ctf_get_top_level_scope (event, BT_EVENT_FIELDS);

  SET_ENUM_FIELD (event, scope, ts, enum trace_stop_reason, stop_reason);
  SET_INT32_FIELD (event, scope, ts, stopping_tracepoint);
  SET_INT32_FIELD (event, scope, ts, traceframe_count);
  SET_INT32_FIELD (event, scope, ts, traceframes_created);
  SET_INT32_FIELD (event, scope, ts, buffer_free);
  SET_INT32_FIELD (event, scope, ts, buffer_size);
  SET_INT32_FIELD (event, scope, ts, disconnected_tracing);
  SET_INT32_FIELD (event, scope, ts, circular_buffer);

  bt_iter_next (bt_ctf_get_iter (ctf_iter));
}

/* Read the events "tsv_def" one by one, extract its contents and fill
   in the list UPLOADED_TSVS.  */

static void
ctf_read_tsv (struct uploaded_tsv **uploaded_tsvs)
{
  gdb_assert (ctf_iter != NULL);

  while (1)
    {
      struct bt_ctf_event *event;
      const struct bt_definition *scope;
      const struct bt_definition *def;
      uint32_t event_id;
      struct uploaded_tsv *utsv = NULL;

      event = bt_ctf_iter_read_event (ctf_iter);
      scope = bt_ctf_get_top_level_scope (event,
					  BT_STREAM_EVENT_HEADER);
      event_id = bt_ctf_get_uint64 (bt_ctf_get_field (event, scope,
						      "id"));
      if (event_id != CTF_EVENT_ID_TSV_DEF)
	break;

      scope = bt_ctf_get_top_level_scope (event,
					  BT_EVENT_FIELDS);

      def = bt_ctf_get_field (event, scope, "number");
      utsv = get_uploaded_tsv ((int32_t) bt_ctf_get_int64 (def),
			       uploaded_tsvs);

      def = bt_ctf_get_field (event, scope, "builtin");
      utsv->builtin = (int32_t) bt_ctf_get_int64 (def);
      def = bt_ctf_get_field (event, scope, "initial_value");
      utsv->initial_value = bt_ctf_get_int64 (def);

      def = bt_ctf_get_field (event, scope, "name");
      utsv->name =  xstrdup (bt_ctf_get_string (def));

      if (bt_iter_next (bt_ctf_get_iter (ctf_iter)) < 0)
	break;
    }

}

/* Read the value of element whose index is NUM from CTF and write it
   to the corresponding VAR->ARRAY. */

#define SET_ARRAY_FIELD(EVENT, SCOPE, VAR, NUM, ARRAY)	\
  do							\
    {							\
      uint32_t lu32, i;						\
      const struct bt_definition *def;				\
								\
      lu32 = (uint32_t) bt_ctf_get_uint64 (bt_ctf_get_field ((EVENT),	\
							     (SCOPE),	\
							     #NUM));	\
      def = bt_ctf_get_field ((EVENT), (SCOPE), #ARRAY);		\
      for (i = 0; i < lu32; i++)					\
	{								\
	  const struct bt_definition *element				\
	    = bt_ctf_get_index ((EVENT), def, i);			\
									\
	  (VAR)->ARRAY.emplace_back					\
	    (xstrdup (bt_ctf_get_string (element)));			\
	}								\
    }									\
  while (0)

/* Read a string from CTF and set VAR->FIELD. If the length of string
   is zero, set VAR->FIELD to NULL.  */

#define SET_STRING_FIELD(EVENT, SCOPE, VAR, FIELD)			\
  do									\
    {									\
      const char *p = bt_ctf_get_string (bt_ctf_get_field ((EVENT),	\
							   (SCOPE),	\
							   #FIELD));	\
									\
      if (strlen (p) > 0)						\
	(VAR)->FIELD.reset (xstrdup (p));				\
      else								\
	(VAR)->FIELD = NULL;						\
    }									\
  while (0)

/* Read the events "tp_def" one by one, extract its contents and fill
   in the list UPLOADED_TPS.  */

static void
ctf_read_tp (struct uploaded_tp **uploaded_tps)
{
  gdb_assert (ctf_iter != NULL);

  while (1)
    {
      struct bt_ctf_event *event;
      const struct bt_definition *scope;
      uint32_t u32;
      int32_t int32;
      uint64_t u64;
      struct uploaded_tp *utp = NULL;

      event = bt_ctf_iter_read_event (ctf_iter);
      scope = bt_ctf_get_top_level_scope (event,
					  BT_STREAM_EVENT_HEADER);
      u32 = bt_ctf_get_uint64 (bt_ctf_get_field (event, scope,
						 "id"));
      if (u32 != CTF_EVENT_ID_TP_DEF)
	break;

      scope = bt_ctf_get_top_level_scope (event,
					  BT_EVENT_FIELDS);
      int32 = (int32_t) bt_ctf_get_int64 (bt_ctf_get_field (event,
							    scope,
							    "number"));
      u64 = bt_ctf_get_uint64 (bt_ctf_get_field (event, scope,
						 "addr"));
      utp = get_uploaded_tp (int32, u64,  uploaded_tps);

      SET_INT32_FIELD (event, scope, utp, enabled);
      SET_INT32_FIELD (event, scope, utp, step);
      SET_INT32_FIELD (event, scope, utp, pass);
      SET_INT32_FIELD (event, scope, utp, hit_count);
      SET_ENUM_FIELD (event, scope, utp, enum bptype, type);

      /* Read 'cmd_strings'.  */
      SET_ARRAY_FIELD (event, scope, utp, cmd_num, cmd_strings);
      /* Read 'actions'.  */
      SET_ARRAY_FIELD (event, scope, utp, action_num, actions);
      /* Read 'step_actions'.  */
      SET_ARRAY_FIELD (event, scope, utp, step_action_num,
		       step_actions);

      SET_STRING_FIELD(event, scope, utp, at_string);
      SET_STRING_FIELD(event, scope, utp, cond_string);

      if (bt_iter_next (bt_ctf_get_iter (ctf_iter)) < 0)
	break;
    }
}

/* This is the implementation of target_ops method to_open.  Open CTF
   trace data, read trace status, trace state variables and tracepoint
   definitions from the first packet.  Set the start position at the
   second packet which contains events on trace blocks.  */

static void
ctf_target_open (const char *dirname, int from_tty)
{
  struct bt_ctf_event *event;
  uint32_t event_id;
  const struct bt_definition *scope;
  struct uploaded_tsv *uploaded_tsvs = NULL;
  struct uploaded_tp *uploaded_tps = NULL;

  if (!dirname)
    error (_("No CTF directory specified."));

  ctf_open_dir (dirname);

  target_preopen (from_tty);

  /* Skip the first packet which about the trace status.  The first
     event is "frame".  */
  event = bt_ctf_iter_read_event (ctf_iter);
  scope = bt_ctf_get_top_level_scope (event, BT_STREAM_EVENT_HEADER);
  event_id = bt_ctf_get_uint64 (bt_ctf_get_field (event, scope, "id"));
  if (event_id != CTF_EVENT_ID_FRAME)
    error (_("Wrong event id of the first event"));
  /* The second event is "status".  */
  bt_iter_next (bt_ctf_get_iter (ctf_iter));
  event = bt_ctf_iter_read_event (ctf_iter);
  scope = bt_ctf_get_top_level_scope (event, BT_STREAM_EVENT_HEADER);
  event_id = bt_ctf_get_uint64 (bt_ctf_get_field (event, scope, "id"));
  if (event_id != CTF_EVENT_ID_STATUS)
    error (_("Wrong event id of the second event"));
  ctf_read_status (event, current_trace_status ());

  ctf_read_tsv (&uploaded_tsvs);

  ctf_read_tp (&uploaded_tps);

  event = bt_ctf_iter_read_event (ctf_iter);
  /* EVENT can be NULL if we've already gone to the end of stream of
     events.  */
  if (event != NULL)
    {
      scope = bt_ctf_get_top_level_scope (event,
					  BT_STREAM_EVENT_HEADER);
      event_id = bt_ctf_get_uint64 (bt_ctf_get_field (event,
						      scope, "id"));
      if (event_id != CTF_EVENT_ID_FRAME)
	error (_("Wrong event id of the first event of the second packet"));
    }

  start_pos = bt_iter_get_pos (bt_ctf_get_iter (ctf_iter));
  gdb_assert (start_pos->type == BT_SEEK_RESTORE);

  trace_dirname = make_unique_xstrdup (dirname);
  current_inferior ()->push_target (&ctf_ops);

  inferior_appeared (current_inferior (), CTF_PID);

  thread_info *thr = add_thread_silent (&ctf_ops, ptid_t (CTF_PID));
  switch_to_thread (thr);

  merge_uploaded_trace_state_variables (&uploaded_tsvs);
  merge_uploaded_tracepoints (&uploaded_tps);

  post_create_inferior (from_tty);
}

/* This is the implementation of target_ops method to_close.  Destroy
   CTF iterator and context.  */

void
ctf_target::close ()
{
  ctf_destroy ();
  trace_dirname.reset ();

  switch_to_no_thread ();	/* Avoid confusion from thread stuff.  */
  exit_inferior (current_inferior ());

  trace_reset_local_state ();
}

/* This is the implementation of target_ops method to_files_info.
   Print the directory name of CTF trace data.  */

void
ctf_target::files_info ()
{
  gdb_printf ("\t`%s'\n", trace_dirname.get ());
}

/* This is the implementation of target_ops method to_fetch_registers.
   Iterate over events whose name is "register" in current frame,
   extract contents from events, and set REGCACHE with the contents.
   If no matched events are found, mark registers unavailable.  */

void
ctf_target::fetch_registers (struct regcache *regcache, int regno)
{
  struct gdbarch *gdbarch = regcache->arch ();
  struct bt_ctf_event *event = NULL;
  struct bt_iter_pos *pos;

  /* An uninitialized reg size says we're not going to be
     successful at getting register blocks.  */
  if (trace_regblock_size == 0)
    return;

  gdb_assert (ctf_iter != NULL);
  /* Save the current position.  */
  pos = bt_iter_get_pos (bt_ctf_get_iter (ctf_iter));
  gdb_assert (pos->type == BT_SEEK_RESTORE);

  while (1)
    {
      const char *name;
      struct bt_ctf_event *event1;

      event1 = bt_ctf_iter_read_event (ctf_iter);

      name = bt_ctf_event_name (event1);

      if (name == NULL || strcmp (name, "frame") == 0)
	break;
      else if (strcmp (name, "register") == 0)
	{
	  event = event1;
	  break;
	}

      if (bt_iter_next (bt_ctf_get_iter (ctf_iter)) < 0)
	break;
    }

  /* Restore the position.  */
  bt_iter_set_pos (bt_ctf_get_iter (ctf_iter), pos);

  if (event != NULL)
    {
      int offset, regsize, regn;
      const struct bt_definition *scope
	= bt_ctf_get_top_level_scope (event,
				      BT_EVENT_FIELDS);
      const struct bt_definition *array
	= bt_ctf_get_field (event, scope, "contents");
      gdb_byte *regs = (gdb_byte *) bt_ctf_get_char_array (array);

      /* Assume the block is laid out in GDB register number order,
	 each register with the size that it has in GDB.  */
      offset = 0;
      for (regn = 0; regn < gdbarch_num_regs (gdbarch); regn++)
	{
	  regsize = register_size (gdbarch, regn);
	  /* Make sure we stay within block bounds.  */
	  if (offset + regsize >= trace_regblock_size)
	    break;
	  if (regcache->get_register_status (regn) == REG_UNKNOWN)
	    {
	      if (regno == regn)
		{
		  regcache->raw_supply (regno, regs + offset);
		  break;
		}
	      else if (regno == -1)
		{
		  regcache->raw_supply (regn, regs + offset);
		}
	    }
	  offset += regsize;
	}
    }
  else
    tracefile_fetch_registers (regcache, regno);
}

/* This is the implementation of target_ops method to_xfer_partial.
   Iterate over events whose name is "memory" in
   current frame, extract the address and length from events.  If
   OFFSET is within the range, read the contents from events to
   READBUF.  */

enum target_xfer_status
ctf_target::xfer_partial (enum target_object object,
			  const char *annex, gdb_byte *readbuf,
			  const gdb_byte *writebuf, ULONGEST offset,
			  ULONGEST len, ULONGEST *xfered_len)
{
  /* We're only doing regular memory for now.  */
  if (object != TARGET_OBJECT_MEMORY)
    return TARGET_XFER_E_IO;

  if (readbuf == NULL)
    error (_("ctf_xfer_partial: trace file is read-only"));

  if (get_traceframe_number () != -1)
    {
      struct bt_iter_pos *pos;
      enum target_xfer_status res;
      /* Records the lowest available address of all blocks that
	 intersects the requested range.  */
      ULONGEST low_addr_available = 0;

      gdb_assert (ctf_iter != NULL);
      /* Save the current position.  */
      pos = bt_iter_get_pos (bt_ctf_get_iter (ctf_iter));
      gdb_assert (pos->type == BT_SEEK_RESTORE);

      /* Iterate through the traceframe's blocks, looking for
	 memory.  */
      while (1)
	{
	  ULONGEST amt;
	  uint64_t maddr;
	  uint16_t mlen;
	  const struct bt_definition *scope;
	  const struct bt_definition *def;
	  struct bt_ctf_event *event
	    = bt_ctf_iter_read_event (ctf_iter);
	  const char *name = bt_ctf_event_name (event);

	  if (name == NULL || strcmp (name, "frame") == 0)
	    break;
	  else if (strcmp (name, "memory") != 0)
	    {
	      if (bt_iter_next (bt_ctf_get_iter (ctf_iter)) < 0)
		break;

	      continue;
	    }

	  scope = bt_ctf_get_top_level_scope (event,
					      BT_EVENT_FIELDS);

	  def = bt_ctf_get_field (event, scope, "address");
	  maddr = bt_ctf_get_uint64 (def);
	  def = bt_ctf_get_field (event, scope, "length");
	  mlen = (uint16_t) bt_ctf_get_uint64 (def);

	  /* If the block includes the first part of the desired
	     range, return as much it has; GDB will re-request the
	     remainder, which might be in a different block of this
	     trace frame.  */
	  if (maddr <= offset && offset < (maddr + mlen))
	    {
	      const struct bt_definition *array
		= bt_ctf_get_field (event, scope, "contents");
	      int k;

	      gdb::byte_vector contents (mlen);

	      for (k = 0; k < mlen; k++)
		{
		  const struct bt_definition *element
		    = bt_ctf_get_index (event, array, k);

		  contents[k] = (gdb_byte) bt_ctf_get_uint64 (element);
		}

	      amt = (maddr + mlen) - offset;
	      if (amt > len)
		amt = len;

	      memcpy (readbuf, &contents[offset - maddr], amt);

	      /* Restore the position.  */
	      bt_iter_set_pos (bt_ctf_get_iter (ctf_iter), pos);

	      if (amt == 0)
		return TARGET_XFER_EOF;
	      else
		{
		  *xfered_len = amt;
		  return TARGET_XFER_OK;
		}
	    }

	  if (offset < maddr && maddr < (offset + len))
	    if (low_addr_available == 0 || low_addr_available > maddr)
	      low_addr_available = maddr;

	  if (bt_iter_next (bt_ctf_get_iter (ctf_iter)) < 0)
	    break;
	}

      /* Restore the position.  */
      bt_iter_set_pos (bt_ctf_get_iter (ctf_iter), pos);

      /* Requested memory is unavailable in the context of traceframes,
	 and this address falls within a read-only section, fallback
	 to reading from executable, up to LOW_ADDR_AVAILABLE  */
      if (offset < low_addr_available)
	len = std::min (len, low_addr_available - offset);
      res = exec_read_partial_read_only (readbuf, offset, len, xfered_len);

      if (res == TARGET_XFER_OK)
	return TARGET_XFER_OK;
      else
	{
	  /* No use trying further, we know some memory starting
	     at MEMADDR isn't available.  */
	  *xfered_len = len;
	  return TARGET_XFER_UNAVAILABLE;
	}
    }
  else
    {
      /* Fallback to reading from read-only sections.  */
      return section_table_read_available_memory (readbuf, offset, len, xfered_len);
    }
}

/* This is the implementation of target_ops method
   to_get_trace_state_variable_value.
   Iterate over events whose name is "tsv" in current frame.  When the
   trace variable is found, set the value of it to *VAL and return
   true, otherwise return false.  */

bool
ctf_target::get_trace_state_variable_value (int tsvnum, LONGEST *val)
{
  struct bt_iter_pos *pos;
  bool found = false;

  gdb_assert (ctf_iter != NULL);
  /* Save the current position.  */
  pos = bt_iter_get_pos (bt_ctf_get_iter (ctf_iter));
  gdb_assert (pos->type == BT_SEEK_RESTORE);

  /* Iterate through the traceframe's blocks, looking for 'V'
     block.  */
  while (1)
    {
      struct bt_ctf_event *event
	= bt_ctf_iter_read_event (ctf_iter);
      const char *name = bt_ctf_event_name (event);

      if (name == NULL || strcmp (name, "frame") == 0)
	break;
      else if (strcmp (name, "tsv") == 0)
	{
	  const struct bt_definition *scope;
	  const struct bt_definition *def;

	  scope = bt_ctf_get_top_level_scope (event,
					      BT_EVENT_FIELDS);

	  def = bt_ctf_get_field (event, scope, "num");
	  if (tsvnum == (int32_t) bt_ctf_get_uint64 (def))
	    {
	      def = bt_ctf_get_field (event, scope, "val");
	      *val = bt_ctf_get_uint64 (def);

	      found = true;
	    }
	}

      if (bt_iter_next (bt_ctf_get_iter (ctf_iter)) < 0)
	break;
    }

  /* Restore the position.  */
  bt_iter_set_pos (bt_ctf_get_iter (ctf_iter), pos);

  return found;
}

/* Return the tracepoint number in "frame" event.  */

static int
ctf_get_tpnum_from_frame_event (struct bt_ctf_event *event)
{
  /* The packet context of events has a field "tpnum".  */
  const struct bt_definition *scope
    = bt_ctf_get_top_level_scope (event, BT_STREAM_PACKET_CONTEXT);
  uint64_t tpnum
    = bt_ctf_get_uint64 (bt_ctf_get_field (event, scope, "tpnum"));

  return (int) tpnum;
}

/* Return the address at which the current frame was collected.  */

static CORE_ADDR
ctf_get_traceframe_address (void)
{
  struct bt_ctf_event *event = NULL;
  struct bt_iter_pos *pos;
  CORE_ADDR addr = 0;

  gdb_assert (ctf_iter != NULL);
  pos  = bt_iter_get_pos (bt_ctf_get_iter (ctf_iter));
  gdb_assert (pos->type == BT_SEEK_RESTORE);

  while (1)
    {
      const char *name;
      struct bt_ctf_event *event1;

      event1 = bt_ctf_iter_read_event (ctf_iter);

      name = bt_ctf_event_name (event1);

      if (name == NULL)
	break;
      else if (strcmp (name, "frame") == 0)
	{
	  event = event1;
	  break;
	}

      if (bt_iter_next (bt_ctf_get_iter (ctf_iter)) < 0)
	break;
    }

  if (event != NULL)
    {
      int tpnum = ctf_get_tpnum_from_frame_event (event);
      struct tracepoint *tp
	= get_tracepoint_by_number_on_target (tpnum);

      if (tp != nullptr && tp->has_locations ())
	addr = tp->first_loc ().address;
    }

  /* Restore the position.  */
  bt_iter_set_pos (bt_ctf_get_iter (ctf_iter), pos);

  return addr;
}

/* This is the implementation of target_ops method to_trace_find.
   Iterate the events whose name is "frame", extract the tracepoint
   number in it.  Return traceframe number when matched.  */

int
ctf_target::trace_find (enum trace_find_type type, int num,
			CORE_ADDR addr1, CORE_ADDR addr2, int *tpp)
{
  int tfnum = 0;
  int found = 0;

  if (num == -1)
    {
      if (tpp != NULL)
	*tpp = -1;
      return -1;
    }

  gdb_assert (ctf_iter != NULL);
  /* Set iterator back to the start.  */
  bt_iter_set_pos (bt_ctf_get_iter (ctf_iter), start_pos);

  while (1)
    {
      struct bt_ctf_event *event;
      const char *name;

      event = bt_ctf_iter_read_event (ctf_iter);

      name = bt_ctf_event_name (event);

      if (event == NULL || name == NULL)
	break;

      if (strcmp (name, "frame") == 0)
	{
	  CORE_ADDR tfaddr;

	  if (type == tfind_number)
	    {
	      /* Looking for a specific trace frame.  */
	      if (tfnum == num)
		found = 1;
	    }
	  else
	    {
	      /* Start from the _next_ trace frame.  */
	      if (tfnum > get_traceframe_number ())
		{
		  switch (type)
		    {
		    case tfind_tp:
		      {
			struct tracepoint *tp = get_tracepoint (num);

			if (tp != NULL
			    && (tp->number_on_target
				== ctf_get_tpnum_from_frame_event (event)))
			  found = 1;
			break;
		      }
		    case tfind_pc:
		      tfaddr = ctf_get_traceframe_address ();
		      if (tfaddr == addr1)
			found = 1;
		      break;
		    case tfind_range:
		      tfaddr = ctf_get_traceframe_address ();
		      if (addr1 <= tfaddr && tfaddr <= addr2)
			found = 1;
		      break;
		    case tfind_outside:
		      tfaddr = ctf_get_traceframe_address ();
		      if (!(addr1 <= tfaddr && tfaddr <= addr2))
			found = 1;
		      break;
		    default:
		      internal_error (_("unknown tfind type"));
		    }
		}
	    }
	  if (found)
	    {
	      if (tpp != NULL)
		*tpp = ctf_get_tpnum_from_frame_event (event);

	      /* Skip the event "frame".  */
	      bt_iter_next (bt_ctf_get_iter (ctf_iter));

	      return tfnum;
	    }
	  tfnum++;
	}

      if (bt_iter_next (bt_ctf_get_iter (ctf_iter)) < 0)
	break;
    }

  return -1;
}

/* This is the implementation of target_ops method to_traceframe_info.
   Iterate the events whose name is "memory", in current
   frame, extract memory range information, and return them in
   traceframe_info.  */

traceframe_info_up
ctf_target::traceframe_info ()
{
  traceframe_info_up info (new struct traceframe_info);
  const char *name;
  struct bt_iter_pos *pos;

  gdb_assert (ctf_iter != NULL);
  /* Save the current position.  */
  pos = bt_iter_get_pos (bt_ctf_get_iter (ctf_iter));
  gdb_assert (pos->type == BT_SEEK_RESTORE);

  do
    {
      struct bt_ctf_event *event
	= bt_ctf_iter_read_event (ctf_iter);

      name = bt_ctf_event_name (event);

      if (name == NULL || strcmp (name, "register") == 0
	  || strcmp (name, "frame") == 0)
	;
      else if (strcmp (name, "memory") == 0)
	{
	  const struct bt_definition *scope
	    = bt_ctf_get_top_level_scope (event,
					  BT_EVENT_FIELDS);
	  const struct bt_definition *def;

	  def = bt_ctf_get_field (event, scope, "address");
	  CORE_ADDR start = bt_ctf_get_uint64 (def);

	  def = bt_ctf_get_field (event, scope, "length");
	  int length = (uint16_t) bt_ctf_get_uint64 (def);

	  info->memory.emplace_back (start, length);
	}
      else if (strcmp (name, "tsv") == 0)
	{
	  int vnum;
	  const struct bt_definition *scope
	    = bt_ctf_get_top_level_scope (event,
					  BT_EVENT_FIELDS);
	  const struct bt_definition *def;

	  def = bt_ctf_get_field (event, scope, "num");
	  vnum = (int) bt_ctf_get_uint64 (def);
	  info->tvars.push_back (vnum);
	}
      else
	{
	  warning (_("Unhandled trace block type (%s) "
		     "while building trace frame info."),
		   name);
	}

      if (bt_iter_next (bt_ctf_get_iter (ctf_iter)) < 0)
	break;
    }
  while (name != NULL && strcmp (name, "frame") != 0);

  /* Restore the position.  */
  bt_iter_set_pos (bt_ctf_get_iter (ctf_iter), pos);

  return info;
}

#endif

/* module initialization */

void _initialize_ctf ();
void
_initialize_ctf ()
{
#if HAVE_LIBBABELTRACE
  add_target (ctf_target_info, ctf_target_open, filename_completer);
#endif
}
