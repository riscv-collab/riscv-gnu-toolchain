class vec2 
{
  public:
    vec2() { _v[0] = _v[1] = 0; }
    vec2(int x, int y) { _v[0] = x; _v[1] = y; }
    static vec2 axis[2];
    static vec2 axis6[6];
  private:
    int _v[2];
};

vec2 vec2::axis[2] = { vec2(1,0), vec2(0,1) };
vec2 vec2::axis6[6] = { 
  vec2(1,0), vec2(0,1),
  vec2(2,0), vec2(0,2),
  vec2(3,0), vec2(0,3) 
};

int main(int argc, char*argv[])
{
  vec2 a;

  return 0;  // marker
}
