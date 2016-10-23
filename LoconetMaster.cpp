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
#include "LoconetMaster.h"

LoconetMaster::LoconetMaster(byte slotCount)
{
  _messageNo = 0;
  _slotCount = slotCount;
  _slotData = new SlotData[_slotCount];
  _dispatchSlotNumber = 0;

  //Test Code
  _dispatchSlotNumber = 1;
  _slotData[0].locoAddress = 1101;
  _slotData[0].slotStatus = LOCO_IDLE | DEC_MODE_128;
}

void LoconetMaster::begin(byte rxPin, DCCBaseStation * dccBaseStation)
{
  LocoNet.init(rxPin);
  _dccBaseStation = dccBaseStation;
}

byte LoconetMaster::getLocoSlotNumber(int locoAddress)
{
  byte slotIndex = 0;
  for (slotIndex = 0;  slotIndex < _slotCount; slotIndex++)
  {
    if ((_slotData[slotIndex].slotStatus & LOCOSTAT_MASK) && (_slotData[slotIndex].locoAddress == locoAddress)) {
      return slotIndex + 1;
    }
  }
  return 0;
}

byte LoconetMaster::createLocoSlot(int locoAddress) {
  byte slotIndex = 0;

  for (slotIndex = 0;  slotIndex < _slotCount; slotIndex++)
  {
    if (!(_slotData[slotIndex].slotStatus & LOCOSTAT_MASK) ) {
      _slotData[slotIndex].locoAddress = locoAddress;
      _slotData[slotIndex].slotStatus = LOCO_IDLE | DEC_MODE_128;
      return slotIndex + 1;
    }
  }
  return 0;
}

void LoconetMaster::processReceivedMessages()
{
  lnMsg * receivedMessage;


  receivedMessage = LocoNet.receive() ;
  if ( receivedMessage ) {
    uint8_t msgLen = getLnMsgSize(receivedMessage);

    String logString("RX:");
    logString = String(_messageNo) + ' ' + logString;
    _messageNo++;
    for (uint8_t x = 0; x < msgLen; x++)
    {
      uint8_t val = receivedMessage->data[x];
      // Print a leading 0 if less than 16 to make 2 HEX digits
      if (val < 16)
      {
        logString += '0';
      }
      logString += String(val, HEX);

    }

    Serial.println(logString);

    /*for (byte lineNo = 0; logString.length() > 0; lineNo++) {
      lcd.setCursor(0, lineNo);
      lcd.print(logString.substring(0, 20));
      logString = logString.substring(20);
      }*/

    lnMsg replyMessage;

    if (receivedMessage->data[0] == OPC_LOCO_ADR )
    {
      int locoAddress = receivedMessage->data[1] << 8 | receivedMessage->data[2];
      byte slotNumber = getLocoSlotNumber(locoAddress);
      if (!slotNumber) {
        slotNumber = createLocoSlot(locoAddress);
      }

      if (!slotNumber) {
        LocoNet.sendLongAck(0);
      }
      else {
        sendSlotReadData(slotNumber);
      }
    }
    else if (receivedMessage->data[0] ==  OPC_MOVE_SLOTS )
    {
      byte slotNumber = receivedMessage->data[1];
      if (!slotNumber) {
        slotNumber = _dispatchSlotNumber;
        _dispatchSlotNumber = 0;
      }
      if (!slotNumber) {
        LocoNet.sendLongAck(0);
      }
      else
      {
        _slotData[slotNumber - 1].slotStatus |= LOCO_IN_USE;
        sendSlotReadData(slotNumber);
      }
    }
    else if (receivedMessage->data[0] ==  OPC_WR_SL_DATA )
    {
      byte slotNumber = receivedMessage->data[2];
      _slotData[slotNumber - 1].deviceID = (receivedMessage->data[11] << 8) | receivedMessage->data[12];
      LocoNet.send(OPC_LONG_ACK, 0x6F, 0x7F);
    }
    else if (receivedMessage->data[0] ==  OPC_LOCO_SPD  )
    {
      byte slotNumber = receivedMessage->data[1];
      _slotData[slotNumber - 1].locoSpeed = receivedMessage->data[2];
      _dccBaseStation->getRegisterList()->setThrottle(3,_slotData[slotNumber - 1].locoAddress,_slotData[slotNumber - 1].locoSpeed,_slotData[slotNumber - 1].directionF0F4 & DIRF_DIR);
    }
    else if (receivedMessage->data[0] == OPC_LOCO_DIRF )
    {
      byte slotNumber = receivedMessage->data[1];
      _slotData[slotNumber - 1].directionF0F4 = receivedMessage->data[2];
    }
  }
}

void LoconetMaster::sendSlotReadData(byte slotNumber)
{
  if (slotNumber) {
    lnMsg replyMessage;
    replyMessage.data[ 0 ] = OPC_SL_RD_DATA;
    replyMessage.data[ 1 ] = 0x0E;
    replyMessage.data[ 2 ] = slotNumber;//SLOT#
    replyMessage.data[ 3 ] = _slotData[slotNumber - 1].slotStatus;
    replyMessage.data[ 4 ] = _slotData[slotNumber - 1].locoAddress & 0x00FF; //ADR Address lsb
    replyMessage.data[ 5 ] = _slotData[slotNumber - 1].locoSpeed; //SPD Speed
    replyMessage.data[ 6 ] = _slotData[slotNumber - 1].directionF0F4; //DIRF Direction F0-F4
    replyMessage.data[ 7 ] = 4;//1; //TRK Track Status power is on
    replyMessage.data[ 8 ] = 0; //SS2 Status 2
    replyMessage.data[ 9 ] = _slotData[slotNumber - 1].locoAddress >> 8; //0x04; //ADR2 Address msb
    replyMessage.data[ 10 ] = 0; //SND
    replyMessage.data[ 11 ] = _slotData[slotNumber - 1].deviceID >> 8;
    replyMessage.data[ 12 ] = _slotData[slotNumber - 1].deviceID & 0x00FF;

    LocoNet.send( &replyMessage );
  }
  else
  {
    LocoNet.sendLongAck(0);
  }
}

