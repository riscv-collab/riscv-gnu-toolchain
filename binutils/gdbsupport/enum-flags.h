/* Copyright (C) 2015-2024 Free Software Foundation, Inc.

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

#ifndef COMMON_ENUM_FLAGS_H
#define COMMON_ENUM_FLAGS_H

#include "traits.h"

/* Type-safe wrapper for enum flags.  enum flags are enums where the
   values are bits that are meant to be ORed together.

   This allows writing code like the below, while with raw enums this
   would fail to compile without casts to enum type at the assignments
   to 'f':

    enum some_flag
    {
       flag_val1 = 1 << 1,
       flag_val2 = 1 << 2,
       flag_val3 = 1 << 3,
       flag_val4 = 1 << 4,
    };
    DEF_ENUM_FLAGS_TYPE(enum some_flag, some_flags);

    some_flags f = flag_val1 | flag_val2;
    f |= flag_val3;

   It's also possible to assign literal zero to an enum flags variable
   (meaning, no flags), dispensing adding an awkward explicit "no
   value" value to the enumeration.  For example:

    some_flags f = 0;
    f |= flag_val3 | flag_val4;

   Note that literal integers other than zero fail to compile:

    some_flags f = 1; // error
*/

#ifdef __cplusplus

/* Use this to mark an enum as flags enum.  It defines FLAGS_TYPE as
   enum_flags wrapper class for ENUM, and enables the global operator
   overloads for ENUM.  */
#define DEF_ENUM_FLAGS_TYPE(enum_type, flags_type)	\
  typedef enum_flags<enum_type> flags_type;		\
  void is_enum_flags_enum_type (enum_type *)

/* To enable the global enum_flags operators for enum, declare an
   "is_enum_flags_enum_type" overload that has exactly one parameter,
   of type a pointer to that enum class.  E.g.,:

     void is_enum_flags_enum_type (enum some_flag *);

   The function does not need to be defined, only declared.
   DEF_ENUM_FLAGS_TYPE declares this.

   A function declaration is preferred over a traits type, because the
   former allows calling the DEF_ENUM_FLAGS_TYPE macro inside a
   namespace to define the corresponding enum flags type in that
   namespace.  The compiler finds the corresponding
   is_enum_flags_enum_type function via ADL.  */

/* Note that std::underlying_type<enum_type> is not what we want here,
   since that returns unsigned int even when the enum decays to signed
   int.  */
template<int size, bool sign> class integer_for_size { typedef void type; };
template<> struct integer_for_size<1, 0> { typedef uint8_t type; };
template<> struct integer_for_size<2, 0> { typedef uint16_t type; };
template<> struct integer_for_size<4, 0> { typedef uint32_t type; };
template<> struct integer_for_size<8, 0> { typedef uint64_t type; };
template<> struct integer_for_size<1, 1> { typedef int8_t type; };
template<> struct integer_for_size<2, 1> { typedef int16_t type; };
template<> struct integer_for_size<4, 1> { typedef int32_t type; };
template<> struct integer_for_size<8, 1> { typedef int64_t type; };

template<typename T>
struct enum_underlying_type
{
  DIAGNOSTIC_PUSH
  DIAGNOSTIC_IGNORE_ENUM_CONSTEXPR_CONVERSION
  typedef typename
    integer_for_size<sizeof (T), static_cast<bool>(T (-1) < T (0))>::type
    type;
  DIAGNOSTIC_POP
};

namespace enum_flags_detail
{

/* Private type used to support initializing flag types with zero:

   foo_flags f = 0;

   but not other integers:

   foo_flags f = 1;

   The way this works is that we define an implicit constructor that
   takes a pointer to this private type.  Since nothing can
   instantiate an object of this type, the only possible pointer to
   pass to the constructor is the NULL pointer, or, zero.  */
struct zero_type;

/* gdb::Requires trait helpers.  */
template <typename enum_type>
using EnumIsUnsigned
  = std::is_unsigned<typename enum_underlying_type<enum_type>::type>;
template <typename enum_type>
using EnumIsSigned
  = std::is_signed<typename enum_underlying_type<enum_type>::type>;

}

template <typename E>
class enum_flags
{
public:
  typedef E enum_type;
  typedef typename enum_underlying_type<enum_type>::type underlying_type;

  /* For to_string.  Maps one enumerator of E to a string.  */
  struct string_mapping
  {
    E flag;
    const char *str;
  };

