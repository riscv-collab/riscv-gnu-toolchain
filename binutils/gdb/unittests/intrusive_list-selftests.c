/* Tests fpr intrusive double linked list for GDB, the GNU debugger.
   Copyright (C) 2021-2024 Free Software Foundation, Inc.

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

#include "gdbsupport/intrusive_list.h"
#include "gdbsupport/selftest.h"
#include <unordered_set>

/* An item type using intrusive_list_node by inheriting from it and its
   corresponding list type.  Put another base before intrusive_list_node
   so that a pointer to the node != a pointer to the item.  */

struct other_base
{
  int n = 1;
};

struct item_with_base : public other_base,
			public intrusive_list_node<item_with_base>
{
  explicit item_with_base (const char *name)
    : name (name)
  {}

  const char *const name;
};

using item_with_base_list = intrusive_list<item_with_base>;

/* An item type using intrusive_list_node as a field and its corresponding
   list type.  Put the other field before the node, so that a pointer to the
   node != a pointer to the item.  */

struct item_with_member
{
  explicit item_with_member (const char *name)
    : name (name)
  {}

  const char *const name;
  intrusive_list_node<item_with_member> node;
};

using item_with_member_node
  = intrusive_member_node<item_with_member, &item_with_member::node>;
using item_with_member_list
  = intrusive_list<item_with_member, item_with_member_node>;

/* To run all tests using both the base and member methods, all tests are
   declared in this templated class, which is instantiated once for each
   list type.  */

template <typename ListType>
struct intrusive_list_test
{
  using item_type = typename ListType::value_type;

  /* Verify that LIST contains exactly the items in EXPECTED.

     Traverse the list forward and backwards to exercise all links.  */

  static void
  verify_items (const ListType &list,
		gdb::array_view<const typename ListType::value_type *> expected)
  {
    int i = 0;

    for (typename ListType::iterator it = list.begin ();
	 it != list.end ();
	 ++it)
      {
	const item_type &item = *it;

	SELF_CHECK (i < expected.size ());
	SELF_CHECK (&item == expected[i]);

	++i;
      }

    SELF_CHECK (i == expected.size ());

    for (typename ListType::reverse_iterator it = list.rbegin ();
	 it != list.rend ();
	 ++it)
      {
	const item_type &item = *it;

	--i;

	SELF_CHECK (i >= 0);
	SELF_CHECK (&item == expected[i]);
      }

    SELF_CHECK (i == 0);
  }

  static void
  test_move_constructor ()
  {
    {
      /* Other list is not empty.  */
      item_type a ("a"), b ("b"), c ("c");
      ListType list1;
      std::vector<const item_type *> expected;

      list1.push_back (a);
      list1.push_back (b);
      list1.push_back (c);

      ListType list2 (std::move (list1));

      expected = {};
      verify_items (list1, expected);

      expected = {&a, &b, &c};
      verify_items (list2, expected);
    }

    {
      /* Other list contains 1 element.  */
      item_type a ("a");
      ListType list1;
      std::vector<const item_type *> expected;

      list1.push_back (a);

      ListType list2 (std::move (list1));

      expected = {};
      verify_items (list1, expected);

      expected = {&a};
      verify_items (list2, expected);
    }

    {
      /* Other list is empty.  */
      ListType list1;
      std::vector<const item_type *> expected;

      ListType list2 (std::move (list1));

      expected = {};
      verify_items (list1, expected);

      expected = {};
      verify_items (list2, expected);
    }
  }

