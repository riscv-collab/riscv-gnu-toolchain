/* Read DWARF macro information

   Copyright (C) 1994-2024 Free Software Foundation, Inc.

   Adapted by Gary Funck (gary@intrepid.com), Intrepid Technology,
   Inc.  with support from Florida State University (under contract
   with the Ada Joint Program Office), and Silicon Graphics, Inc.
   Initial contribution by Brent Benson, Harris Computer Systems, Inc.,
   based on Fred Fish's (Cygnus Support) implementation of DWARF 1
   support.

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
#include "dwarf2/read.h"
#include "dwarf2/leb.h"
#include "dwarf2/expr.h"
#include "dwarf2/line-header.h"
#include "dwarf2/section.h"
#include "dwarf2/macro.h"
#include "dwarf2/dwz.h"
#include "buildsym.h"
#include "macrotab.h"
#include "complaints.h"
#include "objfiles.h"

static void
dwarf2_macro_malformed_definition_complaint (const char *arg1)
{
  complaint (_("macro debug info contains a "
	       "malformed macro definition:\n`%s'"),
	     arg1);
}

static struct macro_source_file *
macro_start_file (buildsym_compunit *builder,
		  int file, int line,
		  struct macro_source_file *current_file,
		  const struct line_header *lh)
{
  /* File name relative to the compilation directory of this source file.  */
  const file_entry *fe = lh->file_name_at (file);
  std::string file_name;

  if (fe != nullptr)
    file_name = lh->file_file_name (*fe);
  else
    {
      /* The compiler produced a bogus file number.  We can at least
	 record the macro definitions made in the file, even if we
	 won't be able to find the file by name.  */
      complaint (_("bad file number in macro information (%d)"),
		 file);

      file_name = string_printf ("<bad macro file number %d>", file);
    }

  if (! current_file)
    {
      /* Note: We don't create a macro table for this compilation unit
	 at all until we actually get a filename.  */
      struct macro_table *macro_table = builder->get_macro_table ();

      /* If we have no current file, then this must be the start_file
	 directive for the compilation unit's main source file.  */
      current_file = macro_set_main (macro_table, file_name.c_str ());
      macro_define_special (macro_table);
    }
  else
    current_file = macro_include (current_file, line, file_name.c_str ());

  return current_file;
}

static const char *
consume_improper_spaces (const char *p, const char *body)
{
  if (*p == ' ')
    {
      complaint (_("macro definition contains spaces "
		   "in formal argument list:\n`%s'"),
		 body);

      while (*p == ' ')
	p++;
    }

  return p;
}


