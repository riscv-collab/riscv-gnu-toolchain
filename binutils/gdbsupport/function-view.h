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

#ifndef COMMON_FUNCTION_VIEW_H
#define COMMON_FUNCTION_VIEW_H

/* function_view is a polymorphic type-erasing wrapper class that
   encapsulates a non-owning reference to arbitrary callable objects.

   A way to put it is that function_view is to std::function like
   std::string_view is to std::string.  While std::function stores a
   type-erased callable object internally, function_view holds a
   type-erased reference to an external callable object.

   This is meant to be used as callback type of a function that:

     #1 - Takes a callback as parameter.

     #2 - Wants to support arbitrary callable objects as callback type
	  (e.g., stateful function objects, lambda closures, free
	  functions).

     #3 - Does not store the callback anywhere; instead the function
	  just calls the callback directly or forwards it to some
	  other function that calls it.

     #4 - Can't be, or we don't want it to be, a template function
	  with the callable type as template parameter.  For example,
	  when the callback is a parameter of a virtual member
	  function, or when putting the function template in a header
	  would expose too much implementation detail.

   Note that the C-style "function pointer" + "void *data" callback
   parameter idiom fails requirement #2 above.  Please don't add new
   uses of that idiom.  I.e., something like this wouldn't work;

    typedef bool (iterate_over_foos_cb) (foo *f, void *user_data),
    void iterate_over_foos (iterate_over_foos_cb *callback, void *user_data);

    foo *find_foo_by_type (int type)
    {
      foo *found = nullptr;

      iterate_over_foos ([&] (foo *f, void *data)
	{
	  if (foo->type == type)
	    {
	      found = foo;
	      return true; // stop iterating
	    }
	  return false; // continue iterating
	}, NULL);

      return found;
    }

   The above wouldn't compile, because lambdas with captures can't be
   implicitly converted to a function pointer (because a capture means
   some context data must be passed to the lambda somehow).

   C++11 gave us std::function as type-erased wrapper around arbitrary
   callables, however, std::function is not an ideal fit for transient
   callbacks such as the use case above.  For this use case, which is
   quite pervasive, a function_view is a better choice, because while
   function_view is light and does not require any heap allocation,
   std::function is a heavy-weight object with value semantics that
   generally requires a heap allocation on construction/assignment of
   the target callable.  In addition, while it is possible to use
   std::function in such a way that avoids most of the overhead by
   making sure to only construct it with callables of types that fit
   std::function's small object optimization, such as function
   pointers and std::reference_wrapper callables, that is quite
   inconvenient in practice, because restricting to free-function
   callables would imply no state/capture/closure, which we need in
   most cases, and std::reference_wrapper implies remembering to use
   std::ref/std::cref where the callable is constructed, with the
   added inconvenience that std::ref/std::cref have deleted rvalue-ref
   overloads, meaning you can't use unnamed/temporary lambdas with
   them.

   Note that because function_view is a non-owning view of a callable,
   care must be taken to ensure that the callable outlives the
   function_view that calls it.  This is not really a problem for the
   use case function_view is intended for, such as passing a temporary
   function object / lambda to a function that accepts a callback,
   because in those cases, the temporary is guaranteed to be live
   until the called function returns.

   Calling a function_view with no associated target is undefined,
   unlike with std::function, which throws std::bad_function_call.
   This is by design, to avoid the otherwise necessary NULL check in
   function_view::operator().

   Since function_view objects are small (a pair of pointers), they
   should generally be passed around by value.

   Usage:

   Given this function that accepts a callback:

    void
    iterate_over_foos (gdb::function_view<void (foo *)> callback)
    {
       for (auto &foo : foos)
	 callback (&foo);
    }

   you can call it like this, passing a lambda as callback:

    iterate_over_foos ([&] (foo *f)
      {
	process_one_foo (f);
      });

   or like this, passing a function object as callback:

    struct function_object
    {
      void operator() (foo *f)
      {
	if (s->check ())
	  process_one_foo (f);
      }

      // some state
      state *s;
    };

    state mystate;
    function_object matcher {&mystate};
    iterate_over_foos (matcher);

  or like this, passing a function pointer as callback:

    iterate_over_foos (process_one_foo);

  There's also a gdb::make_function_view function that you can use to
  automatically create a function_view from a callable without having
  to specify the function_view's template parameter.  E.g.:

    auto lambda = [&] (int) { ... };
    auto fv = gdb::make_function_view (lambda);

  This can be useful for example when calling a template function
  whose function_view parameter type depends on the function's
  template parameters.  In such case, you can't rely on implicit
  callable->function_view conversion for the function_view argument.
  You must pass a function_view argument already of the right type to
  the template function.  E.g., with this:

    template<typename T>
    void my_function (T v, gdb::function_view<void(T)> callback = nullptr);

  this wouldn't compile:

    auto lambda = [&] (int) { ... };
    my_function (1, lambda);

  Note that this immediately dangles the temporary lambda object:

    gdb::function_view<void(int)> fv = [&] (int) { ... };  // dangles
    my_function (fv);

  To avoid the dangling you'd have to use a named temporary for the
  lambda:

    auto lambda = [&] (int) { ... };
    gdb::function_view<void(int)> fv = lambda;
    my_function (fv);

  Using gdb::make_function_view instead automatically deduces the
  function_view's full type, and, avoids worrying about dangling.  For
  the example above, we could write instead:

    auto lambda = [&] (int) { ... };
    my_function (1, gdb::make_function_view (lambda));

  You can find unit tests covering the whole API in
  unittests/function-view-selftests.c.  */

