/* This testcase is part of GDB, the GNU debugger.

   Copyright 2008-2024 Free Software Foundation, Inc.

   Contributed by Red Hat, originally written by Keith Seitz.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Please email any bugs, comments, and/or additions to this file to:
   bug-gdb@gnu.org  */

#include <stdlib.h>
#include <iostream>

// Forward decls
class base;
class derived;

// A simple template with specializations
template <typename T>
class tclass
{
public:
  void do_something () { } // tclass<T>::do_something
};

template <>
void tclass<char>::do_something () { } // tclass<char>::do_something

template <>
void tclass<int>::do_something () { } // tclass<int>::do_something

template<>
void tclass<long>::do_something () { } // tclass<long>::do_something

template<>
void tclass<short>::do_something () { } // tclass<short>::do_something

// A simple template with multiple template parameters
template <class A, class B, class C, class D, class E>
void flubber (void) { // flubber
  A a;
  B b;
  C c;
  D d;
  E e;

  ++a;
  ++b;
  ++c;
  ++d;
  ++e;
}

// Some contrived policies
template <class T>
struct operation_1
{
  static void function (void) { } // operation_1<T>::function
};

template <class T>
struct operation_2
{
  static void function (void) { } // operation_2<T>::function
};

template <class T>
struct operation_3
{
  static void function (void) { } // operation_3<T>::function
};

template <class T>
struct operation_4
{
  static void function (void) { } // operation_4<T>::function
};

// A policy-based class w/ and w/o default policy
template <class T, class Policy>
class policy : public Policy
{
public:
  policy (T obj) : obj_ (obj) { } // policy<T, Policy>::policy

private:
  T obj_;
};

template <class T, class Policy = operation_1<T> >
class policyd : public Policy
{
public:
  policyd (T obj) : obj_ (obj) { } // policyd<T, Policy>::policyd
  ~policyd (void) { } // policyd<T, Policy>::~policyd

private:
  T obj_;
};

typedef policy<int, operation_1<void*> > policy1;
typedef policy<int, operation_2<void*> > policy2;
typedef policy<int, operation_3<void*> > policy3;
typedef policy<int, operation_4<void*> > policy4;

typedef policyd<int> policyd1;
typedef policyd<long> policyd2;
typedef policyd<char> policyd3;
typedef policyd<base> policyd4;
typedef policyd<tclass<int> > policyd5;

class fluff { };
static fluff *g_fluff = new fluff ();

class base
{
protected:
  int foo_;

public:
  base (void) : foo_ (42) { } // base::base(void)
  base (int foo) : foo_ (foo) { } // base::base(int)
  ~base (void) { } // base::~base

  // Some overloaded methods
  int overload (void) const { return 0; } // base::overload(void) const
  int overload (int i) const { return 1; } // base::overload(int) const
  int overload (short s) const { return 2; } // base::overload(short) const
  int overload (long l) const { return 3; } // base::overload(long) const
  int overload (char* a) const { return 4; } // base::overload(char*) const
  int overload (base& b) const { return 5; } // base::overload(base&) const

  // Operators
  int operator+ (base const& o) const { // base::operator+
    return foo_ + o.foo_;  }

  base operator++ (void) { // base::operator++
    ++foo_; return *this; }

  base operator+=(base const& o) { // base::operator+=
    foo_ += o.foo_; return *this; }

  int operator- (base const& o) const { // base::operator-
    return foo_ - o.foo_; }

  base operator-- (void) { // base::operator--
    --foo_; return *this; }

  base operator-= (base const& o) { // base::operator-=
    foo_ -= o.foo_; return *this; }

  int operator* (base const& o) const { // base::operator*
    return foo_ * o.foo_; }

  base operator*= (base const& o) { // base::operator*=
    foo_ *= o.foo_; return *this; }

  int operator/ (base const& o) const { // base::operator/
    return foo_ / o.foo_; }

  base operator/= (base const& o) { // base::operator/=
    foo_ /= o.foo_; return *this; }

  int operator% (base const& o) const { // base::operator%
    return foo_ % o.foo_; }
  
