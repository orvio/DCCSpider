/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
  Copyright (C) 2016 Martin Mueller mm@nohost.de
  This file is part of DCCSpider
*                                                                       *
  DCCSpider is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
*                                                                       *
  DCCSpider is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
*                                                                       *
  You should have received a copy of the GNU General Public License
  along with DCCSpider.  If not, see <http://www.gnu.org/licenses/>.
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#include "DisplayController.h"
#include "LoconetMaster.h"
#include "DCCBaseStation.h"

DisplayController displayController(15, 40, 16, 38, 17, 36, 18, 34, 19, 32, 20);
DCCBaseStation dccBaseStation(12, 3, A0, 20);

ISR(TIMER1_COMPB_vect) {             // set interrupt service for OCR1B of TIMER-1 which flips direction bit of Motor Shield Channel A controlling Main Track
  DCC_SIGNAL((*dccBaseStation.getRegisterList()), 1, 16)
}
LoconetMaster loconetMaster(20);

void setup() {
  Serial.begin(115200);            // configure serial interface
  Serial.flush();
  
  dccBaseStation.begin(1);
  displayController.begin( 20, 2, "DCCSpider", "V0.00");
  displayController.setDisplayState(DisplayController::EnterDispatchAddress);
  loconetMaster.begin(46,&dccBaseStation);
  
  dccBaseStation.enableTrackPower();
}

void loop() {
  displayController.updateDisplay();
  loconetMaster.processReceivedMessages();
}
