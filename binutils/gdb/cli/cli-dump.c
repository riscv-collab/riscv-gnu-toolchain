/* Dump-to-file commands, for GDB, the GNU debugger.

   Copyright (C) 2002-2024 Free Software Foundation, Inc.

   Contributed by Red Hat.

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
#include "cli/cli-decode.h"
#include "cli/cli-cmds.h"
#include "value.h"
#include "completer.h"
#include <ctype.h>
#include "target.h"
#include "readline/tilde.h"
#include "gdbcore.h"
#include "cli/cli-utils.h"
#include "gdb_bfd.h"
#include "gdbsupport/filestuff.h"
#include "gdbsupport/byte-vector.h"
#include "gdbarch.h"
#include "inferior.h"

static gdb::unique_xmalloc_ptr<char>
scan_expression (const char **cmd, const char *def)
{
  if ((*cmd) == NULL || (**cmd) == '\0')
    return make_unique_xstrdup (def);
  else
    {
      char *exp;
      const char *end;

      end = (*cmd) + strcspn (*cmd, " \t");
      exp = savestring ((*cmd), end - (*cmd));
      (*cmd) = skip_spaces (end);
      return gdb::unique_xmalloc_ptr<char> (exp);
    }
}


static gdb::unique_xmalloc_ptr<char>
scan_filename (const char **cmd, const char *defname)
{
  gdb::unique_xmalloc_ptr<char> filename;

  /* FIXME: Need to get the ``/a(ppend)'' flag from somewhere.  */

  /* File.  */
  if ((*cmd) == NULL)
    {
      if (defname == NULL)
	error (_("Missing filename."));
      filename.reset (xstrdup (defname));
    }
  else
    {
      /* FIXME: should parse a possibly quoted string.  */
      const char *end;

      (*cmd) = skip_spaces (*cmd);
      end = *cmd + strcspn (*cmd, " \t");
      filename.reset (savestring ((*cmd), end - (*cmd)));
      (*cmd) = skip_spaces (end);
    }
  gdb_assert (filename != NULL);

  return gdb::unique_xmalloc_ptr<char> (tilde_expand (filename.get ()));
}

static gdb_bfd_ref_ptr
bfd_openr_or_error (const char *filename, const char *target)
{
  gdb_bfd_ref_ptr ibfd (gdb_bfd_openr (filename, target));
  if (ibfd == NULL)
    error (_("Failed to open %s: %s."), filename,
	   bfd_errmsg (bfd_get_error ()));

  if (!bfd_check_format (ibfd.get (), bfd_object))
    error (_("'%s' is not a recognized file format."), filename);

  return ibfd;
}

static gdb_bfd_ref_ptr
bfd_openw_or_error (const char *filename, const char *target, const char *mode)
{
  gdb_bfd_ref_ptr obfd;

  if (*mode == 'w')	/* Write: create new file */
    {
      obfd = gdb_bfd_openw (filename, target);
      if (obfd == NULL)
	error (_("Failed to open %s: %s."), filename,
	       bfd_errmsg (bfd_get_error ()));
      if (!bfd_set_format (obfd.get (), bfd_object))
	error (_("bfd_openw_or_error: %s."), bfd_errmsg (bfd_get_error ()));
    }
  else if (*mode == 'a')	/* Append to existing file.  */
    {	/* FIXME -- doesn't work...  */
      error (_("bfd_openw does not work with append."));
    }
  else
    error (_("bfd_openw_or_error: unknown mode %s."), mode);

  return obfd;
}

static struct cmd_list_element *dump_cmdlist;
static struct cmd_list_element *append_cmdlist;
static struct cmd_list_element *srec_cmdlist;
static struct cmd_list_element *ihex_cmdlist;
static struct cmd_list_element *verilog_cmdlist;
static struct cmd_list_element *tekhex_cmdlist;
static struct cmd_list_element *binary_dump_cmdlist;
static struct cmd_list_element *binary_append_cmdlist;

