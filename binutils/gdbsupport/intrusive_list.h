/* Intrusive double linked list for GDB, the GNU debugger.
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

#ifndef GDBSUPPORT_INTRUSIVE_LIST_H
#define GDBSUPPORT_INTRUSIVE_LIST_H

#define INTRUSIVE_LIST_UNLINKED_VALUE ((T *) -1)

/* A list node.  The elements put in an intrusive_list either inherit
   from this, or have a field of this type.  */
template<typename T>
class intrusive_list_node
{
public:
  bool is_linked () const
  {
    return next != INTRUSIVE_LIST_UNLINKED_VALUE;
  }

private:
  T *next = INTRUSIVE_LIST_UNLINKED_VALUE;
  T *prev = INTRUSIVE_LIST_UNLINKED_VALUE;

  template<typename T2, typename AsNode>
  friend struct intrusive_list_iterator;

  template<typename T2, typename AsNode>
  friend struct intrusive_list_reverse_iterator;

  template<typename T2, typename AsNode>
  friend struct intrusive_list;
};

/* Follows a couple types used by intrusive_list as template parameter to find
   the intrusive_list_node for a given element.  One for lists where the
   elements inherit intrusive_list_node, and another for elements that keep the
   node as member field.  */

/* For element types that inherit from intrusive_list_node.  */

template<typename T>
struct intrusive_base_node
{
  static intrusive_list_node<T> *as_node (T *elem)
  { return elem; }
};

/* For element types that keep the node as member field.  */

template<typename T, intrusive_list_node<T> T::*MemberNode>
struct intrusive_member_node
{
  static intrusive_list_node<T> *as_node (T *elem)
  { return &(elem->*MemberNode); }
};

/* Common code for forward and reverse iterators.  */

template<typename T, typename AsNode, typename SelfType>
struct intrusive_list_base_iterator
{
  using self_type = SelfType;
  using iterator_category = std::bidirectional_iterator_tag;
  using value_type = T;
  using pointer = T *;
  using const_pointer = const T *;
  using reference = T &;
  using const_reference = const T &;
  using difference_type = ptrdiff_t;
  using size_type = size_t;
  using node_type = intrusive_list_node<T>;

  /* Create an iterator pointing to ELEM.  */
  explicit intrusive_list_base_iterator (pointer elem)
    : m_elem (elem)
  {}

  /* Create a past-the-end iterator.  */
  intrusive_list_base_iterator ()
    : m_elem (nullptr)
  {}

  reference operator* () const
  { return *m_elem; }

  pointer operator-> () const
  { return m_elem; }

  bool operator== (const self_type &other) const
  { return m_elem == other.m_elem; }

  bool operator!= (const self_type &other) const
  { return m_elem != other.m_elem; }

protected:
  static node_type *as_node (pointer elem)
  { return AsNode::as_node (elem); }

  /* A past-end-the iterator points to the list's head.  */
  pointer m_elem;
};

/* Forward iterator for an intrusive_list.  */

template<typename T, typename AsNode = intrusive_base_node<T>>
struct intrusive_list_iterator
  : public intrusive_list_base_iterator
	     <T, AsNode, intrusive_list_iterator<T, AsNode>>
{
  using base = intrusive_list_base_iterator
		 <T, AsNode, intrusive_list_iterator<T, AsNode>>;
  using self_type = typename base::self_type;
  using node_type = typename base::node_type;

  /* Inherit constructor and M_NODE visibility from base.  */
  using base::base;
  using base::m_elem;

  self_type &operator++ ()
  {
    node_type *node = this->as_node (m_elem);
    m_elem = node->next;
    return *this;
  }

  self_type operator++ (int)
  {
    self_type temp = *this;
    node_type *node = this->as_node (m_elem);
    m_elem = node->next;
    return temp;
  }

  self_type &operator-- ()
  {
    node_type *node = this->as_node (m_elem);
    m_elem = node->prev;
    return *this;
  }

  self_type operator-- (int)
  {
    self_type temp = *this;
    node_type *node = this->as_node (m_elem);
    m_elem = node->prev;
    return temp;
  }
};

/* Reverse iterator for an intrusive_list.  */

template<typename T, typename AsNode = intrusive_base_node<T>>
struct intrusive_list_reverse_iterator
  : public intrusive_list_base_iterator
	     <T, AsNode, intrusive_list_reverse_iterator<T, AsNode>>
{
  using base = intrusive_list_base_iterator
		 <T, AsNode, intrusive_list_reverse_iterator<T, AsNode>>;
  using self_type = typename base::self_type;

  /* Inherit constructor and M_NODE visibility from base.  */
  using base::base;
  using base::m_elem;
  using node_type = typename base::node_type;

  self_type &operator++ ()
  {
    node_type *node = this->as_node (m_elem);
    m_elem = node->prev;
    return *this;
  }

  self_type operator++ (int)
  {
    self_type temp = *this;
    node_type *node = this->as_node (m_elem);
    m_elem = node->prev;
    return temp;
  }

  self_type &operator-- ()
  {
    node_type *node = this->as_node (m_elem);
    m_elem = node->next;
    return *this;
  }

  self_type operator-- (int)
  {
    self_type temp = *this;
    node_type *node = this->as_node (m_elem);
    m_elem = node->next;
    return temp;
  }
};

