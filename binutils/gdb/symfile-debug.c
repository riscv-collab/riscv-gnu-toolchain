/* Debug logging for the symbol file functions for the GNU debugger, GDB.

   Copyright (C) 2013-2024 Free Software Foundation, Inc.

   Contributed by Cygnus Support, using pieces from other GDB modules.

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

/* Note: Be careful with functions that can throw errors.
   We want to see a logging message regardless of whether an error was thrown.
   This typically means printing a message before calling the real function
   and then if the function returns a result printing a message after it
   returns.  */

#include "defs.h"
#include "gdbcmd.h"
#include "objfiles.h"
#include "observable.h"
#include "source.h"
#include "symtab.h"
#include "symfile.h"
#include "block.h"
#include "filenames.h"
#include "cli/cli-style.h"
#include "build-id.h"
#include "debuginfod-support.h"

/* We need to save a pointer to the real symbol functions.
   Plus, the debug versions are malloc'd because we have to NULL out the
   ones that are NULL in the real copy.  */

struct debug_sym_fns_data
{
  const struct sym_fns *real_sf = nullptr;
  struct sym_fns debug_sf {};
};

/* We need to record a pointer to the real set of functions for each
   objfile.  */
static const registry<objfile>::key<debug_sym_fns_data>
  symfile_debug_objfile_data_key;

/* If true all calls to the symfile functions are logged.  */
static bool debug_symfile = false;

/* Return non-zero if symfile debug logging is installed.  */

static int
symfile_debug_installed (struct objfile *objfile)
{
  return (objfile->sf != NULL
	  && symfile_debug_objfile_data_key.get (objfile) != NULL);
}

/* Utility return the name to print for SYMTAB.  */

static const char *
debug_symtab_name (struct symtab *symtab)
{
  return symtab_to_filename_for_display (symtab);
}


/* See objfiles.h.  */

bool
objfile::has_partial_symbols ()
{
  bool retval = false;

  /* If we have not read psymbols, but we have a function capable of reading
     them, then that is an indication that they are in fact available.  Without
     this function the symbols may have been already read in but they also may
     not be present in this objfile.  */
  for (const auto &iter : qf)
    {
      retval = iter->has_symbols (this);
      if (retval)
	break;
    }

  if (debug_symfile)
    gdb_printf (gdb_stdlog, "qf->has_symbols (%s) = %d\n",
		objfile_debug_name (this), retval);

  return retval;
}

/* See objfiles.h.  */
bool
objfile::has_unexpanded_symtabs ()
{
  if (debug_symfile)
    gdb_printf (gdb_stdlog, "qf->has_unexpanded_symtabs (%s)\n",
		objfile_debug_name (this));

  bool result = false;
  for (const auto &iter : qf)
    {
      if (iter->has_unexpanded_symtabs (this))
	{
	  result = true;
	  break;
	}
    }

  if (debug_symfile)
    gdb_printf (gdb_stdlog, "qf->has_unexpanded_symtabs (%s) = %d\n",
		objfile_debug_name (this), (result ? 1 : 0));

  return result;
}

struct symtab *
objfile::find_last_source_symtab ()
{
  struct symtab *retval = nullptr;

  if (debug_symfile)
    gdb_printf (gdb_stdlog, "qf->find_last_source_symtab (%s)\n",
		objfile_debug_name (this));

  for (const auto &iter : qf)
    {
      retval = iter->find_last_source_symtab (this);
      if (retval != nullptr)
	break;
    }

  if (debug_symfile)
    gdb_printf (gdb_stdlog, "qf->find_last_source_symtab (...) = %s\n",
		retval ? debug_symtab_name (retval) : "NULL");

  return retval;
}

void
objfile::forget_cached_source_info ()
{
  if (debug_symfile)
    gdb_printf (gdb_stdlog, "qf->forget_cached_source_info (%s)\n",
		objfile_debug_name (this));

  for (compunit_symtab *cu : compunits ())
    {
      for (symtab *s : cu->filetabs ())
	{
	  if (s->fullname != NULL)
	    {
	      xfree (s->fullname);
	      s->fullname = NULL;
	    }
	}
    }

  for (const auto &iter : qf)
    iter->forget_cached_source_info (this);
}

