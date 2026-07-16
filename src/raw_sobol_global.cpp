/* Raw sobol sample on global (T1, tau1, T2, tau2) space
 * for benchmarking purposes only
 * */

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <ffunc.hpp>
#include <vector>

#include "base_sampling.hpp"
#include "flowsheet.hpp"

static constexpr size_t ASSUMED_SCALING_LIM = 200;
static constexpr size_t REQ_LIVE_PTS        = 8192;

int main()
{
  mc::FFGraph dag;
  Flowsheet fs(dag);

  const auto th     = Flowsheet::theta_nominal();
  auto out          = fs.G;
  mc::FFSubgraph sg = dag.subgraph(out);
  std::vector<double> wk, res(out.size());

  std::vector<double> dP{th[0], th[1], th[0], th[1]};
  std::vector<double> dC{0.05, 0.55, 0.70};

  const std::vector<double> dLB{298.15, 2.0, 298.15, 2.0};
  const std::vector<double> dUB{353.15, 30.0, 353.15, 30.0};
  std::list<std::vector<double>> samples = mc::BASE_SAMPLING::uniform_sample(
      REQ_LIVE_PTS * ASSUMED_SCALING_LIM, dLB, dUB);

  std::cout << "Done Sampling\n";

  std::ofstream fsam_feas("data/rsb_global_feas.csv");
  std::ofstream fsam_infs("data/rsb_global_infs.csv");

  assert(fsam_feas.is_open() && fsam_infs.is_open());
  // evaluate until 8192 points are found feasible.
  fsam_feas << "T1,tau1,T2,tau2,g1,g2,g3,maxvio\n";
  fsam_infs << "T1,tau1,T2,tau2,g1,g2,g3,maxvio\n";
  size_t feas_found{}, total_eval{};
  for (const auto& dD : samples)
  {
    if (feas_found >= REQ_LIVE_PTS) break;
    ++total_eval;
    dag.eval(sg, wk, out, res, fs.D, dD, fs.P, dP, fs.C, dC);

    if (res[0] <= 0 && res[1] <= 0 && res[2] <= 0)
    {  // is feasible
      ++feas_found;
      for (size_t k = 0; k < 4; ++k) fsam_feas << dD[k] << ",";
      fsam_feas << res[0] << "," << res[1] << "," << res[2] << ","
                << (std::max({res[0], res[1], res[2]})) << "\n";
    }

    else
    {  // infeasible
      for (size_t k = 0; k < 4; ++k) fsam_infs << dD[k] << ",";
      fsam_infs << res[0] << "," << res[1] << "," << res[2] << ","
                << (std::max({res[0], res[1], res[2]})) << "\n";
    }
  }
  std::cout << "Found Feasible: " << feas_found
            << " total evaluated: " << total_eval
            << " fraction: " << (static_cast<double>(feas_found) / total_eval)
            << "\n";

  return 0;
}