static void
parse_macro_definition (struct macro_source_file *file, int line,
			const char *body)
{
  const char *p;

  /* The body string takes one of two forms.  For object-like macro
     definitions, it should be:

	<macro name> " " <definition>

     For function-like macro definitions, it should be:

	<macro name> "() " <definition>
     or
	<macro name> "(" <arg name> ( "," <arg name> ) * ") " <definition>

     Spaces may appear only where explicitly indicated, and in the
     <definition>.

     The Dwarf 2 spec says that an object-like macro's name is always
     followed by a space, but versions of GCC around March 2002 omit
     the space when the macro's definition is the empty string.

     The Dwarf 2 spec says that there should be no spaces between the
     formal arguments in a function-like macro's formal argument list,
     but versions of GCC around March 2002 include spaces after the
     commas.  */


  /* Find the extent of the macro name.  The macro name is terminated
     by either a space or null character (for an object-like macro) or
     an opening paren (for a function-like macro).  */
  for (p = body; *p; p++)
    if (*p == ' ' || *p == '(')
      break;

  if (*p == ' ' || *p == '\0')
    {
      /* It's an object-like macro.  */
      int name_len = p - body;
      std::string name (body, name_len);
      const char *replacement;

      if (*p == ' ')
	replacement = body + name_len + 1;
      else
	{
	  dwarf2_macro_malformed_definition_complaint (body);
	  replacement = body + name_len;
	}

      macro_define_object (file, line, name.c_str (), replacement);
    }
  else if (*p == '(')
    {
      /* It's a function-like macro.  */
      std::string name (body, p - body);
      int argc = 0;
      int argv_size = 1;
      char **argv = XNEWVEC (char *, argv_size);

      p++;

      p = consume_improper_spaces (p, body);

      /* Parse the formal argument list.  */
      while (*p && *p != ')')
	{
	  /* Find the extent of the current argument name.  */
	  const char *arg_start = p;

	  while (*p && *p != ',' && *p != ')' && *p != ' ')
	    p++;

	  if (! *p || p == arg_start)
	    dwarf2_macro_malformed_definition_complaint (body);
	  else
	    {
	      /* Make sure argv has room for the new argument.  */
	      if (argc >= argv_size)
		{
		  argv_size *= 2;
		  argv = XRESIZEVEC (char *, argv, argv_size);
		}

	      argv[argc++] = savestring (arg_start, p - arg_start);
	    }

	  p = consume_improper_spaces (p, body);

	  /* Consume the comma, if present.  */
	  if (*p == ',')
	    {
	      p++;

	      p = consume_improper_spaces (p, body);
	    }
	}

      if (*p == ')')
	{
	  p++;

	  if (*p == ' ')
	    /* Perfectly formed definition, no complaints.  */
	    macro_define_function (file, line, name.c_str (),
				   argc, (const char **) argv,
				   p + 1);
	  else if (*p == '\0')
	    {
	      /* Complain, but do define it.  */
	      dwarf2_macro_malformed_definition_complaint (body);
	      macro_define_function (file, line, name.c_str (),
				     argc, (const char **) argv,
				     p);
	    }
	  else
	    /* Just complain.  */
	    dwarf2_macro_malformed_definition_complaint (body);
	}
      else
	/* Just complain.  */
	dwarf2_macro_malformed_definition_complaint (body);

      {
	int i;

	for (i = 0; i < argc; i++)
	  xfree (argv[i]);
      }
      xfree (argv);
    }
  else
    dwarf2_macro_malformed_definition_complaint (body);
}

/* Skip some bytes from BYTES according to the form given in FORM.
   Returns the new pointer.  */

static const gdb_byte *
skip_form_bytes (bfd *abfd, const gdb_byte *bytes, const gdb_byte *buffer_end,
		 enum dwarf_form form,
		 unsigned int offset_size,
		 const struct dwarf2_section_info *section)
{
  unsigned int bytes_read;

  switch (form)
    {
    case DW_FORM_data1:
    case DW_FORM_flag:
      ++bytes;
      break;

    case DW_FORM_data2:
      bytes += 2;
      break;

    case DW_FORM_data4:
      bytes += 4;
      break;

    case DW_FORM_data8:
      bytes += 8;
      break;

    case DW_FORM_data16:
      bytes += 16;
      break;

    case DW_FORM_string:
      read_direct_string (abfd, bytes, &bytes_read);
      bytes += bytes_read;
      break;

    case DW_FORM_sec_offset:
    case DW_FORM_strp:
    case DW_FORM_GNU_strp_alt:
      bytes += offset_size;
      break;

    case DW_FORM_block:
      bytes += read_unsigned_leb128 (abfd, bytes, &bytes_read);
      bytes += bytes_read;
      break;

    case DW_FORM_block1:
      bytes += 1 + read_1_byte (abfd, bytes);
      break;
    case DW_FORM_block2:
      bytes += 2 + read_2_bytes (abfd, bytes);
      break;
    case DW_FORM_block4:
      bytes += 4 + read_4_bytes (abfd, bytes);
      break;

    case DW_FORM_addrx:
    case DW_FORM_sdata:
    case DW_FORM_strx:
    case DW_FORM_udata:
    case DW_FORM_GNU_addr_index:
    case DW_FORM_GNU_str_index:
      bytes = gdb_skip_leb128 (bytes, buffer_end);
      if (bytes == NULL)
	{
	  section->overflow_complaint ();
	  return NULL;
	}
      break;

    case DW_FORM_implicit_const:
      break;

    default:
      {
	complaint (_("invalid form 0x%x in `%s'"),
		   form, section->get_name ());
	return NULL;
      }
    }

  return bytes;
}

/* A helper for dwarf_decode_macros that handles skipping an unknown
   opcode.  Returns an updated pointer to the macro data buffer; or,
   on error, issues a complaint and returns NULL.  */

