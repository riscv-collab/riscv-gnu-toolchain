/* Poison symbols at compile time.

   Copyright (C) 2017-2024 Free Software Foundation, Inc.

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

#ifndef COMMON_POISON_H
#define COMMON_POISON_H

#include "traits.h"
#include "obstack.h"

/* Poison memset of non-POD types.  The idea is catching invalid
   initialization of non-POD structs that is easy to be introduced as
   side effect of refactoring.  For example, say this:

 struct S { VEC(foo_s) *m_data; };

is converted to this at some point:

 struct S {
   S() { m_data.reserve (10); }
   std::vector<foo> m_data;
 };

and old code was initializing S objects like this:

 struct S s;
 memset (&s, 0, sizeof (S)); // whoops, now wipes vector.

Declaring memset as deleted for non-POD types makes the memset above
be a compile-time error.  */

/* Helper for SFINAE.  True if "T *" is memsettable.  I.e., if T is
   either void, or POD.  */
template<typename T>
struct IsMemsettable
  : gdb::Or<std::is_void<T>,
	    gdb::And<std::is_standard_layout<T>, std::is_trivial<T>>>
{};

template <typename T,
	  typename = gdb::Requires<gdb::Not<IsMemsettable<T>>>>
void *memset (T *s, int c, size_t n) = delete;

#if HAVE_IS_TRIVIALLY_COPYABLE

/* Similarly, poison memcpy and memmove of non trivially-copyable
   types, which is undefined.  */

/* True if "T *" is relocatable.  I.e., copyable with memcpy/memmove.
   I.e., T is either trivially copyable, or void.  */
template<typename T>
struct IsRelocatable
  : gdb::Or<std::is_void<T>,
	    std::is_trivially_copyable<T>>
{};

/* True if both source and destination are relocatable.  */

template <typename D, typename S>
using BothAreRelocatable
  = gdb::And<IsRelocatable<D>, IsRelocatable<S>>;

template <typename D, typename S,
	  typename = gdb::Requires<gdb::Not<BothAreRelocatable<D, S>>>>
void *memcpy (D *dest, const S *src, size_t n) = delete;

template <typename D, typename S,
	  typename = gdb::Requires<gdb::Not<BothAreRelocatable<D, S>>>>
void *memmove (D *dest, const S *src, size_t n) = delete;

#endif /* HAVE_IS_TRIVIALLY_COPYABLE */

/* Poison XNEW and friends to catch usages of malloc-style allocations on
   objects that require new/delete.  */

template<typename T>
#if HAVE_IS_TRIVIALLY_CONSTRUCTIBLE
using IsMallocable = std::is_trivially_constructible<T>;
#else
using IsMallocable = std::true_type;
#endif

template<typename T>
using IsFreeable = gdb::Or<std::is_trivially_destructible<T>, std::is_void<T>>;

template <typename T, typename = gdb::Requires<gdb::Not<IsFreeable<T>>>>
void free (T *ptr) = delete;

template<typename T>
static T *
xnew ()
{
  static_assert (IsMallocable<T>::value, "Trying to use XNEW with a non-POD \
data type.  Use operator new instead.");
  return XNEW (T);
}

#undef XNEW
#define XNEW(T) xnew<T>()

template<typename T>
static T *
xcnew ()
{
  static_assert (IsMallocable<T>::value, "Trying to use XCNEW with a non-POD \
data type.  Use operator new instead.");
  return XCNEW (T);
}

#undef XCNEW
#define XCNEW(T) xcnew<T>()

template<typename T>
static void
xdelete (T *p)
{
  static_assert (IsFreeable<T>::value, "Trying to use XDELETE with a non-POD \
data type.  Use operator delete instead.");
  XDELETE (p);
}

#undef XDELETE
#define XDELETE(P) xdelete (P)

template<typename T>
static T *
xnewvec (size_t n)
{
  static_assert (IsMallocable<T>::value, "Trying to use XNEWVEC with a \
non-POD data type.  Use operator new[] (or std::vector) instead.");
  return XNEWVEC (T, n);
}

#undef XNEWVEC
#define XNEWVEC(T, N) xnewvec<T> (N)

template<typename T>
static T *
xcnewvec (size_t n)
{
  static_assert (IsMallocable<T>::value, "Trying to use XCNEWVEC with a \
non-POD data type.  Use operator new[] (or std::vector) instead.");
  return XCNEWVEC (T, n);
}

#undef XCNEWVEC
#define XCNEWVEC(T, N) xcnewvec<T> (N)

template<typename T>
static T *
xresizevec (T *p, size_t n)
{
  static_assert (IsMallocable<T>::value, "Trying to use XRESIZEVEC with a \
non-POD data type.");
  return XRESIZEVEC (T, p, n);
}

#undef XRESIZEVEC
#define XRESIZEVEC(T, P, N) xresizevec<T> (P, N)

template<typename T>
static void
xdeletevec (T *p)
{
  static_assert (IsFreeable<T>::value, "Trying to use XDELETEVEC with a \
non-POD data type.  Use operator delete[] (or std::vector) instead.");
  XDELETEVEC (p);
}

#undef XDELETEVEC
#define XDELETEVEC(P) xdeletevec (P)

template<typename T>
static T *
xnewvar (size_t s)
{
  static_assert (IsMallocable<T>::value, "Trying to use XNEWVAR with a \
non-POD data type.");
  return XNEWVAR (T, s);;
}

#undef XNEWVAR
#define XNEWVAR(T, S) xnewvar<T> (S)

template<typename T>
static T *
xcnewvar (size_t s)
{
  static_assert (IsMallocable<T>::value, "Trying to use XCNEWVAR with a \
non-POD data type.");
  return XCNEWVAR (T, s);
}

#undef XCNEWVAR
#define XCNEWVAR(T, S) xcnewvar<T> (S)

template<typename T>
static T *
xresizevar (T *p, size_t s)
{
  static_assert (IsMallocable<T>::value, "Trying to use XRESIZEVAR with a \
non-POD data type.");
  return XRESIZEVAR (T, p, s);
}

#undef XRESIZEVAR
#define XRESIZEVAR(T, P, S) xresizevar<T> (P, S)

template<typename T>
static T *
xobnew (obstack *ob)
{
  static_assert (IsMallocable<T>::value, "Trying to use XOBNEW with a \
non-POD data type.");
  return XOBNEW (ob, T);
}

#undef XOBNEW
#define XOBNEW(O, T) xobnew<T> (O)

template<typename T>
static T *
xobnewvec (obstack *ob, size_t n)
{
  static_assert (IsMallocable<T>::value, "Trying to use XOBNEWVEC with a \
non-POD data type.");
  return XOBNEWVEC (ob, T, n);
}

#undef XOBNEWVEC
#define XOBNEWVEC(O, T, N) xobnewvec<T> (O, N)

#endif /* COMMON_POISON_H */
