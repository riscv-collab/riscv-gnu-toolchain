/* scoped_restore, a simple class for saving and restoring a value

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

#ifndef COMMON_SCOPED_RESTORE_H
#define COMMON_SCOPED_RESTORE_H

/* Base class for scoped_restore_tmpl.  */
class scoped_restore_base
{
public:
  /* This informs the (scoped_restore_tmpl<T>) dtor that you no longer
     want the original value restored.  */
  void release () const
  { m_saved_var = NULL; }

protected:
  scoped_restore_base (void *saved_var)
    : m_saved_var (saved_var)
  {}

  /* The type-erased saved variable.  This is here so that clients can
     call release() on a "scoped_restore" local, which is a typedef to
     a scoped_restore_base.  See below.  */
  mutable void *m_saved_var;
};

/* A convenience typedef.  Users of make_scoped_restore declare the
   local RAII object as having this type.  */
typedef const scoped_restore_base &scoped_restore;

/* An RAII-based object that saves a variable's value, and then
   restores it again when this object is destroyed. */
template<typename T>
class scoped_restore_tmpl : public scoped_restore_base
{
 public:

  /* Create a new scoped_restore object that saves the current value
     of *VAR.  *VAR will be restored when this scoped_restore object
     is destroyed.  */
  scoped_restore_tmpl (T *var)
    : scoped_restore_base (var),
      m_saved_value (*var)
  {
  }

  /* Create a new scoped_restore object that saves the current value
     of *VAR, and sets *VAR to VALUE.  *VAR will be restored when this
     scoped_restore object is destroyed.  This is templated on T2 to
     allow passing VALUEs of types convertible to T.
     E.g.: T='base'; T2='derived'.  */
  template <typename T2>
  scoped_restore_tmpl (T *var, T2 value)
    : scoped_restore_base (var),
      m_saved_value (*var)
  {
    *var = value;
  }

  scoped_restore_tmpl (const scoped_restore_tmpl<T> &other)
    : scoped_restore_base {other.m_saved_var},
      m_saved_value (other.m_saved_value)
  {
    other.m_saved_var = NULL;
  }

  ~scoped_restore_tmpl ()
  {
    if (saved_var () != NULL)
      *saved_var () = m_saved_value;
  }

private:
  /* Return a pointer to the saved variable with its type
     restored.  */
  T *saved_var ()
  { return static_cast<T *> (m_saved_var); }

  /* No need for this.  It is intentionally not defined anywhere.  */
  scoped_restore_tmpl &operator= (const scoped_restore_tmpl &);

  /* The saved value.  */
  const T m_saved_value;
};

/* Make a scoped_restore.  This is useful because it lets template
   argument deduction work.  */
template<typename T>
scoped_restore_tmpl<T> make_scoped_restore (T *var)
{
  return scoped_restore_tmpl<T> (var);
}

/* Make a scoped_restore.  This is useful because it lets template
   argument deduction work.  */
template<typename T, typename T2>
scoped_restore_tmpl<T> make_scoped_restore (T *var, T2 value)
{
  return scoped_restore_tmpl<T> (var, value);
}

#endif /* COMMON_SCOPED_RESTORE_H */