bool
objfile::map_symtabs_matching_filename
  (const char *name, const char *real_path,
   gdb::function_view<bool (symtab *)> callback)
{
  if (debug_symfile)
    gdb_printf (gdb_stdlog,
		"qf->map_symtabs_matching_filename (%s, \"%s\", "
		"\"%s\", %s)\n",
		objfile_debug_name (this), name,
		real_path ? real_path : NULL,
		host_address_to_string (&callback));

  bool retval = true;
  const char *name_basename = lbasename (name);

  auto match_one_filename = [&] (const char *filename, bool basenames)
  {
    if (compare_filenames_for_search (filename, name))
      return true;
    if (basenames && FILENAME_CMP (name_basename, filename) == 0)
      return true;
    if (real_path != nullptr && IS_ABSOLUTE_PATH (filename)
	&& IS_ABSOLUTE_PATH (real_path))
      return filename_cmp (filename, real_path) == 0;
    return false;
  };

  compunit_symtab *last_made = this->compunit_symtabs;

  auto on_expansion = [&] (compunit_symtab *symtab)
  {
    /* The callback to iterate_over_some_symtabs returns false to keep
       going and true to continue, so we have to invert the result
       here, for expand_symtabs_matching.  */
    bool result = !iterate_over_some_symtabs (name, real_path,
					      this->compunit_symtabs,
					      last_made,
					      callback);
    last_made = this->compunit_symtabs;
    return result;
  };

  for (const auto &iter : qf)
    {
      if (!iter->expand_symtabs_matching (this,
					  match_one_filename,
					  nullptr,
					  nullptr,
					  on_expansion,
					  (SEARCH_GLOBAL_BLOCK
					   | SEARCH_STATIC_BLOCK),
					  UNDEF_DOMAIN,
					  ALL_DOMAIN))
	{
	  retval = false;
	  break;
	}
    }

  if (debug_symfile)
    gdb_printf (gdb_stdlog,
		"qf->map_symtabs_matching_filename (...) = %d\n",
		retval);

  /* We must re-invert the return value here to match the caller's
     expectations.  */
  return !retval;
}

struct compunit_symtab *
objfile::lookup_symbol (block_enum kind, const char *name, domain_enum domain)
{
  struct compunit_symtab *retval = nullptr;

  if (debug_symfile)
    gdb_printf (gdb_stdlog,
		"qf->lookup_symbol (%s, %d, \"%s\", %s)\n",
		objfile_debug_name (this), kind, name,
		domain_name (domain));

  lookup_name_info lookup_name (name, symbol_name_match_type::FULL);

  auto search_one_symtab = [&] (compunit_symtab *stab)
  {
    struct symbol *sym, *with_opaque = NULL;
    const struct blockvector *bv = stab->blockvector ();
    const struct block *block = bv->block (kind);

    sym = block_find_symbol (block, lookup_name, domain, &with_opaque);

    /* Some caution must be observed with overloaded functions
       and methods, since the index will not contain any overload
       information (but NAME might contain it).  */

    if (sym != nullptr)
      {
	retval = stab;
	/* Found it.  */
	return false;
      }
    if (with_opaque != nullptr)
      retval = stab;

    /* Keep looking through other psymtabs.  */
    return true;
  };

  for (const auto &iter : qf)
    {
      if (!iter->expand_symtabs_matching (this,
					  nullptr,
					  &lookup_name,
					  nullptr,
					  search_one_symtab,
					  kind == GLOBAL_BLOCK
					  ? SEARCH_GLOBAL_BLOCK
					  : SEARCH_STATIC_BLOCK,
					  domain,
					  ALL_DOMAIN))
	break;
    }

  if (debug_symfile)
    gdb_printf (gdb_stdlog, "qf->lookup_symbol (...) = %s\n",
		retval
		? debug_symtab_name (retval->primary_filetab ())
		: "NULL");

  return retval;
}

void
objfile::print_stats (bool print_bcache)
{
  if (debug_symfile)
    gdb_printf (gdb_stdlog, "qf->print_stats (%s, %d)\n",
		objfile_debug_name (this), print_bcache);

  for (const auto &iter : qf)
    iter->print_stats (this, print_bcache);
}

