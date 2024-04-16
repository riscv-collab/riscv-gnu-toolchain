/* Base class of intrusively reference-counted objects.
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

#ifndef COMMON_REFCOUNTED_OBJECT_H
#define COMMON_REFCOUNTED_OBJECT_H

/* Base class of intrusively reference-countable objects.
   Incrementing and decrementing the reference count is an external
   responsibility.  */

class refcounted_object
{
public:
  refcounted_object () = default;

  /* Increase the refcount.  */
  void incref ()
  {
    gdb_assert (m_refcount >= 0);
    m_refcount++;
  }

  /* Decrease the refcount.  */
  void decref ()
  {
    m_refcount--;
    gdb_assert (m_refcount >= 0);
  }

  int refcount () const { return m_refcount; }

private:
  DISABLE_COPY_AND_ASSIGN (refcounted_object);

  /* The reference count.  */
  int m_refcount = 0;
};

/* A policy class to interface gdb::ref_ptr with a
   refcounted_object.  */

struct refcounted_object_ref_policy
{
  static void incref (refcounted_object *ptr)
  {
    ptr->incref ();
  }

  static void decref (refcounted_object *ptr)
  {
    ptr->decref ();
  }
};

/* A policy class to interface gdb::ref_ptr with a refcounted_object, that
   deletes the object once the refcount reaches 0..  */

template<typename T>
struct refcounted_object_delete_ref_policy
{
  static void incref (T *obj)
  { obj->incref (); }

  static void decref (T *obj)
  {
    obj->decref ();
    if (obj->refcount () == 0)
      delete obj;
  }
};

#endif /* COMMON_REFCOUNTED_OBJECT_H */
