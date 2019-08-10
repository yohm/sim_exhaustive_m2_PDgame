#include <iostream>
#include <cassert>
#include "Strategy.hpp"

void test_State() {
  State s("dccd");
  assert(s.a_2 == D);
  assert(s.a_1 == C);
  assert(s.b_2 == C);
  assert(s.b_1 == D);

  uint64_t id = s.ID();
  assert( id == 9 );

  assert( s == State(id) );

  assert( s.NextState(D,C) == State("cddc") );

  assert( s.RelativePayoff() == -1 );
  assert( State("dcdc").RelativePayoff() == 0 );
  assert( State("cddc").RelativePayoff() == 1 );

  assert( State("cddc").SwapAB() == State("dccd") );

  auto noised = State("dcdc").NoisedStates();
  assert( noised[0] == State("dddc") );
  assert( noised[1] == State("dcdd") );

  auto prev = State("ddcd").PossiblePrevStates();
  assert( prev[0] == State("cdcc") );
  assert( prev[1] == State("cddc") );
  assert( prev[2] == State("ddcc") );
  assert( prev[3] == State("dddc") );
}

void test_Strategy() {
  const std::array<Action,16> acts = {
      C,C,C,C,D,D,D,D,
      C,C,C,C,D,D,D,D
  };
  Strategy s1(acts);
  assert( s1.actions[0] == C );
  assert( s1.actions[7] == D );
  assert( s1.actions[11] == C );
  assert( s1.actions[15] == D );

  std::string bits("ccccddddccccdddd");
  assert( s1.ToString() == bits );
  assert( s1 == Strategy(bits.c_str()) );

  assert( s1.ActionAt(State("cccc")) == C );
  assert( s1.ActionAt("dddd") == D );  // implicit conversion

  {
    Strategy alld("dddddddddddddddd");
    assert( alld.IsDefensible() == true );
    assert( alld.IsEfficient() == false );
    assert( alld.IsEfficientTopo() == false );
    auto dests = alld.DestsOfITG();
    for(int i: dests) { assert( i == 15 ); } // all goes to dddd

    auto stat = alld.StationaryState(0.001);
    for(int i=0; i<15; i++) { assert(stat[i] < 0.01); }
    assert(stat[15] > 0.99);

    assert( alld.IsDistinguishable() == true );
    assert( alld.IsDistinguishableTopo() == true );
  }
  {
    Strategy allc("cccccccccccccccc");
    assert( allc.IsDefensible() == false );
    assert( allc.IsEfficient() == true );
    assert( allc.IsEfficientTopo() == true );
    auto dests = allc.DestsOfITG();
    for(int i: dests) { assert( i == 0 ); } // all goes to cccc

    auto stat = allc.StationaryState(0.001);
    for(int i=1; i<16; i++) { assert(stat[i] < 0.01); }
    assert(stat[0] > 0.99);

    assert( allc.IsDistinguishable() == false );
    assert( allc.IsDistinguishableTopo() == false );
  }
  {
    Strategy tft("cdcdcdcdcdcdcdcd");
    assert( tft.IsDefensible() == true );
    assert( tft.IsEfficient() == false );
    assert( tft.IsEfficientTopo() == false );
    auto dests = tft.DestsOfITG();
    for(int i: dests) { assert( i == 0 || i == 15 || State("cddc").ID() ); } // all goes to either cccccc, dddddd, cddc

    auto stat = tft.StationaryState(0.001);
    assert( abs(stat[0]-0.25) < 0.01 );
    assert( abs(stat[6]-0.25) < 0.01 );
    assert( abs(stat[9]-0.25) < 0.01 );
    assert( abs(stat[15]-0.25) < 0.01 );

    assert( tft.IsDistinguishable() == false );
    assert( tft.IsDistinguishableTopo() == false );
  }
  {
    Strategy wsls("ccccccccccccccccc");
    for(int i=0; i<16; i++) {
      Action act = ( (i & 1) == (i>>2 & 1) ) ? C : D;
      wsls.SetAction(i, act);
    }
    assert( wsls.IsDefensible() == false );
    assert( wsls.IsEfficient() == true );
    assert( wsls.IsEfficientTopo() == true );
    auto dests = wsls.DestsOfITG();
    for(int i: dests) { assert( i == 0 ); } // all goes to cccc

    auto stat = wsls.StationaryState(0.001);
    for(int i=1; i<16; i++) { assert(stat[i] < 0.01); }
    assert(stat[0] > 0.99);

    assert( wsls.IsDistinguishable() == true );
    assert( wsls.IsDistinguishableTopo() == true );
  }
  {
    Strategy tf2t("ccccccccccccccccc");
    for(int i=0; i<16; i++) {
      Action act = ((i&3)==3) ? D : C;
      tf2t.SetAction(i, act);
    }
    assert( tf2t.IsDefensible() == false );
    assert( tf2t.IsEfficient() == true );
    assert( tf2t.IsEfficientTopo() == true );
    auto dests = tf2t.DestsOfITG();
    for(int i: dests) { assert( i == 0 || i == 15 ); }

    auto stat = tf2t.StationaryState(0.001);
    assert(stat[0] > 0.99);
    for(int i=1; i<16; i++) { assert(stat[i] < 0.01); }

    assert( tf2t.IsDistinguishable() == false );
    assert( tf2t.IsDistinguishableTopo() == false );
  }
}

void test_TFTATFT() {
  // 0  cc,cc : c , 8  dc,cc : c
  // 1  cc,cd : d , 9  dc,cd : d
  // 2  cc,dc : c , 10 dc,dc : c
  // 3  cc,dd : d , 11 dc,dd : c
  // 4  cd,cc : d , 12 dd,cc : d
  // 5  cd,cd : c , 13 dd,cd : c
  // 6  cd,dc : c , 14 dd,dc : c
  // 7  cd,dd : d , 15 dd,dd : d
  const char* actions = "cdcddccdcdccdccd";
  Strategy tft_atft(actions);
  assert( tft_atft.IsDefensible() );
  assert( tft_atft.IsEfficient() );

  assert( tft_atft.IsDistinguishable() == true );
  assert( tft_atft.IsDistinguishableTopo() == true );
}

int main() {
  std::cout << "Testing Strategy class" << std::endl;

  test_State();
  test_Strategy();
  test_TFTATFT();
  return 0;
}

