namespace A
{
  int ax = 9;
}

namespace B
{
  using namespace A;
}

namespace C
{
  using namespace B;
}

using namespace C;

//---------------
namespace D
{
  using namespace D;
  int dx = 99;
}
using namespace D;

//---------------
namespace
{
  namespace
  {
    int xx = 999;
  }
}

//---------------
namespace E
{
  int ex = 9999;
}

namespace F
{
  namespace FE = E;
}

namespace G
{
  namespace GF = F;
}

//----------------
int main ()
{
  using namespace D;
  namespace GX = G;
  return ax + dx + xx + G::GF::FE::ex;
}
