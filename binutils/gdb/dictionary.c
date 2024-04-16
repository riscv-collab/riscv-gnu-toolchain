/* Routines for name->symbol lookups in GDB.
   
   Copyright (C) 2003-2024 Free Software Foundation, Inc.

   Contributed by David Carlton <carlton@bactrian.org> and by Kealia,
   Inc.

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
#include <ctype.h>
#include "gdbsupport/gdb_obstack.h"
#include "symtab.h"
#include "buildsym.h"
#include "dictionary.h"
#include "gdbsupport/gdb-safe-ctype.h"
#include <unordered_map>
#include "language.h"

/* This file implements dictionaries, which are tables that associate
   symbols to names.  They are represented by an opaque type 'struct
   dictionary'.  That type has various internal implementations, which
   you can choose between depending on what properties you need
   (e.g. fast lookup, order-preserving, expandable).

   Each dictionary starts with a 'virtual function table' that
   contains the functions that actually implement the various
   operations that dictionaries provide.  (Note, however, that, for
   the sake of client code, we also provide some functions that can be
   implemented generically in terms of the functions in the vtable.)

   To add a new dictionary implementation <impl>, what you should do
   is:

   * Add a new element DICT_<IMPL> to dict_type.
   
   * Create a new structure dictionary_<impl>.  If your new
   implementation is a variant of an existing one, make sure that
   their structs have the same initial data members.  Define accessor
   macros for your new data members.

   * Implement all the functions in dict_vector as static functions,
   whose name is the same as the corresponding member of dict_vector
   plus _<impl>.  You don't have to do this for those members where
   you can reuse existing generic functions
   (e.g. add_symbol_nonexpandable, free_obstack) or in the case where
   your new implementation is a variant of an existing implementation
   and where the variant doesn't affect the member function in
   question.

   * Define a static const struct dict_vector dict_<impl>_vector.

   * Define a function dict_create_<impl> to create these
   gizmos.  Add its declaration to dictionary.h.

   To add a new operation <op> on all existing implementations, what
   you should do is:

   * Add a new member <op> to struct dict_vector.

   * If there is useful generic behavior <op>, define a static
   function <op>_something_informative that implements that behavior.
   (E.g. add_symbol_nonexpandable, free_obstack.)

   * For every implementation <impl> that should have its own specific
   behavior for <op>, define a static function <op>_<impl>
   implementing it.

   * Modify all existing dict_vector_<impl>'s to include the appropriate
   member.

   * Define a function dict_<op> that looks up <op> in the dict_vector
   and calls the appropriate function.  Add a declaration for
   dict_<op> to dictionary.h.  */

/* An enum representing the various implementations of dictionaries.
   Used only for debugging.  */

enum dict_type
  {
    /* Symbols are stored in a fixed-size hash table.  */
    DICT_HASHED,
    /* Symbols are stored in an expandable hash table.  */
    DICT_HASHED_EXPANDABLE,
    /* Symbols are stored in a fixed-size array.  */
    DICT_LINEAR,
    /* Symbols are stored in an expandable array.  */
    DICT_LINEAR_EXPANDABLE
  };

/* The virtual function table.  */

struct dict_vector
{
  /* The type of the dictionary.  This is only here to make debugging
     a bit easier; it's not actually used.  */
  enum dict_type type;
  /* The function to free a dictionary.  */
  void (*free) (struct dictionary *dict);
  /* Add a symbol to a dictionary, if possible.  */
  void (*add_symbol) (struct dictionary *dict, struct symbol *sym);
  /* Iterator functions.  */
  struct symbol *(*iterator_first) (const struct dictionary *dict,
				    struct dict_iterator *iterator);
  struct symbol *(*iterator_next) (struct dict_iterator *iterator);
  /* Functions to iterate over symbols with a given name.  */
  struct symbol *(*iter_match_first) (const struct dictionary *dict,
				      const lookup_name_info &name,
				      struct dict_iterator *iterator);
  struct symbol *(*iter_match_next) (const lookup_name_info &name,
				     struct dict_iterator *iterator);
  /* A size function, for maint print symtabs.  */
  int (*size) (const struct dictionary *dict);
};

/* Now comes the structs used to store the data for different
   implementations.  If two implementations have data in common, put
   the common data at the top of their structs, ordered in the same
   way.  */

struct dictionary_hashed
{
  int nbuckets;
  struct symbol **buckets;
};

struct dictionary_hashed_expandable
{
  /* How many buckets we currently have.  */
  int nbuckets;
  struct symbol **buckets;
  /* How many syms we currently have; we need this so we will know
     when to add more buckets.  */
  int nsyms;
};

struct dictionary_linear
{
  int nsyms;
  struct symbol **syms;
};

struct dictionary_linear_expandable
{
  /* How many symbols we currently have.  */
  int nsyms;
  struct symbol **syms;
  /* How many symbols we can store before needing to reallocate.  */
  int capacity;
};

/* And now, the star of our show.  */

