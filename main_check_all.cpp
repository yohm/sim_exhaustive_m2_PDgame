#include <iostream>
#include <cstdint>
#include <vector>
#include <string>
#include "Strategy.hpp"

int main(int argc, char* argv[]) {
  if(argc != 4) {
    std::cerr << "[Error] invalid number of arguments" << std::endl;
    std::cerr << "  Usage: " << argv[0] << " <1/0 defensibility> <1/0 efficiency> <1/0 distinguishability>" << std::endl;
    std::cerr << "  example: " << argv[0] << " 1 1 0" << std::endl;
  }
  int check_def = atoi(argv[1]);
  int check_eff = atoi(argv[2]);
  int check_dis = atoi(argv[3]);

  uint64_t num_passed = 0ULL;
  uint64_t num_cand = 0ULL;
  for(uint64_t i=0ULL; i<(1ULL<<16); i++) {
    num_cand++;
    char bits[17] = {'\0'};
    for(uint64_t j=0; j<16; j++) { bits[j] = (((i>>j)&1) ? 'd' : 'c'); }
    const Strategy s(bits);

    if( check_def && ! s.IsDefensible() ) continue;
    if( check_eff ) {
      bool e = s.IsEfficient();
      if (e != s.IsEfficientTopo()) { throw "must not happen"; }
      if (!e) { continue; }
    }
    if( check_dis ) {
      bool dis = s.IsDistinguishable();
      if( dis != s.IsDistinguishableTopo() ) { throw "must not happen"; }
      if(!dis) { continue; }
    }
    std::cout << s.ToString() << std::endl;
    num_passed++;
  }

  std::cerr << "# passed / # candidates : " << num_passed << " / " << num_cand << std::endl;
  return 0;
}