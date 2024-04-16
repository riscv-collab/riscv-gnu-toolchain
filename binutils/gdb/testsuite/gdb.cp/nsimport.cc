namespace A {
  int x = 11;
  namespace{
    int xx = 22;
  }
}

using namespace A;

namespace{
  int xxx = 33;
};

int main()
{
  (void) x;
  (void) xx;
  (void) xxx;
  return 0;
}
