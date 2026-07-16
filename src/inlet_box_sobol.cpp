// step 0: sobol sample joint design space, recording unit-1 outlets.
// Per-componoent min/max +5% span inflation
// -> inlet box for unit-2 subproblem
// Outputs:
// - data/step0_sobol.csv  NOTE: Not currently used downstream!
// - data/unit2_inlet_box.csv
#include <algorithm>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <list>
#include <vector>

#include "base_sampling.hpp"
#include "ffunc.hpp"
#include "flowsheet.hpp"

int main()
{
  mc::FFGraph dag;
  Flowsheet fs(dag);
  // only considering CSTR-1 and it's DCB fraction cap
  std::vector<mc::FFVar> out = fs.out1;  // cstr-1: (cA, cB, cC)-nodes of dag
  out.push_back(fs.G[0]);

  // NOTE: sg only contains the three output concentrations of cstr-1 (cA, cB,
  // cC) and the constraint G[0]
  // NOTE: The subgraph `sg` is spawned up for efficiency purposes (similar to
  // wk)
  mc::FFSubgraph sg = dag.subgraph(out);
  std::vector<double> wk, res(out.size());  // res holds results
  const auto th = Flowsheet::theta_nominal();
  std::vector<double> dP{th[0], th[1], th[0], th[1]};
  std::vector<double> dC{0.05, 0.55, 0.70};  // C[0] needed by G[0]

  // same joint box as (src/main.cpp)
  const std::vector<double> dLB{298.15, 2.0, 298.15, 2.0};
  const std::vector<double> dUB{353.15, 30.0, 353.15, 30.0};
  std::list<std::vector<double>> samples =
      mc::BASE_SAMPLING::uniform_sample(1024, dLB, dUB);

  std::ofstream fsam("data/step0_sobol.csv");
  fsam << "T1,tau1,T2,tau2,cA1,cB1,cC1,g1\n";

  // running per componnet min/max of unit-1 outlet
  std::vector<double> uLB(3, 1e30), uUB(3, -1e30);

  for (const std::vector<double>& dD : samples)
  {
    // NOTE: wk is a caller owned scratch space that FFGraph::eval()
    // resizes on first use and uses repeatedly to avoid dynamic memory
    // allocation.
    // Var: independent variables (DAG's inputs)
    // Dep: dependents, expressions being evaluated (nodes that dpeend on Vars)
    // Prefix:
    // - v: vector<FFVar>: symbolic handles into expression graph
    // - u: vector<U>: acutal numeric vlaues of generic arithmetic type U
    // wk: workspace scratch storage for intermeditate operations during
    // traversal
    // vVar: symbolic input nodes (design/uncertain variables)
    // uVar: actual numbers for those inputs
    // vDep: symbolic output nodes (constraints g)
    // uDep: resulting numberic values of those constraitns
    // wkDep: scratch buffer for intermediate results while computing vDep
    // sgDep: cached subgraph for vDep

    /* The function parameter pack <a>args</a> can be any number of extra vector
     * pairs {std::vector<FFVar> const& vVar, std::vector<U> const& uVar}. */
    dag.eval(sg, wk, out, res, fs.D, dD, fs.P, dP, fs.C, dC);  // subgraph only
    for (size_t k = 0; k < 4; ++k) fsam << dD[k] << ",";
    fsam << res[0] << "," << res[1] << "," << res[2] << "," << res[3] << "\n";

    // update lower/upper bounds of inputs (c1A, c1B, c1C) for next unit
    for (size_t k = 0; k < 3; ++k)
    {
      uLB[k] = std::min(uLB[k], res[k]);
      uUB[k] = std::max(uUB[k], res[k]);
    }
  }

  /* Output results to file and console */

  // inflate by 5% of span on each side (total 10%); concentrations remain >= 0
  std::ofstream fbox("data/unit2_inlet_box.csv");
  fbox << "bound,cA,cB,cC\n";
  std::cout << "unit-2 inlet box from 1024 Sobol points:\n";
  fbox << "lb";
  for (size_t k = 0; k < 3; ++k)
  {
    const double span = uUB[k] - uLB[k];
    std::cout << " raw [" << uLB[k] << ", " << uUB[k] << "]";
    uLB[k] = std::max(0.0, uLB[k] - 0.05 * span);
    uUB[k] += 0.05 * span;
    std::cout << " inflated [" << uLB[k] << ", " << uUB[k] << "]\n";
    fbox << "," << uLB[k];
  }
  fbox << "\nub";
  for (size_t k = 0; k < 3; ++k) fbox << "," << uUB[k];
  fbox << "\n";

  return 0;
}
