/* Symbol table lookup for the GNU debugger, GDB.

   Copyright (C) 1986-2024 Free Software Foundation, Inc.

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
#include "dwarf2/call-site.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "gdbcore.h"
#include "frame.h"
#include "target.h"
#include "value.h"
#include "symfile.h"
#include "objfiles.h"
#include "gdbcmd.h"
#include "gdbsupport/gdb_regex.h"
#include "expression.h"
#include "language.h"
#include "demangle.h"
#include "inferior.h"
#include "source.h"
#include "filenames.h"
#include "objc-lang.h"
#include "d-lang.h"
#include "ada-lang.h"
#include "go-lang.h"
#include "p-lang.h"
#include "addrmap.h"
#include "cli/cli-utils.h"
#include "cli/cli-style.h"
#include "cli/cli-cmds.h"
#include "fnmatch.h"
#include "hashtab.h"
#include "typeprint.h"

#include "gdbsupport/gdb_obstack.h"
#include "block.h"
#include "dictionary.h"

#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <ctype.h>
#include "cp-abi.h"
#include "cp-support.h"
#include "observable.h"
#include "solist.h"
#include "macrotab.h"
#include "macroscope.h"

#include "parser-defs.h"
#include "completer.h"
#include "progspace-and-thread.h"
#include <optional>
#include "filename-seen-cache.h"
#include "arch-utils.h"
#include <algorithm>
#include <string_view>
#include "gdbsupport/pathstuff.h"
#include "gdbsupport/common-utils.h"
#include <optional>

/* Forward declarations for local functions.  */

static void rbreak_command (const char *, int);

static int find_line_common (const linetable *, int, int *, int);

static struct block_symbol
  lookup_symbol_aux (const char *name,
		     symbol_name_match_type match_type,
		     const struct block *block,
		     const domain_enum domain,
		     enum language language,
		     struct field_of_this_result *);

static
struct block_symbol lookup_local_symbol (const char *name,
					 symbol_name_match_type match_type,
					 const struct block *block,
					 const domain_enum domain,
					 enum language language);

static struct block_symbol
  lookup_symbol_in_objfile (struct objfile *objfile,
			    enum block_enum block_index,
			    const char *name, const domain_enum domain);

static void set_main_name (program_space *pspace, const char *name,
			   language lang);

/* Type of the data stored on the program space.  */

struct main_info
{
  /* Name of "main".  */

  std::string name_of_main;

  /* Language of "main".  */

  enum language language_of_main = language_unknown;
};

/* Program space key for finding name and language of "main".  */

static const registry<program_space>::key<main_info> main_progspace_key;

/* The default symbol cache size.
   There is no extra cpu cost for large N (except when flushing the cache,
   which is rare).  The value here is just a first attempt.  A better default
   value may be higher or lower.  A prime number can make up for a bad hash
   computation, so that's why the number is what it is.  */
#define DEFAULT_SYMBOL_CACHE_SIZE 1021

/* The maximum symbol cache size.
   There's no method to the decision of what value to use here, other than
   there's no point in allowing a user typo to make gdb consume all memory.  */
#define MAX_SYMBOL_CACHE_SIZE (1024*1024)

/* symbol_cache_lookup returns this if a previous lookup failed to find the
   symbol in any objfile.  */
#define SYMBOL_LOOKUP_FAILED \
 ((struct block_symbol) {(struct symbol *) 1, NULL})
#define SYMBOL_LOOKUP_FAILED_P(SIB) (SIB.symbol == (struct symbol *) 1)

/* Recording lookups that don't find the symbol is just as important, if not
   more so, than recording found symbols.  */

enum symbol_cache_slot_state
{
  SYMBOL_SLOT_UNUSED,
  SYMBOL_SLOT_NOT_FOUND,
  SYMBOL_SLOT_FOUND
};

struct symbol_cache_slot
{
  enum symbol_cache_slot_state state;

  /* The objfile that was current when the symbol was looked up.
     This is only needed for global blocks, but for simplicity's sake
     we allocate the space for both.  If data shows the extra space used
     for static blocks is a problem, we can split things up then.

     Global blocks need cache lookup to include the objfile context because
     we need to account for gdbarch_iterate_over_objfiles_in_search_order
     which can traverse objfiles in, effectively, any order, depending on
     the current objfile, thus affecting which symbol is found.  Normally,
     only the current objfile is searched first, and then the rest are
     searched in recorded order; but putting cache lookup inside
     gdbarch_iterate_over_objfiles_in_search_order would be awkward.
     Instead we just make the current objfile part of the context of
     cache lookup.  This means we can record the same symbol multiple times,
     each with a different "current objfile" that was in effect when the
     lookup was saved in the cache, but cache space is pretty cheap.  */
  const struct objfile *objfile_context;

  union
  {
    struct block_symbol found;
    struct
    {
      char *name;
      domain_enum domain;
    } not_found;
  } value;
};

/* Clear out SLOT.  */

static void
symbol_cache_clear_slot (struct symbol_cache_slot *slot)
{
  if (slot->state == SYMBOL_SLOT_NOT_FOUND)
    xfree (slot->value.not_found.name);
  slot->state = SYMBOL_SLOT_UNUSED;
}

/* Symbols don't specify global vs static block.
   So keep them in separate caches.  */

struct block_symbol_cache
{
  unsigned int hits;
  unsigned int misses;
  unsigned int collisions;

  /* SYMBOLS is a variable length array of this size.
     One can imagine that in general one cache (global/static) should be a
     fraction of the size of the other, but there's no data at the moment
     on which to decide.  */
  unsigned int size;

  struct symbol_cache_slot symbols[1];
};

/* Clear all slots of BSC and free BSC.  */

static void
destroy_block_symbol_cache (struct block_symbol_cache *bsc)
{
  if (bsc != nullptr)
    {
      for (unsigned int i = 0; i < bsc->size; i++)
	symbol_cache_clear_slot (&bsc->symbols[i]);
      xfree (bsc);
    }
}

/* The symbol cache.

   Searching for symbols in the static and global blocks over multiple objfiles
   again and again can be slow, as can searching very big objfiles.  This is a
   simple cache to improve symbol lookup performance, which is critical to
   overall gdb performance.

   Symbols are hashed on the name, its domain, and block.
   They are also hashed on their objfile for objfile-specific lookups.  */

struct symbol_cache
{
  symbol_cache () = default;

  ~symbol_cache ()
  {
    destroy_block_symbol_cache (global_symbols);
    destroy_block_symbol_cache (static_symbols);
  }

  struct block_symbol_cache *global_symbols = nullptr;
  struct block_symbol_cache *static_symbols = nullptr;
};

/* Program space key for finding its symbol cache.  */

static const registry<program_space>::key<symbol_cache> symbol_cache_key;

/* When non-zero, print debugging messages related to symtab creation.  */
unsigned int symtab_create_debug = 0;

/* When non-zero, print debugging messages related to symbol lookup.  */
unsigned int symbol_lookup_debug = 0;

/* The size of the cache is staged here.  */
static unsigned int new_symbol_cache_size = DEFAULT_SYMBOL_CACHE_SIZE;

/* The current value of the symbol cache size.
   This is saved so that if the user enters a value too big we can restore
   the original value from here.  */
static unsigned int symbol_cache_size = DEFAULT_SYMBOL_CACHE_SIZE;

/* True if a file may be known by two different basenames.
   This is the uncommon case, and significantly slows down gdb.
   Default set to "off" to not slow down the common case.  */
bool basenames_may_differ = false;

/* Allow the user to configure the debugger behavior with respect
   to multiple-choice menus when more than one symbol matches during
   a symbol lookup.  */

const char multiple_symbols_ask[] = "ask";
const char multiple_symbols_all[] = "all";
const char multiple_symbols_cancel[] = "cancel";
static const char *const multiple_symbols_modes[] =
{
  multiple_symbols_ask,
  multiple_symbols_all,
  multiple_symbols_cancel,
  NULL
};
static const char *multiple_symbols_mode = multiple_symbols_all;

/* When TRUE, ignore the prologue-end flag in linetable_entry when searching
   for the SAL past a function prologue.  */
static bool ignore_prologue_end_flag = false;

/* Read-only accessor to AUTO_SELECT_MODE.  */

const char *
multiple_symbols_select_mode (void)
{
  return multiple_symbols_mode;
}

/* Return the name of a domain_enum.  */

const char *
domain_name (domain_enum e)
{
  switch (e)
    {
    case UNDEF_DOMAIN: return "UNDEF_DOMAIN";
    case VAR_DOMAIN: return "VAR_DOMAIN";
    case STRUCT_DOMAIN: return "STRUCT_DOMAIN";
    case MODULE_DOMAIN: return "MODULE_DOMAIN";
    case LABEL_DOMAIN: return "LABEL_DOMAIN";
    case COMMON_BLOCK_DOMAIN: return "COMMON_BLOCK_DOMAIN";
    default: gdb_assert_not_reached ("bad domain_enum");
    }
}

/* Return the name of a search_domain .  */

const char *
search_domain_name (enum search_domain e)
{
  switch (e)
    {
    case VARIABLES_DOMAIN: return "VARIABLES_DOMAIN";
    case FUNCTIONS_DOMAIN: return "FUNCTIONS_DOMAIN";
    case TYPES_DOMAIN: return "TYPES_DOMAIN";
    case MODULES_DOMAIN: return "MODULES_DOMAIN";
    case ALL_DOMAIN: return "ALL_DOMAIN";
    default: gdb_assert_not_reached ("bad search_domain");
    }
}

/* See symtab.h.  */

CORE_ADDR
linetable_entry::pc (const struct objfile *objfile) const
{
  return CORE_ADDR (m_pc) + objfile->text_section_offset ();
}

/* See symtab.h.  */

call_site *
compunit_symtab::find_call_site (CORE_ADDR pc) const
{
  if (m_call_site_htab == nullptr)
    return nullptr;

  CORE_ADDR delta = this->objfile ()->text_section_offset ();
  unrelocated_addr unrelocated_pc = (unrelocated_addr) (pc - delta);

  struct call_site call_site_local (unrelocated_pc, nullptr, nullptr);
  void **slot
    = htab_find_slot (m_call_site_htab, &call_site_local, NO_INSERT);
  if (slot != nullptr)
    return (call_site *) *slot;

  /* See if the arch knows another PC we should try.  On some
     platforms, GCC emits a DWARF call site that is offset from the
     actual return location.  */
  struct gdbarch *arch = objfile ()->arch ();
  CORE_ADDR new_pc = gdbarch_update_call_site_pc (arch, pc);
  if (pc == new_pc)
    return nullptr;

  unrelocated_pc = (unrelocated_addr) (new_pc - delta);
  call_site new_call_site_local (unrelocated_pc, nullptr, nullptr);
  slot = htab_find_slot (m_call_site_htab, &new_call_site_local, NO_INSERT);
  if (slot == nullptr)
    return nullptr;

  return (call_site *) *slot;
}

/* See symtab.h.  */

void
compunit_symtab::set_call_site_htab (htab_t call_site_htab)
{
  gdb_assert (m_call_site_htab == nullptr);
  m_call_site_htab = call_site_htab;
}

/* See symtab.h.  */

void
compunit_symtab::set_primary_filetab (symtab *primary_filetab)
{
  symtab *prev_filetab = nullptr;

  /* Move PRIMARY_FILETAB to the head of the filetab list.  */
  for (symtab *filetab : this->filetabs ())
    {
      if (filetab == primary_filetab)
	{
	  if (prev_filetab != nullptr)
	    {
	      prev_filetab->next = primary_filetab->next;
	      primary_filetab->next = m_filetabs;
	      m_filetabs = primary_filetab;
	    }

	  break;
	}

      prev_filetab = filetab;
    }

  gdb_assert (primary_filetab == m_filetabs);
}

/* See symtab.h.  */

struct symtab *
compunit_symtab::primary_filetab () const
{
  gdb_assert (m_filetabs != nullptr);

  /* The primary file symtab is the first one in the list.  */
  return m_filetabs;
}

/* See symtab.h.  */

enum language
compunit_symtab::language () const
{
  struct symtab *symtab = primary_filetab ();

  /* The language of the compunit symtab is the language of its
     primary source file.  */
  return symtab->language ();
}

/* The relocated address of the minimal symbol, using the section
   offsets from OBJFILE.  */

CORE_ADDR
minimal_symbol::value_address (objfile *objfile) const
{
  if (this->maybe_copied (objfile))
    return this->get_maybe_copied_address (objfile);
  else
    return (CORE_ADDR (this->unrelocated_address ())
	    + objfile->section_offsets[this->section_index ()]);
}

/* See symtab.h.  */

bool
minimal_symbol::data_p () const
{
  return m_type == mst_data
    || m_type == mst_bss
    || m_type == mst_abs
    || m_type == mst_file_data
    || m_type == mst_file_bss;
}

/* See symtab.h.  */

bool
minimal_symbol::text_p () const
{
  return m_type == mst_text
    || m_type == mst_text_gnu_ifunc
    || m_type == mst_data_gnu_ifunc
    || m_type == mst_slot_got_plt
    || m_type == mst_solib_trampoline
    || m_type == mst_file_text;
}

/* See symtab.h.  */

bool
minimal_symbol::maybe_copied (objfile *objfile) const
{
  return (objfile->object_format_has_copy_relocs
	  && (objfile->flags & OBJF_MAINLINE) == 0
	  && (m_type == mst_data || m_type == mst_bss));
}

/* See whether FILENAME matches SEARCH_NAME using the rule that we
   advertise to the user.  (The manual's description of linespecs
   describes what we advertise).  Returns true if they match, false
   otherwise.  */

bool
compare_filenames_for_search (const char *filename, const char *search_name)
{
  int len = strlen (filename);
  size_t search_len = strlen (search_name);

  if (len < search_len)
    return false;

  /* The tail of FILENAME must match.  */
  if (FILENAME_CMP (filename + len - search_len, search_name) != 0)
    return false;

  /* Either the names must completely match, or the character
     preceding the trailing SEARCH_NAME segment of FILENAME must be a
     directory separator.

     The check !IS_ABSOLUTE_PATH ensures SEARCH_NAME "/dir/file.c"
     cannot match FILENAME "/path//dir/file.c" - as user has requested
     absolute path.  The sama applies for "c:\file.c" possibly
     incorrectly hypothetically matching "d:\dir\c:\file.c".

     The HAS_DRIVE_SPEC purpose is to make FILENAME "c:file.c"
     compatible with SEARCH_NAME "file.c".  In such case a compiler had
     to put the "c:file.c" name into debug info.  Such compatibility
     works only on GDB built for DOS host.  */
  return (len == search_len
	  || (!IS_ABSOLUTE_PATH (search_name)
	      && IS_DIR_SEPARATOR (filename[len - search_len - 1]))
	  || (HAS_DRIVE_SPEC (filename)
	      && STRIP_DRIVE_SPEC (filename) == &filename[len - search_len]));
}

/* Same as compare_filenames_for_search, but for glob-style patterns.
   Heads up on the order of the arguments.  They match the order of
   compare_filenames_for_search, but it's the opposite of the order of
   arguments to gdb_filename_fnmatch.  */

bool
compare_glob_filenames_for_search (const char *filename,
				   const char *search_name)
{
  /* We rely on the property of glob-style patterns with FNM_FILE_NAME that
     all /s have to be explicitly specified.  */
  int file_path_elements = count_path_elements (filename);
  int search_path_elements = count_path_elements (search_name);

  if (search_path_elements > file_path_elements)
    return false;

  if (IS_ABSOLUTE_PATH (search_name))
    {
      return (search_path_elements == file_path_elements
	      && gdb_filename_fnmatch (search_name, filename,
				       FNM_FILE_NAME | FNM_NOESCAPE) == 0);
    }

  {
    const char *file_to_compare
      = strip_leading_path_elements (filename,
				     file_path_elements - search_path_elements);

    return gdb_filename_fnmatch (search_name, file_to_compare,
				 FNM_FILE_NAME | FNM_NOESCAPE) == 0;
  }
}

/* Check for a symtab of a specific name by searching some symtabs.
   This is a helper function for callbacks of iterate_over_symtabs.

   If NAME is not absolute, then REAL_PATH is NULL
   If NAME is absolute, then REAL_PATH is the gdb_realpath form of NAME.

   The return value, NAME, REAL_PATH and CALLBACK are identical to the
   `map_symtabs_matching_filename' method of quick_symbol_functions.

   FIRST and AFTER_LAST indicate the range of compunit symtabs to search.
   Each symtab within the specified compunit symtab is also searched.
   AFTER_LAST is one past the last compunit symtab to search; NULL means to
   search until the end of the list.  */

bool
iterate_over_some_symtabs (const char *name,
			   const char *real_path,
			   struct compunit_symtab *first,
			   struct compunit_symtab *after_last,
			   gdb::function_view<bool (symtab *)> callback)
{
  struct compunit_symtab *cust;
  const char* base_name = lbasename (name);

  for (cust = first; cust != NULL && cust != after_last; cust = cust->next)
    {
      /* Skip included compunits.  */
      if (cust->user != nullptr)
	continue;

      for (symtab *s : cust->filetabs ())
	{
	  if (compare_filenames_for_search (s->filename, name))
	    {
	      if (callback (s))
		return true;
	      continue;
	    }

	  /* Before we invoke realpath, which can get expensive when many
	     files are involved, do a quick comparison of the basenames.  */
	  if (! basenames_may_differ
	      && FILENAME_CMP (base_name, lbasename (s->filename)) != 0)
	    continue;

	  if (compare_filenames_for_search (symtab_to_fullname (s), name))
	    {
	      if (callback (s))
		return true;
	      continue;
	    }

	  /* If the user gave us an absolute path, try to find the file in
	     this symtab and use its absolute path.  */
	  if (real_path != NULL)
	    {
	      const char *fullname = symtab_to_fullname (s);

	      gdb_assert (IS_ABSOLUTE_PATH (real_path));
	      gdb_assert (IS_ABSOLUTE_PATH (name));
	      gdb::unique_xmalloc_ptr<char> fullname_real_path
		= gdb_realpath (fullname);
	      fullname = fullname_real_path.get ();
	      if (FILENAME_CMP (real_path, fullname) == 0)
		{
		  if (callback (s))
		    return true;
		  continue;
		}
	    }
	}
    }

  return false;
}

/* Check for a symtab of a specific name; first in symtabs, then in
   psymtabs.  *If* there is no '/' in the name, a match after a '/'
   in the symtab filename will also work.

   Calls CALLBACK with each symtab that is found.  If CALLBACK returns
   true, the search stops.  */

void
iterate_over_symtabs (const char *name,
		      gdb::function_view<bool (symtab *)> callback)
{
  gdb::unique_xmalloc_ptr<char> real_path;

  /* Here we are interested in canonicalizing an absolute path, not
     absolutizing a relative path.  */
  if (IS_ABSOLUTE_PATH (name))
    {
      real_path = gdb_realpath (name);
      gdb_assert (IS_ABSOLUTE_PATH (real_path.get ()));
    }

  for (objfile *objfile : current_program_space->objfiles ())
    {
      if (iterate_over_some_symtabs (name, real_path.get (),
				     objfile->compunit_symtabs, NULL,
				     callback))
	return;
    }

  /* Same search rules as above apply here, but now we look thru the
     psymtabs.  */

  for (objfile *objfile : current_program_space->objfiles ())
    {
      if (objfile->map_symtabs_matching_filename (name, real_path.get (),
						  callback))
	return;
    }
}

/* A wrapper for iterate_over_symtabs that returns the first matching
   symtab, or NULL.  */

struct symtab *
lookup_symtab (const char *name)
{
  struct symtab *result = NULL;

  iterate_over_symtabs (name, [&] (symtab *symtab)
    {
      result = symtab;
      return true;
    });

  return result;
}


/* Mangle a GDB method stub type.  This actually reassembles the pieces of the
   full method name, which consist of the class name (from T), the unadorned
   method name from METHOD_ID, and the signature for the specific overload,
   specified by SIGNATURE_ID.  Note that this function is g++ specific.  */

char *
gdb_mangle_name (struct type *type, int method_id, int signature_id)
{
  int mangled_name_len;
  char *mangled_name;
  struct fn_field *f = TYPE_FN_FIELDLIST1 (type, method_id);
  struct fn_field *method = &f[signature_id];
  const char *field_name = TYPE_FN_FIELDLIST_NAME (type, method_id);
  const char *physname = TYPE_FN_FIELD_PHYSNAME (f, signature_id);
  const char *newname = type->name ();

  /* Does the form of physname indicate that it is the full mangled name
     of a constructor (not just the args)?  */
  int is_full_physname_constructor;

  int is_constructor;
  int is_destructor = is_destructor_name (physname);
  /* Need a new type prefix.  */
  const char *const_prefix = method->is_const ? "C" : "";
  const char *volatile_prefix = method->is_volatile ? "V" : "";
  char buf[20];
  int len = (newname == NULL ? 0 : strlen (newname));

  /* Nothing to do if physname already contains a fully mangled v3 abi name
     or an operator name.  */
  if ((physname[0] == '_' && physname[1] == 'Z')
      || is_operator_name (field_name))
    return xstrdup (physname);

  is_full_physname_constructor = is_constructor_name (physname);

  is_constructor = is_full_physname_constructor 
    || (newname && strcmp (field_name, newname) == 0);

  if (!is_destructor)
    is_destructor = (startswith (physname, "__dt"));

  if (is_destructor || is_full_physname_constructor)
    {
      mangled_name = (char *) xmalloc (strlen (physname) + 1);
      strcpy (mangled_name, physname);
      return mangled_name;
    }

  if (len == 0)
    {
      xsnprintf (buf, sizeof (buf), "__%s%s", const_prefix, volatile_prefix);
    }
  else if (physname[0] == 't' || physname[0] == 'Q')
    {
      /* The physname for template and qualified methods already includes
	 the class name.  */
      xsnprintf (buf, sizeof (buf), "__%s%s", const_prefix, volatile_prefix);
      newname = NULL;
      len = 0;
    }
  else
    {
      xsnprintf (buf, sizeof (buf), "__%s%s%d", const_prefix,
		 volatile_prefix, len);
    }
  mangled_name_len = ((is_constructor ? 0 : strlen (field_name))
		      + strlen (buf) + len + strlen (physname) + 1);

  mangled_name = (char *) xmalloc (mangled_name_len);
  if (is_constructor)
    mangled_name[0] = '\0';
  else
    strcpy (mangled_name, field_name);

  strcat (mangled_name, buf);
  /* If the class doesn't have a name, i.e. newname NULL, then we just
     mangle it using 0 for the length of the class.  Thus it gets mangled
     as something starting with `::' rather than `classname::'.  */
  if (newname != NULL)
    strcat (mangled_name, newname);

  strcat (mangled_name, physname);
  return (mangled_name);
}

/* See symtab.h.  */

void
general_symbol_info::set_demangled_name (const char *name,
					 struct obstack *obstack)
{
  if (language () == language_ada)
    {
      if (name == NULL)
	{
	  ada_mangled = 0;
	  language_specific.obstack = obstack;
	}
      else
	{
	  ada_mangled = 1;
	  language_specific.demangled_name = name;
	}
    }
  else
    language_specific.demangled_name = name;
}


/* Initialize the language dependent portion of a symbol
   depending upon the language for the symbol.  */

void
general_symbol_info::set_language (enum language language,
				   struct obstack *obstack)
{
  m_language = language;
  if (language == language_cplus
      || language == language_d
      || language == language_go
      || language == language_objc
      || language == language_fortran)
    {
      set_demangled_name (NULL, obstack);
    }
  else if (language == language_ada)
    {
      gdb_assert (ada_mangled == 0);
      language_specific.obstack = obstack;
    }
  else
    {
      memset (&language_specific, 0, sizeof (language_specific));
    }
}

/* Functions to initialize a symbol's mangled name.  */

/* Objects of this type are stored in the demangled name hash table.  */
struct demangled_name_entry
{
  demangled_name_entry (std::string_view mangled_name)
    : mangled (mangled_name) {}

  std::string_view mangled;
  enum language language;
  gdb::unique_xmalloc_ptr<char> demangled;
};

/* Hash function for the demangled name hash.  */

static hashval_t
hash_demangled_name_entry (const void *data)
{
  const struct demangled_name_entry *e
    = (const struct demangled_name_entry *) data;

  return gdb::string_view_hash () (e->mangled);
}

/* Equality function for the demangled name hash.  */

static int
eq_demangled_name_entry (const void *a, const void *b)
{
  const struct demangled_name_entry *da
    = (const struct demangled_name_entry *) a;
  const struct demangled_name_entry *db
    = (const struct demangled_name_entry *) b;

  return da->mangled == db->mangled;
}

static void
free_demangled_name_entry (void *data)
{
  struct demangled_name_entry *e
    = (struct demangled_name_entry *) data;

  e->~demangled_name_entry();
}

/* Create the hash table used for demangled names.  Each hash entry is
   a pair of strings; one for the mangled name and one for the demangled
   name.  The entry is hashed via just the mangled name.  */

static void
create_demangled_names_hash (struct objfile_per_bfd_storage *per_bfd)
{
  /* Choose 256 as the starting size of the hash table, somewhat arbitrarily.
     The hash table code will round this up to the next prime number.
     Choosing a much larger table size wastes memory, and saves only about
     1% in symbol reading.  However, if the minsym count is already
     initialized (e.g. because symbol name setting was deferred to
     a background thread) we can initialize the hashtable with a count
     based on that, because we will almost certainly have at least that
     many entries.  If we have a nonzero number but less than 256,
     we still stay with 256 to have some space for psymbols, etc.  */

  /* htab will expand the table when it is 3/4th full, so we account for that
     here.  +2 to round up.  */
  int minsym_based_count = (per_bfd->minimal_symbol_count + 2) / 3 * 4;
  int count = std::max (per_bfd->minimal_symbol_count, minsym_based_count);

  per_bfd->demangled_names_hash.reset (htab_create_alloc
    (count, hash_demangled_name_entry, eq_demangled_name_entry,
     free_demangled_name_entry, xcalloc, xfree));
}

/* See symtab.h  */

gdb::unique_xmalloc_ptr<char>
symbol_find_demangled_name (struct general_symbol_info *gsymbol,
			    const char *mangled)
{
  gdb::unique_xmalloc_ptr<char> demangled;
  int i;

  if (gsymbol->language () != language_unknown)
    {
      const struct language_defn *lang = language_def (gsymbol->language ());

      lang->sniff_from_mangled_name (mangled, &demangled);
      return demangled;
    }

  for (i = language_unknown; i < nr_languages; ++i)
    {
      enum language l = (enum language) i;
      const struct language_defn *lang = language_def (l);

      if (lang->sniff_from_mangled_name (mangled, &demangled))
	{
	  gsymbol->m_language = l;
	  return demangled;
	}
    }

  return NULL;
}