struct dictionary
{
  const struct language_defn *language;
  const struct dict_vector *vector;
  union
  {
    struct dictionary_hashed hashed;
    struct dictionary_hashed_expandable hashed_expandable;
    struct dictionary_linear linear;
    struct dictionary_linear_expandable linear_expandable;
  }
  data;
};

/* Accessor macros.  */

#define DICT_VECTOR(d)			(d)->vector
#define DICT_LANGUAGE(d)                (d)->language

/* These can be used for DICT_HASHED_EXPANDABLE, too.  */

#define DICT_HASHED_NBUCKETS(d)		(d)->data.hashed.nbuckets
#define DICT_HASHED_BUCKETS(d)		(d)->data.hashed.buckets
#define DICT_HASHED_BUCKET(d,i)		DICT_HASHED_BUCKETS (d) [i]

#define DICT_HASHED_EXPANDABLE_NSYMS(d)	(d)->data.hashed_expandable.nsyms

/* These can be used for DICT_LINEAR_EXPANDABLEs, too.  */

#define DICT_LINEAR_NSYMS(d)		(d)->data.linear.nsyms
#define DICT_LINEAR_SYMS(d)		(d)->data.linear.syms
#define DICT_LINEAR_SYM(d,i)		DICT_LINEAR_SYMS (d) [i]

#define DICT_LINEAR_EXPANDABLE_CAPACITY(d) \
		(d)->data.linear_expandable.capacity

/* The initial size of a DICT_*_EXPANDABLE dictionary.  */

#define DICT_EXPANDABLE_INITIAL_CAPACITY 10

/* This calculates the number of buckets we'll use in a hashtable,
   given the number of symbols that it will contain.  */

#define DICT_HASHTABLE_SIZE(n)	((n)/5 + 1)

/* Accessor macros for dict_iterators; they're here rather than
   dictionary.h because code elsewhere should treat dict_iterators as
   opaque.  */

/* The dictionary that the iterator is associated to.  */
#define DICT_ITERATOR_DICT(iter)		(iter)->dict
/* For linear dictionaries, the index of the last symbol returned; for
   hashed dictionaries, the bucket of the last symbol returned.  */
#define DICT_ITERATOR_INDEX(iter)		(iter)->index
/* For hashed dictionaries, this points to the last symbol returned;
   otherwise, this is unused.  */
#define DICT_ITERATOR_CURRENT(iter)		(iter)->current

/* Declarations of functions for vectors.  */

/* Functions that might work across a range of dictionary types.  */

static void add_symbol_nonexpandable (struct dictionary *dict,
				      struct symbol *sym);

static void free_obstack (struct dictionary *dict);

/* Functions for DICT_HASHED and DICT_HASHED_EXPANDABLE
   dictionaries.  */

static struct symbol *iterator_first_hashed (const struct dictionary *dict,
					     struct dict_iterator *iterator);

static struct symbol *iterator_next_hashed (struct dict_iterator *iterator);

static struct symbol *iter_match_first_hashed (const struct dictionary *dict,
					       const lookup_name_info &name,
					      struct dict_iterator *iterator);

static struct symbol *iter_match_next_hashed (const lookup_name_info &name,
					      struct dict_iterator *iterator);

/* Functions only for DICT_HASHED.  */

static int size_hashed (const struct dictionary *dict);

/* Functions only for DICT_HASHED_EXPANDABLE.  */

static void free_hashed_expandable (struct dictionary *dict);

static void add_symbol_hashed_expandable (struct dictionary *dict,
					  struct symbol *sym);

static int size_hashed_expandable (const struct dictionary *dict);

/* Functions for DICT_LINEAR and DICT_LINEAR_EXPANDABLE
   dictionaries.  */

static struct symbol *iterator_first_linear (const struct dictionary *dict,
					     struct dict_iterator *iterator);

static struct symbol *iterator_next_linear (struct dict_iterator *iterator);

static struct symbol *iter_match_first_linear (const struct dictionary *dict,
					       const lookup_name_info &name,
					       struct dict_iterator *iterator);

static struct symbol *iter_match_next_linear (const lookup_name_info &name,
					      struct dict_iterator *iterator);

static int size_linear (const struct dictionary *dict);

/* Functions only for DICT_LINEAR_EXPANDABLE.  */

static void free_linear_expandable (struct dictionary *dict);

static void add_symbol_linear_expandable (struct dictionary *dict,
					  struct symbol *sym);

/* Various vectors that we'll actually use.  */

static const struct dict_vector dict_hashed_vector =
  {
    DICT_HASHED,			/* type */
    free_obstack,			/* free */
    add_symbol_nonexpandable,		/* add_symbol */
    iterator_first_hashed,		/* iterator_first */
    iterator_next_hashed,		/* iterator_next */
    iter_match_first_hashed,		/* iter_name_first */
    iter_match_next_hashed,		/* iter_name_next */
    size_hashed,			/* size */
  };

