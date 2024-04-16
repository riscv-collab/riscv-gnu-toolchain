/* Macros for general registry objects.

   Copyright (C) 2011-2024 Free Software Foundation, Inc.

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

#ifndef REGISTRY_H
#define REGISTRY_H

#include <type_traits>

template<typename T> class registry;

/* An accessor class that is used by registry_key.

   Normally, a container class has a registry<> field named
   "registry_fields".  In this case, the default accessor is used, as
   it simply returns the object.

   However, a container may sometimes need to store the registry
   elsewhere.  In this case, registry_accessor can be specialized to
   perform the needed indirection.  */

template<typename T>
struct registry_accessor
{
  /* Given a container of type T, return its registry.  */
  static registry<T> *get (T *obj)
  {
    return &obj->registry_fields;
  }
};

/* In gdb, sometimes there is a need for one module (e.g., the Python
   Type code) to attach some data to another object (e.g., an
   objfile); but it's also desirable that this be done such that the
   base object (the objfile in this example) not need to know anything
   about the attaching module (the Python code).

   This is handled using the registry system.

   A class needing to allow this sort registration can add a registry
   field.  For example, you would write:

   class some_container { registry<some_container> registry_fields; };

   The name of the field matters by default, see registry_accessor.

   A module wanting to attach data to instances of some_container uses
   the "key" class to register a key.  This key can then be passed to
   the "get" and "set" methods to handle this module's data.  */

template<typename T>
class registry
{
public:

  registry ()
    : m_fields (get_registrations ().size ())
  {
  }

  ~registry ()
  {
    clear_registry ();
  }

  DISABLE_COPY_AND_ASSIGN (registry);

  /* A type-safe registry key.

     The registry itself holds just a "void *".  This is not always
     convenient to manage, so this template class can be used instead,
     to provide a type-safe interface, that also helps manage the
     lifetime of the stored objects.

     When the container is destroyed, this key arranges to destroy the
     underlying data using Deleter.  This defaults to
     std::default_delete.  */

  template<typename DATA, typename Deleter = std::default_delete<DATA>>
  class key
  {
  public:

    key ()
      : m_key (registry<T>::new_key (cleanup))
    {
    }

    DISABLE_COPY_AND_ASSIGN (key);

    /* Fetch the data attached to OBJ that is associated with this key.
       If no such data has been attached, nullptr is returned.  */
    DATA *get (T *obj) const
    {
      registry<T> *reg_obj = registry_accessor<T>::get (obj);
      return (DATA *) reg_obj->get (m_key);
    }

    /* Attach DATA to OBJ, associated with this key.  Note that any
       previous data is simply dropped -- if destruction is needed,
       'clear' should be called.  */
    void set (T *obj, DATA *data) const
    {
      registry<T> *reg_obj = registry_accessor<T>::get (obj);
      reg_obj->set (m_key, (typename std::remove_const<DATA> *) data);
    }

    /* If this key uses the default deleter, then this method is
       available.  It emplaces a new instance of the associated data
       type and attaches it to OBJ using this key.  The arguments, if
       any, are forwarded to the constructor.  */
    template<typename Dummy = DATA *, typename... Args>
    typename std::enable_if<std::is_same<Deleter,
					 std::default_delete<DATA>>::value,
			    Dummy>::type
    emplace (T *obj, Args &&...args) const
    {
      DATA *result = new DATA (std::forward<Args> (args)...);
      set (obj, result);
      return result;
    }

    /* Clear the data attached to OBJ that is associated with this KEY.
       Any existing data is destroyed using the deleter, and the data is
       reset to nullptr.  */
    void clear (T *obj) const
    {
      DATA *datum = get (obj);
      if (datum != nullptr)
	{
	  cleanup (datum);
	  set (obj, nullptr);
	}
    }

  private:

    /* A helper function that is called by the registry to delete the
       contained object.  */
    static void cleanup (void *arg)
    {
      DATA *datum = (DATA *) arg;
      Deleter d;
      d (datum);
    }

    /* The underlying key.  */
    const unsigned m_key;
  };

  /* Clear all the data associated with this container.  This is
     dangerous and should not normally be done.  */
  void clear_registry ()
  {
    /* Call all the free functions.  */
    std::vector<registry_data_callback> &registrations
      = get_registrations ();
    unsigned last = registrations.size ();
    for (unsigned i = 0; i < last; ++i)
      {
	void *elt = m_fields[i];
	if (elt != nullptr)
	  {
	    registrations[i] (elt);
	    m_fields[i] = nullptr;
	  }
      }
  }

private:

  /* Registry callbacks have this type.  */
  typedef void (*registry_data_callback) (void *);

  /* Get a new key for this particular registry.  FREE is a callback.
     When the container object is destroyed, all FREE functions are
     called.  The data associated with the container object is passed
     to the callback.  */
  static unsigned new_key (registry_data_callback free)
  {
    std::vector<registry_data_callback> &registrations
      = get_registrations ();
    unsigned result = registrations.size ();
    registrations.push_back (free);
    return result;
  }

  /* Set the datum associated with KEY in this container.  */
  void set (unsigned key, void *datum)
  {
    m_fields[key] = datum;
  }

  /* Fetch the datum associated with KEY in this container.  If 'set'
     has not been called for this key, nullptr is returned.  */
  void *get (unsigned key)
  {
    return m_fields[key];
  }

  /* The data stored in this instance.  */
  std::vector<void *> m_fields;

  /* Return a reference to the vector of all the registrations that
     have been made.  */
  static std::vector<registry_data_callback> &get_registrations ()
  {
    static std::vector<registry_data_callback> registrations;
    return registrations;
  }
};

#endif /* REGISTRY_H */
