/* Offset types for GDB.

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

/* Define an "offset" type.  Offset types are distinct integer types
   that are used to represent an offset into anything that is
   addressable.  For example, an offset into a DWARF debug section.
   The idea is catch mixing unrelated offset types at compile time, in
   code that needs to manipulate multiple different kinds of offsets
   that are easily confused.  They're safer to use than native
   integers, because they have no implicit conversion to anything.
   And also, since they're implemented as "enum class" strong
   typedefs, they're still integers ABI-wise, making them a bit more
   efficient than wrapper structs on some ABIs.

   Some properties of offset types, loosely modeled on pointers:

   - You can compare offsets of the same type for equality and order.
     You can't compare an offset with an unrelated type.

   - You can add/substract an integer to/from an offset, which gives
     you back a shifted offset.

   - You can subtract two offsets of the same type, which gives you
     back the delta as an integer (of the enum class's underlying
     type), not as an offset type.

   - You can't add two offsets of the same type, as that would not
     make sense.

   However, unlike pointers, you can't deference offset types.  */

#ifndef COMMON_OFFSET_TYPE_H
#define COMMON_OFFSET_TYPE_H

/* Declare TYPE as being an offset type.  This declares the type and
   enables the operators defined below.  */
#define DEFINE_OFFSET_TYPE(TYPE, UNDERLYING)	\
  enum class TYPE : UNDERLYING {};		\
  void is_offset_type (TYPE)

/* The macro macro is all you need to know use offset types.  The rest
   below is all implementation detail.  */

/* For each enum class type that you want to support arithmetic
   operators, declare an "is_offset_type" overload that has exactly
   one parameter, of type that enum class.  E.g.,:

     void is_offset_type (sect_offset);

   The function does not need to be defined, only declared.
   DEFINE_OFFSET_TYPE declares this.

   A function declaration is preferred over a traits type, because the
   former allows calling the DEFINE_OFFSET_TYPE macro inside a
   namespace to define the corresponding offset type in that
   namespace.  The compiler finds the corresponding is_offset_type
   function via ADL.
*/

/* Adding or subtracting an integer to an offset type shifts the
   offset.  This is like "PTR = PTR + INT" and "PTR += INT".  */

#define DEFINE_OFFSET_ARITHM_OP(OP)					\
  template<typename E,							\
	   typename = decltype (is_offset_type (std::declval<E> ()))>	\
  constexpr E								\
  operator OP (E lhs, typename std::underlying_type<E>::type rhs)	\
  {									\
    using underlying = typename std::underlying_type<E>::type;		\
    return (E) (static_cast<underlying> (lhs) OP rhs);			\
  }									\
									\
  template<typename E,							\
	   typename = decltype (is_offset_type (std::declval<E> ()))>	\
  constexpr E								\
  operator OP (typename std::underlying_type<E>::type lhs, E rhs)	\
  {									\
    using underlying = typename std::underlying_type<E>::type;		\
    return (E) (lhs OP static_cast<underlying> (rhs));			\
  }									\
									\
  template<typename E,							\
	   typename = decltype (is_offset_type (std::declval<E> ()))>	\
  E &									\
  operator OP ## = (E &lhs, typename std::underlying_type<E>::type rhs)	\
  {									\
    using underlying = typename std::underlying_type<E>::type;		\
    lhs = (E) (static_cast<underlying> (lhs) OP rhs);			\
    return lhs;								\
  }

DEFINE_OFFSET_ARITHM_OP(+)
DEFINE_OFFSET_ARITHM_OP(-)

/* Adding two offset types doesn't make sense, just like "PTR + PTR"
   doesn't make sense.  This is defined as a deleted function so that
   a compile error easily brings you to this comment.  */

template<typename E,
	 typename = decltype (is_offset_type (std::declval<E> ()))>
constexpr typename std::underlying_type<E>::type
operator+ (E lhs, E rhs) = delete;

/* Subtracting two offset types, however, gives you back the
   difference between the offsets, as an underlying type.  Similar to
   how "PTR2 - PTR1" returns a ptrdiff_t.  */

template<typename E,
	 typename = decltype (is_offset_type (std::declval<E> ()))>
constexpr typename std::underlying_type<E>::type
operator- (E lhs, E rhs)
{
  using underlying = typename std::underlying_type<E>::type;
  return static_cast<underlying> (lhs) - static_cast<underlying> (rhs);
}

#endif /* COMMON_OFFSET_TYPE_H */
