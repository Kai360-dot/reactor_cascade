/*  Solves cstr-2 subproblem by sampling the feed of cstr-2 that arrives
 *  from upstream unit cstr-1. We thus sample over (T2, tau2, c1A, c1B, c1C)
 *  subject to three constraints: g2, g3 (product purity and conversion floor
 * from  global problem) and rho: (reachability, assessed via cu ANN)
 * outputs: (all with [crit,feasprob,T2,tau2,cA,cB,cC] header line)
 * - unit2_live.csv (final design-space sample)
 * - unit2_dead.csv
 * - unit2_discard.csv
 */

#include <armadillo>
#include <cstddef>
#include <ffunc.hpp>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "flowsheet.hpp"
#include "mlp_ffvar.hpp"
#include "nsfeas.hpp"

static void load_box(std::vector<double>& lb, std::vector<double>& ub)
{
  // load u2 bounds box
  std::ifstream ifs("data/unit2_inlet_box.csv");
  if (!ifs) throw std::runtime_error("error loading file");
  std::string line;
  std::getline(ifs, line);  // skip header
  std::string word_;

  auto read_ = [&ifs, &word_](std::vector<double>& vec) {
    std::getline(ifs, word_, ',');  // skip first
    for (size_t i{}; i < 3; ++i)
    {
      (i == 2) ? std::getline(ifs, word_) : std::getline(ifs, word_, ',');
      vec.push_back(std::stod(word_));
    }
  };

  read_(lb);
  read_(ub);
  // lb, ub
  for (auto e : lb) std::cout << e << "\n";
  for (auto e : ub) std::cout << e << "\n";
}

int main()
{
  arma::arma_rng::set_seed(42);
  // load inflated bounds on input from initial global sampling file
  std::vector<double> uLB, uUB;
  load_box(uLB, uUB);

  mc::FFGraph dag;
  auto D2 = dag.add_vars(2, "d");  // T2, tau2
  auto U2 = dag.add_vars(3, "u");  // cA, cB, cC (inlet)
  auto P2 = dag.add_vars(2, "p");  // theta1, theta2 (Arrhenius)
  // constraint values (cu constraint is implicit, purity and conversion floor)
  auto C2 = dag.add_vars(2, "c");

  // reactor setup
  auto out2 = Flowsheet::cstr(U2, D2[0], D2[1], P2[0], P2[1]);
  auto tot2 = out2[0] + out2[1] + out2[2];

  std::vector<mc::FFVar> G2{
      C2[0] - out2[1] / tot2,                 // MCB purity floor cstr-2
      C2[1] - (1 - out2[0] / Flowsheet::CA0)  // conversion floor benzene cstr-2
  };
  // third constraint from ANN
  MLP cu = MLP::load("data/mlp_cu.txt");
  G2.push_back(cu(U2)[0]);

  mc::NSFEAS NS;
  NS.set_dag(dag);
  NS.options.FEASCRIT  = mc::NSFEAS::Options::VAR;
  NS.options.NUMLIVE   = 8192;
  NS.options.NUMPROP   = 256;
  NS.options.MAXITER   = 20'000;
  NS.options.MAXTHREAD = 0;
  NS.options.DISPLEVEL = 1;

  std::vector<mc::FFVar> ctrl_v{D2};
  ctrl_v.insert(ctrl_v.cend(), U2.cbegin(), U2.cend());

  std::vector<double> ctrl_v_lb{298.15, 2.0};
  ctrl_v_lb.insert(ctrl_v_lb.cend(), uLB.cbegin(), uLB.cend());

  std::vector<double> ctrl_v_ub{353.15, 30.0};
  ctrl_v_ub.insert(ctrl_v_ub.cend(), uUB.cbegin(), uUB.cend());

  NS.set_control(ctrl_v, ctrl_v_lb,  // lower bounds
                 ctrl_v_ub);         // upper bounds
  auto th = Flowsheet::theta_nominal();
  NS.set_parameter(P2, {th[0], th[1]});
  NS.set_constant(C2);
  NS.set_constraint(G2);

  // run nested sampling
  if (!NS.setup()) return 1;

  // NOTE: only last two values are used as compared to src/main.cpp
  int status = NS.sample({0.55, 0.70});
  NS.stats.display();

  auto dump = [](const auto& pts, const std::string& name) {
    std::ofstream f(name);
    f << "crit,feasprob,T2,tau2,c1A,c1B,c1C\n";
    for (const auto& [crit, pt] : pts)
    {
      f << crit << "," << std::get<1>(pt);
      for (size_t i{}; i < 5; ++i)
      {
        f << "," << std::get<0>(pt)[i];
      };
      f << "\n";
    }
  };

  dump(NS.live_points(), "data/unit2_live_points.csv");  // DS sample
  dump(NS.dead_points(), "data/unit2_dead_points.csv");  // killed during run
  dump(NS.discard_points(), "data/unit2_discard_points.csv");  // rejected
  // proposals

  return status;
}
