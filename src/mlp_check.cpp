/* verifies MLPs using two comparsions per held out point
 * 1. C++ (T=double) forward pass vs python predicition
 * 2. FFVar DAG eval vs C++ double pass.
 */

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <ffunc.hpp>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "mlp_ffvar.hpp"

int main()
{
  MLP c1 = MLP::load("data/mlp_c1.txt");
  MLP s1 = MLP::load("data/mlp_s1.txt");
  MLP cu = MLP::load("data/mlp_cu.txt");

  mc::FFGraph dag;
  std::vector<mc::FFVar> vd = dag.add_vars(2, "d");  // (T1, tau1)
  std::vector<mc::FFVar> vu = dag.add_vars(3, "u");  // (cA, cB, cC) in cstr-1

  // records entire c1 network into dag
  std::vector<mc::FFVar> dep = c1(vd);  // dep[0]
  // same for s1, cu
  for (mc::FFVar& v : s1(vd)) dep.push_back(v);  // dep[1..3]
  for (mc::FFVar& v : cu(vu)) dep.push_back(v);  // dep[4]

  mc::FFSubgraph sg = dag.subgraph(dep);
  std::vector<double> wk, res(dep.size());

  std::ifstream f("data/mlp_predcheck.csv");
  std::string line;
  std::getline(f, line);  // header
  size_t n{}, fail{};
  while (std::getline(f, line))
  {
    // NOTE: This uses the T = double instantiation of the MLP::operator()
    // template
    std::vector<double> v;
    std::stringstream ss{line};
    for (std::string tok; std::getline(ss, tok, ',');)
      v.push_back(std::stod(tok));

    // inputs: T1, tau1, cA1, cB1, cC1,
    // predictions: c1_pred, sA, sB, sC, cu_pred
    // columns: T1, tau1, cA1, cB1, cC1, c1_pred, sA, sB, sC, cu_pred
    std::vector<double> d{v[0], v[1]};
    std::vector<double> u{v[2], v[3], v[4]};

    // python predictions:
    std::vector<double> py(v.begin() + 5, v.end());

    // cpp predicitoins:
    std::vector<double> cpp{c1(d)[0]};
    for (double s : s1(d)) cpp.push_back(s);
    cpp.push_back(cu(u)[0]);

    // NOTE: This turns the ANN into DAG using MC++ for efficient evaluation
    dag.eval(sg, wk, dep, res, vd, d, vu, u);
    for (size_t k{}; k < 5; ++k)
    {
      if (std::fabs(cpp[k] - py[k]) > 1e-9 || std::fabs(res[k] - cpp[k]) > 1e-9)
      {
        std::cout << "MISMATCH row " << n << " comp " << k << ": py " << py[k]
                  << " cpp" << cpp[k] << " dag " << res[k] << "\n";
        ++fail;
      }
    }
    ++n;
  }
  std::cout << n << " rows checked, " << fail << " mismatches -> "
            << (fail ? "FAIL :(" : "PASS :)") << "\n";
  return fail ? 1 : 0;
}
