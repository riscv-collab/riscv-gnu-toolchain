/* Copyright (C) 2013-2024 Free Software Foundation, Inc.

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
#include "solib-aix.h"
#include "solib.h"
#include "solist.h"
#include "inferior.h"
#include "gdb_bfd.h"
#include "objfiles.h"
#include "symtab.h"
#include "xcoffread.h"
#include "observable.h"

/* Our private data in struct shobj.  */

struct lm_info_aix final : public lm_info
{
  /* The name of the file mapped by the loader.  Apart from the entry
     for the main executable, this is usually a shared library (which,
     on AIX, is an archive library file, created using the "ar"
     command).  */
  std::string filename;

  /* The name of the shared object file with the actual dynamic
     loading dependency.  This may be empty (Eg. main executable).  */
  std::string member_name;

  /* The address in inferior memory where the text section got mapped.  */
  CORE_ADDR text_addr = 0;

  /* The size of the text section, obtained via the loader data.  */
  ULONGEST text_size = 0;

  /* The address in inferior memory where the data section got mapped.  */
  CORE_ADDR data_addr = 0;

  /* The size of the data section, obtained via the loader data.  */
  ULONGEST data_size = 0;
};

/* This module's per-inferior data.  */

struct solib_aix_inferior_data
{
  /* The list of shared libraries.

     Note that the first element of this list is always the main
     executable, which is not technically a shared library.  But
     we need that information to perform its relocation, and
     the same principles applied to shared libraries also apply
     to the main executable.  So it's simpler to keep it as part
     of this list.  */
  std::optional<std::vector<lm_info_aix>> library_list;
};

/* Key to our per-inferior data.  */
static const registry<inferior>::key<solib_aix_inferior_data>
  solib_aix_inferior_data_handle;

/* Return this module's data for the given inferior.
   If none is found, add a zero'ed one now.  */

static struct solib_aix_inferior_data *
get_solib_aix_inferior_data (struct inferior *inf)
{
  struct solib_aix_inferior_data *data;

  data = solib_aix_inferior_data_handle.get (inf);
  if (data == NULL)
    data = solib_aix_inferior_data_handle.emplace (inf);

  return data;
}

#if !defined(HAVE_LIBEXPAT)

/* Dummy implementation if XML support is not compiled in.  */

static std::optional<std::vector<lm_info_aix>>
solib_aix_parse_libraries (const char *library)
{
  static int have_warned;

  if (!have_warned)
    {
      have_warned = 1;
      warning (_("Can not parse XML library list; XML support was disabled "
		 "at compile time"));
    }

  return {};
}

#else /* HAVE_LIBEXPAT */

#include "xml-support.h"

/* Handle the start of a <library> element.  */

static void
library_list_start_library (struct gdb_xml_parser *parser,
			    const struct gdb_xml_element *element,
			    void *user_data,
			    std::vector<gdb_xml_value> &attributes)
{
  std::vector<lm_info_aix> *list = (std::vector<lm_info_aix> *) user_data;
  lm_info_aix item;
  struct gdb_xml_value *attr;

  attr = xml_find_attribute (attributes, "name");
  item.filename = (const char *) attr->value.get ();

  attr = xml_find_attribute (attributes, "member");
  if (attr != NULL)
    item.member_name = (const char *) attr->value.get ();

  attr = xml_find_attribute (attributes, "text_addr");
  item.text_addr = * (ULONGEST *) attr->value.get ();

  attr = xml_find_attribute (attributes, "text_size");
  item.text_size = * (ULONGEST *) attr->value.get ();

  attr = xml_find_attribute (attributes, "data_addr");
  item.data_addr = * (ULONGEST *) attr->value.get ();

  attr = xml_find_attribute (attributes, "data_size");
  item.data_size = * (ULONGEST *) attr->value.get ();

  list->push_back (std::move (item));
}

/* Handle the start of a <library-list-aix> element.  */

static void
library_list_start_list (struct gdb_xml_parser *parser,
			 const struct gdb_xml_element *element,
			 void *user_data,
			 std::vector<gdb_xml_value> &attributes)
{
  char *version
    = (char *) xml_find_attribute (attributes, "version")->value.get ();

  if (strcmp (version, "1.0") != 0)
    gdb_xml_error (parser,
		   _("Library list has unsupported version \"%s\""),
		   version);
}

/* The allowed elements and attributes for an AIX library list
   described in XML format.  The root element is a <library-list-aix>.  */