  static void
  test_move_assignment ()
  {
    {
      /* Both lists are not empty.  */
      item_type a ("a"), b ("b"), c ("c"), d ("d"), e ("e");
      ListType list1;
      ListType list2;
      std::vector<const item_type *> expected;

      list1.push_back (a);
      list1.push_back (b);
      list1.push_back (c);

      list2.push_back (d);
      list2.push_back (e);

      list2 = std::move (list1);

      expected = {};
      verify_items (list1, expected);

      expected = {&a, &b, &c};
      verify_items (list2, expected);
    }

    {
      /* rhs list is empty.  */
      item_type a ("a"), b ("b"), c ("c");
      ListType list1;
      ListType list2;
      std::vector<const item_type *> expected;

      list2.push_back (a);
      list2.push_back (b);
      list2.push_back (c);

      list2 = std::move (list1);

      expected = {};
      verify_items (list1, expected);

      expected = {};
      verify_items (list2, expected);
    }

    {
      /* lhs list is empty.  */
      item_type a ("a"), b ("b"), c ("c");
      ListType list1;
      ListType list2;
      std::vector<const item_type *> expected;

      list1.push_back (a);
      list1.push_back (b);
      list1.push_back (c);

      list2 = std::move (list1);

      expected = {};
      verify_items (list1, expected);

      expected = {&a, &b, &c};
      verify_items (list2, expected);
    }

    {
      /* Both lists contain 1 item.  */
      item_type a ("a"), b ("b");
      ListType list1;
      ListType list2;
      std::vector<const item_type *> expected;

      list1.push_back (a);
      list2.push_back (b);

      list2 = std::move (list1);

      expected = {};
      verify_items (list1, expected);

      expected = {&a};
      verify_items (list2, expected);
    }

    {
      /* Both lists are empty.  */
      ListType list1;
      ListType list2;
      std::vector<const item_type *> expected;

      list2 = std::move (list1);

      expected = {};
      verify_items (list1, expected);

      expected = {};
      verify_items (list2, expected);
    }
  }

  static void
  test_swap ()
  {
    {
      /* Two non-empty lists.  */
      item_type a ("a"), b ("b"), c ("c"), d ("d"), e ("e");
      ListType list1;
      ListType list2;
      std::vector<const item_type *> expected;

      list1.push_back (a);
      list1.push_back (b);
      list1.push_back (c);

      list2.push_back (d);
      list2.push_back (e);

      std::swap (list1, list2);

      expected = {&d, &e};
      verify_items (list1, expected);

      expected = {&a, &b, &c};
      verify_items (list2, expected);
    }

    {
      /* Other is empty.  */
      item_type a ("a"), b ("b"), c ("c");
      ListType list1;
      ListType list2;
      std::vector<const item_type *> expected;

      list1.push_back (a);
      list1.push_back (b);
      list1.push_back (c);

      std::swap (list1, list2);

      expected = {};
      verify_items (list1, expected);

      expected = {&a, &b, &c};
      verify_items (list2, expected);
    }

    {
      /* *this is empty.  */
      item_type a ("a"), b ("b"), c ("c");
      ListType list1;
      ListType list2;
      std::vector<const item_type *> expected;

      list2.push_back (a);
      list2.push_back (b);
      list2.push_back (c);

      std::swap (list1, list2);

      expected = {&a, &b, &c};
      verify_items (list1, expected);

      expected = {};
      verify_items (list2, expected);
    }

    {
      /* Both lists empty.  */
      ListType list1;
      ListType list2;
      std::vector<const item_type *> expected;

      std::swap (list1, list2);

      expected = {};
      verify_items (list1, expected);

      expected = {};
      verify_items (list2, expected);
    }

    {
      /* Swap one element twice.  */
      item_type a ("a");
      ListType list1;
      ListType list2;
      std::vector<const item_type *> expected;

      list1.push_back (a);

      std::swap (list1, list2);

      expected = {};
      verify_items (list1, expected);

      expected = {&a};
      verify_items (list2, expected);

      std::swap (list1, list2);

      expected = {&a};
      verify_items (list1, expected);

      expected = {};
      verify_items (list2, expected);
    }
  }

  static void
  test_front_back ()
  {
    item_type a ("a"), b ("b"), c ("c");
    ListType list;
    const ListType &clist = list;

    list.push_back (a);
    list.push_back (b);
    list.push_back (c);

    SELF_CHECK (&list.front () == &a);
    SELF_CHECK (&clist.front () == &a);
    SELF_CHECK (&list.back () == &c);
    SELF_CHECK (&clist.back () == &c);
  }

  static void
  test_push_front ()
  {
    item_type a ("a"), b ("b"), c ("c");
    ListType list;
    std::vector<const item_type *> expected;

    expected = {};
    verify_items (list, expected);

    list.push_front (a);
    expected = {&a};
    verify_items (list, expected);

    list.push_front (b);
    expected = {&b, &a};
    verify_items (list, expected);

    list.push_front (c);
    expected = {&c, &b, &a};
    verify_items (list, expected);
  }

  static void
  test_push_back ()
  {
    item_type a ("a"), b ("b"), c ("c");
    ListType list;
    std::vector<const item_type *> expected;

    expected = {};
    verify_items (list, expected);

    list.push_back (a);
    expected = {&a};
    verify_items (list, expected);

    list.push_back (b);
    expected = {&a, &b};
    verify_items (list, expected);

    list.push_back (c);
    expected = {&a, &b, &c};
    verify_items (list, expected);
  }

