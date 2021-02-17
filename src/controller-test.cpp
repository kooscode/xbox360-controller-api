/* XBOX 360 Wireless Controller API [SAMPLE APP]

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

#include <iostream>
#include <thread>
#include <chrono>

#include "XBOX360.hpp"

int main(int argc, char** argv) 
{  
  XKCTRL::XBOX360 x360;
  std::cout << "Use Triggers for Rumble and press XBOX button to quit" << std::endl;

  int32_t idx = 0;
  XKCTRL::CONTROLLER_STATE ctrl;
  while(!ctrl.XBOX)
  {
    // wait for 1sec for any valid changes to controller..
    if (!x360.GetWaitControllerState(idx, ctrl, 250))
    {
      std::cout << "\r[IDLE]" << std::flush;
    }
    else
    {
      std::cout << "\r[ OK ]";

      //Rumble with triggers
      x360.SetRumble(idx, ctrl.LTRIG, ctrl.RTRIG);
    }
    
    // Update all button states
    std::cout << "  UP:" <<  ctrl.UP
              << ", DWN:" <<  ctrl.DOWN
              << ", LFT:" <<  ctrl.LEFT
              << ", RHT:" <<  ctrl.RIGHT
              << ", STR:" <<  ctrl.START
              << ", BCK:" <<  ctrl.BACK
              << ", LH:" <<  ctrl.LH
              << ", RH:" <<  ctrl.RH
              << ", LB:" <<  ctrl.LB
              << ", RB:" <<  ctrl.RB
              << ", XBX:" <<  ctrl.XBOX
              << ", A:" <<  ctrl.A
              << ", B:" <<  ctrl.B
              << ", X:" <<  ctrl.X
              << ", Y:" <<  ctrl.Y
              << ", LT:" << (int)ctrl.LTRIG 
              << ", RT:" << (int)ctrl.RTRIG
              << ", LSX:" << ctrl.LSTICK_X
              << ", LSY:" << ctrl.LSTICK_Y
              << ", RSX:" << ctrl.RSTICK_X
              << ", RSY:" << ctrl.RSTICK_Y
              << "      " << std::flush;

  } //end while true
  x360.SetRumble(idx, 0x00, 0x00);

  std::cout << std::endl;
  return 0;
}
