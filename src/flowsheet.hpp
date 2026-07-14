#ifndef FLOWSHEET_HPP
#define FLOWSHEET_HPP
#include <array>
#include <cmath>
#include <vector>

#include "ffunc.hpp"

// Two CSTR benzene chlorination cascade
// Feasible iff for all j : G[j] <= 0
struct Flowsheet
{
  static constexpr double R    = 8.314;   // J/mol/K
  static constexpr double E1   = 48'000;  // J/mol
  static constexpr double E2   = 55'000;  // J/mol
  static constexpr double CA0  = 11.28;   // mol/L; pure benzene feed
  static constexpr double TREF = 328.15;  // K; temperature

  // nominal pseudo-first-order pre-exponentials
  static std::array<double, 2> theta_nominal()
  {
    double const k1ref = 8.84e-3 * 60.0 * 0.5;  // 1/min
    double const k2ref = k1ref / 8;             // 1/min
    return {k1ref * std::exp(E1 / (R * TREF)),
            k2ref * std::exp(E2 / (R * TREF))};
  }
  std::vector<mc::FFVar> D;  // controls: T1, tau1, T2, tau2
  std::vector<mc::FFVar> P;  // parameters: th1_u1, th2_u1, th1_u2, th2_u2
  std::vector<mc::FFVar> C;  // constants: xC1_max, purB_min, xA_min
  std::vector<mc::FFVar> out1, out2;  // (cA, cB, cC) per unit
  std::vector<mc::FFVar> G;           // constraints

  explicit Flowsheet(mc::FFGraph& dag)
  {
    D = dag.add_vars(4, "d");
    P = dag.add_vars(4, "p");
    C = dag.add_vars(3, "c");

    // only benzene in feed initially:
    std::vector<mc::FFVar> feed{mc::FFVar(CA0), mc::FFVar(0.0), mc::FFVar(0.0)};
    // the calls to Flowsheet::cstr() build up expression trees
    out1 = cstr(feed, D[0], D[1], P[0], P[1]);
    out2 = cstr(out1, D[2], D[3], P[2], P[3]);

    mc::FFVar tot1 = out1[0] + out1[1] + out1[2];  // cA + cB + cC
    mc::FFVar tot2 = out2[0] + out2[1] + out2[2];  // cA + cB + cC

    // constraints: (C[i] : constraint scalar)
    G = {
        out1[2] / tot1 - C[0],      // DCB fraction cap cstr-1
        C[1] - out2[1] / tot2,      // MCB purity floor cstr-2
        C[2] - (1 - out2[0] / CA0)  // conversion floor benzene cstr-2
    };
  }

  static std::vector<mc::FFVar> cstr(std::vector<mc::FFVar> const& in,
                                     mc::FFVar const& T, mc::FFVar const& tau,
                                     mc::FFVar const& th1, mc::FFVar const& th2)
  {
    mc::FFVar k1 = th1 * exp(-E1 / (R * T));
    mc::FFVar k2 = th2 * exp(-E2 / (R * T));
    mc::FFVar cA = in[0] / (1.0 + k1 * tau);
    mc::FFVar cB = (in[1] + k1 * tau * cA) / (1.0 + k2 * tau);
    mc::FFVar cC = in[2] + k2 * tau * cB;
    return {cA, cB, cC};
  }
};

#endif  // FLOWSHEET_HPP
