// 2002-08-16

class gnu_obj_4
{
 public:
  static const int elsewhere;
  static const int nowhere;
  static const int everywhere = 317;
#if __cplusplus >= 201103L
  constexpr
#endif
  static const float somewhere = 3.14159;

  // try to ensure test4 is actually allocated
  int dummy;
};

