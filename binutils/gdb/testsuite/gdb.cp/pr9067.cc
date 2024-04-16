struct B;

struct A {
  static B b;
};

struct B {
  A a;
};

B A::b;
B b;

int main(int,char **)
{
  return 0;
}
