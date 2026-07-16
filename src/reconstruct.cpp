#include <algorithm>
#include <cmath>
#include <cstddef>
#include <ffunc.hpp>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "flowsheet.hpp"
#include "loaders.hpp"
#include "mlp_ffvar.hpp"

static constexpr double RTOL = 0.05;
// Number of Subsamples, as 8192 are more than is needed
static constexpr size_t NSUB = 2000;
// limit on number of points selected from cstr-2 per cstr-1 point
static constexpr size_t MMAX = 5;

static void run_reconstruction();
static void validate_global_agreement();

int main()
{
  run_reconstruction();
  validate_global_agreement();
  return 0;
}

static void run_reconstruction()
{
  // NOTE: This simulation simulates sampled pairs that have both been suggested
  // by unit1_live.csv and unit2_live.csv. In a first step a (T1, tau1) pair
  // from unit1_live.csv is selected, (Note that its simulation results are
  // already known with full certainty as no surrogates were involved here.)
  // Now a set of other configurations (T2, tau2) from unit2_live.csv is
  // required to fully specify the simulation. However random sampling is
  // unlikely to satisfy the feasibility constraints. Thus we choose only
  // configurations that have a `sampled input` which is relatively (+- 5%
  // relative to unit2_inlet_box.csv's span.) close to the outputs of the
  // specific cstr-1 configuration. If no such configurations can be found the
  // chosen configuration from cstr-1 will be skipped.
  // Every joint pair is then simulated on the entire flowsheet.
  // A true-feasible rate (fraction of joint configurations actually
  // feasible) is computed based thereon.

  // crit,feasprob,T1,tau1,cA1,cB1,cC1
  auto live1 = load_csv("data/unit1_live.csv");
  // crit,feasprob,T2,tau2,c1A,c1B,c1C
  auto live2 = load_csv("data/unit2_live.csv");

  // load inflated bounds on input from initial sobol sampling
  std::vector<double> uLB, uUB;
  load_box(uLB, uUB);
  std::vector<double> span(3);
  for (size_t k = 0; k < 3; ++k) span[k] = uUB.at(k) - uLB.at(k);

  mc::FFGraph dag;
  Flowsheet fs(dag);
  auto sg       = dag.subgraph(fs.G);
  const auto th = Flowsheet::theta_nominal();

  std::vector<double> wk, g(3), dP{th[0], th[1], th[0], th[1]},
      dC{0.05, 0.55, 0.70};

  std::ofstream ofs("data/recon_joint.csv");
  ofs << "T1,tau1,T2,tau2,g1,g2,g3,feas\n";

  size_t stride = live1.size() > NSUB ? live1.size() / NSUB : 1;
  size_t cand{}, truefeas{}, unmatched{}, tried{};

  for (size_t i = 0; i < live1.size(); i += stride)
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

  // metric for true/false feasibility
  if (cand)
  {
    auto false_feas_rate = 100.0 * (cand - truefeas) / cand;
    std::cout << "reconstruction: " << cand << " pairs " << truefeas
              << " true-feasible " << false_feas_rate << " false-feasible rate "
              << unmatched << "/" << tried << " unmatched.\n";
  }
}

static void validate_global_agreement()
{  // metric for pipelines coverage applied to points from global NSampling
   // src/global_sample.cpp

  // columns: [crit,feasprob,T1,tau1,T2,tau2]
  auto m1 = load_csv("data/global_live_points.csv");
  MLP c1  = MLP::load("data/mlp_c1.txt");
  MLP s1  = MLP::load("data/mlp_s1.txt");
  MLP cu  = MLP::load("data/mlp_cu.txt");
  // requires separate dag due to unit-2's feed being hardwired to unit1's
  // outlet
  mc::FFGraph dag2;
  auto vu = dag2.add_vars(3, "u");  // inlets (c1A, c1B, c1C)
  auto vd = dag2.add_vars(2, "d");  // decision variables T2, tau2
  auto vp = dag2.add_vars(2, "p");  // theta1, theta2
  // o2: (c2A, c2B, c2C)
  auto o2 = Flowsheet::cstr(vu, vd[0], vd[1], vp[0], vp[1]);
  auto t2 = o2[0] + o2[1] + o2[2];  // total sum of concentrations
  auto G2 = std::vector<mc::FFVar>{
      0.55 - o2[1] / t2,                   // MCB purity floor cstr-2
      0.70 - (1 - o2[0] / Flowsheet::CA0)  // conversion floor benzene cstr-2
  };
  auto sg2 = dag2.subgraph(G2);
  auto th  = Flowsheet::theta_nominal();
  std::vector<double> wk2, g2v(2), dP2{th[0], th[1]};

  size_t agree{};
  for (const auto& r : m1)
  {
    std::vector<double> d1 = {r[2], r[3]};  // T1, tau1
    std::vector<double> d2 = {r[4], r[5]};  // T2, tau2

    // NOTE: Technically all the points that are tested here have already been
    // proven to be feasible, hence they are part of the global_live points set.
    // Here we use the ANN s1 to predict the inlets for cstr-2 based on
    // the settings from the points in the global live set that went into cstr-1
    // If the pipeline of ANNs covers well, it should be able to correctly
    // classify the feasibility of a configuration based on the classifications
    // made by
    //  - c1 : T1, tau1 -> feasibiliity for cstr-1
    //  - s1 : T1, tau1 -> estimated inputs for cstr-2 (c1A, c1B, c1C)
    //  - cu : c1A, c1B, c1C "<-- predicted by s1!"
    // if both ANNs c1 and cu (based on s1) agree, and furthermore cstr-2's two
    // own constraints g2, g3 are not violated when the s1 computed input and
    // the (T2, tau2) configuration from the global live set are applied to it,
    // only then do we consider the pipeline as producing a valid classification
    // for the point under consideration (which is indeed feasible)
    //
    auto uh = s1(d1);  // ANN prediction of (T1, tau1) -> (c1A, c1B, c1C)
    auto ok = c1(d1)[0] <= 0 && cu(uh)[0] <= 0;
    if (ok)
    {
      dag2.eval(sg2, wk2, G2, g2v, vu, uh, vd, d2, vp, dP2);
      ok = g2v[0] <= 0 && g2v[1] <= 0;
    }
    agree += ok;
  }
  std::cout << "agree: " << agree << " global live data points = " << m1.size()
            << " fraction solved = " << (static_cast<double>(agree) / m1.size())
            << "\n";
}
