#include <iostream>
#include <set>
#include <map>
#include "Strategy.hpp"


Strategy::Strategy(const std::array<Action,16>& acts): actions(acts) {}

Strategy::Strategy(const char acts[16]) {
  for( size_t i=0; i<16; i++) {
    actions[i] = C2A(acts[i]);
  }
}

std::vector<State> Strategy::NextPossibleStates(State current) const {
  std::vector<State> next_states;
  Action act_a = ActionAt(current);
  next_states.emplace_back( current.NextState(act_a,C) );
  next_states.emplace_back( current.NextState(act_a,D) );
  return next_states;
}

std::ostream &operator<<(std::ostream &os, const Strategy &strategy) {
  for(size_t i=0; i<16; i++) {
    os << strategy.actions[i] << '|' << State(i) << "  ";
    if(i%4 == 3) { os << std::endl; }
  }
  return os;
}

std::string Strategy::ToString() const {
  char c[17];
  for(size_t i=0; i<16; i++) {
    c[i] = A2C(actions[i]);
  }
  c[16] = '\0';
  return std::string(c);
}

inline int8_t MIN(int8_t a, int8_t b) { return (a<b)?a:b; }

bool Strategy::IsDefensible() const {
  const size_t N = 16;

  int8_t d[N][N] = {0};

  // construct adjacency matrix
  const int INF = 32; // 32 is large enough since the path length is between -16 to 16.
  for(size_t i=0; i<N; i++) {
    for(size_t j=0; j<N; j++) {
      d[i][j] = INF;
    }
  }

  for(size_t i=0; i<N; i++) {
    State si(i);
    std::vector<State> sjs = NextPossibleStates(si);
    for( auto sj: sjs) {
      size_t j = sj.ID();
      d[i][j] = si.RelativePayoff();
    }
    if(d[i][i] < 0) { return false; }
  }

  for(size_t k=0; k<N; k++) {
    for(size_t i=0; i<N; i++) {
      for(size_t j=0; j<N; j++) {
        d[i][j] = MIN(d[i][j], d[i][k]+d[k][j]);
      }
      if(d[i][i] < 0) { return false; }
    }
  }
  return true;
}

std::array<int, 16> Strategy::DestsOfITG() const {
  std::array<int, 16> dests = {};
  std::array<bool, 16> fixed = {false};

  for(int i=0; i<16; i++) {
    std::array<bool,16> visited = { false }; // initialize by false
    visited[i] = true;
    State init(i);
    int next = NextITGState(init);
    while(next >= 0) {
      if(visited[next] || fixed[next]) { break; }
      visited[next] = true;
      next = NextITGState(State(next));
    }
    int d = next;
    if(next >= 0) {
      d = fixed[next] ? dests[next] : next;
    }
    for(uint64_t j=0; j<16; j++) {
      if(visited[j]) {
        dests[j] = d;
        fixed[j] = true;
      }
    }
  }
  return dests;
}

int Strategy::NextITGState(const State &s) const {
  Action move_a = ActionAt(s);
  Action move_b = ActionAt(s.SwapAB());
  return s.NextState(move_a, move_b).ID();
}