void
objfile::dump ()
{
  if (debug_symfile)
    gdb_printf (gdb_stdlog, "qf->dump (%s)\n",
		objfile_debug_name (this));

  for (const auto &iter : qf)
    iter->dump (this);
}

void
objfile::expand_symtabs_for_function (const char *func_name)
{
  if (debug_symfile)
    gdb_printf (gdb_stdlog,
		"qf->expand_symtabs_for_function (%s, \"%s\")\n",
		objfile_debug_name (this), func_name);

  lookup_name_info base_lookup (func_name, symbol_name_match_type::FULL);
  lookup_name_info lookup_name = base_lookup.make_ignore_params ();

  for (const auto &iter : qf)
    iter->expand_symtabs_matching (this,
				   nullptr,
				   &lookup_name,
				   nullptr,
				   nullptr,
				   (SEARCH_GLOBAL_BLOCK
				    | SEARCH_STATIC_BLOCK),
				   VAR_DOMAIN,
				   ALL_DOMAIN);
}

void
objfile::expand_all_symtabs ()
{
  if (debug_symfile)
    gdb_printf (gdb_stdlog, "qf->expand_all_symtabs (%s)\n",
		objfile_debug_name (this));

  for (const auto &iter : qf)
    iter->expand_all_symtabs (this);
}

void
objfile::expand_symtabs_with_fullname (const char *fullname)
{
  if (debug_symfile)
    gdb_printf (gdb_stdlog,
		"qf->expand_symtabs_with_fullname (%s, \"%s\")\n",
		objfile_debug_name (this), fullname);

  const char *basename = lbasename (fullname);
  auto file_matcher = [&] (const char *filename, bool basenames)
  {
    return filename_cmp (basenames ? basename : fullname, filename) == 0;
  };

  for (const auto &iter : qf)
    iter->expand_symtabs_matching (this,
				   file_matcher,
				   nullptr,
				   nullptr,
				   nullptr,
				   (SEARCH_GLOBAL_BLOCK
				    | SEARCH_STATIC_BLOCK),
				   UNDEF_DOMAIN,
				   ALL_DOMAIN);
}

bool
objfile::expand_symtabs_matching
  (gdb::function_view<expand_symtabs_file_matcher_ftype> file_matcher,
   const lookup_name_info *lookup_name,
   gdb::function_view<expand_symtabs_symbol_matcher_ftype> symbol_matcher,
   gdb::function_view<expand_symtabs_exp_notify_ftype> expansion_notify,
   block_search_flags search_flags,
   domain_enum domain,
   enum search_domain kind)
{
  /* This invariant is documented in quick-functions.h.  */
  gdb_assert (lookup_name != nullptr || symbol_matcher == nullptr);

  if (debug_symfile)
    gdb_printf (gdb_stdlog,
		"qf->expand_symtabs_matching (%s, %s, %s, %s, %s)\n",
		objfile_debug_name (this),
		host_address_to_string (&file_matcher),
		host_address_to_string (&symbol_matcher),
		host_address_to_string (&expansion_notify),
		search_domain_name (kind));

  for (const auto &iter : qf)
    if (!iter->expand_symtabs_matching (this, file_matcher, lookup_name,
					symbol_matcher, expansion_notify,
					search_flags, domain, kind))
      return false;
  return true;
}

struct compunit_symtab *
objfile::find_pc_sect_compunit_symtab (struct bound_minimal_symbol msymbol,
				       CORE_ADDR pc,
				       struct obj_section *section,
				       int warn_if_readin)
{
  struct compunit_symtab *retval = nullptr;

  if (debug_symfile)
    gdb_printf (gdb_stdlog,
		"qf->find_pc_sect_compunit_symtab (%s, %s, %s, %s, %d)\n",
		objfile_debug_name (this),
		host_address_to_string (msymbol.minsym),
		hex_string (pc),
		host_address_to_string (section),
		warn_if_readin);

  for (const auto &iter : qf)
    {
      retval = iter->find_pc_sect_compunit_symtab (this, msymbol, pc, section,
						   warn_if_readin);
      if (retval != nullptr)
	break;
    }

  if (debug_symfile)
    gdb_printf (gdb_stdlog,
		"qf->find_pc_sect_compunit_symtab (...) = %s\n",
		retval
		? debug_symtab_name (retval->primary_filetab ())
		: "NULL");

  return retval;
}

