/* addrmap.c --- implementation of address map data structure.

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

#include "defs.h"
#include "gdbsupport/gdb_obstack.h"
#include "addrmap.h"
#include "gdbsupport/selftest.h"

/* Make sure splay trees can actually hold the values we want to
   store in them.  */
static_assert (sizeof (splay_tree_key) >= sizeof (CORE_ADDR *));
static_assert (sizeof (splay_tree_value) >= sizeof (void *));


/* Fixed address maps.  */

void
addrmap_fixed::set_empty (CORE_ADDR start, CORE_ADDR end_inclusive,
			  void *obj)
{
  internal_error ("addrmap_fixed_set_empty: "
		  "fixed addrmaps can't be changed\n");
}


void *
addrmap_fixed::do_find (CORE_ADDR addr) const
{
  const struct addrmap_transition *bottom = &transitions[0];
  const struct addrmap_transition *top = &transitions[num_transitions - 1];

  while (bottom < top)
    {
      /* This needs to round towards top, or else when top = bottom +
	 1 (i.e., two entries are under consideration), then mid ==
	 bottom, and then we may not narrow the range when (mid->addr
	 < addr).  */
      const addrmap_transition *mid = top - (top - bottom) / 2;

      if (mid->addr == addr)
	{
	  bottom = mid;
	  break;
	}
      else if (mid->addr < addr)
	/* We don't eliminate mid itself here, since each transition
	   covers all subsequent addresses until the next.  This is why
	   we must round up in computing the midpoint.  */
	bottom = mid;
      else
	top = mid - 1;
    }

  return bottom->value;
}


void
addrmap_fixed::relocate (CORE_ADDR offset)
{
  size_t i;

  for (i = 0; i < num_transitions; i++)
    transitions[i].addr += offset;
}


int
addrmap_fixed::do_foreach (addrmap_foreach_fn fn) const
{
  size_t i;

  for (i = 0; i < num_transitions; i++)
    {
      int res = fn (transitions[i].addr, transitions[i].value);

      if (res != 0)
	return res;
    }

  return 0;
}



/* Mutable address maps.  */

/* Allocate a copy of CORE_ADDR.  */
splay_tree_key
addrmap_mutable::allocate_key (CORE_ADDR addr)
{
  CORE_ADDR *key = XNEW (CORE_ADDR);

  *key = addr;
  return (splay_tree_key) key;
}


/* Type-correct wrappers for splay tree access.  */
splay_tree_node
addrmap_mutable::splay_tree_lookup (CORE_ADDR addr) const
{
  return ::splay_tree_lookup (tree, (splay_tree_key) &addr);
}


splay_tree_node
addrmap_mutable::splay_tree_predecessor (CORE_ADDR addr) const
{
  return ::splay_tree_predecessor (tree, (splay_tree_key) &addr);
}


splay_tree_node
addrmap_mutable::splay_tree_successor (CORE_ADDR addr)
{
  return ::splay_tree_successor (tree, (splay_tree_key) &addr);
}


void
addrmap_mutable::splay_tree_remove (CORE_ADDR addr)
{
  ::splay_tree_remove (tree, (splay_tree_key) &addr);
}


static CORE_ADDR
addrmap_node_key (splay_tree_node node)
{
  return * (CORE_ADDR *) node->key;
}


static void *
addrmap_node_value (splay_tree_node node)
{
  return (void *) node->value;
}


static void
addrmap_node_set_value (splay_tree_node node, void *value)
{
  node->value = (splay_tree_value) value;
}


void
addrmap_mutable::splay_tree_insert (CORE_ADDR key, void *value)
{
  ::splay_tree_insert (tree,
		       allocate_key (key),
		       (splay_tree_value) value);
}


/* Without changing the mapping of any address, ensure that there is a
   tree node at ADDR, even if it would represent a "transition" from
   one value to the same value.  */
void
addrmap_mutable::force_transition (CORE_ADDR addr)
{
  splay_tree_node n = splay_tree_lookup (addr);

  if (! n)
    {
      n = splay_tree_predecessor (addr);
      splay_tree_insert (addr, n ? addrmap_node_value (n) : NULL);
    }
}