/* An intrusive double-linked list.

   T is the type of the elements to link.  The type T must either:

    - inherit from intrusive_list_node<T>
    - have an intrusive_list_node<T> member

   AsNode is a type with an as_node static method used to get a node from an
   element.  If elements inherit from intrusive_list_node<T>, use the default
   intrusive_base_node<T>.  If elements have an intrusive_list_node<T> member,
   use:

     intrusive_member_node<T, &T::member>

   where `member` is the name of the member.  */

template <typename T, typename AsNode = intrusive_base_node<T>>
class intrusive_list
{
public:
  using value_type = T;
  using pointer = T *;
  using const_pointer = const T *;
  using reference = T &;
  using const_reference = const T &;
  using difference_type = ptrdiff_t;
  using size_type = size_t;
  using iterator = intrusive_list_iterator<T, AsNode>;
  using reverse_iterator = intrusive_list_reverse_iterator<T, AsNode>;
  using const_iterator = const intrusive_list_iterator<T, AsNode>;
  using const_reverse_iterator
    = const intrusive_list_reverse_iterator<T, AsNode>;
  using node_type = intrusive_list_node<T>;

  intrusive_list () = default;

  ~intrusive_list ()
  {
    clear ();
  }

  intrusive_list (intrusive_list &&other)
    : m_front (other.m_front),
      m_back (other.m_back)
  {
    other.m_front = nullptr;
    other.m_back = nullptr;
  }

  intrusive_list &operator= (intrusive_list &&other)
  {
    m_front = other.m_front;
    m_back = other.m_back;
    other.m_front = nullptr;
    other.m_back = nullptr;

    return *this;
  }

  void swap (intrusive_list &other)
  {
    std::swap (m_front, other.m_front);
    std::swap (m_back, other.m_back);
  }

  iterator iterator_to (reference value)
  {
    return iterator (&value);
  }

  const_iterator iterator_to (const_reference value)
  {
    return const_iterator (&value);
  }

  reference front ()
  {
    gdb_assert (!this->empty ());
    return *m_front;
  }

  const_reference front () const
  {
    gdb_assert (!this->empty ());
    return *m_front;
  }

  reference back ()
  {
    gdb_assert (!this->empty ());
    return *m_back;
  }

  const_reference back () const
  {
    gdb_assert (!this->empty ());
    return *m_back;
  }

  void push_front (reference elem)
  {
    intrusive_list_node<T> *elem_node = as_node (&elem);

    gdb_assert (elem_node->next == INTRUSIVE_LIST_UNLINKED_VALUE);
    gdb_assert (elem_node->prev == INTRUSIVE_LIST_UNLINKED_VALUE);

    if (this->empty ())
      this->push_empty (elem);
    else
      this->push_front_non_empty (elem);
  }

  void push_back (reference elem)
  {
    intrusive_list_node<T> *elem_node = as_node (&elem);

    gdb_assert (elem_node->next == INTRUSIVE_LIST_UNLINKED_VALUE);
    gdb_assert (elem_node->prev == INTRUSIVE_LIST_UNLINKED_VALUE);

    if (this->empty ())
      this->push_empty (elem);
    else
      this->push_back_non_empty (elem);
  }

  /* Inserts ELEM before POS.  */
  void insert (const_iterator pos, reference elem)
  {
    if (this->empty ())
      return this->push_empty (elem);

    if (pos == this->begin ())
      return this->push_front_non_empty (elem);

    if (pos == this->end ())
      return this->push_back_non_empty (elem);

    intrusive_list_node<T> *elem_node = as_node (&elem);
    pointer pos_elem = &*pos;
    intrusive_list_node<T> *pos_node = as_node (pos_elem);
    pointer prev_elem = pos_node->prev;
    intrusive_list_node<T> *prev_node = as_node (prev_elem);

    gdb_assert (elem_node->next == INTRUSIVE_LIST_UNLINKED_VALUE);
    gdb_assert (elem_node->prev == INTRUSIVE_LIST_UNLINKED_VALUE);

    elem_node->prev = prev_elem;
    prev_node->next = &elem;
    elem_node->next = pos_elem;
    pos_node->prev = &elem;
  }

  /* Move elements from LIST at the end of the current list.  */
  void splice (intrusive_list &&other)
  {
    if (other.empty ())
      return;

    if (this->empty ())
      {
	*this = std::move (other);
	return;
      }

    /* [A ... B] + [C ... D] */
    pointer b_elem = m_back;
    node_type *b_node = as_node (b_elem);
    pointer c_elem = other.m_front;
    node_type *c_node = as_node (c_elem);
    pointer d_elem = other.m_back;

    b_node->next = c_elem;
    c_node->prev = b_elem;
    m_back = d_elem;

    other.m_front = nullptr;
    other.m_back = nullptr;
  }

  void pop_front ()
  {
    gdb_assert (!this->empty ());
    erase_element (*m_front);
  }

