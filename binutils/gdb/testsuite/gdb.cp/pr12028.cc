class A{};
class B{};
class C: public B {};

namespace D{
  int foo (A) { return 11; }
  int foo (C) { return 12; }
}

int main()
{
  A a;
  B b;
  C c;

  D::foo (a);
  //  D::foo (b);
  D::foo (c);

  return 0;
}
