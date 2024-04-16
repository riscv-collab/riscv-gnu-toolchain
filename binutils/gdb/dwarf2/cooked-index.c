/* DIE indexing 

   Copyright (C) 2022-2024 Free Software Foundation, Inc.

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
#include "dwarf2/cooked-index.h"
#include "dwarf2/read.h"
#include "dwarf2/stringify.h"
#include "dwarf2/index-cache.h"
#include "cp-support.h"
#include "c-lang.h"
#include "ada-lang.h"
#include "split-name.h"
#include "observable.h"
#include "run-on-main-thread.h"
#include <algorithm>
#include "gdbsupport/gdb-safe-ctype.h"
#include "gdbsupport/selftest.h"
#include <chrono>
#include <unordered_set>
#include "cli/cli-cmds.h"

/* We don't want gdb to exit while it is in the process of writing to
   the index cache.  So, all live cooked index vectors are stored
   here, and then these are all waited for before exit proceeds.  */
static std::unordered_set<cooked_index *> active_vectors;

/* See cooked-index.h.  */

std::string
to_string (cooked_index_flag flags)
{
  static constexpr cooked_index_flag::string_mapping mapping[] = {
    MAP_ENUM_FLAG (IS_MAIN),
    MAP_ENUM_FLAG (IS_STATIC),
    MAP_ENUM_FLAG (IS_ENUM_CLASS),
    MAP_ENUM_FLAG (IS_LINKAGE),
    MAP_ENUM_FLAG (IS_TYPE_DECLARATION),
    MAP_ENUM_FLAG (IS_PARENT_DEFERRED),
  };

  return flags.to_string (mapping);
}

/* See cooked-index.h.  */

bool
language_requires_canonicalization (enum language lang)
{
  return (lang == language_ada
	  || lang == language_c
	  || lang == language_cplus);
}

/* Return true if a plain "main" could be the main program for this
   language.  Languages that are known to use some other mechanism are
   excluded here.  */

static bool
language_may_use_plain_main (enum language lang)
{
  /* No need to handle "unknown" here.  */
  return (lang == language_c
	  || lang == language_objc
	  || lang == language_cplus
	  || lang == language_m2
	  || lang == language_asm
	  || lang == language_opencl
	  || lang == language_minimal);
}

/* See cooked-index.h.  */

int
cooked_index_entry::compare (const char *stra, const char *strb,
			     comparison_mode mode)
{
  auto munge = [] (char c) -> unsigned char
    {
      /* We want to sort '<' before any other printable character.
	 So, rewrite '<' to something just before ' '.  */
      if (c == '<')
	return '\x1f';
      return TOLOWER ((unsigned char) c);
    };

  while (*stra != '\0'
	 && *strb != '\0'
	 && (munge (*stra) == munge (*strb)))
    {
      ++stra;
      ++strb;
    }

  unsigned char c1 = munge (*stra);
  unsigned char c2 = munge (*strb);

  if (c1 == c2)
    return 0;

  /* When completing, if STRB ends earlier than STRA, consider them as
     equal.  When comparing, if STRB ends earlier and STRA ends with
     '<', consider them as equal.  */
  if (mode == COMPLETE || (mode == MATCH && c1 == munge ('<')))
    {
      if (c2 == '\0')
	return 0;
    }

  return c1 < c2 ? -1 : 1;
}

#if GDB_SELF_TEST