static const gdb_byte *
skip_unknown_opcode (unsigned int opcode,
		     const gdb_byte **opcode_definitions,
		     const gdb_byte *mac_ptr, const gdb_byte *mac_end,
		     bfd *abfd,
		     unsigned int offset_size,
		     const struct dwarf2_section_info *section)
{
  unsigned int bytes_read, i;
  unsigned long arg;
  const gdb_byte *defn;

  if (opcode_definitions[opcode] == NULL)
    {
      complaint (_("unrecognized DW_MACINFO or DW_MACRO opcode 0x%x"),
		 opcode);
      return NULL;
    }

  defn = opcode_definitions[opcode];
  arg = read_unsigned_leb128 (abfd, defn, &bytes_read);
  defn += bytes_read;

  for (i = 0; i < arg; ++i)
    {
      mac_ptr = skip_form_bytes (abfd, mac_ptr, mac_end,
				 (enum dwarf_form) defn[i], offset_size,
				 section);
      if (mac_ptr == NULL)
	{
	  /* skip_form_bytes already issued the complaint.  */
	  return NULL;
	}
    }

  return mac_ptr;
}

/* A helper function which parses the header of a macro section.
   If the macro section is the extended (for now called "GNU") type,
   then this updates *OFFSET_SIZE.  Returns a pointer to just after
   the header, or issues a complaint and returns NULL on error.  */

static const gdb_byte *
dwarf_parse_macro_header (const gdb_byte **opcode_definitions,
			  bfd *abfd,
			  const gdb_byte *mac_ptr,
			  unsigned int *offset_size,
			  int section_is_gnu)
{
  memset (opcode_definitions, 0, 256 * sizeof (gdb_byte *));

  if (section_is_gnu)
    {
      unsigned int version, flags;

      version = read_2_bytes (abfd, mac_ptr);
      if (version != 4 && version != 5)
	{
	  complaint (_("unrecognized version `%d' in .debug_macro section"),
		     version);
	  return NULL;
	}
      mac_ptr += 2;

      flags = read_1_byte (abfd, mac_ptr);
      ++mac_ptr;
      *offset_size = (flags & 1) ? 8 : 4;

      if ((flags & 2) != 0)
	/* We don't need the line table offset.  */
	mac_ptr += *offset_size;

      /* Vendor opcode descriptions.  */
      if ((flags & 4) != 0)
	{
	  unsigned int i, count;

	  count = read_1_byte (abfd, mac_ptr);
	  ++mac_ptr;
	  for (i = 0; i < count; ++i)
	    {
	      unsigned int opcode, bytes_read;
	      unsigned long arg;

	      opcode = read_1_byte (abfd, mac_ptr);
	      ++mac_ptr;
	      opcode_definitions[opcode] = mac_ptr;
	      arg = read_unsigned_leb128 (abfd, mac_ptr, &bytes_read);
	      mac_ptr += bytes_read;
	      mac_ptr += arg;
	    }
	}
    }

  return mac_ptr;
}

/* A helper for dwarf_decode_macros that handles the GNU extensions,
   including DW_MACRO_import.  */

