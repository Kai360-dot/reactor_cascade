#include "flowsheet.hpp"

#include <gtest/gtest.h>

#include <ffunc.hpp>
#include <vector>

TEST(Flowsheet, ValidateBasicSetup)
{
  mc::FFGraph dag;
  Flowsheet fs(dag);
  // collect all 9 output handles
  std::vector<mc::FFVar> out = fs.out1;
  out.insert(out.end(), fs.out2.begin(), fs.out2.end());
  out.insert(out.end(), fs.G.begin(), fs.G.end());

  mc::FFSubgraph sg = dag.subgraph(out);
  std::vector<double> wk, res(out.size());
  auto const th = Flowsheet::theta_nominal();
  std::vector<double> dD{328.15, 3.0, 328.15, 5.0};  // nominal design (T, tau)
  std::vector<double> dP{th[0], th[1], th[0], th[1]};
  std::vector<double> dC{0.05, 0.55, 0.70};

  dag.eval(sg, wk, out, res, fs.D, dD, fs.P, dP, fs.C, dC);

  // CSTR 1 outlet
  EXPECT_NEAR(res[0], 6.282023, 1e-5);
  EXPECT_NEAR(res[1], 4.545889, 1e-5);
  EXPECT_NEAR(res[2], 0.452089, 1e-5);
  // CSTR2 outlet
  EXPECT_NEAR(res[3], 2.700784, 1e-5);
  EXPECT_NEAR(res[4], 6.971587, 1e-5);
  EXPECT_NEAR(res[5], 1.607629, 1e-5);
  // constraint margins at nominal (feasible, g <= 0)
  EXPECT_NEAR(res[6], -0.0099, 2e-4);
  EXPECT_NEAR(res[7], -0.0680, 2e-4);
  EXPECT_NEAR(res[8], -0.0606, 2e-4);
}