/* Set both the mangled and demangled (if any) names for GSYMBOL based
   on LINKAGE_NAME and LEN.  Ordinarily, NAME is copied onto the
   objfile's obstack; but if COPY_NAME is 0 and if NAME is
   NUL-terminated, then this function assumes that NAME is already
   correctly saved (either permanently or with a lifetime tied to the
   objfile), and it will not be copied.

   The hash table corresponding to OBJFILE is used, and the memory
   comes from the per-BFD storage_obstack.  LINKAGE_NAME is copied,
   so the pointer can be discarded after calling this function.  */

void
general_symbol_info::compute_and_set_names (std::string_view linkage_name,
					    bool copy_name,
					    objfile_per_bfd_storage *per_bfd,
					    std::optional<hashval_t> hash)
{
  struct demangled_name_entry **slot;

  if (language () == language_ada)
    {
      /* In Ada, we do the symbol lookups using the mangled name, so
	 we can save some space by not storing the demangled name.  */
      if (!copy_name)
	m_name = linkage_name.data ();
      else
	m_name = obstack_strndup (&per_bfd->storage_obstack,
				  linkage_name.data (),
				  linkage_name.length ());
      set_demangled_name (NULL, &per_bfd->storage_obstack);

      return;
    }

  if (per_bfd->demangled_names_hash == NULL)
    create_demangled_names_hash (per_bfd);

  struct demangled_name_entry entry (linkage_name);
  if (!hash.has_value ())
    hash = hash_demangled_name_entry (&entry);
  slot = ((struct demangled_name_entry **)
	  htab_find_slot_with_hash (per_bfd->demangled_names_hash.get (),
				    &entry, *hash, INSERT));

  /* The const_cast is safe because the only reason it is already
     initialized is if we purposefully set it from a background
     thread to avoid doing the work here.  However, it is still
     allocated from the heap and needs to be freed by us, just
     like if we called symbol_find_demangled_name here.  If this is
     nullptr, we call symbol_find_demangled_name below, but we put
     this smart pointer here to be sure that we don't leak this name.  */
  gdb::unique_xmalloc_ptr<char> demangled_name
    (const_cast<char *> (language_specific.demangled_name));

  /* If this name is not in the hash table, add it.  */
  if (*slot == NULL
      /* A C version of the symbol may have already snuck into the table.
	 This happens to, e.g., main.init (__go_init_main).  Cope.  */
      || (language () == language_go && (*slot)->demangled == nullptr))
    {
      /* A 0-terminated copy of the linkage name.  Callers must set COPY_NAME
	 to true if the string might not be nullterminated.  We have to make
	 this copy because demangling needs a nullterminated string.  */
      std::string_view linkage_name_copy;
      if (copy_name)
	{
	  char *alloc_name = (char *) alloca (linkage_name.length () + 1);
	  memcpy (alloc_name, linkage_name.data (), linkage_name.length ());
	  alloc_name[linkage_name.length ()] = '\0';

	  linkage_name_copy = std::string_view (alloc_name,
						linkage_name.length ());
	}
      else
	linkage_name_copy = linkage_name;

      if (demangled_name.get () == nullptr)
	 demangled_name
	   = symbol_find_demangled_name (this, linkage_name_copy.data ());

      /* Suppose we have demangled_name==NULL, copy_name==0, and
	 linkage_name_copy==linkage_name.  In this case, we already have the
	 mangled name saved, and we don't have a demangled name.  So,
	 you might think we could save a little space by not recording
	 this in the hash table at all.

	 It turns out that it is actually important to still save such
	 an entry in the hash table, because storing this name gives
	 us better bcache hit rates for partial symbols.  */
      if (!copy_name)
	{
	  *slot
	    = ((struct demangled_name_entry *)
	       obstack_alloc (&per_bfd->storage_obstack,
			      sizeof (demangled_name_entry)));
	  new (*slot) demangled_name_entry (linkage_name);
	}
      else
	{
	  /* If we must copy the mangled name, put it directly after
	     the struct so we can have a single allocation.  */
	  *slot
	    = ((struct demangled_name_entry *)
	       obstack_alloc (&per_bfd->storage_obstack,
			      sizeof (demangled_name_entry)
			      + linkage_name.length () + 1));
	  char *mangled_ptr = reinterpret_cast<char *> (*slot + 1);
	  memcpy (mangled_ptr, linkage_name.data (), linkage_name.length ());
	  mangled_ptr [linkage_name.length ()] = '\0';
	  new (*slot) demangled_name_entry
	    (std::string_view (mangled_ptr, linkage_name.length ()));
	}
      (*slot)->demangled = std::move (demangled_name);
      (*slot)->language = language ();
    }
  else if (language () == language_unknown)
    m_language = (*slot)->language;

  m_name = (*slot)->mangled.data ();
  set_demangled_name ((*slot)->demangled.get (), &per_bfd->storage_obstack);
}

/* See symtab.h.  */

const char *
general_symbol_info::natural_name () const
{
  switch (language ())
    {
    case language_cplus:
    case language_d:
    case language_go:
    case language_objc:
    case language_fortran:
    case language_rust:
      if (language_specific.demangled_name != nullptr)
	return language_specific.demangled_name;
      break;
    case language_ada:
      return ada_decode_symbol (this);
    default:
      break;
    }
  return linkage_name ();
}

/* See symtab.h.  */

const char *
general_symbol_info::demangled_name () const
{
  const char *dem_name = NULL;

  switch (language ())
    {
    case language_cplus:
    case language_d:
    case language_go:
    case language_objc:
    case language_fortran:
    case language_rust:
      dem_name = language_specific.demangled_name;
      break;
    case language_ada:
      dem_name = ada_decode_symbol (this);
      break;
    default:
      break;
    }
  return dem_name;
}

/* See symtab.h.  */

const char *
general_symbol_info::search_name () const
{
  if (language () == language_ada)
    return linkage_name ();
  else
    return natural_name ();
}

/* See symtab.h.  */

struct obj_section *
general_symbol_info::obj_section (const struct objfile *objfile) const
{
  if (section_index () >= 0)
    return &objfile->sections_start[section_index ()];
  return nullptr;
}

/* See symtab.h.  */

bool
symbol_matches_search_name (const struct general_symbol_info *gsymbol,
			    const lookup_name_info &name)
{
  symbol_name_matcher_ftype *name_match
    = language_def (gsymbol->language ())->get_symbol_name_matcher (name);
  return name_match (gsymbol->search_name (), name, NULL);
}



/* Return true if the two sections are the same, or if they could
   plausibly be copies of each other, one in an original object
   file and another in a separated debug file.  */

bool
matching_obj_sections (struct obj_section *obj_first,
		       struct obj_section *obj_second)
{
  asection *first = obj_first? obj_first->the_bfd_section : NULL;
  asection *second = obj_second? obj_second->the_bfd_section : NULL;

  /* If they're the same section, then they match.  */
  if (first == second)
    return true;

  /* If either is NULL, give up.  */
  if (first == NULL || second == NULL)
    return false;

  /* This doesn't apply to absolute symbols.  */
  if (first->owner == NULL || second->owner == NULL)
    return false;

  /* If they're in the same object file, they must be different sections.  */
  if (first->owner == second->owner)
    return false;

  /* Check whether the two sections are potentially corresponding.  They must
     have the same size, address, and name.  We can't compare section indexes,
     which would be more reliable, because some sections may have been
     stripped.  */
  if (bfd_section_size (first) != bfd_section_size (second))
    return false;

  /* In-memory addresses may start at a different offset, relativize them.  */
  if (bfd_section_vma (first) - bfd_get_start_address (first->owner)
      != bfd_section_vma (second) - bfd_get_start_address (second->owner))
    return false;

  if (bfd_section_name (first) == NULL
      || bfd_section_name (second) == NULL
      || strcmp (bfd_section_name (first), bfd_section_name (second)) != 0)
    return false;

  /* Otherwise check that they are in corresponding objfiles.  */

  struct objfile *obj = NULL;
  for (objfile *objfile : current_program_space->objfiles ())
    if (objfile->obfd == first->owner)
      {
	obj = objfile;
	break;
      }
  gdb_assert (obj != NULL);

  if (obj->separate_debug_objfile != NULL
      && obj->separate_debug_objfile->obfd == second->owner)
    return true;
  if (obj->separate_debug_objfile_backlink != NULL
      && obj->separate_debug_objfile_backlink->obfd == second->owner)
    return true;

  return false;
}

/* Hash function for the symbol cache.  */

static unsigned int
hash_symbol_entry (const struct objfile *objfile_context,
		   const char *name, domain_enum domain)
{
  unsigned int hash = (uintptr_t) objfile_context;

  if (name != NULL)
    hash += htab_hash_string (name);

  /* Because of symbol_matches_domain we need VAR_DOMAIN and STRUCT_DOMAIN
     to map to the same slot.  */
  if (domain == STRUCT_DOMAIN)
    hash += VAR_DOMAIN * 7;
  else
    hash += domain * 7;

  return hash;
}

/* Equality function for the symbol cache.  */

static int
eq_symbol_entry (const struct symbol_cache_slot *slot,
		 const struct objfile *objfile_context,
		 const char *name, domain_enum domain)
{
  const char *slot_name;
  domain_enum slot_domain;

  if (slot->state == SYMBOL_SLOT_UNUSED)
    return 0;

  if (slot->objfile_context != objfile_context)
    return 0;

  if (slot->state == SYMBOL_SLOT_NOT_FOUND)
    {
      slot_name = slot->value.not_found.name;
      slot_domain = slot->value.not_found.domain;
    }
  else
    {
      slot_name = slot->value.found.symbol->search_name ();
      slot_domain = slot->value.found.symbol->domain ();
    }

  /* NULL names match.  */
  if (slot_name == NULL && name == NULL)
    {
      /* But there's no point in calling symbol_matches_domain in the
	 SYMBOL_SLOT_FOUND case.  */
      if (slot_domain != domain)
	return 0;
    }
  else if (slot_name != NULL && name != NULL)
    {
      /* It's important that we use the same comparison that was done
	 the first time through.  If the slot records a found symbol,
	 then this means using the symbol name comparison function of
	 the symbol's language with symbol->search_name ().  See
	 dictionary.c.  It also means using symbol_matches_domain for
	 found symbols.  See block.c.

	 If the slot records a not-found symbol, then require a precise match.
	 We could still be lax with whitespace like strcmp_iw though.  */

      if (slot->state == SYMBOL_SLOT_NOT_FOUND)
	{
	  if (strcmp (slot_name, name) != 0)
	    return 0;
	  if (slot_domain != domain)
	    return 0;
	}
      else
	{
	  struct symbol *sym = slot->value.found.symbol;
	  lookup_name_info lookup_name (name, symbol_name_match_type::FULL);

	  if (!symbol_matches_search_name (sym, lookup_name))
	    return 0;

	  if (!symbol_matches_domain (sym->language (), slot_domain, domain))
	    return 0;
	}
    }
  else
    {
      /* Only one name is NULL.  */
      return 0;
    }

  return 1;
}

/* Given a cache of size SIZE, return the size of the struct (with variable
   length array) in bytes.  */

static size_t
symbol_cache_byte_size (unsigned int size)
{
  return (sizeof (struct block_symbol_cache)
	  + ((size - 1) * sizeof (struct symbol_cache_slot)));
}

/* Resize CACHE.  */

static void
resize_symbol_cache (struct symbol_cache *cache, unsigned int new_size)
{
  /* If there's no change in size, don't do anything.
     All caches have the same size, so we can just compare with the size
     of the global symbols cache.  */
  if ((cache->global_symbols != NULL
       && cache->global_symbols->size == new_size)
      || (cache->global_symbols == NULL
	  && new_size == 0))
    return;

  destroy_block_symbol_cache (cache->global_symbols);
  destroy_block_symbol_cache (cache->static_symbols);

  if (new_size == 0)
    {
      cache->global_symbols = NULL;
      cache->static_symbols = NULL;
    }
  else
    {
      size_t total_size = symbol_cache_byte_size (new_size);

      cache->global_symbols
	= (struct block_symbol_cache *) xcalloc (1, total_size);
      cache->static_symbols
	= (struct block_symbol_cache *) xcalloc (1, total_size);
      cache->global_symbols->size = new_size;
      cache->static_symbols->size = new_size;
    }
}

/* Return the symbol cache of PSPACE.
   Create one if it doesn't exist yet.  */

static struct symbol_cache *
get_symbol_cache (struct program_space *pspace)
{
  struct symbol_cache *cache = symbol_cache_key.get (pspace);

  if (cache == NULL)
    {
      cache = symbol_cache_key.emplace (pspace);
      resize_symbol_cache (cache, symbol_cache_size);
    }

  return cache;
}

/* Set the size of the symbol cache in all program spaces.  */

static void
set_symbol_cache_size (unsigned int new_size)
{
  for (struct program_space *pspace : program_spaces)
    {
      struct symbol_cache *cache = symbol_cache_key.get (pspace);

      /* The pspace could have been created but not have a cache yet.  */
      if (cache != NULL)
	resize_symbol_cache (cache, new_size);
    }
}

/* Called when symbol-cache-size is set.  */

static void
set_symbol_cache_size_handler (const char *args, int from_tty,
			       struct cmd_list_element *c)
{
  if (new_symbol_cache_size > MAX_SYMBOL_CACHE_SIZE)
    {
      /* Restore the previous value.
	 This is the value the "show" command prints.  */
      new_symbol_cache_size = symbol_cache_size;

      error (_("Symbol cache size is too large, max is %u."),
	     MAX_SYMBOL_CACHE_SIZE);
    }
  symbol_cache_size = new_symbol_cache_size;

  set_symbol_cache_size (symbol_cache_size);
}

/* Lookup symbol NAME,DOMAIN in BLOCK in the symbol cache of PSPACE.
   OBJFILE_CONTEXT is the current objfile, which may be NULL.
   The result is the symbol if found, SYMBOL_LOOKUP_FAILED if a previous lookup
   failed (and thus this one will too), or NULL if the symbol is not present
   in the cache.
   *BSC_PTR and *SLOT_PTR are set to the cache and slot of the symbol, which
   can be used to save the result of a full lookup attempt.  */

static struct block_symbol
symbol_cache_lookup (struct symbol_cache *cache,
		     struct objfile *objfile_context, enum block_enum block,
		     const char *name, domain_enum domain,
		     struct block_symbol_cache **bsc_ptr,
		     struct symbol_cache_slot **slot_ptr)
{
  struct block_symbol_cache *bsc;
  unsigned int hash;
  struct symbol_cache_slot *slot;

  if (block == GLOBAL_BLOCK)
    bsc = cache->global_symbols;
  else
    bsc = cache->static_symbols;
  if (bsc == NULL)
    {
      *bsc_ptr = NULL;
      *slot_ptr = NULL;
      return {};
    }

  hash = hash_symbol_entry (objfile_context, name, domain);
  slot = bsc->symbols + hash % bsc->size;

  *bsc_ptr = bsc;
  *slot_ptr = slot;

  if (eq_symbol_entry (slot, objfile_context, name, domain))
    {
      symbol_lookup_debug_printf ("%s block symbol cache hit%s for %s, %s",
				  block == GLOBAL_BLOCK ? "Global" : "Static",
				  slot->state == SYMBOL_SLOT_NOT_FOUND
				  ? " (not found)" : "", name,
				  domain_name (domain));
      ++bsc->hits;
      if (slot->state == SYMBOL_SLOT_NOT_FOUND)
	return SYMBOL_LOOKUP_FAILED;
      return slot->value.found;
    }

  /* Symbol is not present in the cache.  */

  symbol_lookup_debug_printf ("%s block symbol cache miss for %s, %s",
			      block == GLOBAL_BLOCK ? "Global" : "Static",
			      name, domain_name (domain));
  ++bsc->misses;
  return {};
}

/* Mark SYMBOL as found in SLOT.
   OBJFILE_CONTEXT is the current objfile when the lookup was done, or NULL
   if it's not needed to distinguish lookups (STATIC_BLOCK).  It is *not*
   necessarily the objfile the symbol was found in.  */

static void
symbol_cache_mark_found (struct block_symbol_cache *bsc,
			 struct symbol_cache_slot *slot,
			 struct objfile *objfile_context,
			 struct symbol *symbol,
			 const struct block *block)
{
  if (bsc == NULL)
    return;
  if (slot->state != SYMBOL_SLOT_UNUSED)
    {
      ++bsc->collisions;
      symbol_cache_clear_slot (slot);
    }
  slot->state = SYMBOL_SLOT_FOUND;
  slot->objfile_context = objfile_context;
  slot->value.found.symbol = symbol;
  slot->value.found.block = block;
}

/* Mark symbol NAME, DOMAIN as not found in SLOT.
   OBJFILE_CONTEXT is the current objfile when the lookup was done, or NULL
   if it's not needed to distinguish lookups (STATIC_BLOCK).  */

static void
symbol_cache_mark_not_found (struct block_symbol_cache *bsc,
			     struct symbol_cache_slot *slot,
			     struct objfile *objfile_context,
			     const char *name, domain_enum domain)
{
  if (bsc == NULL)
    return;
  if (slot->state != SYMBOL_SLOT_UNUSED)
    {
      ++bsc->collisions;
      symbol_cache_clear_slot (slot);
    }
  slot->state = SYMBOL_SLOT_NOT_FOUND;
  slot->objfile_context = objfile_context;
  slot->value.not_found.name = xstrdup (name);
  slot->value.not_found.domain = domain;
}

/* Flush the symbol cache of PSPACE.  */

static void
symbol_cache_flush (struct program_space *pspace)
{
  struct symbol_cache *cache = symbol_cache_key.get (pspace);
  int pass;

  if (cache == NULL)
    return;
  if (cache->global_symbols == NULL)
    {
      gdb_assert (symbol_cache_size == 0);
      gdb_assert (cache->static_symbols == NULL);
      return;
    }

  /* If the cache is untouched since the last flush, early exit.
     This is important for performance during the startup of a program linked
     with 100s (or 1000s) of shared libraries.  */
  if (cache->global_symbols->misses == 0
      && cache->static_symbols->misses == 0)
    return;

  gdb_assert (cache->global_symbols->size == symbol_cache_size);
  gdb_assert (cache->static_symbols->size == symbol_cache_size);

  for (pass = 0; pass < 2; ++pass)
    {
      struct block_symbol_cache *bsc
	= pass == 0 ? cache->global_symbols : cache->static_symbols;
      unsigned int i;

      for (i = 0; i < bsc->size; ++i)
	symbol_cache_clear_slot (&bsc->symbols[i]);
    }

  cache->global_symbols->hits = 0;
  cache->global_symbols->misses = 0;
  cache->global_symbols->collisions = 0;
  cache->static_symbols->hits = 0;
  cache->static_symbols->misses = 0;
  cache->static_symbols->collisions = 0;
}

/* Dump CACHE.  */

static void
symbol_cache_dump (const struct symbol_cache *cache)
{
  int pass;

  if (cache->global_symbols == NULL)
    {
      gdb_printf ("  <disabled>\n");
      return;
    }

  for (pass = 0; pass < 2; ++pass)
    {
      const struct block_symbol_cache *bsc
	= pass == 0 ? cache->global_symbols : cache->static_symbols;
      unsigned int i;

      if (pass == 0)
	gdb_printf ("Global symbols:\n");
      else
	gdb_printf ("Static symbols:\n");

      for (i = 0; i < bsc->size; ++i)
	{
	  const struct symbol_cache_slot *slot = &bsc->symbols[i];

	  QUIT;

	  switch (slot->state)
	    {
	    case SYMBOL_SLOT_UNUSED:
	      break;
	    case SYMBOL_SLOT_NOT_FOUND:
	      gdb_printf ("  [%4u] = %s, %s %s (not found)\n", i,
			  host_address_to_string (slot->objfile_context),
			  slot->value.not_found.name,
			  domain_name (slot->value.not_found.domain));
	      break;
	    case SYMBOL_SLOT_FOUND:
	      {
		struct symbol *found = slot->value.found.symbol;
		const struct objfile *context = slot->objfile_context;

		gdb_printf ("  [%4u] = %s, %s %s\n", i,
			    host_address_to_string (context),
			    found->print_name (),
			    domain_name (found->domain ()));
		break;
	      }
	    }
	}
    }
}

/* The "mt print symbol-cache" command.  */

static void
maintenance_print_symbol_cache (const char *args, int from_tty)
{
  for (struct program_space *pspace : program_spaces)
    {
      struct symbol_cache *cache;

      gdb_printf (_("Symbol cache for pspace %d\n%s:\n"),
		  pspace->num,
		  pspace->symfile_object_file != NULL
		  ? objfile_name (pspace->symfile_object_file)
		  : "(no object file)");

      /* If the cache hasn't been created yet, avoid creating one.  */
      cache = symbol_cache_key.get (pspace);
      if (cache == NULL)
	gdb_printf ("  <empty>\n");
      else
	symbol_cache_dump (cache);
    }
}

/* The "mt flush-symbol-cache" command.  */

static void
maintenance_flush_symbol_cache (const char *args, int from_tty)
{
  for (struct program_space *pspace : program_spaces)
    {
      symbol_cache_flush (pspace);
    }
}

/* Print usage statistics of CACHE.  */

static void
symbol_cache_stats (struct symbol_cache *cache)
{
  int pass;

  if (cache->global_symbols == NULL)
    {
      gdb_printf ("  <disabled>\n");
      return;
    }

  for (pass = 0; pass < 2; ++pass)
    {
      const struct block_symbol_cache *bsc
	= pass == 0 ? cache->global_symbols : cache->static_symbols;

      QUIT;

      if (pass == 0)
	gdb_printf ("Global block cache stats:\n");
      else
	gdb_printf ("Static block cache stats:\n");

      gdb_printf ("  size:       %u\n", bsc->size);
      gdb_printf ("  hits:       %u\n", bsc->hits);
      gdb_printf ("  misses:     %u\n", bsc->misses);
      gdb_printf ("  collisions: %u\n", bsc->collisions);
    }
}

/* The "mt print symbol-cache-statistics" command.  */

static void
maintenance_print_symbol_cache_statistics (const char *args, int from_tty)
{
  for (struct program_space *pspace : program_spaces)
    {
      struct symbol_cache *cache;

      gdb_printf (_("Symbol cache statistics for pspace %d\n%s:\n"),
		  pspace->num,
		  pspace->symfile_object_file != NULL
		  ? objfile_name (pspace->symfile_object_file)
		  : "(no object file)");

      /* If the cache hasn't been created yet, avoid creating one.  */
      cache = symbol_cache_key.get (pspace);
      if (cache == NULL)
	gdb_printf ("  empty, no stats available\n");
      else
	symbol_cache_stats (cache);
    }
}

/* This module's 'new_objfile' observer.  */

static void
symtab_new_objfile_observer (struct objfile *objfile)
{
  symbol_cache_flush (objfile->pspace);
}

/* This module's 'all_objfiles_removed' observer.  */

static void
symtab_all_objfiles_removed (program_space *pspace)
{
  symbol_cache_flush (pspace);

  /* Forget everything we know about the main function.  */
  set_main_name (pspace, nullptr, language_unknown);
}

/* This module's 'free_objfile' observer.  */

static void
symtab_free_objfile_observer (struct objfile *objfile)
{
  symbol_cache_flush (objfile->pspace);
}

/* See symtab.h.  */

void
fixup_symbol_section (struct symbol *sym, struct objfile *objfile)
{
  gdb_assert (sym != nullptr);
  gdb_assert (sym->is_objfile_owned ());
  gdb_assert (objfile != nullptr);
  gdb_assert (sym->section_index () == -1);

  /* Note that if this ends up as -1, fixup_section will handle that
     reasonably well.  So, it's fine to use the objfile's section
     index without doing the check that is done by the wrapper macros
     like SECT_OFF_TEXT.  */
  int fallback;
  switch (sym->aclass ())
    {
    case LOC_STATIC:
      fallback = objfile->sect_index_data;
      break;

    case LOC_LABEL:
      fallback = objfile->sect_index_text;
      break;

    default:
      /* Nothing else will be listed in the minsyms -- no use looking
	 it up.  */
      return;
    }

  CORE_ADDR addr = sym->value_address ();

  struct minimal_symbol *msym;

  /* First, check whether a minimal symbol with the same name exists
     and points to the same address.  The address check is required
     e.g. on PowerPC64, where the minimal symbol for a function will
     point to the function descriptor, while the debug symbol will
     point to the actual function code.  */
  msym = lookup_minimal_symbol_by_pc_name (addr, sym->linkage_name (),
					   objfile);
  if (msym)
    sym->set_section_index (msym->section_index ());
  else
    {
      /* Static, function-local variables do appear in the linker
	 (minimal) symbols, but are frequently given names that won't
	 be found via lookup_minimal_symbol().  E.g., it has been
	 observed in frv-uclinux (ELF) executables that a static,
	 function-local variable named "foo" might appear in the
	 linker symbols as "foo.6" or "foo.3".  Thus, there is no
	 point in attempting to extend the lookup-by-name mechanism to
	 handle this case due to the fact that there can be multiple
	 names.

	 So, instead, search the section table when lookup by name has
	 failed.  The ``addr'' and ``endaddr'' fields may have already
	 been relocated.  If so, the relocation offset needs to be
	 subtracted from these values when performing the comparison.
	 We unconditionally subtract it, because, when no relocation
	 has been performed, the value will simply be zero.

	 The address of the symbol whose section we're fixing up HAS
	 NOT BEEN adjusted (relocated) yet.  It can't have been since
	 the section isn't yet known and knowing the section is
	 necessary in order to add the correct relocation value.  In
	 other words, we wouldn't even be in this function (attempting
	 to compute the section) if it were already known.

	 Note that it is possible to search the minimal symbols
	 (subtracting the relocation value if necessary) to find the
	 matching minimal symbol, but this is overkill and much less
	 efficient.  It is not necessary to find the matching minimal
	 symbol, only its section.

	 Note that this technique (of doing a section table search)
	 can fail when unrelocated section addresses overlap.  For
	 this reason, we still attempt a lookup by name prior to doing
	 a search of the section table.  */

      for (obj_section *s : objfile->sections ())
	{
	  if ((bfd_section_flags (s->the_bfd_section) & SEC_ALLOC) == 0)
	    continue;

	  int idx = s - objfile->sections_start;
	  CORE_ADDR offset = objfile->section_offsets[idx];

	  if (fallback == -1)
	    fallback = idx;

	  if (s->addr () - offset <= addr && addr < s->endaddr () - offset)
	    {
	      sym->set_section_index (idx);
	      return;
	    }
	}

      /* If we didn't find the section, assume it is in the first
	 section.  If there is no allocated section, then it hardly
	 matters what we pick, so just pick zero.  */
      if (fallback == -1)
	sym->set_section_index (0);
      else
	sym->set_section_index (fallback);
    }
}

/* See symtab.h.  */

