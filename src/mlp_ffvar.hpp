#ifndef MLP_FFVAR_HPP
#define MLP_FFVAR_HPP

#include <algorithm>
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

};

#endif  // MLP_FFVAR_HPP