  /* Convenience for to_string implementations, to build a
     string_mapping array.  */
#define MAP_ENUM_FLAG(ENUM_FLAG) { ENUM_FLAG, #ENUM_FLAG }

public:
  /* Allow default construction.  */
  constexpr enum_flags ()
    : m_enum_value ((enum_type) 0)
  {}

  /* The default move/copy ctor/assignment do the right thing.  */

  /* If you get an error saying these two overloads are ambiguous,
     then you tried to mix values of different enum types.  */
  constexpr enum_flags (enum_type e)
    : m_enum_value (e)
  {}
  constexpr enum_flags (enum_flags_detail::zero_type *zero)
    : m_enum_value ((enum_type) 0)
  {}

  enum_flags &operator&= (enum_flags e) &
  {
    m_enum_value = (enum_type) (m_enum_value & e.m_enum_value);
    return *this;
  }
  enum_flags &operator|= (enum_flags e) &
  {
    m_enum_value = (enum_type) (m_enum_value | e.m_enum_value);
    return *this;
  }
  enum_flags &operator^= (enum_flags e) &
  {
    m_enum_value = (enum_type) (m_enum_value ^ e.m_enum_value);
    return *this;
  }

  /* Delete rval versions.  */
  void operator&= (enum_flags e) && = delete;
  void operator|= (enum_flags e) && = delete;
  void operator^= (enum_flags e) && = delete;

  /* Like raw enums, allow conversion to the underlying type.  */
  constexpr operator underlying_type () const
  {
    return m_enum_value;
  }

  /* Get the underlying value as a raw enum.  */
  constexpr enum_type raw () const
  {
    return m_enum_value;
  }

  /* Binary operations involving some unrelated type (which would be a
     bug) are implemented as non-members, and deleted.  */

  /* Convert this object to a std::string, using MAPPING as
     enumerator-to-string mapping array.  This is not meant to be
     called directly.  Instead, enum_flags specializations should have
     their own to_string function wrapping this one, thus hiding the
     mapping array from callers.

     Note: this is defined outside the template class so it can use
     the global operators for enum_type, which are only defined after
     the template class.  */
  template<size_t N>
  std::string to_string (const string_mapping (&mapping)[N]) const;

private:
  /* Stored as enum_type because GDB knows to print the bit flags
     neatly if the enum values look like bit flags.  */
  enum_type m_enum_value;
};

template <typename E>
using is_enum_flags_enum_type_t
  = decltype (is_enum_flags_enum_type (std::declval<E *> ()));

/* Global operator overloads.  */

/* Generate binary operators.  */

#define ENUM_FLAGS_GEN_BINOP(OPERATOR_OP, OP)				\
									\
  /* Raw enum on both LHS/RHS.  Returns raw enum type.  */		\
  template <typename enum_type,						\
	    typename = is_enum_flags_enum_type_t<enum_type>>		\
  constexpr enum_type							\
  OPERATOR_OP (enum_type e1, enum_type e2)				\
  {									\
    using underlying = typename enum_flags<enum_type>::underlying_type;	\
    return (enum_type) (underlying (e1) OP underlying (e2));		\
  }									\
									\
  /* enum_flags on the LHS.  */						\
  template <typename enum_type,						\
	    typename = is_enum_flags_enum_type_t<enum_type>>		\
  constexpr enum_flags<enum_type>					\
  OPERATOR_OP (enum_flags<enum_type> e1, enum_type e2)			\
  { return e1.raw () OP e2; }						\
									\
  /* enum_flags on the RHS.  */						\
  template <typename enum_type,						\
	    typename = is_enum_flags_enum_type_t<enum_type>>		\
  constexpr enum_flags<enum_type>					\
  OPERATOR_OP (enum_type e1, enum_flags<enum_type> e2)			\
  { return e1 OP e2.raw (); }						\
									\
  /* enum_flags on both LHS/RHS.  */					\
  template <typename enum_type,						\
	    typename = is_enum_flags_enum_type_t<enum_type>>		\
  constexpr enum_flags<enum_type>					\
  OPERATOR_OP (enum_flags<enum_type> e1, enum_flags<enum_type> e2)	\
  { return e1.raw () OP e2.raw (); }					\
									\
  /* Delete cases involving unrelated types.  */			\
									\
  template <typename enum_type, typename unrelated_type,		\
	    typename = is_enum_flags_enum_type_t<enum_type>>		\
  constexpr enum_flags<enum_type>					\
  OPERATOR_OP (enum_type e1, unrelated_type e2) = delete;		\
									\
  template <typename enum_type, typename unrelated_type,		\
	    typename = is_enum_flags_enum_type_t<enum_type>>		\
  constexpr enum_flags<enum_type>					\
  OPERATOR_OP (unrelated_type e1, enum_type e2) = delete;		\
									\
  template <typename enum_type, typename unrelated_type,		\
	    typename = is_enum_flags_enum_type_t<enum_type>>		\
  constexpr enum_flags<enum_type>					\
  OPERATOR_OP (enum_flags<enum_type> e1, unrelated_type e2) = delete;	\
									\
  template <typename enum_type, typename unrelated_type,		\
	    typename = is_enum_flags_enum_type_t<enum_type>>		\
  constexpr enum_flags<enum_type>					\
  OPERATOR_OP (unrelated_type e1, enum_flags<enum_type> e2) = delete;