static void
dump_binary_file (const char *filename, const char *mode, 
		  const bfd_byte *buf, ULONGEST len)
{
  int status;

  gdb_file_up file = gdb_fopen_cloexec (filename, mode);
  if (file == nullptr)
    perror_with_name (filename);

  status = fwrite (buf, len, 1, file.get ());
  if (status != 1)
    perror_with_name (filename);
}

static void
dump_bfd_file (const char *filename, const char *mode, 
	       const char *target, CORE_ADDR vaddr, 
	       const bfd_byte *buf, ULONGEST len)
{
  asection *osection;

  gdb_bfd_ref_ptr obfd (bfd_openw_or_error (filename, target, mode));
  osection = bfd_make_section_anyway (obfd.get (), ".newsec");
  bfd_set_section_size (osection, len);
  bfd_set_section_vma (osection, vaddr);
  bfd_set_section_alignment (osection, 0);
  bfd_set_section_flags (osection, (SEC_HAS_CONTENTS | SEC_ALLOC | SEC_LOAD));
  osection->entsize = 0;
  if (!bfd_set_section_contents (obfd.get (), osection, buf, 0, len))
    warning (_("writing dump file '%s' (%s)"), filename,
	     bfd_errmsg (bfd_get_error ()));
}

static void
dump_memory_to_file (const char *cmd, const char *mode, const char *file_format)
{
  CORE_ADDR lo;
  CORE_ADDR hi;
  ULONGEST count;
  const char *hi_exp;

  /* Open the file.  */
  gdb::unique_xmalloc_ptr<char> filename = scan_filename (&cmd, NULL);

  /* Find the low address.  */
  if (cmd == NULL || *cmd == '\0')
    error (_("Missing start address."));
  gdb::unique_xmalloc_ptr<char> lo_exp = scan_expression (&cmd, NULL);

  /* Find the second address - rest of line.  */
  if (cmd == NULL || *cmd == '\0')
    error (_("Missing stop address."));
  hi_exp = cmd;

  lo = parse_and_eval_address (lo_exp.get ());
  hi = parse_and_eval_address (hi_exp);
  if (hi <= lo)
    error (_("Invalid memory address range (start >= end)."));
  count = hi - lo;

  /* FIXME: Should use read_memory_partial() and a magic blocking
     value.  */
  gdb::byte_vector buf (count);
  read_memory (lo, buf.data (), count);
  
  /* Have everything.  Open/write the data.  */
  if (file_format == NULL || strcmp (file_format, "binary") == 0)
    dump_binary_file (filename.get (), mode, buf.data (), count);
  else
    dump_bfd_file (filename.get (), mode, file_format, lo, buf.data (), count);
}

static void
dump_memory_command (const char *cmd, const char *mode)
{
  dump_memory_to_file (cmd, mode, "binary");
}

static void
dump_value_to_file (const char *cmd, const char *mode, const char *file_format)
{
  struct value *val;

  /* Open the file.  */
  gdb::unique_xmalloc_ptr<char> filename = scan_filename (&cmd, NULL);

  /* Find the value.  */
  if (cmd == NULL || *cmd == '\0')
    error (_("No value to %s."), *mode == 'a' ? "append" : "dump");
  val = parse_and_eval (cmd);
  if (val == NULL)
    error (_("Invalid expression."));

  /* Have everything.  Open/write the data.  */
  if (file_format == NULL || strcmp (file_format, "binary") == 0)
    dump_binary_file (filename.get (), mode, val->contents ().data (),
		      val->type ()->length ());
  else
    {
      CORE_ADDR vaddr;

      if (val->lval ())
	{
	  vaddr = val->address ();
	}
      else
	{
	  vaddr = 0;
	  warning (_("value is not an lval: address assumed to be zero"));
	}

      dump_bfd_file (filename.get (), mode, file_format, vaddr,
		     val->contents ().data (), 
		     val->type ()->length ());
    }
}