  static void
  test_insert ()
  {
    std::vector<const item_type *> expected;

    {
      /* Insert at beginning.  */
      item_type a ("a"), b ("b"), c ("c");
      ListType list;


      list.insert (list.begin (), a);
      expected = {&a};
      verify_items (list, expected);

      list.insert (list.begin (), b);
      expected = {&b, &a};
      verify_items (list, expected);

      list.insert (list.begin (), c);
      expected = {&c, &b, &a};
      verify_items (list, expected);
    }

    {
      /* Insert at end.  */
      item_type a ("a"), b ("b"), c ("c");
      ListType list;


      list.insert (list.end (), a);
      expected = {&a};
      verify_items (list, expected);

      list.insert (list.end (), b);
      expected = {&a, &b};
      verify_items (list, expected);

      list.insert (list.end (), c);
      expected = {&a, &b, &c};
      verify_items (list, expected);
    }

    {
      /* Insert in the middle.  */
      item_type a ("a"), b ("b"), c ("c");
      ListType list;

      list.push_back (a);
      list.push_back (b);

      list.insert (list.iterator_to (b), c);
      expected = {&a, &c, &b};
      verify_items (list, expected);
    }

    {
      /* Insert in empty list. */
      item_type a ("a");
      ListType list;

      list.insert (list.end (), a);
      expected = {&a};
      verify_items (list, expected);
    }
  }

  static void
  test_splice ()
  {
    {
      /* Two non-empty lists.  */
      item_type a ("a"), b ("b"), c ("c"), d ("d"), e ("e");
      ListType list1;
      ListType list2;
      std::vector<const item_type *> expected;

      list1.push_back (a);
      list1.push_back (b);
      list1.push_back (c);

      list2.push_back (d);
      list2.push_back (e);

      list1.splice (std::move (list2));

      expected = {&a, &b, &c, &d, &e};
      verify_items (list1, expected);

      expected = {};
      verify_items (list2, expected);
    }

    {
      /* Receiving list empty.  */
      item_type a ("a"), b ("b"), c ("c");
      ListType list1;
      ListType list2;
      std::vector<const item_type *> expected;

      list2.push_back (a);
      list2.push_back (b);
      list2.push_back (c);

      list1.splice (std::move (list2));

      expected = {&a, &b, &c};
      verify_items (list1, expected);

      expected = {};
      verify_items (list2, expected);
    }

    {
      /* Giving list empty.  */
      item_type a ("a"), b ("b"), c ("c");
      ListType list1;
      ListType list2;
      std::vector<const item_type *> expected;

      list1.push_back (a);
      list1.push_back (b);
      list1.push_back (c);

      list1.splice (std::move (list2));

      expected = {&a, &b, &c};
      verify_items (list1, expected);

      expected = {};
      verify_items (list2, expected);
    }

    {
      /* Both lists empty.  */
      item_type a ("a"), b ("b"), c ("c");
      ListType list1;
      ListType list2;
      std::vector<const item_type *> expected;

      list1.splice (std::move (list2));

      expected = {};
      verify_items (list1, expected);

      expected = {};
      verify_items (list2, expected);
    }
  }

  static void
  test_pop_front ()
  {
    item_type a ("a"), b ("b"), c ("c");
    ListType list;
    std::vector<const item_type *> expected;

    list.push_back (a);
    list.push_back (b);
    list.push_back (c);

    list.pop_front ();
    expected = {&b, &c};
    verify_items (list, expected);

    list.pop_front ();
    expected = {&c};
    verify_items (list, expected);

    list.pop_front ();
    expected = {};
    verify_items (list, expected);
  }

  static void
  test_pop_back ()
  {
    item_type a ("a"), b ("b"), c ("c");
    ListType list;
    std::vector<const item_type *> expected;

    list.push_back (a);
    list.push_back (b);
    list.push_back (c);

    list.pop_back();
    expected = {&a, &b};
    verify_items (list, expected);

    list.pop_back ();
    expected = {&a};
    verify_items (list, expected);

    list.pop_back ();
    expected = {};
    verify_items (list, expected);
  }