  void pop_back ()
  {
    gdb_assert (!this->empty ());
    erase_element (*m_back);
  }

private:
  /* Push ELEM in the list, knowing the list is empty.  */
  void push_empty (reference elem)
  {
    gdb_assert (this->empty ());

    intrusive_list_node<T> *elem_node = as_node (&elem);

    gdb_assert (elem_node->next == INTRUSIVE_LIST_UNLINKED_VALUE);
    gdb_assert (elem_node->prev == INTRUSIVE_LIST_UNLINKED_VALUE);

    m_front = &elem;
    m_back = &elem;
    elem_node->prev = nullptr;
    elem_node->next = nullptr;
  }

  /* Push ELEM at the front of the list, knowing the list is not empty.  */
  void push_front_non_empty (reference elem)
  {
    gdb_assert (!this->empty ());

    intrusive_list_node<T> *elem_node = as_node (&elem);
    intrusive_list_node<T> *front_node = as_node (m_front);

    gdb_assert (elem_node->next == INTRUSIVE_LIST_UNLINKED_VALUE);
    gdb_assert (elem_node->prev == INTRUSIVE_LIST_UNLINKED_VALUE);

    elem_node->next = m_front;
    front_node->prev = &elem;
    elem_node->prev = nullptr;
    m_front = &elem;
  }

  /* Push ELEM at the back of the list, knowing the list is not empty.  */
  void push_back_non_empty (reference elem)
  {
    gdb_assert (!this->empty ());

    intrusive_list_node<T> *elem_node = as_node (&elem);
    intrusive_list_node<T> *back_node = as_node (m_back);

    gdb_assert (elem_node->next == INTRUSIVE_LIST_UNLINKED_VALUE);
    gdb_assert (elem_node->prev == INTRUSIVE_LIST_UNLINKED_VALUE);

    elem_node->prev = m_back;
    back_node->next = &elem;
    elem_node->next = nullptr;
    m_back = &elem;
  }

  void erase_element (reference elem)
  {
    intrusive_list_node<T> *elem_node = as_node (&elem);

    gdb_assert (elem_node->prev != INTRUSIVE_LIST_UNLINKED_VALUE);
    gdb_assert (elem_node->next != INTRUSIVE_LIST_UNLINKED_VALUE);

    if (m_front == &elem)
      {
	gdb_assert (elem_node->prev == nullptr);
	m_front = elem_node->next;
      }
    else
      {
	gdb_assert (elem_node->prev != nullptr);
	intrusive_list_node<T> *prev_node = as_node (elem_node->prev);
	prev_node->next = elem_node->next;
      }

    if (m_back == &elem)
      {
	gdb_assert (elem_node->next == nullptr);
	m_back = elem_node->prev;
      }
    else
      {
	gdb_assert (elem_node->next != nullptr);
	intrusive_list_node<T> *next_node = as_node (elem_node->next);
	next_node->prev = elem_node->prev;
      }

    elem_node->next = INTRUSIVE_LIST_UNLINKED_VALUE;
    elem_node->prev = INTRUSIVE_LIST_UNLINKED_VALUE;
  }

public:
  /* Remove the element pointed by I from the list.  The element
     pointed by I is not destroyed.  */
  iterator erase (const_iterator i)
  {
    iterator ret = i;
    ++ret;

    erase_element (*i);

    return ret;
  }

  /* Erase all the elements.  The elements are not destroyed.  */
  void clear ()
  {
    while (!this->empty ())
      pop_front ();
  }

  /* Erase all the elements.  Disposer::operator()(pointer) is called
     for each of the removed elements.  */
  template<typename Disposer>
  void clear_and_dispose (Disposer disposer)
  {
    while (!this->empty ())
      {
	pointer p = &front ();
	pop_front ();
	disposer (p);
      }
  }

  bool empty () const
  {
    return m_front == nullptr;
  }

  iterator begin () noexcept
  {
    return iterator (m_front);
  }

  const_iterator begin () const noexcept
  {
    return const_iterator (m_front);
  }

  const_iterator cbegin () const noexcept
  {
    return const_iterator (m_front);
  }

  iterator end () noexcept
  {
    return {};
  }

  const_iterator end () const noexcept
  {
    return {};
  }

  const_iterator cend () const noexcept
  {
    return {};
  }

  reverse_iterator rbegin () noexcept
  {
    return reverse_iterator (m_back);
  }

  const_reverse_iterator rbegin () const noexcept
  {
    return const_reverse_iterator (m_back);
  }

  const_reverse_iterator crbegin () const noexcept
  {
    return const_reverse_iterator (m_back);
  }

  reverse_iterator rend () noexcept
  {
    return {};
  }

  const_reverse_iterator rend () const noexcept
  {
    return {};
  }

  const_reverse_iterator crend () const noexcept
  {
    return {};
  }

private:
  static node_type *as_node (pointer elem)
  {
    return AsNode::as_node (elem);
  }

  pointer m_front = nullptr;
  pointer m_back = nullptr;
};

#endif /* GDBSUPPORT_INTRUSIVE_LIST_H */
