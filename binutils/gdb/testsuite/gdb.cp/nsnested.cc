namespace A
{
  namespace B
  {
    int ab = 11;
  }
}

namespace C
{
  namespace D
  {
    using namespace A::B;

    int
    second()
    {
      (void) ab;
      return 0;
    }
  }

  int
  first()
  {
    //ab;
    return D::second();
  }
}

int
main()
{
  //ab;
  return C::first();
}
