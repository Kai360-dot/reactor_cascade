#include <vector>
#include <armadillo>

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
  std::vector<mc::FFVar> out;
  std::vector<mc::FFVar> feed{mc::FFVar(Flowsheet::CA0), mc::FFVar(0.0),
                              mc::FFVar(0.0)};

  out            = Flowsheet::cstr(feed, D[0], D[1], P[0], P[1]);
  mc::FFVar tot1 = out[0] + out[1] + out[2];
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
  const auto th = Flowsheet::theta_nominal();
  NS.set_parameter(P, {th[0], th[1]});  // nominal CSP
  NS.set_constant(C);
  NS.set_constraint(G);
  if (!NS.setup()) return 1;
  int status = NS.sample({0.05});  // constraint thresholds
  NS.stats.display();
}