void
addrmap_mutable::set_empty (CORE_ADDR start, CORE_ADDR end_inclusive,
			    void *obj)
{
  splay_tree_node n, next;
  void *prior_value;

  /* If we're being asked to set all empty portions of the given
     address range to empty, then probably the caller is confused.
     (If that turns out to be useful in some cases, then we can change
     this to simply return, since overriding NULL with NULL is a
     no-op.)  */
  gdb_assert (obj);

  /* We take a two-pass approach, for simplicity.
     - Establish transitions where we think we might need them.
     - First pass: change all NULL regions to OBJ.
     - Second pass: remove any unnecessary transitions.  */

  /* Establish transitions at the start and end.  */
  force_transition (start);
  if (end_inclusive < CORE_ADDR_MAX)
    force_transition (end_inclusive + 1);

  /* Walk the area, changing all NULL regions to OBJ.  */
  for (n = splay_tree_lookup (start), gdb_assert (n);
       n && addrmap_node_key (n) <= end_inclusive;
       n = splay_tree_successor (addrmap_node_key (n)))
    {
      if (! addrmap_node_value (n))
	addrmap_node_set_value (n, obj);
    }

  /* Walk the area again, removing transitions from any value to
     itself.  Be sure to visit both the transitions we forced
     above.  */
  n = splay_tree_predecessor (start);
  prior_value = n ? addrmap_node_value (n) : NULL;
  for (n = splay_tree_lookup (start), gdb_assert (n);
       n && (end_inclusive == CORE_ADDR_MAX
	     || addrmap_node_key (n) <= end_inclusive + 1);
       n = next)
    {
      next = splay_tree_successor (addrmap_node_key (n));
      if (addrmap_node_value (n) == prior_value)
	splay_tree_remove (addrmap_node_key (n));
      else
	prior_value = addrmap_node_value (n);
    }
}


void *
addrmap_mutable::do_find (CORE_ADDR addr) const
{
  splay_tree_node n = splay_tree_lookup (addr);
  if (n != nullptr)
    {
      gdb_assert (addrmap_node_key (n) == addr);
      return addrmap_node_value (n);
    }

  n = splay_tree_predecessor (addr);
  if (n != nullptr)
    {
      gdb_assert (addrmap_node_key (n) < addr);
      return addrmap_node_value (n);
    }

  return nullptr;
}


addrmap_fixed::addrmap_fixed (struct obstack *obstack, addrmap_mutable *mut)
{
  size_t transition_count = 0;

  /* Count the number of transitions in the tree.  */
  mut->foreach ([&] (CORE_ADDR start, void *obj)
    {
      ++transition_count;
      return 0;
    });

  /* Include an extra entry for the transition at zero (which fixed
     maps have, but mutable maps do not.)  */
  transition_count++;

  num_transitions = 1;
  transitions = XOBNEWVEC (obstack, struct addrmap_transition,
			   transition_count);
  transitions[0].addr = 0;
  transitions[0].value = NULL;

  /* Copy all entries from the splay tree to the array, in order 
     of increasing address.  */
  mut->foreach ([&] (CORE_ADDR start, void *obj)
    {
      transitions[num_transitions].addr = start;
      transitions[num_transitions].value = obj;
      ++num_transitions;
      return 0;
    });

  /* We should have filled the array.  */
  gdb_assert (num_transitions == transition_count);
}


void
addrmap_mutable::relocate (CORE_ADDR offset)
{
  /* Not needed yet.  */
  internal_error (_("addrmap_relocate is not implemented yet "
		    "for mutable addrmaps"));
}


/* This is a splay_tree_foreach_fn.  */

static int
addrmap_mutable_foreach_worker (splay_tree_node node, void *data)
{
  addrmap_foreach_fn *fn = (addrmap_foreach_fn *) data;

  return (*fn) (addrmap_node_key (node), addrmap_node_value (node));
}


int
addrmap_mutable::do_foreach (addrmap_foreach_fn fn) const
{
  return splay_tree_foreach (tree, addrmap_mutable_foreach_worker, &fn);
}


/* Compare keys as CORE_ADDR * values.  */
static int
splay_compare_CORE_ADDR_ptr (splay_tree_key ak, splay_tree_key bk)
{
  CORE_ADDR a = * (CORE_ADDR *) ak;
  CORE_ADDR b = * (CORE_ADDR *) bk;

  /* We can't just return a-b here, because of over/underflow.  */
  if (a < b)
    return -1;
  else if (a == b)
    return 0;
  else
    return 1;
}


static void
xfree_wrapper (splay_tree_key key)
{
  xfree ((void *) key);
}

addrmap_mutable::addrmap_mutable ()
  : tree (splay_tree_new (splay_compare_CORE_ADDR_ptr, xfree_wrapper,
			  nullptr /* no delete value */))
{
}

addrmap_mutable::~addrmap_mutable ()
{
  splay_tree_delete (tree);
}


/* See addrmap.h.  */