  base operator%= (base const& o) { // base::operator%=
    foo_ %= o.foo_; return *this; }

  bool operator< (base const& o) const { // base::operator<
    return foo_ < o.foo_; }

  bool operator<= (base const& o) const { // base::operator<=
    return foo_ <= o.foo_; }

  bool operator> (base const& o) const { // base::operator>
    return foo_ > o.foo_; }

  bool operator>= (base const& o) const { // base::operator>=
    return foo_ >= o.foo_; }

  bool operator!= (base const& o) const { // base::operator!=
    return foo_ != o.foo_; }

  bool operator== (base const& o) const { // base::operator==
    return foo_ == o.foo_; }

  bool operator! (void) const { // base::operator!
    return !foo_; }

  bool operator&& (base const& o) const { // base::operator&&
    return foo_ && o.foo_; }

  bool operator|| (base const& o) const { // base::operator||
    return foo_ || o.foo_; }

  int operator<< (int value) const { // base::operator<<
    return foo_  << value; }

  base operator<<= (int value) { // base::operator<<=
    foo_ <<= value; return *this; }

  int operator>> (int value) const { // base::operator>>
    return foo_  >> value; }

  base operator>>= (int value) { // base::operator>>=
    foo_ >>= value; return *this; }

  int operator~ (void) const { // base::operator~
    return ~foo_; }

  int operator& (base const& o) const { // base::operator&
    return foo_ & o.foo_; }

  base operator&= (base const& o) { // base::operator&=
    foo_ &= o.foo_; return *this; }

  int operator| (base const& o) const { // base::operator|
    return foo_ | o.foo_; }

  base operator|= (base const& o) { // base::operator|=
    foo_ |= o.foo_; return *this; }
  
  int operator^ (base const& o) const { // base::operator^
    return foo_ ^ o.foo_; }

  base operator^= (base const& o) { // base::operator^=
    foo_ ^= o.foo_; return *this; }

  base operator= (base const& o) { // base::operator=
    foo_ = o.foo_; return *this; }

  void operator() (void) const { // base::operator()
    return; }

  int operator[] (int idx) const { // base::operator[]
    return idx; }

  void* operator new (size_t size) throw () { // base::operator new
    return malloc (size); }

  void operator delete (void* ptr) { // base::operator delete
    free (ptr); }

  void* operator new[] (size_t size) throw () { // base::operator new[]
    return malloc (size); }

  void operator delete[] (void* ptr) { // base::operator delete[]
    free (ptr); }

  base const* operator-> (void) const { // base::operator->
    return this; }

  int operator->* (base const& b) const { // base::operator->*
    return foo_ * b.foo_; }

  operator char* () const { return const_cast<char*> ("hello"); } // base::operator char*
  operator int () const { return 21; } // base::operator int
  operator fluff* () const { return new fluff (); } // base::operator fluff*
  operator fluff** () const { return &g_fluff; } // base::operator fluff**
  operator fluff const* const* () const { return &g_fluff; } // base::operator fluff const* const*
};

class base1 : public virtual base
{
public:
  base1 (void) : foo_ (21) { } // base1::base1(void)
  base1 (int a) : foo_(a) { } // base1::base1(int)
  void a_function (void) const { } // base1::a_function

protected:
  int foo_;
};

class base2 : public virtual base
{
public:
  base2 () : foo_ (3) { } // base2::base2

protected:
  void a_function (void) const { } // base2::a_function
  int foo_;
};

class derived : public base1, public base2
{
  public:
  derived(void) : foo_ (4) { } // derived::derived
  void a_function (void) const { // derived::a_function
    this->base1::a_function ();
    this->base2::a_function ();
  }

  protected:
  int foo_;
};

