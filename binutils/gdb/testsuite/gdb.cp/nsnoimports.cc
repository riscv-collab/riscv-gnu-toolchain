
namespace A
{
  int _a = 11;

  namespace B{

    int ab = 22;

    namespace C{

      int abc = 33;

      int second(){
        return 0;
      }

    }

    int first(){
      (void) _a;
      (void) ab;
      (void) C::abc;
      return C::second();
    }
  }
}


int
main()
{
  (void) A::_a;
  (void) A::B::ab;
  (void) A::B::C::abc;
  return A::B::first();
}