static const struct dict_vector dict_hashed_expandable_vector =
  {
    DICT_HASHED_EXPANDABLE,		/* type */
    free_hashed_expandable,		/* free */
    add_symbol_hashed_expandable,	/* add_symbol */
    iterator_first_hashed,		/* iterator_first */
    iterator_next_hashed,		/* iterator_next */
    iter_match_first_hashed,		/* iter_name_first */
    iter_match_next_hashed,		/* iter_name_next */
    size_hashed_expandable,		/* size */
  };

static const struct dict_vector dict_linear_vector =
  {
    DICT_LINEAR,			/* type */
    free_obstack,			/* free */
    add_symbol_nonexpandable,		/* add_symbol */
    iterator_first_linear,		/* iterator_first */
    iterator_next_linear,		/* iterator_next */
    iter_match_first_linear,		/* iter_name_first */
    iter_match_next_linear,		/* iter_name_next */
    size_linear,			/* size */
  };

static const struct dict_vector dict_linear_expandable_vector =
  {
    DICT_LINEAR_EXPANDABLE,		/* type */
    free_linear_expandable,		/* free */
    add_symbol_linear_expandable,	/* add_symbol */
    iterator_first_linear,		/* iterator_first */
    iterator_next_linear,		/* iterator_next */
    iter_match_first_linear,		/* iter_name_first */
    iter_match_next_linear,		/* iter_name_next */
    size_linear,			/* size */
  };

/* Declarations of helper functions (i.e. ones that don't go into
   vectors).  */

static struct symbol *iterator_hashed_advance (struct dict_iterator *iter);

static void insert_symbol_hashed (struct dictionary *dict,
				  struct symbol *sym);

static void expand_hashtable (struct dictionary *dict);

/* The creation functions.  */

/* Create a hashed dictionary of a given language.  */

static struct dictionary *
dict_create_hashed (struct obstack *obstack,
		    enum language language,
		    const std::vector<symbol *> &symbol_list)
{
  /* Allocate the dictionary.  */
  struct dictionary *retval = XOBNEW (obstack, struct dictionary);
  DICT_VECTOR (retval) = &dict_hashed_vector;
  DICT_LANGUAGE (retval) = language_def (language);

  /* Allocate space for symbols.  */
  int nsyms = symbol_list.size ();
  int nbuckets = DICT_HASHTABLE_SIZE (nsyms);
  DICT_HASHED_NBUCKETS (retval) = nbuckets;
  struct symbol **buckets = XOBNEWVEC (obstack, struct symbol *, nbuckets);
  memset (buckets, 0, nbuckets * sizeof (struct symbol *));
  DICT_HASHED_BUCKETS (retval) = buckets;

  /* Now fill the buckets.  */
  for (const auto &sym : symbol_list)
    insert_symbol_hashed (retval, sym);

  return retval;
}

/* Create an expandable hashed dictionary of a given language.  */

static struct dictionary *
dict_create_hashed_expandable (enum language language)
{
  struct dictionary *retval = XNEW (struct dictionary);

  DICT_VECTOR (retval) = &dict_hashed_expandable_vector;
  DICT_LANGUAGE (retval) = language_def (language);
  DICT_HASHED_NBUCKETS (retval) = DICT_EXPANDABLE_INITIAL_CAPACITY;
  DICT_HASHED_BUCKETS (retval) = XCNEWVEC (struct symbol *,
					   DICT_EXPANDABLE_INITIAL_CAPACITY);
  DICT_HASHED_EXPANDABLE_NSYMS (retval) = 0;

  return retval;
}

/* Create a linear dictionary of a given language.  */

static struct dictionary *
dict_create_linear (struct obstack *obstack,
		    enum language language,
		    const std::vector<symbol *> &symbol_list)
{
  struct dictionary *retval = XOBNEW (obstack, struct dictionary);
  DICT_VECTOR (retval) = &dict_linear_vector;
  DICT_LANGUAGE (retval) = language_def (language);

  /* Allocate space for symbols.  */
  int nsyms = symbol_list.size ();
  DICT_LINEAR_NSYMS (retval) = nsyms;
  struct symbol **syms = XOBNEWVEC (obstack, struct symbol *, nsyms);
  DICT_LINEAR_SYMS (retval) = syms;

  /* Now fill in the symbols.  */
  int idx = nsyms - 1;
  for (const auto &sym : symbol_list)
    syms[idx--] = sym;

  return retval;
}

/* Create an expandable linear dictionary of a given language.  */

static struct dictionary *
dict_create_linear_expandable (enum language language)
{
  struct dictionary *retval = XNEW (struct dictionary);

  DICT_VECTOR (retval) = &dict_linear_expandable_vector;
  DICT_LANGUAGE (retval) = language_def (language);
  DICT_LINEAR_NSYMS (retval) = 0;
  DICT_LINEAR_EXPANDABLE_CAPACITY (retval) = DICT_EXPANDABLE_INITIAL_CAPACITY;
  DICT_LINEAR_SYMS (retval)
    = XNEWVEC (struct symbol *, DICT_LINEAR_EXPANDABLE_CAPACITY (retval));

  return retval;
}

