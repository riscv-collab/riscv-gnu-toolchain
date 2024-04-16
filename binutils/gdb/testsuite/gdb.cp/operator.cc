class A
{
};

int operator== (A, int)
{
  return 11;
}

int operator== (A, char)
{
  return 12;
}

//------------------

namespace B
{
  class C
  {
  };

  int operator== (C, int)
  {
    return 22;
  }

  int operator== (C, char)
  {
    return 23;
  }

  namespace BD
  {
    int operator== (C, int)
    {
      return 24;
    }
  }
}

//------------------

class D
{
};
namespace
{
  int operator== (D, int)
  {
    return 33;
  }

  int operator== (D, char)
  {
    return 34;
  }
}

int operator== (D, float)
{
  return 35;
}

//------------------

class E
{
};
namespace F
{
  int operator== (E, int)
  {
    return 44;
  }

  int operator== (E, char)
  {
    return 45;
  }
}

int operator== (E, float)
{
  return 46;
}

using namespace F;

//-----------------

class G
{
public:
  int operator== (int)
  {
    return 55;
  }
};

int operator== (G, char)
{
  return 56;
}

//------------------

class H
{
};
namespace I
{
  int operator== (H, int)
  {
    return 66;
  }
}

namespace ALIAS = I;

//------------------

class J
{
};

namespace K
{
  int i;
  int operator== (J, int)
  {
    return 77;
  }
}

using K::i;

//------------------

class L
{
};
namespace M
{
  int operator== (L, int)
  {
    return 88;
  }
}

namespace N
{
  using namespace M;
}

using namespace N;

//------------------

namespace O
{
  namespace P
    {
      using namespace ::O;
    }
  using namespace P;
}

using namespace O;

class test { };
test x;

//------------------

int main ()
{
  A a;
  a == 1;
  a == 'a';

  B::C bc;
  bc == 1;
  bc == 'a';
  B::BD::operator== (bc,'a');

  D d;
  d == 1;
  d == 'a';
  d == 1.0f;

  E e;
  e == 1;
  e == 'a';
  e == 1.0f;

  G g;
  g == 1;
  g == 'a';

  H h;
  I::operator== (h, 1);

  J j;
  K::operator== (j, 1);

  L l;
  l == 1;

  return 0;
}
