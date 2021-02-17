/* XBOX 360 Wireless Controller API

Copyright (C) 2021 Koos du Preez (kdupreez@hotmail.com)

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef _XBOX360_DEFINES_
#define _XBOX360_DEFINES_

namespace XKCTRL
{
  enum LED_SETTING
  {
    OFF_ALL,
    BLINK_ALL_NORM,
    BLINK_1_ON,
    BLINK_2_ON,
    BLINK_3_ON,
    BLINK_4_ON,
    ON_1,
    ON_2,
    ON_3,
    ON_4,
    ROTATE,
    BLINK,
    BLINK_SLOW,
    FAN,
    BLINK_SLOW_ALL,
    BLINK_ONCE
  };

  enum BUTTON_MASK
  {
    MASK_DPAD_UP =    0x0001,
    MASK_DPAD_DOWN =  0x0002,
    MASK_DPAD_LEFT =  0x0004,
    MASK_DPAD_RIGHT = 0x0008,
    MASK_BTN_START =  0x0010,
    MASK_BTN_BACK =   0x0020,
    MASK_BTN_LH =     0x0040, 
    MASK_BTN_RH =     0x0080, 
    MASK_BTN_LB =     0x0100,
    MASK_BTN_RB =     0x0200,
    MASK_BTN_XBOX =   0x0400,
    MASK_BTN_A =      0x1000,
    MASK_BTN_B =      0x2000,
    MASK_BTN_X =      0x4000,
    MASK_BTN_Y =      0x8000
  };

  struct CONTROLLER_LAYOUT
  {
    uint16_t BUTTONS;
    uint8_t LTRIG, RTRIG;
    int16_t LSTICK_X, LSTICK_Y, RSTICK_X, RSTICK_Y;
  };

  struct CONTROLLER_STATE
  {
    // DPad
    bool UP, DOWN, RIGHT, LEFT;
    // Buttons
    bool START, BACK, LH, RH, LB, RB, XBOX, A, B, X, Y;
    // Analog Controls
    uint8_t LTRIG, RTRIG;
    int16_t LSTICK_X, LSTICK_Y, RSTICK_X, RSTICK_Y;    
    //Is connected
    bool CONNECTED;
  };

}

#endif //_XBOX360_DEFINES_