static const struct gdb_xml_attribute library_attributes[] =
{
  { "name", GDB_XML_AF_NONE, NULL, NULL },
  { "member", GDB_XML_AF_OPTIONAL, NULL, NULL },
  { "text_addr", GDB_XML_AF_NONE, gdb_xml_parse_attr_ulongest, NULL },
  { "text_size", GDB_XML_AF_NONE, gdb_xml_parse_attr_ulongest, NULL },
  { "data_addr", GDB_XML_AF_NONE, gdb_xml_parse_attr_ulongest, NULL },
  { "data_size", GDB_XML_AF_NONE, gdb_xml_parse_attr_ulongest, NULL },
  { NULL, GDB_XML_AF_NONE, NULL, NULL }
};

static const struct gdb_xml_element library_list_children[] =
{
  { "library", library_attributes, NULL,
    GDB_XML_EF_REPEATABLE | GDB_XML_EF_OPTIONAL,
    library_list_start_library, NULL},
  { NULL, NULL, NULL, GDB_XML_EF_NONE, NULL, NULL }
};

static const struct gdb_xml_attribute library_list_attributes[] =
{
  { "version", GDB_XML_AF_NONE, NULL, NULL },
  { NULL, GDB_XML_AF_NONE, NULL, NULL }
};

static const struct gdb_xml_element library_list_elements[] =
{
  { "library-list-aix", library_list_attributes, library_list_children,
    GDB_XML_EF_NONE, library_list_start_list, NULL },
  { NULL, NULL, NULL, GDB_XML_EF_NONE, NULL, NULL }
};

/* Parse LIBRARY, a string containing the loader info in XML format,
   and return a vector of lm_info_aix objects.

   Return an empty option if the parsing failed.  */

static std::optional<std::vector<lm_info_aix>>
solib_aix_parse_libraries (const char *library)
{
  std::vector<lm_info_aix> result;

  if (gdb_xml_parse_quick (_("aix library list"), "library-list-aix.dtd",
			   library_list_elements, library, &result) == 0)
    return result;

  return {};
}

#endif /* HAVE_LIBEXPAT */

/* Return the loader info for the given inferior (INF), or an empty
   option if the list could not be computed.

   Cache the result in per-inferior data, so as to avoid recomputing it
   each time this function is called.

   If an error occurs while computing this list, and WARNING_MSG
   is not NULL, then print a warning including WARNING_MSG and
   a description of the error.  */

static std::optional<std::vector<lm_info_aix>> &
solib_aix_get_library_list (struct inferior *inf, const char *warning_msg)
{
  struct solib_aix_inferior_data *data;

  /* If already computed, return the cached value.  */
  data = get_solib_aix_inferior_data (inf);
  if (data->library_list.has_value ())
    return data->library_list;

  std::optional<gdb::char_vector> library_document
    = target_read_stralloc (current_inferior ()->top_target (),
			    TARGET_OBJECT_LIBRARIES_AIX,
			    NULL);
  if (!library_document && warning_msg != NULL)
    {
      warning (_("%s (failed to read TARGET_OBJECT_LIBRARIES_AIX)"),
	       warning_msg);
      return data->library_list;
    }

  solib_debug_printf ("TARGET_OBJECT_LIBRARIES_AIX = %s",
		      library_document->data ());

  data->library_list = solib_aix_parse_libraries (library_document->data ());
  if (!data->library_list.has_value () && warning_msg != NULL)
    warning (_("%s (missing XML support?)"), warning_msg);

  return data->library_list;
}

/* If the .bss section's VMA is set to an address located before
   the end of the .data section, causing the two sections to overlap,
   return the overlap in bytes.  Otherwise, return zero.

   Motivation:

   The GNU linker sometimes sets the start address of the .bss session
   before the end of the .data section, making the 2 sections overlap.
   The loader appears to handle this situation gracefully, by simply
   loading the bss section right after the end of the .data section.

   This means that the .data and the .bss sections are sometimes
   no longer relocated by the same amount.  The problem is that
   the ldinfo data does not contain any information regarding
   the relocation of the .bss section, assuming that it would be
   identical to the information provided for the .data section
   (this is what would normally happen if the program was linked
   correctly).

   GDB therefore needs to detect those cases, and make the corresponding
   adjustment to the .bss section offset computed from the ldinfo data
   when necessary.  This function returns the adjustment amount  (or
   zero when no adjustment is needed).  */