static void
dwarf_decode_macro_bytes (dwarf2_per_objfile *per_objfile,
			  buildsym_compunit *builder,
			  bfd *abfd,
			  const gdb_byte *mac_ptr, const gdb_byte *mac_end,
			  struct macro_source_file *current_file,
			  const struct line_header *lh,
			  const struct dwarf2_section_info *section,
			  int section_is_gnu, int section_is_dwz,
			  unsigned int offset_size,
			  struct dwarf2_section_info *str_section,
			  struct dwarf2_section_info *str_offsets_section,
			  std::optional<ULONGEST> str_offsets_base,
			  htab_t include_hash, struct dwarf2_cu *cu)
{
  struct objfile *objfile = per_objfile->objfile;
  enum dwarf_macro_record_type macinfo_type;
  int at_commandline;
  const gdb_byte *opcode_definitions[256];

  mac_ptr = dwarf_parse_macro_header (opcode_definitions, abfd, mac_ptr,
				      &offset_size, section_is_gnu);
  if (mac_ptr == NULL)
    {
      /* We already issued a complaint.  */
      return;
    }

  /* Determines if GDB is still before first DW_MACINFO_start_file.  If true
     GDB is still reading the definitions from command line.  First
     DW_MACINFO_start_file will need to be ignored as it was already executed
     to create CURRENT_FILE for the main source holding also the command line
     definitions.  On first met DW_MACINFO_start_file this flag is reset to
     normally execute all the remaining DW_MACINFO_start_file macinfos.  */

  at_commandline = 1;

  do
    {
      /* Do we at least have room for a macinfo type byte?  */
      if (mac_ptr >= mac_end)
	{
	  section->overflow_complaint ();
	  break;
	}

      macinfo_type = (enum dwarf_macro_record_type) read_1_byte (abfd, mac_ptr);
      mac_ptr++;

      /* Note that we rely on the fact that the corresponding GNU and
	 DWARF constants are the same.  */
      DIAGNOSTIC_PUSH
      DIAGNOSTIC_IGNORE_SWITCH_DIFFERENT_ENUM_TYPES
      switch (macinfo_type)
	{
	  /* A zero macinfo type indicates the end of the macro
	     information.  */
	case 0:
	  break;

	case DW_MACRO_define:
	case DW_MACRO_undef:
	case DW_MACRO_define_strp:
	case DW_MACRO_undef_strp:
	case DW_MACRO_define_sup:
	case DW_MACRO_undef_sup:
	  {
	    unsigned int bytes_read;
	    int line;
	    const char *body;
	    int is_define;

	    line = read_unsigned_leb128 (abfd, mac_ptr, &bytes_read);
	    mac_ptr += bytes_read;

	    if (macinfo_type == DW_MACRO_define
		|| macinfo_type == DW_MACRO_undef)
	      {
		body = read_direct_string (abfd, mac_ptr, &bytes_read);
		mac_ptr += bytes_read;
	      }
	    else
	      {
		LONGEST str_offset;

		str_offset = read_offset (abfd, mac_ptr, offset_size);
		mac_ptr += offset_size;

		if (macinfo_type == DW_MACRO_define_sup
		    || macinfo_type == DW_MACRO_undef_sup
		    || section_is_dwz)
		  {
		    dwz_file *dwz = dwarf2_get_dwz_file (per_objfile->per_bfd,
							 true);

		    body = dwz->read_string (objfile, str_offset);
		  }
		else
		  body = per_objfile->per_bfd->str.read_string (objfile,
								str_offset,
								"DW_FORM_strp");
	      }

	    is_define = (macinfo_type == DW_MACRO_define
			 || macinfo_type == DW_MACRO_define_strp
			 || macinfo_type == DW_MACRO_define_sup);
	    if (! current_file)
	      {
		/* DWARF violation as no main source is present.  */
		complaint (_("debug info with no main source gives macro %s "
			     "on line %d: %s"),
			   is_define ? _("definition") : _("undefinition"),
			   line, body);
		break;
	      }
	    if ((line == 0 && !at_commandline)
		|| (line != 0 && at_commandline))
	      complaint (_("debug info gives %s macro %s with %s line %d: %s"),
			 at_commandline ? _("command-line") : _("in-file"),
			 is_define ? _("definition") : _("undefinition"),
			 line == 0 ? _("zero") : _("non-zero"), line, body);

	    if (body == NULL)
	      {
		/* Fedora's rpm-build's "debugedit" binary
		   corrupted .debug_macro sections.

		   For more info, see
		   https://bugzilla.redhat.com/show_bug.cgi?id=1708786 */
		complaint (_("debug info gives %s invalid macro %s "
			     "without body (corrupted?) at line %d "
			     "on file %s"),
			   at_commandline ? _("command-line") : _("in-file"),
			   is_define ? _("definition") : _("undefinition"),
			   line, current_file->filename);
	      }
	    else if (is_define)
	      parse_macro_definition (current_file, line, body);
	    else
	      {
		gdb_assert (macinfo_type == DW_MACRO_undef
			    || macinfo_type == DW_MACRO_undef_strp
			    || macinfo_type == DW_MACRO_undef_sup);
		macro_undef (current_file, line, body);
	      }
	  }
	  break;

	case DW_MACRO_define_strx:
	case DW_MACRO_undef_strx:
	  {
	    unsigned int bytes_read;

	    int line = read_unsigned_leb128 (abfd, mac_ptr, &bytes_read);
	    mac_ptr += bytes_read;
	    int offset_index = read_unsigned_leb128 (abfd, mac_ptr, &bytes_read);
	    mac_ptr += bytes_read;

	    /* Use of the strx operators requires a DW_AT_str_offsets_base.  */
	    if (!str_offsets_base.has_value ())
	      {
		complaint (_("use of %s with unknown string offsets base "
			     "[in module %s]"),
			   (macinfo_type == DW_MACRO_define_strx
			    ? "DW_MACRO_define_strx"
			    : "DW_MACRO_undef_strx"),
			   objfile_name (objfile));
		break;
	      }

	    str_offsets_section->read (objfile);
	    const gdb_byte *info_ptr = (str_offsets_section->buffer
					+ *str_offsets_base
					+ offset_index * offset_size);

	    const char *macinfo_str = (macinfo_type == DW_MACRO_define_strx ?
				       "DW_MACRO_define_strx" : "DW_MACRO_undef_strx");

	    if (*str_offsets_base + offset_index * offset_size
		>= str_offsets_section->size)
	      {
		complaint (_("%s pointing outside of .debug_str_offsets section "
			     "[in module %s]"), macinfo_str, objfile_name (objfile));
		break;
	      }

	    ULONGEST str_offset = read_offset (abfd, info_ptr, offset_size);

	    const char *body = str_section->read_string (objfile, str_offset,
							 macinfo_str);
	    if (current_file == nullptr)
	      {
		/* DWARF violation as no main source is present.  */
		complaint (_("debug info with no main source gives macro %s "
			     "on line %d: %s"),
			     macinfo_type == DW_MACRO_define_strx ? _("definition")
			     : _("undefinition"), line, body);
		break;
	      }

	    if (macinfo_type == DW_MACRO_define_strx)
	      parse_macro_definition (current_file, line, body);
	    else
	      macro_undef (current_file, line, body);
	   }
	   break;

	case DW_MACRO_start_file:
	  {
	    unsigned int bytes_read;
	    int line, file;

	    line = read_unsigned_leb128 (abfd, mac_ptr, &bytes_read);
	    mac_ptr += bytes_read;
	    file = read_unsigned_leb128 (abfd, mac_ptr, &bytes_read);
	    mac_ptr += bytes_read;

	    if ((line == 0 && !at_commandline)
		|| (line != 0 && at_commandline))
	      complaint (_("debug info gives source %d included "
			   "from %s at %s line %d"),
			 file, at_commandline ? _("command-line") : _("file"),
			 line == 0 ? _("zero") : _("non-zero"), line);

	    if (at_commandline)
	      {
		/* This DW_MACRO_start_file was executed in the
		   pass one.  */
		at_commandline = 0;
	      }
	    else
	      current_file = macro_start_file (builder, file, line,
					       current_file, lh);
	  }
	  break;

	case DW_MACRO_end_file:
	  if (! current_file)
	    complaint (_("macro debug info has an unmatched "
			 "`close_file' directive"));
	  else if (current_file->included_by == nullptr
		   && producer_is_clang (cu))
	    {
	      /* Clang, until the current version, misplaces some macro
		 definitions - such as ones defined in the command line,
		 putting them after the last DW_MACRO_end_file instead of
		 before the first DW_MACRO_start_file.  Since at the time
		 of writing there is no clang version with this bug fixed,
		 we check for any clang producer.  This should be changed
		 to producer_is_clang_lt_XX when possible. */
	    }
	  else
	    {
	      current_file = current_file->included_by;
	      if (! current_file)
		{
		  enum dwarf_macro_record_type next_type;

		  /* GCC circa March 2002 doesn't produce the zero
		     type byte marking the end of the compilation
		     unit.  Complain if it's not there, but exit no
		     matter what.  */

		  /* Do we at least have room for a macinfo type byte?  */
		  if (mac_ptr >= mac_end)
		    {
		      section->overflow_complaint ();
		      return;
		    }

		  /* We don't increment mac_ptr here, so this is just
		     a look-ahead.  */
		  next_type
		    = (enum dwarf_macro_record_type) read_1_byte (abfd,
								  mac_ptr);
		  if (next_type != 0)
		    complaint (_("no terminating 0-type entry for "
				 "macros in `.debug_macinfo' section"));

		  return;
		}
	    }
	  break;

	case DW_MACRO_import:
	case DW_MACRO_import_sup:
	  {
	    LONGEST offset;
	    void **slot;
	    bfd *include_bfd = abfd;
	    const struct dwarf2_section_info *include_section = section;
	    const gdb_byte *include_mac_end = mac_end;
	    int is_dwz = section_is_dwz;
	    const gdb_byte *new_mac_ptr;

	    offset = read_offset (abfd, mac_ptr, offset_size);
	    mac_ptr += offset_size;

	    if (macinfo_type == DW_MACRO_import_sup)
	      {
		dwz_file *dwz = dwarf2_get_dwz_file (per_objfile->per_bfd,
						     true);

		include_section = &dwz->macro;
		include_bfd = include_section->get_bfd_owner ();
		include_mac_end = dwz->macro.buffer + dwz->macro.size;
		is_dwz = 1;
	      }

	    new_mac_ptr = include_section->buffer + offset;
	    slot = htab_find_slot (include_hash, new_mac_ptr, INSERT);

	    if (*slot != NULL)
	      {
		/* This has actually happened; see
		   http://sourceware.org/bugzilla/show_bug.cgi?id=13568.  */
		complaint (_("recursive DW_MACRO_import in "
			     ".debug_macro section"));
	      }
	    else
	      {
		*slot = (void *) new_mac_ptr;

		dwarf_decode_macro_bytes (per_objfile, builder, include_bfd,
					  new_mac_ptr, include_mac_end,
					  current_file, lh, section,
					  section_is_gnu, is_dwz, offset_size,
					  str_section, str_offsets_section,
					  str_offsets_base, include_hash, cu);

		htab_remove_elt (include_hash, (void *) new_mac_ptr);
	      }
	  }
	  break;

	case DW_MACINFO_vendor_ext:
	  if (!section_is_gnu)
	    {
	      unsigned int bytes_read;

	      /* This reads the constant, but since we don't recognize
		 any vendor extensions, we ignore it.  */
	      read_unsigned_leb128 (abfd, mac_ptr, &bytes_read);
	      mac_ptr += bytes_read;
	      read_direct_string (abfd, mac_ptr, &bytes_read);
	      mac_ptr += bytes_read;

	      /* We don't recognize any vendor extensions.  */
	      break;
	    }
	  [[fallthrough]];

	default:
	  mac_ptr = skip_unknown_opcode (macinfo_type, opcode_definitions,
					 mac_ptr, mac_end, abfd, offset_size,
					 section);
	  if (mac_ptr == NULL)
	    return;
	  break;
	}
      DIAGNOSTIC_POP
    } while (macinfo_type != 0);
}