std::array<double, 16> Strategy::StationaryState(double e, const Strategy* coplayer) const {
  if(coplayer==NULL) { coplayer = this; }
  Eigen::Matrix<double,17,16> A;

  for(int i=0; i<16; i++) {
    const State si(i);
    for(int j=0; j<16; j++) {
      // calculate transition probability from j to i
      const State sj(j);
      // State next = NextITGState(sj);
      Action act_a = ActionAt(sj);
      Action act_b = coplayer->ActionAt( sj.SwapAB() );
      State next = sj.NextState(act_a, act_b);
      int d = next.NumDiffInT1(si);
      if( d < 0 ) {
        A(i,j) = 0.0;
      }
      else if( d == 0 ) {
        A(i,j) = (1.0-e)*(1.0-e);
      }
      else if( d == 1 ) {
        A(i,j) = (1.0-e)*e;
      }
      else if( d == 2 ) {
        A(i,j) = e*e;
      }
      else {
        assert(false);
      }
    }
    A(i,i) = A(i,i) - 1.0;  // subtract unit matrix
  }
  for(int i=0; i<16; i++) { A(16,i) = 1.0; }  // normalization condition

  Eigen::VectorXd b(17);
  for(int i=0; i<16; i++) { b(i) = 0.0;}
  b(16) = 1.0;

  Eigen::VectorXd x = A.colPivHouseholderQr().solve(b);

  std::array<double,16> ans = {0};
  for(int i=0; i<16; i++) {
    ans[i] = x(i);
  }
  return ans;
}

DirectedGraph Strategy::ITG() const {
  DirectedGraph g(16);
  for(int i=0; i<16; i++) {
    State sa(i);
    State sb = sa.SwapAB();
    int j = sb.ID();
    Action act_a = actions[i], act_b = actions[j];
    int n = sa.NextState(act_a, act_b).ID();
    g.AddLink(i, n);
  }

  return std::move(g);
}

bool Strategy::IsEfficientTopo() const {
  if( actions[0] != C ) { return false; }

  auto UpdateGn = [](DirectedGraph& gn) {
    components_t sinks = gn.SinkSCCs();
    for(const comp_t& sink: sinks) {
      for(long from: sink) {
        for(int i=0; i<2; i++) {
          long to = (unsigned long)from^((i==0)?1UL:4UL);
          if( ! gn.HasLink(from, to) ) {
            gn.AddLink(from, to);
          }
        }
      }
    }
  };

  std::vector<int> checked(16, 0);
  checked[0] = 1;
  auto complete = [&checked]() {
    for(int i: checked) {
      if(i==0) { return false; }
    }
    return true;
  };

  DirectedGraph gn = ITG();
  for( int n=0; !complete(); n++ ) {
    if(n > 0) {
      UpdateGn(gn);
    }
    for(int i=1; i<16; i++) {
      if( checked[i] == 1 ) { continue; }
      if( gn.Reachable(i,0) ) {
        if( gn.Reachable(0,i) ) {
          return false;   // inefficient
        }
        else {
          checked[i] = 1;
        }
      }
    }
  }

  return true;
}
bool Strategy::IsDistinguishableTopo() const {
  const Strategy allc("cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc");
  if( actions[0] != C ) { return true; }

  auto UpdateGn = [](DirectedGraph& gn) {
    components_t sinks = gn.SinkSCCs();
    for(const comp_t& sink: sinks) {
      for(long from: sink) {
        for(int i=0; i<2; i++) {
          long to = (unsigned long)from^((i==0)?1UL:4UL);
          if( ! gn.HasLink(from, to) ) {
            gn.AddLink(from, to);
          }
        }
      }
    }
  };

  std::vector<int> checked(16, 0);
  checked[0] = 1;
  auto complete = [&checked]() {
    for(int i: checked) {
      if(i==0) { return false; }
    }
    return true;
  };

  DirectedGraph gn(16);
  for(int i=0; i<16; i++) {
    State sa(i);
    State sb = sa.SwapAB();
    Action act_a = ActionAt(sa);
    Action act_b = allc.ActionAt(sb);
    assert( act_b == C );  // assert AllC
    int j = sa.NextState(act_a, act_b).ID();
    gn.AddLink(i, j);
  }

  for( int n=0; !complete(); n++ ) {
    if(n > 0) {
      UpdateGn(gn);
    }
    for(int i=1; i<16; i++) {
      if( checked[i] == 1 ) { continue; }
      if( gn.Reachable(i,0) ) {
        if( gn.Reachable(0,i) ) {
          return true;   // inefficient
        }
        else {
          checked[i] = 1;
        }
      }
    }
  }

  return false;
}

