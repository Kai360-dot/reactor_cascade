#ifndef LOADERS_HPP
#define LOADERS_HPP

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sstream>
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


[[nodiscard]]
inline static std::vector<std::vector<double>> load_csv(const std::string& name)
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


#endif  // LOADERS_HPP
