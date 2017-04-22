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
#include "DisplayController.h"

DisplayController::DisplayController(byte rsPin, byte rwPin, byte enablePin,
                                     byte d0Pin, byte d1Pin, byte d2Pin, byte d3Pin,
                                     byte d4Pin, byte d5Pin, byte d6Pin, byte d7Pin)
{
  _display = new LiquidCrystal(rsPin, rwPin, enablePin,
                               d0Pin, d1Pin, d2Pin, d3Pin,
                               d4Pin, d5Pin, d6Pin, d7Pin);

}

void DisplayController::begin(byte columnCount, byte rowCount, String projectName, String versionString)
{
  _columnCount = columnCount;
  _display->begin(columnCount, rowCount);
  _display->setCursor(0, 0);
  _display->print (projectName);
  _display->setCursor(0, 1);
  _display->print (versionString);
  _scrollPosition = 0;
  _lastUpdateMillis = millis();
}

void DisplayController::end()
{
  delete _display;
}

void DisplayController::setDisplayState(DisplayState state)
{
  _display->clear();
  _state = state;

  //setup static part of display
  switch (_state)
  {
    case EnterDispatchAddress:
      _staticString = "Loco Address:";
      _inputString = "";
      _scrollString = "Press ENT to dispatch address.";
      _displayCursor = true;
      break;
    case TakeoverDispatchedAddress:
      _staticString = "Disp. Address:";
      //_inputString for address remains unchanged
      _scrollString = "Press STOP+SHIFT on Fred to take over.";
      _displayCursor = false;
      break;
    case DisplaySystemStats:
      _staticString = "Free Memory: " + freeRam();
      _inputString = "";
      _scrollString = __DATE__ " " __TIME__;
      _displayCursor = false;
      break;
  }
  _display->setCursor(0, 0);
  _display->print(_staticString);


  //Add extra space if we actually need to scroll
  if (_scrollString.length() > _columnCount )
  {
    _scrollString += " ";
  }
  _display->setCursor(0, 1);
  _display->print(_scrollString.substring(0, _columnCount));

  if (_displayCursor) {
    _display->cursor();
  }
  else {
    _display->noCursor();
  }

  _lastUpdateMillis = 0; //force display update
  _scrollPosition = 0; //reset scrolling
  updateDisplay();
}

void DisplayController::updateDisplay()
{
  if ((_lastUpdateMillis + 1000) < millis())
  {
    _lastUpdateMillis = millis();


    _display->setCursor(0, 1);
    if (_scrollString.length() <= _columnCount)
    {
      _display->print(_scrollString);
    }
    else {
      String printString;
      if (_scrollPosition >= _scrollString.length())
      {
        _scrollPosition = 0;
      }

      if ((_scrollPosition + _columnCount) > _scrollString.length())
      {
        printString = _scrollString.substring(_scrollPosition, _scrollString.length())
                      + _scrollString.substring(0, _columnCount - (_scrollString.length() - _scrollPosition));
      }
      else
      {
        printString = _scrollString.substring(_scrollPosition, _scrollPosition + _columnCount);
      }

      _display->print(printString);
      _scrollPosition++;
    }

    if (_displayCursor)
    {
      _display->setCursor(_staticString.length() + _inputString.length(), 0);
      _display->print(_inputString);
    }

  }
}

void DisplayController::setInputString(String inputString) {
  _inputString = inputString;
}

int DisplayController::freeRam ()
{
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}
