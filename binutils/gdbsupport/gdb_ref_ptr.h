/* Reference-counted smart pointer class

   Copyright (C) 2016-2024 Free Software Foundation, Inc.

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

#ifndef COMMON_GDB_REF_PTR_H
#define COMMON_GDB_REF_PTR_H

#include <cstddef>

namespace gdb
{

/* An instance of this class either holds a reference to a
   reference-counted object or is "NULL".  Reference counting is
   handled externally by a policy class.  If the object holds a
   reference, then when the object is destroyed, the reference is
   decref'd.

   Normally an instance is constructed using a pointer.  This sort of
   initialization lets this class manage the lifetime of that
   reference.

   Assignment and copy construction will make a new reference as
   appropriate.  Assignment from a plain pointer is disallowed to
   avoid confusion about whether this acquires a new reference;
   instead use the "reset" method -- which, like the pointer
   constructor, transfers ownership.

   The policy class must provide two static methods:
   void incref (T *);
   void decref (T *);
*/
template<typename T, typename Policy>
class ref_ptr
{
 public:

  /* Create a new NULL instance.  */
  ref_ptr ()
    : m_obj (NULL)
  {
  }

  /* Create a new NULL instance.  Note that this is not explicit.  */
  ref_ptr (const std::nullptr_t)
    : m_obj (NULL)
  {
  }

  /* Create a new instance.  OBJ is a reference, management of which
     is now transferred to this class.  */
  explicit ref_ptr (T *obj)
    : m_obj (obj)
  {
  }

  /* Copy another instance.  */
  ref_ptr (const ref_ptr &other)
    : m_obj (other.m_obj)
  {
    if (m_obj != NULL)
      Policy::incref (m_obj);
  }

  /* Transfer ownership from OTHER.  */
  ref_ptr (ref_ptr &&other) noexcept
    : m_obj (other.m_obj)
  {
    other.m_obj = NULL;
  }

  /* Destroy this instance.  */
  ~ref_ptr ()
  {
    if (m_obj != NULL)
      Policy::decref (m_obj);
  }

  /* Copy another instance.  */
  ref_ptr &operator= (const ref_ptr &other)
  {
    /* Do nothing on self-assignment.  */
    if (this != &other)
      {
	reset (other.m_obj);
	if (m_obj != NULL)
	  Policy::incref (m_obj);
      }
    return *this;
  }

  /* Transfer ownership from OTHER.  */
  ref_ptr &operator= (ref_ptr &&other)
  {
    /* Do nothing on self-assignment.  */
    if (this != &other)
      {
	reset (other.m_obj);
	other.m_obj = NULL;
      }
    return *this;
  }

  /* Change this instance's referent.  OBJ is a reference, management
     of which is now transferred to this class.  */
  void reset (T *obj)
  {
    if (m_obj != NULL)
      Policy::decref (m_obj);
    m_obj = obj;
  }

  /* Return this instance's referent without changing the state of
     this class.  */
  T *get () const
  {
    return m_obj;
  }

  /* Return this instance's referent, and stop managing this
     reference.  The caller is now responsible for the ownership of
     the reference.  */
  ATTRIBUTE_UNUSED_RESULT T *release ()
  {
    T *result = m_obj;

    m_obj = NULL;
    return result;
  }

  /* Let users refer to members of the underlying pointer.  */
  T *operator-> () const
  {
    return m_obj;
  }

  /* Acquire a new reference and return a ref_ptr that owns it.  */
  static ref_ptr<T, Policy> new_reference (T *obj)
  {
    Policy::incref (obj);
    return ref_ptr<T, Policy> (obj);
  }

 private:

  T *m_obj;
};

template<typename T, typename Policy>
inline bool operator== (const ref_ptr<T, Policy> &lhs,
			const ref_ptr<T, Policy> &rhs)
{
  return lhs.get () == rhs.get ();
}

template<typename T, typename Policy>
inline bool operator== (const ref_ptr<T, Policy> &lhs, const T *rhs)
{
  return lhs.get () == rhs;
}

template<typename T, typename Policy>
inline bool operator== (const ref_ptr<T, Policy> &lhs, const std::nullptr_t)
{
  return lhs.get () == nullptr;
}

template<typename T, typename Policy>
inline bool operator== (const T *lhs, const ref_ptr<T, Policy> &rhs)
{
  return lhs == rhs.get ();
}

template<typename T, typename Policy>
inline bool operator== (const std::nullptr_t, const ref_ptr<T, Policy> &rhs)
{
  return nullptr == rhs.get ();
}

template<typename T, typename Policy>
inline bool operator!= (const ref_ptr<T, Policy> &lhs,
			const ref_ptr<T, Policy> &rhs)
{
  return lhs.get () != rhs.get ();
}

template<typename T, typename Policy>
inline bool operator!= (const ref_ptr<T, Policy> &lhs, const T *rhs)
{
  return lhs.get () != rhs;
}

template<typename T, typename Policy>
inline bool operator!= (const ref_ptr<T, Policy> &lhs, const std::nullptr_t)
{
  return lhs.get () != nullptr;
}

template<typename T, typename Policy>
inline bool operator!= (const T *lhs, const ref_ptr<T, Policy> &rhs)
{
  return lhs != rhs.get ();
}

template<typename T, typename Policy>
inline bool operator!= (const std::nullptr_t, const ref_ptr<T, Policy> &rhs)
{
  return nullptr != rhs.get ();
}

}

#endif /* COMMON_GDB_REF_PTR_H */