namespace {

void
test_compare ()
{
  /* Convenience aliases.  */
  const auto mode_compare = cooked_index_entry::MATCH;
  const auto mode_sort = cooked_index_entry::SORT;
  const auto mode_complete = cooked_index_entry::COMPLETE;

  SELF_CHECK (cooked_index_entry::compare ("abcd", "abcd",
					   mode_compare) == 0);
  SELF_CHECK (cooked_index_entry::compare ("abcd", "abcd",
					   mode_complete) == 0);

  SELF_CHECK (cooked_index_entry::compare ("abcd", "ABCDE",
					   mode_compare) < 0);
  SELF_CHECK (cooked_index_entry::compare ("ABCDE", "abcd",
					   mode_compare) > 0);
  SELF_CHECK (cooked_index_entry::compare ("abcd", "ABCDE",
					   mode_complete) < 0);
  SELF_CHECK (cooked_index_entry::compare ("ABCDE", "abcd",
					   mode_complete) == 0);

  SELF_CHECK (cooked_index_entry::compare ("name", "name<>",
					   mode_compare) < 0);
  SELF_CHECK (cooked_index_entry::compare ("name<>", "name",
					   mode_compare) == 0);
  SELF_CHECK (cooked_index_entry::compare ("name", "name<>",
					   mode_complete) < 0);
  SELF_CHECK (cooked_index_entry::compare ("name<>", "name",
					   mode_complete) == 0);

  SELF_CHECK (cooked_index_entry::compare ("name<arg>", "name<arg>",
					   mode_compare) == 0);
  SELF_CHECK (cooked_index_entry::compare ("name<arg>", "name<ag>",
					   mode_compare) > 0);
  SELF_CHECK (cooked_index_entry::compare ("name<arg>", "name<arg>",
					   mode_complete) == 0);
  SELF_CHECK (cooked_index_entry::compare ("name<arg>", "name<ag>",
					   mode_complete) > 0);

  SELF_CHECK (cooked_index_entry::compare ("name<arg<more>>",
					   "name<arg<more>>",
					   mode_compare) == 0);

  SELF_CHECK (cooked_index_entry::compare ("name", "name<arg<more>>",
					   mode_compare) < 0);
  SELF_CHECK (cooked_index_entry::compare ("name<arg<more>>", "name",
					   mode_compare) == 0);
  SELF_CHECK (cooked_index_entry::compare ("name<arg<more>>", "name<arg<",
					   mode_compare) > 0);
  SELF_CHECK (cooked_index_entry::compare ("name<arg<more>>", "name<arg<",
					   mode_complete) == 0);

  SELF_CHECK (cooked_index_entry::compare ("", "abcd", mode_compare) < 0);
  SELF_CHECK (cooked_index_entry::compare ("", "abcd", mode_complete) < 0);
  SELF_CHECK (cooked_index_entry::compare ("abcd", "", mode_compare) > 0);
  SELF_CHECK (cooked_index_entry::compare ("abcd", "", mode_complete) == 0);

  SELF_CHECK (cooked_index_entry::compare ("func", "func<type>",
					   mode_sort) < 0);
  SELF_CHECK (cooked_index_entry::compare ("func<type>", "func1",
					   mode_sort) < 0);
}

} /* anonymous namespace */

#endif /* GDB_SELF_TEST */

/* See cooked-index.h.  */

const char *
cooked_index_entry::full_name (struct obstack *storage, bool for_main) const
{
  const char *local_name = for_main ? name : canonical;

  if ((flags & IS_LINKAGE) != 0 || get_parent () == nullptr)
    return local_name;

  const char *sep = nullptr;
  switch (per_cu->lang ())
    {
    case language_cplus:
    case language_rust:
      sep = "::";
      break;

    case language_go:
    case language_d:
    case language_ada:
      sep = ".";
      break;

    default:
      return local_name;
    }

  get_parent ()->write_scope (storage, sep, for_main);
  obstack_grow0 (storage, local_name, strlen (local_name));
  return (const char *) obstack_finish (storage);
}

/* See cooked-index.h.  */

void
cooked_index_entry::write_scope (struct obstack *storage,
				 const char *sep,
				 bool for_main) const
{
  if (get_parent () != nullptr)
    get_parent ()->write_scope (storage, sep, for_main);
  const char *local_name = for_main ? name : canonical;
  obstack_grow (storage, local_name, strlen (local_name));
  obstack_grow (storage, sep, strlen (sep));
}

/* See cooked-index.h.  */

