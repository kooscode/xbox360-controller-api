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

#include <string.h>
#include <stdexcept>
#include <vector>
#include <future>

#include "XBOX360.hpp"

#define CONTROLLER_BOUNDS(C) std::max(std::min(C, MAX_CONTROLLERS), 0)
#define APPLY_MASK(VALUE, MASK) ((VALUE & MASK) > 0)

XKCTRL::XBOX360::XBOX360()
  : USBDeviceThreadRunning_(false)
{
  // start with cleared controller states
  ControllerDisconnectAll();

  //start Wireless Device polling thread.
  USBDeviceThreadRunning_ = true;
  USBDeviceThread_ = std::thread(&XBOX360::USBDeviceThread, this);
}

XKCTRL::XBOX360::~XBOX360()
{
  //kill async polling
  USBDeviceThreadRunning_ = false;
  USBDeviceThread_.join();

  if (USBDeviceHandle_)
  {
    // attempt to release all interfaces
    for (auto iface : USBInterfaces_)
      libusb_release_interface(USBDeviceHandle_, iface);

    libusb_close(USBDeviceHandle_);
  }

  //close out context
  if (USBContext_)
    libusb_exit(USBContext_);
}

void XKCTRL::XBOX360::USBDeviceInit()
{
// Init USB
  int ret = libusb_init(&USBContext_);
  if (ret != LIBUSB_SUCCESS)
    throw std::runtime_error(libusb_strerror(static_cast<libusb_error>(ret)));

  // Create XBOX360 Wireless Receiver USB Device
  USBDeviceHandle_ = libusb_open_device_with_vid_pid(USBContext_, USBVendorID_, USBProductID_);
  if (!USBDeviceHandle_)
    throw std::runtime_error("Error finding XBOX360 Wireless Receiver");

  // Claim all interfaces for all controllers
  for (auto iface : USBInterfaces_)
  {
    // detach from any kernel drivers - requires sudo
    if (libusb_kernel_driver_active(USBDeviceHandle_, iface) == 0x01)
      libusb_detach_kernel_driver(USBDeviceHandle_, iface);

    // claim interface
    ret = libusb_claim_interface(USBDeviceHandle_, iface);
    if (ret != LIBUSB_SUCCESS)
      throw std::runtime_error(libusb_strerror(static_cast<libusb_error>(ret)));
  }

  //clear all controller states
  ControllerDisconnectAll();
}

int32_t XKCTRL::XBOX360::USBRXAsync(const int32_t ControllerIndex)
{
  int32_t controlleridx = CONTROLLER_BOUNDS(ControllerIndex);

  //Read Controller data
  memset(USBDataIn_[controlleridx], 0x00, MAX_USB_INBUFF);
  int32_t datasize;
  int32_t ret = libusb_interrupt_transfer(USBDeviceHandle_, USBEndpointsIn_[controlleridx], 
                                          USBDataIn_[controlleridx], MAX_USB_INBUFF, &datasize, 
                                          MAX_USB_TIMEOUT);

  return (ret == LIBUSB_SUCCESS)?controlleridx:ret;
}