class CV { public:
  static const int i;
  typedef int t;
  void m(t);
  void m(t) const;
  void m(t) volatile;
  void m(t) const volatile;
};
const int CV::i = 42;
#ifdef __GNUC__
# define ATTRIBUTE_USED __attribute__((used))
#else
# define ATTRIBUTE_USED
#endif
ATTRIBUTE_USED void CV::m(CV::t) {}
ATTRIBUTE_USED void CV::m(CV::t) const {}
ATTRIBUTE_USED void CV::m(CV::t) volatile {}
ATTRIBUTE_USED void CV::m(CV::t) const volatile {}
int CV_f (int x)
{
  return x + 1;
}

int
test_function (int argc, char* argv[]) // test_function
{ // test_function
  derived d;
  void (derived::*pfunc) (void) const = &derived::a_function;
  (d.*pfunc) ();

  base a (1), b (3), c (8);
  (void) a.overload ();
  (void) a.overload (static_cast<int> (0));
  (void) a.overload (static_cast<short> (0));
  (void) a.overload (static_cast<long> (0));
  (void) a.overload (static_cast<char*> (0));
  (void) a.overload (a);

  int r;
  r = b + c;
  ++a;
  a += b;
  r = b - c;
  --a;
  a -= b;
  r = b * c;
  a *= b;
  r = b / c;
  a /= b;
  r = b % c;
  a %= b;
  bool x = (b < c);
  x = (b <= c);
  x = (b > c);
  x = (b >= c);
  x = (b != c);
  x = (b == c);
  x = (!b);
  x = (b && c);
  x = (b || c);
  r = b << 2;
  a <<= 1;
  r = b >> 2;
  a >>= 1;
  r = ~b;
  r = b & c;
  a &= c;
  r = b | c;
  a |= c;
  r = b ^ c;
  a ^= c;
  a = c;
  a ();
  int i = a[3];
  derived* f = new derived ();
  derived* g = new derived[3];
  delete f;
  delete[] g;
  a->overload ();
  r = a->*b;

  tclass<char> char_tclass;
  tclass<int> int_tclass;
  tclass<short> short_tclass;
  tclass<long> long_tclass;
  tclass<base> base_tclass;
  char_tclass.do_something ();
  int_tclass.do_something ();
  short_tclass.do_something ();
  long_tclass.do_something ();
  base_tclass.do_something ();

  flubber<int, int, int, int, int> ();
  flubber<int, int, int, int, short> ();
  flubber<int, int, int, int, long> ();
  flubber<int, int, int, int, char> ();
  flubber<int, int, int, short, int> ();
  flubber<int, int, int, short, short> ();
  flubber<int, int, int, short, long> ();
  flubber<int, int, int, short, char> ();
  flubber<int, int, int, long, int> ();
  flubber<int, int, int, long, short> ();
  flubber<int, int, int, long, long> ();
  flubber<int, int, int, long, char> ();
  flubber<int, int, int, char, int> ();
  flubber<int, int, int, char, short> ();
  flubber<int, int, int, char, long> ();
  flubber<int, int, int, char, char> ();
  flubber<int, int, short, int, int> ();
  flubber<int, int, short, int, short> ();
  flubber<int, int, short, int, long> ();
  flubber<int, int, short, int, char> ();
  flubber<int, int, short, short, int> ();
  flubber<short, int, short, int, short> ();
  flubber<long, short, long, short, long> ();

  policy1 p1 (1);
  p1.function ();
  policy2 p2 (2);
  p2.function ();
  policy3 p3 (3);
  p3.function ();
  policy4 p4 (4);
  p4.function ();

  policyd1 pd1 (5);
  pd1.function ();
  policyd2 pd2 (6);
  pd2.function ();
  policyd3 pd3 (7);
  pd3.function ();
  policyd4 pd4 (d);
  pd4.function ();
  policyd5 pd5 (int_tclass);
  pd5.function ();

  base1 b1 (3);

  r = a;
  char* str = a;
  fluff* flp = a;
  fluff** flpp = a;
  fluff const* const* flcpcp = a;

  CV_f(CV::i);

  return 0;
}

int
main (int argc, char* argv[])
{
  int i;

  /* Call the test function repeatedly, enough times for all our tests
     without running forever if something goes wrong.  */
  for (i = 0; i < 1000; i++)
    test_function (argc, argv);

  return 0;
}