void
objfile::map_symbol_filenames (gdb::function_view<symbol_filename_ftype> fun,
			       bool need_fullname)
{
  if (debug_symfile)
    gdb_printf (gdb_stdlog,
		"qf->map_symbol_filenames (%s, ..., %d)\n",
		objfile_debug_name (this),
		need_fullname);

  for (const auto &iter : qf)
    iter->map_symbol_filenames (this, fun, need_fullname);
}

void
objfile::compute_main_name ()
{
  if (debug_symfile)
    gdb_printf (gdb_stdlog,
		"qf->compute_main_name (%s)\n",
		objfile_debug_name (this));

  for (const auto &iter : qf)
    iter->compute_main_name (this);
}

struct compunit_symtab *
objfile::find_compunit_symtab_by_address (CORE_ADDR address)
{
  if (debug_symfile)
    gdb_printf (gdb_stdlog,
		"qf->find_compunit_symtab_by_address (%s, %s)\n",
		objfile_debug_name (this),
		hex_string (address));

  struct compunit_symtab *result = NULL;
  for (const auto &iter : qf)
    {
      result = iter->find_compunit_symtab_by_address (this, address);
      if (result != nullptr)
	break;
    }

  if (debug_symfile)
    gdb_printf (gdb_stdlog,
		"qf->find_compunit_symtab_by_address (...) = %s\n",
		result
		? debug_symtab_name (result->primary_filetab ())
		: "NULL");

  return result;
}

enum language
objfile::lookup_global_symbol_language (const char *name,
					domain_enum domain,
					bool *symbol_found_p)
{
  enum language result = language_unknown;
  *symbol_found_p = false;

  for (const auto &iter : qf)
    {
      result = iter->lookup_global_symbol_language (this, name, domain,
						    symbol_found_p);
      if (*symbol_found_p)
	break;
    }

  return result;
}

/* Call LOOKUP_FUNC to find the filename of a file containing the separate
   debug information matching OBJFILE.  If LOOKUP_FUNC does return a
   filename then open this file and return a std::pair containing the
   gdb_bfd_ref_ptr of the open file and the filename returned by
   LOOKUP_FUNC, otherwise this function returns an empty pair; the first
   item will be nullptr, and the second will be an empty string.

   Any warnings generated by this function, or by calling LOOKUP_FUNC are
   placed into WARNINGS, these warnings are only displayed to the user if
   GDB is unable to find the separate debug information via any route.  */
static std::pair<gdb_bfd_ref_ptr, std::string>
simple_find_and_open_separate_symbol_file
  (struct objfile *objfile,
   std::string (*lookup_func) (struct objfile *, deferred_warnings *),
   deferred_warnings *warnings)
{
  std::string filename = lookup_func (objfile, warnings);

  if (!filename.empty ())
    {
      gdb_bfd_ref_ptr symfile_bfd
	= symfile_bfd_open_no_error (filename.c_str ());
      if (symfile_bfd != nullptr)
	return { symfile_bfd, filename };
    }

  return {};
}

/* Lookup separate debug information for OBJFILE via debuginfod.  If
   successful the debug information will be have been downloaded into the
   debuginfod cache and this function will return a std::pair containing a
   gdb_bfd_ref_ptr of the open debug information file and the filename for
   the file within the debuginfod cache.  If no debug information could be
   found then this function returns an empty pair; the first item will be
   nullptr, and the second will be an empty string.  */

