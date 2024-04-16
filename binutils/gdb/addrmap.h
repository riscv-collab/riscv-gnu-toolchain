/* addrmap.h --- interface to address map data structure.

   Copyright (C) 2007-2024 Free Software Foundation, Inc.

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

#ifndef ADDRMAP_H
#define ADDRMAP_H

#include "splay-tree.h"
#include "gdbsupport/function-view.h"

/* An address map is essentially a table mapping CORE_ADDRs onto GDB
   data structures, like blocks, symtabs, partial symtabs, and so on.
   An address map uses memory proportional to the number of
   transitions in the map, where a CORE_ADDR N is mapped to one
   object, and N+1 is mapped to a different object.

   Address maps come in two flavors: fixed, and mutable.  Mutable
   address maps consume more memory, but can be changed and extended.
   A fixed address map, once constructed (from a mutable address map),
   can't be edited.  */

/* The type of a function used to iterate over the map.
   OBJ is NULL for unmapped regions.  */
using addrmap_foreach_fn
  = gdb::function_view<int (CORE_ADDR start_addr, void *obj)>;
using addrmap_foreach_const_fn
  = gdb::function_view<int (CORE_ADDR start_addr, const void *obj)>;

/* The base class for addrmaps.  */
struct addrmap
{
  virtual ~addrmap () = default;

  /* In the mutable address map MAP, associate the addresses from START
     to END_INCLUSIVE that are currently associated with NULL with OBJ
     instead.  Addresses mapped to an object other than NULL are left
     unchanged.

     As the name suggests, END_INCLUSIVE is also mapped to OBJ.  This
     convention is unusual, but it allows callers to accurately specify
     ranges that abut the top of the address space, and ranges that
     cover the entire address space.

     This operation seems a bit complicated for a primitive: if it's
     needed, why not just have a simpler primitive operation that sets a
     range to a value, wiping out whatever was there before, and then
     let the caller construct more complicated operations from that,
     along with some others for traversal?

     It turns out this is the mutation operation we want to use all the
     time, at least for now.  Our immediate use for address maps is to
     represent lexical blocks whose address ranges are not contiguous.
     We walk the tree of lexical blocks present in the debug info, and
     only create 'struct block' objects after we've traversed all a
     block's children.  If a lexical block declares no local variables
     (and isn't the lexical block for a function's body), we omit it
     from GDB's data structures entirely.

     However, this means that we don't decide to create a block (and
     thus record it in the address map) until after we've traversed its
     children.  If we do decide to create the block, we do so at a time
     when all its children have already been recorded in the map.  So
     this operation --- change only those addresses left unset --- is
     actually the operation we want to use every time.

     It seems simpler to let the code which operates on the
     representation directly deal with the hair of implementing these
     semantics than to provide an interface which allows it to be
     implemented efficiently, but doesn't reveal too much of the
     representation.  */
  virtual void set_empty (CORE_ADDR start, CORE_ADDR end_inclusive,
			  void *obj) = 0;

  /* Return the object associated with ADDR in MAP.  */
  const void *find (CORE_ADDR addr) const
  { return this->do_find (addr); }

  void *find (CORE_ADDR addr)
  { return this->do_find (addr); }

  /* Relocate all the addresses in MAP by OFFSET.  (This can be applied
     to either mutable or immutable maps.)  */
  virtual void relocate (CORE_ADDR offset) = 0;

  /* Call FN for every address in MAP, following an in-order traversal.
     If FN ever returns a non-zero value, the iteration ceases
     immediately, and the value is returned.  Otherwise, this function
     returns 0.  */
  int foreach (addrmap_foreach_const_fn fn) const
  { return this->do_foreach (fn); }

  int foreach (addrmap_foreach_fn fn)
  { return this->do_foreach (fn); }


private:
  /* Worker for find, implemented by sub-classes.  */
  virtual void *do_find (CORE_ADDR addr) const = 0;

  /* Worker for foreach, implemented by sub-classes.  */
  virtual int do_foreach (addrmap_foreach_fn fn) const = 0;
};

struct addrmap_mutable;

/* Fixed address maps.  */
struct addrmap_fixed : public addrmap,
		       public allocate_on_obstack
{
public:

  addrmap_fixed (struct obstack *obstack, addrmap_mutable *mut);
  DISABLE_COPY_AND_ASSIGN (addrmap_fixed);

  void set_empty (CORE_ADDR start, CORE_ADDR end_inclusive,
		  void *obj) override;
  void relocate (CORE_ADDR offset) override;

private:
  void *do_find (CORE_ADDR addr) const override;
  int do_foreach (addrmap_foreach_fn fn) const override;

  /* A transition: a point in an address map where the value changes.
     The map maps ADDR to VALUE, but if ADDR > 0, it maps ADDR-1 to
     something else.  */
  struct addrmap_transition
  {
    CORE_ADDR addr;
    void *value;
  };

  /* The number of transitions in TRANSITIONS.  */
  size_t num_transitions;

  /* An array of transitions, sorted by address.  For every point in
     the map where either ADDR == 0 or ADDR is mapped to one value and
     ADDR - 1 is mapped to something different, we have an entry here
     containing ADDR and VALUE.  (Note that this means we always have
     an entry for address 0).  */
  struct addrmap_transition *transitions;
};

/* Mutable address maps.  */

struct addrmap_mutable : public addrmap
{
public:

  addrmap_mutable ();
  ~addrmap_mutable ();
  DISABLE_COPY_AND_ASSIGN (addrmap_mutable);

  void set_empty (CORE_ADDR start, CORE_ADDR end_inclusive,
		  void *obj) override;
  void relocate (CORE_ADDR offset) override;

private:
  void *do_find (CORE_ADDR addr) const override;
  int do_foreach (addrmap_foreach_fn fn) const override;

  /* A splay tree, with a node for each transition; there is a
     transition at address T if T-1 and T map to different objects.

     Any addresses below the first node map to NULL.  (Unlike
     fixed maps, we have no entry at (CORE_ADDR) 0; it doesn't 
     simplify enough.)

     The last region is assumed to end at CORE_ADDR_MAX.

     Since we can't know whether CORE_ADDR is larger or smaller than
     splay_tree_key (unsigned long) --- I think both are possible,
     given all combinations of 32- and 64-bit hosts and targets ---
     our keys are pointers to CORE_ADDR values.  Since the splay tree
     library doesn't pass any closure pointer to the key free
     function, we can't keep a freelist for keys.  Since mutable
     addrmaps are only used temporarily right now, we just leak keys
     from deleted nodes; they'll be freed when the obstack is freed.  */
  splay_tree tree;

  /* Various helper methods.  */
  splay_tree_key allocate_key (CORE_ADDR addr);
  void force_transition (CORE_ADDR addr);
  splay_tree_node splay_tree_lookup (CORE_ADDR addr) const;
  splay_tree_node splay_tree_predecessor (CORE_ADDR addr) const;
  splay_tree_node splay_tree_successor (CORE_ADDR addr);
  void splay_tree_remove (CORE_ADDR addr);
  void splay_tree_insert (CORE_ADDR key, void *value);
};


/* Dump the addrmap to OUTFILE.  If PAYLOAD is non-NULL, only dump any
   components that map to PAYLOAD.  (If PAYLOAD is NULL, the entire
   map is dumped.)  */
void addrmap_dump (struct addrmap *map, struct ui_file *outfile,
		   void *payload);

#endif /* ADDRMAP_H */
