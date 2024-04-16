class empty { };

class A1 {
public:
  int x;
  int y;
};

class A2 {
public:
  int x;
  int y;
};

class A3 {
public:
  int x;
  int y;
};

#if !defined (__GNUC__) || __GNUC__ > 7
# define NO_UNIQUE_ADDRESS [[no_unique_address]]
#else
# define NO_UNIQUE_ADDRESS
#endif

class A4 {
public:
    NO_UNIQUE_ADDRESS empty x;
};

class X : public A1, public A2 {
public:
  int z;
};

class L : public A1 {
public:
  int z;
};

class LV : public virtual A1 {
public:
  int z;
};

class M : public A2 {
public:
  int w;
};

class N : public L, public M {
public:
  int r;
};

class K : public A1 {
public:
  int i;
};

class KV : public virtual A1 {
public:
  int i;
};

class J : public K, public L {
public:
  int j;
};

class JV : public KV, public LV {
public:
  int jv;
};

class JVA1 : public KV, public LV, public A1 {
public:
  int jva1;
};

class JVA2 : public KV, public LV, public A2 {
public:
  int jva2;
};

class JVA1V : public KV, public LV, public virtual A1 {
public:
  int jva1v;
};

class JE : public A1, public A4 {
public:
};

int main()
{
  A1 a1;
  A2 a2;
  A3 a3;
  X x;
  L l;
  M m;
  N n;
  K k;
  J j;
  JV jv;
  JVA1 jva1;
  JVA2 jva2;
  JVA1V jva1v;
  JE je;
  
  int i;

  i += k.i + m.w + a1.x + a2.x + a3.x + x.z + l.z + n.r + j.j;

  /* Initialize all the fields.  Keep the order the same as in the
     .exp file.  */

  a1.x = 1;
  a1.y = 2;

  a2.x = 1;
  a2.y = 2;

  a3.x = 1;
  a3.y = 2;

  x.A1::x = 1;
  x.A1::y = 2;
  x.A2::x = 3;
  x.A2::y = 4;
  x.z = 5;

  l.x = 1;
  l.y = 2;
  l.z = 3;

  m.x = 1;
  m.y = 2;
  m.w = 3;

  n.A1::x = 1;
  n.A1::y = 2;
  n.A2::x = 3;
  n.A2::y = 4;
  n.w = 5;
  n.r = 6;
  n.z = 7;

  k.x = 1;
  k.y = 2;
  k.i = 3;

  j.K::x = 1;
  j.K::y = 2;
  j.L::x = 3;
  j.L::y = 4;
  j.i = 5;
  j.z = 6;
  j.j = 7;

  jv.x = 1;
  jv.y = 2;
  jv.i = 3;
  jv.z = 4;
  jv.jv = 5;

  jva1.KV::x = 1;
  jva1.KV::y = 2;
  jva1.LV::x = 3;
  jva1.LV::y = 4;
  jva1.z = 5;
  jva1.i = 6;
  jva1.jva1 = 7;

  jva2.KV::x = 1;
  jva2.KV::y = 2;
  jva2.LV::x = 3;
  jva2.LV::y = 4;
  jva2.A2::x = 5;
  jva2.A2::y = 6;
  jva2.z = 7;
  jva2.i = 8;
  jva2.jva2 = 9;

  jva1v.x = 1;
  jva1v.y = 2;
  jva1v.z = 3;
  jva1v.i = 4;
  jva1v.jva1v = 5;

  je.A1::x = 1;

  return 0; /* set breakpoint here */
}