cooked_index_entry *
cooked_index_shard::add (sect_offset die_offset, enum dwarf_tag tag,
			 cooked_index_flag flags, const char *name,
			 cooked_index_entry_ref parent_entry,
			 dwarf2_per_cu_data *per_cu)
{
  cooked_index_entry *result = create (die_offset, tag, flags, name,
				       parent_entry, per_cu);
  m_entries.push_back (result);

  /* An explicitly-tagged main program should always override the
     implicit "main" discovery.  */
  if ((flags & IS_MAIN) != 0)
    m_main = result;
  else if ((flags & IS_PARENT_DEFERRED) == 0
	   && parent_entry.resolved == nullptr
	   && m_main == nullptr
	   && language_may_use_plain_main (per_cu->lang ())
	   && strcmp (name, "main") == 0)
    m_main = result;

  return result;
}

/* See cooked-index.h.  */

gdb::unique_xmalloc_ptr<char>
cooked_index_shard::handle_gnat_encoded_entry (cooked_index_entry *entry,
					       htab_t gnat_entries)
{
  /* We decode Ada names in a particular way: operators and wide
     characters are left as-is.  This is done to make name matching a
     bit simpler; and for wide characters, it means the choice of Ada
     source charset does not affect the indexer directly.  */
  std::string canonical = ada_decode (entry->name, false, false, false);
  if (canonical.empty ())
    return {};
  std::vector<std::string_view> names = split_name (canonical.c_str (),
						    split_style::DOT_STYLE);
  std::string_view tail = names.back ();
  names.pop_back ();

  const cooked_index_entry *parent = nullptr;
  for (const auto &name : names)
    {
      uint32_t hashval = dwarf5_djb_hash (name);
      void **slot = htab_find_slot_with_hash (gnat_entries, &name,
					      hashval, INSERT);
      /* CUs are processed in order, so we only need to check the most
	 recent entry.  */
      cooked_index_entry *last = (cooked_index_entry *) *slot;
      if (last == nullptr || last->per_cu != entry->per_cu)
	{
	  gdb::unique_xmalloc_ptr<char> new_name
	    = make_unique_xstrndup (name.data (), name.length ());
	  last = create (entry->die_offset, DW_TAG_namespace,
			 0, new_name.get (), parent,
			 entry->per_cu);
	  last->canonical = last->name;
	  m_names.push_back (std::move (new_name));
	  *slot = last;
	}

      parent = last;
    }

  entry->set_parent (parent);
  return make_unique_xstrndup (tail.data (), tail.length ());
}

/* See cooked-index.h.  */

