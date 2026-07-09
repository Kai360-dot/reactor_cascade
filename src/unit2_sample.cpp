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
#include <fstream>
#include <iostream>
#include <sstream>
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
  std::vector<double> inp_lb, inp_ub;
  load_box(inp_lb, inp_ub);
}
