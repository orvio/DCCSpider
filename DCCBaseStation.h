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

#include "Arduino.h"
#include "PacketRegister.h"
#include "CurrentMonitor.h"

#ifndef DCCBaseStation_h
#define DCCBaseStation_h

class DCCBaseStation {
  public:
    DCCBaseStation(byte dccSignalPin, byte enablePin, byte currentSensePin, byte registerCount);
    void begin(byte timerNo);
    volatile RegisterList * getRegisterList() const;
    void enableTrackPower();
    boolean checkCurrentDraw();
    

  private:
    volatile RegisterList * _registerList;
    CurrentMonitor * _currentMonitor;
    byte _enablePin;
    byte _dccSignalPin;
    byte _timerNo;
};

#define DCC_ZERO_BIT_TOTAL_DURATION_16BIT_TIMER 3199
#define DCC_ZERO_BIT_PULSE_DURATION_16BIT_TIMER 1599

#define DCC_ONE_BIT_TOTAL_DURATION_16BIT_TIMER 1855
#define DCC_ONE_BIT_PULSE_DURATION_16BIT_TIMER 927

//Interrupt generation makro from DCC++
///////////////////////////////////////////////////////////////////////////////
// DEFINE THE INTERRUPT LOGIC THAT GENERATES THE DCC SIGNAL
///////////////////////////////////////////////////////////////////////////////

// The code below will be called every time an interrupt is triggered on OCNB, where N can be 0 or 1.
// It is designed to read the current bit of the current register packet and
// updates the OCNA and OCNB counters of Timer-N to values that will either produce
// a long (200 microsecond) pulse, or a short (116 microsecond) pulse, which respectively represent
// DCC ZERO and DCC ONE bits.

// These are hardware-driven interrupts that will be called automatically when triggered regardless of what
// DCC++ BASE STATION was otherwise processing.  But once inside the interrupt, all other interrupt routines are temporarily diabled.
// Since a short pulse only lasts for 116 microseconds, and there are TWO separate interrupts
// (one for Main Track Registers and one for the Program Track Registers), the interrupt code must complete
// in much less than 58 microsends, otherwise there would be no time for the rest of the program to run.  Worse, if the logic
// of the interrupt code ever caused it to run longer than 58 microsends, an interrupt trigger would be missed, the OCNA and OCNB
// registers would not be updated, and the net effect would be a DCC signal that keeps sending the same DCC bit repeatedly until the
// interrupt code completes and can be called again.

// A significant portion of this entire program is designed to do as much of the heavy processing of creating a properly-formed
// DCC bit stream upfront, so that the interrupt code below can be as simple and efficient as possible.

// Note that we need to create two very similar copies of the code --- one for the Main Track OC1B interrupt and one for the
// Programming Track OCOB interrupt.  But rather than create a generic function that incurrs additional overhead, we create a macro
// that can be invoked with proper paramters for each interrupt.  This slightly increases the size of the code base by duplicating
// some of the logic for each interrupt, but saves additional time.

// As structured, the interrupt code below completes at an average of just under 6 microseconds with a worse-case of just under 11 microseconds
// when a new register is loaded and the logic needs to switch active register packet pointers.

// THE INTERRUPT CODE MACRO:  R=REGISTER LIST (mainRegs or progRegs), and N=TIMER (0 or 1)
#define DCC_SIGNAL(R,N,BC) \
  if(R.currentBit==R.currentReg->activePacket->nBits){    /* IF no more bits in this DCC Packet */ \
    R.currentBit=0;                                       /*   reset current bit pointer and determine which Register and Packet to process next--- */ \
    if(R.nRepeat>0 && R.currentReg==R.reg){               /*   IF current Register is first Register AND should be repeated */ \
      R.nRepeat--;                                        /*     decrement repeat count; result is this same Packet will be repeated */ \
    } else if(R.nextReg!=NULL){                           /*   ELSE IF another Register has been updated */ \
      R.currentReg=R.nextReg;                             /*     update currentReg to nextReg */ \
      R.nextReg=NULL;                                     /*     reset nextReg to NULL */ \
      R.tempPacket=R.currentReg->activePacket;            /*     flip active and update Packets */ \
      R.currentReg->activePacket=R.currentReg->updatePacket; \
      R.currentReg->updatePacket=R.tempPacket; \
    } else{                                               /*   ELSE simply move to next Register */ \
      if(R.currentReg==R.maxLoadedReg)                    /*     BUT IF this is last Register loaded */ \
        R.currentReg=R.reg;                               /*       first reset currentReg to base Register, THEN */ \
      R.currentReg++;                                     /*     increment current Register (note this logic causes Register[0] to be skipped when simply cycling through all Registers) */ \
    }                                                     /*   END-ELSE */ \
  }                                                       /* END-IF: currentReg, activePacket, and currentBit should now be properly set to point to next DCC bit */ \
  \
  if(R.currentReg->activePacket->buf[R.currentBit/8] & R.bitMask[R.currentBit%8]){     /* IF bit is a ONE */ \
    OCR ## N ## A=DCC_ONE_BIT_TOTAL_DURATION_## BC ##BIT_TIMER;                               /*   set OCRA for timer N to full cycle duration of DCC ONE bit */ \
    OCR ## N ## B=DCC_ONE_BIT_PULSE_DURATION_## BC ##BIT_TIMER;                               /*   set OCRB for timer N to half cycle duration of DCC ONE but */ \
  } else{                                                                              /* ELSE it is a ZERO */ \
    OCR ## N ## A=DCC_ZERO_BIT_TOTAL_DURATION_## BC ##BIT_TIMER;                              /*   set OCRA for timer N to full cycle duration of DCC ZERO bit */ \
    OCR ## N ## B=DCC_ZERO_BIT_PULSE_DURATION_## BC ##BIT_TIMER;                              /*   set OCRB for timer N to half cycle duration of DCC ZERO bit */ \
  }                                                                                    /* END-ELSE */ \
  \
  R.currentBit++;                                         /* point to next bit in current Packet */

#endif
