#include <string>
#include <array>
#include <vector>
#include <set>
#include <map>
#include <queue>
#include <stack>
#include <sstream>
#include <cstdint>
#include <ostream>
#include <algorithm>
#include <Eigen/Dense>
#include "Action.hpp"
#include "DirectedGraph.hpp"

#ifndef STRATEGY_HPP
#define STRATEGY_HPP

class Strategy;

class State {
public:
  State(Action _a_2, Action _a_1, Action _b_2, Action _b_1):
      a_2(_a_2), a_1(_a_1), b_2(_b_2), b_1(_b_1) {assert(AllCorD());};
  State(uint64_t id):  // upper bit [a_2,a_1,b_2,b_1] lower bit
      a_2( ((id>>3)&1)?D:C ), a_1( ((id>>2)&1)?D:C ),
      b_2( ((id>>1)&1)?D:C ), b_1( ((id>>0)&1)?D:C ) { assert(AllCorD());};
  State(const char str[4]):
      a_2(C2A(str[0])), a_1(C2A(str[1])),
      b_2(C2A(str[2])), b_1(C2A(str[3])) {assert(AllCorD());};
  const Action a_2, a_1, b_2, b_1;
  bool AllCorD() const {
    return (
        (a_2==D || a_2==C) && (a_1==D || a_1==C) &&
        (b_2==D || b_2==C) && (b_1==D || b_1==C)
    );
  }

  bool operator==(const State & rhs) const {
    return (a_2==rhs.a_2 && a_1==rhs.a_1 && b_2==rhs.b_2 && b_1==rhs.b_1);
  }
  friend std::ostream &operator<<(std::ostream &os, const State &s) {
    os << s.a_2 << s.a_1 << s.b_2 << s.b_1;
    return os;
  };

  State NextState(Action act_a, Action act_b) const {
    assert(act_a == C || act_a == D);
    assert(act_b == C || act_b == D);
    return State(a_1,act_a,b_1,act_b);
  };

  std::array<State,4> PossiblePrevStates() const {
    std::array<State,4> ans = {
        State(C, a_2, C, b_2),
        State(C, a_2, D, b_2),
        State(D, a_2, C, b_2),
        State(D, a_2, D, b_2)
    };
    return ans;
  }

  int RelativePayoff() const {
    if( a_1 == C && b_1 == D ) { return -1; }
    else if( a_1 == D && b_1 == C ) { return  1; }
    else if( a_1 == b_1 ) { return 0; }
    else { assert(false); return -10000; }
  }

  State SwapAB() const { return State(b_2, b_1, a_2, a_1); } // state from B's viewpoint

  std::array<State,2> NoisedStates() const {
    Action a_1_n = (a_1==C) ? D : C;
    Action b_1_n = (b_1==C) ? D : C;
    std::array<State,2> ans = { State(a_2,a_1_n,b_2,b_1), State(a_2,a_1,b_2,b_1_n) };
    return std::move(ans);
  }
  int NumDiffInT1(const State& other) const {
    if(a_2 != other.a_2 || b_2 != other.b_2 ) {
      return -1;
    }
    else {
      int cnt = 0;
      if(a_1 != other.a_1) cnt++;
      if(b_1 != other.b_1) cnt++;
      return cnt;
    }
  }

  uint64_t ID() const {  // ID must be 0~15 integer. AllC: 0, AllD: 15
    uint64_t id = 0;
    if( a_2 == D ) { id += 1ULL << 3; }
    if( a_1 == D ) { id += 1ULL << 2; }
    if( b_2 == D ) { id += 1ULL << 1; }
    if( b_1 == D ) { id += 1ULL << 0; }
    return id;
  }
  bool operator<( const State & rhs ) const{
    return ( ID() < rhs.ID() );
  }

};


class Strategy {
public:
  Strategy( const std::array<Action,16>& acts ); // construct a strategy from a list of actions
  Strategy( const char acts[16] );
  std::array<Action,16> actions;

  std::string ToString() const;
  uint32_t ID() const;
  friend std::ostream &operator<<(std::ostream &os, const Strategy &strategy);
  bool operator==(const Strategy & rhs) const {
    for(size_t i=0; i<actions.size(); i++) { if(actions[i] != rhs.actions[i]) return false; }
    return true;
  }

  Action ActionAt( const State& s ) const { return actions[s.ID()]; }
  void SetAction( const State& s, Action a ) { actions[s.ID()] = a; }
  bool IsDefensible() const;  // check defensibility.
  std::array<double,16> StationaryState(double e=0.00001, const Strategy* coplayer = NULL) const;
  bool IsEfficient(double e=0.00001, double th=0.95) const {
    return (StationaryState(e)[0]>th);
  } // check efficiency. all actions must be fixed
  bool IsEfficientTopo() const; // check efficiency using ITG
  bool IsDistinguishable(double e=0.00001, double th=0.95) const {
    const Strategy allc("cccccccccccccccc");
    return (StationaryState(e,&allc)[0]<th); };  // check distinguishability against AllC
  bool IsDistinguishableTopo() const; // check distinguishability using the transition graph
  DirectedGraph ITG() const;  // construct ITG.
  std::array<int,16> DestsOfITG() const; // Trace the intra-transition-graph from node i. Destination is stored in i'th element.
  int NextITGState(const State& s) const; // Trace the intra-transition graph by one step
private:
  std::vector<State> NextPossibleStates(State current) const;
};

#endif //STRATEGY_HPP

