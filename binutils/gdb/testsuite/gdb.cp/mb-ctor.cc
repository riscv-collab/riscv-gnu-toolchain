
#include <stdio.h>

class Base 
{
public:
  Base(int k);
  ~Base();
  virtual void foo() {}
private:
  int k;
};

Base::Base(int k)
{
  this->k = k;
}

Base::~Base()
{
    printf("~Base\n");
}

class Derived : public virtual Base
{
public:
  Derived(int i);
  ~Derived();
private:
  int i;
  int i2;
};

Derived::Derived(int i) : Base(i)
{
  this->i = i;
  /* The next statement is spread over two lines on purpose to exercise
     a bug where breakpoints set on all but the last line of a statement
     would not get multiple breakpoints.
     The second line's text for gdb_get_line_number is a subset of the
     first line so that we don't care which line gdb prints when it stops.  */
  this->i2 = // set breakpoint here
    i; // breakpoint here
}

Derived::~Derived()
{
    printf("~Derived\n");
}

class DeeplyDerived : public Derived
{
public:
  DeeplyDerived(int i) : Base(i), Derived(i) {}
};

int main()
{
  /* Invokes the Derived ctor that constructs both
     Derived and Base.  */
  Derived d(7);
  /* Invokes the Derived ctor that constructs only
     Derived. Base is constructed separately by
     DeeplyDerived's ctor.  */
  DeeplyDerived dd(15);

  Derived *dyn_d = new Derived (24);
  DeeplyDerived *dyn_dd = new DeeplyDerived (42);

  delete dyn_d;
  delete dyn_dd;

  return 0;
}