demangle_for_lookup_info::demangle_for_lookup_info
  (const lookup_name_info &lookup_name, language lang)
{
  demangle_result_storage storage;

  if (lookup_name.ignore_parameters () && lang == language_cplus)
    {
      gdb::unique_xmalloc_ptr<char> without_params
	= cp_remove_params_if_any (lookup_name.c_str (),
				   lookup_name.completion_mode ());

      if (without_params != NULL)
	{
	  if (lookup_name.match_type () != symbol_name_match_type::SEARCH_NAME)
	    m_demangled_name = demangle_for_lookup (without_params.get (),
						    lang, storage);
	  return;
	}
    }

  if (lookup_name.match_type () == symbol_name_match_type::SEARCH_NAME)
    m_demangled_name = lookup_name.c_str ();
  else
    m_demangled_name = demangle_for_lookup (lookup_name.c_str (),
					    lang, storage);
}

/* See symtab.h.  */

const lookup_name_info &
lookup_name_info::match_any ()
{
  /* Lookup any symbol that "" would complete.  I.e., this matches all
     symbol names.  */
  static const lookup_name_info lookup_name ("", symbol_name_match_type::FULL,
					     true);

  return lookup_name;
}

/* Compute the demangled form of NAME as used by the various symbol
   lookup functions.  The result can either be the input NAME
   directly, or a pointer to a buffer owned by the STORAGE object.

   For Ada, this function just returns NAME, unmodified.
   Normally, Ada symbol lookups are performed using the encoded name
   rather than the demangled name, and so it might seem to make sense
   for this function to return an encoded version of NAME.
   Unfortunately, we cannot do this, because this function is used in
   circumstances where it is not appropriate to try to encode NAME.
   For instance, when displaying the frame info, we demangle the name
   of each parameter, and then perform a symbol lookup inside our
   function using that demangled name.  In Ada, certain functions
   have internally-generated parameters whose name contain uppercase
   characters.  Encoding those name would result in those uppercase
   characters to become lowercase, and thus cause the symbol lookup
   to fail.  */

const char *
demangle_for_lookup (const char *name, enum language lang,
		     demangle_result_storage &storage)
{
  /* If we are using C++, D, or Go, demangle the name before doing a
     lookup, so we can always binary search.  */
  if (lang == language_cplus)
    {
      gdb::unique_xmalloc_ptr<char> demangled_name
	= gdb_demangle (name, DMGL_ANSI | DMGL_PARAMS);
      if (demangled_name != NULL)
	return storage.set_malloc_ptr (std::move (demangled_name));

      /* If we were given a non-mangled name, canonicalize it
	 according to the language (so far only for C++).  */
      gdb::unique_xmalloc_ptr<char> canon = cp_canonicalize_string (name);
      if (canon != nullptr)
	return storage.set_malloc_ptr (std::move (canon));
    }
  else if (lang == language_d)
    {
      gdb::unique_xmalloc_ptr<char> demangled_name = d_demangle (name, 0);
      if (demangled_name != NULL)
	return storage.set_malloc_ptr (std::move (demangled_name));
    }
  else if (lang == language_go)
    {
      gdb::unique_xmalloc_ptr<char> demangled_name
	= language_def (language_go)->demangle_symbol (name, 0);
      if (demangled_name != NULL)
	return storage.set_malloc_ptr (std::move (demangled_name));
    }

  return name;
}

/* See symtab.h.  */

unsigned int
search_name_hash (enum language language, const char *search_name)
{
  return language_def (language)->search_name_hash (search_name);
}

/* See symtab.h.

   This function (or rather its subordinates) have a bunch of loops and
   it would seem to be attractive to put in some QUIT's (though I'm not really
   sure whether it can run long enough to be really important).  But there
   are a few calls for which it would appear to be bad news to quit
   out of here: e.g., find_proc_desc in alpha-mdebug-tdep.c.  (Note
   that there is C++ code below which can error(), but that probably
   doesn't affect these calls since they are looking for a known
   variable and thus can probably assume it will never hit the C++
   code).  */

struct block_symbol
lookup_symbol_in_language (const char *name, const struct block *block,
			   const domain_enum domain, enum language lang,
			   struct field_of_this_result *is_a_field_of_this)
{
  SYMBOL_LOOKUP_SCOPED_DEBUG_ENTER_EXIT;

  demangle_result_storage storage;
  const char *modified_name = demangle_for_lookup (name, lang, storage);

  return lookup_symbol_aux (modified_name,
			    symbol_name_match_type::FULL,
			    block, domain, lang,
			    is_a_field_of_this);
}

/* See symtab.h.  */

struct block_symbol
lookup_symbol (const char *name, const struct block *block,
	       domain_enum domain,
	       struct field_of_this_result *is_a_field_of_this)
{
  return lookup_symbol_in_language (name, block, domain,
				    current_language->la_language,
				    is_a_field_of_this);
}

/* See symtab.h.  */

struct block_symbol
lookup_symbol_search_name (const char *search_name, const struct block *block,
			   domain_enum domain)
{
  return lookup_symbol_aux (search_name, symbol_name_match_type::SEARCH_NAME,
			    block, domain, language_asm, NULL);
}

/* See symtab.h.  */

struct block_symbol
lookup_language_this (const struct language_defn *lang,
		      const struct block *block)
{
  if (lang->name_of_this () == NULL || block == NULL)
    return {};

  symbol_lookup_debug_printf_v ("lookup_language_this (%s, %s (objfile %s))",
				lang->name (), host_address_to_string (block),
				objfile_debug_name (block->objfile ()));

  while (block)
    {
      struct symbol *sym;

      sym = block_lookup_symbol (block, lang->name_of_this (),
				 symbol_name_match_type::SEARCH_NAME,
				 VAR_DOMAIN);
      if (sym != NULL)
	{
	  symbol_lookup_debug_printf_v
	    ("lookup_language_this (...) = %s (%s, block %s)",
	     sym->print_name (), host_address_to_string (sym),
	     host_address_to_string (block));
	  return (struct block_symbol) {sym, block};
	}
      if (block->function ())
	break;
      block = block->superblock ();
    }

  symbol_lookup_debug_printf_v ("lookup_language_this (...) = NULL");
  return {};
}

/* Given TYPE, a structure/union,
   return 1 if the component named NAME from the ultimate target
   structure/union is defined, otherwise, return 0.  */

static int
check_field (struct type *type, const char *name,
	     struct field_of_this_result *is_a_field_of_this)
{
  int i;

  /* The type may be a stub.  */
  type = check_typedef (type);

  for (i = type->num_fields () - 1; i >= TYPE_N_BASECLASSES (type); i--)
    {
      const char *t_field_name = type->field (i).name ();

      if (t_field_name && (strcmp_iw (t_field_name, name) == 0))
	{
	  is_a_field_of_this->type = type;
	  is_a_field_of_this->field = &type->field (i);
	  return 1;
	}
    }

  /* C++: If it was not found as a data field, then try to return it
     as a pointer to a method.  */

  for (i = TYPE_NFN_FIELDS (type) - 1; i >= 0; --i)
    {
      if (strcmp_iw (TYPE_FN_FIELDLIST_NAME (type, i), name) == 0)
	{
	  is_a_field_of_this->type = type;
	  is_a_field_of_this->fn_field = &TYPE_FN_FIELDLIST (type, i);
	  return 1;
	}
    }

  for (i = TYPE_N_BASECLASSES (type) - 1; i >= 0; i--)
    if (check_field (TYPE_BASECLASS (type, i), name, is_a_field_of_this))
      return 1;

  return 0;
}

/* Behave like lookup_symbol except that NAME is the natural name
   (e.g., demangled name) of the symbol that we're looking for.  */

static struct block_symbol
lookup_symbol_aux (const char *name, symbol_name_match_type match_type,
		   const struct block *block,
		   const domain_enum domain, enum language language,
		   struct field_of_this_result *is_a_field_of_this)
{
  SYMBOL_LOOKUP_SCOPED_DEBUG_ENTER_EXIT;

  struct block_symbol result;
  const struct language_defn *langdef;

  if (symbol_lookup_debug)
    {
      struct objfile *objfile = (block == nullptr
				 ? nullptr : block->objfile ());

      symbol_lookup_debug_printf
	("demangled symbol name = \"%s\", block @ %s (objfile %s)",
	 name, host_address_to_string (block),
	 objfile != NULL ? objfile_debug_name (objfile) : "NULL");
      symbol_lookup_debug_printf
	("domain name = \"%s\", language = \"%s\")",
	 domain_name (domain), language_str (language));
    }

  /* Make sure we do something sensible with is_a_field_of_this, since
     the callers that set this parameter to some non-null value will
     certainly use it later.  If we don't set it, the contents of
     is_a_field_of_this are undefined.  */
  if (is_a_field_of_this != NULL)
    memset (is_a_field_of_this, 0, sizeof (*is_a_field_of_this));

  /* Search specified block and its superiors.  Don't search
     STATIC_BLOCK or GLOBAL_BLOCK.  */

  result = lookup_local_symbol (name, match_type, block, domain, language);
  if (result.symbol != NULL)
    {
      symbol_lookup_debug_printf
	("found symbol @ %s (using lookup_local_symbol)",
	 host_address_to_string (result.symbol));
      return result;
    }

  /* If requested to do so by the caller and if appropriate for LANGUAGE,
     check to see if NAME is a field of `this'.  */

  langdef = language_def (language);

  /* Don't do this check if we are searching for a struct.  It will
     not be found by check_field, but will be found by other
     means.  */
  if (is_a_field_of_this != NULL && domain != STRUCT_DOMAIN)
    {
      result = lookup_language_this (langdef, block);

      if (result.symbol)
	{
	  struct type *t = result.symbol->type ();

	  /* I'm not really sure that type of this can ever
	     be typedefed; just be safe.  */
	  t = check_typedef (t);
	  if (t->is_pointer_or_reference ())
	    t = t->target_type ();

	  if (t->code () != TYPE_CODE_STRUCT
	      && t->code () != TYPE_CODE_UNION)
	    error (_("Internal error: `%s' is not an aggregate"),
		   langdef->name_of_this ());

	  if (check_field (t, name, is_a_field_of_this))
	    {
	      symbol_lookup_debug_printf ("no symbol found");
	      return {};
	    }
	}
    }

  /* Now do whatever is appropriate for LANGUAGE to look
     up static and global variables.  */

  result = langdef->lookup_symbol_nonlocal (name, block, domain);
  if (result.symbol != NULL)
    {
      symbol_lookup_debug_printf
	("found symbol @ %s (using language lookup_symbol_nonlocal)",
	 host_address_to_string (result.symbol));
      return result;
    }

  /* Now search all static file-level symbols.  Not strictly correct,
     but more useful than an error.  */

  result = lookup_static_symbol (name, domain);
  symbol_lookup_debug_printf
    ("found symbol @ %s (using lookup_static_symbol)",
     result.symbol != NULL ? host_address_to_string (result.symbol) : "NULL");
  return result;
}

/* Check to see if the symbol is defined in BLOCK or its superiors.
   Don't search STATIC_BLOCK or GLOBAL_BLOCK.  */

static struct block_symbol
lookup_local_symbol (const char *name,
		     symbol_name_match_type match_type,
		     const struct block *block,
		     const domain_enum domain,
		     enum language language)
{
  if (block == nullptr)
    return {};

  struct symbol *sym;
  const struct block *static_block = block->static_block ();
  const char *scope = block->scope ();
  
  /* Check if it's a global block.  */
  if (static_block == nullptr)
    return {};

  while (block != static_block)
    {
      sym = lookup_symbol_in_block (name, match_type, block, domain);
      if (sym != NULL)
	return (struct block_symbol) {sym, block};

      if (language == language_cplus || language == language_fortran)
	{
	  struct block_symbol blocksym
	    = cp_lookup_symbol_imports_or_template (scope, name, block,
						    domain);

	  if (blocksym.symbol != NULL)
	    return blocksym;
	}

      if (block->function () != NULL && block->inlined_p ())
	break;
      block = block->superblock ();
    }

  /* We've reached the end of the function without finding a result.  */

  return {};
}

/* See symtab.h.  */

struct symbol *
lookup_symbol_in_block (const char *name, symbol_name_match_type match_type,
			const struct block *block,
			const domain_enum domain)
{
  struct symbol *sym;

  if (symbol_lookup_debug)
    {
      struct objfile *objfile
	= block == nullptr ? nullptr : block->objfile ();

      symbol_lookup_debug_printf_v
	("lookup_symbol_in_block (%s, %s (objfile %s), %s)",
	 name, host_address_to_string (block),
	 objfile != nullptr ? objfile_debug_name (objfile) : "NULL",
	 domain_name (domain));
    }

  sym = block_lookup_symbol (block, name, match_type, domain);
  if (sym)
    {
      symbol_lookup_debug_printf_v ("lookup_symbol_in_block (...) = %s",
				    host_address_to_string (sym));
      return sym;
    }

  symbol_lookup_debug_printf_v ("lookup_symbol_in_block (...) = NULL");
  return NULL;
}

/* See symtab.h.  */

struct block_symbol
lookup_global_symbol_from_objfile (struct objfile *main_objfile,
				   enum block_enum block_index,
				   const char *name,
				   const domain_enum domain)
{
  gdb_assert (block_index == GLOBAL_BLOCK || block_index == STATIC_BLOCK);

  for (objfile *objfile : main_objfile->separate_debug_objfiles ())
    {
      struct block_symbol result
	= lookup_symbol_in_objfile (objfile, block_index, name, domain);

      if (result.symbol != nullptr)
	return result;
    }

  return {};
}

/* Check to see if the symbol is defined in one of the OBJFILE's
   symtabs.  BLOCK_INDEX should be either GLOBAL_BLOCK or STATIC_BLOCK,
   depending on whether or not we want to search global symbols or
   static symbols.  */

static struct block_symbol
lookup_symbol_in_objfile_symtabs (struct objfile *objfile,
				  enum block_enum block_index, const char *name,
				  const domain_enum domain)
{
  gdb_assert (block_index == GLOBAL_BLOCK || block_index == STATIC_BLOCK);

  symbol_lookup_debug_printf_v
    ("lookup_symbol_in_objfile_symtabs (%s, %s, %s, %s)",
     objfile_debug_name (objfile),
     block_index == GLOBAL_BLOCK ? "GLOBAL_BLOCK" : "STATIC_BLOCK",
     name, domain_name (domain));

  struct block_symbol other;
  other.symbol = NULL;
  for (compunit_symtab *cust : objfile->compunits ())
    {
      const struct blockvector *bv;
      const struct block *block;
      struct block_symbol result;

      bv = cust->blockvector ();
      block = bv->block (block_index);
      result.symbol = block_lookup_symbol_primary (block, name, domain);
      result.block = block;
      if (result.symbol == NULL)
	continue;
      if (best_symbol (result.symbol, domain))
	{
	  other = result;
	  break;
	}
      if (result.symbol->matches (domain))
	{
	  struct symbol *better
	    = better_symbol (other.symbol, result.symbol, domain);
	  if (better != other.symbol)
	    {
	      other.symbol = better;
	      other.block = block;
	    }
	}
    }

  if (other.symbol != NULL)
    {
      symbol_lookup_debug_printf_v
	("lookup_symbol_in_objfile_symtabs (...) = %s (block %s)",
	 host_address_to_string (other.symbol),
	 host_address_to_string (other.block));
      return other;
    }

  symbol_lookup_debug_printf_v
    ("lookup_symbol_in_objfile_symtabs (...) = NULL");
  return {};
}

/* Wrapper around lookup_symbol_in_objfile_symtabs for search_symbols.
   Look up LINKAGE_NAME in DOMAIN in the global and static blocks of OBJFILE
   and all associated separate debug objfiles.

   Normally we only look in OBJFILE, and not any separate debug objfiles
   because the outer loop will cause them to be searched too.  This case is
   different.  Here we're called from search_symbols where it will only
   call us for the objfile that contains a matching minsym.  */

static struct block_symbol
lookup_symbol_in_objfile_from_linkage_name (struct objfile *objfile,
					    const char *linkage_name,
					    domain_enum domain)
{
  enum language lang = current_language->la_language;
  struct objfile *main_objfile;

  demangle_result_storage storage;
  const char *modified_name = demangle_for_lookup (linkage_name, lang, storage);

  if (objfile->separate_debug_objfile_backlink)
    main_objfile = objfile->separate_debug_objfile_backlink;
  else
    main_objfile = objfile;

  for (::objfile *cur_objfile : main_objfile->separate_debug_objfiles ())
    {
      struct block_symbol result;

      result = lookup_symbol_in_objfile_symtabs (cur_objfile, GLOBAL_BLOCK,
						 modified_name, domain);
      if (result.symbol == NULL)
	result = lookup_symbol_in_objfile_symtabs (cur_objfile, STATIC_BLOCK,
						   modified_name, domain);
      if (result.symbol != NULL)
	return result;
    }

  return {};
}

/* A helper function that throws an exception when a symbol was found
   in a psymtab but not in a symtab.  */

static void ATTRIBUTE_NORETURN
error_in_psymtab_expansion (enum block_enum block_index, const char *name,
			    struct compunit_symtab *cust)
{
  error (_("\
Internal: %s symbol `%s' found in %s psymtab but not in symtab.\n\
%s may be an inlined function, or may be a template function\n	 \
(if a template, try specifying an instantiation: %s<type>)."),
	 block_index == GLOBAL_BLOCK ? "global" : "static",
	 name,
	 symtab_to_filename_for_display (cust->primary_filetab ()),
	 name, name);
}

/* A helper function for various lookup routines that interfaces with
   the "quick" symbol table functions.  */

static struct block_symbol
lookup_symbol_via_quick_fns (struct objfile *objfile,
			     enum block_enum block_index, const char *name,
			     const domain_enum domain)
{
  struct compunit_symtab *cust;
  const struct blockvector *bv;
  const struct block *block;
  struct block_symbol result;

  symbol_lookup_debug_printf_v
    ("lookup_symbol_via_quick_fns (%s, %s, %s, %s)",
     objfile_debug_name (objfile),
     block_index == GLOBAL_BLOCK ? "GLOBAL_BLOCK" : "STATIC_BLOCK",
     name, domain_name (domain));

  cust = objfile->lookup_symbol (block_index, name, domain);
  if (cust == NULL)
    {
      symbol_lookup_debug_printf_v
	("lookup_symbol_via_quick_fns (...) = NULL");
      return {};
    }

  bv = cust->blockvector ();
  block = bv->block (block_index);
  result.symbol = block_lookup_symbol (block, name,
				       symbol_name_match_type::FULL, domain);
  if (result.symbol == NULL)
    error_in_psymtab_expansion (block_index, name, cust);

  symbol_lookup_debug_printf_v
    ("lookup_symbol_via_quick_fns (...) = %s (block %s)",
     host_address_to_string (result.symbol),
     host_address_to_string (block));

  result.block = block;
  return result;
}

/* See language.h.  */

struct block_symbol
language_defn::lookup_symbol_nonlocal (const char *name,
				       const struct block *block,
				       const domain_enum domain) const
{
  struct block_symbol result;

  /* NOTE: dje/2014-10-26: The lookup in all objfiles search could skip
     the current objfile.  Searching the current objfile first is useful
     for both matching user expectations as well as performance.  */

  result = lookup_symbol_in_static_block (name, block, domain);
  if (result.symbol != NULL)
    return result;

  /* If we didn't find a definition for a builtin type in the static block,
     search for it now.  This is actually the right thing to do and can be
     a massive performance win.  E.g., when debugging a program with lots of
     shared libraries we could search all of them only to find out the
     builtin type isn't defined in any of them.  This is common for types
     like "void".  */
  if (domain == VAR_DOMAIN)
    {
      struct gdbarch *gdbarch;

      if (block == NULL)
	gdbarch = current_inferior ()->arch ();
      else
	gdbarch = block->gdbarch ();
      result.symbol = language_lookup_primitive_type_as_symbol (this,
								gdbarch, name);
      result.block = NULL;
      if (result.symbol != NULL)
	return result;
    }

  return lookup_global_symbol (name, block, domain);
}

/* See symtab.h.  */

struct block_symbol
lookup_symbol_in_static_block (const char *name,
			       const struct block *block,
			       const domain_enum domain)
{
  if (block == nullptr)
    return {};

  const struct block *static_block = block->static_block ();
  struct symbol *sym;

  if (static_block == NULL)
    return {};

  if (symbol_lookup_debug)
    {
      struct objfile *objfile = (block == nullptr
				 ? nullptr : block->objfile ());

      symbol_lookup_debug_printf
	("lookup_symbol_in_static_block (%s, %s (objfile %s), %s)",
	 name, host_address_to_string (block),
	 objfile != nullptr ? objfile_debug_name (objfile) : "NULL",
	 domain_name (domain));
    }

  sym = lookup_symbol_in_block (name,
				symbol_name_match_type::FULL,
				static_block, domain);
  symbol_lookup_debug_printf ("lookup_symbol_in_static_block (...) = %s",
			      sym != NULL
			      ? host_address_to_string (sym) : "NULL");
  return (struct block_symbol) {sym, static_block};
}

/* Perform the standard symbol lookup of NAME in OBJFILE:
   1) First search expanded symtabs, and if not found
   2) Search the "quick" symtabs (partial or .gdb_index).
   BLOCK_INDEX is one of GLOBAL_BLOCK or STATIC_BLOCK.  */

static struct block_symbol
lookup_symbol_in_objfile (struct objfile *objfile, enum block_enum block_index,
			  const char *name, const domain_enum domain)
{
  struct block_symbol result;

  gdb_assert (block_index == GLOBAL_BLOCK || block_index == STATIC_BLOCK);

  symbol_lookup_debug_printf ("lookup_symbol_in_objfile (%s, %s, %s, %s)",
			      objfile_debug_name (objfile),
			      block_index == GLOBAL_BLOCK
			      ? "GLOBAL_BLOCK" : "STATIC_BLOCK",
			      name, domain_name (domain));

  result = lookup_symbol_in_objfile_symtabs (objfile, block_index,
					     name, domain);
  if (result.symbol != NULL)
    {
      symbol_lookup_debug_printf
	("lookup_symbol_in_objfile (...) = %s (in symtabs)",
	 host_address_to_string (result.symbol));
      return result;
    }

  result = lookup_symbol_via_quick_fns (objfile, block_index,
					name, domain);
  symbol_lookup_debug_printf ("lookup_symbol_in_objfile (...) = %s%s",
			      result.symbol != NULL
			      ? host_address_to_string (result.symbol)
			      : "NULL",
			      result.symbol != NULL ? " (via quick fns)"
			      : "");
  return result;
}

/* This function contains the common code of lookup_{global,static}_symbol.
   OBJFILE is only used if BLOCK_INDEX is GLOBAL_SCOPE, in which case it is
   the objfile to start the lookup in.  */

static struct block_symbol
lookup_global_or_static_symbol (const char *name,
				enum block_enum block_index,
				struct objfile *objfile,
				const domain_enum domain)
{
  struct symbol_cache *cache = get_symbol_cache (current_program_space);
  struct block_symbol result;
  struct block_symbol_cache *bsc;
  struct symbol_cache_slot *slot;

  gdb_assert (block_index == GLOBAL_BLOCK || block_index == STATIC_BLOCK);
  gdb_assert (objfile == nullptr || block_index == GLOBAL_BLOCK);

  /* First see if we can find the symbol in the cache.
     This works because we use the current objfile to qualify the lookup.  */
  result = symbol_cache_lookup (cache, objfile, block_index, name, domain,
				&bsc, &slot);
  if (result.symbol != NULL)
    {
      if (SYMBOL_LOOKUP_FAILED_P (result))
	return {};
      return result;
    }

  /* Do a global search (of global blocks, heh).  */
  if (result.symbol == NULL)
    gdbarch_iterate_over_objfiles_in_search_order
      (objfile != NULL ? objfile->arch () : current_inferior ()->arch (),
       [&result, block_index, name, domain] (struct objfile *objfile_iter)
	 {
	   result = lookup_symbol_in_objfile (objfile_iter, block_index,
					      name, domain);
	   return result.symbol != nullptr;
	 },
       objfile);

  if (result.symbol != NULL)
    symbol_cache_mark_found (bsc, slot, objfile, result.symbol, result.block);
  else
    symbol_cache_mark_not_found (bsc, slot, objfile, name, domain);

  return result;
}

/* See symtab.h.  */

struct block_symbol
lookup_static_symbol (const char *name, const domain_enum domain)
{
  return lookup_global_or_static_symbol (name, STATIC_BLOCK, nullptr, domain);
}

/* See symtab.h.  */

struct block_symbol
lookup_global_symbol (const char *name,
		      const struct block *block,
		      const domain_enum domain)
{
  /* If a block was passed in, we want to search the corresponding
     global block first.  This yields "more expected" behavior, and is
     needed to support 'FILENAME'::VARIABLE lookups.  */
  const struct block *global_block
    = block == nullptr ? nullptr : block->global_block ();
  symbol *sym = NULL;
  if (global_block != nullptr)
    {
      sym = lookup_symbol_in_block (name,
				    symbol_name_match_type::FULL,
				    global_block, domain);
      if (sym != NULL && best_symbol (sym, domain))
	return { sym, global_block };
    }

  struct objfile *objfile = nullptr;
  if (block != nullptr)
    {
      objfile = block->objfile ();
      if (objfile->separate_debug_objfile_backlink != nullptr)
	objfile = objfile->separate_debug_objfile_backlink;
    }

  block_symbol bs
    = lookup_global_or_static_symbol (name, GLOBAL_BLOCK, objfile, domain);
  if (better_symbol (sym, bs.symbol, domain) == sym)
    return { sym, global_block };
  else
    return bs;
}

bool
symbol_matches_domain (enum language symbol_language,
		       domain_enum symbol_domain,
		       domain_enum domain)
{
  /* For C++ "struct foo { ... }" also defines a typedef for "foo".
     Similarly, any Ada type declaration implicitly defines a typedef.  */
  if (symbol_language == language_cplus
      || symbol_language == language_d
      || symbol_language == language_ada
      || symbol_language == language_rust)
    {
      if ((domain == VAR_DOMAIN || domain == STRUCT_DOMAIN)
	  && symbol_domain == STRUCT_DOMAIN)
	return true;
    }
  /* For all other languages, strict match is required.  */
  return (symbol_domain == domain);
}

/* See symtab.h.  */

struct type *
lookup_transparent_type (const char *name)
{
  return current_language->lookup_transparent_type (name);
}

/* A helper for basic_lookup_transparent_type that interfaces with the
   "quick" symbol table functions.  */

static struct type *
basic_lookup_transparent_type_quick (struct objfile *objfile,
				     enum block_enum block_index,
				     const char *name)
{
  struct compunit_symtab *cust;
  const struct blockvector *bv;
  const struct block *block;
  struct symbol *sym;

  cust = objfile->lookup_symbol (block_index, name, STRUCT_DOMAIN);
  if (cust == NULL)
    return NULL;

  bv = cust->blockvector ();
  block = bv->block (block_index);

  lookup_name_info lookup_name (name, symbol_name_match_type::FULL);
  sym = block_find_symbol (block, lookup_name, STRUCT_DOMAIN, nullptr);
  if (sym == nullptr)
    error_in_psymtab_expansion (block_index, name, cust);
  gdb_assert (!TYPE_IS_OPAQUE (sym->type ()));
  return sym->type ();
}