/* The functions providing the dictionary interface.  */

/* Free the memory used by a dictionary that's not on an obstack.  (If
   any.)  */

static void
dict_free (struct dictionary *dict)
{
  (DICT_VECTOR (dict))->free (dict);
}

/* Add SYM to DICT.  DICT had better be expandable.  */

static void
dict_add_symbol (struct dictionary *dict, struct symbol *sym)
{
  (DICT_VECTOR (dict))->add_symbol (dict, sym);
}

/* Utility to add a list of symbols to a dictionary.
   DICT must be an expandable dictionary.  */

static void
dict_add_pending (struct dictionary *dict,
		  const std::vector<symbol *> &symbol_list)
{
  /* Preserve ordering by reversing the list.  */
  for (auto sym = symbol_list.rbegin (); sym != symbol_list.rend (); ++sym)
    dict_add_symbol (dict, *sym);
}

/* Initialize ITERATOR to point at the first symbol in DICT, and
   return that first symbol, or NULL if DICT is empty.  */

static struct symbol *
dict_iterator_first (const struct dictionary *dict,
		     struct dict_iterator *iterator)
{
  return (DICT_VECTOR (dict))->iterator_first (dict, iterator);
}

/* Advance ITERATOR, and return the next symbol, or NULL if there are
   no more symbols.  */

static struct symbol *
dict_iterator_next (struct dict_iterator *iterator)
{
  return (DICT_VECTOR (DICT_ITERATOR_DICT (iterator)))
    ->iterator_next (iterator);
}

static struct symbol *
dict_iter_match_first (const struct dictionary *dict,
		       const lookup_name_info &name,
		       struct dict_iterator *iterator)
{
  return (DICT_VECTOR (dict))->iter_match_first (dict, name, iterator);
}

static struct symbol *
dict_iter_match_next (const lookup_name_info &name,
		      struct dict_iterator *iterator)
{
  return (DICT_VECTOR (DICT_ITERATOR_DICT (iterator)))
    ->iter_match_next (name, iterator);
}

static int
dict_size (const struct dictionary *dict)
{
  return (DICT_VECTOR (dict))->size (dict);
}
 
/* Now come functions (well, one function, currently) that are
   implemented generically by means of the vtable.  Typically, they're
   rarely used.  */


/* The functions implementing the dictionary interface.  */

/* Generic functions, where appropriate.  */

static void
free_obstack (struct dictionary *dict)
{
  /* Do nothing!  */
}

static void
add_symbol_nonexpandable (struct dictionary *dict, struct symbol *sym)
{
  internal_error (_("dict_add_symbol: non-expandable dictionary"));
}

/* Functions for DICT_HASHED and DICT_HASHED_EXPANDABLE.  */

static struct symbol *
iterator_first_hashed (const struct dictionary *dict,
		       struct dict_iterator *iterator)
{
  DICT_ITERATOR_DICT (iterator) = dict;
  DICT_ITERATOR_INDEX (iterator) = -1;
  return iterator_hashed_advance (iterator);
}

static struct symbol *
iterator_next_hashed (struct dict_iterator *iterator)
{
  struct symbol *next;

  next = DICT_ITERATOR_CURRENT (iterator)->hash_next;
  
  if (next == NULL)
    return iterator_hashed_advance (iterator);
  else
    {
      DICT_ITERATOR_CURRENT (iterator) = next;
      return next;
    }
}

static struct symbol *
iterator_hashed_advance (struct dict_iterator *iterator)
{
  const struct dictionary *dict = DICT_ITERATOR_DICT (iterator);
  int nbuckets = DICT_HASHED_NBUCKETS (dict);
  int i;

  for (i = DICT_ITERATOR_INDEX (iterator) + 1; i < nbuckets; ++i)
    {
      struct symbol *sym = DICT_HASHED_BUCKET (dict, i);
      
      if (sym != NULL)
	{
	  DICT_ITERATOR_INDEX (iterator) = i;
	  DICT_ITERATOR_CURRENT (iterator) = sym;
	  return sym;
	}
    }

  return NULL;
}

static struct symbol *
iter_match_first_hashed (const struct dictionary *dict,
			 const lookup_name_info &name,
			 struct dict_iterator *iterator)
{
  const language_defn *lang = DICT_LANGUAGE (dict);
  unsigned int hash_index = (name.search_name_hash (lang->la_language)
			     % DICT_HASHED_NBUCKETS (dict));
  symbol_name_matcher_ftype *matches_name
    = lang->get_symbol_name_matcher (name);
  struct symbol *sym;

  DICT_ITERATOR_DICT (iterator) = dict;

  /* Loop through the symbols in the given bucket, breaking when SYM
     first matches.  If SYM never matches, it will be set to NULL;
     either way, we have the right return value.  */
  
  for (sym = DICT_HASHED_BUCKET (dict, hash_index);
       sym != NULL;
       sym = sym->hash_next)
    {
      /* Warning: the order of arguments to compare matters!  */
      if (matches_name (sym->search_name (), name, NULL))
	break;
    }