void XKCTRL::XBOX360::USBDeviceThread()
{
  while (USBDeviceThreadRunning_)
  {
    // If Wireles Receiver not connected, try connect
    if (USBDeviceHandle_ == nullptr)
    {
      try
      {
        // Find and initialize XBOX360 Wireless device
        USBDeviceInit();            
      }
      catch(const std::exception& e)
      {
        // No Wireless device found.. will try again
        std::cerr << e.what() << '\n';
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
      }
    }
    else
    {
      // Wireless Device Connected, schedule parralel async data transfer for all controllers
      std::vector<std::future<int32_t>> future_transfers;
      for (uint32_t i = 0; i < MAX_CONTROLLERS; i++)
        future_transfers.push_back(std::async(std::launch::async, &XBOX360::USBRXAsync, this, i));

      //process results from all controllers
      for (auto& future_transfer : future_transfers)
      {
        int32_t ret = future_transfer.get();

        // returns controller index on success or negative value on error.
        if (ret >= 0)
        {
          //Process USB Controller data.  
          try
          {
            ControllerDataProcessing(ret);
          }
          catch(const std::exception& e)
          {
            // Error Processing Data..
            std::cerr << "ERROR Processing USB Data: " << e.what() << '\n';
          }
        }
        else if (ret == LIBUSB_ERROR_NO_DEVICE)
        {
          try
          {
            //Device was disconnected..
            std::cerr << "XBOX360 Wireless Device Disconnected!" << std::endl;

            //clear all controller states
            ControllerDisconnectAll();

            //release all interfaces
            for (auto iface : USBInterfaces_)
              libusb_release_interface(USBDeviceHandle_, iface);
            
            // release device
            libusb_close(USBDeviceHandle_);
            USBDeviceHandle_ = nullptr;
          }
          catch(const std::exception& e)
          {
            // Error Processing Disconnect..
            std::cerr << "ERROR Disconnecting: " << e.what() << '\n';
          }

          break;
        }

      } //end for controllers
    } //end else device != nullptr
  } //end while polling
}

void XKCTRL::XBOX360::ControllerDataProcessing(const int32_t ControllerIndex)
{
  int32_t controlleridx = CONTROLLER_BOUNDS(ControllerIndex);

  // *****  Check if report is Connection event *****
  if (USBDataIn_[controlleridx][0] == USBReportType::CONNECTION) 
  {
    // Check if connected or disconnected..
    if (USBDataIn_[controlleridx][1] == 0x80) 
    {
      // Connected, Initialize, Set LED's and small Rumble
      ControllerConnect(controlleridx, ControllerInit(controlleridx));
    }
    else
    {    
      //Seems like this was a disconnect msg.  
      ControllerConnect(controlleridx, false);
    }
    return;
  }// end if connection event

  // ***** Generic event for further parsing..  *****
  if (USBDataIn_[controlleridx][0] == USBReportType::DATA)
  {
    // Standard Controller event (i.e buttons pushed)
    if (USBDataIn_[controlleridx][1] == 0x01 &&  
        USBDataIn_[controlleridx][3] == 0xF0 &&  //data input
        USBDataIn_[controlleridx][5] == 0x13)    //type controller buttons
    { 

      // If controller status is not connected and we are receiving
      // data from controller, it's alive, set as connected
      ControllerConnect(controlleridx, true);
      
      { // Protect ControllerStates shared obj  
        std::lock_guard<std::mutex> guard(mutex_);
          
        // printbuff(USBDataIn_[controlleridx], MAX_USB_INBUFF);

        CONTROLLER_LAYOUT* ctrl_in = reinterpret_cast<CONTROLLER_LAYOUT*>(&USBDataIn_[controlleridx][0x06]);
        ControllerStates_[controlleridx].UP = APPLY_MASK(ctrl_in->BUTTONS, MASK_DPAD_UP);
        ControllerStates_[controlleridx].DOWN = APPLY_MASK(ctrl_in->BUTTONS, MASK_DPAD_DOWN);
        ControllerStates_[controlleridx].LEFT = APPLY_MASK(ctrl_in->BUTTONS, MASK_DPAD_LEFT);
        ControllerStates_[controlleridx].RIGHT = APPLY_MASK(ctrl_in->BUTTONS, MASK_DPAD_RIGHT);
        ControllerStates_[controlleridx].START = APPLY_MASK(ctrl_in->BUTTONS, MASK_BTN_START);
        ControllerStates_[controlleridx].BACK = APPLY_MASK(ctrl_in->BUTTONS, MASK_BTN_BACK);
        ControllerStates_[controlleridx].LH = APPLY_MASK(ctrl_in->BUTTONS, MASK_BTN_LH);
        ControllerStates_[controlleridx].RH = APPLY_MASK(ctrl_in->BUTTONS, MASK_BTN_RH);
        ControllerStates_[controlleridx].LB = APPLY_MASK(ctrl_in->BUTTONS, MASK_BTN_LB);
        ControllerStates_[controlleridx].RB = APPLY_MASK(ctrl_in->BUTTONS, MASK_BTN_RB);
        ControllerStates_[controlleridx].XBOX = APPLY_MASK(ctrl_in->BUTTONS, MASK_BTN_XBOX);
        ControllerStates_[controlleridx].A = APPLY_MASK(ctrl_in->BUTTONS, MASK_BTN_A);
        ControllerStates_[controlleridx].B = APPLY_MASK(ctrl_in->BUTTONS, MASK_BTN_B);
        ControllerStates_[controlleridx].X = APPLY_MASK(ctrl_in->BUTTONS, MASK_BTN_X);
        ControllerStates_[controlleridx].Y = APPLY_MASK(ctrl_in->BUTTONS, MASK_BTN_Y);
        // Update controller Analog states
        ControllerStates_[controlleridx].LTRIG = ctrl_in->LTRIG;
        ControllerStates_[controlleridx].RTRIG = ctrl_in->RTRIG;
        ControllerStates_[controlleridx].LSTICK_X = ctrl_in->LSTICK_X;
        ControllerStates_[controlleridx].LSTICK_Y = ctrl_in->LSTICK_Y;
        ControllerStates_[controlleridx].RSTICK_X = ctrl_in->RSTICK_X;
        ControllerStates_[controlleridx].RSTICK_Y = ctrl_in->RSTICK_Y;

        // valid data received, notify any waiting requests..
        ControllersNotify_[controlleridx].notify_one();

      }// end lock guard


    } // end if buttons pushed
  }// end if data event
}