#include <type_traits>
namespace gdb {

namespace fv_detail {
/* Bits shared by all function_view instantiations that do not depend
   on the template parameters.  */

/* Storage for the erased callable.  This is a union in order to be
   able to save both a function object (data) pointer or a function
   pointer without triggering undefined behavior.  */
union erased_callable
{
  /* For function objects.  */
  void *data;

    /* For function pointers.  */
  void (*fn) ();
};

} /* namespace fv_detail */

/* Use partial specialization to get access to the callable's
   signature. */
template<class Signature>
struct function_view;

template<typename Res, typename... Args>
class function_view<Res (Args...)>
{
  template<typename From, typename To>
  using CompatibleReturnType
    = Or<std::is_void<To>,
	 std::is_same<From, To>,
	 std::is_convertible<From, To>>;

  /* True if Func can be called with Args, and either the result is
     Res, convertible to Res or Res is void.  */
  template<typename Callable,
	   typename Res2 = typename std::invoke_result<Callable &, Args...>::type>
  struct IsCompatibleCallable : CompatibleReturnType<Res2, Res>
  {};

  /* True if Callable is a function_view.  Used to avoid hijacking the
     copy ctor.  */
  template <typename Callable>
  struct IsFunctionView
    : std::is_same<function_view, typename std::decay<Callable>::type>
  {};

 public:

  /* NULL by default.  */
  constexpr function_view () noexcept
    : m_erased_callable {},
      m_invoker {}
  {}

  /* Default copy/assignment is fine.  */
  function_view (const function_view &) = default;
  function_view &operator= (const function_view &) = default;

  /* This is the main entry point.  Use SFINAE to avoid hijacking the
     copy constructor and to ensure that the target type is
     compatible.  */
  template
    <typename Callable,
     typename = Requires<Not<IsFunctionView<Callable>>>,
     typename = Requires<IsCompatibleCallable<Callable>>>
  function_view (Callable &&callable) noexcept
  {
    bind (callable);
  }

  /* Construct a NULL function_view.  */
  constexpr function_view (std::nullptr_t) noexcept
    : m_erased_callable {},
      m_invoker {}
  {}

  /* Clear a function_view.  */
  function_view &operator= (std::nullptr_t) noexcept
  {
    m_invoker = nullptr;
    return *this;
  }

  /* Return true if the wrapper has a target, false otherwise.  Note
     we check M_INVOKER instead of M_ERASED_CALLABLE because we don't
     know which member of the union is active right now.  */
  constexpr explicit operator bool () const noexcept
  { return m_invoker != nullptr; }

  /* Call the callable.  */
  Res operator () (Args... args) const
  { return m_invoker (m_erased_callable, std::forward<Args> (args)...); }

 private:

  /* Bind this function_view to a compatible function object
     reference.  */
  template <typename Callable>
  void bind (Callable &callable) noexcept
  {
    m_erased_callable.data = (void *) std::addressof (callable);
    m_invoker = [] (fv_detail::erased_callable ecall, Args... args)
      noexcept (noexcept (callable (std::forward<Args> (args)...))) -> Res
      {
	auto &restored_callable = *static_cast<Callable *> (ecall.data);
	/* The explicit cast to Res avoids a compile error when Res is
	   void and the callable returns non-void.  */
	return (Res) restored_callable (std::forward<Args> (args)...);
      };
  }

