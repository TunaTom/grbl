/*
  hbridgetest.c - test for the converter to hbridge signals.
  

  Copyright (c) 2012 Tom Vollerthun

  Grbl is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Grbl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Grbl.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <stdio.h>

#include "nuts_bolts.h"

/*define some string helpers to display ints as binary pattern*/
#define BYTETOBINARYPATTERN "%d%d%d%d%d%d%d%d"
#define BYTETOBINARY(uint8_t)  \
  (uint8_t & 0x80 ? 1 : 0), \
  (uint8_t & 0x40 ? 1 : 0), \
  (uint8_t & 0x20 ? 1 : 0), \
  (uint8_t & 0x10 ? 1 : 0), \
  (uint8_t & 0x08 ? 1 : 0), \
  (uint8_t & 0x04 ? 1 : 0), \
  (uint8_t & 0x02 ? 1 : 0), \
  (uint8_t & 0x01 ? 1 : 0) 

/** Assert that the expected and the actual bit-pattern equal.
 * The other parameters are used to display a more meaningful errormessage
 * in case the assertion fails.
 */
void assertEqualsPattern(int input, int state, uint8_t actual, uint8_t expected)
{
  if (actual != expected) {
    printf("FAIL! ["BYTETOBINARYPATTERN"] AND ["BYTETOBINARYPATTERN"] was ["BYTETOBINARYPATTERN"], but expected ["BYTETOBINARYPATTERN"]\n", 
	   BYTETOBINARY(input), 
	   BYTETOBINARY(state),  
	   BYTETOBINARY(actual), 
	   BYTETOBINARY(expected));
    exit(1);
  }
}

/**
 * Assert that the expected and the actual integer values are equal.
 */
void assertEquals(int actual, int expected)
{
  if (actual != expected) {
    printf("FAIL! Was [%d], but expected [%d]\n", actual, expected);
    exit(1);
  }
}

/**
 * tests if looping through the phase-array works correctly
 * (index check).
 */
void shouldGiveNextIndex() 
{
  /* test stepping forwards*/
  assertEquals(getNextIndex(0), 1);
  assertEquals(getNextIndex(1), 2);
  assertEquals(getNextIndex(2), 3);
  assertEquals(getNextIndex(3), 0);

  /*if values are way out, start at 0 again*/
  assertEquals(getNextIndex(4), 0);
  assertEquals(getNextIndex(100), 0);
  assertEquals(getNextIndex(-1), 0);
  assertEquals(getNextIndex(-10), 0);
}

/**
 * tests if the next phase is computed correctly given
 * the step/dir-pattern and the previous phase
 */
void shouldReturnPatternDependingOnPreviousPhase() 
{ 
  /* stepping forwards */
  assertEqualsPattern(1, 1, getNextPhase(1, 1, 0), 1);
  assertEqualsPattern(1, 1, getNextPhase(1, 1, 1), 3);
  assertEqualsPattern(1, 1, getNextPhase(1, 1, 2), 2);
  assertEqualsPattern(1, 1, getNextPhase(1, 1, 3), 0);
  
  /* reverse step direction if dir-bit is 0 */
  assertEqualsPattern(1, 0, getNextPhase(1, 0, 0), 2);
  assertEqualsPattern(1, 0, getNextPhase(1, 0, 1), 0);
  assertEqualsPattern(1, 0, getNextPhase(1, 0, 2), 1);
  assertEqualsPattern(1, 0, getNextPhase(1, 0, 3), 3);
  
  /*if no step is given, pattern should be zero in both directions*/
  assertEqualsPattern(0, 1, getNextPhase(0, 1, 0), 0);
  assertEqualsPattern(0, 1, getNextPhase(0, 1, 1), 0);
  assertEqualsPattern(0, 1, getNextPhase(0, 1, 2), 0);
  assertEqualsPattern(0, 1, getNextPhase(0, 1, 3), 0);
  assertEqualsPattern(0, 0, getNextPhase(0, 0, 0), 0);
  assertEqualsPattern(0, 0, getNextPhase(0, 0, 1), 0);
  assertEqualsPattern(0, 0, getNextPhase(0, 0, 2), 0);
  assertEqualsPattern(0, 0, getNextPhase(0, 0, 3), 0);
}

