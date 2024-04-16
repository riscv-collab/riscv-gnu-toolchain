
class Base
{
public:
  virtual int get_foo () { return 1; }
  int base_function_only () { return 2; }
};

class Foo : public Base
{

private:
  int foo_value;

public:
  Foo () { foo_value = 0;}
  Foo (int i) { foo_value = i;}
  ~Foo () { }
  void set_foo (int value);
  int get_foo ();

  // Something similar to a constructor name.
  void Foofoo ();

  bool operator== (const Foo &other) { return foo_value == other.foo_value; }
};
 
void Foo::set_foo (int value)
{
  foo_value = value;
}

int Foo::get_foo ()
{
  return foo_value;
}

void Foo::Foofoo ()
{
}

namespace Test_NS {

int foo;
int bar;

namespace Nested {

int qux;

} /* namespace Nested */

} /* namespace Test_NS */

/* The important thing with class baz is that both the class and the
   constructor must have a template argument, we need the symbol to look
   like:

   baz<TYPE_1>::baz<TYPE_2>(int,....whatever...)

   It doesn't really matter if TYPE_1 and TYPE_2 are the same or different,
   but we create them differently in this test as it makes debugging GDB
   slightly easier.  */

template<typename S>
struct baz
{
  template<typename T>
  baz (int p1, T a)
  {
    s = 0;
  }

  S s;
};

int main ()
{
  baz<int> obj (2.3, 0.1);
  // Anonymous struct with method.
  struct {
    int get() { return 5; }
  } a;
  Foo foo1;
  foo1.set_foo (42);		// Set breakpoint here.
  a.get();			// Prevent compiler from throwing 'a' away.
  return 0;
}