  /* Bind this function_view to a compatible function pointer.

     Making this a separate function allows avoiding one indirection,
     by storing the function pointer directly in the storage, instead
     of a pointer to pointer.  erased_callable is then a union in
     order to avoid storing a function pointer as a data pointer here,
     which would be undefined.  */
  template<class Res2, typename... Args2>
  void bind (Res2 (*fn) (Args2...)) noexcept
  {
    m_erased_callable.fn = reinterpret_cast<void (*) ()> (fn);
    m_invoker = [] (fv_detail::erased_callable ecall, Args... args)
      noexcept (noexcept (fn (std::forward<Args> (args)...))) -> Res
      {
	auto restored_fn = reinterpret_cast<Res2 (*) (Args2...)> (ecall.fn);
	/* The explicit cast to Res avoids a compile error when Res is
	   void and the callable returns non-void.  */
	return (Res) restored_fn (std::forward<Args> (args)...);
      };
  }

  /* Storage for the erased callable.  */
  fv_detail::erased_callable m_erased_callable;

  /* The invoker.  This is set to a capture-less lambda by one of the
     'bind' overloads.  The lambda restores the right type of the
     callable (which is passed as first argument), and forwards the
     args.  */
  Res (*m_invoker) (fv_detail::erased_callable, Args...);
};

/* Allow comparison with NULL.  Defer the work to the in-class
   operator bool implementation.  */

template<typename Res, typename... Args>
constexpr inline bool
operator== (const function_view<Res (Args...)> &f, std::nullptr_t) noexcept
{ return !static_cast<bool> (f); }

template<typename Res, typename... Args>
constexpr inline bool
operator== (std::nullptr_t, const function_view<Res (Args...)> &f) noexcept
{ return !static_cast<bool> (f); }

template<typename Res, typename... Args>
constexpr inline bool
operator!= (const function_view<Res (Args...)> &f, std::nullptr_t) noexcept
{ return static_cast<bool> (f); }

template<typename Res, typename... Args>
constexpr inline bool
operator!= (std::nullptr_t, const function_view<Res (Args...)> &f) noexcept
{ return static_cast<bool> (f); }

namespace fv_detail {

/* Helper traits type to automatically find the right function_view
   type for a callable.  */

/* Use partial specialization to get access to the callable's
   signature, for all the different callable variants.  */

template<typename>
struct function_view_traits;

/* Main partial specialization with plain function signature type.
   All others end up redirected here.  */
template<typename Res, typename... Args>
struct function_view_traits<Res (Args...)>
{
  using type = gdb::function_view<Res (Args...)>;
};

/* Function pointers.  */
template<typename Res, typename... Args>
struct function_view_traits<Res (*) (Args...)>
  : function_view_traits<Res (Args...)>
{
};

/* Function references.  */
template<typename Res, typename... Args>
struct function_view_traits<Res (&) (Args...)>
  : function_view_traits<Res (Args...)>
{
};

/* Reference to function pointers.  */
template<typename Res, typename... Args>
struct function_view_traits<Res (*&) (Args...)>
  : function_view_traits<Res (Args...)>
{
};

/* Reference to const function pointers.  */
template<typename Res, typename... Args>
struct function_view_traits<Res (* const &) (Args...)>
  : function_view_traits<Res (Args...)>
{
};

/* Const member functions.  function_view doesn't support these, but
   we need this in order to extract the type of function objects.
   Lambdas pass here, after starting at the operator() case,
   below.  */
template<typename Res, typename Class, typename... Args>
struct function_view_traits<Res (Class::*) (Args...) const>
  : function_view_traits<Res (Args...)>
{
};

/* Member functions.  Ditto, for function objects with non-const
   operator().  */
template<typename Res, typename Class, typename... Args>
struct function_view_traits<Res (Class::*) (Args...)>
  : function_view_traits<Res (Args...)>
{
};

/* Function objects, lambdas, std::function, any type that defines
   operator().  */
template<typename FuncObj>
struct function_view_traits
  : function_view_traits <decltype
			  (&std::remove_reference<FuncObj>::type::operator())>
{
};

} /* namespace fv_detail */

/* Make a function_view from a callable.  Useful to automatically
   deduce the function_view's template argument type.  */
template<typename Callable>
auto make_function_view (Callable &&callable)
  -> typename fv_detail::function_view_traits<Callable>::type
{
  using fv = typename fv_detail::function_view_traits<Callable>::type;
  return fv (std::forward<Callable> (callable));
}

} /* namespace gdb */

#endif