  DICT_ITERATOR_CURRENT (iterator) = sym;
  return sym;
}

static struct symbol *
iter_match_next_hashed (const lookup_name_info &name,
			struct dict_iterator *iterator)
{
  const language_defn *lang = DICT_LANGUAGE (DICT_ITERATOR_DICT (iterator));
  symbol_name_matcher_ftype *matches_name
    = lang->get_symbol_name_matcher (name);
  struct symbol *next;

  for (next = DICT_ITERATOR_CURRENT (iterator)->hash_next;
       next != NULL;
       next = next->hash_next)
    {
      if (matches_name (next->search_name (), name, NULL))
	break;
    }

  DICT_ITERATOR_CURRENT (iterator) = next;

  return next;
}

/* Insert SYM into DICT.  */

static void
insert_symbol_hashed (struct dictionary *dict,
		      struct symbol *sym)
{
  unsigned int hash_index;
  unsigned int hash;
  struct symbol **buckets = DICT_HASHED_BUCKETS (dict);

  /* We don't want to insert a symbol into a dictionary of a different
     language.  The two may not use the same hashing algorithm.  */
  gdb_assert (sym->language () == DICT_LANGUAGE (dict)->la_language);

  hash = search_name_hash (sym->language (), sym->search_name ());
  hash_index = hash % DICT_HASHED_NBUCKETS (dict);
  sym->hash_next = buckets[hash_index];
  buckets[hash_index] = sym;
}

static int
size_hashed (const struct dictionary *dict)
{
  int nbuckets = DICT_HASHED_NBUCKETS (dict);
  int total = 0;

  for (int i = 0; i < nbuckets; ++i)
    {
      for (struct symbol *sym = DICT_HASHED_BUCKET (dict, i);
	   sym != nullptr;
	   sym = sym->hash_next)
	total++;
    }

  return total;
}

/* Functions only for DICT_HASHED_EXPANDABLE.  */

static void
free_hashed_expandable (struct dictionary *dict)
{
  xfree (DICT_HASHED_BUCKETS (dict));
  xfree (dict);
}

static void
add_symbol_hashed_expandable (struct dictionary *dict,
			      struct symbol *sym)
{
  int nsyms = ++DICT_HASHED_EXPANDABLE_NSYMS (dict);

  if (DICT_HASHTABLE_SIZE (nsyms) > DICT_HASHED_NBUCKETS (dict))
    expand_hashtable (dict);

  insert_symbol_hashed (dict, sym);
  DICT_HASHED_EXPANDABLE_NSYMS (dict) = nsyms;
}

static int
size_hashed_expandable (const struct dictionary *dict)
{
  return DICT_HASHED_EXPANDABLE_NSYMS (dict);
}

static void
expand_hashtable (struct dictionary *dict)
{
  int old_nbuckets = DICT_HASHED_NBUCKETS (dict);
  struct symbol **old_buckets = DICT_HASHED_BUCKETS (dict);
  int new_nbuckets = 2 * old_nbuckets + 1;
  struct symbol **new_buckets = XCNEWVEC (struct symbol *, new_nbuckets);
  int i;

  DICT_HASHED_NBUCKETS (dict) = new_nbuckets;
  DICT_HASHED_BUCKETS (dict) = new_buckets;

  for (i = 0; i < old_nbuckets; ++i)
    {
      struct symbol *sym, *next_sym;

      sym = old_buckets[i];
      if (sym != NULL) 
	{
	  for (next_sym = sym->hash_next;
	       next_sym != NULL;
	       next_sym = sym->hash_next)
	    {
	      insert_symbol_hashed (dict, sym);
	      sym = next_sym;
	    }

	  insert_symbol_hashed (dict, sym);
	}
    }

  xfree (old_buckets);
}

/* See dictionary.h.  */

unsigned int
language_defn::search_name_hash (const char *string0) const
{
  /* The Ada-encoded version of a name P1.P2...Pn has either the form
     P1__P2__...Pn<suffix> or _ada_P1__P2__...Pn<suffix> (where the Pi
     are lower-cased identifiers).  The <suffix> (which can be empty)
     encodes additional information about the denoted entity.  This
     routine hashes such names to msymbol_hash_iw(Pn).  It actually
     does this for a superset of both valid Pi and of <suffix>, but 
     in other cases it simply returns msymbol_hash_iw(STRING0).  */

  const char *string;
  unsigned int hash;

  string = string0;
  if (*string == '_')
    {
      if (startswith (string, "_ada_"))
	string += 5;
      else
	return msymbol_hash_iw (string0);
    }

  hash = 0;
  while (*string)
    {
      switch (*string)
	{
	case '$':
	case '.':
	case 'X':
	  if (string0 == string)
	    return msymbol_hash_iw (string0);
	  else
	    return hash;
	case ' ':
	case '(':
	  return msymbol_hash_iw (string0);
	case '_':
	  if (string[1] == '_' && string != string0)
	    {
	      int c = string[2];

	      if (c == 'B' && string[3] == '_')
		{
		  for (string += 4; ISDIGIT (*string); ++string)
		    ;
		  continue;
		}

	      if ((c < 'a' || c > 'z') && c != 'O')
		return hash;
	      hash = 0;
	      string += 2;
	      continue;
	    }
	  break;
	case 'T':
	  /* Ignore "TKB" suffixes.

	     These are used by Ada for subprograms implementing a task body.
	     For instance for a task T inside package Pck, the name of the
	     subprogram implementing T's body is `pck__tTKB'.  We need to
	     ignore the "TKB" suffix because searches for this task body
	     subprogram are going to be performed using `pck__t' (the encoded
	     version of the natural name `pck.t').  */
	  if (strcmp (string, "TKB") == 0)
	    return hash;
	  break;
	}

      hash = SYMBOL_HASH_NEXT (hash, *string);
      string += 1;
    }
  return hash;
}

