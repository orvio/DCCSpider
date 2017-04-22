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

#include "UIController.h"

UIController::UIController(DisplayController *displayController, Keypad *keypad) :
  _displayController (*displayController),
  _keypad(*keypad) {
}

void UIController::begin() {
  _displayController.begin( 20, 2, "DCCSpider", "V0.00");
  _displayController.setDisplayState(DisplayController::EnterDispatchAddress);
}

void UIController::updateUI() {
  char key = _keypad.getKey();

  if (key) {
    if (isDigit(key)) {
      _addressString += key;
      if (_addressString.length() > 5) { //DCC addresses cannot be longer than 5 digits
        _addressString.remove(0, 1);
      }
      if (_addressString.toInt() > 10240) { //DCC addresses cannot be higher than 10240
        _addressString = String(10240);
      }
      _displayController.setInputString(_addressString);
    }
    else if (key == '<') {
      _addressString.remove(_addressString.length() - 1, 1);
    }
    else if(key == 'C') {
      _addressString = "";
    }
  }

  _displayController.updateDisplay();
}

