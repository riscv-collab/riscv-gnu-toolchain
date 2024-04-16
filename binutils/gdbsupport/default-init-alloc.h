/* Copyright (C) 2017-2024 Free Software Foundation, Inc.

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

#ifndef COMMON_DEFAULT_INIT_ALLOC_H
#define COMMON_DEFAULT_INIT_ALLOC_H

#if __cplusplus >= 202002L
#include <memory_resource>
#endif

namespace gdb {

/* An allocator that default constructs using default-initialization
   rather than value-initialization.  The idea is to use this when you
   don't want to default construct elements of containers of trivial
   types using zero-initialization.  */

/* Mostly as implementation convenience, this is implemented as an
   adapter that given an allocator A, overrides 'A::construct()'.  'A'
   defaults to std::allocator<T>.  */

template<typename T,
	 typename A
#if __cplusplus >= 202002L
	 = std::pmr::polymorphic_allocator<T>
#else
	 = std::allocator<T>
#endif
	 >
class default_init_allocator : public A
{
public:
  /* Pull in A's ctors.  */
  using A::A;

  /* Override rebind.  */
  template<typename U>
  struct rebind
  {
    /* A couple helpers just to make it a bit more readable.  */
    typedef std::allocator_traits<A> traits_;
    typedef typename traits_::template rebind_alloc<U> alloc_;

    /* This is what we're after.  */
    typedef default_init_allocator<U, alloc_> other;
  };

  /* Make the base allocator's construct method(s) visible.  */
  using A::construct;

  /* .. and provide an override/overload for the case of default
     construction (i.e., no arguments).  This is where we construct
     with default-init.  */
  template <typename U>
  void construct (U *ptr)
    noexcept (std::is_nothrow_default_constructible<U>::value)
  {
    ::new ((void *) ptr) U; /* default-init */
  }
};

} /* namespace gdb */

#endif /* COMMON_DEFAULT_INIT_ALLOC_H */