static CORE_ADDR
solib_aix_bss_data_overlap (bfd *abfd)
{
  struct bfd_section *data_sect, *bss_sect;

  data_sect = bfd_get_section_by_name (abfd, ".data");
  if (data_sect == NULL)
    return 0; /* No overlap possible.  */

  bss_sect = bfd_get_section_by_name (abfd, ".bss");
  if (bss_sect == NULL)
    return 0; /* No overlap possible.  */

  /* Assume the problem only occurs with linkers that place the .bss
     section after the .data section (the problem has only been
     observed when using the GNU linker, and the default linker
     script always places the .data and .bss sections in that order).  */
  if (bfd_section_vma (bss_sect) < bfd_section_vma (data_sect))
    return 0;

  if (bfd_section_vma (bss_sect)
      < bfd_section_vma (data_sect) + bfd_section_size (data_sect))
    return (bfd_section_vma (data_sect) + bfd_section_size (data_sect)
	    - bfd_section_vma (bss_sect));

  return 0;
}

/* Implement the "relocate_section_addresses" target_so_ops method.  */

static void
solib_aix_relocate_section_addresses (shobj &so, target_section *sec)
{
  struct bfd_section *bfd_sect = sec->the_bfd_section;
  bfd *abfd = bfd_sect->owner;
  const char *section_name = bfd_section_name (bfd_sect);
  auto *info = gdb::checked_static_cast<lm_info_aix *> (so.lm_info.get ());

  if (strcmp (section_name, ".text") == 0)
    {
      sec->addr = info->text_addr;
      sec->endaddr = sec->addr + info->text_size;

      /* The text address given to us by the loader contains
	 XCOFF headers, so we need to adjust by this much.  */
      sec->addr += bfd_sect->filepos;
    }
  else if (strcmp (section_name, ".data") == 0)
    {
      sec->addr = info->data_addr;
      sec->endaddr = sec->addr + info->data_size;
    }
  else if (strcmp (section_name, ".bss") == 0)
    {
      /* The information provided by the loader does not include
	 the address of the .bss section, but we know that it gets
	 relocated by the same offset as the .data section.  So,
	 compute the relocation offset for the .data section, and
	 apply it to the .bss section as well.  If the .data section
	 is not defined (which seems highly unlikely), do our best
	 by assuming no relocation.  */
      struct bfd_section *data_sect
	= bfd_get_section_by_name (abfd, ".data");
      CORE_ADDR data_offset = 0;

      if (data_sect != NULL)
	data_offset = info->data_addr - bfd_section_vma (data_sect);

      sec->addr = bfd_section_vma (bfd_sect) + data_offset;
      sec->addr += solib_aix_bss_data_overlap (abfd);
      sec->endaddr = sec->addr + bfd_section_size (bfd_sect);
    }
  else
    {
      /* All other sections should not be relocated.  */
      sec->addr = bfd_section_vma (bfd_sect);
      sec->endaddr = sec->addr + bfd_section_size (bfd_sect);
    }
}

/* Compute and return the OBJFILE's section_offset array, using
   the associated loader info (INFO).  */

static section_offsets
solib_aix_get_section_offsets (struct objfile *objfile,
			       lm_info_aix *info)
{
  bfd *abfd = objfile->obfd.get ();

  section_offsets offsets (objfile->section_offsets.size ());

  /* .text */

  if (objfile->sect_index_text != -1)
    {
      struct bfd_section *sect
	= objfile->sections_start[objfile->sect_index_text].the_bfd_section;

      offsets[objfile->sect_index_text]
	= info->text_addr + sect->filepos - bfd_section_vma (sect);
    }

  /* .data */

  if (objfile->sect_index_data != -1)
    {
      struct bfd_section *sect
	= objfile->sections_start[objfile->sect_index_data].the_bfd_section;

      offsets[objfile->sect_index_data]
	= info->data_addr - bfd_section_vma (sect);
    }

  /* .bss

     The offset of the .bss section should be identical to the offset
     of the .data section.  If no .data section (which seems hard to
     believe it is possible), assume it is zero.  */

  if (objfile->sect_index_bss != -1
      && objfile->sect_index_data != -1)
    {
      offsets[objfile->sect_index_bss]
	= (offsets[objfile->sect_index_data]
	   + solib_aix_bss_data_overlap (abfd));
    }

  /* All other sections should not need relocation.  */

  return offsets;
}

/* Implement the "solib_create_inferior_hook" target_so_ops method.  */