/* Subroutine of basic_lookup_transparent_type to simplify it.
   Look up the non-opaque definition of NAME in BLOCK_INDEX of OBJFILE.
   BLOCK_INDEX is either GLOBAL_BLOCK or STATIC_BLOCK.  */

static struct type *
basic_lookup_transparent_type_1 (struct objfile *objfile,
				 enum block_enum block_index,
				 const char *name)
{
  const struct blockvector *bv;
  const struct block *block;
  const struct symbol *sym;

  lookup_name_info lookup_name (name, symbol_name_match_type::FULL);
  for (compunit_symtab *cust : objfile->compunits ())
    {
      bv = cust->blockvector ();
      block = bv->block (block_index);
      sym = block_find_symbol (block, lookup_name, STRUCT_DOMAIN, nullptr);
      if (sym != nullptr)
	{
	  gdb_assert (!TYPE_IS_OPAQUE (sym->type ()));
	  return sym->type ();
	}
    }

  return NULL;
}

/* The standard implementation of lookup_transparent_type.  This code
   was modeled on lookup_symbol -- the parts not relevant to looking
   up types were just left out.  In particular it's assumed here that
   types are available in STRUCT_DOMAIN and only in file-static or
   global blocks.  */

struct type *
basic_lookup_transparent_type (const char *name)
{
  struct type *t;

  /* Now search all the global symbols.  Do the symtab's first, then
     check the psymtab's.  If a psymtab indicates the existence
     of the desired name as a global, then do psymtab-to-symtab
     conversion on the fly and return the found symbol.  */

  for (objfile *objfile : current_program_space->objfiles ())
    {
      t = basic_lookup_transparent_type_1 (objfile, GLOBAL_BLOCK, name);
      if (t)
	return t;
    }

  for (objfile *objfile : current_program_space->objfiles ())
    {
      t = basic_lookup_transparent_type_quick (objfile, GLOBAL_BLOCK, name);
      if (t)
	return t;
    }

  /* Now search the static file-level symbols.
     Not strictly correct, but more useful than an error.
     Do the symtab's first, then
     check the psymtab's.  If a psymtab indicates the existence
     of the desired name as a file-level static, then do psymtab-to-symtab
     conversion on the fly and return the found symbol.  */

  for (objfile *objfile : current_program_space->objfiles ())
    {
      t = basic_lookup_transparent_type_1 (objfile, STATIC_BLOCK, name);
      if (t)
	return t;
    }

  for (objfile *objfile : current_program_space->objfiles ())
    {
      t = basic_lookup_transparent_type_quick (objfile, STATIC_BLOCK, name);
      if (t)
	return t;
    }

  return (struct type *) 0;
}

/* See symtab.h.  */

bool
iterate_over_symbols (const struct block *block,
		      const lookup_name_info &name,
		      const domain_enum domain,
		      gdb::function_view<symbol_found_callback_ftype> callback)
{
  for (struct symbol *sym : block_iterator_range (block, &name))
    {
      if (sym->matches (domain))
	{
	  struct block_symbol block_sym = {sym, block};

	  if (!callback (&block_sym))
	    return false;
	}
    }
  return true;
}

/* See symtab.h.  */

bool
iterate_over_symbols_terminated
  (const struct block *block,
   const lookup_name_info &name,
   const domain_enum domain,
   gdb::function_view<symbol_found_callback_ftype> callback)
{
  if (!iterate_over_symbols (block, name, domain, callback))
    return false;
  struct block_symbol block_sym = {nullptr, block};
  return callback (&block_sym);
}

/* Find the compunit symtab associated with PC and SECTION.
   This will read in debug info as necessary.  */

struct compunit_symtab *
find_pc_sect_compunit_symtab (CORE_ADDR pc, struct obj_section *section)
{
  struct compunit_symtab *best_cust = NULL;
  CORE_ADDR best_cust_range = 0;
  struct bound_minimal_symbol msymbol;

  /* If we know that this is not a text address, return failure.  This is
     necessary because we loop based on the block's high and low code
     addresses, which do not include the data ranges, and because
     we call find_pc_sect_psymtab which has a similar restriction based
     on the partial_symtab's texthigh and textlow.  */
  msymbol = lookup_minimal_symbol_by_pc_section (pc, section);
  if (msymbol.minsym && msymbol.minsym->data_p ())
    return NULL;

  /* Search all symtabs for the one whose file contains our address, and which
     is the smallest of all the ones containing the address.  This is designed
     to deal with a case like symtab a is at 0x1000-0x2000 and 0x3000-0x4000
     and symtab b is at 0x2000-0x3000.  So the GLOBAL_BLOCK for a is from
     0x1000-0x4000, but for address 0x2345 we want to return symtab b.

     This happens for native ecoff format, where code from included files
     gets its own symtab.  The symtab for the included file should have
     been read in already via the dependency mechanism.
     It might be swifter to create several symtabs with the same name
     like xcoff does (I'm not sure).

     It also happens for objfiles that have their functions reordered.
     For these, the symtab we are looking for is not necessarily read in.  */

  for (objfile *obj_file : current_program_space->objfiles ())
    {
      for (compunit_symtab *cust : obj_file->compunits ())
	{
	  const struct blockvector *bv = cust->blockvector ();
	  const struct block *global_block = bv->global_block ();
	  CORE_ADDR start = global_block->start ();
	  CORE_ADDR end = global_block->end ();
	  bool in_range_p = start <= pc && pc < end;
	  if (!in_range_p)
	    continue;

	  if (bv->map () != nullptr)
	    {
	      if (bv->map ()->find (pc) == nullptr)
		continue;

	      return cust;
	    }

	  CORE_ADDR range = end - start;
	  if (best_cust != nullptr
	      && range >= best_cust_range)
	    /* Cust doesn't have a smaller range than best_cust, skip it.  */
	    continue;
	
	  /* For an objfile that has its functions reordered,
	     find_pc_psymtab will find the proper partial symbol table
	     and we simply return its corresponding symtab.  */
	  /* In order to better support objfiles that contain both
	     stabs and coff debugging info, we continue on if a psymtab
	     can't be found.  */
	  struct compunit_symtab *result
	    = obj_file->find_pc_sect_compunit_symtab (msymbol, pc,
						      section, 0);
	  if (result != nullptr)
	    return result;

	  if (section != 0)
	    {
	      struct symbol *found_sym = nullptr;

	      for (int b_index = GLOBAL_BLOCK;
		   b_index <= STATIC_BLOCK && found_sym == nullptr;
		   ++b_index)
		{
		  const struct block *b = bv->block (b_index);
		  for (struct symbol *sym : block_iterator_range (b))
		    {
		      if (matching_obj_sections (sym->obj_section (obj_file),
						 section))
			{
			  found_sym = sym;
			  break;
			}
		    }
		}
	      if (found_sym == nullptr)
		continue;		/* No symbol in this symtab matches
					   section.  */
	    }

	  /* Cust is best found sofar, save it.  */
	  best_cust = cust;
	  best_cust_range = range;
	}
    }

  if (best_cust != NULL)
    return best_cust;

  /* Not found in symtabs, search the "quick" symtabs (e.g. psymtabs).  */

  for (objfile *objf : current_program_space->objfiles ())
    {
      struct compunit_symtab *result
	= objf->find_pc_sect_compunit_symtab (msymbol, pc, section, 1);
      if (result != NULL)
	return result;
    }

  return NULL;
}

/* Find the compunit symtab associated with PC.
   This will read in debug info as necessary.
   Backward compatibility, no section.  */

struct compunit_symtab *
find_pc_compunit_symtab (CORE_ADDR pc)
{
  return find_pc_sect_compunit_symtab (pc, find_pc_mapped_section (pc));
}

/* See symtab.h.  */

struct symbol *
find_symbol_at_address (CORE_ADDR address)
{
  /* A helper function to search a given symtab for a symbol matching
     ADDR.  */
  auto search_symtab = [] (compunit_symtab *symtab, CORE_ADDR addr) -> symbol *
    {
      const struct blockvector *bv = symtab->blockvector ();

      for (int i = GLOBAL_BLOCK; i <= STATIC_BLOCK; ++i)
	{
	  const struct block *b = bv->block (i);

	  for (struct symbol *sym : block_iterator_range (b))
	    {
	      if (sym->aclass () == LOC_STATIC
		  && sym->value_address () == addr)
		return sym;
	    }
	}
      return nullptr;
    };

  for (objfile *objfile : current_program_space->objfiles ())
    {
      /* If this objfile was read with -readnow, then we need to
	 search the symtabs directly.  */
      if ((objfile->flags & OBJF_READNOW) != 0)
	{
	  for (compunit_symtab *symtab : objfile->compunits ())
	    {
	      struct symbol *sym = search_symtab (symtab, address);
	      if (sym != nullptr)
		return sym;
	    }
	}
      else
	{
	  struct compunit_symtab *symtab
	    = objfile->find_compunit_symtab_by_address (address);
	  if (symtab != NULL)
	    {
	      struct symbol *sym = search_symtab (symtab, address);
	      if (sym != nullptr)
		return sym;
	    }
	}
    }

  return NULL;
}



/* Find the source file and line number for a given PC value and SECTION.
   Return a structure containing a symtab pointer, a line number,
   and a pc range for the entire source line.
   The value's .pc field is NOT the specified pc.
   NOTCURRENT nonzero means, if specified pc is on a line boundary,
   use the line that ends there.  Otherwise, in that case, the line
   that begins there is used.  */

/* The big complication here is that a line may start in one file, and end just
   before the start of another file.  This usually occurs when you #include
   code in the middle of a subroutine.  To properly find the end of a line's PC
   range, we must search all symtabs associated with this compilation unit, and
   find the one whose first PC is closer than that of the next line in this
   symtab.  */

struct symtab_and_line
find_pc_sect_line (CORE_ADDR pc, struct obj_section *section, int notcurrent)
{
  struct compunit_symtab *cust;
  const linetable *l;
  int len;
  const linetable_entry *item;
  const struct blockvector *bv;
  struct bound_minimal_symbol msymbol;

  /* Info on best line seen so far, and where it starts, and its file.  */

  const linetable_entry *best = NULL;
  CORE_ADDR best_end = 0;
  struct symtab *best_symtab = 0;

  /* Store here the first line number
     of a file which contains the line at the smallest pc after PC.
     If we don't find a line whose range contains PC,
     we will use a line one less than this,
     with a range from the start of that file to the first line's pc.  */
  const linetable_entry *alt = NULL;

  /* Info on best line seen in this file.  */

  const linetable_entry *prev;

  /* If this pc is not from the current frame,
     it is the address of the end of a call instruction.
     Quite likely that is the start of the following statement.
     But what we want is the statement containing the instruction.
     Fudge the pc to make sure we get that.  */

  /* It's tempting to assume that, if we can't find debugging info for
     any function enclosing PC, that we shouldn't search for line
     number info, either.  However, GAS can emit line number info for
     assembly files --- very helpful when debugging hand-written
     assembly code.  In such a case, we'd have no debug info for the
     function, but we would have line info.  */

  if (notcurrent)
    pc -= 1;

  /* elz: added this because this function returned the wrong
     information if the pc belongs to a stub (import/export)
     to call a shlib function.  This stub would be anywhere between
     two functions in the target, and the line info was erroneously
     taken to be the one of the line before the pc.  */

  /* RT: Further explanation:

   * We have stubs (trampolines) inserted between procedures.
   *
   * Example: "shr1" exists in a shared library, and a "shr1" stub also
   * exists in the main image.
   *
   * In the minimal symbol table, we have a bunch of symbols
   * sorted by start address.  The stubs are marked as "trampoline",
   * the others appear as text. E.g.:
   *
   *  Minimal symbol table for main image
   *     main:  code for main (text symbol)
   *     shr1: stub  (trampoline symbol)
   *     foo:   code for foo (text symbol)
   *     ...
   *  Minimal symbol table for "shr1" image:
   *     ...
   *     shr1: code for shr1 (text symbol)
   *     ...
   *
   * So the code below is trying to detect if we are in the stub
   * ("shr1" stub), and if so, find the real code ("shr1" trampoline),
   * and if found,  do the symbolization from the real-code address
   * rather than the stub address.
   *
   * Assumptions being made about the minimal symbol table:
   *   1. lookup_minimal_symbol_by_pc() will return a trampoline only
   *      if we're really in the trampoline.s If we're beyond it (say
   *      we're in "foo" in the above example), it'll have a closer
   *      symbol (the "foo" text symbol for example) and will not
   *      return the trampoline.
   *   2. lookup_minimal_symbol_text() will find a real text symbol
   *      corresponding to the trampoline, and whose address will
   *      be different than the trampoline address.  I put in a sanity
   *      check for the address being the same, to avoid an
   *      infinite recursion.
   */
  msymbol = lookup_minimal_symbol_by_pc (pc);
  if (msymbol.minsym != NULL)
    if (msymbol.minsym->type () == mst_solib_trampoline)
      {
	struct bound_minimal_symbol mfunsym
	  = lookup_minimal_symbol_text (msymbol.minsym->linkage_name (),
					NULL);

	if (mfunsym.minsym == NULL)
	  /* I eliminated this warning since it is coming out
	   * in the following situation:
	   * gdb shmain // test program with shared libraries
	   * (gdb) break shr1  // function in shared lib
	   * Warning: In stub for ...
	   * In the above situation, the shared lib is not loaded yet,
	   * so of course we can't find the real func/line info,
	   * but the "break" still works, and the warning is annoying.
	   * So I commented out the warning.  RT */
	  /* warning ("In stub for %s; unable to find real function/line info",
	     msymbol->linkage_name ()); */
	  ;
	/* fall through */
	else if (mfunsym.value_address ()
		 == msymbol.value_address ())
	  /* Avoid infinite recursion */
	  /* See above comment about why warning is commented out.  */
	  /* warning ("In stub for %s; unable to find real function/line info",
	     msymbol->linkage_name ()); */
	  ;
	/* fall through */
	else
	  {
	    /* Detect an obvious case of infinite recursion.  If this
	       should occur, we'd like to know about it, so error out,
	       fatally.  */
	    if (mfunsym.value_address () == pc)
	      internal_error (_("Infinite recursion detected in find_pc_sect_line;"
		  "please file a bug report"));

	    return find_pc_line (mfunsym.value_address (), 0);
	  }
      }

  symtab_and_line val;
  val.pspace = current_program_space;

  cust = find_pc_sect_compunit_symtab (pc, section);
  if (cust == NULL)
    {
      /* If no symbol information, return previous pc.  */
      if (notcurrent)
	pc++;
      val.pc = pc;
      return val;
    }

  bv = cust->blockvector ();
  struct objfile *objfile = cust->objfile ();

  /* Look at all the symtabs that share this blockvector.
     They all have the same apriori range, that we found was right;
     but they have different line tables.  */

  for (symtab *iter_s : cust->filetabs ())
    {
      /* Find the best line in this symtab.  */
      l = iter_s->linetable ();
      if (!l)
	continue;
      len = l->nitems;
      if (len <= 0)
	{
	  /* I think len can be zero if the symtab lacks line numbers
	     (e.g. gcc -g1).  (Either that or the LINETABLE is NULL;
	     I'm not sure which, and maybe it depends on the symbol
	     reader).  */
	  continue;
	}

      prev = NULL;
      item = l->item;		/* Get first line info.  */

      /* Is this file's first line closer than the first lines of other files?
	 If so, record this file, and its first line, as best alternate.  */
      if (item->pc (objfile) > pc
	  && (!alt || item->unrelocated_pc () < alt->unrelocated_pc ()))
	alt = item;

      auto pc_compare = [] (const unrelocated_addr &comp_pc,
			    const struct linetable_entry & lhs)
      {
	return comp_pc < lhs.unrelocated_pc ();
      };

      const linetable_entry *first = item;
      const linetable_entry *last = item + len;
      item = (std::upper_bound
	      (first, last,
	       unrelocated_addr (pc - objfile->text_section_offset ()),
	       pc_compare));
      if (item != first)
	{
	  prev = item - 1;		/* Found a matching item.  */
	  /* At this point, prev is a line whose address is <= pc.  However, we
	     don't know if ITEM is pointing to the same statement or not.  */
	  while (item != last && prev->line == item->line && !item->is_stmt)
	    item++;
	}

      /* At this point, prev points at the line whose start addr is <= pc, and
	 item points at the next statement.  If we ran off the end of the linetable
	 (pc >= start of the last line), then prev == item.  If pc < start of
	 the first line, prev will not be set.  */

      /* Is this file's best line closer than the best in the other files?
	 If so, record this file, and its best line, as best so far.  Don't
	 save prev if it represents the end of a function (i.e. line number
	 0) instead of a real line.  */

      if (prev && prev->line
	  && (!best || prev->unrelocated_pc () > best->unrelocated_pc ()))
	{
	  best = prev;
	  best_symtab = iter_s;

	  /* If during the binary search we land on a non-statement entry,
	     scan backward through entries at the same address to see if
	     there is an entry marked as is-statement.  In theory this
	     duplication should have been removed from the line table
	     during construction, this is just a double check.  If the line
	     table has had the duplication removed then this should be
	     pretty cheap.  */
	  if (!best->is_stmt)
	    {
	      const linetable_entry *tmp = best;
	      while (tmp > first
		     && (tmp - 1)->unrelocated_pc () == tmp->unrelocated_pc ()
		     && (tmp - 1)->line != 0 && !tmp->is_stmt)
		--tmp;
	      if (tmp->is_stmt)
		best = tmp;
	    }

	  /* Discard BEST_END if it's before the PC of the current BEST.  */
	  if (best_end <= best->pc (objfile))
	    best_end = 0;
	}

      /* If another line (denoted by ITEM) is in the linetable and its
	 PC is after BEST's PC, but before the current BEST_END, then
	 use ITEM's PC as the new best_end.  */
      if (best && item < last
	  && item->unrelocated_pc () > best->unrelocated_pc ()
	  && (best_end == 0 || best_end > item->pc (objfile)))
	best_end = item->pc (objfile);
    }

  if (!best_symtab)
    {
      /* If we didn't find any line number info, just return zeros.
	 We used to return alt->line - 1 here, but that could be
	 anywhere; if we don't have line number info for this PC,
	 don't make some up.  */
      val.pc = pc;
    }
  else if (best->line == 0)
    {
      /* If our best fit is in a range of PC's for which no line
	 number info is available (line number is zero) then we didn't
	 find any valid line information.  */
      val.pc = pc;
    }
  else
    {
      val.is_stmt = best->is_stmt;
      val.symtab = best_symtab;
      val.line = best->line;
      val.pc = best->pc (objfile);
      if (best_end && (!alt || best_end < alt->pc (objfile)))
	val.end = best_end;
      else if (alt)
	val.end = alt->pc (objfile);
      else
	val.end = bv->global_block ()->end ();
    }
  val.section = section;
  return val;
}

/* Backward compatibility (no section).  */

struct symtab_and_line
find_pc_line (CORE_ADDR pc, int notcurrent)
{
  struct obj_section *section;

  section = find_pc_overlay (pc);
  if (!pc_in_unmapped_range (pc, section))
    return find_pc_sect_line (pc, section, notcurrent);

  /* If the original PC was an unmapped address then we translate this to a
     mapped address in order to lookup the sal.  However, as the user
     passed us an unmapped address it makes more sense to return a result
     that has the pc and end fields translated to unmapped addresses.  */
  pc = overlay_mapped_address (pc, section);
  symtab_and_line sal = find_pc_sect_line (pc, section, notcurrent);
  sal.pc = overlay_unmapped_address (sal.pc, section);
  sal.end = overlay_unmapped_address (sal.end, section);
  return sal;
}

/* Compare two symtab_and_line entries.  Return true if both have
   the same line number and the same symtab pointer.  That means we
   are dealing with two entries from the same line and from the same
   source file.

   Return false otherwise.  */

static bool
sal_line_symtab_matches_p (const symtab_and_line &sal1,
			   const symtab_and_line &sal2)
{
  return sal1.line == sal2.line && sal1.symtab == sal2.symtab;
}

/* See symtah.h.  */

std::optional<CORE_ADDR>
find_line_range_start (CORE_ADDR pc)
{
  struct symtab_and_line current_sal = find_pc_line (pc, 0);

  if (current_sal.line == 0)
    return {};

  struct symtab_and_line prev_sal = find_pc_line (current_sal.pc - 1, 0);

  /* If the previous entry is for a different line, that means we are already
     at the entry with the start PC for this line.  */
  if (!sal_line_symtab_matches_p (prev_sal, current_sal))
    return current_sal.pc;

  /* Otherwise, keep looking for entries for the same line but with
     smaller PC's.  */
  bool done = false;
  CORE_ADDR prev_pc;
  while (!done)
    {
      prev_pc = prev_sal.pc;

      prev_sal = find_pc_line (prev_pc - 1, 0);

      /* Did we notice a line change?  If so, we are done searching.  */
      if (!sal_line_symtab_matches_p (prev_sal, current_sal))
	done = true;
    }

  return prev_pc;
}

/* See symtab.h.  */

struct symtab *
find_pc_line_symtab (CORE_ADDR pc)
{
  struct symtab_and_line sal;

  /* This always passes zero for NOTCURRENT to find_pc_line.
     There are currently no callers that ever pass non-zero.  */
  sal = find_pc_line (pc, 0);
  return sal.symtab;
}

/* Find line number LINE in any symtab whose name is the same as
   SYMTAB.

   If found, return the symtab that contains the linetable in which it was
   found, set *INDEX to the index in the linetable of the best entry
   found, and set *EXACT_MATCH to true if the value returned is an
   exact match.

   If not found, return NULL.  */

struct symtab *
find_line_symtab (struct symtab *sym_tab, int line,
		  int *index, bool *exact_match)
{
  int exact = 0;  /* Initialized here to avoid a compiler warning.  */

  /* BEST_INDEX and BEST_LINETABLE identify the smallest linenumber > LINE
     so far seen.  */

  int best_index;
  const struct linetable *best_linetable;
  struct symtab *best_symtab;

  /* First try looking it up in the given symtab.  */
  best_linetable = sym_tab->linetable ();
  best_symtab = sym_tab;
  best_index = find_line_common (best_linetable, line, &exact, 0);
  if (best_index < 0 || !exact)
    {
      /* Didn't find an exact match.  So we better keep looking for
	 another symtab with the same name.  In the case of xcoff,
	 multiple csects for one source file (produced by IBM's FORTRAN
	 compiler) produce multiple symtabs (this is unavoidable
	 assuming csects can be at arbitrary places in memory and that
	 the GLOBAL_BLOCK of a symtab has a begin and end address).  */

      /* BEST is the smallest linenumber > LINE so far seen,
	 or 0 if none has been seen so far.
	 BEST_INDEX and BEST_LINETABLE identify the item for it.  */
      int best;

      if (best_index >= 0)
	best = best_linetable->item[best_index].line;
      else
	best = 0;

      for (objfile *objfile : current_program_space->objfiles ())
	objfile->expand_symtabs_with_fullname (symtab_to_fullname (sym_tab));

      for (objfile *objfile : current_program_space->objfiles ())
	{
	  for (compunit_symtab *cu : objfile->compunits ())
	    {
	      for (symtab *s : cu->filetabs ())
		{
		  const struct linetable *l;
		  int ind;

		  if (FILENAME_CMP (sym_tab->filename, s->filename) != 0)
		    continue;
		  if (FILENAME_CMP (symtab_to_fullname (sym_tab),
				    symtab_to_fullname (s)) != 0)
		    continue;	
		  l = s->linetable ();
		  ind = find_line_common (l, line, &exact, 0);
		  if (ind >= 0)
		    {
		      if (exact)
			{
			  best_index = ind;
			  best_linetable = l;
			  best_symtab = s;
			  goto done;
			}
		      if (best == 0 || l->item[ind].line < best)
			{
			  best = l->item[ind].line;
			  best_index = ind;
			  best_linetable = l;
			  best_symtab = s;
			}
		    }
		}
	    }
	}
    }
done:
  if (best_index < 0)
    return NULL;

  if (index)
    *index = best_index;
  if (exact_match)
    *exact_match = (exact != 0);

  return best_symtab;
}

/* Given SYMTAB, returns all the PCs function in the symtab that
   exactly match LINE.  Returns an empty vector if there are no exact
   matches, but updates BEST_ITEM in this case.  */

std::vector<CORE_ADDR>
find_pcs_for_symtab_line (struct symtab *symtab, int line,
			  const linetable_entry **best_item)
{
  int start = 0;
  std::vector<CORE_ADDR> result;
  struct objfile *objfile = symtab->compunit ()->objfile ();

  /* First, collect all the PCs that are at this line.  */
  while (1)
    {
      int was_exact;
      int idx;

      idx = find_line_common (symtab->linetable (), line, &was_exact,
			      start);
      if (idx < 0)
	break;

      if (!was_exact)
	{
	  const linetable_entry *item = &symtab->linetable ()->item[idx];

	  if (*best_item == NULL
	      || (item->line < (*best_item)->line && item->is_stmt))
	    *best_item = item;

	  break;
	}

      result.push_back (symtab->linetable ()->item[idx].pc (objfile));
      start = idx + 1;
    }

  return result;
}


/* Set the PC value for a given source file and line number and return true.
   Returns false for invalid line number (and sets the PC to 0).
   The source file is specified with a struct symtab.  */

bool
find_line_pc (struct symtab *symtab, int line, CORE_ADDR *pc)
{
  const struct linetable *l;
  int ind;

  *pc = 0;
  if (symtab == 0)
    return false;

  symtab = find_line_symtab (symtab, line, &ind, NULL);
  if (symtab != NULL)
    {
      l = symtab->linetable ();
      *pc = l->item[ind].pc (symtab->compunit ()->objfile ());
      return true;
    }
  else
    return false;
}

/* Find the range of pc values in a line.
   Store the starting pc of the line into *STARTPTR
   and the ending pc (start of next line) into *ENDPTR.
   Returns true to indicate success.
   Returns false if could not find the specified line.  */

bool
find_line_pc_range (struct symtab_and_line sal, CORE_ADDR *startptr,
		    CORE_ADDR *endptr)
{
  CORE_ADDR startaddr;
  struct symtab_and_line found_sal;

  startaddr = sal.pc;
  if (startaddr == 0 && !find_line_pc (sal.symtab, sal.line, &startaddr))
    return false;

  /* This whole function is based on address.  For example, if line 10 has
     two parts, one from 0x100 to 0x200 and one from 0x300 to 0x400, then
     "info line *0x123" should say the line goes from 0x100 to 0x200
     and "info line *0x355" should say the line goes from 0x300 to 0x400.
     This also insures that we never give a range like "starts at 0x134
     and ends at 0x12c".  */

  found_sal = find_pc_sect_line (startaddr, sal.section, 0);
  if (found_sal.line != sal.line)
    {
      /* The specified line (sal) has zero bytes.  */
      *startptr = found_sal.pc;
      *endptr = found_sal.pc;
    }
  else
    {
      *startptr = found_sal.pc;
      *endptr = found_sal.end;
    }
  return true;
}