static void
dump_value_command (const char *cmd, const char *mode)
{
  dump_value_to_file (cmd, mode, "binary");
}

static void
dump_srec_memory (const char *args, int from_tty)
{
  dump_memory_to_file (args, FOPEN_WB, "srec");
}

static void
dump_srec_value (const char *args, int from_tty)
{
  dump_value_to_file (args, FOPEN_WB, "srec");
}

static void
dump_ihex_memory (const char *args, int from_tty)
{
  dump_memory_to_file (args, FOPEN_WB, "ihex");
}

static void
dump_ihex_value (const char *args, int from_tty)
{
  dump_value_to_file (args, FOPEN_WB, "ihex");
}

static void
dump_verilog_memory (const char *args, int from_tty)
{
  dump_memory_to_file (args, FOPEN_WB, "verilog");
}

static void
dump_verilog_value (const char *args, int from_tty)
{
  dump_value_to_file (args, FOPEN_WB, "verilog");
}

static void
dump_tekhex_memory (const char *args, int from_tty)
{
  dump_memory_to_file (args, FOPEN_WB, "tekhex");
}

static void
dump_tekhex_value (const char *args, int from_tty)
{
  dump_value_to_file (args, FOPEN_WB, "tekhex");
}

static void
dump_binary_memory (const char *args, int from_tty)
{
  dump_memory_to_file (args, FOPEN_WB, "binary");
}

static void
dump_binary_value (const char *args, int from_tty)
{
  dump_value_to_file (args, FOPEN_WB, "binary");
}

static void
append_binary_memory (const char *args, int from_tty)
{
  dump_memory_to_file (args, FOPEN_AB, "binary");
}

static void
append_binary_value (const char *args, int from_tty)
{
  dump_value_to_file (args, FOPEN_AB, "binary");
}

struct dump_context
{
  void (*func) (const char *cmd, const char *mode);
  const char *mode;
};

static void
call_dump_func (const char *args, int from_tty, cmd_list_element *c)
{
  struct dump_context *d = (struct dump_context *) c->context ();

  d->func (args, d->mode);
}

static void
add_dump_command (const char *name,
		  void (*func) (const char *args, const char *mode),
		  const char *descr)

{
  struct cmd_list_element *c;
  struct dump_context *d;

  c = add_cmd (name, all_commands, descr, &dump_cmdlist);
  c->completer =  filename_completer;
  d = XNEW (struct dump_context);
  d->func = func;
  d->mode = FOPEN_WB;
  c->set_context (d);
  c->func = call_dump_func;

  c = add_cmd (name, all_commands, descr, &append_cmdlist);
  c->completer =  filename_completer;
  d = XNEW (struct dump_context);
  d->func = func;
  d->mode = FOPEN_AB;
  c->set_context (d);
  c->func = call_dump_func;

  /* Replace "Dump " at start of docstring with "Append " (borrowed
     from [deleted] deprecated_add_show_from_set).  */
  if (   c->doc[0] == 'W' 
      && c->doc[1] == 'r' 
      && c->doc[2] == 'i'
      && c->doc[3] == 't' 
      && c->doc[4] == 'e'
      && c->doc[5] == ' ')
    c->doc = concat ("Append ", c->doc + 6, (char *)NULL);
}

/* Selectively loads the sections into memory.  */

