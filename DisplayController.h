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
#include <LiquidCrystal.h>

#ifndef DisplayController_h
#define DisplayController_h
class DisplayController
{
  public:
    typedef enum
    {
      EnterDispatchAddress,
      TakeoverDispatchedAddress,
      DisplaySystemStats,
    } DisplayState ;
    DisplayController(byte rsPin, byte rwPin, byte enablePin,
                      byte d0Pin, byte d1Pin, byte d2Pin, byte d3Pin,
                      byte d4Pin, byte d5Pin, byte d6Pin, byte d7Pin);
    void begin(byte columnCount, byte rowCount, String projectName, String versionString);

    void end();

    void setDisplayState(DisplayState state);

    void updateDisplay();

    void setInputString(String inputString);

  private:
    LiquidCrystal * _display;
    DisplayState _state;
    byte _scrollPosition;
    unsigned long _lastUpdateMillis;
    String _scrollString;
    String _staticString;
    String _inputString;
    byte _columnCount;
    boolean _displayCursor;

    int freeRam();
};
#endif
