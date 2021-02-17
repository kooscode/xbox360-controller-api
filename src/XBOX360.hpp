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

#ifndef _XBOX360_
#define _XBOX360_

#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <mutex> 
#include <condition_variable>

// requires "libusb-1.0-0-dev"
// link against "usb-1.0"
#include <libusb-1.0/libusb.h>

#include "XBOX360Defines.hpp"

#define MAX_CONTROLLERS 4
#define MAX_USB_INBUFF 32
#define MAX_USB_OUTBUFF 12
#define MAX_USB_TIMEOUT 50

namespace XKCTRL
{
  class XBOX360
  {
    public:
      XBOX360();
      ~XBOX360();

      void SetLED(const int32_t ControllerIndex, const LED_SETTING LEDSetting);
      void SetRumble(const int32_t ControllerIndex, const uint8_t BigWeight, const uint8_t SmallWeight);
      void SetRumbleTimed(const int32_t ControllerIndex, const uint8_t BigWeight, const uint8_t SmallWeight, const uint32_t RumbleTimeMS);
      void GetControllerState(const int32_t ControllerIndex, CONTROLLER_STATE& ControllerState);
      bool GetWaitControllerState(const int32_t ControllerIndex, CONTROLLER_STATE& ControllerState, uint32_t TimeoutMS);

    private:
      enum USBReportType
      {
          DATA = 0x00,
          CONNECTION = 0x08
      };

      // XBOX 360 Wireless USB Device
      const uint16_t USBVendorID_ = 0x045E;
      const uint16_t USBProductID_ = 0x02A9;
      libusb_context* USBContext_ = nullptr;

      // Device Descriptor for XBOX360 wireless device
      libusb_device_handle* USBDeviceHandle_ = nullptr;

      // XBOX 360 Wireless has only 1 Configuratin Descriptor with
      // 8 Interface Descriptors, we are interrested in 0,2,4,6
      // which represents conected controllers 1,2,3 and 4
      int32_t USBInterfaces_[MAX_CONTROLLERS] = {0,2,4,6};

      // Each Interface Descriptor has 2 Endpoint Descriptors
      // one for data transfer In and another for Out
      int32_t USBEndpointsIn_[MAX_CONTROLLERS] =  {0x81, 0x83, 0x85, 0x87};
      int32_t USBEndpointsOut_[MAX_CONTROLLERS] = {0x01, 0x03, 0x05, 0x07};
      
      // USB Input buffer for each controller
      uint8_t USBDataIn_[MAX_CONTROLLERS][MAX_USB_INBUFF];
      uint8_t USBDataOut_[MAX_CONTROLLERS][MAX_USB_OUTBUFF];

      // Thread for continious controller polling
      std::thread USBDeviceThread_;
      bool USBDeviceThreadRunning_;

      //Protection of shared resources is done via mutex
      std::mutex mutex_;
      
      //shared object that holds current state for all controllers.
      CONTROLLER_STATE ControllerStates_[MAX_CONTROLLERS];

      //notifications for Controllers state change
      std::condition_variable ControllersNotify_[MAX_CONTROLLERS];

      void    USBDeviceThread();
      void    USBDeviceInit();
      int32_t USBRXAsync(const int32_t ControllerIndex);
      bool    USBTXSync(const int32_t ControllerIndex);
      void    ControllerDataProcessing(const int32_t ControllerIndex);
      bool    ControllerInit(const int32_t ControllerIndex);
      bool    ControllerReady(const int32_t ControllerIndex);
      void    ControllerConnect(const int32_t ControllerIndex, bool IsConnected);
      void    ControllerDisconnectAll();
      void    ControllerRumbleAsync(const int32_t ControllerIndex, const uint8_t BigWeight, const uint8_t SmallWeight, const uint32_t RumbleTimeMS);  

      // debug
      void printbuff(const uint8_t* buff, size_t buffsize)
      {
        for (size_t i=0; i < buffsize; i++)
          std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)buff[i] << " ";
        std::cout <<  std::dec << std::endl;
      }
  };
}

#endif //_XBOX360_