bool XKCTRL::XBOX360::ControllerInit(const int32_t ControllerIndex)
{
  bool retval = false;
  int32_t controlleridx = CONTROLLER_BOUNDS(ControllerIndex);
  SetLED(controlleridx, LED_SETTING::OFF_ALL);
  retval = ControllerReady(controlleridx);
  return retval;
}

bool XKCTRL::XBOX360::USBTXSync(const int32_t ControllerIndex)
{
  bool retval = false;
  int32_t controlleridx = CONTROLLER_BOUNDS(ControllerIndex);
  if (USBDeviceHandle_ != nullptr)
  {
    int32_t datalen = 0x00;
    int32_t res = libusb_interrupt_transfer(USBDeviceHandle_, USBEndpointsOut_[controlleridx], 
                              USBDataOut_[controlleridx], MAX_USB_OUTBUFF, 
                              &datalen, MAX_USB_TIMEOUT);
    retval = (res == LIBUSB_SUCCESS);
  }
  return retval;
}

bool XKCTRL::XBOX360::ControllerReady(const int32_t ControllerIndex)
{
  //syncronize access to resources for entire function call
  std::lock_guard<std::mutex> guard(mutex_);

  // Transfer Controller LED Setting
  int32_t controlleridx = CONTROLLER_BOUNDS(ControllerIndex);
  memset(USBDataOut_[controlleridx], 0x00, MAX_USB_OUTBUFF);
  USBDataOut_[controlleridx][2] = 0x02;
  USBDataOut_[controlleridx][3] = 0x80;
  return USBTXSync(controlleridx);
}

void XKCTRL::XBOX360::ControllerConnect(const int32_t ControllerIndex, bool IsConnected) 
{
  int32_t controlleridx = CONTROLLER_BOUNDS(ControllerIndex);
  bool AlreadyConnected = false;
  
  {// Protect ControllerStates shared obj  
    std::lock_guard<std::mutex> guard(mutex_);
    AlreadyConnected = ControllerStates_[controlleridx].CONNECTED;
    ControllerStates_[controlleridx].CONNECTED = IsConnected;
  }

  // small buzz on connect..
  if (!AlreadyConnected && IsConnected)
  {
    SetLED(controlleridx, static_cast<LED_SETTING>(LED_SETTING::BLINK_1_ON + ControllerIndex));
    SetRumbleTimed(controlleridx, 0x00, 0xFF, 250);
  }
}

