/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright (C) 2016 Martin Mueller mm@nohost.de                        *
 * This file is part of DCCSpider                                        *
 *                                                                       *
 * DCCSpider is free software: you can redistribute it and/or modify     *
 * it under the terms of the GNU General Public License as published by  *
 * the Free Software Foundation, either version 3 of the License, or     *
 * (at your option) any later version.                                   *
 *                                                                       *
 * DCCSpider is distributed in the hope that it will be useful,          *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 * GNU General Public License for more details.                          *
 *                                                                       *
 * You should have received a copy of the GNU General Public License     *
 * along with DCCSpider.  If not, see <http://www.gnu.org/licenses/>.    *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#include "Arduino.h"
#include <LocoNet.h>
#include "DCCBaseStation.h"

#ifndef LoconetMaster_h
#define LoconetMaster_h

class LoconetMaster
{
  public:
    LoconetMaster(byte slotCount);
    void begin(byte rxPin, DCCBaseStation * dccBaseStation);
    void processReceivedMessages();

  private:
    typedef struct
    {
      byte slotStatus = LOCO_FREE;
      int locoAddress = 0;
      byte locoSpeed = 0;
      byte directionF0F4 = 0;
      byte slotSound = 0;
      int deviceID = 0;
    } SlotData;

    byte getLocoSlotNumber(int locoAddress);
    byte createLocoSlot(int locoAddress);
    void sendSlotReadData(byte slotNumber);
    byte _dispatchSlotNumber;
    byte _slotCount;
    SlotData * _slotData;
    int _messageNo;
    DCCBaseStation * _dccBaseStation;

};
#endif