static void
restore_one_section (bfd *ibfd, asection *isec,
		     CORE_ADDR load_offset,
		     CORE_ADDR load_start,
		     CORE_ADDR load_end)
{
  bfd_vma sec_start  = bfd_section_vma (isec);
  bfd_size_type size = bfd_section_size (isec);
  bfd_vma sec_end    = sec_start + size;
  bfd_size_type sec_offset = 0;
  bfd_size_type sec_load_count = size;
  int ret;

  /* Ignore non-loadable sections, eg. from elf files.  */
  if (!(bfd_section_flags (isec) & SEC_LOAD))
    return;

  /* Does the section overlap with the desired restore range? */
  if (sec_end <= load_start
      || (load_end > 0 && sec_start >= load_end))
    {
      /* No, no useable data in this section.  */
      gdb_printf (_("skipping section %s...\n"), 
		  bfd_section_name (isec));
      return;
    }

  /* Compare section address range with user-requested
     address range (if any).  Compute where the actual
     transfer should start and end.  */
  if (sec_start < load_start)
    sec_offset = load_start - sec_start;
  /* Size of a partial transfer.  */
  sec_load_count -= sec_offset;
  if (load_end > 0 && sec_end > load_end)
    sec_load_count -= sec_end - load_end;

  /* Get the data.  */
  gdb::byte_vector buf (size);
  if (!bfd_get_section_contents (ibfd, isec, buf.data (), 0, size))
    error (_("Failed to read bfd file %s: '%s'."), bfd_get_filename (ibfd), 
	   bfd_errmsg (bfd_get_error ()));

  gdb_printf ("Restoring section %s (0x%lx to 0x%lx)",
	      bfd_section_name (isec), 
	      (unsigned long) sec_start, 
	      (unsigned long) sec_end);

  if (load_offset != 0 || load_start != 0 || load_end != 0)
    gdb_printf (" into memory (%s to %s)\n",
		paddress (current_inferior ()->arch (),
			  (unsigned long) sec_start
			  + sec_offset + load_offset),
		paddress (current_inferior ()->arch (),
			  (unsigned long) sec_start + sec_offset
			  + load_offset + sec_load_count));
  else
    gdb_puts ("\n");

  /* Write the data.  */
  ret = target_write_memory (sec_start + sec_offset + load_offset,
			     &buf[sec_offset], sec_load_count);
  if (ret != 0)
    warning (_("restore: memory write failed (%s)."), safe_strerror (ret));
}

static void
restore_binary_file (const char *filename, CORE_ADDR load_offset,
		     CORE_ADDR load_start, CORE_ADDR load_end)

{
  gdb_file_up file = gdb_fopen_cloexec (filename, FOPEN_RB);
  long len;

  if (file == NULL)
    error (_("Failed to open %s: %s"), filename, safe_strerror (errno));

  /* Get the file size for reading.  */
  if (fseek (file.get (), 0, SEEK_END) == 0)
    {
      len = ftell (file.get ());
      if (len < 0)
	perror_with_name (filename);
    }
  else
    perror_with_name (filename);

  if (len <= load_start)
    error (_("Start address is greater than length of binary file %s."), 
	   filename);

  /* Chop off "len" if it exceeds the requested load_end addr.  */
  if (load_end != 0 && load_end < len)
    len = load_end;
  /* Chop off "len" if the requested load_start addr skips some bytes.  */
  if (load_start > 0)
    len -= load_start;

  gdb_printf 
    ("Restoring binary file %s into memory (0x%lx to 0x%lx)\n", 
     filename, 
     (unsigned long) (load_start + load_offset),
     (unsigned long) (load_start + load_offset + len));

  /* Now set the file pos to the requested load start pos.  */
  if (fseek (file.get (), load_start, SEEK_SET) != 0)
    perror_with_name (filename);

  /* Now allocate a buffer and read the file contents.  */
  gdb::byte_vector buf (len);
  if (fread (buf.data (), 1, len, file.get ()) != len)
    perror_with_name (filename);

  /* Now write the buffer into target memory.  */
  len = target_write_memory (load_start + load_offset, buf.data (), len);
  if (len != 0)
    warning (_("restore: memory write failed (%s)."), safe_strerror (len));
}

