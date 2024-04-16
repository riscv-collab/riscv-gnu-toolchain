namespace A
{
  int x = 11;
}

int x = 22;
int y = 0;

class B
{
public:
  int x;

  int
  func()
  {
    x = 33;
    y++; // marker1

      {
        int x = 44;
        y++; // marker2

          {
            int x = 55;
            y++; // marker3

              {
                using namespace A;
                y++; // marker4

                {
                  using A::x;
                  y++; // marker5
                }
              }
          }
      }
    return 0;
  }
};

int
main()
{
  B theB;
  return theB.func();
}
