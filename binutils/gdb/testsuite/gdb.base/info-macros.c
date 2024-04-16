#ifdef DEF_MACROS

  #ifdef ONE
    #ifdef FOO
    #undef FOO
    #endif
    #define FOO "hello"
  #else
    #undef FOO
  #endif


  #ifdef TWO
    #ifdef FOO
    #undef FOO
    #endif
    #define FOO " "
  #endif

  #ifdef THREE
    #ifdef FOO
    #undef FOO
    #endif
    #define FOO "world"
  #endif

  #ifdef FOUR
    #ifdef FOO
    #undef FOO
    #endif
    #define FOO(a) foo = a
  #endif
#else

int main (int argc, const char **argv)
{
  char *foo;

  #define DEF_MACROS
  #define ONE
  #include "info-macros.c"
  foo = FOO;

  #define TWO
  #include "info-macros.c"
  foo = FOO;

  #define THREE
  #include "info-macros.c"
  foo = FOO;

  #undef THREE
  #include "info-macros.c"
  foo = FOO;

  #undef TWO
  #include "info-macros.c"
  foo = FOO;

  #undef ONE
  #include "info-macros.c"
  foo = (char *)0;

  #define FOUR
  #include "info-macros.c"
  FOO ("the end.");

  return 0;
}
#endif