/* Generate non-member compound assignment operators.  Only the raw
   enum versions are defined here.  The enum_flags versions are
   defined as member functions, simply because it's less code that
   way.

   Note we delete operators that would allow e.g.,

     "enum_type | 1" or "enum_type1 | enum_type2"

   because that would allow a mistake like :
     enum flags1 { F1_FLAGS1 = 1 };
     enum flags2 { F2_FLAGS2 = 2 };
     enum flags1 val;
     switch (val) {
       case F1_FLAGS1 | F2_FLAGS2:
     ...

   If you really need to 'or' enumerators of different flag types,
   cast to integer first.
*/
#define ENUM_FLAGS_GEN_COMPOUND_ASSIGN(OPERATOR_OP, OP)			\
  /* lval reference version.  */					\
  template <typename enum_type,						\
	    typename = is_enum_flags_enum_type_t<enum_type>>		\
  constexpr enum_type &							\
  OPERATOR_OP (enum_type &e1, enum_type e2)				\
  { return e1 = e1 OP e2; }						\
									\
  /* rval reference version.  */					\
  template <typename enum_type,						\
	    typename = is_enum_flags_enum_type_t<enum_type>>		\
  void									\
  OPERATOR_OP (enum_type &&e1, enum_type e2) = delete;			\
									\
  /* Delete compound assignment from unrelated types.  */		\
									\
  template <typename enum_type, typename other_enum_type,		\
	    typename = is_enum_flags_enum_type_t<enum_type>>		\
  constexpr enum_type &							\
  OPERATOR_OP (enum_type &e1, other_enum_type e2) = delete;		\
									\
  template <typename enum_type, typename other_enum_type,		\
	    typename = is_enum_flags_enum_type_t<enum_type>>		\
  void									\
  OPERATOR_OP (enum_type &&e1, other_enum_type e2) = delete;

ENUM_FLAGS_GEN_BINOP (operator|, |)
ENUM_FLAGS_GEN_BINOP (operator&, &)
ENUM_FLAGS_GEN_BINOP (operator^, ^)

ENUM_FLAGS_GEN_COMPOUND_ASSIGN (operator|=, |)
ENUM_FLAGS_GEN_COMPOUND_ASSIGN (operator&=, &)
ENUM_FLAGS_GEN_COMPOUND_ASSIGN (operator^=, ^)

/* Allow comparison with enum_flags, raw enum, and integers, only.
   The latter case allows "== 0".  As side effect, it allows comparing
   with integer variables too, but that's not a common mistake to
   make.  It's important to disable comparison with unrelated types to
   prevent accidentally comparing with unrelated enum values, which
   are convertible to integer, and thus coupled with enum_flags
   conversion to underlying type too, would trigger the built-in 'bool
   operator==(unsigned, int)' operator.  */

#define ENUM_FLAGS_GEN_COMP(OPERATOR_OP, OP)				\
									\
  /* enum_flags OP enum_flags */					\
									\
  template <typename enum_type>						\
  constexpr bool							\
  OPERATOR_OP (enum_flags<enum_type> lhs, enum_flags<enum_type> rhs)	\
  { return lhs.raw () OP rhs.raw (); }					\
									\
  /* enum_flags OP other */						\
									\
  template <typename enum_type>						\
  constexpr bool							\
  OPERATOR_OP (enum_flags<enum_type> lhs, enum_type rhs)		\
  { return lhs.raw () OP rhs; }						\
									\
  template <typename enum_type>						\
  constexpr bool							\
  OPERATOR_OP (enum_flags<enum_type> lhs, int rhs)			\
  { return lhs.raw () OP rhs; }						\
									\
  template <typename enum_type, typename U>				\
  constexpr bool							\
  OPERATOR_OP (enum_flags<enum_type> lhs, U rhs) = delete;		\
									\
  /* other OP enum_flags */						\
									\
  template <typename enum_type>						\
  constexpr bool							\
  OPERATOR_OP (enum_type lhs, enum_flags<enum_type> rhs)		\
  { return lhs OP rhs.raw (); }						\
									\
  template <typename enum_type>						\
  constexpr bool							\
  OPERATOR_OP (int lhs, enum_flags<enum_type> rhs)			\
  { return lhs OP rhs.raw (); }						\
									\
  template <typename enum_type, typename U>				\
  constexpr bool							\
  OPERATOR_OP (U lhs, enum_flags<enum_type> rhs) = delete;

