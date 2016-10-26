/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
   Copyright (C) 2016 Martin Mueller mm@nohost.de
   This file is part of DCCSpider
   Some parts are taken from DCC++ BASE STATION
   COPYRIGHT (c) 2013-2016 Gregg E. Berman
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
#include "DCCBaseStation.h"

DCCBaseStation::DCCPacketList * DCCBaseStation::DCCPriorityList::initPacketList(byte packetCount)
{
  DCCPacketList * packetList =  new DCCPacketList;
  packetList->firstPacket = NULL;
  packetList->lastPacket = NULL;
  packetList->newOrUpdatedPackets = new DCCBufferPacket*[packetCount];
  for (int i = 0; i < packetCount; i++)
  {
    packetList->newOrUpdatedPackets[i] = NULL;
  }
  return packetList;
}
DCCBaseStation::DCCPriorityList::DCCPriorityList(byte packetCount)
{
  packets = new DCCBaseStation::DCCBufferPacket[packetCount];
  for ( int i = 0; i < packetCount; i++)
  {
    packets[i].locoAddress = 0;
    packets[i].instructionByte = 0;
    packets[i].lastUpdateMillis = millis();
    packets[i].nextPacket = NULL;
  }
  currentPacket = NULL;
  currentList = NULL;
  currentBit = 0;
  lowPriorityList = initPacketList(packetCount);
  highPriorityList = initPacketList(packetCount);
  criticalPriorityList = initPacketList(packetCount);
}


DCCBaseStation::DCCBaseStation(byte dccSignalPin, byte enablePin, byte currentSensePin, byte registerCount)
{
  _registerList = new volatile RegisterList(registerCount);
  _priorityList = new volatile DCCPriorityList(registerCount);
  _currentMonitor = new CurrentMonitor(currentSensePin, enablePin);
  _enablePin = enablePin;
  _dccSignalPin = dccSignalPin;
}

void DCCBaseStation::enableTrackPower()
{
  digitalWrite(_enablePin, HIGH);
}

volatile RegisterList * DCCBaseStation::getRegisterList() const
{
  return _registerList;
}

void DCCBaseStation::begin(byte timerNo)
{
  switch (timerNo)
  {
    case 1:
      pinMode(_dccSignalPin, OUTPUT);      // THIS ARDUINO OUPUT PIN MUST BE PHYSICALLY CONNECTED TO THE PIN FOR DIRECTION-A OF MOTOR CHANNEL-A

      bitSet(TCCR1A, WGM10);    // set Timer 1 to FAST PWM, with TOP=OCR1A
      bitSet(TCCR1A, WGM11);
      bitSet(TCCR1B, WGM12);
      bitSet(TCCR1B, WGM13);

      bitSet(TCCR1A, COM1B1);   // set Timer 1, OC1B (pin 10/UNO, pin 12/MEGA) to inverting toggle (actual direction is arbitrary)
      bitSet(TCCR1A, COM1B0);

      bitClear(TCCR1B, CS12);   // set Timer 1 prescale=1
      bitClear(TCCR1B, CS11);
      bitSet(TCCR1B, CS10);

      OCR1A = DCC_ONE_BIT_TOTAL_DURATION_16BIT_TIMER;
      OCR1B = DCC_ONE_BIT_PULSE_DURATION_16BIT_TIMER;

      pinMode(_enablePin, OUTPUT);  // master enable for motor channel A

      _registerList->loadPacket(1, RegisterList::idlePacket, 2, 0); // load idle packet into register 1

      bitSet(TIMSK1, OCIE1B);   // enable interrupt vector for Timer 1 Output Compare B Match (OCR1B)
      break;
  }
}

boolean DCCBaseStation::checkCurrentDraw() {
  return _currentMonitor->check();
}

void DCCBaseStation::setLocoSpeed(unsigned int locoAddress, byte locoSpeed) {
  //find out wheter a loco speed packet is already in the lists
  //speed stuff is either high or critical priority
  DCCBufferPacket * currentPacket = NULL;

  //check high prioriy list
  for (currentPacket = _priorityList->highPriorityList->firstPacket; currentPacket != NULL; currentPacket = currentPacket->nextPacket) {
    if (currentPacket->locoAddress == locoAddress) { //hit
      break;
    }
  }

  //check critical priority list
  if (currentPacket == NULL) {
    for (currentPacket = _priorityList->criticalPriorityList->firstPacket; currentPacket != NULL; currentPacket = currentPacket->nextPacket) {
      if (currentPacket->locoAddress == locoAddress) { //hit
        break;
      }
    }
  }

  //create packet if still not found
  if (currentPacket == NULL) {
    //TODO need efficient way to determine first available slot in packet space

    if (_priorityList->highPriorityList->firstPacket == NULL) {
      _priorityList->highPriorityList->firstPacket = currentPacket;
      _priorityList->highPriorityList->lastPacket = currentPacket;
    }
    else {
      _priorityList->highPriorityList->lastPacket->nextPacket = currentPacket;
      _priorityList->highPriorityList->lastPacket = currentPacket;
    }
  }
}

