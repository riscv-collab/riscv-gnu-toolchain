class A
{
public:
  A() : member_ (0) {};
  int member_;
};
class B : public A {};

typedef A TA1;
typedef A TA2;
typedef TA2 TA3;

enum my_enum {MY_A, MY_B, MY_C, MY_D};

/* Without this variable older 'enum my_enum' incl. its 'MY_A' would be omitted
   by older versions of GCC (~4.1) failing the testcase using it below.  */
enum my_enum my_enum_var;

int foo0_1 (TA1)  { return 1; }
int foo0_2 (TA3)  { return 2; }
int foo0_3 (A***) { return 3; }

int foo1_1 (char *) {return 11;}
int foo1_2 (char[]) {return 12;}
int foo1_3 (int*)   {return 13;}
int foo1_4 (A*)     {return 14;}
int foo1_5 (void*)  {return 15;}
int foo1_6 (void**) {return 16;}
int foo1_7 (bool)   {return 17;}
int foo1_8 (long)   {return 18;}

int foo2_1 (char**  )  {return 21;}
int foo2_2 (char[][1]) {return 22;}
int foo2_3 (char *[])  {return 23;}
int foo2_4 (int  *[])  {return 24;}

int foo3_1 (int a, const char **b) { return 31; }
int foo3_2 (int a, int b) { return 32; }
int foo3_2 (int a, const char **b) { return 320; }

int foo1_type_check (char *a) { return 1000; }
int foo2_type_check (char *a, char *b) { return 1001; }
int foo3_type_check (char *a, char *b, char *c) { return 1002; }

int main()
{

  TA2 ta;      // typedef to..
  foo0_1 (ta); // ..another typedef
  foo0_2 (ta); // ..typedef of a typedef

  B*** bppp;    // Pointer-to-pointer-to-pointer-to-derived..
//foo0_3(bppp); // Pointer-to-pointer-to-pointer base.
  foo0_3((A***)bppp); // to ensure that the function is emitted.

  char av = 'a';
  char *a = &av;       // pointer to..
  B *bp = new B();
  foo1_1 (a);          // ..pointer
  foo1_2 (a);          // ..array
  foo1_3 ((int*)a);    // ..pointer of wrong type
  foo1_3 ((int*)bp);   // ..pointer of wrong type
  foo1_4 (bp);         // ..ancestor pointer
  foo1_5 (bp);         // ..void pointer
  foo1_6 ((void**)bp); // ..void pointer pointer
  foo1_7 (bp);         // ..boolean
  foo1_8 ((long)bp);   // ..long int

  char **b;          // pointer pointer to..
  char ba[1][1];
  foo1_5 (b);        // ..void pointer
  foo2_1 (b);        // ..pointer pointer
  foo2_2 (ba);       // ..array of arrays
  foo2_3 (b);        // ..array of pointers
  foo2_4 ((int**)b); // ..array of wrong pointers

  // X to boolean conversions allowed by the standard
  int integer = 0;
  long long_int = 1;
  float fp = 1.0;
  double dp = 1.0;
  foo1_7 (integer);		// integer to boolean
  foo1_7 (long_int);		// long to boolean
  foo1_7 (*a);			// char to boolean
  foo1_7 (MY_A);		// unscoped enum to boolean
  /* converts.exp tests the next statement directly.  It is not compiled
     here for verification because older versions of GCC (~4.1) fail to
     compile it:

     warning: the address of 'int foo1_7(bool)' will always evaluate as true
     
  foo1_7 (&foo1_7);		// pointer to boolean
  */
     
  foo1_7 (&A::member_);		// pointer to member to boolean
  foo1_7 (a);			// pointer to boolean
  foo1_7 (fp);			// float to boolean
  foo1_7 (dp);			// double  to boolean

  foo3_1 (0, 0);
  foo3_2 (0, static_cast<char const**> (0));
  foo3_2 (0, 0);

  foo1_type_check (a);
  foo2_type_check (a, a);
  foo3_type_check (a, a, a);

  return 0;          // end of main
}