/* Given a line table and a line number, return the index into the line
   table for the pc of the nearest line whose number is >= the specified one.
   Return -1 if none is found.  The value is >= 0 if it is an index.
   START is the index at which to start searching the line table.

   Set *EXACT_MATCH nonzero if the value returned is an exact match.  */

static int
find_line_common (const linetable *l, int lineno,
		  int *exact_match, int start)
{
  int i;
  int len;

  /* BEST is the smallest linenumber > LINENO so far seen,
     or 0 if none has been seen so far.
     BEST_INDEX identifies the item for it.  */

  int best_index = -1;
  int best = 0;

  *exact_match = 0;

  if (lineno <= 0)
    return -1;
  if (l == 0)
    return -1;

  len = l->nitems;
  for (i = start; i < len; i++)
    {
      const linetable_entry *item = &(l->item[i]);

      /* Ignore non-statements.  */
      if (!item->is_stmt)
	continue;

      if (item->line == lineno)
	{
	  /* Return the first (lowest address) entry which matches.  */
	  *exact_match = 1;
	  return i;
	}

      if (item->line > lineno && (best == 0 || item->line < best))
	{
	  best = item->line;
	  best_index = i;
	}
    }

  /* If we got here, we didn't get an exact match.  */
  return best_index;
}

bool
find_pc_line_pc_range (CORE_ADDR pc, CORE_ADDR *startptr, CORE_ADDR *endptr)
{
  struct symtab_and_line sal;

  sal = find_pc_line (pc, 0);
  *startptr = sal.pc;
  *endptr = sal.end;
  return sal.symtab != 0;
}

/* Helper for find_function_start_sal.  Does most of the work, except
   setting the sal's symbol.  */

static symtab_and_line
find_function_start_sal_1 (CORE_ADDR func_addr, obj_section *section,
			   bool funfirstline)
{
  symtab_and_line sal = find_pc_sect_line (func_addr, section, 0);

  if (funfirstline && sal.symtab != NULL
      && (sal.symtab->compunit ()->locations_valid ()
	  || sal.symtab->language () == language_asm))
    {
      struct gdbarch *gdbarch = sal.symtab->compunit ()->objfile ()->arch ();

      sal.pc = func_addr;
      if (gdbarch_skip_entrypoint_p (gdbarch))
	sal.pc = gdbarch_skip_entrypoint (gdbarch, sal.pc);
      return sal;
    }

  /* We always should have a line for the function start address.
     If we don't, something is odd.  Create a plain SAL referring
     just the PC and hope that skip_prologue_sal (if requested)
     can find a line number for after the prologue.  */
  if (sal.pc < func_addr)
    {
      sal = {};
      sal.pspace = current_program_space;
      sal.pc = func_addr;
      sal.section = section;
    }

  if (funfirstline)
    skip_prologue_sal (&sal);

  return sal;
}

/* See symtab.h.  */

symtab_and_line
find_function_start_sal (CORE_ADDR func_addr, obj_section *section,
			 bool funfirstline)
{
  symtab_and_line sal
    = find_function_start_sal_1 (func_addr, section, funfirstline);

  /* find_function_start_sal_1 does a linetable search, so it finds
     the symtab and linenumber, but not a symbol.  Fill in the
     function symbol too.  */
  sal.symbol = find_pc_sect_containing_function (sal.pc, sal.section);

  return sal;
}

/* See symtab.h.  */

symtab_and_line
find_function_start_sal (symbol *sym, bool funfirstline)
{
  symtab_and_line sal
    = find_function_start_sal_1 (sym->value_block ()->entry_pc (),
				 sym->obj_section (sym->objfile ()),
				 funfirstline);
  sal.symbol = sym;
  return sal;
}


/* Given a function start address FUNC_ADDR and SYMTAB, find the first
   address for that function that has an entry in SYMTAB's line info
   table.  If such an entry cannot be found, return FUNC_ADDR
   unaltered.  */

static CORE_ADDR
skip_prologue_using_lineinfo (CORE_ADDR func_addr, struct symtab *symtab)
{
  CORE_ADDR func_start, func_end;
  const struct linetable *l;
  int i;

  /* Give up if this symbol has no lineinfo table.  */
  l = symtab->linetable ();
  if (l == NULL)
    return func_addr;

  /* Get the range for the function's PC values, or give up if we
     cannot, for some reason.  */
  if (!find_pc_partial_function (func_addr, NULL, &func_start, &func_end))
    return func_addr;

  struct objfile *objfile = symtab->compunit ()->objfile ();

  /* Linetable entries are ordered by PC values, see the commentary in
     symtab.h where `struct linetable' is defined.  Thus, the first
     entry whose PC is in the range [FUNC_START..FUNC_END[ is the
     address we are looking for.  */
  for (i = 0; i < l->nitems; i++)
    {
      const linetable_entry *item = &(l->item[i]);
      CORE_ADDR item_pc = item->pc (objfile);

      /* Don't use line numbers of zero, they mark special entries in
	 the table.  See the commentary on symtab.h before the
	 definition of struct linetable.  */
      if (item->line > 0 && func_start <= item_pc && item_pc < func_end)
	return item_pc;
    }

  return func_addr;
}

/* Try to locate the address where a breakpoint should be placed past the
   prologue of function starting at FUNC_ADDR using the line table.

   Return the address associated with the first entry in the line-table for
   the function starting at FUNC_ADDR which has prologue_end set to true if
   such entry exist, otherwise return an empty optional.  */

static std::optional<CORE_ADDR>
skip_prologue_using_linetable (CORE_ADDR func_addr)
{
  CORE_ADDR start_pc, end_pc;

  if (!find_pc_partial_function (func_addr, nullptr, &start_pc, &end_pc))
    return {};

  const struct symtab_and_line prologue_sal = find_pc_line (start_pc, 0);
  if (prologue_sal.symtab != nullptr
      && prologue_sal.symtab->language () != language_asm)
    {
      const linetable *linetable = prologue_sal.symtab->linetable ();

      struct objfile *objfile = prologue_sal.symtab->compunit ()->objfile ();

      unrelocated_addr unrel_start
	= unrelocated_addr (start_pc - objfile->text_section_offset ());
      unrelocated_addr unrel_end
	= unrelocated_addr (end_pc - objfile->text_section_offset ());

      auto it = std::lower_bound
	(linetable->item, linetable->item + linetable->nitems, unrel_start,
	 [] (const linetable_entry &lte, unrelocated_addr pc)
	 {
	   return lte.unrelocated_pc () < pc;
	 });

      for (;
	   (it < linetable->item + linetable->nitems
	    && it->unrelocated_pc () < unrel_end);
	   it++)
	if (it->prologue_end)
	  return {it->pc (objfile)};
    }

  return {};
}

/* Adjust SAL to the first instruction past the function prologue.
   If the PC was explicitly specified, the SAL is not changed.
   If the line number was explicitly specified then the SAL can still be
   updated, unless the language for SAL is assembler, in which case the SAL
   will be left unchanged.
   If SAL is already past the prologue, then do nothing.  */

void
skip_prologue_sal (struct symtab_and_line *sal)
{
  struct symbol *sym;
  struct symtab_and_line start_sal;
  CORE_ADDR pc, saved_pc;
  struct obj_section *section;
  const char *name;
  struct objfile *objfile;
  struct gdbarch *gdbarch;
  const struct block *b, *function_block;
  int force_skip, skip;

  /* Do not change the SAL if PC was specified explicitly.  */
  if (sal->explicit_pc)
    return;

  /* In assembly code, if the user asks for a specific line then we should
     not adjust the SAL.  The user already has instruction level
     visibility in this case, so selecting a line other than one requested
     is likely to be the wrong choice.  */
  if (sal->symtab != nullptr
      && sal->explicit_line
      && sal->symtab->language () == language_asm)
    return;

  scoped_restore_current_pspace_and_thread restore_pspace_thread;

  switch_to_program_space_and_thread (sal->pspace);

  sym = find_pc_sect_function (sal->pc, sal->section);
  if (sym != NULL)
    {
      objfile = sym->objfile ();
      pc = sym->value_block ()->entry_pc ();
      section = sym->obj_section (objfile);
      name = sym->linkage_name ();
    }
  else
    {
      struct bound_minimal_symbol msymbol
	= lookup_minimal_symbol_by_pc_section (sal->pc, sal->section);

      if (msymbol.minsym == NULL)
	return;

      objfile = msymbol.objfile;
      pc = msymbol.value_address ();
      section = msymbol.minsym->obj_section (objfile);
      name = msymbol.minsym->linkage_name ();
    }

  gdbarch = objfile->arch ();

  /* Process the prologue in two passes.  In the first pass try to skip the
     prologue (SKIP is true) and verify there is a real need for it (indicated
     by FORCE_SKIP).  If no such reason was found run a second pass where the
     prologue is not skipped (SKIP is false).  */

  skip = 1;
  force_skip = 1;

  /* Be conservative - allow direct PC (without skipping prologue) only if we
     have proven the CU (Compilation Unit) supports it.  sal->SYMTAB does not
     have to be set by the caller so we use SYM instead.  */
  if (sym != NULL
      && sym->symtab ()->compunit ()->locations_valid ())
    force_skip = 0;

  saved_pc = pc;
  do
    {
      pc = saved_pc;

      /* Check if the compiler explicitly indicated where a breakpoint should
	 be placed to skip the prologue.  */
      if (!ignore_prologue_end_flag && skip)
	{
	  std::optional<CORE_ADDR> linetable_pc
	    = skip_prologue_using_linetable (pc);
	  if (linetable_pc)
	    {
	      pc = *linetable_pc;
	      start_sal = find_pc_sect_line (pc, section, 0);
	      force_skip = 1;
	      continue;
	    }
	}

      /* If the function is in an unmapped overlay, use its unmapped LMA address,
	 so that gdbarch_skip_prologue has something unique to work on.  */
      if (section_is_overlay (section) && !section_is_mapped (section))
	pc = overlay_unmapped_address (pc, section);

      /* Skip "first line" of function (which is actually its prologue).  */
      pc += gdbarch_deprecated_function_start_offset (gdbarch);
      if (gdbarch_skip_entrypoint_p (gdbarch))
	pc = gdbarch_skip_entrypoint (gdbarch, pc);
      if (skip)
	pc = gdbarch_skip_prologue_noexcept (gdbarch, pc);

      /* For overlays, map pc back into its mapped VMA range.  */
      pc = overlay_mapped_address (pc, section);

      /* Calculate line number.  */
      start_sal = find_pc_sect_line (pc, section, 0);

      /* Check if gdbarch_skip_prologue left us in mid-line, and the next
	 line is still part of the same function.  */
      if (skip && start_sal.pc != pc
	  && (sym ? (sym->value_block ()->entry_pc () <= start_sal.end
		     && start_sal.end < sym->value_block()->end ())
	      : (lookup_minimal_symbol_by_pc_section (start_sal.end, section).minsym
		 == lookup_minimal_symbol_by_pc_section (pc, section).minsym)))
	{
	  /* First pc of next line */
	  pc = start_sal.end;
	  /* Recalculate the line number (might not be N+1).  */
	  start_sal = find_pc_sect_line (pc, section, 0);
	}

      /* On targets with executable formats that don't have a concept of
	 constructors (ELF with .init has, PE doesn't), gcc emits a call
	 to `__main' in `main' between the prologue and before user
	 code.  */
      if (gdbarch_skip_main_prologue_p (gdbarch)
	  && name && strcmp_iw (name, "main") == 0)
	{
	  pc = gdbarch_skip_main_prologue (gdbarch, pc);
	  /* Recalculate the line number (might not be N+1).  */
	  start_sal = find_pc_sect_line (pc, section, 0);
	  force_skip = 1;
	}
    }
  while (!force_skip && skip--);

  /* If we still don't have a valid source line, try to find the first
     PC in the lineinfo table that belongs to the same function.  This
     happens with COFF debug info, which does not seem to have an
     entry in lineinfo table for the code after the prologue which has
     no direct relation to source.  For example, this was found to be
     the case with the DJGPP target using "gcc -gcoff" when the
     compiler inserted code after the prologue to make sure the stack
     is aligned.  */
  if (!force_skip && sym && start_sal.symtab == NULL)
    {
      pc = skip_prologue_using_lineinfo (pc, sym->symtab ());
      /* Recalculate the line number.  */
      start_sal = find_pc_sect_line (pc, section, 0);
    }

  /* If we're already past the prologue, leave SAL unchanged.  Otherwise
     forward SAL to the end of the prologue.  */
  if (sal->pc >= pc)
    return;

  sal->pc = pc;
  sal->section = section;
  sal->symtab = start_sal.symtab;
  sal->line = start_sal.line;
  sal->end = start_sal.end;

  /* Check if we are now inside an inlined function.  If we can,
     use the call site of the function instead.  */
  b = block_for_pc_sect (sal->pc, sal->section);
  function_block = NULL;
  while (b != NULL)
    {
      if (b->function () != NULL && b->inlined_p ())
	function_block = b;
      else if (b->function () != NULL)
	break;
      b = b->superblock ();
    }
  if (function_block != NULL
      && function_block->function ()->line () != 0)
    {
      sal->line = function_block->function ()->line ();
      sal->symtab = function_block->function ()->symtab ();
    }
}

/* Given PC at the function's start address, attempt to find the
   prologue end using SAL information.  Return zero if the skip fails.

   A non-optimized prologue traditionally has one SAL for the function
   and a second for the function body.  A single line function has
   them both pointing at the same line.

   An optimized prologue is similar but the prologue may contain
   instructions (SALs) from the instruction body.  Need to skip those
   while not getting into the function body.

   The functions end point and an increasing SAL line are used as
   indicators of the prologue's endpoint.

   This code is based on the function refine_prologue_limit
   (found in ia64).  */

CORE_ADDR
skip_prologue_using_sal (struct gdbarch *gdbarch, CORE_ADDR func_addr)
{
  struct symtab_and_line prologue_sal;
  CORE_ADDR start_pc;
  CORE_ADDR end_pc;
  const struct block *bl;

  /* Get an initial range for the function.  */
  find_pc_partial_function (func_addr, NULL, &start_pc, &end_pc);
  start_pc += gdbarch_deprecated_function_start_offset (gdbarch);

  prologue_sal = find_pc_line (start_pc, 0);
  if (prologue_sal.line != 0)
    {
      /* For languages other than assembly, treat two consecutive line
	 entries at the same address as a zero-instruction prologue.
	 The GNU assembler emits separate line notes for each instruction
	 in a multi-instruction macro, but compilers generally will not
	 do this.  */
      if (prologue_sal.symtab->language () != language_asm)
	{
	  struct objfile *objfile
	    = prologue_sal.symtab->compunit ()->objfile ();
	  const linetable *linetable = prologue_sal.symtab->linetable ();
	  gdb_assert (linetable->nitems > 0);
	  int idx = 0;

	  /* Skip any earlier lines, and any end-of-sequence marker
	     from a previous function.  */
	  while (idx + 1 < linetable->nitems
		 && (linetable->item[idx].pc (objfile) != prologue_sal.pc
		     || linetable->item[idx].line == 0))
	    idx++;

	  if (idx + 1 < linetable->nitems
	      && linetable->item[idx+1].line != 0
	      && linetable->item[idx+1].pc (objfile) == start_pc)
	    return start_pc;
	}

      /* If there is only one sal that covers the entire function,
	 then it is probably a single line function, like
	 "foo(){}".  */
      if (prologue_sal.end >= end_pc)
	return 0;

      while (prologue_sal.end < end_pc)
	{
	  struct symtab_and_line sal;

	  sal = find_pc_line (prologue_sal.end, 0);
	  if (sal.line == 0)
	    break;
	  /* Assume that a consecutive SAL for the same (or larger)
	     line mark the prologue -> body transition.  */
	  if (sal.line >= prologue_sal.line)
	    break;
	  /* Likewise if we are in a different symtab altogether
	     (e.g. within a file included via #include).  */
	  if (sal.symtab != prologue_sal.symtab)
	    break;

	  /* The line number is smaller.  Check that it's from the
	     same function, not something inlined.  If it's inlined,
	     then there is no point comparing the line numbers.  */
	  bl = block_for_pc (prologue_sal.end);
	  while (bl)
	    {
	      if (bl->inlined_p ())
		break;
	      if (bl->function ())
		{
		  bl = NULL;
		  break;
		}
	      bl = bl->superblock ();
	    }
	  if (bl != NULL)
	    break;

	  /* The case in which compiler's optimizer/scheduler has
	     moved instructions into the prologue.  We look ahead in
	     the function looking for address ranges whose
	     corresponding line number is less the first one that we
	     found for the function.  This is more conservative then
	     refine_prologue_limit which scans a large number of SALs
	     looking for any in the prologue.  */
	  prologue_sal = sal;
	}
    }

  if (prologue_sal.end < end_pc)
    /* Return the end of this line, or zero if we could not find a
       line.  */
    return prologue_sal.end;
  else
    /* Don't return END_PC, which is past the end of the function.  */
    return prologue_sal.pc;
}

/* See symtab.h.  */

std::optional<CORE_ADDR>
find_epilogue_using_linetable (CORE_ADDR func_addr)
{
  CORE_ADDR start_pc, end_pc;

  if (!find_pc_partial_function (func_addr, nullptr, &start_pc, &end_pc))
    return {};

  const struct symtab_and_line sal = find_pc_line (start_pc, 0);
  if (sal.symtab != nullptr && sal.symtab->language () != language_asm)
    {
      struct objfile *objfile = sal.symtab->compunit ()->objfile ();
      unrelocated_addr unrel_start
	= unrelocated_addr (start_pc - objfile->text_section_offset ());
      unrelocated_addr unrel_end
	= unrelocated_addr (end_pc - objfile->text_section_offset ());

      const linetable *linetable = sal.symtab->linetable ();
      /* This should find the last linetable entry of the current function.
	 It is probably where the epilogue begins, but since the DWARF 5
	 spec doesn't guarantee it, we iterate backwards through the function
	 until we either find it or are sure that it doesn't exist.  */
      auto it = std::lower_bound
	(linetable->item, linetable->item + linetable->nitems, unrel_end,
	 [] (const linetable_entry &lte, unrelocated_addr pc)
	 {
	   return lte.unrelocated_pc () < pc;
	 });

      while (it->unrelocated_pc () >= unrel_start)
      {
	if (it->epilogue_begin)
	  return {it->pc (objfile)};
	it --;
      }
    }
  return {};
}

/* See symtab.h.  */

symbol *
find_function_alias_target (bound_minimal_symbol msymbol)
{
  CORE_ADDR func_addr;
  if (!msymbol_is_function (msymbol.objfile, msymbol.minsym, &func_addr))
    return NULL;

  symbol *sym = find_pc_function (func_addr);
  if (sym != NULL
      && sym->aclass () == LOC_BLOCK
      && sym->value_block ()->entry_pc () == func_addr)
    return sym;

  return NULL;
}


/* If P is of the form "operator[ \t]+..." where `...' is
   some legitimate operator text, return a pointer to the
   beginning of the substring of the operator text.
   Otherwise, return "".  */

static const char *
operator_chars (const char *p, const char **end)
{
  *end = "";
  if (!startswith (p, CP_OPERATOR_STR))
    return *end;
  p += CP_OPERATOR_LEN;

  /* Don't get faked out by `operator' being part of a longer
     identifier.  */
  if (isalpha (*p) || *p == '_' || *p == '$' || *p == '\0')
    return *end;

  /* Allow some whitespace between `operator' and the operator symbol.  */
  while (*p == ' ' || *p == '\t')
    p++;

  /* Recognize 'operator TYPENAME'.  */

  if (isalpha (*p) || *p == '_' || *p == '$')
    {
      const char *q = p + 1;

      while (isalnum (*q) || *q == '_' || *q == '$')
	q++;
      *end = q;
      return p;
    }

  while (*p)
    switch (*p)
      {
      case '\\':			/* regexp quoting */
	if (p[1] == '*')
	  {
	    if (p[2] == '=')		/* 'operator\*=' */
	      *end = p + 3;
	    else			/* 'operator\*'  */
	      *end = p + 2;
	    return p;
	  }
	else if (p[1] == '[')
	  {
	    if (p[2] == ']')
	      error (_("mismatched quoting on brackets, "
		       "try 'operator\\[\\]'"));
	    else if (p[2] == '\\' && p[3] == ']')
	      {
		*end = p + 4;	/* 'operator\[\]' */
		return p;
	      }
	    else
	      error (_("nothing is allowed between '[' and ']'"));
	  }
	else
	  {
	    /* Gratuitous quote: skip it and move on.  */
	    p++;
	    continue;
	  }
	break;
      case '!':
      case '=':
      case '*':
      case '/':
      case '%':
      case '^':
	if (p[1] == '=')
	  *end = p + 2;
	else
	  *end = p + 1;
	return p;
      case '<':
      case '>':
      case '+':
      case '-':
      case '&':
      case '|':
	if (p[0] == '-' && p[1] == '>')
	  {
	    /* Struct pointer member operator 'operator->'.  */
	    if (p[2] == '*')
	      {
		*end = p + 3;	/* 'operator->*' */
		return p;
	      }
	    else if (p[2] == '\\')
	      {
		*end = p + 4;	/* Hopefully 'operator->\*' */
		return p;
	      }
	    else
	      {
		*end = p + 2;	/* 'operator->' */
		return p;
	      }
	  }
	if (p[1] == '=' || p[1] == p[0])
	  *end = p + 2;
	else
	  *end = p + 1;
	return p;
      case '~':
      case ',':
	*end = p + 1;
	return p;
      case '(':
	if (p[1] != ')')
	  error (_("`operator ()' must be specified "
		   "without whitespace in `()'"));
	*end = p + 2;
	return p;
      case '?':
	if (p[1] != ':')
	  error (_("`operator ?:' must be specified "
		   "without whitespace in `?:'"));
	*end = p + 2;
	return p;
      case '[':
	if (p[1] != ']')
	  error (_("`operator []' must be specified "
		   "without whitespace in `[]'"));
	*end = p + 2;
	return p;
      default:
	error (_("`operator %s' not supported"), p);
	break;
      }

  *end = "";
  return *end;
}


/* See class declaration.  */

info_sources_filter::info_sources_filter (match_on match_type,
					  const char *regexp)
  : m_match_type (match_type),
    m_regexp (regexp)
{
  /* Setup the compiled regular expression M_C_REGEXP based on M_REGEXP.  */
  if (m_regexp != nullptr && *m_regexp != '\0')
    {
      gdb_assert (m_regexp != nullptr);

      int cflags = REG_NOSUB;
#ifdef HAVE_CASE_INSENSITIVE_FILE_SYSTEM
      cflags |= REG_ICASE;
#endif
      m_c_regexp.emplace (m_regexp, cflags, _("Invalid regexp"));
    }
}

/* See class declaration.  */

bool
info_sources_filter::matches (const char *fullname) const
{
  /* Does it match regexp?  */
  if (m_c_regexp.has_value ())
    {
      const char *to_match;
      std::string dirname;

      switch (m_match_type)
	{
	case match_on::DIRNAME:
	  dirname = ldirname (fullname);
	  to_match = dirname.c_str ();
	  break;
	case match_on::BASENAME:
	  to_match = lbasename (fullname);
	  break;
	case match_on::FULLNAME:
	  to_match = fullname;
	  break;
	default:
	  gdb_assert_not_reached ("bad m_match_type");
	}

      if (m_c_regexp->exec (to_match, 0, NULL, 0) != 0)
	return false;
    }

  return true;
}

/* Data structure to maintain the state used for printing the results of
   the 'info sources' command.  */

struct output_source_filename_data
{
  /* Create an object for displaying the results of the 'info sources'
     command to UIOUT.  FILTER must remain valid and unchanged for the
     lifetime of this object as this object retains a reference to FILTER.  */
  output_source_filename_data (struct ui_out *uiout,
			       const info_sources_filter &filter)
    : m_filter (filter),
      m_uiout (uiout)
  { /* Nothing.  */ }

  DISABLE_COPY_AND_ASSIGN (output_source_filename_data);

  /* Reset enough state of this object so we can match against a new set of
     files.  The existing regular expression is retained though.  */
  void reset_output ()
  {
    m_first = true;
    m_filename_seen_cache.clear ();
  }

  /* Worker for sources_info, outputs the file name formatted for either
     cli or mi (based on the current_uiout).  In cli mode displays
     FULLNAME with a comma separating this name from any previously
     printed name (line breaks are added at the comma).  In MI mode
     outputs a tuple containing DISP_NAME (the files display name),
     FULLNAME, and EXPANDED_P (true when this file is from a fully
     expanded symtab, otherwise false).  */
  void output (const char *disp_name, const char *fullname, bool expanded_p);

  /* An overload suitable for use as a callback to
     quick_symbol_functions::map_symbol_filenames.  */
  void operator() (const char *filename, const char *fullname)
  {
    /* The false here indicates that this file is from an unexpanded
       symtab.  */
    output (filename, fullname, false);
  }

  /* Return true if at least one filename has been printed (after a call to
     output) since either this object was created, or the last call to
     reset_output.  */
  bool printed_filename_p () const
  {
    return !m_first;
  }

private:

  /* Flag of whether we're printing the first one.  */
  bool m_first = true;

  /* Cache of what we've seen so far.  */
  filename_seen_cache m_filename_seen_cache;

  /* How source filename should be filtered.  */
  const info_sources_filter &m_filter;

  /* The object to which output is sent.  */
  struct ui_out *m_uiout;
};

/* See comment in class declaration above.  */

void
output_source_filename_data::output (const char *disp_name,
				     const char *fullname,
				     bool expanded_p)
{
  /* Since a single source file can result in several partial symbol
     tables, we need to avoid printing it more than once.  Note: if
     some of the psymtabs are read in and some are not, it gets
     printed both under "Source files for which symbols have been
     read" and "Source files for which symbols will be read in on
     demand".  I consider this a reasonable way to deal with the
     situation.  I'm not sure whether this can also happen for
     symtabs; it doesn't hurt to check.  */

  /* Was NAME already seen?  If so, then don't print it again.  */
  if (m_filename_seen_cache.seen (fullname))
    return;

  /* If the filter rejects this file then don't print it.  */
  if (!m_filter.matches (fullname))
    return;

  ui_out_emit_tuple ui_emitter (m_uiout, nullptr);

  /* Print it and reset *FIRST.  */
  if (!m_first)
    m_uiout->text (", ");
  m_first = false;

  m_uiout->wrap_hint (0);
  if (m_uiout->is_mi_like_p ())
    {
      m_uiout->field_string ("file", disp_name, file_name_style.style ());
      if (fullname != nullptr)
	m_uiout->field_string ("fullname", fullname,
			       file_name_style.style ());
      m_uiout->field_string ("debug-fully-read",
			     (expanded_p ? "true" : "false"));
    }
  else
    {
      if (fullname == nullptr)
	fullname = disp_name;
      m_uiout->field_string ("fullname", fullname,
			     file_name_style.style ());
    }
}