static std::pair<gdb_bfd_ref_ptr, std::string>
debuginfod_find_and_open_separate_symbol_file (struct objfile * objfile)
{
  const struct bfd_build_id *build_id
    = build_id_bfd_get (objfile->obfd.get ());
  const char *filename = bfd_get_filename (objfile->obfd.get ());

  if (build_id != nullptr)
    {
      gdb::unique_xmalloc_ptr<char> symfile_path;
      scoped_fd fd (debuginfod_debuginfo_query (build_id->data, build_id->size,
						filename, &symfile_path));

      if (fd.get () >= 0)
	{
	  /* File successfully retrieved from server.  */
	  gdb_bfd_ref_ptr debug_bfd
	    (symfile_bfd_open_no_error (symfile_path.get ()));

	  if (debug_bfd != nullptr
	      && build_id_verify (debug_bfd.get (),
				  build_id->size, build_id->data))
	    return { debug_bfd, std::string (symfile_path.get ()) };
	}
    }

  return {};
}

/* See objfiles.h.  */

bool
objfile::find_and_add_separate_symbol_file (symfile_add_flags symfile_flags)
{
  bool has_dwarf2 = false;

  /* Usually we only make a single pass when looking for separate debug
     information.  However, it is possible for an extension language hook
     to request that GDB make a second pass, in which case max_attempts
     will be updated, and the loop restarted.  */
  for (unsigned attempt = 0, max_attempts = 1;
       attempt < max_attempts && !has_dwarf2;
       ++attempt)
    {
      gdb_assert (max_attempts <= 2);

      deferred_warnings warnings;
      gdb_bfd_ref_ptr debug_bfd;
      std::string filename;

      std::tie (debug_bfd, filename)
	= simple_find_and_open_separate_symbol_file
	    (this, find_separate_debug_file_by_buildid, &warnings);

      if (debug_bfd == nullptr)
	std::tie (debug_bfd, filename)
	  = simple_find_and_open_separate_symbol_file
	      (this, find_separate_debug_file_by_debuglink, &warnings);

      /* Only try debuginfod on the first attempt.  Sure, we could imagine
	 an extension that somehow adds the required debug info to the
	 debuginfod server but, at least for now, we don't support this
	 scenario.  Better for the extension to return new debug info
	 directly to GDB.  Plus, going to the debuginfod server might be
	 slow, so that's a good argument for only doing this once.  */
      if (debug_bfd == nullptr && attempt == 0)
	std::tie (debug_bfd, filename)
	  = debuginfod_find_and_open_separate_symbol_file (this);

      if (debug_bfd != nullptr)
	{
	  /* We found a separate debug info symbol file.  If this is our
	     first attempt then setting HAS_DWARF2 will cause us to break
	     from the attempt loop.  */
	  symbol_file_add_separate (debug_bfd, filename.c_str (),
				    symfile_flags, this);
	  has_dwarf2 = true;
	}
      else if (attempt == 0)
	{
	  /* Failed to find a separate debug info symbol file.  Call out to
	     the extension languages.  The user might have registered an
	     extension that can find the debug info for us, or maybe give
	     the user a system specific message that guides them to finding
	     the missing debug info.  */

	  ext_lang_missing_debuginfo_result ext_result
	    = ext_lang_handle_missing_debuginfo (this);
	  if (!ext_result.filename ().empty ())
	    {
	      /* Extension found a suitable debug file for us.  */
	      debug_bfd
		= symfile_bfd_open_no_error (ext_result.filename ().c_str ());

	      if (debug_bfd != nullptr)
		{
		  symbol_file_add_separate (debug_bfd,
					    ext_result.filename ().c_str (),
					    symfile_flags, this);
		  has_dwarf2 = true;
		}
	    }
	  else if (ext_result.try_again ())
	    {
	      max_attempts = 2;
	      continue;
	    }
	}

      /* If we still have not got a separate debug symbol file, then
	 emit any warnings we've collected so far.  */
      if (!has_dwarf2)
	warnings.emit ();
    }

  return has_dwarf2;
}


/* Debugging version of struct sym_probe_fns.  */

static const std::vector<std::unique_ptr<probe>> &
debug_sym_get_probes (struct objfile *objfile)
{
  const struct debug_sym_fns_data *debug_data
    = symfile_debug_objfile_data_key.get (objfile);

  const std::vector<std::unique_ptr<probe>> &retval
    = debug_data->real_sf->sym_probe_fns->sym_get_probes (objfile);

  gdb_printf (gdb_stdlog,
	      "probes->sym_get_probes (%s) = %s\n",
	      objfile_debug_name (objfile),
	      host_address_to_string (retval.data ()));

  return retval;
}

