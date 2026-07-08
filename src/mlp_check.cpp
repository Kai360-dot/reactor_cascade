/* verifies MLPs using two comparsions per held out point
 * 1. C++ double forward pass vs python predicition
 * 2. FFVar DAG eval vs C++ double pass.
 */

#include <cmath>
#include <cstdio>
#include <ffunc.hpp>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "mlp_ffvar.hpp"

int main()
{
  MLP c1 = MLP::load("mlp_c1.txt");
  MLP s1 = MLP::load("mlp_s1.txt");
  MLP cu = MLP::load("mlp_cu.txt");

  mc::FFGraph dag;
  std::vector<mc::FFVar> vd  = dag.add_vars(2, "d");  // (T1, tau1)
  std::vector<mc::FFVar> vu  = dag.add_vars(3, "u");  // (cA, cB, cC) in cstr-1

  // records entire c1 network into dag
  std::vector<mc::FFVar> dep = c1(vd);                // dep[0]
  // same for s1, cu
  for (mc::FFVar& v : s1(vd)) dep.push_back(v);       // dep[1..3]
  for (mc::FFVar& v : cu(vu)) dep.push_back(v);       // dep[4]

  mc::FFSubgraph sg = dag.subgraph(dep);
  std::vector<double> wk, res(dep.size());
}
