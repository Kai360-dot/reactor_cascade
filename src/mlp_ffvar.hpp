#ifndef MLP_FFVAR_HPP
#define MLP_FFVAR_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

struct MLP
{
  struct Layer
  {
    size_t nin{}, nout{};
    std::vector<std::vector<double>> W;  // [nout][nin]
    std::vector<double> b;               // [nout]
  };
  std::vector<Layer> layers;

  [[nodiscard]]
  static MLP load(std::string const& path)
  {
    std::ifstream f(path);
    if (!f) throw std::runtime_error("MLP::load: can't open " + path);
    MLP net;
    size_t L{};
    f >> L;
    net.layers.resize(L);
    for (auto& l : net.layers)
    {
      f >> l.nin >> l.nout;
      l.W.assign(l.nout, std::vector<double>(l.nin));
      for (auto& row : l.W)
      {
        for (double& w : row) f >> w;
      }
      l.b.resize(l.nout);
      for (double& v : l.b) f >> v;
    }
    if (!f) throw std::runtime_error("MLP::load: malformed file" + path);
    return net;
  }

  template <typename T>
  std::vector<T> operator()(std::vector<T> x) const
  {
    using std::tanh; // ADL picks mc::FFVar's tanh or std::tanh depending on T
    for (size_t l{}; l < layers.size(); ++l)
    {
      const auto& L = layers[l];
      std::vector<T> y;
      y.reserve(L.nout);
      for (size_t i{}; i < L.nout; ++i)
      {
        T s = L.b[i]; // initialize w/ bias
        for (size_t j{}; j < L.nin; ++j) s += L.W[i][j] * x[j];
        // tanh isn't applied to last layers output
        y.push_back(l + 1 < layers.size() ? tanh(s) : s);
      }
      x = std::move(y);
    }
    return x;
  }
};

#endif  // MLP_FFVAR_HPP