static const struct sym_probe_fns debug_sym_probe_fns =
{
  debug_sym_get_probes,
};

/* Debugging version of struct sym_fns.  */

static void
debug_sym_new_init (struct objfile *objfile)
{
  const struct debug_sym_fns_data *debug_data
    = symfile_debug_objfile_data_key.get (objfile);

  gdb_printf (gdb_stdlog, "sf->sym_new_init (%s)\n",
	      objfile_debug_name (objfile));

  debug_data->real_sf->sym_new_init (objfile);
}

static void
debug_sym_init (struct objfile *objfile)
{
  const struct debug_sym_fns_data *debug_data
    = symfile_debug_objfile_data_key.get (objfile);

  gdb_printf (gdb_stdlog, "sf->sym_init (%s)\n",
	      objfile_debug_name (objfile));

  debug_data->real_sf->sym_init (objfile);
}

static void
debug_sym_read (struct objfile *objfile, symfile_add_flags symfile_flags)
{
  const struct debug_sym_fns_data *debug_data
    = symfile_debug_objfile_data_key.get (objfile);

  gdb_printf (gdb_stdlog, "sf->sym_read (%s, 0x%x)\n",
	      objfile_debug_name (objfile), (unsigned) symfile_flags);

  debug_data->real_sf->sym_read (objfile, symfile_flags);
}

static void
debug_sym_finish (struct objfile *objfile)
{
  const struct debug_sym_fns_data *debug_data
    = symfile_debug_objfile_data_key.get (objfile);

  gdb_printf (gdb_stdlog, "sf->sym_finish (%s)\n",
	      objfile_debug_name (objfile));

  debug_data->real_sf->sym_finish (objfile);
}

static void
debug_sym_offsets (struct objfile *objfile,
		   const section_addr_info &info)
{
  const struct debug_sym_fns_data *debug_data
    = symfile_debug_objfile_data_key.get (objfile);

  gdb_printf (gdb_stdlog, "sf->sym_offsets (%s, %s)\n",
	      objfile_debug_name (objfile),
	      host_address_to_string (&info));

  debug_data->real_sf->sym_offsets (objfile, info);
}

static symfile_segment_data_up
debug_sym_segments (bfd *abfd)
{
  /* This API function is annoying, it doesn't take a "this" pointer.
     Fortunately it is only used in one place where we (re-)lookup the
     sym_fns table to use.  Thus we will never be called.  */
  gdb_assert_not_reached ("debug_sym_segments called");
}

static void
debug_sym_read_linetable (struct objfile *objfile)
{
  const struct debug_sym_fns_data *debug_data
    = symfile_debug_objfile_data_key.get (objfile);

  gdb_printf (gdb_stdlog, "sf->sym_read_linetable (%s)\n",
	      objfile_debug_name (objfile));

  debug_data->real_sf->sym_read_linetable (objfile);
}

static bfd_byte *
debug_sym_relocate (struct objfile *objfile, asection *sectp, bfd_byte *buf)
{
  const struct debug_sym_fns_data *debug_data
    = symfile_debug_objfile_data_key.get (objfile);
  bfd_byte *retval;

  retval = debug_data->real_sf->sym_relocate (objfile, sectp, buf);

  gdb_printf (gdb_stdlog,
	      "sf->sym_relocate (%s, %s, %s) = %s\n",
	      objfile_debug_name (objfile),
	      host_address_to_string (sectp),
	      host_address_to_string (buf),
	      host_address_to_string (retval));

  return retval;
}

/* Template of debugging version of struct sym_fns.
   A copy is made, with sym_flavour updated, and a pointer to the real table
   installed in real_sf, and then a pointer to the copy is installed in the
   objfile.  */

static const struct sym_fns debug_sym_fns =
{
  debug_sym_new_init,
  debug_sym_init,
  debug_sym_read,
  debug_sym_finish,
  debug_sym_offsets,
  debug_sym_segments,
  debug_sym_read_linetable,
  debug_sym_relocate,
  &debug_sym_probe_fns,
};

