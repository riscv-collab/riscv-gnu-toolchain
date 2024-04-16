/* Observers

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

#ifndef COMMON_OBSERVABLE_H
#define COMMON_OBSERVABLE_H

#include <algorithm>
#include <functional>
#include <vector>

/* Print an "observer" debug statement.  */

#define observer_debug_printf(fmt, ...) \
  debug_prefixed_printf_cond (observer_debug, "observer", fmt, ##__VA_ARGS__)

/* Print "observer" start/end debug statements.  */

#define OBSERVER_SCOPED_DEBUG_START_END(fmt, ...) \
  scoped_debug_start_end (observer_debug, "observer", fmt, ##__VA_ARGS__)

namespace gdb
{

namespace observers
{

extern bool observer_debug;

/* An observer is an entity which is interested in being notified
   when GDB reaches certain states, or certain events occur in GDB.
   The entity being observed is called the observable.  To receive
   notifications, the observer attaches a callback to the observable.
   One observable can have several observers.

   The observer implementation is also currently not reentrant.  In
   particular, it is therefore not possible to call the attach or
   detach routines during a notification.  */

/* The type of a key that can be passed to attach, which can be passed
   to detach to remove associated observers.  Tokens have address
   identity, and are thus usually const globals.  */
struct token
{
  token () = default;

  DISABLE_COPY_AND_ASSIGN (token);
};

namespace detail
{
  /* Types that don't depend on any template parameter.  This saves a
     bit of code and debug info size, compared to putting them inside
     class observable.  */

  /* Use for sorting algorithm, to indicate which observer we have
     visited.  */
  enum class visit_state
  {
    NOT_VISITED,
    VISITING,
    VISITED,
  };
}

template<typename... T>
class observable
{
public:
  typedef std::function<void (T...)> func_type;

private:
  struct observer
  {
    observer (const struct token *token, func_type func, const char *name,
	      const std::vector<const struct token *> &dependencies)
      : token (token), func (func), name (name), dependencies (dependencies)
    {}

    const struct token *token;
    func_type func;
    const char *name;
    std::vector<const struct token *> dependencies;
  };

public:
  explicit observable (const char *name)
    : m_name (name)
  {
  }

  DISABLE_COPY_AND_ASSIGN (observable);

  /* Attach F as an observer to this observable.  F cannot be detached or
     specified as a dependency.

     DEPENDENCIES is a list of tokens of observers to be notified before this
     one.

     NAME is the name of the observer, used for debug output purposes.  Its
     lifetime must be at least as long as the observer is attached.  */
  void attach (const func_type &f, const char *name,
	       const std::vector<const struct token *> &dependencies = {})
  {
    attach (f, nullptr, name, dependencies);
  }

  /* Attach F as an observer to this observable.

     T is a reference to a token that can be used to later remove F or specify F
     as a dependency of another observer.

     DEPENDENCIES is a list of tokens of observers to be notified before this
     one.

     NAME is the name of the observer, used for debug output purposes.  Its
     lifetime must be at least as long as the observer is attached.  */
  void attach (const func_type &f, const token &t, const char *name,
	       const std::vector<const struct token *> &dependencies = {})
  {
    attach (f, &t, name, dependencies);
  }

  /* Remove observers associated with T from this observable.  T is
     the token that was previously passed to any number of "attach"
     calls.  */
  void detach (const token &t)
  {
    auto iter = std::remove_if (m_observers.begin (),
				m_observers.end (),
				[&] (const observer &o)
				{
				  return o.token == &t;
				});

    observer_debug_printf ("Detaching observable %s from observer %s",
			   iter->name, m_name);

    m_observers.erase (iter, m_observers.end ());
  }

  /* Notify all observers that are attached to this observable.  */
  void notify (T... args) const
  {
    OBSERVER_SCOPED_DEBUG_START_END ("observable %s notify() called", m_name);

    for (auto &&e : m_observers)
      {
	OBSERVER_SCOPED_DEBUG_START_END ("calling observer %s of observable %s",
					 e.name, m_name);
	e.func (args...);
      }
  }

private:

  std::vector<observer> m_observers;
  const char *m_name;

  /* Helper method for topological sort using depth-first search algorithm.

     Visit all dependencies of observer at INDEX in M_OBSERVERS (later referred
     to as "the observer").  Then append the observer to SORTED_OBSERVERS.

     If the observer is already visited, do nothing.  */
  void visit_for_sorting (std::vector<observer> &sorted_observers,
			  std::vector<detail::visit_state> &visit_states,
			  int index)
  {
    if (visit_states[index] == detail::visit_state::VISITED)
      return;

    /* If we are already visiting this observer, it means there's a cycle.  */
    gdb_assert (visit_states[index] != detail::visit_state::VISITING);

    visit_states[index] = detail::visit_state::VISITING;

    /* For each dependency of this observer...  */
    for (const token *dep : m_observers[index].dependencies)
      {
	/* ... find the observer that has token DEP.  If found, visit it.  */
	auto it_dep
	  = std::find_if (m_observers.begin (), m_observers.end (),
			    [&] (observer o) { return o.token == dep; });
	if (it_dep != m_observers.end ())
	{
	  int i = std::distance (m_observers.begin (), it_dep);
	  visit_for_sorting (sorted_observers, visit_states, i);
	}
      }

    visit_states[index] = detail::visit_state::VISITED;
    sorted_observers.push_back (m_observers[index]);
  }

  /* Sort the observers, so that dependencies come before observers
     depending on them.

     Uses depth-first search algorithm for topological sorting, see
     https://en.wikipedia.org/wiki/Topological_sorting#Depth-first_search .  */
  void sort_observers ()
  {
    std::vector<observer> sorted_observers;
    std::vector<detail::visit_state> visit_states
      (m_observers.size (), detail::visit_state::NOT_VISITED);

    for (size_t i = 0; i < m_observers.size (); i++)
      visit_for_sorting (sorted_observers, visit_states, i);

    m_observers = std::move (sorted_observers);
  }

  void attach (const func_type &f, const token *t, const char *name,
	       const std::vector<const struct token *> &dependencies)
  {

    observer_debug_printf ("Attaching observable %s to observer %s",
			   name, m_name);

    m_observers.emplace_back (t, f, name, dependencies);

    /* The observer has been inserted at the end of the vector, so it will be
       after any of its potential dependencies attached earlier.  If the
       observer has a token, it means that other observers can specify it as
       a dependency, so sorting is necessary to ensure those will be after the
       newly inserted observer afterwards.  */
    if (t != nullptr)
      sort_observers ();
  };
};

} /* namespace observers */

} /* namespace gdb */

#endif /* COMMON_OBSERVABLE_H */