void
cooked_index_shard::finalize ()
{
  auto hash_name_ptr = [] (const void *p)
    {
      const cooked_index_entry *entry = (const cooked_index_entry *) p;
      return htab_hash_pointer (entry->name);
    };

  auto eq_name_ptr = [] (const void *a, const void *b) -> int
    {
      const cooked_index_entry *ea = (const cooked_index_entry *) a;
      const cooked_index_entry *eb = (const cooked_index_entry *) b;
      return ea->name == eb->name;
    };

  /* We can use pointer equality here because names come from
     .debug_str, which will normally be unique-ified by the linker.
     Also, duplicates are relatively harmless -- they just mean a bit
     of extra memory is used.  */
  htab_up seen_names (htab_create_alloc (10, hash_name_ptr, eq_name_ptr,
					 nullptr, xcalloc, xfree));

  auto hash_entry = [] (const void *e)
    {
      const cooked_index_entry *entry = (const cooked_index_entry *) e;
      return dwarf5_djb_hash (entry->canonical);
    };

  auto eq_entry = [] (const void *a, const void *b) -> int
    {
      const cooked_index_entry *ae = (const cooked_index_entry *) a;
      const std::string_view *sv = (const std::string_view *) b;
      return (strlen (ae->canonical) == sv->length ()
	      && strncasecmp (ae->canonical, sv->data (), sv->length ()) == 0);
    };

  htab_up gnat_entries (htab_create_alloc (10, hash_entry, eq_entry,
					   nullptr, xcalloc, xfree));

  for (cooked_index_entry *entry : m_entries)
    {
      /* Note that this code must be kept in sync with
	 language_requires_canonicalization.  */
      gdb_assert (entry->canonical == nullptr);
      if ((entry->flags & IS_LINKAGE) != 0)
	entry->canonical = entry->name;
      else if (entry->per_cu->lang () == language_ada)
	{
	  gdb::unique_xmalloc_ptr<char> canon_name
	    = handle_gnat_encoded_entry (entry, gnat_entries.get ());
	  if (canon_name == nullptr)
	    entry->canonical = entry->name;
	  else
	    {
	      entry->canonical = canon_name.get ();
	      m_names.push_back (std::move (canon_name));
	    }
	}
      else if (entry->per_cu->lang () == language_cplus
	       || entry->per_cu->lang () == language_c)
	{
	  void **slot = htab_find_slot (seen_names.get (), entry,
					INSERT);
	  if (*slot == nullptr)
	    {
	      gdb::unique_xmalloc_ptr<char> canon_name
		= (entry->per_cu->lang () == language_cplus
		   ? cp_canonicalize_string (entry->name)
		   : c_canonicalize_name (entry->name));
	      if (canon_name == nullptr)
		entry->canonical = entry->name;
	      else
		{
		  entry->canonical = canon_name.get ();
		  m_names.push_back (std::move (canon_name));
		}
	      *slot = entry;
	    }
	  else
	    {
	      const cooked_index_entry *other
		= (const cooked_index_entry *) *slot;
	      entry->canonical = other->canonical;
	    }
	}
      else
	entry->canonical = entry->name;
    }

  m_names.shrink_to_fit ();
  m_entries.shrink_to_fit ();
  std::sort (m_entries.begin (), m_entries.end (),
	     [] (const cooked_index_entry *a, const cooked_index_entry *b)
	     {
	       return *a < *b;
	     });
}

/* See cooked-index.h.  */

cooked_index_shard::range
cooked_index_shard::find (const std::string &name, bool completing) const
{
  cooked_index_entry::comparison_mode mode = (completing
					      ? cooked_index_entry::COMPLETE
					      : cooked_index_entry::MATCH);

  auto lower = std::lower_bound (m_entries.cbegin (), m_entries.cend (), name,
				 [=] (const cooked_index_entry *entry,
				      const std::string &n)
  {
    return cooked_index_entry::compare (entry->canonical, n.c_str (), mode) < 0;
  });

  auto upper = std::upper_bound (m_entries.cbegin (), m_entries.cend (), name,
				 [=] (const std::string &n,
				      const cooked_index_entry *entry)
  {
    return cooked_index_entry::compare (entry->canonical, n.c_str (), mode) > 0;
  });

  return range (lower, upper);
}


cooked_index::cooked_index (dwarf2_per_objfile *per_objfile)
  : m_state (std::make_unique<cooked_index_worker> (per_objfile)),
    m_per_bfd (per_objfile->per_bfd)
{
  /* ACTIVE_VECTORS is not locked, and this assert ensures that this
     will be caught if ever moved to the background.  */
  gdb_assert (is_main_thread ());
  active_vectors.insert (this);
}

void
cooked_index::start_reading ()
{
  m_state->start ();
}

void
cooked_index::wait (cooked_state desired_state, bool allow_quit)
{
  gdb_assert (desired_state != cooked_state::INITIAL);

  /* If the state object has been deleted, then that means waiting is
     completely done.  */
  if (m_state == nullptr)
    return;

  if (m_state->wait (desired_state, allow_quit))
    {
      /* Only the main thread can modify this.  */
      gdb_assert (is_main_thread ());
      m_state.reset (nullptr);
    }
}

