#ifndef LOADERS_HPP
#define LOADERS_HPP

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

inline void load_box(std::vector<double>& lb, std::vector<double>& ub)
{
  // load u2 bounds box
  std::ifstream ifs("data/unit2_inlet_box.csv");
  if (!ifs) throw std::runtime_error("error loading file");
  std::string line;
  std::getline(ifs, line);  // skip header
  std::string word_;

  auto read_ = [&ifs, &word_](std::vector<double>& vec) {
    std::getline(ifs, word_, ',');  // skip first
    for (size_t i = 0; i < 3; ++i)
    {
      (i == 2) ? std::getline(ifs, word_) : std::getline(ifs, word_, ',');
      vec.push_back(std::stod(word_));
    }
  };

  read_(lb);
  read_(ub);
  // lb, ub
}

#endif  // LOADERS_HPP
