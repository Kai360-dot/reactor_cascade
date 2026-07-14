/* This simulation simulates sampled pairs that have both been suggested by
 * unit1_live.csv and unit2_live_points.csv. In a first step a (T1, tau1) pair
 * from unit1_live.csv is selected, (Note that its simulation results are
 * already known with full certainty as no surrogates were involved here.)
 * Now a set of other configurations (T2, tau2) from unit2_live_points.csv is
 * required to fully specify the simulation. However random sampling is unlikely
 * to satisfy the feasibility constraints. Thus we choose only configurations
 * that have a `sampled input` which is relatively (+- 5% relative to
 * unit2_inlet_box.csv's span.) close to the outputs of the specific cstr-1
 * configuration. If no such configurations can be found the chosen
 * configuration from cstr-1 will be skipped.
 * */

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <ffunc.hpp>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "flowsheet.hpp"
#include "loaders.hpp"

static constexpr double RTOL = 0.05;
// Number of Subsamples, as 8192 are more than is needed
static constexpr size_t NSUB = 2000;
// limit on number of points selected from cstr-2 per cstr-1 point
static constexpr size_t MMAX = 5;

[[nodiscard]]
static std::vector<std::vector<double>> load_csv(const std::string& name)
{
  std::ifstream ifs(name);
  if (!ifs.is_open()) throw std::runtime_error("can't open input file" + name);
  std::string line, word;
  std::getline(ifs, line);  // discard header

  using Point = std::vector<double>;
  Point pt_;
  pt_.reserve(7);  // # columns
  std::vector<Point> points_;
  points_.reserve(8'200);  // ~ # points

  while (std::getline(ifs, line))
  {
    std::stringstream ss{line};
    while (std::getline(ss, word, ','))
    {
      pt_.push_back(std::stod(word));
    }
    points_.push_back(pt_);
    pt_.clear();
  }
  return points_;
};

int main()
{
  // crit,feasprob,T1,tau1,cA1,cB1,cC1
  auto live1 = load_csv("data/unit1_live.csv");
  // crit,feasprob,T2,tau2,c1A,c1B,c1C
  auto live2 = load_csv("data/unit2_live.csv");

  // load inflated bounds on input from initial global sampling file
  std::vector<double> uLB, uUB;
  load_box(uLB, uUB);
  std::vector<double> span(3);
  for (size_t k{}; k < 3; ++k) span[k] = uUB.at(k) - uLB.at(k);

  mc::FFGraph dag;
  Flowsheet fs(dag);
  auto sg       = dag.subgraph(fs.G);
  const auto th = Flowsheet::theta_nominal();

  std::vector<double> wk, g(3), dP{th[0], th[1], th[0], th[1]},
      dC{0.05, 0.55, 0.70};

  std::ofstream ofs("data/recon_joint.csv");
  ofs << "T1,tau1,T2,tau2,g1,g2,g3,feas\n";

  assert(live1.size() >= NSUB && "NSUB must be a subset of all points");
  size_t stride = live1.size() > NSUB ? live1.size() / NSUB : 1;
  size_t cand{}, truefeas{}, unmatched{}, tried{};

  for (size_t i{}; i < live1.size(); i += stride)
  {
    std::vector<double> r1 = live1[i];
    size_t m               = 0;  // count matches on cstr-2 set
    ++tried;
    for (const auto& r2 : live2)
    {
      double dist = 0.0;
      for (size_t k = 0; k < 3; ++k)
      {
        dist = std::max(dist, std::fabs(r2[4 + k] - r1[4 + k]) / span[k]);
      }
      if (dist > RTOL) continue;  // point doesn't qualify
      std::vector<double> dD{r1[2], r1[3], r2[2], r2[3]};  // T1, tau1, T2, tau2
      dag.eval(sg, wk, fs.G, g, fs.D, dD, fs.P, dP, fs.C, dC);
      bool feas = g[0] <= 0 && g[1] <= 0 && g[2] <= 0;
      ofs << dD[0] << "," << dD[1] << "," << dD[2] << "," << dD[3] << ","
          << g[0] << "," << g[1] << "," << g[2] << "," << static_cast<int>(feas)
          << "\n";
      ++cand;
      truefeas += static_cast<int>(feas);
      if (++m >= MMAX) break;
    }
    if (m == 0) ++unmatched;
  }
  if (cand)
  {
    auto false_feas_rate = 100.0 * (cand - truefeas) / cand;
    std::cout << "reconstruction: " << cand << " pairs " << truefeas
              << " true-feasible " << false_feas_rate << " false-feasible rate "
              << unmatched << "/" << tried << " unmatched.\n";
  }

  return 0;
}