/* Functions for DICT_LINEAR and DICT_LINEAR_EXPANDABLE.  */

static struct symbol *
iterator_first_linear (const struct dictionary *dict,
		       struct dict_iterator *iterator)
{
  DICT_ITERATOR_DICT (iterator) = dict;
  DICT_ITERATOR_INDEX (iterator) = 0;
  return DICT_LINEAR_NSYMS (dict) ? DICT_LINEAR_SYM (dict, 0) : NULL;
}

static struct symbol *
iterator_next_linear (struct dict_iterator *iterator)
{
  const struct dictionary *dict = DICT_ITERATOR_DICT (iterator);

  if (++DICT_ITERATOR_INDEX (iterator) >= DICT_LINEAR_NSYMS (dict))
    return NULL;
  else
    return DICT_LINEAR_SYM (dict, DICT_ITERATOR_INDEX (iterator));
}

static struct symbol *
iter_match_first_linear (const struct dictionary *dict,
			 const lookup_name_info &name,
			 struct dict_iterator *iterator)
{
  DICT_ITERATOR_DICT (iterator) = dict;
  DICT_ITERATOR_INDEX (iterator) = -1;

  return iter_match_next_linear (name, iterator);
}

static struct symbol *
iter_match_next_linear (const lookup_name_info &name,
			struct dict_iterator *iterator)
{
  const struct dictionary *dict = DICT_ITERATOR_DICT (iterator);
  const language_defn *lang = DICT_LANGUAGE (dict);
  symbol_name_matcher_ftype *matches_name
    = lang->get_symbol_name_matcher (name);

  int i, nsyms = DICT_LINEAR_NSYMS (dict);
  struct symbol *sym, *retval = NULL;

  for (i = DICT_ITERATOR_INDEX (iterator) + 1; i < nsyms; ++i)
    {
      sym = DICT_LINEAR_SYM (dict, i);

      if (matches_name (sym->search_name (), name, NULL))
	{
	  retval = sym;
	  break;
	}
    }

  DICT_ITERATOR_INDEX (iterator) = i;
  
  return retval;
}

static int
size_linear (const struct dictionary *dict)
{
  return DICT_LINEAR_NSYMS (dict);
}

/* Functions only for DICT_LINEAR_EXPANDABLE.  */

static void
free_linear_expandable (struct dictionary *dict)
{
  xfree (DICT_LINEAR_SYMS (dict));
  xfree (dict);
}


static void
add_symbol_linear_expandable (struct dictionary *dict,
			      struct symbol *sym)
{
  int nsyms = ++DICT_LINEAR_NSYMS (dict);

  /* Do we have enough room?  If not, grow it.  */
  if (nsyms > DICT_LINEAR_EXPANDABLE_CAPACITY (dict))
    {
      DICT_LINEAR_EXPANDABLE_CAPACITY (dict) *= 2;
      DICT_LINEAR_SYMS (dict)
	= XRESIZEVEC (struct symbol *, DICT_LINEAR_SYMS (dict),
		      DICT_LINEAR_EXPANDABLE_CAPACITY (dict));
    }

  DICT_LINEAR_SYM (dict, nsyms - 1) = sym;
}

/* Multi-language dictionary support.  */

/* The structure describing a multi-language dictionary.  */

struct multidictionary
{
  /* An array of dictionaries, one per language.  All dictionaries
     must be of the same type.  This should be free'd for expandable
     dictionary types.  */
  struct dictionary **dictionaries;

  /* The number of language dictionaries currently allocated.
     Only used for expandable dictionaries.  */
  unsigned short n_allocated_dictionaries;
};

/* A hasher for enum language.  Injecting this into std is a convenience
   when using unordered_map with C++11.  */

namespace std
{
  template<> struct hash<enum language>
  {
    typedef enum language argument_type;
    typedef std::size_t result_type;

    result_type operator() (const argument_type &l) const noexcept
    {
      return static_cast<result_type> (l);
    }
  };
} /* namespace std */

