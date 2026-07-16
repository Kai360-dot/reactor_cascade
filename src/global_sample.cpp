#include <armadillo>
#include <cstddef>
#include <ffunc.hpp>
#include <fstream>

#include "flowsheet.hpp"
#include "nsfeas.hpp"

int main()
{
  arma::arma_rng::set_seed(42);
  mc::FFGraph dag;
  // creates a DAG of operations: inputs (D, P, C) at bottom,
  // and outputs of (out1, out2, G) at top, with operations linking them.
  Flowsheet fs(dag);

  mc::NSFEAS NS;
  NS.options.FEASCRIT  = mc::NSFEAS::Options::VAR;
  NS.options.NUMLIVE   = 8192;
  NS.options.NUMPROP   = 256;
  NS.options.MAXITER   = 20'000;
  NS.options.MAXTHREAD = 0;
  NS.options.DISPLEVEL = 1;

  NS.set_dag(dag);
  NS.set_control(fs.D,                         // nodes in graph to set values
                 {298.15, 2.0, 298.15, 2.0},   // lower bounds
                 {353.15, 30.0, 353.15, 30.0}  // upper bounds
  );
  const auto th = Flowsheet::theta_nominal();
  NS.set_parameter(fs.P, {th[0], th[1], th[0], th[1]});  // nominal CSP
  NS.set_constant(fs.C);
  NS.set_constraint(fs.G);

  if (!NS.setup()) return 1;
  // run nested sampling
  int status = NS.sample({0.05, 0.55, 0.70});  // constraint thresholds
  // NOTE: this prints a table of progress:
  // Iterations vs Contour, #Feas, #Dead, Factor
  // - Iterations: Each iteration a new ellipsoidal nest is refit around live
  // set and NUMPROP cnadidates are proposed inside of it.
  // - Contour: criterion value of "worst" live poitn.
  // (_liveFEAS.rbegin()->first) has to drop <= 0 for entire live set to be
  // feasible.
  // - #Feas: cumulative count of feasible points thus far.
  // - #Dead: number of dead poitns thus far
  // - Factor: current inflation applied to live-set ellipsoid when proposing
  // (ELLMAG * nestMass^ELLRED) shrinks with iterations

  NS.stats.display();
  // refer to: _liveFEAS.insert in MANGUS NSFEAS
  auto dump = [](const auto& pts, const std::string& name) {
    std::ofstream f(name);
    f << "crit,feasprob,T1,tau1,T2,tau2\n";
    for (const auto& [crit, pt] : pts)
    {
      f << crit << "," << std::get<1>(pt);
      for (size_t i = 0; i < 4; ++i)
      {
        f << "," << std::get<0>(pt)[i];
      };
      f << "\n";
    }
  };

  dump(NS.live_points(), "data/global_live_points.csv");  // DS sample
  dump(NS.dead_points(), "data/global_dead_points.csv");  // killed during run
  dump(NS.discard_points(),
       "data/global_discard_points.csv");  // rejected proposals

  return status;
}