void XKCTRL::XBOX360::ControllerDisconnectAll()
{
  //syncronize access to resources for entire function call
  std::lock_guard<std::mutex> guard(mutex_);

  //Clear all controllers state
  for (uint32_t i = 0; i < MAX_CONTROLLERS; i++)
    memset(&ControllerStates_[i], 0x00, sizeof(CONTROLLER_STATE));
}

void XKCTRL::XBOX360::SetLED(const int32_t ControllerIndex, const XKCTRL::LED_SETTING LEDSetting)
{
  //syncronize access to resources for entire function call
  std::lock_guard<std::mutex> guard(mutex_);

  // Transfer Controller LED Setting
  int32_t controlleridx = CONTROLLER_BOUNDS(ControllerIndex);
  memset(USBDataOut_[controlleridx], 0x00, MAX_USB_OUTBUFF);
  USBDataOut_[controlleridx][2] = 0x08;
  USBDataOut_[controlleridx][3] = 0x40|static_cast<uint8_t>(LEDSetting);
  USBTXSync(controlleridx);
}

void XKCTRL::XBOX360::SetRumble(const int32_t ControllerIndex, const uint8_t BigWeight, const uint8_t SmallWeight)
{
  //syncronize access to resources for entire function call
  std::lock_guard<std::mutex> guard(mutex_);

  int32_t controlleridx = CONTROLLER_BOUNDS(ControllerIndex);
  memset(USBDataOut_[controlleridx], 0x00, MAX_USB_OUTBUFF);
  USBDataOut_[controlleridx][1] = 0x01;
  USBDataOut_[controlleridx][2] = 0x0f;
  USBDataOut_[controlleridx][3] = 0xc0;
  USBDataOut_[controlleridx][5] = BigWeight; 
  USBDataOut_[controlleridx][6] = SmallWeight; 
  USBTXSync(controlleridx);
}

void XKCTRL::XBOX360::ControllerRumbleAsync(const int32_t ControllerIndex, const uint8_t BigWeight, const uint8_t SmallWeight, const uint32_t RumbleTimeMS)
{
  SetRumble(ControllerIndex, BigWeight, SmallWeight);
  std::this_thread::sleep_for(std::chrono::milliseconds(RumbleTimeMS));
  SetRumble(ControllerIndex, 0x00, 0x00);
}

void XKCTRL::XBOX360::SetRumbleTimed(const int32_t ControllerIndex, const uint8_t BigWeight, const uint8_t SmallWeight, const uint32_t RumbleTimeMS)
{
  // kick off a timed rumbler via async thread.
  std::thread rumble_task(&XBOX360::ControllerRumbleAsync, this, ControllerIndex, BigWeight, SmallWeight, RumbleTimeMS);
  rumble_task.detach();
}

void XKCTRL::XBOX360::GetControllerState(const int32_t ControllerIndex, XKCTRL::CONTROLLER_STATE& ControllerState)
{
  //syncronize access to resources for entire function call
  std::lock_guard<std::mutex> guard(mutex_);
  int32_t controlleridx = CONTROLLER_BOUNDS(ControllerIndex);
  ControllerState = ControllerStates_[controlleridx];
}

bool  XKCTRL::XBOX360::GetWaitControllerState(const int32_t ControllerIndex,  XKCTRL::CONTROLLER_STATE& ControllerState, uint32_t TimeoutMS)
{
  // syncronize access to resources for entire function call
  // use unique lock so the condition_variable checking can lock/relock..
  std::unique_lock<std::mutex> lock(mutex_);

  // wait for controller data changes notification to get latest changed values
  // if noting received after timout, return false and populate current stale values.
  // waits for condition_variable notify OR timeout.
  int32_t controlleridx = CONTROLLER_BOUNDS(ControllerIndex);
  auto notified = ControllersNotify_[controlleridx].wait_for(lock, std::chrono::milliseconds(TimeoutMS));
  ControllerState = ControllerStates_[controlleridx];
  return (notified == std::cv_status::no_timeout);
}