static void
restore_command (const char *args, int from_tty)
{
  int binary_flag = 0;

  if (!target_has_execution ())
    noprocess ();

  CORE_ADDR load_offset = 0;
  CORE_ADDR load_start  = 0;
  CORE_ADDR load_end    = 0;

  /* Parse the input arguments.  First is filename (required).  */
  gdb::unique_xmalloc_ptr<char> filename = scan_filename (&args, NULL);
  if (args != NULL && *args != '\0')
    {
      static const char binary_string[] = "binary";

      /* Look for optional "binary" flag.  */
      if (startswith (args, binary_string))
	{
	  binary_flag = 1;
	  args += strlen (binary_string);
	  args = skip_spaces (args);
	}
      /* Parse offset (optional).  */
      if (args != NULL && *args != '\0')
	load_offset
	  = (binary_flag
	     ? parse_and_eval_address (scan_expression (&args, NULL).get ())
	     : parse_and_eval_long (scan_expression (&args, NULL).get ()));
      if (args != NULL && *args != '\0')
	{
	  /* Parse start address (optional).  */
	  load_start =
	    parse_and_eval_long (scan_expression (&args, NULL).get ());
	  if (args != NULL && *args != '\0')
	    {
	      /* Parse end address (optional).  */
	      load_end = parse_and_eval_long (args);
	      if (load_end <= load_start)
		error (_("Start must be less than end."));
	    }
	}
    }

  if (info_verbose)
    gdb_printf ("Restore file %s offset 0x%lx start 0x%lx end 0x%lx\n",
		filename.get (), (unsigned long) load_offset,
		(unsigned long) load_start,
		(unsigned long) load_end);

  if (binary_flag)
    {
      restore_binary_file (filename.get (), load_offset, load_start,
			   load_end);
    }
  else
    {
      /* Open the file for loading.  */
      gdb_bfd_ref_ptr ibfd (bfd_openr_or_error (filename.get (), NULL));

      /* Process the sections.  */
      for (asection *sect : gdb_bfd_sections (ibfd))
	restore_one_section (ibfd.get (), sect, load_offset, load_start,
			     load_end);
    }
}