/* A helper function to collate symbols on the pending list by language.  */

static std::unordered_map<enum language, std::vector<symbol *>>
collate_pending_symbols_by_language (const struct pending *symbol_list)
{
  std::unordered_map<enum language, std::vector<symbol *>> nsyms;

  for (const pending *list_counter = symbol_list;
       list_counter != nullptr; list_counter = list_counter->next)
    {
      for (int i = list_counter->nsyms - 1; i >= 0; --i)
	{
	  enum language language = list_counter->symbol[i]->language ();
	  nsyms[language].push_back (list_counter->symbol[i]);
	}
    }

  return nsyms;
}

/* See dictionary.h.  */

struct multidictionary *
mdict_create_hashed (struct obstack *obstack,
		     const struct pending *symbol_list)
{
  struct multidictionary *retval
    = XOBNEW (obstack, struct multidictionary);
  std::unordered_map<enum language, std::vector<symbol *>> nsyms
    = collate_pending_symbols_by_language (symbol_list);

  /* Loop over all languages and create/populate dictionaries.  */
  retval->dictionaries
    = XOBNEWVEC (obstack, struct dictionary *, nsyms.size ());
  retval->n_allocated_dictionaries = nsyms.size ();

  int idx = 0;
  for (const auto &pair : nsyms)
    {
      enum language language = pair.first;
      std::vector<symbol *> symlist = pair.second;

      retval->dictionaries[idx++]
	= dict_create_hashed (obstack, language, symlist);
    }

  return retval;
}

/* See dictionary.h.  */

struct multidictionary *
mdict_create_hashed_expandable (enum language language)
{
  struct multidictionary *retval = XNEW (struct multidictionary);

  /* We have no symbol list to populate, but we create an empty
     dictionary of the requested language to populate later.  */
  retval->n_allocated_dictionaries = 1;
  retval->dictionaries = XNEW (struct dictionary *);
  retval->dictionaries[0] = dict_create_hashed_expandable (language);

  return retval;
}

/* See dictionary.h.  */

struct multidictionary *
mdict_create_linear (struct obstack *obstack,
		     const struct pending *symbol_list)
{
  struct multidictionary *retval
    = XOBNEW (obstack, struct multidictionary);
  std::unordered_map<enum language, std::vector<symbol *>> nsyms
    = collate_pending_symbols_by_language (symbol_list);

  /* Loop over all languages and create/populate dictionaries.  */
  retval->dictionaries
    = XOBNEWVEC (obstack, struct dictionary *, nsyms.size ());
  retval->n_allocated_dictionaries = nsyms.size ();

  int idx = 0;
  for (const auto &pair : nsyms)
    {
      enum language language = pair.first;
      std::vector<symbol *> symlist = pair.second;

      retval->dictionaries[idx++]
	= dict_create_linear (obstack, language, symlist);
    }

  return retval;
}

/* See dictionary.h.  */

struct multidictionary *
mdict_create_linear_expandable (enum language language)
{
  struct multidictionary *retval = XNEW (struct multidictionary);

  /* We have no symbol list to populate, but we create an empty
     dictionary to populate later.  */
  retval->n_allocated_dictionaries = 1;
  retval->dictionaries = XNEW (struct dictionary *);
  retval->dictionaries[0] = dict_create_linear_expandable (language);

  return retval;
}

/* See dictionary.h.  */

void
mdict_free (struct multidictionary *mdict)
{
  /* Grab the type of dictionary being used.  */
  enum dict_type type = mdict->dictionaries[0]->vector->type;

  /* Loop over all dictionaries and free them.  */
  for (unsigned short idx = 0; idx < mdict->n_allocated_dictionaries; ++idx)
    dict_free (mdict->dictionaries[idx]);

  /* Free the dictionary list, if needed.  */
  switch (type)
    {
    case DICT_HASHED:
    case DICT_LINEAR:
      /* Memory was allocated on an obstack when created.  */
      break;

    case DICT_HASHED_EXPANDABLE:
    case DICT_LINEAR_EXPANDABLE:
      xfree (mdict->dictionaries);
      break;
    }
}

/* Helper function to find the dictionary associated with LANGUAGE
   or NULL if there is no dictionary of that language.  */

static struct dictionary *
find_language_dictionary (const struct multidictionary *mdict,
			  enum language language)
{
  for (unsigned short idx = 0; idx < mdict->n_allocated_dictionaries; ++idx)
    {
      if (DICT_LANGUAGE (mdict->dictionaries[idx])->la_language == language)
	return mdict->dictionaries[idx];
    }

  return nullptr;
}

/* Create a new language dictionary for LANGUAGE and add it to the
   multidictionary MDICT's list of dictionaries.  If MDICT is not
   based on expandable dictionaries, this function throws an
   internal error.  */

static struct dictionary *
create_new_language_dictionary (struct multidictionary *mdict,
				enum language language)
{
  struct dictionary *retval = nullptr;

