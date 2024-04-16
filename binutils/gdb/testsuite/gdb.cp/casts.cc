#include <cstdint>

struct A
{
  int a;
  A (int aa): a (aa) {}
};

struct B: public A
{
  int b;
  B (int aa, int bb): A (aa), b(bb) {}
};


struct Alpha
{
  virtual void x() { }
};

struct Gamma
{
};

struct Derived : public Alpha
{
};

struct VirtuallyDerived : public virtual Alpha
{
};

struct DoublyDerived : public VirtuallyDerived,
		       public virtual Alpha,
		       public Gamma
{
};

struct Left
{
  int left;
};

struct Right
{
  int right;
};

struct LeftRight : public Left, public Right
{
};

struct VirtualLeft
{
  virtual ~VirtualLeft () {}

  int left;
};

struct VirtualRight
{
  virtual ~VirtualRight () {}

  int right;
};

struct VirtualLeftRight : public VirtualLeft, public VirtualRight
{
};

int
main (int argc, char **argv)
{
  A *a = new B(42, 1729);
  B *b = (B *) a;
  A &ar = *b;
  B &br = (B&)ar;

  Derived derived;
  DoublyDerived doublyderived;

  Alpha *ad = &derived;
  Alpha *add = &doublyderived;

  LeftRight gd;
  gd.left = 23;
  gd.right = 27;
  unsigned long long gd_value = (unsigned long long) (std::uintptr_t)&gd;
  unsigned long long r_value = (unsigned long long) (Right *) &gd;

  VirtualLeftRight *vlr = new VirtualLeftRight ();
  VirtualLeft *vl = vlr;
  VirtualRight *vr = vlr;

  return 0;  /* breakpoint spot: casts.exp: 1 */
}