/* For the 'info sources' command, what part of the file names should we be
   matching the user supplied regular expression against?  */

struct filename_partial_match_opts
{
  /* Only match the directory name part.   */
  bool dirname = false;

  /* Only match the basename part.  */
  bool basename = false;
};

using isrc_flag_option_def
  = gdb::option::flag_option_def<filename_partial_match_opts>;

static const gdb::option::option_def info_sources_option_defs[] = {

  isrc_flag_option_def {
    "dirname",
    [] (filename_partial_match_opts *opts) { return &opts->dirname; },
    N_("Show only the files having a dirname matching REGEXP."),
  },

  isrc_flag_option_def {
    "basename",
    [] (filename_partial_match_opts *opts) { return &opts->basename; },
    N_("Show only the files having a basename matching REGEXP."),
  },

};

/* Create an option_def_group for the "info sources" options, with
   ISRC_OPTS as context.  */

static inline gdb::option::option_def_group
make_info_sources_options_def_group (filename_partial_match_opts *isrc_opts)
{
  return {{info_sources_option_defs}, isrc_opts};
}

/* Completer for "info sources".  */

static void
info_sources_command_completer (cmd_list_element *ignore,
				completion_tracker &tracker,
				const char *text, const char *word)
{
  const auto group = make_info_sources_options_def_group (nullptr);
  if (gdb::option::complete_options
      (tracker, &text, gdb::option::PROCESS_OPTIONS_UNKNOWN_IS_OPERAND, group))
    return;
}

/* See symtab.h.  */

void
info_sources_worker (struct ui_out *uiout,
		     bool group_by_objfile,
		     const info_sources_filter &filter)
{
  output_source_filename_data data (uiout, filter);

  ui_out_emit_list results_emitter (uiout, "files");
  std::optional<ui_out_emit_tuple> output_tuple;
  std::optional<ui_out_emit_list> sources_list;

  gdb_assert (group_by_objfile || uiout->is_mi_like_p ());

  for (objfile *objfile : current_program_space->objfiles ())
    {
      if (group_by_objfile)
	{
	  output_tuple.emplace (uiout, nullptr);
	  uiout->field_string ("filename", objfile_name (objfile),
			       file_name_style.style ());
	  uiout->text (":\n");
	  bool debug_fully_readin = !objfile->has_unexpanded_symtabs ();
	  if (uiout->is_mi_like_p ())
	    {
	      const char *debug_info_state;
	      if (objfile_has_symbols (objfile))
		{
		  if (debug_fully_readin)
		    debug_info_state = "fully-read";
		  else
		    debug_info_state = "partially-read";
		}
	      else
		debug_info_state = "none";
	      current_uiout->field_string ("debug-info", debug_info_state);
	    }
	  else
	    {
	      if (!debug_fully_readin)
		uiout->text ("(Full debug information has not yet been read "
			     "for this file.)\n");
	      if (!objfile_has_symbols (objfile))
		uiout->text ("(Objfile has no debug information.)\n");
	      uiout->text ("\n");
	    }
	  sources_list.emplace (uiout, "sources");
	}

      for (compunit_symtab *cu : objfile->compunits ())
	{
	  for (symtab *s : cu->filetabs ())
	    {
	      const char *file = symtab_to_filename_for_display (s);
	      const char *fullname = symtab_to_fullname (s);
	      data.output (file, fullname, true);
	    }
	}

      if (group_by_objfile)
	{
	  objfile->map_symbol_filenames (data, true /* need_fullname */);
	  if (data.printed_filename_p ())
	    uiout->text ("\n\n");
	  data.reset_output ();
	  sources_list.reset ();
	  output_tuple.reset ();
	}
    }

  if (!group_by_objfile)
    {
      data.reset_output ();
      map_symbol_filenames (data, true /*need_fullname*/);
    }
}

/* Implement the 'info sources' command.  */

static void
info_sources_command (const char *args, int from_tty)
{
  if (!have_full_symbols () && !have_partial_symbols ())
    error (_("No symbol table is loaded.  Use the \"file\" command."));

  filename_partial_match_opts match_opts;
  auto group = make_info_sources_options_def_group (&match_opts);
  gdb::option::process_options
    (&args, gdb::option::PROCESS_OPTIONS_UNKNOWN_IS_ERROR, group);

  if (match_opts.dirname && match_opts.basename)
    error (_("You cannot give both -basename and -dirname to 'info sources'."));

  const char *regex = nullptr;
  if (args != NULL && *args != '\000')
    regex = args;

  if ((match_opts.dirname || match_opts.basename) && regex == nullptr)
    error (_("Missing REGEXP for 'info sources'."));

  info_sources_filter::match_on match_type;
  if (match_opts.dirname)
    match_type = info_sources_filter::match_on::DIRNAME;
  else if (match_opts.basename)
    match_type = info_sources_filter::match_on::BASENAME;
  else
    match_type = info_sources_filter::match_on::FULLNAME;

  info_sources_filter filter (match_type, regex);
  info_sources_worker (current_uiout, true, filter);
}

/* Compare FILE against all the entries of FILENAMES.  If BASENAMES is
   true compare only lbasename of FILENAMES.  */

static bool
file_matches (const char *file, const std::vector<const char *> &filenames,
	      bool basenames)
{
  if (filenames.empty ())
    return true;

  for (const char *name : filenames)
    {
      name = (basenames ? lbasename (name) : name);
      if (compare_filenames_for_search (file, name))
	return true;
    }

  return false;
}

/* Helper function for std::sort on symbol_search objects.  Can only sort
   symbols, not minimal symbols.  */

int
symbol_search::compare_search_syms (const symbol_search &sym_a,
				    const symbol_search &sym_b)
{
  int c;

  c = FILENAME_CMP (sym_a.symbol->symtab ()->filename,
		    sym_b.symbol->symtab ()->filename);
  if (c != 0)
    return c;

  if (sym_a.block != sym_b.block)
    return sym_a.block - sym_b.block;

  return strcmp (sym_a.symbol->print_name (), sym_b.symbol->print_name ());
}

/* Returns true if the type_name of symbol_type of SYM matches TREG.
   If SYM has no symbol_type or symbol_name, returns false.  */

bool
treg_matches_sym_type_name (const compiled_regex &treg,
			    const struct symbol *sym)
{
  struct type *sym_type;
  std::string printed_sym_type_name;

  symbol_lookup_debug_printf_v ("treg_matches_sym_type_name, sym %s",
				sym->natural_name ());

  sym_type = sym->type ();
  if (sym_type == NULL)
    return false;

  {
    scoped_switch_to_sym_language_if_auto l (sym);

    printed_sym_type_name = type_to_string (sym_type);
  }

  symbol_lookup_debug_printf_v ("sym_type_name %s",
				printed_sym_type_name.c_str ());

  if (printed_sym_type_name.empty ())
    return false;

  return treg.exec (printed_sym_type_name.c_str (), 0, NULL, 0) == 0;
}

/* See symtab.h.  */

bool
global_symbol_searcher::is_suitable_msymbol
	(const enum search_domain kind, const minimal_symbol *msymbol)
{
  switch (msymbol->type ())
    {
    case mst_data:
    case mst_bss:
    case mst_file_data:
    case mst_file_bss:
      return kind == VARIABLES_DOMAIN;
    case mst_text:
    case mst_file_text:
    case mst_solib_trampoline:
    case mst_text_gnu_ifunc:
      return kind == FUNCTIONS_DOMAIN;
    default:
      return false;
    }
}

/* See symtab.h.  */

bool
global_symbol_searcher::expand_symtabs
	(objfile *objfile, const std::optional<compiled_regex> &preg) const
{
  enum search_domain kind = m_kind;
  bool found_msymbol = false;

  auto do_file_match = [&] (const char *filename, bool basenames)
    {
      return file_matches (filename, filenames, basenames);
    };
  gdb::function_view<expand_symtabs_file_matcher_ftype> file_matcher = nullptr;
  if (!filenames.empty ())
    file_matcher = do_file_match;

  objfile->expand_symtabs_matching
    (file_matcher,
     &lookup_name_info::match_any (),
     [&] (const char *symname)
     {
       return (!preg.has_value ()
	       || preg->exec (symname, 0, NULL, 0) == 0);
     },
     NULL,
     SEARCH_GLOBAL_BLOCK | SEARCH_STATIC_BLOCK,
     UNDEF_DOMAIN,
     kind);

  /* Here, we search through the minimal symbol tables for functions and
     variables that match, and force their symbols to be read.  This is in
     particular necessary for demangled variable names, which are no longer
     put into the partial symbol tables.  The symbol will then be found
     during the scan of symtabs later.

     For functions, find_pc_symtab should succeed if we have debug info for
     the function, for variables we have to call
     lookup_symbol_in_objfile_from_linkage_name to determine if the
     variable has debug info.  If the lookup fails, set found_msymbol so
     that we will rescan to print any matching symbols without debug info.
     We only search the objfile the msymbol came from, we no longer search
     all objfiles.  In large programs (1000s of shared libs) searching all
     objfiles is not worth the pain.  */
  if (filenames.empty ()
      && (kind == VARIABLES_DOMAIN || kind == FUNCTIONS_DOMAIN))
    {
      for (minimal_symbol *msymbol : objfile->msymbols ())
	{
	  QUIT;

	  if (msymbol->created_by_gdb)
	    continue;

	  if (is_suitable_msymbol (kind, msymbol))
	    {
	      if (!preg.has_value ()
		  || preg->exec (msymbol->natural_name (), 0,
				 NULL, 0) == 0)
		{
		  /* An important side-effect of these lookup functions is
		     to expand the symbol table if msymbol is found, later
		     in the process we will add matching symbols or
		     msymbols to the results list, and that requires that
		     the symbols tables are expanded.  */
		  if (kind == FUNCTIONS_DOMAIN
		      ? (find_pc_compunit_symtab
			 (msymbol->value_address (objfile)) == NULL)
		      : (lookup_symbol_in_objfile_from_linkage_name
			 (objfile, msymbol->linkage_name (),
			  VAR_DOMAIN)
			 .symbol == NULL))
		    found_msymbol = true;
		}
	    }
	}
    }

  return found_msymbol;
}

/* See symtab.h.  */

bool
global_symbol_searcher::add_matching_symbols
	(objfile *objfile,
	 const std::optional<compiled_regex> &preg,
	 const std::optional<compiled_regex> &treg,
	 std::set<symbol_search> *result_set) const
{
  enum search_domain kind = m_kind;

  /* Add matching symbols (if not already present).  */
  for (compunit_symtab *cust : objfile->compunits ())
    {
      const struct blockvector *bv  = cust->blockvector ();

      for (block_enum block : { GLOBAL_BLOCK, STATIC_BLOCK })
	{
	  const struct block *b = bv->block (block);

	  for (struct symbol *sym : block_iterator_range (b))
	    {
	      struct symtab *real_symtab = sym->symtab ();

	      QUIT;

	      /* Check first sole REAL_SYMTAB->FILENAME.  It does
		 not need to be a substring of symtab_to_fullname as
		 it may contain "./" etc.  */
	      if ((file_matches (real_symtab->filename, filenames, false)
		   || ((basenames_may_differ
			|| file_matches (lbasename (real_symtab->filename),
					 filenames, true))
		       && file_matches (symtab_to_fullname (real_symtab),
					filenames, false)))
		  && ((!preg.has_value ()
		       || preg->exec (sym->natural_name (), 0,
				      NULL, 0) == 0)
		      && ((kind == VARIABLES_DOMAIN
			   && sym->aclass () != LOC_TYPEDEF
			   && sym->aclass () != LOC_UNRESOLVED
			   && sym->aclass () != LOC_BLOCK
			   /* LOC_CONST can be used for more than
			      just enums, e.g., c++ static const
			      members.  We only want to skip enums
			      here.  */
			   && !(sym->aclass () == LOC_CONST
				&& (sym->type ()->code ()
				    == TYPE_CODE_ENUM))
			   && (!treg.has_value ()
			       || treg_matches_sym_type_name (*treg, sym)))
			  || (kind == FUNCTIONS_DOMAIN
			      && sym->aclass () == LOC_BLOCK
			      && (!treg.has_value ()
				  || treg_matches_sym_type_name (*treg,
								 sym)))
			  || (kind == TYPES_DOMAIN
			      && sym->aclass () == LOC_TYPEDEF
			      && sym->domain () != MODULE_DOMAIN)
			  || (kind == MODULES_DOMAIN
			      && sym->domain () == MODULE_DOMAIN
			      && sym->line () != 0))))
		{
		  if (result_set->size () < m_max_search_results)
		    {
		      /* Match, insert if not already in the results.  */
		      symbol_search ss (block, sym);
		      if (result_set->find (ss) == result_set->end ())
			result_set->insert (ss);
		    }
		  else
		    return false;
		}
	    }
	}
    }

  return true;
}

/* See symtab.h.  */

bool
global_symbol_searcher::add_matching_msymbols
	(objfile *objfile, const std::optional<compiled_regex> &preg,
	 std::vector<symbol_search> *results) const
{
  enum search_domain kind = m_kind;

  for (minimal_symbol *msymbol : objfile->msymbols ())
    {
      QUIT;

      if (msymbol->created_by_gdb)
	continue;

      if (is_suitable_msymbol (kind, msymbol))
	{
	  if (!preg.has_value ()
	      || preg->exec (msymbol->natural_name (), 0,
			     NULL, 0) == 0)
	    {
	      /* For functions we can do a quick check of whether the
		 symbol might be found via find_pc_symtab.  */
	      if (kind != FUNCTIONS_DOMAIN
		  || (find_pc_compunit_symtab
		      (msymbol->value_address (objfile)) == NULL))
		{
		  if (lookup_symbol_in_objfile_from_linkage_name
		      (objfile, msymbol->linkage_name (),
		       VAR_DOMAIN).symbol == NULL)
		    {
		      /* Matching msymbol, add it to the results list.  */
		      if (results->size () < m_max_search_results)
			results->emplace_back (GLOBAL_BLOCK, msymbol, objfile);
		      else
			return false;
		    }
		}
	    }
	}
    }

  return true;
}

/* See symtab.h.  */

std::vector<symbol_search>
global_symbol_searcher::search () const
{
  std::optional<compiled_regex> preg;
  std::optional<compiled_regex> treg;

  gdb_assert (m_kind != ALL_DOMAIN);

  if (m_symbol_name_regexp != NULL)
    {
      const char *symbol_name_regexp = m_symbol_name_regexp;
      std::string symbol_name_regexp_holder;

      /* Make sure spacing is right for C++ operators.
	 This is just a courtesy to make the matching less sensitive
	 to how many spaces the user leaves between 'operator'
	 and <TYPENAME> or <OPERATOR>.  */
      const char *opend;
      const char *opname = operator_chars (symbol_name_regexp, &opend);

      if (*opname)
	{
	  int fix = -1;		/* -1 means ok; otherwise number of
				    spaces needed.  */

	  if (isalpha (*opname) || *opname == '_' || *opname == '$')
	    {
	      /* There should 1 space between 'operator' and 'TYPENAME'.  */
	      if (opname[-1] != ' ' || opname[-2] == ' ')
		fix = 1;
	    }
	  else
	    {
	      /* There should 0 spaces between 'operator' and 'OPERATOR'.  */
	      if (opname[-1] == ' ')
		fix = 0;
	    }
	  /* If wrong number of spaces, fix it.  */
	  if (fix >= 0)
	    {
	      symbol_name_regexp_holder
		= string_printf ("operator%.*s%s", fix, " ", opname);
	      symbol_name_regexp = symbol_name_regexp_holder.c_str ();
	    }
	}

      int cflags = REG_NOSUB | (case_sensitivity == case_sensitive_off
				? REG_ICASE : 0);
      preg.emplace (symbol_name_regexp, cflags,
		    _("Invalid regexp"));
    }

  if (m_symbol_type_regexp != NULL)
    {
      int cflags = REG_NOSUB | (case_sensitivity == case_sensitive_off
				? REG_ICASE : 0);
      treg.emplace (m_symbol_type_regexp, cflags,
		    _("Invalid regexp"));
    }

  bool found_msymbol = false;
  std::set<symbol_search> result_set;
  for (objfile *objfile : current_program_space->objfiles ())
    {
      /* Expand symtabs within objfile that possibly contain matching
	 symbols.  */
      found_msymbol |= expand_symtabs (objfile, preg);

      /* Find matching symbols within OBJFILE and add them in to the
	 RESULT_SET set.  Use a set here so that we can easily detect
	 duplicates as we go, and can therefore track how many unique
	 matches we have found so far.  */
      if (!add_matching_symbols (objfile, preg, treg, &result_set))
	break;
    }

  /* Convert the result set into a sorted result list, as std::set is
     defined to be sorted then no explicit call to std::sort is needed.  */
  std::vector<symbol_search> result (result_set.begin (), result_set.end ());

  /* If there are no debug symbols, then add matching minsyms.  But if the
     user wants to see symbols matching a type regexp, then never give a
     minimal symbol, as we assume that a minimal symbol does not have a
     type.  */
  if ((found_msymbol || (filenames.empty () && m_kind == VARIABLES_DOMAIN))
      && !m_exclude_minsyms
      && !treg.has_value ())
    {
      gdb_assert (m_kind == VARIABLES_DOMAIN || m_kind == FUNCTIONS_DOMAIN);
      for (objfile *objfile : current_program_space->objfiles ())
	if (!add_matching_msymbols (objfile, preg, &result))
	  break;
    }

  return result;
}

/* See symtab.h.  */

std::string
symbol_to_info_string (struct symbol *sym, int block,
		       enum search_domain kind)
{
  std::string str;

  gdb_assert (block == GLOBAL_BLOCK || block == STATIC_BLOCK);

  if (kind != TYPES_DOMAIN && block == STATIC_BLOCK)
    str += "static ";

  /* Typedef that is not a C++ class.  */
  if (kind == TYPES_DOMAIN
      && sym->domain () != STRUCT_DOMAIN)
    {
      string_file tmp_stream;

      /* FIXME: For C (and C++) we end up with a difference in output here
	 between how a typedef is printed, and non-typedefs are printed.
	 The TYPEDEF_PRINT code places a ";" at the end in an attempt to
	 appear C-like, while TYPE_PRINT doesn't.

	 For the struct printing case below, things are worse, we force
	 printing of the ";" in this function, which is going to be wrong
	 for languages that don't require a ";" between statements.  */
      if (sym->type ()->code () == TYPE_CODE_TYPEDEF)
	typedef_print (sym->type (), sym, &tmp_stream);
      else
	type_print (sym->type (), "", &tmp_stream, -1);
      str += tmp_stream.string ();
    }
  /* variable, func, or typedef-that-is-c++-class.  */
  else if (kind < TYPES_DOMAIN
	   || (kind == TYPES_DOMAIN
	       && sym->domain () == STRUCT_DOMAIN))
    {
      string_file tmp_stream;

      type_print (sym->type (),
		  (sym->aclass () == LOC_TYPEDEF
		   ? "" : sym->print_name ()),
		  &tmp_stream, 0);

      str += tmp_stream.string ();
      str += ";";
    }
  /* Printing of modules is currently done here, maybe at some future
     point we might want a language specific method to print the module
     symbol so that we can customise the output more.  */
  else if (kind == MODULES_DOMAIN)
    str += sym->print_name ();

  return str;
}

/* Helper function for symbol info commands, for example 'info functions',
   'info variables', etc.  KIND is the kind of symbol we searched for, and
   BLOCK is the type of block the symbols was found in, either GLOBAL_BLOCK
   or STATIC_BLOCK.  SYM is the symbol we found.  If LAST is not NULL,
   print file and line number information for the symbol as well.  Skip
   printing the filename if it matches LAST.  */

static void
print_symbol_info (enum search_domain kind,
		   struct symbol *sym,
		   int block, const char *last)
{
  scoped_switch_to_sym_language_if_auto l (sym);
  struct symtab *s = sym->symtab ();

  if (last != NULL)
    {
      const char *s_filename = symtab_to_filename_for_display (s);

      if (filename_cmp (last, s_filename) != 0)
	{
	  gdb_printf (_("\nFile %ps:\n"),
		      styled_string (file_name_style.style (),
				     s_filename));
	}

      if (sym->line () != 0)
	gdb_printf ("%d:\t", sym->line ());
      else
	gdb_puts ("\t");
    }

  std::string str = symbol_to_info_string (sym, block, kind);
  gdb_printf ("%s\n", str.c_str ());
}

/* This help function for symtab_symbol_info() prints information
   for non-debugging symbols to gdb_stdout.  */

static void
print_msymbol_info (struct bound_minimal_symbol msymbol)
{
  struct gdbarch *gdbarch = msymbol.objfile->arch ();
  char *tmp;

  if (gdbarch_addr_bit (gdbarch) <= 32)
    tmp = hex_string_custom (msymbol.value_address ()
			     & (CORE_ADDR) 0xffffffff,
			     8);
  else
    tmp = hex_string_custom (msymbol.value_address (),
			     16);

  ui_file_style sym_style = (msymbol.minsym->text_p ()
			     ? function_name_style.style ()
			     : ui_file_style ());

  gdb_printf (_("%ps  %ps\n"),
	      styled_string (address_style.style (), tmp),
	      styled_string (sym_style, msymbol.minsym->print_name ()));
}

/* This is the guts of the commands "info functions", "info types", and
   "info variables".  It calls search_symbols to find all matches and then
   print_[m]symbol_info to print out some useful information about the
   matches.  */

static void
symtab_symbol_info (bool quiet, bool exclude_minsyms,
		    const char *regexp, enum search_domain kind,
		    const char *t_regexp, int from_tty)
{
  static const char * const classnames[] =
    {"variable", "function", "type", "module"};
  const char *last_filename = "";
  int first = 1;

  gdb_assert (kind != ALL_DOMAIN);

  if (regexp != nullptr && *regexp == '\0')
    regexp = nullptr;

  global_symbol_searcher spec (kind, regexp);
  spec.set_symbol_type_regexp (t_regexp);
  spec.set_exclude_minsyms (exclude_minsyms);
  std::vector<symbol_search> symbols = spec.search ();

  if (!quiet)
    {
      if (regexp != NULL)
	{
	  if (t_regexp != NULL)
	    gdb_printf
	      (_("All %ss matching regular expression \"%s\""
		 " with type matching regular expression \"%s\":\n"),
	       classnames[kind], regexp, t_regexp);
	  else
	    gdb_printf (_("All %ss matching regular expression \"%s\":\n"),
			classnames[kind], regexp);
	}
      else
	{
	  if (t_regexp != NULL)
	    gdb_printf
	      (_("All defined %ss"
		 " with type matching regular expression \"%s\" :\n"),
	       classnames[kind], t_regexp);
	  else
	    gdb_printf (_("All defined %ss:\n"), classnames[kind]);
	}
    }

  for (const symbol_search &p : symbols)
    {
      QUIT;

      if (p.msymbol.minsym != NULL)
	{
	  if (first)
	    {
	      if (!quiet)
		gdb_printf (_("\nNon-debugging symbols:\n"));
	      first = 0;
	    }
	  print_msymbol_info (p.msymbol);
	}
      else
	{
	  print_symbol_info (kind,
			     p.symbol,
			     p.block,
			     last_filename);
	  last_filename
	    = symtab_to_filename_for_display (p.symbol->symtab ());
	}
    }
}

/* Structure to hold the values of the options used by the 'info variables'
   and 'info functions' commands.  These correspond to the -q, -t, and -n
   options.  */

struct info_vars_funcs_options
{
  bool quiet = false;
  bool exclude_minsyms = false;
  std::string type_regexp;
};

/* The options used by the 'info variables' and 'info functions'
   commands.  */

static const gdb::option::option_def info_vars_funcs_options_defs[] = {
  gdb::option::boolean_option_def<info_vars_funcs_options> {
    "q",
    [] (info_vars_funcs_options *opt) { return &opt->quiet; },
    nullptr, /* show_cmd_cb */
    nullptr /* set_doc */
  },

  gdb::option::boolean_option_def<info_vars_funcs_options> {
    "n",
    [] (info_vars_funcs_options *opt) { return &opt->exclude_minsyms; },
    nullptr, /* show_cmd_cb */
    nullptr /* set_doc */
  },

  gdb::option::string_option_def<info_vars_funcs_options> {
    "t",
    [] (info_vars_funcs_options *opt) { return &opt->type_regexp; },
    nullptr, /* show_cmd_cb */
    nullptr /* set_doc */
  }
};

/* Returns the option group used by 'info variables' and 'info
   functions'.  */

static gdb::option::option_def_group
make_info_vars_funcs_options_def_group (info_vars_funcs_options *opts)
{
  return {{info_vars_funcs_options_defs}, opts};
}

/* Command completer for 'info variables' and 'info functions'.  */

static void
info_vars_funcs_command_completer (struct cmd_list_element *ignore,
				   completion_tracker &tracker,
				   const char *text, const char * /* word */)
{
  const auto group
    = make_info_vars_funcs_options_def_group (nullptr);
  if (gdb::option::complete_options
      (tracker, &text, gdb::option::PROCESS_OPTIONS_UNKNOWN_IS_OPERAND, group))
    return;

  const char *word = advance_to_expression_complete_word_point (tracker, text);
  symbol_completer (ignore, tracker, text, word);
}

/* Implement the 'info variables' command.  */

static void
info_variables_command (const char *args, int from_tty)
{
  info_vars_funcs_options opts;
  auto grp = make_info_vars_funcs_options_def_group (&opts);
  gdb::option::process_options
    (&args, gdb::option::PROCESS_OPTIONS_UNKNOWN_IS_OPERAND, grp);
  if (args != nullptr && *args == '\0')
    args = nullptr;

  symtab_symbol_info
    (opts.quiet, opts.exclude_minsyms, args, VARIABLES_DOMAIN,
     opts.type_regexp.empty () ? nullptr : opts.type_regexp.c_str (),
     from_tty);
}

/* Implement the 'info functions' command.  */

static void
info_functions_command (const char *args, int from_tty)
{
  info_vars_funcs_options opts;

  auto grp = make_info_vars_funcs_options_def_group (&opts);
  gdb::option::process_options
    (&args, gdb::option::PROCESS_OPTIONS_UNKNOWN_IS_OPERAND, grp);
  if (args != nullptr && *args == '\0')
    args = nullptr;

  symtab_symbol_info
    (opts.quiet, opts.exclude_minsyms, args, FUNCTIONS_DOMAIN,
     opts.type_regexp.empty () ? nullptr : opts.type_regexp.c_str (),
     from_tty);
}

/* Holds the -q option for the 'info types' command.  */

struct info_types_options
{
  bool quiet = false;
};

/* The options used by the 'info types' command.  */

static const gdb::option::option_def info_types_options_defs[] = {
  gdb::option::boolean_option_def<info_types_options> {
    "q",
    [] (info_types_options *opt) { return &opt->quiet; },
    nullptr, /* show_cmd_cb */
    nullptr /* set_doc */
  }
};