  /* We use the first dictionary entry to decide what create function
     to call.  Not optimal but sufficient.  */
  gdb_assert (mdict->dictionaries[0] != nullptr);
  switch (mdict->dictionaries[0]->vector->type)
    {
    case DICT_HASHED:
    case DICT_LINEAR:
      internal_error (_("create_new_language_dictionary: attempted to expand "
			"non-expandable multidictionary"));

    case DICT_HASHED_EXPANDABLE:
      retval = dict_create_hashed_expandable (language);
      break;

    case DICT_LINEAR_EXPANDABLE:
      retval = dict_create_linear_expandable (language);
      break;
    }

  /* Grow the dictionary vector and save the new dictionary.  */
  mdict->dictionaries
    = (struct dictionary **) xrealloc (mdict->dictionaries,
				       (++mdict->n_allocated_dictionaries
					* sizeof (struct dictionary *)));
  mdict->dictionaries[mdict->n_allocated_dictionaries - 1] = retval;

  return retval;
}

/* See dictionary.h.  */

void
mdict_add_symbol (struct multidictionary *mdict, struct symbol *sym)
{
  struct dictionary *dict
    = find_language_dictionary (mdict, sym->language ());

  if (dict == nullptr)
    {
      /* SYM is of a new language that we haven't previously seen.
	 Create a new dictionary for it.  */
      dict = create_new_language_dictionary (mdict, sym->language ());
    }

  dict_add_symbol (dict, sym);
}

/* See dictionary.h.  */

void
mdict_add_pending (struct multidictionary *mdict,
		   const struct pending *symbol_list)
{
  std::unordered_map<enum language, std::vector<symbol *>> nsyms
    = collate_pending_symbols_by_language (symbol_list);

  for (const auto &pair : nsyms)
    {
      enum language language = pair.first;
      std::vector<symbol *> symlist = pair.second;
      struct dictionary *dict = find_language_dictionary (mdict, language);

      if (dict == nullptr)
	{
	  /* The language was not previously seen.  Create a new dictionary
	     for it.  */
	  dict = create_new_language_dictionary (mdict, language);
	}

      dict_add_pending (dict, symlist);
    }
}

/* See dictionary.h.  */

struct symbol *
mdict_iterator_first (const multidictionary *mdict,
		      struct mdict_iterator *miterator)
{
  miterator->mdict = mdict;
  miterator->current_idx = 0;

  for (unsigned short idx = miterator->current_idx;
       idx < mdict->n_allocated_dictionaries; ++idx)
    {
      struct symbol *result
	= dict_iterator_first (mdict->dictionaries[idx], &miterator->iterator);

      if (result != nullptr)
	{
	  miterator->current_idx = idx;
	  return result;
	}
    }

  return nullptr;
}

/* See dictionary.h.  */

struct symbol *
mdict_iterator_next (struct mdict_iterator *miterator)
{
  struct symbol *result = dict_iterator_next (&miterator->iterator);

  if (result != nullptr)
    return result;

  /* The current dictionary had no matches -- move to the next
     dictionary, if any.  */
  for (unsigned short idx = ++miterator->current_idx;
       idx < miterator->mdict->n_allocated_dictionaries; ++idx)
    {
      result
	= dict_iterator_first (miterator->mdict->dictionaries[idx],
			       &miterator->iterator);
      if (result != nullptr)
	{
	  miterator->current_idx = idx;
	  return result;
	}
    }

  return nullptr;
}

/* See dictionary.h.  */

struct symbol *
mdict_iter_match_first (const struct multidictionary *mdict,
			const lookup_name_info &name,
			struct mdict_iterator *miterator)
{
  miterator->mdict = mdict;
  miterator->current_idx = 0;

  for (unsigned short idx = miterator->current_idx;
       idx < mdict->n_allocated_dictionaries; ++idx)
    {
      struct symbol *result
	= dict_iter_match_first (mdict->dictionaries[idx], name,
				 &miterator->iterator);

      if (result != nullptr)
	return result;
    }

  return nullptr;
}

/* See dictionary.h.  */

struct symbol *
mdict_iter_match_next (const lookup_name_info &name,
		       struct mdict_iterator *miterator)
{
  /* Search the current dictionary.  */
  struct symbol *result = dict_iter_match_next (name, &miterator->iterator);

  if (result != nullptr)
    return result;

  /* The current dictionary had no matches -- move to the next
     dictionary, if any.  */
  for (unsigned short idx = ++miterator->current_idx;
       idx < miterator->mdict->n_allocated_dictionaries; ++idx)
    {
      result
	= dict_iter_match_first (miterator->mdict->dictionaries[idx],
				 name, &miterator->iterator);
      if (result != nullptr)
	{
	  miterator->current_idx = idx;
	  return result;
	}
    }

  return nullptr;
}

/* See dictionary.h.  */

int
mdict_size (const struct multidictionary *mdict)
{
  int size = 0;

  for (unsigned short idx = 0; idx < mdict->n_allocated_dictionaries; ++idx)
    size += dict_size (mdict->dictionaries[idx]);

  return size;
}