  static void
  test_erase ()
  {
    item_type a ("a"), b ("b"), c ("c");
    ListType list;
    std::vector<const item_type *> expected;

    list.push_back (a);
    list.push_back (b);
    list.push_back (c);

    list.erase (list.iterator_to (b));
    expected = {&a, &c};
    verify_items (list, expected);

    list.erase (list.iterator_to (c));
    expected = {&a};
    verify_items (list, expected);

    list.erase (list.iterator_to (a));
    expected = {};
    verify_items (list, expected);
  }

  static void
  test_clear ()
  {
    item_type a ("a"), b ("b"), c ("c");
    ListType list;
    std::vector<const item_type *> expected;

    list.push_back (a);
    list.push_back (b);
    list.push_back (c);

    list.clear ();
    expected = {};
    verify_items (list, expected);

    /* Verify idempotency.  */
    list.clear ();
    expected = {};
    verify_items (list, expected);
  }

  static void
  test_clear_and_dispose ()
  {
    item_type a ("a"), b ("b"), c ("c");
    ListType list;
    std::vector<const item_type *> expected;
    std::unordered_set<const item_type *> disposer_seen;
    int disposer_calls = 0;

    list.push_back (a);
    list.push_back (b);
    list.push_back (c);

    auto disposer = [&] (const item_type *item)
      {
	disposer_seen.insert (item);
	disposer_calls++;
      };
    list.clear_and_dispose (disposer);

    expected = {};
    verify_items (list, expected);
    SELF_CHECK (disposer_calls == 3);
    SELF_CHECK (disposer_seen.find (&a) != disposer_seen.end ());
    SELF_CHECK (disposer_seen.find (&b) != disposer_seen.end ());
    SELF_CHECK (disposer_seen.find (&c) != disposer_seen.end ());

    /* Verify idempotency.  */
    list.clear_and_dispose (disposer);
    SELF_CHECK (disposer_calls == 3);
  }

  static void
  test_empty ()
  {
    item_type a ("a");
    ListType list;

    SELF_CHECK (list.empty ());
    list.push_back (a);
    SELF_CHECK (!list.empty ());
    list.erase (list.iterator_to (a));
    SELF_CHECK (list.empty ());
  }

  static void
  test_begin_end ()
  {
    item_type a ("a"), b ("b"), c ("c");
    ListType list;
    const ListType &clist = list;

    list.push_back (a);
    list.push_back (b);
    list.push_back (c);

    SELF_CHECK (&*list.begin () == &a);
    SELF_CHECK (&*list.cbegin () == &a);
    SELF_CHECK (&*clist.begin () == &a);
    SELF_CHECK (&*list.rbegin () == &c);
    SELF_CHECK (&*list.crbegin () == &c);
    SELF_CHECK (&*clist.rbegin () == &c);

    /* At least check that they compile.  */
    list.end ();
    list.cend ();
    clist.end ();
    list.rend ();
    list.crend ();
    clist.end ();
  }
};

template <typename ListType>
static void
test_intrusive_list_1 ()
{
  intrusive_list_test<ListType> tests;

  tests.test_move_constructor ();
  tests.test_move_assignment ();
  tests.test_swap ();
  tests.test_front_back ();
  tests.test_push_front ();
  tests.test_push_back ();
  tests.test_insert ();
  tests.test_splice ();
  tests.test_pop_front ();
  tests.test_pop_back ();
  tests.test_erase ();
  tests.test_clear ();
  tests.test_clear_and_dispose ();
  tests.test_empty ();
  tests.test_begin_end ();
}

static void
test_node_is_linked ()
{
  {
    item_with_base a ("a");
    item_with_base_list list;

    SELF_CHECK (!a.is_linked ());
    list.push_back (a);
    SELF_CHECK (a.is_linked ());
    list.pop_back ();
    SELF_CHECK (!a.is_linked ());
  }

  {
    item_with_member a ("a");
    item_with_member_list list;

    SELF_CHECK (!a.node.is_linked ());
    list.push_back (a);
    SELF_CHECK (a.node.is_linked ());
    list.pop_back ();
    SELF_CHECK (!a.node.is_linked ());
  }
}

static void
test_intrusive_list ()
{
  test_intrusive_list_1<item_with_base_list> ();
  test_intrusive_list_1<item_with_member_list> ();
  test_node_is_linked ();
}

void _initialize_intrusive_list_selftests ();
void
_initialize_intrusive_list_selftests ()
{
  selftests::register_test
    ("intrusive_list", test_intrusive_list);
}