/**
 * tests if the next phase is computed correctly for multiple axis
 * given the step/dir-pattern and the previous phase
 */
void shouldConvertMultiaxisPhases() 
{
//inputPattern=00000000(=0)  statePattern=00000000(=0) outputPattern=00000000(=0)
  assertEqualsPattern(0, 0, getMultiAxisPhase(0, 0), 0);
  
// ys=1,yd=0                   y=11         yNew=10
//inputPattern = 00001000(=8)   statePattern=11011100(=220) outputPattern=01000000(=64)
  assertEqualsPattern(8, 220, getMultiAxisPhase(8, 220), 64);
  
//xs=1,xd=1 ys=1,yd=0     x=00 y=11          xNew=10 yNew=10
//inputPattern = 00101100(=44)   statePattern=11011000(=216) outputPattern=01100000(=96)
  assertEqualsPattern(44, 216, getMultiAxisPhase(44, 216), 96);
                
//ys=1,yd=0 zs=1,zd=1     y=00 z=01          yNew=01 zNew=00
//inputPattern = 10011000(=152)   statePattern=00110000(=48) outputPattern=00001000(=8)
  assertEqualsPattern(152, 48, getMultiAxisPhase(152, 48), 8);

//xs=1,xd=1 ys=1,yd=1 zs=1,zd=1  x=00 y=11 z=10  xNew=10 yNew=01 zNew=11
//inputPattern = 11111100(=252)   statePattern=11001000(=200) outputPattern=10111000(=184)
  assertEqualsPattern(252, 200, getMultiAxisPhase(252, 200), 184);
}

/**
 * tests if the next phase for multiple axis is computed 
 * correctly given the step/dir-pattern.
 * The previous phase is kept as internal state.
 */
void shouldKeepStateOverTime() 
{
  // 0 0
  // 0 1
  // 1 1
  // 1 0
  /*no op*/
  //inputPattern=00000000(=0)  statePattern=00000000(=0) outputPattern=00000000(=0)
  assertEqualsPattern(0, 0, toHbridge(0), 0);
  
  /* Stepping all axis simultaneously forwards */
  //inputPattern=000 111 00(=28)  statePattern=000 000 00(=0) outputPattern=000 111 00(=28)
  assertEqualsPattern(28, 0, toHbridge(28), 28);
  //inputPattern=000 111 00(=28)  statePattern=000 111 00(=28) outputPattern=111 111 00(=252)
  assertEqualsPattern(28, 28, toHbridge(28), 252);
  //inputPattern=000 111 00(=28)  statePattern=111 111 00(=252) outputPattern=111 000 00(=224)
  assertEqualsPattern(28, 252, toHbridge(28), 224);
  //inputPattern=000 111 00(=28)  statePattern=111 000 00(=242) outputPattern=000 000 00(=0)
  assertEqualsPattern(28, 224, toHbridge(28), 0);
  
  /* Stepping x and z forwards, y backwards*/
  //inputPattern=010 111 00(=92)  statePattern=000 000 00(=0) outputPattern=010 101 00(=84)
  assertEqualsPattern(92, 0, toHbridge(92), 84);
  //inputPattern=010 111 00(=92)  statePattern=010 101 00(=84) outputPattern=111 111 00(=252)
  assertEqualsPattern(92, 84, toHbridge(92), 252);
  //inputPattern=010 111 00(=92)  statePattern=111 111 00(=252) outputPattern=101 010 00(=168)
  assertEqualsPattern(92, 252, toHbridge(92), 168);
  //inputPattern=010 111 00(=92)  statePattern=101 010 00(=168) outputPattern=000 000 00(=0)
  assertEqualsPattern(92, 168, toHbridge(92), 0);
}

/**
 * Run all tests
 */
int main(void)
{
  shouldGiveNextIndex();
  shouldReturnPatternDependingOnPreviousPhase();
  shouldConvertMultiaxisPhases();
  shouldKeepStateOverTime();
  
  printf("SUCCESS: All tests green.\n");
  
  return 0;
}
