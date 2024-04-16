namespace A
{
  class C
  {
  public:
    static const int x = 11;
  };

  int
  first (C c)
  {
    return 11;
  }

  int
  first (int a, C c)
  {
    return 22;
  }

  int
  second (int a, int b, C cc, int c, int d)
  {
    return 33;
  }

  int
  entry (C c)
  {
    return 44;
  }
}

struct B
{
  A::C c;
};

//------------

namespace E
{
  class O{};
  int foo (O o){return 1; }
  int foo (O o, O o2){return 2; }
  int foo (O o, O o2, int i){return 3; }
}

namespace F
{
  class O{};
  int foo (       O fo, ::E::O eo){ return 4;}
  int foo (int i, O fo, ::E::O eo){ return 5;}
}

namespace G
{
  class O{};
  int foo (O go, ::F::O fo, ::E::O eo){ return 6; }
}

//------------

namespace H
{
  class O{};
  int foo (O){ return 7;}
}

namespace I
{
  class O: public H::O {};
  class X: H::O{};
}

//------------

namespace J
{
  union U{};
  struct S{};
  enum E{};

  class A{
  public:
    class B{};
  };

  class C{};

  int foo (U){ return 8;}
  int foo (S){ return 9;}
  int foo (E){ return 10;}
  int foo (A::B){ return 11;}
  int foo (A*){ return 12;}
  int foo (A**){ return 13;}
  int foo (C[]){ return 14;}

}
//------------

namespace K{
  class O{};

  int foo(O, int){
    return 15;
  }

  int bar(O, int){
    return 15;
  }
}

int foo(K::O, float){
  return 16;
}

int bar(K::O, int){
  return 16;
}
//------------

namespace L {
  namespace A{
    namespace B{
    class O {};

    int foo (O){
      return 17;
    }

    }
  }
}

//------------

namespace M {
  class A{
  public:
    int foo(char) {
      return 18;
    }
  };

  int foo(A,char){
      return 19;
  }

  int foo(A *,char){
    return 23;
  }

  int bar(char){
    return 21;
  }

  namespace N {
    int foo(::M::A,int){
      return 20;
    }

    int bar(int){
      return 22;
    }
  }
}
//------------

namespace O {
  class A{};

  int foo(A,int){
    return 23;
  }

}

typedef O::A TOA;
typedef TOA  TTOA;

//------------

static union {
    int  a;
    char b;
}p_union;

//------------

namespace P {
  class Q{
  public:
    int operator== (int)
      {
        return 24;
      }

    int operator== (float)
      {
        return 25;
      }

    int operator+ (float)
      {
        return 26;
      }

  };

  int operator!= (Q, int)
    {
      return 27;
    }

  int operator!= (Q, double)
    {
      return 28;
    }

  int operator+ (Q, int)
    {
      return 29;
    }

  int operator++ (Q)
    {
      return 30;
    }
}

//------------

class R {
  public:
    int rfoo(){ return 31; }
    int rbar(){
      return 1; // marker1
    }
};

//------------

int
main ()
{
  A::C c;
  B b;

  A::first (c);
  first (0, c);
  second (0, 0, c, 0, 0);
  entry (c);
  A::first (b.c);

  E::O eo;
  F::O fo;
  G::O go;

  foo (eo);
  foo (eo, eo);
  foo (eo, eo, 1);
  foo (fo, eo);
  foo (1  ,fo, eo);
  foo (go, fo, eo);

  I::O io;
  I::X ix;

  foo (io);
//foo (ix);

  J::U ju;
  J::S js;
  J::E je;
  J::A::B jab;
  J::A *jap;
  J::A **japp;
  J::C jca[3];

  foo (ju);
  foo (js);
  foo (je);
  foo (jab);
  foo (jap);
  foo (japp);
  foo (jca);

  K::O ko;
  foo (ko, 1);
  foo (ko, 1.0f);
  //bar(ko,1);

  L::A::B::O labo;
  foo (labo);
  
  M::A ma;
  foo(ma,'a');
  ma.foo('a');
  M::N::foo(ma,'a');

  M::bar('a');
  M::N::bar('a');

  TTOA ttoa;
  foo (ttoa, 'a');

  p_union = {0};

  P::Q q;
  q == 5;
  q == 5.0f;
  q != 5;
  q != 5.0f;
  q + 5;
  q + 5.0f;

  ++q;

  R r;
  r.rbar();
  r.rfoo();

  return first (0, c) + foo (eo) +
         foo (eo, eo) + foo (eo, eo, 1)  +
         foo (fo, eo) + foo (1  ,fo, eo) +
         foo (go, fo, eo);
}
