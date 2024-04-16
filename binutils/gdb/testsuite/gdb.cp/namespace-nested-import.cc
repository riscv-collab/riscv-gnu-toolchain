namespace A{
  namespace B{
    namespace C{
      int       x = 5;
    }
  }
}

int main(){
  using namespace A::B;
  return C::x;
}