static void
solib_aix_solib_create_inferior_hook (int from_tty)
{
  const char *warning_msg = "unable to relocate main executable";

  /* We need to relocate the main executable...  */

  std::optional<std::vector<lm_info_aix>> &library_list
    = solib_aix_get_library_list (current_inferior (), warning_msg);
  if (!library_list.has_value ())
    return;  /* Warning already printed.  */

  if (library_list->empty ())
    {
      warning (_("unable to relocate main executable (no info from loader)"));
      return;
    }

  lm_info_aix &exec_info = (*library_list)[0];
  if (current_program_space->symfile_object_file != NULL)
    {
      objfile *objf = current_program_space->symfile_object_file;
      section_offsets offsets = solib_aix_get_section_offsets (objf,
							       &exec_info);

      objfile_relocate (objf, offsets);
    }
}

/* Implement the "current_sos" target_so_ops method.  */

static intrusive_list<shobj>
solib_aix_current_sos ()
{
  std::optional<std::vector<lm_info_aix>> &library_list
    = solib_aix_get_library_list (current_inferior (), NULL);
  if (!library_list.has_value ())
    return {};

  intrusive_list<shobj> sos;

  /* Build a struct shobj for each entry on the list.
     We skip the first entry, since this is the entry corresponding
     to the main executable, not a shared library.  */
  for (int ix = 1; ix < library_list->size (); ix++)
    {
      shobj *new_solib = new shobj;
      std::string so_name;

      lm_info_aix &info = (*library_list)[ix];
      if (info.member_name.empty ())
	{
	 /* INFO.FILENAME is probably not an archive, but rather
	    a shared object.  Unusual, but it should be possible
	    to link a program against a shared object directory,
	    without having to put it in an archive first.  */
	 so_name = info.filename;
	}
      else
	{
	 /* This is the usual case on AIX, where the shared object
	    is a member of an archive.  Create a synthetic so_name
	    that follows the same convention as AIX's ldd tool
	    (Eg: "/lib/libc.a(shr.o)").  */
	 so_name = string_printf ("%s(%s)", info.filename.c_str (),
				  info.member_name.c_str ());
	}

      new_solib->so_original_name = so_name;
      new_solib->so_name = so_name;
      new_solib->lm_info = std::make_unique<lm_info_aix> (info);

      /* Add it to the list.  */
      sos.push_back (*new_solib);
    }

  return sos;
}

/* Implement the "open_symbol_file_object" target_so_ops method.  */

static int
solib_aix_open_symbol_file_object (int from_tty)
{
  return 0;
}

/* Implement the "in_dynsym_resolve_code" target_so_ops method.  */

static int
solib_aix_in_dynsym_resolve_code (CORE_ADDR pc)
{
  return 0;
}

/* Implement the "bfd_open" target_so_ops method.  */

