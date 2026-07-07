#include <armadillo>
#include <array>
#include <cstddef>
#include <fstream>
#include <vector>

#include "ffunc.hpp"
#include "flowsheet.hpp"
#include "nsfeas.hpp"

// NOTE: we include flowsheet.hpp but only to use its static member
// cstr() function not to create a Flowsheet object

int main()
{
  arma::arma_rng::set_seed(42);
  mc::FFGraph dag;
  std::vector<mc::FFVar> D = dag.add_vars(2, "d");  // T1, tau1
  std::vector<mc::FFVar> P = dag.add_vars(2, "p");  // th1, th2
  std::vector<mc::FFVar> C = dag.add_vars(1, "c");  // DCB fraction cap
  std::vector<mc::FFVar> feed{mc::FFVar(Flowsheet::CA0), mc::FFVar(0.0),
                              mc::FFVar(0.0)};

  std::vector<mc::FFVar> out = Flowsheet::cstr(feed, D[0], D[1], P[0], P[1]);
  mc::FFVar tot1             = out[0] + out[1] + out[2];
  std::vector<mc::FFVar> G{out[2] / tot1 - C[0]};

  mc::NSFEAS NS;
  NS.options.FEASCRIT  = mc::NSFEAS::Options::VAR;
  NS.options.NUMLIVE   = 8192;
  NS.options.NUMPROP   = 256;
  NS.options.MAXITER   = 20'000;
  NS.options.MAXTHREAD = 0;
  NS.options.DISPLEVEL = 1;

  NS.set_dag(dag);
  // set lower and upper bounds
  NS.set_control(D, {298.15, 2.0}, {353.15, 30.0});
  const std::array<double, 2> th = Flowsheet::theta_nominal();
  NS.set_parameter(P, {th[0], th[1]});  // nominal CSP
  NS.set_constant(C);
  NS.set_constraint(G);
  if (!NS.setup()) return 1;

  // run simulation:
  int status = NS.sample({0.05});  // constraint thresholds

  NS.stats.display();

  mc::FFSubgraph sg = dag.subgraph(out);
  std::vector<double> wk, res(3), dP{th[0], th[1]};

  auto dump = [&dag, &sg, &wk, &res, &out, &D, &P, &dP](
                  const auto& pts, const std::string& name) {
    std::ofstream f(name);
    f << "crit,feasprob,T1,tau1,cA1,cB1,cC1\n";
    for (const auto& [crit, pt] : pts)
    {
      // NOTE: dD stores the numerical coordinates of the design config
      // already contained in pt.
      std::vector<double> dD{std::get<0>(pt)[0], std::get<0>(pt)[1]};
      // NOTE: below call takes the subgraph to evaluate in. wk is simply
      // provided as a scratchpad for internal use. out (vDep) are the nodes
      // that correspond to the results that we want to read ffrom the dag.
      // res (uDep) is the vector that these results will be read into.
      // D(vVar) are the nodes into which the values provided through dD(uVar)
      // will be read into. P are the nodes for the parameter and dP their
      // concrete numerical values. More such pairs can be provided if needed.
      dag.eval(sg, wk, out, res, D, dD, P, dP);
      f << crit << "," << std::get<1>(pt);
      for (size_t i{}; i < 2; ++i) f << "," << dD[i];
      for (double v : res) f << "," << v;
      f << "\n";
    }
  };

  dump(NS.live_points(), "unit1_live.csv");        // DS sample
  dump(NS.dead_points(), "unit1_dead.csv");        // killed during run
  dump(NS.discard_points(), "unit1_discard.csv");  // rejected proposals

  return status;
}
