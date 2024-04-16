#include <cstdlib>

/* Make sure `malloc' is linked into the program.  If we don't, tests
   in the accompanying expect file may fail:

   evaluation of this expression requires the program to have a function
   "malloc".  */

void
dummy ()
{
  void *p = malloc (16);

  free (p);
}

/* 1. A standard conversion sequence is better than a user-defined sequence
      which is better than an elipses conversion sequence.  */

class A{};
class B: public A {public: operator int (){ return 1;}};

// standard vs user-defined
int foo0  (int) { return 10; }
int foo1  (int) { return 11; } // B -> int   : user defined
int foo1  (A)   { return 12; } // B -> A     : standard
int test1 () {
  B b;
  return foo1(b); // 12
}

// user-defined vs ellipsis
int foo2 (int) { return 13;} // B -> int   : user defined
int foo2 (...) { return 14;} // B -> ...   : ellipsis
int test2(){
  B b;
  return foo2(b); // 13
}

/* 2. Standard Conversion sequence S1 is better than standard Conversion
      S2 if:  */

//      - S1 has a better rank than S2
//        see overload.exp for more comprehensive testing of this.
int foo3 (double) { return 21; } // float->double is 'promotion rank'
int foo3 (int)    { return 22; } // float->int is 'conversion rank'
int test3(){
  return foo3 (1.0f); // 21
}

//      - S1 and S2 are both 'qualification conversions' but S1 cv-qualification
//        is a subset of S2 cv-qualification.
int foo4 (const volatile int*) { return 23; }
int foo4 (      volatile int*) { return 24; }
int test4 () {
  volatile int a = 5;
  return foo4(&a); // 24
}

//      - S1 and S2 have the same rank but:
//        - S2 is a conversion of pointer or member-pointer to bool
int foo5 (bool)  { return 25; }
int foo5 (void*) { return 26; }
int test5 () {
  char *a;
  return foo5(a); // 26
}

//        - Class B publicly extends class A and S1 is a conversion of
//          B* to A* and S2 is a conversion B* to void*
int foo6 (void*) { return 27; }
int foo6 (A*)    { return 28; }
int test6 () {
  B *bp;
  return foo6(bp); // 28
}

//        - Class C publicly extends Class B which publicly extends
//          class A and S1 is a conversion of C* to B* and S2 is a
//          conversion C* to A*.
class C: public B {};
int foo7 (A*)    { return 29; }
int foo7 (B*) { return 210; }
int test7 () {
  C *cp;
  return foo7(cp); // 210
}

//        - Same as above but for references.
int foo8 (A&) { return 211; }
int foo8 (B&) { return 212; }
int test8 () {
  C c;
  return foo8(c); // 212
}

//        - Same as above but passing by copy.
int foo9 (A) { return 213; }
int foo9 (B) { return 214; }
int test9 () {
  C c;
  return foo9(c); // 212
}

//        - S1 is a conversion of A::* to B::* and S2 is a conversion of
//          A::* to C::8.
int foo10 (void (C::*)()) { return 215; }
int foo10 (void (B::*)()) { return 216; }
int test10 () {
  void (A::*amp)();
  return foo10(amp); // 216
}

//      - S1 is a subsequence of S2
int foo101 (volatile const char*) { return 217; } // array-to-pointer conversion
                                                 // plus qualification conversion
int foo101 (         const char*) { return 218; } // array-to-pointer conversion

int test101 () {
  return foo101("abc"); // 216
}

/* 3. User defined conversion U1 is better than user defined Conversion U2,
      if U1 and U2 are using the same conversion function but U1 has a better
      second standard conversion sequence than U2.  */
class D {public: operator short(){ return 0;}};
int foo11 (float) { return 31; }
int foo11 (int)   { return 32; }
int test11 () {
  D d;
  return foo11(d); // 32
}

/* 4. Function Level Ranking.
      All else being equal some functions are preferred by overload resolution.
      Function F1 is better than function F2 if: */
//    - F1 is a non-template function and F2 is a template function
template<class T> int foo12(T)   { return 41; }
                  int foo12(int) { return 42; }
int test12 (){
  return foo12(1); //42
}

//    - F1 is a more specialized template instance
template<class T> int foo13(T)  { return 43; }
template<class T> int foo13(T*) { return 44; }
int test13 (){
  char *c;
  return foo13(c); // 44
}

//    - The context is user defined conversion and F1 has
//      a better return type than F2
class E {
  public:
    operator double () {return 45; }
    operator int    () {return 46; }
};
int foo14 (int a) {return a;}
int test14 (){
  E e;
  return foo14(e); // 46
}

/* Test cv qualifier overloads.  */
int foo15 (char *arg) { return 47; }
int foo15 (const char *arg) { return 48; }
int foo15 (volatile char *arg) { return 49; }
int foo15 (const volatile char *arg) { return 50; }
static int
test15 ()
{
  char *c = 0;
  const char *cc = 0;
  volatile char *vc = 0;
  const volatile char *cvc = 0;

  // 47 + 48 + 49 + 50 = 194
  return foo15 (c) + foo15 (cc) + foo15 (vc)  + foo15 (cvc);
}

int main() {
  dummy ();

  B b;
  foo0(b);
  foo1(b);
  test1();

  foo2(b);
  test2();

  foo3(1.0f);
  test3();

  volatile int a;
  foo4(&a);
  test4();

  char *c;
  foo5(c);
  test5();

  B *bp;
  foo6(bp);
  test6();

  C *cp;
  foo7(cp);
  test7();

  C co;
  foo8(co);
  test8();

  foo9(co);
  test9();

  void (A::*amp)();
  foo10(amp);
  test10();

  foo101("abc");
  test101();

  D d;
  foo11(d);
  test11();

  foo12(1);
  test12();

  foo13(c);
  test13();

  E e;
  foo14(e);
  test14();

  const char *cc = 0;
  volatile char *vc = 0;
  const volatile char *cvc = 0;
  test15 ();

  return 0; // end of main
}