void
dwarf_decode_macros (dwarf2_per_objfile *per_objfile,
		     buildsym_compunit *builder,
		     const dwarf2_section_info *section,
		     const struct line_header *lh, unsigned int offset_size,
		     unsigned int offset, struct dwarf2_section_info *str_section,
		     struct dwarf2_section_info *str_offsets_section,
		     std::optional<ULONGEST> str_offsets_base,
		     int section_is_gnu, struct dwarf2_cu *cu)
{
  bfd *abfd;
  const gdb_byte *mac_ptr, *mac_end;
  struct macro_source_file *current_file = 0;
  enum dwarf_macro_record_type macinfo_type;
  const gdb_byte *opcode_definitions[256];
  void **slot;

  abfd = section->get_bfd_owner ();

  /* First pass: Find the name of the base filename.
     This filename is needed in order to process all macros whose definition
     (or undefinition) comes from the command line.  These macros are defined
     before the first DW_MACINFO_start_file entry, and yet still need to be
     associated to the base file.

     To determine the base file name, we scan the macro definitions until we
     reach the first DW_MACINFO_start_file entry.  We then initialize
     CURRENT_FILE accordingly so that any macro definition found before the
     first DW_MACINFO_start_file can still be associated to the base file.  */

  mac_ptr = section->buffer + offset;
  mac_end = section->buffer + section->size;

  mac_ptr = dwarf_parse_macro_header (opcode_definitions, abfd, mac_ptr,
				      &offset_size, section_is_gnu);
  if (mac_ptr == NULL)
    {
      /* We already issued a complaint.  */
      return;
    }

  do
    {
      /* Do we at least have room for a macinfo type byte?  */
      if (mac_ptr >= mac_end)
	{
	  /* Complaint is printed during the second pass as GDB will probably
	     stop the first pass earlier upon finding
	     DW_MACINFO_start_file.  */
	  break;
	}

      macinfo_type = (enum dwarf_macro_record_type) read_1_byte (abfd, mac_ptr);
      mac_ptr++;

      /* Note that we rely on the fact that the corresponding GNU and
	 DWARF constants are the same.  */
      DIAGNOSTIC_PUSH
      DIAGNOSTIC_IGNORE_SWITCH_DIFFERENT_ENUM_TYPES
      switch (macinfo_type)
	{
	  /* A zero macinfo type indicates the end of the macro
	     information.  */
	case 0:
	  break;

	case DW_MACRO_define:
	case DW_MACRO_undef:
	  /* Only skip the data by MAC_PTR.  */
	  {
	    unsigned int bytes_read;

	    read_unsigned_leb128 (abfd, mac_ptr, &bytes_read);
	    mac_ptr += bytes_read;
	    read_direct_string (abfd, mac_ptr, &bytes_read);
	    mac_ptr += bytes_read;
	  }
	  break;

	case DW_MACRO_start_file:
	  {
	    unsigned int bytes_read;
	    int line, file;

	    line = read_unsigned_leb128 (abfd, mac_ptr, &bytes_read);
	    mac_ptr += bytes_read;
	    file = read_unsigned_leb128 (abfd, mac_ptr, &bytes_read);
	    mac_ptr += bytes_read;

	    current_file = macro_start_file (builder, file, line,
					     current_file, lh);
	  }
	  break;

	case DW_MACRO_end_file:
	  /* No data to skip by MAC_PTR.  */
	  break;

	case DW_MACRO_define_strp:
	case DW_MACRO_undef_strp:
	case DW_MACRO_define_sup:
	case DW_MACRO_undef_sup:
	  {
	    unsigned int bytes_read;

	    read_unsigned_leb128 (abfd, mac_ptr, &bytes_read);
	    mac_ptr += bytes_read;
	    mac_ptr += offset_size;
	  }
	  break;
	case DW_MACRO_define_strx:
	case DW_MACRO_undef_strx:
	  {
	    unsigned int bytes_read;

	    read_unsigned_leb128 (abfd, mac_ptr, &bytes_read);
	    mac_ptr += bytes_read;
	    read_unsigned_leb128 (abfd, mac_ptr, &bytes_read);
	    mac_ptr += bytes_read;
	  }
	  break;

	case DW_MACRO_import:
	case DW_MACRO_import_sup:
	  /* Note that, according to the spec, a transparent include
	     chain cannot call DW_MACRO_start_file.  So, we can just
	     skip this opcode.  */
	  mac_ptr += offset_size;
	  break;

	case DW_MACINFO_vendor_ext:
	  /* Only skip the data by MAC_PTR.  */
	  if (!section_is_gnu)
	    {
	      unsigned int bytes_read;

	      read_unsigned_leb128 (abfd, mac_ptr, &bytes_read);
	      mac_ptr += bytes_read;
	      read_direct_string (abfd, mac_ptr, &bytes_read);
	      mac_ptr += bytes_read;
	    }
	  [[fallthrough]];

	default:
	  mac_ptr = skip_unknown_opcode (macinfo_type, opcode_definitions,
					 mac_ptr, mac_end, abfd, offset_size,
					 section);
	  if (mac_ptr == NULL)
	    return;
	  break;
	}
      DIAGNOSTIC_POP
    } while (macinfo_type != 0 && current_file == NULL);

  /* Second pass: Process all entries.

     Use the AT_COMMAND_LINE flag to determine whether we are still processing
     command-line macro definitions/undefinitions.  This flag is unset when we
     reach the first DW_MACINFO_start_file entry.  */

  htab_up include_hash (htab_create_alloc (1, htab_hash_pointer,
					   htab_eq_pointer,
					   NULL, xcalloc, xfree));
  mac_ptr = section->buffer + offset;
  slot = htab_find_slot (include_hash.get (), mac_ptr, INSERT);
  *slot = (void *) mac_ptr;
  dwarf_decode_macro_bytes (per_objfile, builder, abfd, mac_ptr, mac_end,
			    current_file, lh, section, section_is_gnu, 0,
			    offset_size, str_section, str_offsets_section,
			    str_offsets_base, include_hash.get (), cu);
}
