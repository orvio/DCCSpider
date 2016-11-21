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
  packetList->firstPacket = 0;
  packetList->lastPacket = 0;

  packetList->newOrUpdatedPackets = new DCCBufferPacket*[packetCount];
  for (int i = 0; i < packetCount; i++)
  {
    packetList->newOrUpdatedPackets[i] = 0;
  }
  return packetList;
}

DCCBaseStation::DCCPriorityList::DCCPriorityList(byte packetCount)
{
  this->packetCount = packetCount;
  packets = new DCCBaseStation::DCCBufferPacket[packetCount];
  for ( byte i = 0; i < packetCount; i++)
  {
    packets[i].locoAddress = 0;
    packets[i].instructionByte = 0;
    packets[i].lastUpdateMillis = millis();
    packets[i].previousPacket = NULL;
    packets[i].nextPacket = NULL;
  }
  currentPacket = NULL;
  currentCyclePacket = NULL;
  currentList = 0;
  currentBit = 0;

  _packetLists = new DCCBaseStation::DCCPacketList*[PRIORITY_LIST_COUNT];
  for (byte i = 0; i < PRIORITY_LIST_COUNT; i++) {
    _packetLists[i] = initPacketList(packetCount);
  }
}


DCCBaseStation::DCCBaseStation(byte dccSignalPin, byte enablePin, byte currentSensePin, byte registerCount):
  _priorityList(new volatile DCCPriorityList(registerCount))
{
  _registerList = new volatile RegisterList(registerCount);
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

volatile DCCBaseStation::DCCPriorityList * const DCCBaseStation::getPriorityList() const
{
  return _priorityList;
}

void DCCBaseStation::begin(byte timerNo)
{
  //setup IDLE packet;
  /*_priorityList->packets[0].locoAddress  = 0xFF;
    _priorityList->packets[0].instructionByte = 0x00;
    const byte byteCount = 2;
    byte bytes[byteCount];
    bytes[0] = _priorityList->packets[0].locoAddress;
    bytes[1] = _priorityList->packets[0].instructionByte;
    movePacket(&(_priorityList->packets[0]), NULL, _priorityList->packetLists[LOW_PRIORITY_LIST]);

    setupPacket(&(_priorityList->packets[0]), bytes, byteCount);
    markPacketUpdated(&(_priorityList->packets[0]), _priorityList->packetLists[LOW_PRIORITY_LIST]);*/
  setLocoSpeed(1101, 10, FORWARD);

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

void DCCBaseStation::setLocoSpeed(unsigned int locoAddress, byte locoSpeed, DCCDirection locoDirection) {
  //find out wheter a loco speed packet is already in the lists
  //speed stuff is either high or critical priority
  DCCBufferPacket * currentPacket = NULL;
  DCCPacketList * packetList = NULL;

  //check high prioriy list
  for (currentPacket = _priorityList->_packetLists[HIGH_PRIORITY_LIST]->firstPacket; currentPacket != NULL; currentPacket = currentPacket->nextPacket) {
    if (currentPacket->locoAddress == locoAddress) { //hit
      packetList = _priorityList->_packetLists[HIGH_PRIORITY_LIST];
      break;
    }
  }

  //check critical priority list
  if (currentPacket == NULL) {
    for (currentPacket = _priorityList->_packetLists[CRITICAL_PRIORITY_LIST]->firstPacket; currentPacket != NULL; currentPacket = currentPacket->nextPacket) {
      if (currentPacket->locoAddress == locoAddress) { //hit
        packetList = _priorityList->_packetLists[CRITICAL_PRIORITY_LIST];
        break;
      }
    }
  }

  //create packet if still not found
  if (currentPacket == NULL) {
    //find first available spot
    for ( int i = 0; i < _priorityList->packetCount; i++ ) {
      if ( _priorityList->packets[i].locoAddress == 0x00 || _priorityList->packets[i].locoAddress == 0xFF) {
        Serial.print("Spawning packet in slot: ");
        Serial.println(i, DEC);
        currentPacket =  &(_priorityList->packets[i]);
        currentPacket->locoAddress = locoAddress;
        currentPacket->instructionByte = 0;
        break;
      }
    }

    //new speed packets go into high priority list by default
    packetList = _priorityList->_packetLists[HIGH_PRIORITY_LIST];
    movePacket(currentPacket, NULL, packetList);
  }

  //compare speeds to determine priority change
  if ( currentPacket->instructionByte != 0 ) {
    byte currentSpeed = currentPacket->instructionByte & 0x1F;
    //packet goes into critical list if speed is lower or (emergency) stop is instructed
    if ((locoSpeed < currentSpeed) || locoSpeed == 0 || locoSpeed == 1) {
      movePacket(currentPacket, packetList, _priorityList->_packetLists[CRITICAL_PRIORITY_LIST]);
      packetList = _priorityList->_packetLists[CRITICAL_PRIORITY_LIST];
    }
    else { //high priority otherwise
      movePacket(currentPacket, packetList, _priorityList->_packetLists[HIGH_PRIORITY_LIST]);
      packetList = _priorityList->_packetLists[HIGH_PRIORITY_LIST];
    }
  }

  //setup packet with new data
  const byte byteCount = 4;
  byte bytes[byteCount];
  bytes[0] = highByte(locoAddress) | 0xC0;
  bytes[1] = lowByte(locoAddress);
  bytes[2] = 0x3F;
  bytes[3] = locoSpeed;
  if ( locoDirection == FORWARD ) {
    currentPacket->instructionByte = SPEED_FORWARD & locoSpeed;
    bytes[3] |= 0X80; //set forward bit
  }
  else {
    currentPacket->instructionByte = SPEED_REVERSE & locoSpeed;
  }
  setupPacket(currentPacket, bytes, byteCount);

  markPacketUpdated(currentPacket, packetList);
}

void DCCBaseStation::setupPacket(DCCBufferPacket * packet, byte * bytes, byte byteCount) {
  Serial.print("Setting up packet data. Data bytes: ");
  for ( int i = 0; i < byteCount; i++) {
    Serial.print(bytes[i], HEX);
    Serial.print(" ");
  }

  //Serial.println(packetList->newOrUpdatedCount, DEC);
  packet->rawPackets[0].usedBits = 2 * 8 + byteCount * 9 + 1 + 8; //16 bit preamble, data bits + 0s, checksum
  Serial.print("Total packet bit count: ");
  Serial.println(packet->rawPackets[0].usedBits, DEC);
  packet->rawPackets[0].bytes[0] = 0xFF; //preamble bits 0-7
  packet->rawPackets[0].bytes[1] = 0xFF; //preamble bits 8-15
  //the DCC standard demands at least 14 ones to be sent; we got 16

  //setup bits for data bytes
  byte checkSum  = 0;
  for ( byte i = 0; i <= byteCount; i++) {
    if (i >= byteCount) { //last iteration, we are after the end of the bytes array
      packet->rawPackets[0].bytes[i + 2] = (bytes[i - 1] << (8 - i));
    }
    else if ( i > 0 ) {
      packet->rawPackets[0].bytes[i + 2] = (bytes[i - 1] << (8 - i)) | (bytes[i] >> (i + 1));
      checkSum ^= bytes[i];
    }
    else { //first iteration
      packet->rawPackets[0].bytes[i + 2] = (bytes[i] >> (i + 1));
      checkSum ^= bytes[i];
    }
  }

  Serial.print("Packet check sum: ");
  Serial.println(checkSum, HEX);

  //setup bits for checksum
  packet->rawPackets[0].bytes[byteCount + 2] |= checkSum >> byteCount + 1;
  packet->rawPackets[0].bytes[byteCount + 2 + 1] = checkSum << 8 - (byteCount + 1);

  Serial.print("Resulting bit stream: ");
  for (byte currentBit = 0; currentBit < packet->rawPackets[0].usedBits; currentBit++) {
    //Serial.println(0x80 >> (currentBit % 8),HEX);
    Serial.print((packet->rawPackets[0].bytes[currentBit / 8 ] & (0x80 >> (currentBit % 8) ) ) != 0, BIN);
    if (!((currentBit + 1) % 8)) {
      Serial.print(" ");
    }
  }
  Serial.println();
}

void DCCBaseStation::movePacket(DCCBufferPacket * movedPacket, DCCPacketList * fromList, DCCPacketList * toList) {
  //if lists are identical we have nothing to do
  if (fromList == toList) {
    Serial.println("Not moving packet");
    return;
  }

  if (fromList) {
    Serial.println("Detaching packet from old list");
    //detach from current list first
    if ( fromList->firstPacket == movedPacket) {
      fromList->firstPacket = movedPacket->nextPacket;
    }
    //this might happen if the packet isthe only packet in the list
    if ( fromList->lastPacket == movedPacket) {
      fromList->lastPacket == movedPacket->previousPacket;
    }
  }

  Serial.println("Attaching Packet to new list");
  movedPacket->nextPacket = 0;
  movedPacket->previousPacket = 0;

  //the packet is the first packet in the new list
  if (!toList->firstPacket) {
    toList->firstPacket = movedPacket;
    toList->lastPacket = movedPacket;
  }
  else { //the list has at least one other packet
    toList->lastPacket->nextPacket = movedPacket;
    movedPacket->previousPacket = toList->lastPacket;
    toList->lastPacket = movedPacket;
  }
}

void DCCBaseStation::markPacketUpdated(DCCBufferPacket * currentPacket, DCCPacketList * packetList) {
  //check if packet is already marked
  for (byte i = packetList->firstNewOrUpdatedIndex; i <= packetList->firstNewOrUpdatedIndex + packetList->newOrUpdatedCount; i++) {
    if (packetList->newOrUpdatedPackets[i] == currentPacket ) {
      Serial.println("Packet already marked as updated");
      return;
    }
  }

  //if we arrive here, the packet has not been marked as updated yet
  if (packetList->newOrUpdatedCount == 0) {
    packetList->firstNewOrUpdatedIndex = 0;
  }

  packetList->newOrUpdatedCount++;
  packetList->newOrUpdatedPackets[packetList->firstNewOrUpdatedIndex + packetList->newOrUpdatedCount] = currentPacket;
  Serial.print("Packet marked as updated. Total updated packets in list: ");
  Serial.println(packetList->newOrUpdatedCount, DEC);
}

