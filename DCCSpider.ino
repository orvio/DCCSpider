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

#define DCC_POWER_ENABLE_PIN 3

DisplayController displayController(15, 40, 16, 38, 17, 36, 18, 34, 19, 32, 20);
DCCBaseStation dccBaseStation(12, DCC_POWER_ENABLE_PIN, A0, 20);

/*ISR(TIMER1_COMPB_vect) {             // set interrupt service for OCR1B of TIMER-1 which flips direction bit of Motor Shield Channel A controlling Main Track
  DCC_SIGNAL((*dccBaseStation.getRegisterList()), 1, 16)
  }*/

volatile DCCBaseStation::DCCPriorityList * priorityList = dccBaseStation.getPriorityList();
ISR(TIMER1_COMPB_vect) {
  //if current packet done, select next packet
  if (!priorityList->currentPacket || priorityList->currentBit >= priorityList->currentPacket->usedBits) {
    priorityList->currentPacket = 0;

    //check for updated packets starting in critical list
    for (byte i = 0; i < PRIORITY_LIST_COUNT; i++) {
      if (priorityList->_packetLists[i]->newOrUpdatedCount > 0) {
        priorityList->_packetLists[i]->newOrUpdatedPackets[priorityList->_packetLists[i]->firstNewOrUpdatedIndex]->rawPackets[1] = \
            priorityList->_packetLists[i]->newOrUpdatedPackets[priorityList->_packetLists[i]->firstNewOrUpdatedIndex]->rawPackets[0];
        priorityList->currentPacket = &(priorityList->_packetLists[i]->newOrUpdatedPackets[priorityList->_packetLists[i]->firstNewOrUpdatedIndex]->rawPackets[1]);

        //update newOrUpdatedIndexes
        priorityList->_packetLists[i]->newOrUpdatedCount--;
        priorityList->_packetLists[i]->firstNewOrUpdatedIndex++;
        break; //leave loop
      }
    }

    //select next cycle packet if no updated packets found
    if (!priorityList->currentPacket) {
      if ( priorityList->currentCyclePacket) {
        priorityList->currentCyclePacket = priorityList->currentCyclePacket->nextPacket;
      }

      for ( byte i = 0; i < PRIORITY_LIST_COUNT; i++) {
        priorityList->currentList++;
        priorityList->currentList = priorityList->currentList % PRIORITY_LIST_COUNT;
        priorityList->currentCyclePacket = priorityList->_packetLists[priorityList->currentList]->firstPacket; //note: this might be NULL!
        if (priorityList->currentCyclePacket) {
          break; //packet found, leave loop
        }
      }

      //no packets loaded, send idle packet
      if (!priorityList->currentCyclePacket) {
        priorityList->currentPacket = &(priorityList->_idlePacket);
      }
      else { //packet found
        //move is not necessary, because stuff is moved when the packet is processed after an update
        //priorityList->currentCyclePacket->rawPackets[1] = priorityList->currentCyclePacket->rawPackets[0];
        priorityList->currentPacket = &(priorityList->currentCyclePacket->rawPackets[1]);
      }
    }

    priorityList->currentBit = 0;
  }

  //proceed transmitting current packet
  if (priorityList->currentPacket->bytes[priorityList->currentBit / 8] & (0x80 >> priorityList->currentBit % 8 ) ) {
    OCR1A = DCC_ONE_BIT_TOTAL_DURATION_16BIT_TIMER;
    OCR1B = DCC_ONE_BIT_PULSE_DURATION_16BIT_TIMER;
    // OCR ## N ## A=DCC_ONE_BIT_TOTAL_DURATION_## BC ##BIT_TIMER;                               /*   set OCRA for timer N to full cycle duration of DCC ONE bit */ \
    // OCR ## N ## B=DCC_ONE_BIT_PULSE_DURATION_## BC ##BIT_TIMER;                               /*   set OCRB for timer N to half cycle duration of DCC ONE but */ \

  }
  else {
    OCR1A = DCC_ZERO_BIT_TOTAL_DURATION_16BIT_TIMER;
    OCR1B = DCC_ZERO_BIT_PULSE_DURATION_16BIT_TIMER;
    //  OCR ## N ## A=DCC_ZERO_BIT_TOTAL_DURATION_## BC ##BIT_TIMER;                              /*   set OCRA for timer N to full cycle duration of DCC ZERO bit */ \
    //  OCR ## N ## B=DCC_ZERO_BIT_PULSE_DURATION_## BC ##BIT_TIMER;                              /*   set OCRB for timer N to half cycle duration of DCC ZERO bit */ \

  }
  priorityList->currentBit++;
}

LoconetMaster loconetMaster(20);
boolean shortActive = false;
unsigned long shortStartMillis;

void setup() {
  Serial.begin(115200);            // configure serial interface
  Serial.flush();
  Serial.println("DCCSpider...");

  dccBaseStation.begin(1);
  displayController.begin( 20, 2, "DCCSpider", "V0.00");
  displayController.setDisplayState(DisplayController::EnterDispatchAddress);
  loconetMaster.begin(46, &dccBaseStation);

  dccBaseStation.enableTrackPower();
}

void loop() {
  if ( shortActive && ( millis() > shortStartMillis + 100 )  ) {
    digitalWrite(DCC_POWER_ENABLE_PIN, HIGH);
  }
  if ( dccBaseStation.checkCurrentDraw() ) //short detected, power has been cut
  {
    shortActive = true;
    shortStartMillis = millis();
  }


  displayController.updateDisplay();
  loconetMaster.processReceivedMessages();
}