void _initialize_cli_dump ();
void
_initialize_cli_dump ()
{
  struct cmd_list_element *c;

  add_basic_prefix_cmd ("dump", class_vars,
			_("Dump target code/data to a local file."),
			&dump_cmdlist,
			0/*allow-unknown*/,
			&cmdlist);
  add_basic_prefix_cmd ("append", class_vars,
			_("Append target code/data to a local file."),
			&append_cmdlist,
			0/*allow-unknown*/,
			&cmdlist);

  add_dump_command ("memory", dump_memory_command, "\
Write contents of memory to a raw binary file.\n\
Arguments are FILE START STOP.  Writes the contents of memory within the\n\
range [START .. STOP) to the specified FILE in raw target ordered bytes.");

  add_dump_command ("value", dump_value_command, "\
Write the value of an expression to a raw binary file.\n\
Arguments are FILE EXPRESSION.  Writes the value of EXPRESSION to\n\
the specified FILE in raw target ordered bytes.");

  add_basic_prefix_cmd ("srec", all_commands,
			_("Write target code/data to an srec file."),
			&srec_cmdlist,
			0 /*allow-unknown*/, 
			&dump_cmdlist);

  add_basic_prefix_cmd ("ihex", all_commands,
			_("Write target code/data to an intel hex file."),
			&ihex_cmdlist,
			0 /*allow-unknown*/, 
			&dump_cmdlist);

  add_basic_prefix_cmd ("verilog", all_commands,
			_("Write target code/data to a verilog hex file."),
			&verilog_cmdlist,
			0 /*allow-unknown*/,
			&dump_cmdlist);

  add_basic_prefix_cmd ("tekhex", all_commands,
			_("Write target code/data to a tekhex file."),
			&tekhex_cmdlist,
			0 /*allow-unknown*/, 
			&dump_cmdlist);

  add_basic_prefix_cmd ("binary", all_commands,
			_("Write target code/data to a raw binary file."),
			&binary_dump_cmdlist,
			0 /*allow-unknown*/, 
			&dump_cmdlist);

  add_basic_prefix_cmd ("binary", all_commands,
			_("Append target code/data to a raw binary file."),
			&binary_append_cmdlist,
			0 /*allow-unknown*/, 
			&append_cmdlist);

  add_cmd ("memory", all_commands, dump_srec_memory, _("\
Write contents of memory to an srec file.\n\
Arguments are FILE START STOP.  Writes the contents of memory\n\
within the range [START .. STOP) to the specified FILE in srec format."),
	   &srec_cmdlist);

  add_cmd ("value", all_commands, dump_srec_value, _("\
Write the value of an expression to an srec file.\n\
Arguments are FILE EXPRESSION.  Writes the value of EXPRESSION\n\
to the specified FILE in srec format."),
	   &srec_cmdlist);

  add_cmd ("memory", all_commands, dump_ihex_memory, _("\
Write contents of memory to an ihex file.\n\
Arguments are FILE START STOP.  Writes the contents of memory within\n\
the range [START .. STOP) to the specified FILE in intel hex format."),
	   &ihex_cmdlist);

  add_cmd ("value", all_commands, dump_ihex_value, _("\
Write the value of an expression to an ihex file.\n\
Arguments are FILE EXPRESSION.  Writes the value of EXPRESSION\n\
to the specified FILE in intel hex format."),
	   &ihex_cmdlist);

  add_cmd ("memory", all_commands, dump_verilog_memory, _("\
Write contents of memory to a verilog hex file.\n\
Arguments are FILE START STOP.  Writes the contents of memory within\n\
the range [START .. STOP) to the specified FILE in verilog hex format."),
	   &verilog_cmdlist);

  add_cmd ("value", all_commands, dump_verilog_value, _("\
Write the value of an expression to a verilog hex file.\n\
Arguments are FILE EXPRESSION.  Writes the value of EXPRESSION\n\
to the specified FILE in verilog hex format."),
	   &verilog_cmdlist);

  add_cmd ("memory", all_commands, dump_tekhex_memory, _("\
Write contents of memory to a tekhex file.\n\
Arguments are FILE START STOP.  Writes the contents of memory\n\
within the range [START .. STOP) to the specified FILE in tekhex format."),
	   &tekhex_cmdlist);

  add_cmd ("value", all_commands, dump_tekhex_value, _("\
Write the value of an expression to a tekhex file.\n\
Arguments are FILE EXPRESSION.  Writes the value of EXPRESSION\n\
to the specified FILE in tekhex format."),
	   &tekhex_cmdlist);

  add_cmd ("memory", all_commands, dump_binary_memory, _("\
Write contents of memory to a raw binary file.\n\
Arguments are FILE START STOP.  Writes the contents of memory\n\
within the range [START .. STOP) to the specified FILE in binary format."),
	   &binary_dump_cmdlist);

  add_cmd ("value", all_commands, dump_binary_value, _("\
Write the value of an expression to a raw binary file.\n\
Arguments are FILE EXPRESSION.  Writes the value of EXPRESSION\n\
to the specified FILE in raw target ordered bytes."),
	   &binary_dump_cmdlist);

  add_cmd ("memory", all_commands, append_binary_memory, _("\
Append contents of memory to a raw binary file.\n\
Arguments are FILE START STOP.  Writes the contents of memory within the\n\
range [START .. STOP) to the specified FILE in raw target ordered bytes."),
	   &binary_append_cmdlist);

  add_cmd ("value", all_commands, append_binary_value, _("\
Append the value of an expression to a raw binary file.\n\
Arguments are FILE EXPRESSION.  Writes the value of EXPRESSION\n\
to the specified FILE in raw target ordered bytes."),
	   &binary_append_cmdlist);

  c = add_com ("restore", class_vars, restore_command, _("\
Restore the contents of FILE to target memory.\n\
Arguments are FILE OFFSET START END where all except FILE are optional.\n\
OFFSET will be added to the base address of the file (default zero).\n\
If START and END are given, only the file contents within that range\n\
(file relative) will be restored to target memory."));
  c->completer = filename_completer;
  /* FIXME: completers for other commands.  */
}
