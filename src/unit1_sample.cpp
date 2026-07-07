#include <vector>
#include "ffunc.hpp"
#include "flowsheet.hpp"

// NOTE: we include flowsheet.hpp but only to use its static member
// cstr() function not to create a Flowsheet object

int main()
{
  mc::FFGraph dag;
  std::vector<mc::FFVar> d1 = dag.add_vars(2, "d"); // T1, tau1
  std::vector<mc::FFVar> p1 = dag.add_vars(2, "p"); // th1, th2
  std::vector<mc::FFVar> c1 = dag.add_vars(1, "c"); // DCB fraction cap
  std::vector<mc::FFVar> out;
  std::vector<mc::FFVar> feed{mc::FFVar(Flowsheet::CA0), mc::FFVar(0.0), mc::FFVar(0.0)};

  out = Flowsheet::cstr(feed, d1[0], d1[1], p1[0], p1[1]);
  mc::FFVar tot1 = out[0] + out[1] + out[2];
  std::vector<mc::FFVar> G1{out[2] / tot1 - c1[0]};

}