/* Install the debugging versions of the symfile functions for OBJFILE.
   Do not call this if the debug versions are already installed.  */

static void
install_symfile_debug_logging (struct objfile *objfile)
{
  const struct sym_fns *real_sf;
  struct debug_sym_fns_data *debug_data;

  /* The debug versions should not already be installed.  */
  gdb_assert (!symfile_debug_installed (objfile));

  real_sf = objfile->sf;

  /* Alas we have to preserve NULL entries in REAL_SF.  */
  debug_data = new struct debug_sym_fns_data;

#define COPY_SF_PTR(from, to, name, func)	\
  do {						\
    if ((from)->name)				\
      (to)->debug_sf.name = func;		\
  } while (0)

  COPY_SF_PTR (real_sf, debug_data, sym_new_init, debug_sym_new_init);
  COPY_SF_PTR (real_sf, debug_data, sym_init, debug_sym_init);
  COPY_SF_PTR (real_sf, debug_data, sym_read, debug_sym_read);
  COPY_SF_PTR (real_sf, debug_data, sym_finish, debug_sym_finish);
  COPY_SF_PTR (real_sf, debug_data, sym_offsets, debug_sym_offsets);
  COPY_SF_PTR (real_sf, debug_data, sym_segments, debug_sym_segments);
  COPY_SF_PTR (real_sf, debug_data, sym_read_linetable,
	       debug_sym_read_linetable);
  COPY_SF_PTR (real_sf, debug_data, sym_relocate, debug_sym_relocate);
  if (real_sf->sym_probe_fns)
    debug_data->debug_sf.sym_probe_fns = &debug_sym_probe_fns;

#undef COPY_SF_PTR

  debug_data->real_sf = real_sf;
  symfile_debug_objfile_data_key.set (objfile, debug_data);
  objfile->sf = &debug_data->debug_sf;
}

/* Uninstall the debugging versions of the symfile functions for OBJFILE.
   Do not call this if the debug versions are not installed.  */

static void
uninstall_symfile_debug_logging (struct objfile *objfile)
{
  struct debug_sym_fns_data *debug_data;

  /* The debug versions should be currently installed.  */
  gdb_assert (symfile_debug_installed (objfile));

  debug_data = symfile_debug_objfile_data_key.get (objfile);

  objfile->sf = debug_data->real_sf;
  symfile_debug_objfile_data_key.clear (objfile);
}

/* Call this function to set OBJFILE->SF.
   Do not set OBJFILE->SF directly.  */

void
objfile_set_sym_fns (struct objfile *objfile, const struct sym_fns *sf)
{
  if (symfile_debug_installed (objfile))
    {
      gdb_assert (debug_symfile);
      /* Remove the current one, and reinstall a new one later.  */
      uninstall_symfile_debug_logging (objfile);
    }

  /* Assume debug logging is disabled.  */
  objfile->sf = sf;

  /* Turn debug logging on if enabled.  */
  if (debug_symfile)
    install_symfile_debug_logging (objfile);
}

static void
set_debug_symfile (const char *args, int from_tty, struct cmd_list_element *c)
{
  for (struct program_space *pspace : program_spaces)
    for (objfile *objfile : pspace->objfiles ())
      {
	if (debug_symfile)
	  {
	    if (!symfile_debug_installed (objfile))
	      install_symfile_debug_logging (objfile);
	  }
	else
	  {
	    if (symfile_debug_installed (objfile))
	      uninstall_symfile_debug_logging (objfile);
	  }
      }
}

static void
show_debug_symfile (struct ui_file *file, int from_tty,
			struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Symfile debugging is %s.\n"), value);
}

void _initialize_symfile_debug ();
void
_initialize_symfile_debug ()
{
  add_setshow_boolean_cmd ("symfile", no_class, &debug_symfile, _("\
Set debugging of the symfile functions."), _("\
Show debugging of the symfile functions."), _("\
When enabled, all calls to the symfile functions are logged."),
			   set_debug_symfile, show_debug_symfile,
			   &setdebuglist, &showdebuglist);

  /* Note: We don't need a new-objfile observer because debug logging
     will be installed when objfile init'n calls objfile_set_sym_fns.  */
}