static gdb_bfd_ref_ptr
solib_aix_bfd_open (const char *pathname)
{
  /* The pathname is actually a synthetic filename with the following
     form: "/path/to/sharedlib(member.o)" (double-quotes excluded).
     split this into archive name and member name.

     FIXME: This is a little hacky.  Perhaps we should provide access
     to the solib's lm_info here?  */
  const int path_len = strlen (pathname);
  const char *sep;
  int filename_len;
  int found_file;

  if (pathname[path_len - 1] != ')')
    return solib_bfd_open (pathname);

  /* Search for the associated parens.  */
  sep = strrchr (pathname, '(');
  if (sep == NULL)
    {
      /* Should never happen, but recover as best as we can (trying
	 to open pathname without decoding, possibly leading to
	 a failure), rather than triggering an assert failure).  */
      warning (_("missing '(' in shared object pathname: %s"), pathname);
      return solib_bfd_open (pathname);
    }
  filename_len = sep - pathname;

  std::string filename (string_printf ("%.*s", filename_len, pathname));
  std::string member_name (string_printf ("%.*s", path_len - filename_len - 2,
					  sep + 1));

  /* Calling solib_find makes certain that sysroot path is set properly
     if program has a dependency on .a archive and sysroot is set via
     set sysroot command.  */
  gdb::unique_xmalloc_ptr<char> found_pathname
    = solib_find (filename.c_str (), &found_file);
  if (found_pathname == NULL)
      perror_with_name (pathname);
  gdb_bfd_ref_ptr archive_bfd (solib_bfd_fopen (found_pathname.get (),
						found_file));
  if (archive_bfd == NULL)
    {
      warning (_("Could not open `%s' as an executable file: %s"),
	       filename.c_str (), bfd_errmsg (bfd_get_error ()));
      return NULL;
    }

  if (bfd_check_format (archive_bfd.get (), bfd_object))
    return archive_bfd;

  if (! bfd_check_format (archive_bfd.get (), bfd_archive))
    {
      warning (_("\"%s\": not in executable format: %s."),
	       filename.c_str (), bfd_errmsg (bfd_get_error ()));
      return NULL;
    }

  gdb_bfd_ref_ptr object_bfd
    (gdb_bfd_openr_next_archived_file (archive_bfd.get (), NULL));
  while (object_bfd != NULL)
    {
      if (member_name == bfd_get_filename (object_bfd.get ()))
	break;

      std::string s = bfd_get_filename (object_bfd.get ());

      /* For every inferior after first int bfd system we
	 will have the pathname instead of the member name
	 registered. Hence the below condition exists.  */

      if (s.find ('(') != std::string::npos)
	{
	  int pos = s.find ('(');
	  int len = s.find (')') - s.find ('(');
	  if (s.substr (pos+1, len-1) == member_name)
	    return object_bfd;
	}

      object_bfd = gdb_bfd_openr_next_archived_file (archive_bfd.get (),
						     object_bfd.get ());
    }

  if (object_bfd == NULL)
    {
      warning (_("\"%s\": member \"%s\" missing."), filename.c_str (),
	       member_name.c_str ());
      return NULL;
    }

  if (! bfd_check_format (object_bfd.get (), bfd_object))
    {
      warning (_("%s(%s): not in object format: %s."),
	       filename.c_str (), member_name.c_str (),
	       bfd_errmsg (bfd_get_error ()));
      return NULL;
    }

  /* Override the returned bfd's name with the name returned from solib_find
     along with appended parenthesized member name in order to allow commands
     listing all shared libraries to display.  Otherwise, we would only be
     displaying the name of the archive member object.  */
  std::string fname = string_printf ("%s%s",
				     bfd_get_filename (archive_bfd.get ()),
				     sep);
  bfd_set_filename (object_bfd.get (), fname.c_str ());

  return object_bfd;
}

/* Return the obj_section corresponding to OBJFILE's data section,
   or NULL if not found.  */
/* FIXME: Define in a more general location? */

static struct obj_section *
data_obj_section_from_objfile (struct objfile *objfile)
{
  for (obj_section *osect : objfile->sections ())
    if (strcmp (bfd_section_name (osect->the_bfd_section), ".data") == 0)
      return osect;

  return NULL;
}

/* Return the TOC value corresponding to the given PC address,
   or raise an error if the value could not be determined.  */

CORE_ADDR
solib_aix_get_toc_value (CORE_ADDR pc)
{
  struct obj_section *pc_osect = find_pc_section (pc);
  struct obj_section *data_osect;
  CORE_ADDR result;

  if (pc_osect == NULL)
    error (_("unable to find TOC entry for pc %s "
	     "(no section contains this PC)"),
	   core_addr_to_string (pc));

  data_osect = data_obj_section_from_objfile (pc_osect->objfile);
  if (data_osect == NULL)
    error (_("unable to find TOC entry for pc %s "
	     "(%s has no data section)"),
	   core_addr_to_string (pc), objfile_name (pc_osect->objfile));

  result = data_osect->addr () + xcoff_get_toc_offset (pc_osect->objfile);

  solib_debug_printf ("pc=%s -> %s", core_addr_to_string (pc),
		      core_addr_to_string (result));

  return result;
}

/* This module's normal_stop observer.  */

static void
solib_aix_normal_stop_observer (struct bpstat *unused_1, int unused_2)
{
  struct solib_aix_inferior_data *data
    = get_solib_aix_inferior_data (current_inferior ());

  /* The inferior execution has been resumed, and it just stopped
     again.  This means that the list of shared libraries may have
     evolved.  Reset our cached value.  */
  data->library_list.reset ();
}

/* The target_so_ops for AIX targets.  */
const struct target_so_ops solib_aix_so_ops =
{
  solib_aix_relocate_section_addresses,
  nullptr,
  nullptr,
  solib_aix_solib_create_inferior_hook,
  solib_aix_current_sos,
  solib_aix_open_symbol_file_object,
  solib_aix_in_dynsym_resolve_code,
  solib_aix_bfd_open,
};

void _initialize_solib_aix ();
void
_initialize_solib_aix ()
{
  gdb::observers::normal_stop.attach (solib_aix_normal_stop_observer,
				      "solib-aix");
}