void
cooked_index::set_contents (vec_type &&vec)
{
  gdb_assert (m_vector.empty ());
  m_vector = std::move (vec);

  m_state->set (cooked_state::MAIN_AVAILABLE);

  index_cache_store_context ctx (global_index_cache, m_per_bfd);

  /* This is run after finalization is done -- but not before.  If
     this task were submitted earlier, it would have to wait for
     finalization.  However, that would take a slot in the global
     thread pool, and if enough such tasks were submitted at once, it
     would cause a livelock.  */
  gdb::task_group finalizers ([this, ctx = std::move (ctx)] ()
  {
    m_state->set (cooked_state::FINALIZED);
    maybe_write_index (m_per_bfd, ctx);
  });

  for (auto &idx : m_vector)
    {
      auto this_index = idx.get ();
      finalizers.add_task ([=] () { this_index->finalize (); });
    }

  finalizers.start ();
}

cooked_index::~cooked_index ()
{
  /* Wait for index-creation to be done, though this one must also
     waited for by the per-BFD object to ensure the required data
     remains live.  */
  wait (cooked_state::CACHE_DONE);

  /* Remove our entry from the global list.  See the assert in the
     constructor to understand this.  */
  gdb_assert (is_main_thread ());
  active_vectors.erase (this);
}

/* See cooked-index.h.  */

dwarf2_per_cu_data *
cooked_index::lookup (unrelocated_addr addr)
{
  /* Ensure that the address maps are ready.  */
  wait (cooked_state::MAIN_AVAILABLE, true);
  for (const auto &index : m_vector)
    {
      dwarf2_per_cu_data *result = index->lookup (addr);
      if (result != nullptr)
	return result;
    }
  return nullptr;
}

/* See cooked-index.h.  */

std::vector<const addrmap *>
cooked_index::get_addrmaps ()
{
  /* Ensure that the address maps are ready.  */
  wait (cooked_state::MAIN_AVAILABLE, true);
  std::vector<const addrmap *> result;
  for (const auto &index : m_vector)
    result.push_back (index->m_addrmap);
  return result;
}

/* See cooked-index.h.  */

cooked_index::range
cooked_index::find (const std::string &name, bool completing)
{
  wait (cooked_state::FINALIZED, true);
  std::vector<cooked_index_shard::range> result_range;
  result_range.reserve (m_vector.size ());
  for (auto &entry : m_vector)
    result_range.push_back (entry->find (name, completing));
  return range (std::move (result_range));
}

/* See cooked-index.h.  */

const char *
cooked_index::get_main_name (struct obstack *obstack, enum language *lang)
  const
{
  const cooked_index_entry *entry = get_main ();
  if (entry == nullptr)
    return nullptr;

  *lang = entry->per_cu->lang ();
  return entry->full_name (obstack, true);
}

/* See cooked_index.h.  */

const cooked_index_entry *
cooked_index::get_main () const
{
  const cooked_index_entry *best_entry = nullptr;
  for (const auto &index : m_vector)
    {
      const cooked_index_entry *entry = index->get_main ();
      /* Choose the first "main" we see.  We only do this for names
	 not requiring canonicalization.  At this point in the process
	 names might not have been canonicalized.  However, currently,
	 languages that require this step also do not use
	 DW_AT_main_subprogram.  An assert is appropriate here because
	 this filtering is done in get_main.  */
      if (entry != nullptr)
	{
	  if ((entry->flags & IS_MAIN) != 0)
	    {
	      if (!language_requires_canonicalization (entry->per_cu->lang ()))
		{
		  /* There won't be one better than this.  */
		  return entry;
		}
	    }
	  else
	    {
	      /* This is one that is named "main".  Here we don't care
		 if the language requires canonicalization, due to how
		 the entry is detected.  Entries like this have worse
		 priority than IS_MAIN entries.  */
	      if (best_entry == nullptr)
		best_entry = entry;
	    }
	}
    }

  return best_entry;
}

/* See cooked-index.h.  */

