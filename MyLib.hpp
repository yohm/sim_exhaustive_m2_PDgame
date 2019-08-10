#ifndef MYLIB_HPP
#define MYLIB_HPP

#include <iostream>
#include <iomanip>
#include "Strategy.hpp"

#ifdef NDEBUG
#define DP(x) do {} while (0)
#else
#define DP(x) do { std::cerr << x << std::endl; } while (0)
#endif

namespace {
  void RecursiveCommas(std::ostream& os, uint64_t n)
  {
    uint64_t rest = n % 1000; //"last 3 digits"
    n /= 1000;         //"beginning"

    if (n > 0) {
      RecursiveCommas(os, n); //printing "beginning"

      //and last chunk
      os << ',' << std::setfill('0') << std::setw(3) << rest;
    }
    else {
      os << rest;
    }
  }
}

inline std::string ToC(uint64_t n) {
  std::ostringstream oss;
  RecursiveCommas(oss, n);
  return oss.str();
}

#endif