ENUM_FLAGS_GEN_COMP (operator==, ==)
ENUM_FLAGS_GEN_COMP (operator!=, !=)

/* Unary operators for the raw flags enum.  */

/* We require underlying type to be unsigned when using operator~ --
   if it were not unsigned, undefined behavior could result.  However,
   asserting this in the class itself would require too many
   unnecessary changes to usages of otherwise OK enum types.  */
template <typename enum_type,
	  typename = is_enum_flags_enum_type_t<enum_type>,
	  typename
	    = gdb::Requires<enum_flags_detail::EnumIsUnsigned<enum_type>>>
constexpr enum_type
operator~ (enum_type e)
{
  using underlying = typename enum_flags<enum_type>::underlying_type;
  return (enum_type) ~underlying (e);
}

template <typename enum_type,
	  typename = is_enum_flags_enum_type_t<enum_type>,
	  typename = gdb::Requires<enum_flags_detail::EnumIsSigned<enum_type>>>
constexpr void operator~ (enum_type e) = delete;

template <typename enum_type,
	  typename = is_enum_flags_enum_type_t<enum_type>,
	  typename
	    = gdb::Requires<enum_flags_detail::EnumIsUnsigned<enum_type>>>
constexpr enum_flags<enum_type>
operator~ (enum_flags<enum_type> e)
{
  using underlying = typename enum_flags<enum_type>::underlying_type;
  return (enum_type) ~underlying (e);
}

template <typename enum_type,
	  typename = is_enum_flags_enum_type_t<enum_type>,
	  typename = gdb::Requires<enum_flags_detail::EnumIsSigned<enum_type>>>
constexpr void operator~ (enum_flags<enum_type> e) = delete;

/* Delete operator<< and operator>>.  */

template <typename enum_type, typename any_type,
	  typename = is_enum_flags_enum_type_t<enum_type>>
void operator<< (const enum_type &, const any_type &) = delete;

template <typename enum_type, typename any_type,
	  typename = is_enum_flags_enum_type_t<enum_type>>
void operator<< (const enum_flags<enum_type> &, const any_type &) = delete;

template <typename enum_type, typename any_type,
	  typename = is_enum_flags_enum_type_t<enum_type>>
void operator>> (const enum_type &, const any_type &) = delete;

template <typename enum_type, typename any_type,
	  typename = is_enum_flags_enum_type_t<enum_type>>
void operator>> (const enum_flags<enum_type> &, const any_type &) = delete;

template<typename E>
template<size_t N>
std::string
enum_flags<E>::to_string (const string_mapping (&mapping)[N]) const
{
  enum_type flags = raw ();
  std::string res = hex_string (flags);
  res += " [";

  bool need_space = false;
  for (const auto &entry : mapping)
    {
      if ((flags & entry.flag) != 0)
	{
	  /* Work with an unsigned version of the underlying type,
	     because if enum_type's underlying type is signed, op~
	     won't be defined for it, and, bitwise operations on
	     signed types are implementation defined.  */
	  using uns = typename std::make_unsigned<underlying_type>::type;
	  flags &= (enum_type) ~(uns) entry.flag;

	  if (need_space)
	    res += " ";
	  res += entry.str;

	  need_space = true;
	}
    }

  /* If there were flags not included in the mapping, print them as
     a hex number.  */
  if (flags != 0)
    {
      if (need_space)
	res += " ";
      res += hex_string (flags);
    }

  res += "]";

  return res;
}

#else /* __cplusplus */

/* In C, the flags type is just a typedef for the enum type.  */

#define DEF_ENUM_FLAGS_TYPE(enum_type, flags_type) \
  typedef enum_type flags_type

#endif /* __cplusplus */

#endif /* COMMON_ENUM_FLAGS_H */