/* Returns the option group used by 'info types'.  */

static gdb::option::option_def_group
make_info_types_options_def_group (info_types_options *opts)
{
  return {{info_types_options_defs}, opts};
}

/* Implement the 'info types' command.  */

static void
info_types_command (const char *args, int from_tty)
{
  info_types_options opts;

  auto grp = make_info_types_options_def_group (&opts);
  gdb::option::process_options
    (&args, gdb::option::PROCESS_OPTIONS_UNKNOWN_IS_OPERAND, grp);
  if (args != nullptr && *args == '\0')
    args = nullptr;
  symtab_symbol_info (opts.quiet, false, args, TYPES_DOMAIN, NULL, from_tty);
}

/* Command completer for 'info types' command.  */

static void
info_types_command_completer (struct cmd_list_element *ignore,
			      completion_tracker &tracker,
			      const char *text, const char * /* word */)
{
  const auto group
    = make_info_types_options_def_group (nullptr);
  if (gdb::option::complete_options
      (tracker, &text, gdb::option::PROCESS_OPTIONS_UNKNOWN_IS_OPERAND, group))
    return;

  const char *word = advance_to_expression_complete_word_point (tracker, text);
  symbol_completer (ignore, tracker, text, word);
}

/* Implement the 'info modules' command.  */

static void
info_modules_command (const char *args, int from_tty)
{
  info_types_options opts;

  auto grp = make_info_types_options_def_group (&opts);
  gdb::option::process_options
    (&args, gdb::option::PROCESS_OPTIONS_UNKNOWN_IS_OPERAND, grp);
  if (args != nullptr && *args == '\0')
    args = nullptr;
  symtab_symbol_info (opts.quiet, true, args, MODULES_DOMAIN, NULL,
		      from_tty);
}

/* Implement the 'info main' command.  */

static void
info_main_command (const char *args, int from_tty)
{
  gdb_printf ("%s\n", main_name ());
}

static void
rbreak_command (const char *regexp, int from_tty)
{
  std::string string;
  const char *file_name = nullptr;

  if (regexp != nullptr)
    {
      const char *colon = strchr (regexp, ':');

      /* Ignore the colon if it is part of a Windows drive.  */
      if (HAS_DRIVE_SPEC (regexp)
	  && (regexp[2] == '/' || regexp[2] == '\\'))
	colon = strchr (STRIP_DRIVE_SPEC (regexp), ':');

      if (colon && *(colon + 1) != ':')
	{
	  int colon_index;
	  char *local_name;

	  colon_index = colon - regexp;
	  local_name = (char *) alloca (colon_index + 1);
	  memcpy (local_name, regexp, colon_index);
	  local_name[colon_index--] = 0;
	  while (isspace (local_name[colon_index]))
	    local_name[colon_index--] = 0;
	  file_name = local_name;
	  regexp = skip_spaces (colon + 1);
	}
    }

  global_symbol_searcher spec (FUNCTIONS_DOMAIN, regexp);
  if (file_name != nullptr)
    spec.filenames.push_back (file_name);
  std::vector<symbol_search> symbols = spec.search ();

  scoped_rbreak_breakpoints finalize;
  for (const symbol_search &p : symbols)
    {
      if (p.msymbol.minsym == NULL)
	{
	  struct symtab *symtab = p.symbol->symtab ();
	  const char *fullname = symtab_to_fullname (symtab);

	  string = string_printf ("%s:'%s'", fullname,
				  p.symbol->linkage_name ());
	  break_command (&string[0], from_tty);
	  print_symbol_info (FUNCTIONS_DOMAIN, p.symbol, p.block, NULL);
	}
      else
	{
	  string = string_printf ("'%s'",
				  p.msymbol.minsym->linkage_name ());

	  break_command (&string[0], from_tty);
	  gdb_printf ("<function, no debug info> %s;\n",
		      p.msymbol.minsym->print_name ());
	}
    }
}


/* Evaluate if SYMNAME matches LOOKUP_NAME.  */

static int
compare_symbol_name (const char *symbol_name, language symbol_language,
		     const lookup_name_info &lookup_name,
		     completion_match_result &match_res)
{
  const language_defn *lang = language_def (symbol_language);

  symbol_name_matcher_ftype *name_match
    = lang->get_symbol_name_matcher (lookup_name);

  return name_match (symbol_name, lookup_name, &match_res);
}

/*  See symtab.h.  */

bool
completion_list_add_name (completion_tracker &tracker,
			  language symbol_language,
			  const char *symname,
			  const lookup_name_info &lookup_name,
			  const char *text, const char *word)
{
  completion_match_result &match_res
    = tracker.reset_completion_match_result ();

  /* Clip symbols that cannot match.  */
  if (!compare_symbol_name (symname, symbol_language, lookup_name, match_res))
    return false;

  /* Refresh SYMNAME from the match string.  It's potentially
     different depending on language.  (E.g., on Ada, the match may be
     the encoded symbol name wrapped in "<>").  */
  symname = match_res.match.match ();
  gdb_assert (symname != NULL);

  /* We have a match for a completion, so add SYMNAME to the current list
     of matches.  Note that the name is moved to freshly malloc'd space.  */

  {
    gdb::unique_xmalloc_ptr<char> completion
      = make_completion_match_str (symname, text, word);

    /* Here we pass the match-for-lcd object to add_completion.  Some
       languages match the user text against substrings of symbol
       names in some cases.  E.g., in C++, "b push_ba" completes to
       "std::vector::push_back", "std::string::push_back", etc., and
       in this case we want the completion lowest common denominator
       to be "push_back" instead of "std::".  */
    tracker.add_completion (std::move (completion),
			    &match_res.match_for_lcd, text, word);
  }

  return true;
}

/* completion_list_add_name wrapper for struct symbol.  */

static void
completion_list_add_symbol (completion_tracker &tracker,
			    symbol *sym,
			    const lookup_name_info &lookup_name,
			    const char *text, const char *word)
{
  if (!completion_list_add_name (tracker, sym->language (),
				 sym->natural_name (),
				 lookup_name, text, word))
    return;

  /* C++ function symbols include the parameters within both the msymbol
     name and the symbol name.  The problem is that the msymbol name will
     describe the parameters in the most basic way, with typedefs stripped
     out, while the symbol name will represent the types as they appear in
     the program.  This means we will see duplicate entries in the
     completion tracker.  The following converts the symbol name back to
     the msymbol name and removes the msymbol name from the completion
     tracker.  */
  if (sym->language () == language_cplus
      && sym->domain () == VAR_DOMAIN
      && sym->aclass () == LOC_BLOCK)
    {
      /* The call to canonicalize returns the empty string if the input
	 string is already in canonical form, thanks to this we don't
	 remove the symbol we just added above.  */
      gdb::unique_xmalloc_ptr<char> str
	= cp_canonicalize_string_no_typedefs (sym->natural_name ());
      if (str != nullptr)
	tracker.remove_completion (str.get ());
    }
}

/* completion_list_add_name wrapper for struct minimal_symbol.  */

static void
completion_list_add_msymbol (completion_tracker &tracker,
			     minimal_symbol *sym,
			     const lookup_name_info &lookup_name,
			     const char *text, const char *word)
{
  completion_list_add_name (tracker, sym->language (),
			    sym->natural_name (),
			    lookup_name, text, word);
}


/* ObjC: In case we are completing on a selector, look as the msymbol
   again and feed all the selectors into the mill.  */

static void
completion_list_objc_symbol (completion_tracker &tracker,
			     struct minimal_symbol *msymbol,
			     const lookup_name_info &lookup_name,
			     const char *text, const char *word)
{
  static char *tmp = NULL;
  static unsigned int tmplen = 0;

  const char *method, *category, *selector;
  char *tmp2 = NULL;

  method = msymbol->natural_name ();

  /* Is it a method?  */
  if ((method[0] != '-') && (method[0] != '+'))
    return;

  if (text[0] == '[')
    /* Complete on shortened method method.  */
    completion_list_add_name (tracker, language_objc,
			      method + 1,
			      lookup_name,
			      text, word);

  while ((strlen (method) + 1) >= tmplen)
    {
      if (tmplen == 0)
	tmplen = 1024;
      else
	tmplen *= 2;
      tmp = (char *) xrealloc (tmp, tmplen);
    }
  selector = strchr (method, ' ');
  if (selector != NULL)
    selector++;

  category = strchr (method, '(');

  if ((category != NULL) && (selector != NULL))
    {
      memcpy (tmp, method, (category - method));
      tmp[category - method] = ' ';
      memcpy (tmp + (category - method) + 1, selector, strlen (selector) + 1);
      completion_list_add_name (tracker, language_objc, tmp,
				lookup_name, text, word);
      if (text[0] == '[')
	completion_list_add_name (tracker, language_objc, tmp + 1,
				  lookup_name, text, word);
    }

  if (selector != NULL)
    {
      /* Complete on selector only.  */
      strcpy (tmp, selector);
      tmp2 = strchr (tmp, ']');
      if (tmp2 != NULL)
	*tmp2 = '\0';

      completion_list_add_name (tracker, language_objc, tmp,
				lookup_name, text, word);
    }
}

/* Break the non-quoted text based on the characters which are in
   symbols.  FIXME: This should probably be language-specific.  */

static const char *
language_search_unquoted_string (const char *text, const char *p)
{
  for (; p > text; --p)
    {
      if (isalnum (p[-1]) || p[-1] == '_' || p[-1] == '\0')
	continue;
      else
	{
	  if ((current_language->la_language == language_objc))
	    {
	      if (p[-1] == ':')     /* Might be part of a method name.  */
		continue;
	      else if (p[-1] == '[' && (p[-2] == '-' || p[-2] == '+'))
		p -= 2;             /* Beginning of a method name.  */
	      else if (p[-1] == ' ' || p[-1] == '(' || p[-1] == ')')
		{                   /* Might be part of a method name.  */
		  const char *t = p;

		  /* Seeing a ' ' or a '(' is not conclusive evidence
		     that we are in the middle of a method name.  However,
		     finding "-[" or "+[" should be pretty un-ambiguous.
		     Unfortunately we have to find it now to decide.  */

		  while (t > text)
		    if (isalnum (t[-1]) || t[-1] == '_' ||
			t[-1] == ' '    || t[-1] == ':' ||
			t[-1] == '('    || t[-1] == ')')
		      --t;
		    else
		      break;

		  if (t[-1] == '[' && (t[-2] == '-' || t[-2] == '+'))
		    p = t - 2;      /* Method name detected.  */
		  /* Else we leave with p unchanged.  */
		}
	    }
	  break;
	}
    }
  return p;
}

static void
completion_list_add_fields (completion_tracker &tracker,
			    struct symbol *sym,
			    const lookup_name_info &lookup_name,
			    const char *text, const char *word)
{
  if (sym->aclass () == LOC_TYPEDEF)
    {
      struct type *t = sym->type ();
      enum type_code c = t->code ();
      int j;

      if (c == TYPE_CODE_UNION || c == TYPE_CODE_STRUCT)
	for (j = TYPE_N_BASECLASSES (t); j < t->num_fields (); j++)
	  if (t->field (j).name ())
	    completion_list_add_name (tracker, sym->language (),
				      t->field (j).name (),
				      lookup_name, text, word);
    }
}

/* See symtab.h.  */

bool
symbol_is_function_or_method (symbol *sym)
{
  switch (sym->type ()->code ())
    {
    case TYPE_CODE_FUNC:
    case TYPE_CODE_METHOD:
      return true;
    default:
      return false;
    }
}

/* See symtab.h.  */

bool
symbol_is_function_or_method (minimal_symbol *msymbol)
{
  switch (msymbol->type ())
    {
    case mst_text:
    case mst_text_gnu_ifunc:
    case mst_solib_trampoline:
    case mst_file_text:
      return true;
    default:
      return false;
    }
}

/* See symtab.h.  */

bound_minimal_symbol
find_gnu_ifunc (const symbol *sym)
{
  if (sym->aclass () != LOC_BLOCK)
    return {};

  lookup_name_info lookup_name (sym->search_name (),
				symbol_name_match_type::SEARCH_NAME);
  struct objfile *objfile = sym->objfile ();

  CORE_ADDR address = sym->value_block ()->entry_pc ();
  minimal_symbol *ifunc = NULL;

  iterate_over_minimal_symbols (objfile, lookup_name,
				[&] (minimal_symbol *minsym)
    {
      if (minsym->type () == mst_text_gnu_ifunc
	  || minsym->type () == mst_data_gnu_ifunc)
	{
	  CORE_ADDR msym_addr = minsym->value_address (objfile);
	  if (minsym->type () == mst_data_gnu_ifunc)
	    {
	      struct gdbarch *gdbarch = objfile->arch ();
	      msym_addr = gdbarch_convert_from_func_ptr_addr
		(gdbarch, msym_addr, current_inferior ()->top_target ());
	    }
	  if (msym_addr == address)
	    {
	      ifunc = minsym;
	      return true;
	    }
	}
      return false;
    });

  if (ifunc != NULL)
    return {ifunc, objfile};
  return {};
}

/* Add matching symbols from SYMTAB to the current completion list.  */

static void
add_symtab_completions (struct compunit_symtab *cust,
			completion_tracker &tracker,
			complete_symbol_mode mode,
			const lookup_name_info &lookup_name,
			const char *text, const char *word,
			enum type_code code)
{
  int i;

  if (cust == NULL)
    return;

  for (i = GLOBAL_BLOCK; i <= STATIC_BLOCK; i++)
    {
      QUIT;

      const struct block *b = cust->blockvector ()->block (i);
      for (struct symbol *sym : block_iterator_range (b))
	{
	  if (completion_skip_symbol (mode, sym))
	    continue;

	  if (code == TYPE_CODE_UNDEF
	      || (sym->domain () == STRUCT_DOMAIN
		  && sym->type ()->code () == code))
	    completion_list_add_symbol (tracker, sym,
					lookup_name,
					text, word);
	}
    }
}

void
default_collect_symbol_completion_matches_break_on
  (completion_tracker &tracker, complete_symbol_mode mode,
   symbol_name_match_type name_match_type,
   const char *text, const char *word,
   const char *break_on, enum type_code code)
{
  /* Problem: All of the symbols have to be copied because readline
     frees them.  I'm not going to worry about this; hopefully there
     won't be that many.  */

  const struct block *b;
  const struct block *surrounding_static_block, *surrounding_global_block;
  /* The symbol we are completing on.  Points in same buffer as text.  */
  const char *sym_text;

  /* Now look for the symbol we are supposed to complete on.  */
  if (mode == complete_symbol_mode::LINESPEC)
    sym_text = text;
  else
    {
      const char *p;
      char quote_found;
      const char *quote_pos = NULL;

      /* First see if this is a quoted string.  */
      quote_found = '\0';
      for (p = text; *p != '\0'; ++p)
	{
	  if (quote_found != '\0')
	    {
	      if (*p == quote_found)
		/* Found close quote.  */
		quote_found = '\0';
	      else if (*p == '\\' && p[1] == quote_found)
		/* A backslash followed by the quote character
		   doesn't end the string.  */
		++p;
	    }
	  else if (*p == '\'' || *p == '"')
	    {
	      quote_found = *p;
	      quote_pos = p;
	    }
	}
      if (quote_found == '\'')
	/* A string within single quotes can be a symbol, so complete on it.  */
	sym_text = quote_pos + 1;
      else if (quote_found == '"')
	/* A double-quoted string is never a symbol, nor does it make sense
	   to complete it any other way.  */
	{
	  return;
	}
      else
	{
	  /* It is not a quoted string.  Break it based on the characters
	     which are in symbols.  */
	  while (p > text)
	    {
	      if (isalnum (p[-1]) || p[-1] == '_' || p[-1] == '\0'
		  || p[-1] == ':' || strchr (break_on, p[-1]) != NULL)
		--p;
	      else
		break;
	    }
	  sym_text = p;
	}
    }

  lookup_name_info lookup_name (sym_text, name_match_type, true);

  /* At this point scan through the misc symbol vectors and add each
     symbol you find to the list.  Eventually we want to ignore
     anything that isn't a text symbol (everything else will be
     handled by the psymtab code below).  */

  if (code == TYPE_CODE_UNDEF)
    {
      for (objfile *objfile : current_program_space->objfiles ())
	{
	  for (minimal_symbol *msymbol : objfile->msymbols ())
	    {
	      QUIT;

	      if (completion_skip_symbol (mode, msymbol))
		continue;

	      completion_list_add_msymbol (tracker, msymbol, lookup_name,
					   sym_text, word);

	      completion_list_objc_symbol (tracker, msymbol, lookup_name,
					   sym_text, word);
	    }
	}
    }

  /* Add completions for all currently loaded symbol tables.  */
  for (objfile *objfile : current_program_space->objfiles ())
    {
      for (compunit_symtab *cust : objfile->compunits ())
	add_symtab_completions (cust, tracker, mode, lookup_name,
				sym_text, word, code);
    }

  /* Look through the partial symtabs for all symbols which begin by
     matching SYM_TEXT.  Expand all CUs that you find to the list.  */
  expand_symtabs_matching (NULL,
			   lookup_name,
			   NULL,
			   [&] (compunit_symtab *symtab) /* expansion notify */
			     {
			       add_symtab_completions (symtab,
						       tracker, mode, lookup_name,
						       sym_text, word, code);
			       return true;
			     },
			   SEARCH_GLOBAL_BLOCK | SEARCH_STATIC_BLOCK,
			   ALL_DOMAIN);

  /* Search upwards from currently selected frame (so that we can
     complete on local vars).  Also catch fields of types defined in
     this places which match our text string.  Only complete on types
     visible from current context.  */

  b = get_selected_block (0);
  surrounding_static_block = b == nullptr ? nullptr : b->static_block ();
  surrounding_global_block = b == nullptr ? nullptr : b->global_block ();
  if (surrounding_static_block != NULL)
    while (b != surrounding_static_block)
      {
	QUIT;

	for (struct symbol *sym : block_iterator_range (b))
	  {
	    if (code == TYPE_CODE_UNDEF)
	      {
		completion_list_add_symbol (tracker, sym, lookup_name,
					    sym_text, word);
		completion_list_add_fields (tracker, sym, lookup_name,
					    sym_text, word);
	      }
	    else if (sym->domain () == STRUCT_DOMAIN
		     && sym->type ()->code () == code)
	      completion_list_add_symbol (tracker, sym, lookup_name,
					  sym_text, word);
	  }

	/* Stop when we encounter an enclosing function.  Do not stop for
	   non-inlined functions - the locals of the enclosing function
	   are in scope for a nested function.  */
	if (b->function () != NULL && b->inlined_p ())
	  break;
	b = b->superblock ();
      }

  /* Add fields from the file's types; symbols will be added below.  */

  if (code == TYPE_CODE_UNDEF)
    {
      if (surrounding_static_block != NULL)
	for (struct symbol *sym : block_iterator_range (surrounding_static_block))
	  completion_list_add_fields (tracker, sym, lookup_name,
				      sym_text, word);

      if (surrounding_global_block != NULL)
	for (struct symbol *sym : block_iterator_range (surrounding_global_block))
	  completion_list_add_fields (tracker, sym, lookup_name,
				      sym_text, word);
    }

  /* Skip macros if we are completing a struct tag -- arguable but
     usually what is expected.  */
  if (current_language->macro_expansion () == macro_expansion_c
      && code == TYPE_CODE_UNDEF)
    {
      gdb::unique_xmalloc_ptr<struct macro_scope> scope;

      /* This adds a macro's name to the current completion list.  */
      auto add_macro_name = [&] (const char *macro_name,
				 const macro_definition *,
				 macro_source_file *,
				 int)
	{
	  completion_list_add_name (tracker, language_c, macro_name,
				    lookup_name, sym_text, word);
	};

      /* Add any macros visible in the default scope.  Note that this
	 may yield the occasional wrong result, because an expression
	 might be evaluated in a scope other than the default.  For
	 example, if the user types "break file:line if <TAB>", the
	 resulting expression will be evaluated at "file:line" -- but
	 at there does not seem to be a way to detect this at
	 completion time.  */
      scope = default_macro_scope ();
      if (scope)
	macro_for_each_in_scope (scope->file, scope->line,
				 add_macro_name);

      /* User-defined macros are always visible.  */
      macro_for_each (macro_user_macros, add_macro_name);
    }
}

/* Collect all symbols (regardless of class) which begin by matching
   TEXT.  */

void
collect_symbol_completion_matches (completion_tracker &tracker,
				   complete_symbol_mode mode,
				   symbol_name_match_type name_match_type,
				   const char *text, const char *word)
{
  current_language->collect_symbol_completion_matches (tracker, mode,
						       name_match_type,
						       text, word,
						       TYPE_CODE_UNDEF);
}

/* Like collect_symbol_completion_matches, but only collect
   STRUCT_DOMAIN symbols whose type code is CODE.  */

void
collect_symbol_completion_matches_type (completion_tracker &tracker,
					const char *text, const char *word,
					enum type_code code)
{
  complete_symbol_mode mode = complete_symbol_mode::EXPRESSION;
  symbol_name_match_type name_match_type = symbol_name_match_type::EXPRESSION;

  gdb_assert (code == TYPE_CODE_UNION
	      || code == TYPE_CODE_STRUCT
	      || code == TYPE_CODE_ENUM);
  current_language->collect_symbol_completion_matches (tracker, mode,
						       name_match_type,
						       text, word, code);
}

/* Like collect_symbol_completion_matches, but collects a list of
   symbols defined in all source files named SRCFILE.  */

void
collect_file_symbol_completion_matches (completion_tracker &tracker,
					complete_symbol_mode mode,
					symbol_name_match_type name_match_type,
					const char *text, const char *word,
					const char *srcfile)
{
  /* The symbol we are completing on.  Points in same buffer as text.  */
  const char *sym_text;

  /* Now look for the symbol we are supposed to complete on.
     FIXME: This should be language-specific.  */
  if (mode == complete_symbol_mode::LINESPEC)
    sym_text = text;
  else
    {
      const char *p;
      char quote_found;
      const char *quote_pos = NULL;

      /* First see if this is a quoted string.  */
      quote_found = '\0';
      for (p = text; *p != '\0'; ++p)
	{
	  if (quote_found != '\0')
	    {
	      if (*p == quote_found)
		/* Found close quote.  */
		quote_found = '\0';
	      else if (*p == '\\' && p[1] == quote_found)
		/* A backslash followed by the quote character
		   doesn't end the string.  */
		++p;
	    }
	  else if (*p == '\'' || *p == '"')
	    {
	      quote_found = *p;
	      quote_pos = p;
	    }
	}
      if (quote_found == '\'')
	/* A string within single quotes can be a symbol, so complete on it.  */
	sym_text = quote_pos + 1;
      else if (quote_found == '"')
	/* A double-quoted string is never a symbol, nor does it make sense
	   to complete it any other way.  */
	{
	  return;
	}
      else
	{
	  /* Not a quoted string.  */
	  sym_text = language_search_unquoted_string (text, p);
	}
    }

  lookup_name_info lookup_name (sym_text, name_match_type, true);

  /* Go through symtabs for SRCFILE and check the externs and statics
     for symbols which match.  */
  iterate_over_symtabs (srcfile, [&] (symtab *s)
    {
      add_symtab_completions (s->compunit (),
			      tracker, mode, lookup_name,
			      sym_text, word, TYPE_CODE_UNDEF);
      return false;
    });
}

/* A helper function for make_source_files_completion_list.  It adds
   another file name to a list of possible completions, growing the
   list as necessary.  */

static void
add_filename_to_list (const char *fname, const char *text, const char *word,
		      completion_list *list)
{
  list->emplace_back (make_completion_match_str (fname, text, word));
}

static int
not_interesting_fname (const char *fname)
{
  static const char *illegal_aliens[] = {
    "_globals_",	/* inserted by coff_symtab_read */
    NULL
  };
  int i;

  for (i = 0; illegal_aliens[i]; i++)
    {
      if (filename_cmp (fname, illegal_aliens[i]) == 0)
	return 1;
    }
  return 0;
}

/* An object of this type is passed as the callback argument to
   map_partial_symbol_filenames.  */
struct add_partial_filename_data
{
  struct filename_seen_cache *filename_seen_cache;
  const char *text;
  const char *word;
  int text_len;
  completion_list *list;

  void operator() (const char *filename, const char *fullname);
};

/* A callback for map_partial_symbol_filenames.  */

void
add_partial_filename_data::operator() (const char *filename,
				       const char *fullname)
{
  if (not_interesting_fname (filename))
    return;
  if (!filename_seen_cache->seen (filename)
      && filename_ncmp (filename, text, text_len) == 0)
    {
      /* This file matches for a completion; add it to the
	 current list of matches.  */
      add_filename_to_list (filename, text, word, list);
    }
  else
    {
      const char *base_name = lbasename (filename);

      if (base_name != filename
	  && !filename_seen_cache->seen (base_name)
	  && filename_ncmp (base_name, text, text_len) == 0)
	add_filename_to_list (base_name, text, word, list);
    }
}

/* Return a list of all source files whose names begin with matching
   TEXT.  The file names are looked up in the symbol tables of this
   program.  */

completion_list
make_source_files_completion_list (const char *text, const char *word)
{
  size_t text_len = strlen (text);
  completion_list list;
  const char *base_name;
  struct add_partial_filename_data datum;

  if (!have_full_symbols () && !have_partial_symbols ())
    return list;

  filename_seen_cache filenames_seen;

  for (objfile *objfile : current_program_space->objfiles ())
    {
      for (compunit_symtab *cu : objfile->compunits ())
	{
	  for (symtab *s : cu->filetabs ())
	    {
	      if (not_interesting_fname (s->filename))
		continue;
	      if (!filenames_seen.seen (s->filename)
		  && filename_ncmp (s->filename, text, text_len) == 0)
		{
		  /* This file matches for a completion; add it to the current
		     list of matches.  */
		  add_filename_to_list (s->filename, text, word, &list);
		}
	      else
		{
		  /* NOTE: We allow the user to type a base name when the
		     debug info records leading directories, but not the other
		     way around.  This is what subroutines of breakpoint
		     command do when they parse file names.  */
		  base_name = lbasename (s->filename);
		  if (base_name != s->filename
		      && !filenames_seen.seen (base_name)
		      && filename_ncmp (base_name, text, text_len) == 0)
		    add_filename_to_list (base_name, text, word, &list);
		}
	    }
	}
    }

  datum.filename_seen_cache = &filenames_seen;
  datum.text = text;
  datum.word = word;
  datum.text_len = text_len;
  datum.list = &list;
  map_symbol_filenames (datum, false /*need_fullname*/);

  return list;
}

/* Track MAIN */

/* Return the "main_info" object for the current program space.  If
   the object has not yet been created, create it and fill in some
   default values.  */