void
addrmap_dump (struct addrmap *map, struct ui_file *outfile, void *payload)
{
  /* True if the previously printed addrmap entry was for PAYLOAD.
     If so, we want to print the next one as well (since the next
     addrmap entry defines the end of the range).  */
  bool previous_matched = false;

  auto callback = [&] (CORE_ADDR start_addr, const void *obj)
  {
    QUIT;

    bool matches = payload == nullptr || payload == obj;
    const char *addr_str = nullptr;
    if (matches)
      addr_str = host_address_to_string (obj);
    else if (previous_matched)
      addr_str = "<ends here>";

    if (matches || previous_matched)
      gdb_printf (outfile, "  %s%s %s\n",
		  payload != nullptr ? "  " : "",
		  core_addr_to_string (start_addr),
		  addr_str);

    previous_matched = matches;

    return 0;
  };

  map->foreach (callback);
}

#if GDB_SELF_TEST
namespace selftests {

/* Convert P to CORE_ADDR.  */

static CORE_ADDR
core_addr (void *p)
{
  return (CORE_ADDR)(uintptr_t)p;
}

/* Check that &ARRAY[LOW]..&ARRAY[HIGH] has VAL in MAP.  */

#define CHECK_ADDRMAP_FIND(MAP, ARRAY, LOW, HIGH, VAL)			\
  do									\
    {									\
      for (unsigned i = LOW; i <= HIGH; ++i)				\
	SELF_CHECK (MAP->find (core_addr (&ARRAY[i])) == VAL);		\
    }									\
  while (0)

/* Entry point for addrmap unit tests.  */

static void
test_addrmap ()
{
  /* We'll verify using the addresses of the elements of this array.  */
  char array[20];

  /* We'll verify using these values stored into the map.  */
  void *val1 = &array[1];
  void *val2 = &array[2];

  /* Create mutable addrmap.  */
  auto_obstack temp_obstack;
  auto map = std::make_unique<struct addrmap_mutable> ();
  SELF_CHECK (map != nullptr);

  /* Check initial state.  */
  CHECK_ADDRMAP_FIND (map, array, 0, 19, nullptr);

  /* Insert address range into mutable addrmap.  */
  map->set_empty (core_addr (&array[10]), core_addr (&array[12]), val1);
  CHECK_ADDRMAP_FIND (map, array, 0, 9, nullptr);
  CHECK_ADDRMAP_FIND (map, array, 10, 12, val1);
  CHECK_ADDRMAP_FIND (map, array, 13, 19, nullptr);

  /* Create corresponding fixed addrmap.  */
  struct addrmap *map2
    = new (&temp_obstack) addrmap_fixed (&temp_obstack, map.get ());
  SELF_CHECK (map2 != nullptr);
  CHECK_ADDRMAP_FIND (map2, array, 0, 9, nullptr);
  CHECK_ADDRMAP_FIND (map2, array, 10, 12, val1);
  CHECK_ADDRMAP_FIND (map2, array, 13, 19, nullptr);

  /* Iterate over both addrmaps.  */
  auto callback = [&] (CORE_ADDR start_addr, void *obj)
    {
      if (start_addr == core_addr (nullptr))
	SELF_CHECK (obj == nullptr);
      else if (start_addr == core_addr (&array[10]))
	SELF_CHECK (obj == val1);
      else if (start_addr == core_addr (&array[13]))
	SELF_CHECK (obj == nullptr);
      else
	SELF_CHECK (false);
      return 0;
    };
  SELF_CHECK (map->foreach (callback) == 0);
  SELF_CHECK (map2->foreach (callback) == 0);

  /* Relocate fixed addrmap.  */
  map2->relocate (1);
  CHECK_ADDRMAP_FIND (map2, array, 0, 10, nullptr);
  CHECK_ADDRMAP_FIND (map2, array, 11, 13, val1);
  CHECK_ADDRMAP_FIND (map2, array, 14, 19, nullptr);

  /* Insert partially overlapping address range into mutable addrmap.  */
  map->set_empty (core_addr (&array[11]), core_addr (&array[13]), val2);
  CHECK_ADDRMAP_FIND (map, array, 0, 9, nullptr);
  CHECK_ADDRMAP_FIND (map, array, 10, 12, val1);
  CHECK_ADDRMAP_FIND (map, array, 13, 13, val2);
  CHECK_ADDRMAP_FIND (map, array, 14, 19, nullptr);
}

} // namespace selftests
#endif /* GDB_SELF_TEST */

void _initialize_addrmap ();
void
_initialize_addrmap ()
{
#if GDB_SELF_TEST
  selftests::register_test ("addrmap", selftests::test_addrmap);
#endif /* GDB_SELF_TEST */
}
