
class Base
{
public:
  int x, y;

  Base() : x(0), y(1)
  {
  }

  virtual ~Base()
  {
    // Break here.
  }
};

class Derived : public Base
{
public:
  int z;

  Derived() : Base(), z(23)
  {
  }

  ~Derived()
  {
  }
};

int main()
{
  Derived d;

  return 0;
}