static main_info *
get_main_info (program_space *pspace)
{
  main_info *info = main_progspace_key.get (pspace);

  if (info == NULL)
    {
      /* It may seem strange to store the main name in the progspace
	 and also in whatever objfile happens to see a main name in
	 its debug info.  The reason for this is mainly historical:
	 gdb returned "main" as the name even if no function named
	 "main" was defined the program; and this approach lets us
	 keep compatibility.  */
      info = main_progspace_key.emplace (pspace);
    }

  return info;
}

static void
set_main_name (program_space *pspace, const char *name, enum language lang)
{
  main_info *info = get_main_info (pspace);

  if (!info->name_of_main.empty ())
    {
      info->name_of_main.clear ();
      info->language_of_main = language_unknown;
    }
  if (name != NULL)
    {
      info->name_of_main = name;
      info->language_of_main = lang;
    }
}

/* Deduce the name of the main procedure, and set NAME_OF_MAIN
   accordingly.  */

static void
find_main_name (void)
{
  const char *new_main_name;
  program_space *pspace = current_program_space;

  /* First check the objfiles to see whether a debuginfo reader has
     picked up the appropriate main name.  Historically the main name
     was found in a more or less random way; this approach instead
     relies on the order of objfile creation -- which still isn't
     guaranteed to get the correct answer, but is just probably more
     accurate.  */
  for (objfile *objfile : current_program_space->objfiles ())
    {
      objfile->compute_main_name ();

      if (objfile->per_bfd->name_of_main != NULL)
	{
	  set_main_name (pspace,
			 objfile->per_bfd->name_of_main,
			 objfile->per_bfd->language_of_main);
	  return;
	}
    }

  /* Try to see if the main procedure is in Ada.  */
  /* FIXME: brobecker/2005-03-07: Another way of doing this would
     be to add a new method in the language vector, and call this
     method for each language until one of them returns a non-empty
     name.  This would allow us to remove this hard-coded call to
     an Ada function.  It is not clear that this is a better approach
     at this point, because all methods need to be written in a way
     such that false positives never be returned.  For instance, it is
     important that a method does not return a wrong name for the main
     procedure if the main procedure is actually written in a different
     language.  It is easy to guaranty this with Ada, since we use a
     special symbol generated only when the main in Ada to find the name
     of the main procedure.  It is difficult however to see how this can
     be guarantied for languages such as C, for instance.  This suggests
     that order of call for these methods becomes important, which means
     a more complicated approach.  */
  new_main_name = ada_main_name ();
  if (new_main_name != NULL)
    {
      set_main_name (pspace, new_main_name, language_ada);
      return;
    }

  new_main_name = d_main_name ();
  if (new_main_name != NULL)
    {
      set_main_name (pspace, new_main_name, language_d);
      return;
    }

  new_main_name = go_main_name ();
  if (new_main_name != NULL)
    {
      set_main_name (pspace, new_main_name, language_go);
      return;
    }

  new_main_name = pascal_main_name ();
  if (new_main_name != NULL)
    {
      set_main_name (pspace, new_main_name, language_pascal);
      return;
    }

  /* The languages above didn't identify the name of the main procedure.
     Fallback to "main".  */

  /* Try to find language for main in psymtabs.  */
  bool symbol_found_p = false;
  gdbarch_iterate_over_objfiles_in_search_order
    (current_inferior ()->arch (),
     [&symbol_found_p, pspace] (objfile *obj)
       {
	 language lang
	   = obj->lookup_global_symbol_language ("main", VAR_DOMAIN,
						 &symbol_found_p);
	 if (symbol_found_p)
	   {
	     set_main_name (pspace, "main", lang);
	     return 1;
	   }

	 return 0;
       }, nullptr);

  if (symbol_found_p)
    return;

  set_main_name (pspace, "main", language_unknown);
}

/* See symtab.h.  */

const char *
main_name ()
{
  main_info *info = get_main_info (current_program_space);

  if (info->name_of_main.empty ())
    find_main_name ();

  return info->name_of_main.c_str ();
}

/* Return the language of the main function.  If it is not known,
   return language_unknown.  */

enum language
main_language (void)
{
  main_info *info = get_main_info (current_program_space);

  if (info->name_of_main.empty ())
    find_main_name ();

  return info->language_of_main;
}

/* Return 1 if the supplied producer string matches the ARM RealView
   compiler (armcc).  */

bool
producer_is_realview (const char *producer)
{
  static const char *const arm_idents[] = {
    "ARM C Compiler, ADS",
    "Thumb C Compiler, ADS",
    "ARM C++ Compiler, ADS",
    "Thumb C++ Compiler, ADS",
    "ARM/Thumb C/C++ Compiler, RVCT",
    "ARM C/C++ Compiler, RVCT"
  };

  if (producer == NULL)
    return false;

  for (const char *ident : arm_idents)
    if (startswith (producer, ident))
      return true;

  return false;
}



/* The next index to hand out in response to a registration request.  */

static int next_aclass_value = LOC_FINAL_VALUE;

/* The maximum number of "aclass" registrations we support.  This is
   constant for convenience.  */
#define MAX_SYMBOL_IMPLS (LOC_FINAL_VALUE + 11)

/* The objects representing the various "aclass" values.  The elements
   from 0 up to LOC_FINAL_VALUE-1 represent themselves, and subsequent
   elements are those registered at gdb initialization time.  */

static struct symbol_impl symbol_impl[MAX_SYMBOL_IMPLS];

/* The globally visible pointer.  This is separate from 'symbol_impl'
   so that it can be const.  */

gdb::array_view<const struct symbol_impl> symbol_impls (symbol_impl);

/* Make sure we saved enough room in struct symbol.  */

static_assert (MAX_SYMBOL_IMPLS <= (1 << SYMBOL_ACLASS_BITS));

/* Register a computed symbol type.  ACLASS must be LOC_COMPUTED.  OPS
   is the ops vector associated with this index.  This returns the new
   index, which should be used as the aclass_index field for symbols
   of this type.  */

int
register_symbol_computed_impl (enum address_class aclass,
			       const struct symbol_computed_ops *ops)
{
  int result = next_aclass_value++;

  gdb_assert (aclass == LOC_COMPUTED);
  gdb_assert (result < MAX_SYMBOL_IMPLS);
  symbol_impl[result].aclass = aclass;
  symbol_impl[result].ops_computed = ops;

  /* Sanity check OPS.  */
  gdb_assert (ops != NULL);
  gdb_assert (ops->tracepoint_var_ref != NULL);
  gdb_assert (ops->describe_location != NULL);
  gdb_assert (ops->get_symbol_read_needs != NULL);
  gdb_assert (ops->read_variable != NULL);

  return result;
}

/* Register a function with frame base type.  ACLASS must be LOC_BLOCK.
   OPS is the ops vector associated with this index.  This returns the
   new index, which should be used as the aclass_index field for symbols
   of this type.  */

int
register_symbol_block_impl (enum address_class aclass,
			    const struct symbol_block_ops *ops)
{
  int result = next_aclass_value++;

  gdb_assert (aclass == LOC_BLOCK);
  gdb_assert (result < MAX_SYMBOL_IMPLS);
  symbol_impl[result].aclass = aclass;
  symbol_impl[result].ops_block = ops;

  /* Sanity check OPS.  */
  gdb_assert (ops != NULL);
  gdb_assert (ops->find_frame_base_location != nullptr
	      || ops->get_block_value != nullptr);

  return result;
}

/* Register a register symbol type.  ACLASS must be LOC_REGISTER or
   LOC_REGPARM_ADDR.  OPS is the register ops vector associated with
   this index.  This returns the new index, which should be used as
   the aclass_index field for symbols of this type.  */

int
register_symbol_register_impl (enum address_class aclass,
			       const struct symbol_register_ops *ops)
{
  int result = next_aclass_value++;

  gdb_assert (aclass == LOC_REGISTER || aclass == LOC_REGPARM_ADDR);
  gdb_assert (result < MAX_SYMBOL_IMPLS);
  symbol_impl[result].aclass = aclass;
  symbol_impl[result].ops_register = ops;

  return result;
}

/* Initialize elements of 'symbol_impl' for the constants in enum
   address_class.  */

static void
initialize_ordinary_address_classes (void)
{
  int i;

  for (i = 0; i < LOC_FINAL_VALUE; ++i)
    symbol_impl[i].aclass = (enum address_class) i;
}



/* See symtab.h.  */

struct objfile *
symbol::objfile () const
{
  gdb_assert (is_objfile_owned ());
  return owner.symtab->compunit ()->objfile ();
}

/* See symtab.h.  */

struct gdbarch *
symbol::arch () const
{
  if (!is_objfile_owned ())
    return owner.arch;
  return owner.symtab->compunit ()->objfile ()->arch ();
}

/* See symtab.h.  */

struct symtab *
symbol::symtab () const
{
  gdb_assert (is_objfile_owned ());
  return owner.symtab;
}

/* See symtab.h.  */

void
symbol::set_symtab (struct symtab *symtab)
{
  gdb_assert (is_objfile_owned ());
  owner.symtab = symtab;
}

/* See symtab.h.  */

CORE_ADDR
symbol::get_maybe_copied_address () const
{
  gdb_assert (this->maybe_copied);
  gdb_assert (this->aclass () == LOC_STATIC);

  const char *linkage_name = this->linkage_name ();
  bound_minimal_symbol minsym = lookup_minimal_symbol_linkage (linkage_name,
							       false);
  if (minsym.minsym != nullptr)
    return minsym.value_address ();
  return this->m_value.address;
}

/* See symtab.h.  */

CORE_ADDR
minimal_symbol::get_maybe_copied_address (objfile *objf) const
{
  gdb_assert (this->maybe_copied (objf));
  gdb_assert ((objf->flags & OBJF_MAINLINE) == 0);

  const char *linkage_name = this->linkage_name ();
  bound_minimal_symbol found = lookup_minimal_symbol_linkage (linkage_name,
							      true);
  if (found.minsym != nullptr)
    return found.value_address ();
  return (this->m_value.address
	  + objf->section_offsets[this->section_index ()]);
}



/* Hold the sub-commands of 'info module'.  */

static struct cmd_list_element *info_module_cmdlist = NULL;

/* See symtab.h.  */

std::vector<module_symbol_search>
search_module_symbols (const char *module_regexp, const char *regexp,
		       const char *type_regexp, search_domain kind)
{
  std::vector<module_symbol_search> results;

  /* Search for all modules matching MODULE_REGEXP.  */
  global_symbol_searcher spec1 (MODULES_DOMAIN, module_regexp);
  spec1.set_exclude_minsyms (true);
  std::vector<symbol_search> modules = spec1.search ();

  /* Now search for all symbols of the required KIND matching the required
     regular expressions.  We figure out which ones are in which modules
     below.  */
  global_symbol_searcher spec2 (kind, regexp);
  spec2.set_symbol_type_regexp (type_regexp);
  spec2.set_exclude_minsyms (true);
  std::vector<symbol_search> symbols = spec2.search ();

  /* Now iterate over all MODULES, checking to see which items from
     SYMBOLS are in each module.  */
  for (const symbol_search &p : modules)
    {
      QUIT;

      /* This is a module.  */
      gdb_assert (p.symbol != nullptr);

      std::string prefix = p.symbol->print_name ();
      prefix += "::";

      for (const symbol_search &q : symbols)
	{
	  if (q.symbol == nullptr)
	    continue;

	  if (strncmp (q.symbol->print_name (), prefix.c_str (),
		       prefix.size ()) != 0)
	    continue;

	  results.push_back ({p, q});
	}
    }

  return results;
}

/* Implement the core of both 'info module functions' and 'info module
   variables'.  */

static void
info_module_subcommand (bool quiet, const char *module_regexp,
			const char *regexp, const char *type_regexp,
			search_domain kind)
{
  /* Print a header line.  Don't build the header line bit by bit as this
     prevents internationalisation.  */
  if (!quiet)
    {
      if (module_regexp == nullptr)
	{
	  if (type_regexp == nullptr)
	    {
	      if (regexp == nullptr)
		gdb_printf ((kind == VARIABLES_DOMAIN
			     ? _("All variables in all modules:")
			     : _("All functions in all modules:")));
	      else
		gdb_printf
		  ((kind == VARIABLES_DOMAIN
		    ? _("All variables matching regular expression"
			" \"%s\" in all modules:")
		    : _("All functions matching regular expression"
			" \"%s\" in all modules:")),
		   regexp);
	    }
	  else
	    {
	      if (regexp == nullptr)
		gdb_printf
		  ((kind == VARIABLES_DOMAIN
		    ? _("All variables with type matching regular "
			"expression \"%s\" in all modules:")
		    : _("All functions with type matching regular "
			"expression \"%s\" in all modules:")),
		   type_regexp);
	      else
		gdb_printf
		  ((kind == VARIABLES_DOMAIN
		    ? _("All variables matching regular expression "
			"\"%s\",\n\twith type matching regular "
			"expression \"%s\" in all modules:")
		    : _("All functions matching regular expression "
			"\"%s\",\n\twith type matching regular "
			"expression \"%s\" in all modules:")),
		   regexp, type_regexp);
	    }
	}
      else
	{
	  if (type_regexp == nullptr)
	    {
	      if (regexp == nullptr)
		gdb_printf
		  ((kind == VARIABLES_DOMAIN
		    ? _("All variables in all modules matching regular "
			"expression \"%s\":")
		    : _("All functions in all modules matching regular "
			"expression \"%s\":")),
		   module_regexp);
	      else
		gdb_printf
		  ((kind == VARIABLES_DOMAIN
		    ? _("All variables matching regular expression "
			"\"%s\",\n\tin all modules matching regular "
			"expression \"%s\":")
		    : _("All functions matching regular expression "
			"\"%s\",\n\tin all modules matching regular "
			"expression \"%s\":")),
		   regexp, module_regexp);
	    }
	  else
	    {
	      if (regexp == nullptr)
		gdb_printf
		  ((kind == VARIABLES_DOMAIN
		    ? _("All variables with type matching regular "
			"expression \"%s\"\n\tin all modules matching "
			"regular expression \"%s\":")
		    : _("All functions with type matching regular "
			"expression \"%s\"\n\tin all modules matching "
			"regular expression \"%s\":")),
		   type_regexp, module_regexp);
	      else
		gdb_printf
		  ((kind == VARIABLES_DOMAIN
		    ? _("All variables matching regular expression "
			"\"%s\",\n\twith type matching regular expression "
			"\"%s\",\n\tin all modules matching regular "
			"expression \"%s\":")
		    : _("All functions matching regular expression "
			"\"%s\",\n\twith type matching regular expression "
			"\"%s\",\n\tin all modules matching regular "
			"expression \"%s\":")),
		   regexp, type_regexp, module_regexp);
	    }
	}
      gdb_printf ("\n");
    }

  /* Find all symbols of type KIND matching the given regular expressions
     along with the symbols for the modules in which those symbols
     reside.  */
  std::vector<module_symbol_search> module_symbols
    = search_module_symbols (module_regexp, regexp, type_regexp, kind);

  std::sort (module_symbols.begin (), module_symbols.end (),
	     [] (const module_symbol_search &a, const module_symbol_search &b)
	     {
	       if (a.first < b.first)
		 return true;
	       else if (a.first == b.first)
		 return a.second < b.second;
	       else
		 return false;
	     });

  const char *last_filename = "";
  const symbol *last_module_symbol = nullptr;
  for (const module_symbol_search &ms : module_symbols)
    {
      const symbol_search &p = ms.first;
      const symbol_search &q = ms.second;

      gdb_assert (q.symbol != nullptr);

      if (last_module_symbol != p.symbol)
	{
	  gdb_printf ("\n");
	  gdb_printf (_("Module \"%s\":\n"), p.symbol->print_name ());
	  last_module_symbol = p.symbol;
	  last_filename = "";
	}

      print_symbol_info (FUNCTIONS_DOMAIN, q.symbol, q.block,
			 last_filename);
      last_filename
	= symtab_to_filename_for_display (q.symbol->symtab ());
    }
}

/* Hold the option values for the 'info module .....' sub-commands.  */

struct info_modules_var_func_options
{
  bool quiet = false;
  std::string type_regexp;
  std::string module_regexp;
};

/* The options used by 'info module variables' and 'info module functions'
   commands.  */

static const gdb::option::option_def info_modules_var_func_options_defs [] = {
  gdb::option::boolean_option_def<info_modules_var_func_options> {
    "q",
    [] (info_modules_var_func_options *opt) { return &opt->quiet; },
    nullptr, /* show_cmd_cb */
    nullptr /* set_doc */
  },

  gdb::option::string_option_def<info_modules_var_func_options> {
    "t",
    [] (info_modules_var_func_options *opt) { return &opt->type_regexp; },
    nullptr, /* show_cmd_cb */
    nullptr /* set_doc */
  },

  gdb::option::string_option_def<info_modules_var_func_options> {
    "m",
    [] (info_modules_var_func_options *opt) { return &opt->module_regexp; },
    nullptr, /* show_cmd_cb */
    nullptr /* set_doc */
  }
};

/* Return the option group used by the 'info module ...' sub-commands.  */

static inline gdb::option::option_def_group
make_info_modules_var_func_options_def_group
	(info_modules_var_func_options *opts)
{
  return {{info_modules_var_func_options_defs}, opts};
}

/* Implements the 'info module functions' command.  */

static void
info_module_functions_command (const char *args, int from_tty)
{
  info_modules_var_func_options opts;
  auto grp = make_info_modules_var_func_options_def_group (&opts);
  gdb::option::process_options
    (&args, gdb::option::PROCESS_OPTIONS_UNKNOWN_IS_OPERAND, grp);
  if (args != nullptr && *args == '\0')
    args = nullptr;

  info_module_subcommand
    (opts.quiet,
     opts.module_regexp.empty () ? nullptr : opts.module_regexp.c_str (), args,
     opts.type_regexp.empty () ? nullptr : opts.type_regexp.c_str (),
     FUNCTIONS_DOMAIN);
}

/* Implements the 'info module variables' command.  */

static void
info_module_variables_command (const char *args, int from_tty)
{
  info_modules_var_func_options opts;
  auto grp = make_info_modules_var_func_options_def_group (&opts);
  gdb::option::process_options
    (&args, gdb::option::PROCESS_OPTIONS_UNKNOWN_IS_OPERAND, grp);
  if (args != nullptr && *args == '\0')
    args = nullptr;

  info_module_subcommand
    (opts.quiet,
     opts.module_regexp.empty () ? nullptr : opts.module_regexp.c_str (), args,
     opts.type_regexp.empty () ? nullptr : opts.type_regexp.c_str (),
     VARIABLES_DOMAIN);
}

/* Command completer for 'info module ...' sub-commands.  */

static void
info_module_var_func_command_completer (struct cmd_list_element *ignore,
					completion_tracker &tracker,
					const char *text,
					const char * /* word */)
{

  const auto group = make_info_modules_var_func_options_def_group (nullptr);
  if (gdb::option::complete_options
      (tracker, &text, gdb::option::PROCESS_OPTIONS_UNKNOWN_IS_OPERAND, group))
    return;

  const char *word = advance_to_expression_complete_word_point (tracker, text);
  symbol_completer (ignore, tracker, text, word);
}



void _initialize_symtab ();
void
_initialize_symtab ()
{
  cmd_list_element *c;

  initialize_ordinary_address_classes ();

  c = add_info ("variables", info_variables_command,
		info_print_args_help (_("\
All global and static variable names or those matching REGEXPs.\n\
Usage: info variables [-q] [-n] [-t TYPEREGEXP] [NAMEREGEXP]\n\
Prints the global and static variables.\n"),
				      _("global and static variables"),
				      true));
  set_cmd_completer_handle_brkchars (c, info_vars_funcs_command_completer);

  c = add_info ("functions", info_functions_command,
		info_print_args_help (_("\
All function names or those matching REGEXPs.\n\
Usage: info functions [-q] [-n] [-t TYPEREGEXP] [NAMEREGEXP]\n\
Prints the functions.\n"),
				      _("functions"),
				      true));
  set_cmd_completer_handle_brkchars (c, info_vars_funcs_command_completer);

  c = add_info ("types", info_types_command, _("\
All type names, or those matching REGEXP.\n\
Usage: info types [-q] [REGEXP]\n\
Print information about all types matching REGEXP, or all types if no\n\
REGEXP is given.  The optional flag -q disables printing of headers."));
  set_cmd_completer_handle_brkchars (c, info_types_command_completer);

  const auto info_sources_opts
    = make_info_sources_options_def_group (nullptr);

  static std::string info_sources_help
    = gdb::option::build_help (_("\
All source files in the program or those matching REGEXP.\n\
Usage: info sources [OPTION]... [REGEXP]\n\
By default, REGEXP is used to match anywhere in the filename.\n\
\n\
Options:\n\
%OPTIONS%"),
			       info_sources_opts);

  c = add_info ("sources", info_sources_command, info_sources_help.c_str ());
  set_cmd_completer_handle_brkchars (c, info_sources_command_completer);

  c = add_info ("modules", info_modules_command,
		_("All module names, or those matching REGEXP."));
  set_cmd_completer_handle_brkchars (c, info_types_command_completer);

  add_info ("main", info_main_command,
	    _("Get main symbol to identify entry point into program."));

  add_basic_prefix_cmd ("module", class_info, _("\
Print information about modules."),
			&info_module_cmdlist, 0, &infolist);

  c = add_cmd ("functions", class_info, info_module_functions_command, _("\
Display functions arranged by modules.\n\
Usage: info module functions [-q] [-m MODREGEXP] [-t TYPEREGEXP] [REGEXP]\n\
Print a summary of all functions within each Fortran module, grouped by\n\
module and file.  For each function the line on which the function is\n\
defined is given along with the type signature and name of the function.\n\
\n\
If REGEXP is provided then only functions whose name matches REGEXP are\n\
listed.  If MODREGEXP is provided then only functions in modules matching\n\
MODREGEXP are listed.  If TYPEREGEXP is given then only functions whose\n\
type signature matches TYPEREGEXP are listed.\n\
\n\
The -q flag suppresses printing some header information."),
	       &info_module_cmdlist);
  set_cmd_completer_handle_brkchars
    (c, info_module_var_func_command_completer);

  c = add_cmd ("variables", class_info, info_module_variables_command, _("\
Display variables arranged by modules.\n\
Usage: info module variables [-q] [-m MODREGEXP] [-t TYPEREGEXP] [REGEXP]\n\
Print a summary of all variables within each Fortran module, grouped by\n\
module and file.  For each variable the line on which the variable is\n\
defined is given along with the type and name of the variable.\n\
\n\
If REGEXP is provided then only variables whose name matches REGEXP are\n\
listed.  If MODREGEXP is provided then only variables in modules matching\n\
MODREGEXP are listed.  If TYPEREGEXP is given then only variables whose\n\
type matches TYPEREGEXP are listed.\n\
\n\
The -q flag suppresses printing some header information."),
	       &info_module_cmdlist);
  set_cmd_completer_handle_brkchars
    (c, info_module_var_func_command_completer);

  add_com ("rbreak", class_breakpoint, rbreak_command,
	   _("Set a breakpoint for all functions matching REGEXP."));

  add_setshow_enum_cmd ("multiple-symbols", no_class,
			multiple_symbols_modes, &multiple_symbols_mode,
			_("\
Set how the debugger handles ambiguities in expressions."), _("\
Show how the debugger handles ambiguities in expressions."), _("\
Valid values are \"ask\", \"all\", \"cancel\", and the default is \"all\"."),
			NULL, NULL, &setlist, &showlist);

  add_setshow_boolean_cmd ("basenames-may-differ", class_obscure,
			   &basenames_may_differ, _("\
Set whether a source file may have multiple base names."), _("\
Show whether a source file may have multiple base names."), _("\
(A \"base name\" is the name of a file with the directory part removed.\n\
Example: The base name of \"/home/user/hello.c\" is \"hello.c\".)\n\
If set, GDB will canonicalize file names (e.g., expand symlinks)\n\
before comparing them.  Canonicalization is an expensive operation,\n\
but it allows the same file be known by more than one base name.\n\
If not set (the default), all source files are assumed to have just\n\
one base name, and gdb will do file name comparisons more efficiently."),
			   NULL, NULL,
			   &setlist, &showlist);

  add_setshow_zuinteger_cmd ("symtab-create", no_class, &symtab_create_debug,
			     _("Set debugging of symbol table creation."),
			     _("Show debugging of symbol table creation."), _("\
When enabled (non-zero), debugging messages are printed when building\n\
symbol tables.  A value of 1 (one) normally provides enough information.\n\
A value greater than 1 provides more verbose information."),
			     NULL,
			     NULL,
			     &setdebuglist, &showdebuglist);

  add_setshow_zuinteger_cmd ("symbol-lookup", no_class, &symbol_lookup_debug,
			   _("\
Set debugging of symbol lookup."), _("\
Show debugging of symbol lookup."), _("\
When enabled (non-zero), symbol lookups are logged."),
			   NULL, NULL,
			   &setdebuglist, &showdebuglist);

  add_setshow_zuinteger_cmd ("symbol-cache-size", no_class,
			     &new_symbol_cache_size,
			     _("Set the size of the symbol cache."),
			     _("Show the size of the symbol cache."), _("\
The size of the symbol cache.\n\
If zero then the symbol cache is disabled."),
			     set_symbol_cache_size_handler, NULL,
			     &maintenance_set_cmdlist,
			     &maintenance_show_cmdlist);

  add_setshow_boolean_cmd ("ignore-prologue-end-flag", no_class,
			   &ignore_prologue_end_flag,
			   _("Set if the PROLOGUE-END flag is ignored."),
			   _("Show if the PROLOGUE-END flag is ignored."),
			   _("\
The PROLOGUE-END flag from the line-table entries is used to place \
breakpoints past the prologue of functions.  Disabling its use forces \
the use of prologue scanners."),
			   nullptr, nullptr,
			   &maintenance_set_cmdlist,
			   &maintenance_show_cmdlist);


  add_cmd ("symbol-cache", class_maintenance, maintenance_print_symbol_cache,
	   _("Dump the symbol cache for each program space."),
	   &maintenanceprintlist);

  add_cmd ("symbol-cache-statistics", class_maintenance,
	   maintenance_print_symbol_cache_statistics,
	   _("Print symbol cache statistics for each program space."),
	   &maintenanceprintlist);

  cmd_list_element *maintenance_flush_symbol_cache_cmd
    = add_cmd ("symbol-cache", class_maintenance,
	       maintenance_flush_symbol_cache,
	       _("Flush the symbol cache for each program space."),
	       &maintenanceflushlist);
  c = add_alias_cmd ("flush-symbol-cache", maintenance_flush_symbol_cache_cmd,
		     class_maintenance, 0, &maintenancelist);
  deprecate_cmd (c, "maintenancelist flush symbol-cache");

  gdb::observers::new_objfile.attach (symtab_new_objfile_observer, "symtab");
  gdb::observers::all_objfiles_removed.attach (symtab_all_objfiles_removed,
					       "symtab");
  gdb::observers::free_objfile.attach (symtab_free_objfile_observer, "symtab");
}
