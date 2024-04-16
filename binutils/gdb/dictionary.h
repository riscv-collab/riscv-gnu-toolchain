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

#ifndef DICTIONARY_H
#define DICTIONARY_H

#include "symfile.h"

/* An opaque type for multi-language dictionaries; only dictionary.c should
   know about its innards.  */

struct multidictionary;

/* Other types needed for declarations.  */

struct symbol;
struct obstack;
struct pending;
struct language_defn;

/* The creation functions for various implementations of
   multi-language dictionaries.  */

/* Create a multi-language dictionary of symbols implemented via
   a fixed-size hashtable.  All memory it uses is allocated on
   OBSTACK; the environment is initialized from SYMBOL_LIST.  */

extern struct multidictionary *
  mdict_create_hashed (struct obstack *obstack,
		       const struct pending *symbol_list);

/* Create a multi-language dictionary of symbols, implemented
   via a hashtable that grows as necessary.  The initial dictionary of
   LANGUAGE is empty; to add symbols to it, call mdict_add_symbol().
   Call mdict_free() when you're done with it.  */

extern struct multidictionary *
  mdict_create_hashed_expandable (enum language language);

/* Create a multi-language dictionary of symbols, implemented
   via a fixed-size array.  All memory it uses is allocated on
   OBSTACK; the environment is initialized from the SYMBOL_LIST.  The
   symbols are ordered in the same order that they're found in
   SYMBOL_LIST.  */

extern struct multidictionary *
  mdict_create_linear (struct obstack *obstack,
		       const struct pending *symbol_list);

/* Create a multi-language dictionary of symbols, implemented
   via an array that grows as necessary.  The multidictionary initially
   contains a single empty dictionary of LANGUAGE; to add symbols to it,
   call mdict_add_symbol().  Call mdict_free() when you're done with it.  */

extern struct multidictionary *
  mdict_create_linear_expandable (enum language language);

/* The functions providing the interface to multi-language dictionaries.
   Note that the most common parts of the interface, namely symbol lookup,
   are only provided via iterator functions.  */

/* Free the memory used by a multidictionary that's not on an obstack.  (If
   any.)  */

extern void mdict_free (struct multidictionary *mdict);

/* Add a symbol to an expandable multidictionary.  */

extern void mdict_add_symbol (struct multidictionary *mdict,
			      struct symbol *sym);

/* Utility to add a list of symbols to a multidictionary.  */

extern void mdict_add_pending (struct multidictionary *mdict,
			       const struct pending *symbol_list);

/* A type containing data that is used when iterating over all symbols
   in a dictionary.  Don't ever look at its innards; this type would
   be opaque if we didn't need to be able to allocate it on the
   stack.  */

struct dict_iterator
{
  /* The dictionary that this iterator is associated to.  */
  const struct dictionary *dict;
  /* The next two members are data that is used in a way that depends
     on DICT's implementation type.  */
  int index;
  struct symbol *current;
};

/* The multi-language dictionary iterator.  Like dict_iterator above,
   these contents should be considered private.  */

struct mdict_iterator
{
  /* The multidictionary with whcih this iterator is associated.  */
  const struct multidictionary *mdict;

  /* The iterator used to iterate through individual dictionaries.  */
  struct dict_iterator iterator;

  /* The current index of the dictionary being iterated over.  */
  unsigned short current_idx;
};

/* Initialize ITERATOR to point at the first symbol in MDICT, and
   return that first symbol, or NULL if MDICT is empty.  */

extern struct symbol *
  mdict_iterator_first (const struct multidictionary *mdict,
			struct mdict_iterator *miterator);

/* Advance MITERATOR, and return the next symbol, or NULL if there are
   no more symbols.  Don't call this if you've previously received
   NULL from mdict_iterator_first or mdict_iterator_next on this
   iteration.  */

extern struct symbol *mdict_iterator_next (struct mdict_iterator *miterator);

/* Initialize MITERATOR to point at the first symbol in MDICT whose
   search_name () is NAME, as tested using COMPARE (which must use
   the same conventions as strcmp_iw and be compatible with any
   dictionary hashing function), and return that first symbol, or NULL
   if there are no such symbols.  */

extern struct symbol *
  mdict_iter_match_first (const struct multidictionary *mdict,
			  const lookup_name_info &name,
			  struct mdict_iterator *miterator);

/* Advance MITERATOR to point at the next symbol in MDICT whose
   search_name () is NAME, as tested using COMPARE (see
   dict_iter_match_first), or NULL if there are no more such symbols.
   Don't call this if you've previously received NULL from 
   mdict_iterator_match_first or mdict_iterator_match_next on this
   iteration.  And don't call it unless MITERATOR was created by a
   previous call to mdict_iter_match_first with the same NAME and COMPARE.  */

extern struct symbol *mdict_iter_match_next (const lookup_name_info &name,
					     struct mdict_iterator *miterator);

/* Return the number of symbols in multidictionary MDICT.  */

extern int mdict_size (const struct multidictionary *mdict);

/* An iterator that wraps an mdict_iterator.  The naming here is
   unfortunate, but mdict_iterator was named before gdb switched to
   C++.  */
struct mdict_iterator_wrapper
{
  typedef mdict_iterator_wrapper self_type;
  typedef struct symbol *value_type;

  explicit mdict_iterator_wrapper (const struct multidictionary *mdict)
    : m_sym (mdict_iterator_first (mdict, &m_iter))
  {
  }

  mdict_iterator_wrapper ()
    : m_sym (nullptr)
  {
  }

  value_type operator* () const
  {
    return m_sym;
  }

  bool operator== (const self_type &other) const
  {
    return m_sym == other.m_sym;
  }

  bool operator!= (const self_type &other) const
  {
    return m_sym != other.m_sym;
  }

  self_type &operator++ ()
  {
    m_sym = mdict_iterator_next (&m_iter);
    return *this;
  }

private:

  struct symbol *m_sym;
  struct mdict_iterator m_iter;
};

#endif /* DICTIONARY_H */
