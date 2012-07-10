/*
  hbridge.c - converts the signals for step/direction that are generated by grbl to signals for phase-type (quadratic? sinusoidal?) stepper drivers.

  Copyright (c) 2012 Tom Vollerthun (vollerthun@gmx.de)

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

#include <stdio.h>
#include "config.h"
#include "nuts_bolts.h"

/** the bit patterns for halfstep movement */
static const uint8_t PHASES[] = {
  0,      //  0 0
  1,      //  0 1
  3,      //  1 1
  2,      //  1 0
};

/** the position within the phase-array. */
static const uint8_t PHASE_INDEX[] = {
  0,      //  0 0 is at index 0
  1,      //  0 1 is at index 1
  3,      //  1 0 is at index 3
  2,      //  1 1 is at index 2
};

static const int AMOUNT_OF_PHASES = sizeof(PHASES)/sizeof(uint8_t);

uint8_t currentPhase = 0;

/** get the next index in the phase array (loop around) */
int getNextIndex(int currentIndex)
{
  int nextIndex = currentIndex + 1;
  if (nextIndex >= AMOUNT_OF_PHASES || nextIndex < 0) {
      nextIndex = 0;
  }

  return nextIndex;
}

/** if dir-bit is not set, the bit pattern is inverted */
uint8_t invertIfNecessary(int dir, uint8_t pattern) {
  /*to move the other way around, invert the halfstep patterns*/
  if (dir == 0) {
    pattern = ~pattern;
  }
  return pattern;
}

/** get the next phase depending on the step, dir and state */
uint8_t getNextPhase(int step, int dir, int currentIndex) 
{
  //not stepping? jump out early
  if (step == 0) {
    return 0;
  }

  int index = getNextIndex(currentIndex);
  uint8_t phase = invertIfNecessary(dir, PHASES[index]);
  
  //get last two bits
  return phase & 3;
}

/** get the bit at the given position */
int bitAt(uint8_t statePattern, int position) {
  return (statePattern & (1 << position)) >> position;
}

/** set the next phase's bit pattern at the step-bit and dir-bit
 * positions 
 */
uint8_t setPhase(uint8_t inputPattern, uint8_t statePattern, uint8_t result, int stepBitPos, int directionBitPos) {
  // extract the step and direction bits
  // from the correct positions
  int step = bitAt(inputPattern, stepBitPos);
  int direction = bitAt(inputPattern, directionBitPos);
  
  // the statePattern holds the high and low bits where
  // the step and direction bits are
  int currentPhase = (bitAt(statePattern, stepBitPos) << 1) | bitAt(statePattern, directionBitPos);
  
  // look up index in reverse array
  int currentIndex = PHASE_INDEX[currentPhase];

  uint8_t nextPhase = getNextPhase(step, direction, currentIndex);

  //clear target bits
  result &= ~(1 << stepBitPos);
  result &= ~(1 << directionBitPos);

  //set target bits with bits of the next phase
  result |= bitAt(nextPhase, 1) << (stepBitPos);
  result |= bitAt(nextPhase, 0) << (directionBitPos);
  return result;
}

/** set the next phase's bit patterns for the three axis. */
uint8_t getMultiAxisPhase(uint8_t inputPattern, uint8_t statePattern) 
{
  uint8_t result = inputPattern;

  result = setPhase(inputPattern, statePattern, result, X_STEP_BIT, X_DIRECTION_BIT);
  result = setPhase(inputPattern, statePattern, result, Y_STEP_BIT, Y_DIRECTION_BIT);
  result = setPhase(inputPattern, statePattern, result, Z_STEP_BIT, Z_DIRECTION_BIT);

  return result;
}

/** convert the given step/dir-pattern to phase-style depending 
 * on the current phase. */
uint8_t toHbridge(uint8_t stepDirPattern)
{
  uint8_t nextPhase = getMultiAxisPhase(stepDirPattern, currentPhase);
  //remember for next time around
  currentPhase = nextPhase;
  return nextPhase;
}