void
cooked_index::dump (gdbarch *arch)
{
  auto_obstack temp_storage;

  gdb_printf ("  entries:\n");
  gdb_printf ("\n");

  size_t i = 0;
  for (const cooked_index_entry *entry : this->all_entries ())
    {
      QUIT;

      gdb_printf ("    [%zu] ((cooked_index_entry *) %p)\n", i++, entry);
      gdb_printf ("    name:       %s\n", entry->name);
      gdb_printf ("    canonical:  %s\n", entry->canonical);
      gdb_printf ("    qualified:  %s\n", entry->full_name (&temp_storage, false));
      gdb_printf ("    DWARF tag:  %s\n", dwarf_tag_name (entry->tag));
      gdb_printf ("    flags:      %s\n", to_string (entry->flags).c_str ());
      gdb_printf ("    DIE offset: %s\n", sect_offset_str (entry->die_offset));

      if ((entry->flags & IS_PARENT_DEFERRED) != 0)
	gdb_printf ("    parent:     deferred (%" PRIx64 ")\n",
		    entry->get_deferred_parent ());
      else if (entry->get_parent () != nullptr)
	gdb_printf ("    parent:     ((cooked_index_entry *) %p) [%s]\n",
		    entry->get_parent (), entry->get_parent ()->name);
      else
	gdb_printf ("    parent:     ((cooked_index_entry *) 0)\n");

      gdb_printf ("\n");
    }

  const cooked_index_entry *main_entry = this->get_main ();
  if (main_entry != nullptr)
    gdb_printf ("  main: ((cooked_index_entry *) %p) [%s]\n", main_entry,
		  main_entry->name);
  else
    gdb_printf ("  main: ((cooked_index_entry *) 0)\n");

  gdb_printf ("\n");
  gdb_printf ("  address maps:\n");
  gdb_printf ("\n");

  std::vector<const addrmap *> addrmaps = this->get_addrmaps ();
  for (i = 0; i < addrmaps.size (); ++i)
    {
      const addrmap &addrmap = *addrmaps[i];

      gdb_printf ("    [%zu] ((addrmap *) %p)\n", i, &addrmap);
      gdb_printf ("\n");

      addrmap.foreach ([arch] (CORE_ADDR start_addr, const void *obj)
	{
	  QUIT;

	  const char *start_addr_str = paddress (arch, start_addr);

	  if (obj != nullptr)
	    {
	      const dwarf2_per_cu_data *per_cu
		= static_cast<const dwarf2_per_cu_data *> (obj);
	      gdb_printf ("      [%s] ((dwarf2_per_cu_data *) %p)\n",
			  start_addr_str, per_cu);
	    }
	  else
	    gdb_printf ("      [%s] ((dwarf2_per_cu_data *) 0)\n",
			start_addr_str);

	  return 0;
	});

      gdb_printf ("\n");
    }
}

void
cooked_index::maybe_write_index (dwarf2_per_bfd *per_bfd,
				 const index_cache_store_context &ctx)
{
  /* (maybe) store an index in the cache.  */
  global_index_cache.store (m_per_bfd, ctx);
  m_state->set (cooked_state::CACHE_DONE);
}

/* Wait for all the index cache entries to be written before gdb
   exits.  */
static void
wait_for_index_cache (int)
{
  gdb_assert (is_main_thread ());
  for (cooked_index *item : active_vectors)
    item->wait_completely ();
}

/* A maint command to wait for the cache.  */

static void
maintenance_wait_for_index_cache (const char *args, int from_tty)
{
  wait_for_index_cache (0);
}

void _initialize_cooked_index ();
void
_initialize_cooked_index ()
{
#if GDB_SELF_TEST
  selftests::register_test ("cooked_index_entry::compare", test_compare);
#endif

  add_cmd ("wait-for-index-cache", class_maintenance,
	   maintenance_wait_for_index_cache, _("\
Wait until all pending writes to the index cache have completed.\n\
Usage: maintenance wait-for-index-cache"),
	   &maintenancelist);

  gdb::observers::gdb_exiting.attach (wait_for_index_cache, "cooked-index");
}
